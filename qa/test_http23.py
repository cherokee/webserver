#!/usr/bin/env python3
"""Wire-level QA for the native adapters. Run from any directory.

Dependencies: python3 -m pip install 'h2>=4.3' 'aioquic>=1.3'
The tests launch the built cherokee-worker with temporary TLS keys and content.
"""
import asyncio
import contextlib
import os
from pathlib import Path
import socket
import ssl
import subprocess
import tempfile
import time
import unittest
import sys

from h2.config import H2Configuration
from h2.connection import H2Connection
from h2.events import DataReceived, ResponseReceived, StreamEnded, StreamReset

ROOT = Path(__file__).resolve().parents[1]


class NativeHTTPTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.host = os.getenv('CHEROKEE_TEST_HOST', '127.0.0.1')
        cls.interface = os.getenv('CHEROKEE_TEST_INTERFACE', cls.host)
        cls.ipv6 = ':' in cls.host
        cls.temp = tempfile.TemporaryDirectory(prefix="cherokee-http23-")
        cls.directory = Path(cls.temp.name)
        cls.content = b"native Cherokee protocols\n" * 10000
        (cls.directory / "data").write_bytes(cls.content)
        script = cls.directory / "echo.cgi"
        script.write_text("#!/usr/bin/env python3\nimport os,sys\n"
                          "body=sys.stdin.buffer.read(int(os.environ.get('CONTENT_LENGTH','0')))\n"
                          "sys.stdout.buffer.write(b'Content-Type: text/plain\\r\\nX-Remote-Addr: ' + "
                          "os.environ.get('REMOTE_ADDR','').encode() + b'\\r\\nX-Cookie: ' + "
                          "os.environ.get('HTTP_COOKIE','').encode() + b'\\r\\nX-Server-Addr: ' + os.environ.get('SERVER_ADDR','').encode() + b'\\r\\nConnection: X-Private\\r\\nX-Private: secret\\r\\n\\r\\n' + "
                          "os.environ['SERVER_PROTOCOL'].encode() + b' ' + "
                          "os.environ.get('HTTPS','').encode() + b' ' + body)\n")
        script.chmod(0o755)
        subprocess.run(["openssl", "req", "-x509", "-newkey", "rsa:2048", "-nodes",
                        "-keyout", str(cls.directory / "key.pem"), "-out", str(cls.directory / "cert.pem"),
                        "-subj", "/CN=localhost", "-days", "1"], check=True, capture_output=True)
        subprocess.run(['openssl', 'req', '-x509', '-newkey', 'rsa:2048', '-nodes',
                        '-keyout', str(cls.directory / 'secure-key.pem'), '-out', str(cls.directory / 'secure-cert.pem'),
                        '-subj', '/CN=secure.local', '-days', '1'], check=True, capture_output=True)
        with socket.socket(socket.AF_INET6 if cls.ipv6 else socket.AF_INET) as sock:
            sock.bind((cls.host, 0))
            cls.port = sock.getsockname()[1]
        cls.config = cls.directory / "cherokee.conf"
        cls.config.write_text(f"""server!bind!1!port = {cls.port}
server!bind!1!interface = {cls.interface}
server!ipv6 = {int(cls.ipv6)}
server!bind!1!tls = 1
server!bind!1!http2 = 1
server!bind!1!http3 = {os.getenv('CHEROKEE_TEST_HTTP3', '0')}
server!tls = libssl
server!thread_number = {os.getenv('CHEROKEE_TEST_THREADS', '2')}
server!pid_file = {cls.directory}/pid
server!module_dir = {ROOT}/cherokee/.libs
server!module_deps = {ROOT}/cherokee
server!fdlimit = 1024
vserver!1!nick = default
vserver!1!document_root = {cls.directory}
vserver!1!ssl_certificate_file = {cls.directory}/cert.pem
vserver!1!ssl_certificate_key_file = {cls.directory}/key.pem
vserver!1!ssl_ciphers = ECDHE-RSA-AES128-GCM-SHA256
vserver!1!rule!10!match = extensions
vserver!1!rule!10!match!extensions = cgi
vserver!1!rule!10!handler = cgi
vserver!1!rule!1!match = default
vserver!1!rule!1!handler = file
vserver!2!nick = secure.local
vserver!2!document_root = {cls.directory}
vserver!2!ssl_certificate_file = {cls.directory}/secure-cert.pem
vserver!2!ssl_certificate_key_file = {cls.directory}/secure-key.pem
vserver!2!ssl_client_certs = require
vserver!2!ssl_ca_list_file = {cls.directory}/cert.pem
vserver!2!rule!1!match = default
vserver!2!rule!1!handler = file
""")
        cls.log = open(cls.directory / "server.log", "w+")
        cls.process = subprocess.Popen([str(ROOT / "cherokee/cherokee-worker"), "-C", str(cls.config)],
                                       stdout=cls.log, stderr=cls.log)
        for _ in range(100):
            if cls.process.poll() is not None:
                cls.log.seek(0)
                raise RuntimeError(cls.log.read())
            try:
                with socket.create_connection((cls.host, cls.port), timeout=0.1):
                    return
            except OSError:
                time.sleep(0.05)
        raise RuntimeError("Cherokee did not start")

    @classmethod
    def tearDownClass(cls):
        crashed = cls.process.poll() is not None
        cls.process.terminate()
        try:
            cls.process.wait(timeout=5)
        except subprocess.TimeoutExpired:
            cls.process.kill()
            cls.process.wait()
        cls.log.seek(0)
        log = cls.log.read()
        cls.log.close()
        cls.temp.cleanup()
        if crashed or 'ERROR: AddressSanitizer' in log or 'ERROR: LeakSanitizer' in log or 'runtime error:' in log:
            raise AssertionError(f"Cherokee crashed or sanitizer reported an error:\n{log}")

    @contextlib.contextmanager
    def h2_connection(self, validate=True, server_name='localhost', client_certificate=False, tls12=False):
        ctx = ssl._create_unverified_context()
        ctx.set_alpn_protocols(["h2", "http/1.1"])
        if tls12:
            ctx.maximum_version = ssl.TLSVersion.TLSv1_2
        if client_certificate:
            ctx.load_cert_chain(self.directory / 'cert.pem', self.directory / 'key.pem')
        with ctx.wrap_socket(socket.create_connection((self.host, self.port), timeout=5),
                             server_hostname=server_name) as sock:
            self.assertEqual(sock.selected_alpn_protocol(), "h2")
            client = H2Connection(config=H2Configuration(client_side=True, header_encoding="utf-8",
                                                            validate_outbound_headers=validate,
                                                            normalize_outbound_headers=validate))
            client.initiate_connection()
            sock.sendall(client.data_to_send())
            yield sock, client

    def send_request(self, client, stream, path, method="GET", body=b"", extra=(), authority='localhost'):
        headers = [(":method", method), (":scheme", "https"),
                   (":authority", f"{authority}:{self.port}"), (":path", path)] + list(extra)
        if body:
            if not any(name == "content-length" for name, _ in extra):
                headers.append(("content-length", str(len(body))))
        client.send_headers(stream, headers, end_stream=not body)
        if body:
            for offset in range(0, len(body), client.max_outbound_frame_size):
                client.send_data(stream, body[offset:offset+client.max_outbound_frame_size],
                                 end_stream=offset+client.max_outbound_frame_size >= len(body))

    def read_responses(self, sock, client, streams):
        results = {stream: {"body": bytearray(), "headers": {}} for stream in streams}
        remaining = set(streams)
        while remaining:
            data = sock.recv(65536)
            self.assertTrue(data, "connection closed before all streams completed")
            for event in client.receive_data(data):
                if isinstance(event, ResponseReceived):
                    results[event.stream_id]["headers"] = dict(event.headers)
                elif isinstance(event, DataReceived):
                    results[event.stream_id]["body"].extend(event.data)
                    client.acknowledge_received_data(event.flow_controlled_length, event.stream_id)
                elif isinstance(event, StreamEnded):
                    remaining.discard(event.stream_id)
                elif isinstance(event, StreamReset):
                    if event.stream_id in remaining:
                        self.fail(f"stream {event.stream_id} reset: {event.error_code}")
            pending = client.data_to_send()
            if pending:
                sock.sendall(pending)
        return results

    def test_h2_multiplex_and_flow_control(self):
        with self.h2_connection() as (sock, client):
            for stream, path in [(1, "/data"), (3, "/absent"), (5, "/data")]:
                self.send_request(client, stream, path)
            sock.sendall(client.data_to_send())
            results = self.read_responses(sock, client, [1, 3, 5])
            self.assertEqual(results[1]["body"], self.content)
            self.assertEqual(results[5]["body"], self.content)
            self.assertEqual(results[3]["headers"][":status"], "404")
            for result in results.values():
                self.assertNotIn("connection", result["headers"])
                self.assertNotIn("transfer-encoding", result["headers"])

    def test_h2_head_and_range(self):
        with self.h2_connection() as (sock, client):
            self.send_request(client, 1, "/data", "HEAD")
            self.send_request(client, 3, "/data", extra=[("range", "bytes=10-29")])
            sock.sendall(client.data_to_send())
            results = self.read_responses(sock, client, [1, 3])
            self.assertEqual(results[1]["body"], b"")
            self.assertEqual(results[3]["headers"][":status"], "206")
            self.assertEqual(results[3]["body"], self.content[10:30])

    def test_h2_cgi_post(self):
        with self.h2_connection() as (sock, client):
            self.send_request(client, 1, "/echo.cgi", "POST", b"request body")
            sock.sendall(client.data_to_send())
            result = self.read_responses(sock, client, [1])[1]
            self.assertEqual(result["headers"][":status"], "200")
            self.assertEqual(result["body"], b"HTTP/2 on request body")
            self.assertEqual(result["headers"]["x-remote-addr"], "::1" if self.ipv6 else "127.0.0.1")
            self.assertEqual(result["headers"]["x-server-addr"], self.host)
            self.assertNotIn('x-private', result['headers'])

    def test_h2_cookie_joining(self):
        with self.h2_connection() as (sock, client):
            self.send_request(client, 1, "/echo.cgi", extra=[("cookie", "a=1"), ("cookie", "b=2")])
            sock.sendall(client.data_to_send())
            result = self.read_responses(sock, client, [1])[1]
            self.assertEqual(result["headers"]["x-cookie"], "a=1; b=2")

    def test_h2_sni_and_client_certificate(self):
        with self.h2_connection() as (sock, client):
            self.send_request(client, 1, '/data', authority='secure.local')
            sock.sendall(client.data_to_send())
            result = self.read_responses(sock, client, [1])[1]
            self.assertEqual(result['headers'][':status'], '403')
            self.assertNotEqual(result['body'], self.content)
        with self.assertRaises(OSError):
            with self.h2_connection(server_name='secure.local') as (sock, client):
                self.send_request(client, 1, '/data', authority='secure.local')
                sock.sendall(client.data_to_send())
                sock.recv(65536)
        with self.h2_connection(server_name='secure.local', client_certificate=True) as (sock, client):
            expected = ssl.PEM_cert_to_DER_cert((self.directory / 'secure-cert.pem').read_text())
            self.assertEqual(sock.getpeercert(binary_form=True), expected)
            self.send_request(client, 1, '/data', authority='secure.local')
            sock.sendall(client.data_to_send())
            self.assertEqual(self.read_responses(sock, client, [1])[1]['body'], self.content)

    def test_h2_tls12_cipher_policy(self):
        with self.h2_connection(tls12=True) as (sock, client):
            self.assertEqual(sock.cipher()[0], 'ECDHE-RSA-AES128-GCM-SHA256')
            self.send_request(client, 1, '/echo.cgi')
            sock.sendall(client.data_to_send())
            self.assertEqual(self.read_responses(sock, client, [1])[1]['body'], b'HTTP/2 on ')

    def test_h2_cancellation_and_connection_reuse(self):
        with self.h2_connection() as (sock, client):
            self.send_request(client, 1, "/data")
            client.reset_stream(1)
            self.send_request(client, 3, "/data", extra=[("range", "bytes=5-30")])
            sock.sendall(client.data_to_send())
            result = self.read_responses(sock, client, [3])[3]
            self.assertEqual(result["body"], self.content[5:31])
            for stream in range(5, 45, 2):
                self.send_request(client, stream, "/echo.cgi")
                sock.sendall(client.data_to_send())
                result = self.read_responses(sock, client, [stream])[stream]
                self.assertEqual(result["body"], b"HTTP/2 on ")

    def test_h2_invalid_headers_and_length(self):
        bad_headers = [
            [("connection", "close")],
            [("x-injection", "value\r\nInjected: yes")],
            [("x-oversize", "x" * 65536)],
            [("content-length", "3")],
            [("content-length", "0"), ("content-length", "0")],
        ]
        for extra in bad_headers:
            with self.subTest(header=extra[0][0]):
                with self.h2_connection(validate=False) as (sock, client):
                    self.send_request(client, 1, "/echo.cgi", extra=extra)
                    self.send_request(client, 3, "/echo.cgi")
                    sock.sendall(client.data_to_send())
                    reset = False
                    valid = bytearray()
                    ended = False
                    while not (reset and ended):
                        data = sock.recv(65536)
                        self.assertTrue(data)
                        for event in client.receive_data(data):
                            if isinstance(event, StreamReset) and event.stream_id == 1:
                                reset = True
                            elif isinstance(event, ResponseReceived) and event.stream_id == 1:
                                self.fail("malformed request reached a handler")
                            elif isinstance(event, DataReceived):
                                if event.stream_id == 3:
                                    valid.extend(event.data)
                                client.acknowledge_received_data(event.flow_controlled_length, event.stream_id)
                            elif isinstance(event, StreamEnded) and event.stream_id == 3:
                                ended = True
                        pending = client.data_to_send()
                        if pending:
                            sock.sendall(pending)
                    self.assertEqual(valid, b"HTTP/2 on ")

    def test_startup_rejects_insecure_protocols(self):
        original = self.config.read_text().replace(f'port = {self.port}', 'port = 0')
        configs = [original.replace("server!bind!1!tls = 1", "server!bind!1!tls = 0"),
                   original.replace("server!tls = libssl\n", ""),
                   '\n'.join(line for line in original.splitlines() if '!ssl_ca_list_file' not in line)]
        for config in configs:
            with self.subTest(configuration=config.splitlines()[:5]):
                path = self.directory / "invalid.conf"
                path.write_text(config)
                result = subprocess.run([str(ROOT / "cherokee/cherokee-worker"), "-C", str(path)],
                                        capture_output=True, timeout=5, check=False)
                self.assertNotEqual(result.returncode, 0)
                self.assertIn(b"require", result.stderr.lower())

    def test_http1_fallback(self):
        ctx = ssl._create_unverified_context()
        ctx.set_alpn_protocols(["http/1.1"])
        with ctx.wrap_socket(socket.create_connection((self.host, self.port), timeout=5),
                             server_hostname="localhost") as sock:
            self.assertEqual(sock.selected_alpn_protocol(), "http/1.1")
            sock.sendall(b"GET /data HTTP/1.1\r\nHost: localhost\r\nConnection: close\r\n\r\n")
            response = bytearray()
            while data := sock.recv(65536):
                response.extend(data)
            self.assertTrue(response.startswith(b"HTTP/1.1 200"))
            self.assertEqual(response.split(b"\r\n\r\n", 1)[1], self.content)

    def test_http1_fallback_certificate_authority(self):
        ctx = ssl._create_unverified_context()
        ctx.set_alpn_protocols(['http/1.1'])
        with ctx.wrap_socket(socket.create_connection((self.host, self.port), timeout=5),
                             server_hostname='localhost') as sock:
            sock.sendall(b'GET /data HTTP/1.1\r\nHost: secure.local\r\nConnection: close\r\n\r\n')
            response = bytearray()
            while data := sock.recv(65536):
                response.extend(data)
            self.assertTrue(response.startswith(b'HTTP/1.1 403'))
            self.assertNotEqual(response.split(b'\r\n\r\n', 1)[1], self.content)


    @unittest.skipUnless(os.getenv("CHEROKEE_TEST_HTTP3") == "1", "enable with CHEROKEE_TEST_HTTP3=1")
    def test_h3_multiplex_cgi_and_flow_control(self):
        from aioquic.asyncio import connect, QuicConnectionProtocol
        from aioquic.h3.connection import H3Connection, H3_ALPN, encode_frame, FrameType
        from aioquic.h3.events import HeadersReceived, DataReceived as H3DataReceived
        from aioquic.quic.configuration import QuicConfiguration
        from aioquic.quic.events import ConnectionTerminated, StreamReset as QuicStreamReset

        class RequestReset(Exception):
            pass

        class RequestAwareH3(H3Connection):
            # aioquic 1.3's generic H3 layer does not track request methods.
            # RFC 9110 allows HEAD's Content-Length to describe the GET body.
            def __init__(self, quic):
                super().__init__(quic)
                self.head_streams = set()

            def _check_content_length(self, stream):
                if stream.stream_id not in self.head_streams:
                    super()._check_content_length(stream)

        class Client(QuicConnectionProtocol):
            def __init__(self, *args, **kwargs):
                super().__init__(*args, **kwargs)
                self.http = RequestAwareH3(self._quic)
                self.response_packets = 0
                self.dropped_packets = 0
                self.drop_packets = True
                self.responses = {}
                self.waiters = {}
                self.closed = asyncio.get_running_loop().create_future()

            def datagram_received(self, data, addr):
                # Drop a few application packets to verify server retransmission
                # after nghttp3 has released the copied response buffers.
                if self.drop_packets and self.waiters:
                    self.response_packets += 1
                    if self.response_packets % 17 == 0 and self.dropped_packets < 3:
                        self.dropped_packets += 1
                        return
                super().datagram_received(data, addr)

            def quic_event_received(self, event):
                if isinstance(event, ConnectionTerminated):
                    if not self.closed.done():
                        self.closed.set_result(event)
                    for waiter in self.waiters.values():
                        if not waiter.done():
                            waiter.set_exception(RuntimeError(f"QUIC closed: {event}"))
                if isinstance(event, QuicStreamReset):
                    waiter = self.waiters.get(event.stream_id)
                    if waiter and not waiter.done():
                        waiter.set_exception(RequestReset(event.error_code))
                for received in self.http.handle_event(event):
                    stream = received.stream_id
                    response = self.responses.setdefault(stream, {"headers": {}, "body": bytearray()})
                    if isinstance(received, HeadersReceived):
                        response["headers"].update(dict(received.headers))
                    elif isinstance(received, H3DataReceived):
                        response["body"].extend(received.data)
                    if received.stream_ended:
                        waiter = self.waiters.get(stream)
                        if waiter and not waiter.done():
                            waiter.set_result(response)

            def request(self, port, path, method=b"GET", body=b"", extra=(), authority=None):
                stream = self._quic.get_next_available_stream_id()
                future = asyncio.get_running_loop().create_future()
                self.waiters[stream] = future
                if method == b"HEAD":
                    self.http.head_streams.add(stream)
                headers = [(b":method", method), (b":scheme", b"https"),
                           (b":authority", f"{authority or self._quic.configuration.server_name}:{port}".encode()), (b":path", path)] + list(extra)
                if body:
                    headers.append((b"content-length", str(len(body)).encode()))
                if sum(len(name) + len(value) for name, value in headers) > 65000:
                    # pylsqpack's encoder has a fixed field-section output
                    # buffer. Assemble static-only field lines individually
                    # to send an intentionally oversized section on the wire.
                    import pylsqpack
                    encoder = pylsqpack.Encoder()
                    encoded = bytearray(b'\x00\x00')
                    for header in headers:
                        instructions, section = encoder.encode(stream, [header])
                        assert not instructions and section[:2] == b'\x00\x00'
                        encoded.extend(section[2:])
                    self._quic.send_stream_data(stream, encode_frame(FrameType.HEADERS, bytes(encoded)), end_stream=not body)
                else:
                    self.http.send_headers(stream, headers, end_stream=not body)
                if body:
                    self.http.send_data(stream, body, end_stream=True)
                self.transmit()
                return future

        async def run():
            config = QuicConfiguration(is_client=True, alpn_protocols=H3_ALPN,
                                       verify_mode=ssl.CERT_NONE, server_name="localhost",
                                       max_data=65536, max_stream_data=32768)
            async with connect(self.host, self.port, configuration=config, create_protocol=Client) as client:
                futures = [client.request(self.port, b"/data"),
                           client.request(self.port, b"/absent"),
                           client.request(self.port, b"/echo.cgi", b"POST", b"quic request"),
                           client.request(self.port, b"/data", b"HEAD"),
                           client.request(self.port, b"/data", extra=[(b"range", b"bytes=7-24")])]
                results = await asyncio.wait_for(asyncio.gather(*futures), timeout=10)
                self.assertEqual(results[0]["body"], self.content)
                self.assertEqual(results[1]["headers"][b":status"], b"404")
                self.assertEqual(results[2]["body"], b"HTTP/3 on quic request")
                self.assertEqual(results[2]["headers"][b"x-remote-addr"], b"::1" if self.ipv6 else b"127.0.0.1")
                self.assertEqual(results[2]["headers"][b"x-server-addr"], self.host.encode())
                self.assertNotIn(b'x-private', results[2]['headers'])
                self.assertEqual(results[3]["body"], b"")
                self.assertEqual(results[4]["body"], self.content[7:25])
                for result in results:
                    self.assertNotIn(b"connection", result["headers"])
                    self.assertNotIn(b"transfer-encoding", result["headers"])
                # Exercise lifetime cleanup by reusing the connection after FIN.
                for _ in range(120):
                    result = await asyncio.wait_for(client.request(self.port, b"/data", extra=[(b"range", b"bytes=7-24")]), timeout=10)
                    self.assertEqual(result["body"], self.content[7:25])
                self.assertEqual(client.dropped_packets, 3)
                body = b'upload' * 10000
                result = await asyncio.wait_for(client.request(self.port, b'/echo.cgi', b'POST', body), timeout=5)
                self.assertEqual(result['body'], b'HTTP/3 on ' + body)
                result = await asyncio.wait_for(client.request(self.port, b'/data', authority='secure.local'), timeout=5)
                self.assertEqual(result['headers'][b':status'], b'403')
                self.assertNotEqual(result['body'], self.content)
                # Reset before headers reach nghttp3, then reuse the session.
                stream = client._quic.get_next_available_stream_id()
                client._quic.send_stream_data(stream, b'', end_stream=False)
                client._quic.reset_stream(stream, 0x10c)
                client.transmit()
                result = await asyncio.wait_for(client.request(self.port, b'/echo.cgi'), timeout=5)
                self.assertEqual(result['body'], b'HTTP/3 on ')
                # Closing a critical control stream is a connection error.
                client._quic.reset_stream(client.http._local_control_stream_id, 0x10c)
                client.transmit()
                closed = await asyncio.wait_for(client.closed, timeout=5)
                self.assertEqual(closed.error_code, 0x104)

            # nghttp3 treats malformed messaging as a connection error. Its
            # API requires teardown after a negative read result. Cherokee's
            # own stricter field/body limits reset the individual request.
            for extra in ([(b'connection', b'close')], [(b'x-bad', b'one\r\ntwo')],
                          [(b'content-length', b'10')],
                          [(f'x-large-{i}'.encode(), b'a' * 1000) for i in range(80)]):
                async with connect(self.host, self.port, configuration=config, create_protocol=Client) as client:
                    client.drop_packets = False
                    bad = client.request(self.port, b'/echo.cgi', extra=extra)
                    good = client.request(self.port, b'/echo.cgi')
                    results = await asyncio.wait_for(asyncio.gather(bad, good, return_exceptions=True), timeout=5)
                    if isinstance(results[0], RequestReset) and isinstance(results[1], dict):
                        self.assertEqual(results[0].args[0], 0x10e)
                        self.assertEqual(results[1]['body'], b'HTTP/3 on ')
                    else:
                        self.assertIsInstance(results[0], (RuntimeError, RequestReset))
                        closed = await asyncio.wait_for(client.closed, timeout=5)
                        self.assertEqual(closed.error_code, 0x10e)

            config.server_name = 'secure.local'
            with self.assertRaises((ConnectionError, RuntimeError)):
                async with connect(self.host, self.port, configuration=config, create_protocol=Client) as client:
                    await asyncio.wait_for(client.request(self.port, b'/data'), timeout=5)
            config.load_cert_chain(self.directory / 'cert.pem', self.directory / 'key.pem')
            async with connect(self.host, self.port, configuration=config, create_protocol=Client) as client:
                result = await asyncio.wait_for(client.request(self.port, b'/data'), timeout=5)
                self.assertEqual(result['body'], self.content)
                self.assertEqual(client._quic.tls._peer_certificate.subject.rfc4514_string(), 'CN=secure.local')

        try:
            asyncio.run(asyncio.wait_for(run(), timeout=20))
        except Exception:
            self.log.flush()
            log = (self.directory / "server.log").read_text()
            print(log[:3000] + "\n...\n" + log[-3000:])
            raise


if __name__ == "__main__":
    unittest.main(verbosity=2)
