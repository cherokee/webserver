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


if __name__ == "__main__":
    unittest.main(verbosity=2)
