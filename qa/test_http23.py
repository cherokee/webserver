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

from h2.config import H2Configuration
from h2.connection import H2Connection
from h2.events import DataReceived, ResponseReceived, StreamEnded, StreamReset

ROOT = Path(__file__).resolve().parents[1]


class NativeHTTPTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.temp = tempfile.TemporaryDirectory(prefix="cherokee-http23-")
        cls.directory = Path(cls.temp.name)
        cls.content = b"native Cherokee protocols\n" * 10000
        (cls.directory / "data").write_bytes(cls.content)
        script = cls.directory / "echo.cgi"
        script.write_text("#!/usr/bin/env python3\nimport os,sys\n"
                          "body=sys.stdin.buffer.read(int(os.environ.get('CONTENT_LENGTH','0')))\n"
                          "sys.stdout.buffer.write(b'Content-Type: text/plain\\r\\n\\r\\n' + "
                          "os.environ['SERVER_PROTOCOL'].encode() + b' ' + "
                          "os.environ.get('HTTPS','').encode() + b' ' + body)\n")
        script.chmod(0o755)
        subprocess.run(["openssl", "req", "-x509", "-newkey", "rsa:2048", "-nodes",
                        "-keyout", str(cls.directory / "key.pem"), "-out", str(cls.directory / "cert.pem"),
                        "-subj", "/CN=localhost", "-days", "1"], check=True, capture_output=True)
        with socket.socket() as sock:
            sock.bind(("127.0.0.1", 0))
            cls.port = sock.getsockname()[1]
        cls.config = cls.directory / "cherokee.conf"
        cls.config.write_text(f"""server!bind!1!port = {cls.port}
server!bind!1!interface = 127.0.0.1
server!bind!1!tls = 1
server!bind!1!http2 = 1
server!bind!1!http3 = {os.getenv('CHEROKEE_TEST_HTTP3', '0')}
server!tls = libssl
server!thread_number = 2
server!pid_file = {cls.directory}/pid
server!module_dir = {ROOT}/cherokee/.libs
server!module_deps = {ROOT}/cherokee
server!fdlimit = 1024
vserver!1!nick = default
vserver!1!document_root = {cls.directory}
vserver!1!ssl_certificate_file = {cls.directory}/cert.pem
vserver!1!ssl_certificate_key_file = {cls.directory}/key.pem
vserver!1!rule!10!match = extensions
vserver!1!rule!10!match!extensions = cgi
vserver!1!rule!10!handler = cgi
vserver!1!rule!1!match = default
vserver!1!rule!1!handler = file
""")
        cls.log = open(cls.directory / "server.log", "w+")
        cls.process = subprocess.Popen([str(ROOT / "cherokee/cherokee-worker"), "-C", str(cls.config)],
                                       stdout=cls.log, stderr=cls.log)
        for _ in range(100):
            if cls.process.poll() is not None:
                cls.log.seek(0)
                raise RuntimeError(cls.log.read())
            try:
                with socket.create_connection(("127.0.0.1", cls.port), timeout=0.1):
                    return
            except OSError:
                time.sleep(0.05)
        raise RuntimeError("Cherokee did not start")

    @classmethod
    def tearDownClass(cls):
        cls.process.terminate()
        try:
            cls.process.wait(timeout=5)
        except subprocess.TimeoutExpired:
            cls.process.kill()
            cls.process.wait()
        cls.log.close()
        cls.temp.cleanup()

    @contextlib.contextmanager
    def h2_connection(self):
        ctx = ssl._create_unverified_context()
        ctx.set_alpn_protocols(["h2", "http/1.1"])
        with ctx.wrap_socket(socket.create_connection(("127.0.0.1", self.port), timeout=5),
                             server_hostname="localhost") as sock:
            self.assertEqual(sock.selected_alpn_protocol(), "h2")
            client = H2Connection(config=H2Configuration(client_side=True, header_encoding="utf-8"))
            client.initiate_connection()
            sock.sendall(client.data_to_send())
            yield sock, client

    def send_request(self, client, stream, path, method="GET", body=b"", extra=()):
        headers = [(":method", method), (":scheme", "https"),
                   (":authority", f"localhost:{self.port}"), (":path", path)] + list(extra)
        if body:
            headers.append(("content-length", str(len(body))))
        client.send_headers(stream, headers, end_stream=not body)
        if body:
            client.send_data(stream, body, end_stream=True)

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

    def test_http1_fallback(self):
        ctx = ssl._create_unverified_context()
        ctx.set_alpn_protocols(["http/1.1"])
        with ctx.wrap_socket(socket.create_connection(("127.0.0.1", self.port), timeout=5),
                             server_hostname="localhost") as sock:
            self.assertEqual(sock.selected_alpn_protocol(), "http/1.1")
            sock.sendall(b"GET /data HTTP/1.1\r\nHost: localhost\r\nConnection: close\r\n\r\n")
            response = bytearray()
            while data := sock.recv(65536):
                response.extend(data)
            self.assertTrue(response.startswith(b"HTTP/1.1 200"))
            self.assertEqual(response.split(b"\r\n\r\n", 1)[1], self.content)


    @unittest.skipUnless(os.getenv("CHEROKEE_TEST_HTTP3") == "1", "enable with CHEROKEE_TEST_HTTP3=1")
    def test_h3_multiplex_cgi_and_flow_control(self):
        from aioquic.asyncio import connect, QuicConnectionProtocol
        from aioquic.h3.connection import H3Connection, H3_ALPN
        from aioquic.h3.events import HeadersReceived, DataReceived as H3DataReceived
        from aioquic.quic.configuration import QuicConfiguration
        from aioquic.quic.events import ConnectionTerminated
        from aioquic.quic.logger import QuicLogger
        self.quic_log = QuicLogger()

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
                self.responses = {}
                self.waiters = {}

            def quic_event_received(self, event):
                if isinstance(event, ConnectionTerminated):
                    for waiter in self.waiters.values():
                        if not waiter.done():
                            waiter.set_exception(RuntimeError(f"QUIC closed: {event}"))
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

            def request(self, port, path, method=b"GET", body=b"", extra=()):
                stream = self._quic.get_next_available_stream_id()
                future = asyncio.get_running_loop().create_future()
                self.waiters[stream] = future
                if method == b"HEAD":
                    self.http.head_streams.add(stream)
                headers = [(b":method", method), (b":scheme", b"https"),
                           (b":authority", f"localhost:{port}".encode()), (b":path", path)] + list(extra)
                if body:
                    headers.append((b"content-length", str(len(body)).encode()))
                self.http.send_headers(stream, headers, end_stream=not body)
                if body:
                    self.http.send_data(stream, body, end_stream=True)
                self.transmit()
                return future

        async def run():
            config = QuicConfiguration(is_client=True, alpn_protocols=H3_ALPN,
                                       verify_mode=ssl.CERT_NONE, server_name="localhost", quic_logger=self.quic_log)
            async with connect("127.0.0.1", self.port, configuration=config, create_protocol=Client) as client:
                futures = [client.request(self.port, b"/data"),
                           client.request(self.port, b"/absent"),
                           client.request(self.port, b"/echo.cgi", b"POST", b"quic request"),
                           client.request(self.port, b"/data", b"HEAD"),
                           client.request(self.port, b"/data", extra=[(b"range", b"bytes=7-24")])]
                results = await asyncio.wait_for(asyncio.gather(*futures), timeout=10)
                self.assertEqual(results[0]["body"], self.content)
                self.assertEqual(results[1]["headers"][b":status"], b"404")
                self.assertEqual(results[2]["body"], b"HTTP/3 on quic request")
                self.assertEqual(results[3]["body"], b"")
                self.assertEqual(results[4]["body"], self.content[7:25])
                for result in results:
                    self.assertNotIn(b"connection", result["headers"])
                    self.assertNotIn(b"transfer-encoding", result["headers"])
                # Exercise lifetime cleanup by reusing the connection after FIN.
                for _ in range(3):
                    result = await asyncio.wait_for(client.request(self.port, b"/data"), timeout=10)
                    self.assertEqual(result["body"], self.content)

        try:
            asyncio.run(asyncio.wait_for(run(), timeout=20))
        except Exception:
            import json
            Path("/tmp/cherokee-last-qlog.json").write_text(json.dumps(self.quic_log.to_dict()))
            self.log.flush()
            log = (self.directory / "server.log").read_text()
            print(log[:3000] + "\n...\n" + log[-3000:])
            raise


if __name__ == "__main__":
    unittest.main(verbosity=2)
