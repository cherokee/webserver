#!/usr/bin/env python3
"""Reproducible FastCGI transport benchmark, with a native backend and clients.

Each sample serves exactly --requests validated HTTP responses. A separate
warmup precedes measurement; nothing runs concurrently with another sample.
Old, new/reuse-off and new/reuse-on orders rotate between repeats.
"""
import argparse
from datetime import datetime, timezone
import hashlib
import json
import os
from pathlib import Path
import platform
import select
import socket
import statistics
import subprocess
import tempfile
import time

ROOT = Path(__file__).resolve().parents[1]
MODES = ('old', 'new_off', 'new_on')


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def save(document, path):
    temporary = path.with_suffix(path.suffix + '.tmp')
    temporary.write_text(json.dumps(document, indent=2) + '\n')
    temporary.replace(path)


def cpu_time(process):
    # Linux proc stat fields 14/15: aggregate process user/system ticks.
    fields = Path(f'/proc/{process.pid}/stat').read_text().rsplit(')', 1)[1].split()
    return (int(fields[11]) + int(fields[12])) / os.sysconf('SC_CLK_TCK')


def read_json(process, timeout=15):
    # Pipes are deliberately unbuffered; select observes the actual pipe.
    if not select.select([process.stdout], [], [], timeout)[0]:
        raise RuntimeError('Backend statistics/startup timeout')
    line = process.stdout.readline()
    if not line:
        raise RuntimeError(f'Backend exited: {process.stderr.read().decode()}')
    return json.loads(line)


def backend_stats(process):
    process.stdin.write(b'stats\n')
    return read_json(process)


def stop(process):
    if process is None:
        return
    if process.poll() is None:
        process.terminate()
        try:
            process.wait(5)
        except subprocess.TimeoutExpired:
            process.kill()
            process.wait()


def measured_load(helper, port, requests, concurrency, cpus):
    command = [str(helper), 'load', str(port), str(requests), str(concurrency)]
    if cpus:
        command = ['taskset', '-c', ','.join(map(str, cpus)), *command]
    result = subprocess.run(command, stdout=subprocess.PIPE, timeout=max(180, requests / 1000 + 60))
    if result.returncode:
        raise RuntimeError(f'Load generator exited with {result.returncode}; diagnostics appear above')
    value = json.loads(result.stdout)
    if value['requests'] != requests or value['errors'] or value['frontend_connections'] != concurrency:
        raise RuntimeError(f'Invalid sample: {value}')
    return value


