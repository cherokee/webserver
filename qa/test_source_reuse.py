#!/usr/bin/env python3
"""Source pooling QA with real TCP/Unix backends; stdlib only.

Run after building: python3 qa/test_source_reuse.py
CHEROKEE_TEST_THREADS=1 exercises the single worker path.
"""
import concurrent.futures
import contextlib
import http.client
import json
import os
from pathlib import Path
import socket
import struct
import subprocess
import tempfile
import threading
import time
import unittest
from urllib.parse import parse_qs, urlencode, urlsplit

ROOT = Path(__file__).resolve().parents[1]


def record(kind, ident, data=b'', padding=0):
    return struct.pack('!BBHHBB', 1, kind, ident, len(data), padding, 0) + data + b'\0' * padding


def decode_params(data):
    result = {}
    offset = 0
    def length():
        nonlocal offset
        value = data[offset]
        if value & 128:
            value = struct.unpack_from('!I', data, offset)[0] & 0x7fffffff
            offset += 4
        else:
            offset += 1
        return value
    while offset < len(data):
        n, v = length(), length()
        key = data[offset:offset+n].decode()
        offset += n
        result[key] = data[offset:offset+v].decode()
        offset += v
    return result


class Backend:
    def __init__(self, protocol, path=None):
        self.protocol = protocol
        self.listener = socket.socket(socket.AF_UNIX if path else socket.AF_INET)
        self.listener.bind(str(path) if path else ('127.0.0.1', 0))
        self.address = str(path) if path else '127.0.0.1:%d' % self.listener.getsockname()[1]
        self.listener.listen(100)
        self.listener.settimeout(.2)
        self.lock = threading.Lock()
        self.accepted = 0
        self.open = 0
        self.attempts = {}
        self.requests = []
        self.clients = set()
        self.stopped = threading.Event()
        self.thread = threading.Thread(target=self.accept, daemon=True)
        self.thread.start()

    def accept(self):
        while not self.stopped.is_set():
            try:
                client, _ = self.listener.accept()
            except socket.timeout:
                continue
            except OSError:
                return
            with self.lock:
                self.accepted += 1
                self.open += 1
                ident = self.accepted
                self.clients.add(client)
            threading.Thread(target=self.serve, args=(client, ident), daemon=True).start()

    def response(self, path, headers, body, conn_id, request_id, flags):
        query = parse_qs(urlsplit(path).query)
        token = query.get('token', [''])[0]
        mode = query.get('mode', [''])[0]
        with self.lock:
            attempt = self.attempts[token] = self.attempts.get(token, 0) + 1
            self.requests.append((token, conn_id, request_id, flags, body))
        if mode == 'pause':
            self.stopped.wait(2.1)
        if mode == 'hang' or (mode == 'hang_once' and attempt == 1):
            self.stopped.wait(3.5)
        if mode == 'close' or (mode == 'close_once' and attempt == 1):
            return None, mode
        payload = json.dumps({'token': token, 'backend': self.address, 'connection': conn_id, 'request_id': request_id,
                              'cookie': headers.get('cookie', ''), 'authorization': headers.get('authorization', ''),
                              'body': body.decode(), 'attempt': attempt}).encode()
        return payload, mode

    def serve(self, client, conn_id):
        client.settimeout(15)
        try:
            with client, client.makefile('rb') as stream:
                while not self.stopped.is_set():
                    if self.protocol == 'fcgi':
                        params, body, flags = b'', b'', 0
                        while True:
                            head = stream.read(8)
                            if not head:
                                return
                            if len(head) != 8:
                                return
                            _, kind, ident, size, pad, _ = struct.unpack('!BBHHBB', head)
                            data = stream.read(size)
                            stream.read(pad)
                            if kind == 1:
                                flags = data[2]
                            elif kind == 4:
                                params += data
                            elif kind == 5:
                                body += data
                                if not data:
                                    break
                        env = decode_params(params)
                        headers = {k[5:].lower().replace('_', '-'): v for k, v in env.items() if k.startswith('HTTP_')}
                        path = env.get('REQUEST_URI', '')
                        payload, mode = self.response(path, headers, body, conn_id, ident, flags)
                        if payload is None:
                            return
                        response = b'Content-Type: application/json\r\nContent-Length: ' + str(len(payload)).encode() + b'\r\n\r\n' + payload
                        if mode == 'application_504':
                            response = b'Status: 504 Gateway Timeout\r\n' + response
                        if mode == 'wrong_id':
                            ident += 1
                        output = b''.join(record(6, ident, response[offset:offset+60000], 5) for offset in range(0, len(response), 60000)) + record(6, ident)
                        if mode == 'stderr':
                            output += record(7, ident, b'not terminated\x00private diagnostic', 7) + record(7, ident)
                        end = b'\0' * 8
                        if mode == 'bad_end':
                            end = b'\0'
                        if mode == 'overload':
                            end = b'\0' * 4 + b'\x02' + b'\0' * 3
                        if mode != 'missing_end':
                            output += record(3, ident, end, 3)
                        if mode == 'extra':
                            output += record(6, ident+1, b'LEAK FROM PREVIOUS REQUEST')
                        if mode == 'partial_hang':
                            header, _, _ = response.partition(b'\r\n\r\n')
                            client.sendall(record(6, ident, header+b'\r\n\r\n'+payload[:10]))
                            self.stopped.wait(3.5)
                            return
                        if mode == 'slow':
                            count = max(1, len(output)//5)
                            for offset in range(0, len(output), count):
                                client.sendall(output[offset:offset+count])
                                self.stopped.wait(.65)
                        elif mode == 'split':
                            for offset in range(0, len(output), 3):
                                client.sendall(output[offset:offset+3])
                        else:
                            client.sendall(output)
                        if not flags & 1 or mode == 'idle_close':
                            return
                    elif self.protocol == 'http':
                        line = stream.readline()
                        if not line:
                            return
                        method, path, _ = line.decode().strip().split(' ')
                        headers = {}
                        while True:
                            line = stream.readline()
                            if line == b'\r\n':
                                break
                            key, value = line.decode().split(':', 1)
                            headers[key.lower()] = value.strip()
                        body = stream.read(int(headers.get('content-length', '0')))
                        payload, mode = self.response(path, headers, body, conn_id, 0, 0)
                        if payload is None:
                            return
                        framing = b'Content-Length: ' + str(len(payload)).encode() + b'\r\n'
                        if mode == 'duplicate_length':
                            framing *= 2
                        if mode in ('chunked', 'chunked_extra'):
                            framing = b'Transfer-Encoding: chunked\r\n'
                            payload = b'%x;test=1\r\n' % len(payload) + payload + b'\r\n0\r\nX-Trailer: discarded\r\n\r\n'
                        if mode in ('extra', 'chunked_extra'):
                            payload += b'HTTP/1.1 200 OK\r\nContent-Length: 4\r\n\r\nLEAK'
                        output = b'HTTP/1.1 200 OK\r\nContent-Type: application/json\r\n' + framing + b'Connection: keep-alive\r\n\r\n'
                        if mode == 'application_504':
                            output = output.replace(b'200 OK', b'504 Gateway Timeout', 1)
                        if mode == 'auth_challenge':
                            output = output.replace(b'Content-Type:', b'WWW-Authenticate: Basic realm=app, Negotiate\r\nContent-Type:')
                        if mode == 'close_tokens':
                            output = output.replace(b'Connection: keep-alive', b'Connection: keep-alive, close')
                        if method != 'HEAD':
                            output += payload
                        if mode == 'partial_hang':
                            client.sendall(output[:-len(payload)] + payload[:10])
                            self.stopped.wait(3.5)
                            return
                        if mode == 'slow':
                            count = max(1, len(output)//5)
                            for offset in range(0, len(output), count):
                                client.sendall(output[offset:offset+count])
                                self.stopped.wait(.65)
                        else:
                            client.sendall(output)
                        if headers.get('connection', '').lower() == 'close' or mode == 'idle_close':
                            return
                    else:
                        if self.protocol == 'scgi':
                            digits = b''
                            while True:
                                byte = stream.read(1)
                                if not byte: return
                                if byte == b':': break
                                digits += byte
                            fields = stream.read(int(digits)).split(b'\0')[:-1]
                            stream.read(1)
                            env = dict(zip((x.decode() for x in fields[::2]), (x.decode() for x in fields[1::2])))
                        else:
                            head = stream.read(4)
                            if len(head) != 4: return
                            data = stream.read(struct.unpack('<H', head[1:3])[0])
                            env, offset = {}, 0
                            while offset < len(data):
                                size = struct.unpack_from('<H', data, offset)[0]; offset += 2
                                key = data[offset:offset+size].decode(); offset += size
                                size = struct.unpack_from('<H', data, offset)[0]; offset += 2
                                env[key] = data[offset:offset+size].decode(); offset += size
                        body = stream.read(int(env.get('CONTENT_LENGTH', '0')))
                        headers = {k[5:].lower().replace('_', '-'): v for k,v in env.items() if k.startswith('HTTP_')}
                        payload, mode = self.response(env.get('REQUEST_URI', ''), headers, body, conn_id, 0, 0)
                        if payload is None: return
                        client.sendall(b'Content-Type: application/json\r\nContent-Length: ' + str(len(payload)).encode() + b'\r\n\r\n' + payload)
                        return
        except (OSError, ValueError):
            pass
        finally:
            with self.lock:
                self.open -= 1
                self.clients.discard(client)

    def close(self):
        self.stopped.set()
        self.listener.close()
        with self.lock:
            clients = list(self.clients)
        for client in clients:
            with contextlib.suppress(OSError):
                client.shutdown(socket.SHUT_RDWR)
        self.thread.join(2)


class SourceReuseTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory(prefix='cherokee-source-qa-')
        self.directory = Path(self.temp.name)
        self.backends = [Backend('fcgi'), Backend('http'), Backend('fcgi', self.directory / 'fcgi.sock')]
        self.process = None
        self.log = open(self.directory / 'server.log', 'w+')

    def start(self, settings=None, balancer=None, eof_adapters=False, server_timeout=10):
        with socket.socket() as sock:
            sock.bind(('127.0.0.1', 0))
            self.port = sock.getsockname()[1]
        config = f'''server!bind!1!port = {self.port}
server!bind!1!interface = 127.0.0.1
server!ipv6 = 0
server!thread_number = {os.getenv('CHEROKEE_TEST_THREADS', '4')}
server!timeout = {server_timeout}
server!pid_file = {self.directory}/pid
server!module_dir = {ROOT}/cherokee/.libs
server!module_deps = {ROOT}/cherokee
vserver!1!nick = default
vserver!1!document_root = {self.directory}
vserver!1!rule!1!match = default
vserver!1!rule!1!handler = file
'''
        options = {'keepalive': 1, 'keepalive_max': 4, 'keepalive_timeout': 2,
                   'keepalive_requests': 100, 'keepalive_age': 20, 'connect_timeout': 1,
                   'io_timeout': 1, 'retries': 1}
        options.update(settings or {})
        prefixes, handlers = ['fcgi', 'http', 'unix'], ['fcgi', 'proxy', 'fcgi']
        if eof_adapters:
            self.backends += [Backend('scgi'), Backend('uwsgi')]
            prefixes += ['scgi', 'uwsgi']; handlers += ['scgi', 'uwsgi']
        for number, (backend, prefix, handler) in enumerate(zip(self.backends, prefixes, handlers), 1):
            config += f'source!{number}!type = host\nsource!{number}!host = {backend.address}\n'
            for key, value in options.items():
                config += f'source!{number}!{key} = {value}\n'
            rule = f'vserver!1!rule!{number+10}'
            config += f'''{rule}!match = directory
{rule}!match!directory = /{prefix}
{rule}!handler = {handler}
{rule}!handler!balancer = round_robin
{rule}!handler!balancer!source!1 = {number}
'''
            if handler == 'fcgi':
                config += f'{rule}!handler!check_file = 0\n'
        if balancer:
            for number in (11, 13):
                config += f'vserver!1!rule!{number}!handler!balancer = {balancer}\n'
                config += f'vserver!1!rule!{number}!handler!balancer!source!1 = 1\n'
                config += f'vserver!1!rule!{number}!handler!balancer!source!2 = 3\n'
        self.config = self.directory / 'cherokee.conf'
        self.config.write_text(config)
        self.process = subprocess.Popen([str(ROOT / 'cherokee/cherokee-worker'), '-C', str(self.config)], stdout=self.log, stderr=self.log)
        for _ in range(100):
            if self.process.poll() is not None:
                self.log.seek(0)
                raise RuntimeError(self.log.read())
            try:
                with socket.create_connection(('127.0.0.1', self.port), .1):
                    return
            except OSError:
                time.sleep(.03)
        raise RuntimeError('Server startup timeout')

    def request(self, protocol, token, mode='', method='GET', body=None, headers=None, address=None):
        client = http.client.HTTPConnection('127.0.0.1', self.port, timeout=12)
        try:
            if address:
                client.source_address = (address, 0)
            client.request(method, '/' + protocol + '/test?' + urlencode({'token': token, 'mode': mode}), body=body, headers=headers or {})
            response = client.getresponse()
            data = response.read()
            return response.status, data
        finally:
            client.close()

    def value(self, protocol, token, **kwargs):
        status, data = self.request(protocol, token, **kwargs)
        self.assertEqual(status, 200, (protocol, token, data))
        value = json.loads(data)
        self.assertEqual(value['token'], token)
        return value

    def tearDown(self):
        crashed = self.process and self.process.poll() is not None
        if self.process:
            self.process.terminate()
            try:
                self.process.wait(5)
            except subprocess.TimeoutExpired:
                self.process.kill()
                self.process.wait()
        for backend in self.backends:
            backend.close()
        self.log.seek(0)
        log = self.log.read()
        self.log.close()
        self.temp.cleanup()
        if crashed or any(text in log for text in ('ERROR: AddressSanitizer', 'ERROR: LeakSanitizer', 'runtime error:')):
            raise AssertionError(log)

    def test_final_failure_is_reported_to_failover_balancer(self):
        self.start(balancer='failover')
        status, _ = self.request('fcgi', 'failover-hang', mode='hang')
        self.assertEqual(status, 504)
        failed = next(backend for backend in self.backends if 'failover-hang' in backend.attempts)
        self.assertEqual(failed.attempts['failover-hang'], 2)
        value = self.value('fcgi', 'after-failover')
        self.assertNotEqual(value['backend'], failed.address)

    def test_application_504_does_not_disable_a_healthy_source(self):
        self.start(balancer='failover')
        for protocol in ('fcgi', 'http', 'unix'):
            status, data = self.request(protocol, protocol+'-application-timeout', mode='application_504')
            self.assertEqual(status, 504)
            first = json.loads(data)
            second = self.value(protocol, protocol+'-after-application-timeout')
            self.assertEqual(first['backend'], second['backend'])
            self.assertEqual(first['connection'], second['connection'])

    def test_round_robin_selection_before_reuse(self):
        self.start(balancer='round_robin')
        values = [self.value('fcgi', f'round-robin-{i}') for i in range(12)]
        selected = [value['backend'] for value in values]
        self.assertEqual(len(set(selected)), 2)
        self.assertEqual(selected[::2], [selected[0]] * 6)
        self.assertEqual(selected[1::2], [selected[1]] * 6)
        self.assertEqual([self.backends[i].accepted for i in (0, 2)], [1, 1])

    def test_ip_hash_affinity_before_reuse(self):
        self.start(balancer='ip_hash')
        selections = []
        for address in ('127.0.0.1', '127.0.0.2'):
            values = [self.value('fcgi', f'hash-{address}-{i}', address=address) for i in range(10)]
            self.assertEqual(len({value['backend'] for value in values}), 1)
            selections.append(values[0]['backend'])
            self.value('fcgi', f'hash-retry-{address}', mode='close_once', address=address)
        self.assertNotEqual(selections[0], selections[1])
        for backend in (self.backends[0], self.backends[2]):
            self.assertEqual(len(backend.requests), 12)

    def test_eof_adapters_timeout_without_reusing_transport(self):
        self.start(eof_adapters=True)
        for protocol in ('scgi', 'uwsgi'):
            first = self.value(protocol, protocol+'-first', method='POST', body='one')
            second = self.value(protocol, protocol+'-second')
            self.assertEqual(first['body'], 'one')
            self.assertNotEqual(first['connection'], second['connection'])
            status, _ = self.request(protocol, protocol+'-hang', mode='hang')
            self.assertEqual(status, 504)
            self.value(protocol, protocol+'-after-timeout')

    def test_age_retires_busy_connection_after_completion(self):
        self.start({'keepalive_age': 1, 'keepalive_timeout': 10})
        for protocol in ('fcgi', 'http', 'unix'):
            first = self.value(protocol, protocol+'-age-first')
            time.sleep(1.1)
            second = self.value(protocol, protocol+'-age-second')
            self.assertNotEqual(first['connection'], second['connection'])

    def test_cancelled_upload_is_not_reused(self):
        self.start()
        for protocol in ('fcgi', 'http', 'unix'):
            with socket.create_connection(('127.0.0.1', self.port)) as client:
                client.sendall(f'POST /{protocol}/test?token=cancel HTTP/1.1\r\nHost: localhost\r\nContent-Length: 10000\r\n\r\nprivate prefix'.encode())
            value = self.value(protocol, protocol+'-after-cancel')
            self.assertEqual(value['body'], '')
            self.assertEqual(value['cookie'], '')

    def test_invalid_source_configuration_is_rejected(self):
        self.start()
        self.process.terminate(); self.process.wait(5); self.process = None
        original = self.config.read_text()
        for key, value in (('keepalive', '2'), ('keepalive_max', '0'), ('retries', '4'),
                           ('io_timeout', '-1'), ('keepalive_requests', '4294967297')):
            self.config.write_text(original + f'source!1!{key} = {value}\n')
            result = subprocess.run([str(ROOT / 'cherokee/cherokee-worker'), '-C', str(self.config)],
                                    capture_output=True, timeout=5)
            self.assertGreater(result.returncode, 0, (key, value, result.stderr))
            diagnostics = result.stdout + result.stderr
            for error in (b'ERROR: AddressSanitizer', b'ERROR: LeakSanitizer', b'runtime error:'):
                self.assertNotIn(error, diagnostics)

    def test_source_timeout_can_exceed_frontend_timeout(self):
        self.start({'io_timeout': 4}, server_timeout=1)
        for protocol in ('fcgi', 'http', 'unix'):
            self.value(protocol, protocol+'-pause', mode='pause')

    def test_backend_progress_extends_inactivity_deadline(self):
        self.start({'io_timeout': 2})
        for protocol in ('fcgi', 'http', 'unix'):
            started = time.monotonic()
            self.value(protocol, protocol+'-slow', mode='slow')
            self.assertGreater(time.monotonic()-started, 2)
            self.value(protocol, protocol+'-after-slow')

    def test_partial_response_timeout_closes_without_replay(self):
        self.start()
        for protocol, backend in zip(('fcgi', 'http', 'unix'), self.backends):
            token = protocol+'-partial-hang'
            with self.assertRaises((http.client.IncompleteRead, ConnectionResetError)):
                self.request(protocol, token, mode='partial_hang')
            self.assertEqual(backend.attempts[token], 1)
            self.value(protocol, protocol+'-after-partial')

    def test_retries_can_be_disabled(self):
        self.start({'retries': 0})
        for protocol, backend in zip(('fcgi', 'http', 'unix'), self.backends):
            token = protocol+'-no-retries'
            status, _ = self.request(protocol, token, mode='hang')
            self.assertEqual(status, 504)
            self.assertEqual(backend.attempts[token], 1)

    def test_connection_authentication_is_not_shared(self):
        self.start()
        for scheme in ('NTLM', 'Negotiate'):
            first = self.value('http', scheme, headers={'Authorization': scheme+' private-token'})
            second = self.value('http', 'after-'+scheme)
            self.assertNotEqual(first['connection'], second['connection'])
            self.assertEqual(second['authorization'], '')
        first = self.value('http', 'challenge', mode='auth_challenge')
        second = self.value('http', 'after-challenge')
        self.assertNotEqual(first['connection'], second['connection'])

    def test_large_requests_and_streamed_responses(self):
        self.start({'io_timeout': 5})
        for protocol in ('fcgi', 'http', 'unix'):
            body = 'private large body' * 30000
            value = self.value(protocol, protocol+'-large', mode='chunked' if protocol == 'http' else '',
                               method='POST', body=body)
            self.assertEqual(value['body'], body)
            second = self.value(protocol, protocol+'-after-large')
            self.assertEqual(value['connection'], second['connection'])
            self.assertEqual(second['body'], '')

    def test_http_connection_close_tokens_are_respected(self):
        self.start()
        first = self.value('http', 'close-tokens', mode='close_tokens')
        second = self.value('http', 'after-close-tokens')
        self.assertNotEqual(first['connection'], second['connection'])
        # Extra bytes after a chunked terminator cannot become a new response.
        with self.assertRaises((http.client.IncompleteRead, http.client.RemoteDisconnected, ConnectionResetError)):
            self.request('http', 'chunked-extra', mode='chunked_extra')
        self.value('http', 'after-chunked-extra')

    def test_reuse_and_request_isolation_tcp_and_unix(self):
        self.start()
        for protocol, backend in zip(('fcgi', 'http', 'unix'), self.backends):
            ids = []
            for index in range(20):
                value = self.value(protocol, f'{protocol}-{index}', method='POST', body=f'private body {index}',
                                   headers={'Cookie': f'private={index}', 'Authorization': f'Bearer {index}'})
                self.assertEqual(value['body'], f'private body {index}')
                self.assertEqual(value['cookie'], f'private={index}')
                self.assertEqual(value['authorization'], f'Bearer {index}')
                ids.append(value['connection'])
            clean = self.value(protocol, f'{protocol}-clean')
            self.assertEqual(clean['cookie'], '')
            self.assertEqual(clean['authorization'], '')
            self.assertEqual(clean['body'], '')
            self.assertEqual(len(set(ids)), 1, ids)
            self.assertEqual(backend.accepted, 1)

    def test_concurrent_request_isolation(self):
        self.start()
        def request(index):
            protocol = 'fcgi' if index % 2 else 'http'
            value = self.value(protocol, f'parallel-{index}', method='POST', body=f'{index}' * 1000,
                               headers={'Cookie': f'id={index}'})
            self.assertEqual(value['cookie'], f'id={index}')
            self.assertEqual(value['body'], f'{index}' * 1000)
        with concurrent.futures.ThreadPoolExecutor(max_workers=12) as pool:
            list(pool.map(request, range(48)))
        time.sleep(.2)
        for backend in self.backends[:2]:
            self.assertLessEqual(backend.open, 4)

    def test_disabled_and_request_limit(self):
        self.start({'keepalive_requests': 2})
        for protocol in ('fcgi', 'http', 'unix'):
            values = [self.value(protocol, f'{protocol}-limit-{i}')['connection'] for i in range(5)]
            self.assertEqual(values[0], values[1])
            self.assertNotEqual(values[1], values[2])
            self.assertEqual(values[2], values[3])

    def test_disabled_reuse(self):
        self.start({'keepalive': 0})
        for protocol in ('fcgi', 'http', 'unix'):
            first = self.value(protocol, f'{protocol}-first')['connection']
            second = self.value(protocol, f'{protocol}-second')['connection']
            self.assertNotEqual(first, second)

    def test_idle_expiry_and_closed_peer(self):
        self.start({'keepalive_timeout': 1})
        for protocol in ('fcgi', 'http', 'unix'):
            self.value(protocol, f'{protocol}-idle-close', mode='idle_close')
            self.value(protocol, f'{protocol}-after-close')
        deadline = time.monotonic() + 4
        while time.monotonic() < deadline and any(b.open for b in self.backends):
            time.sleep(.05)
        self.assertEqual([b.open for b in self.backends], [0, 0, 0])

    def test_safe_retry_after_close_and_timeout(self):
        self.start()
        for protocol in ('fcgi', 'http', 'unix'):
            for mode in ('close_once', 'hang_once'):
                value = self.value(protocol, f'{protocol}-{mode}', mode=mode)
                self.assertEqual(value['attempt'], 2)

    def test_hang_returns_504_and_does_not_retry_post(self):
        self.start()
        for protocol, backend in zip(('fcgi', 'http', 'unix'), self.backends):
            start = time.monotonic()
            status, _ = self.request(protocol, protocol+'-hang-get', mode='hang')
            self.assertEqual(status, 504)
            self.assertLess(time.monotonic() - start, 5)
            self.assertEqual(backend.attempts[protocol+'-hang-get'], 2)
            status, _ = self.request(protocol, protocol+'-hang-post', mode='hang', method='POST', body='charge once')
            self.assertEqual(status, 504)
            self.assertEqual(backend.attempts[protocol+'-hang-post'], 1)
            self.value(protocol, protocol+'-after-timeout')

    def test_fcgi_framing_and_stderr(self):
        self.start()
        for protocol in ('fcgi', 'unix'):
            self.value(protocol, protocol+'-split', mode='split')
            self.value(protocol, protocol+'-stderr', mode='stderr')
            for mode in ('wrong_id', 'bad_end', 'overload', 'extra'):
                status, _ = self.request(protocol, protocol+'-'+mode, mode=mode)
                self.assertEqual(status, 502, (protocol, mode))
                self.value(protocol, protocol+'-after-'+mode)

    def test_http_framing_and_chunked_trailers(self):
        self.start()
        first = self.value('http', 'chunked', mode='chunked')
        second = self.value('http', 'after-chunked')
        self.assertEqual(first['connection'], second['connection'])
        for mode in ('duplicate_length', 'extra'):
            status, _ = self.request('http', mode, mode=mode)
            self.assertEqual(status, 502)
            self.value('http', 'after-'+mode)
        status, data = self.request('http', 'head', method='HEAD')
        self.assertEqual((status, data), (200, b''))
        self.value('http', 'after-head')


if __name__ == '__main__':
    unittest.main(verbosity=2)