def sample(args, helper, transport, concurrency, mode, repeat):
    tree = args.old_tree if mode == 'old' else args.new_tree
    backend = server = None
    with tempfile.TemporaryDirectory(prefix='cherokee-source-benchmark-') as directory:
        directory = Path(directory)
        with (directory / 'server.log').open('w+') as log:
            try:
                command = [str(helper), 'backend', transport, str(directory / 'fcgi.sock')]
                if args.backend_cpus:
                    command = ['taskset', '-c', ','.join(map(str, args.backend_cpus)), *command]
                backend = subprocess.Popen(command, stdin=subprocess.PIPE, stdout=subprocess.PIPE,
                                           stderr=subprocess.PIPE, bufsize=0)
                address = read_json(backend)['address']
                with socket.socket() as listener:
                    listener.bind(('127.0.0.1', 0))
                    port = listener.getsockname()[1]
                config = f'''server!bind!1!port = {port}
server!bind!1!interface = 127.0.0.1
server!ipv6 = 0
server!thread_number = 4
server!timeout = 60
server!keepalive = 1
server!keepalive_max_requests = {args.requests + args.warmup + 100}
server!pid_file = {directory}/pid
server!module_dir = {tree}/cherokee/.libs
server!module_deps = {tree}/cherokee
vserver!1!nick = default
vserver!1!document_root = {directory}
vserver!1!rule!1!match = default
vserver!1!rule!1!handler = fcgi
vserver!1!rule!1!handler!check_file = 0
vserver!1!rule!1!handler!balancer = round_robin
vserver!1!rule!1!handler!balancer!source!1 = 1
source!1!type = host
source!1!host = {address}
'''
                if mode != 'old':
                    config += f'''source!1!keepalive = {int(mode == 'new_on')}
source!1!keepalive_max = 4
source!1!keepalive_timeout = 60
source!1!keepalive_age = 86400
source!1!keepalive_requests = 65535
source!1!connect_timeout = 0
source!1!io_timeout = 0
source!1!retries = 0
'''
                config_file = directory / 'cherokee.conf'
                config_file.write_text(config)
                env = dict(os.environ, LD_LIBRARY_PATH=str(tree / 'cherokee/.libs'))
                server_command = [str(tree / 'cherokee/.libs/cherokee-worker'), '-C', str(config_file)]
                if args.server_cpus:
                    server_command = ['taskset', '-c', ','.join(map(str, args.server_cpus)), *server_command]
                server = subprocess.Popen(server_command, stdout=log, stderr=log, env=env)
                for _ in range(200):
                    if server.poll() is not None:
                        raise RuntimeError('Cherokee exited before startup')
                    try:
                        with socket.create_connection(('127.0.0.1', port), .1):
                            break
                    except OSError:
                        time.sleep(.02)
                else:
                    raise RuntimeError('Cherokee startup timeout')
                measured_load(helper, port, args.warmup, concurrency, args.client_cpus)
                before = backend_stats(backend)
                server_cpu = cpu_time(server)
                load_before = Path('/proc/loadavg').read_text().strip()
                result = measured_load(helper, port, args.requests, concurrency, args.client_cpus)
                result['server_cpu_seconds'] = cpu_time(server) - server_cpu
                after = backend_stats(backend)
                result.update(transport=transport, mode=mode, repeat=repeat,
                              backend_connections=after['accepted'] - before['accepted'],
                              backend_requests=after['requests'] - before['requests'],
                              backend_errors=after['errors'] - before['errors'],
                              backend_cpu_seconds=after['cpu_seconds'] - before['cpu_seconds'],
                              backend_connections_after_warmup=before['accepted'],
                              backend_live_after_measurement=after['live'],
                              load_before=load_before, load_after=Path('/proc/loadavg').read_text().strip())
                if result['backend_requests'] != args.requests or result['backend_errors']:
                    raise RuntimeError(f'Invalid backend counts: {result}')
                if mode != 'new_on' and result['backend_connections'] != args.requests:
                    raise RuntimeError(f'Legacy mode reused a backend connection: {result}')
                if mode == 'new_on':
                    minimum = max(1, (args.requests + 65534) // 65535)
                    maximum = (args.requests + args.warmup + 65534) // 65535 + 2 * concurrency
                    valid_connections = minimum <= result['backend_connections_used'] <= maximum
                else:
                    valid_connections = result['backend_connections_used'] == args.requests
                if not valid_connections:
                    raise RuntimeError(f'Unexpected measured connection identities: {result}')
                stop(server)
                if server.returncode != 0:
                    raise RuntimeError(f'Cherokee shutdown status {server.returncode}')
                log.seek(0)
                server_log = log.read()
                if any(marker in server_log for marker in ('ERROR: AddressSanitizer', 'runtime error:', 'CRITICAL')):
                    raise RuntimeError(server_log)
                return result
            except Exception:
                log.seek(0)
                print(log.read(), flush=True)
                raise
            finally:
                stop(server)
                stop(backend)


def summary(samples):
    groups = {}
    for sample in samples:
        groups.setdefault((sample['transport'], sample['concurrency'], sample['mode']), []).append(sample)
    results = []
    for (transport, concurrency, mode), group in groups.items():
        result = dict(transport=transport, concurrency=concurrency, mode=mode, samples=len(group))
        for field in ('seconds', 'requests_per_second', 'latency_mean_ms', 'latency_p50_ms',
                      'latency_p95_ms', 'latency_p99_ms', 'server_cpu_seconds', 'backend_cpu_seconds',
                      'client_cpu_seconds', 'backend_connections', 'backend_connections_used', 'backend_connections_after_warmup'):
            values = [sample[field] for sample in group]
            result[field] = {'median': statistics.median(values), 'min': min(values), 'max': max(values)}
        results.append(result)
    return results


def build_metadata(tree):
    binary = tree / 'cherokee/.libs/cherokee-worker'
    fields = {}
    for name in ('config.status', 'config.h'):
        fields[name + '_sha256'] = digest(tree / name)
    for line in (tree / 'config.status').read_text().splitlines():
        if line.startswith(('CFLAGS=', 'ac_cs_config=')):
            fields[line.split('=', 1)[0]] = line.split('=', 1)[1]
    fields.update(tree=str(tree), worker_sha256=digest(binary),
                  source_sha256={name: digest(tree / 'cherokee' / name) for name in
                                 ('source.c', 'handler_fcgi.c', 'thread.c', 'socket.c', 'util.c')},
                  library_sha256={path.name: digest(path) for path in (tree / 'cherokee/.libs').glob('*.so')})
    return fields


def parse_cpus(value):
    return [int(cpu) for cpu in value.split(',')] if value else []


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--old-tree', type=Path, required=True)
    parser.add_argument('--new-tree', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--requests', type=int, default=10000)
    parser.add_argument('--warmup', type=int, default=1000)
    parser.add_argument('--repeat', type=int, default=3)
    parser.add_argument('--concurrency', type=int, nargs='+', default=[1, 4])
    parser.add_argument('--transport', choices=('tcp', 'unix'), nargs='+', default=['tcp', 'unix'])
    parser.add_argument('--modes', choices=MODES, nargs='+', default=list(MODES))
    parser.add_argument('--server-cpus', type=parse_cpus, default=[])
    parser.add_argument('--backend-cpus', type=parse_cpus, default=[])
    parser.add_argument('--client-cpus', type=parse_cpus, default=[])
    args = parser.parse_args()
    args.old_tree = args.old_tree.resolve(); args.new_tree = args.new_tree.resolve()
    if args.repeat < 1 or args.requests < 1 or args.warmup < 1 or any(c < 1 or c > 64 or args.requests % c or args.warmup % c for c in args.concurrency):
        parser.error('Positive counts divisible by concurrency (1-64) are required')
    args.output.parent.mkdir(parents=True, exist_ok=True)
    allowed = os.sched_getaffinity(0)
    if any(cpu not in allowed for group in (args.server_cpus, args.backend_cpus, args.client_cpus) for cpu in group):
        parser.error('CPU affinity must use CPUs available to this process')
    cpu_model = next((line.split(':', 1)[1].strip() for line in Path('/proc/cpuinfo').read_text().splitlines()
                      if line.startswith('model name')), 'unknown')
    document = dict(started_at=datetime.now(timezone.utc).isoformat(), cpu=cpu_model,
                    platform=platform.platform(), compiler=subprocess.check_output(['cc', '--version'], text=True).splitlines()[0],
                    arguments={name: str(value) if isinstance(value, Path) else value for name, value in vars(args).items()},
                    helper_source_sha256=digest(ROOT / 'qa/source_reuse_bench.c'),
                    old_build=build_metadata(args.old_tree), new_build=build_metadata(args.new_tree),
                    settings=dict(server_workers=4, frontend_http='HTTP/1.1', frontend_keepalive=True,
                                  backend='native event-driven FastCGI fixture', response_bytes=128,
                                  php_fpm=False, latency_clock='CLOCK_MONOTONIC',
                                  pool_idle_max=4, pool_requests=65535, pool_age_seconds=86400,
                                  pool_idle_seconds=60, source_deadlines=0, retries=0), samples=[])
    with tempfile.TemporaryDirectory(prefix='cherokee-source-bench-helper-') as directory:
        helper = Path(directory) / 'helper'
        subprocess.run(['cc', '-O2', '-Wall', '-Wextra', '-Werror', '-std=c99',
                        '-o', str(helper), str(ROOT / 'qa/source_reuse_bench.c')], check=True)
        for repeat in range(args.repeat):
            modes = tuple(args.modes)
            order = modes[repeat % len(modes):] + modes[:repeat % len(modes)]
            for transport in args.transport:
                for concurrency in args.concurrency:
                    for mode in order:
                        print(f'{transport} c={concurrency} {mode} repeat={repeat+1}', flush=True)
                        result = sample(args, helper, transport, concurrency, mode, repeat+1)
                        document['samples'].append(result)
                        document['summary'] = summary(document['samples'])
                        save(document, args.output)
                        print(f"  {result['requests_per_second']:.0f} req/s, {result['seconds']:.3f}s, "
                              f"p95 {result['latency_p95_ms']:.3f}ms, {result['backend_connections_used']} distinct backend sockets", flush=True)
    document['finished_at'] = datetime.now(timezone.utc).isoformat()
    save(document, args.output)


if __name__ == '__main__':
    main()
