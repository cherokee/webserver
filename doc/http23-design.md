Native HTTP/2 and HTTP/3
======================

`http.h` describes methods, status codes and protocol versions, and `http.c`
converts them to strings. Neither file owns connections or parses the wire.
`header.c` parses HTTP/1 request lines and fields; `connection.c` implements
routing, authentication, handlers, body processing and HTTP/1 serialization.
`thread.c` drives one connection per socket through those phases. TLS belongs
to the cryptor plugin; listeners belong to `bind.c`. CGI and access logs use
`cherokee_http_version_to_string` to report the request protocol.

HTTP/2 (RFC 9113) needs TLS ALPN `h2`, HPACK and independent streams over TCP.
HTTP/3 (RFC 9114) needs ALPN `h3`, QPACK, control streams and QUIC over UDP.
Adding enum entries or recognizing HTTP/2 request lines cannot enable either.

The native adapters use nghttp2 and nghttp3 for binary framing and compression,
and OpenSSL for TLS and QUIC. They run inside the server without subprocesses
or upstream network proxies. A bounded, nonblocking Unix socket pair per request
connects each stream to the existing Cherokee request engine. The adapter uses
HTTP/1.0 only as an internal handler representation, preventing chunked transfer
encoding from escaping into binary DATA frames. The original protocol and peer
address are preserved for CGI, TLS rules and logging. Stream cancellation closes
the adapter end, propagating EOF to the handler engine.

Protocol support is optional at build time and disabled by default at runtime.
`server!bind!N!http2 = 1` and `server!bind!N!http3 = 1` enable the adapters on a
TLS bind; HTTP/3 additionally listens on UDP at the same address and port.
Unsupported or insecure configurations must fail during startup. HTTP/1 remains
available on the TCP bind. HTTP/2 cleartext Upgrade and CONNECT tunnels are not
part of this implementation.

Build and administration
------------------------

HTTP/2 requires libnghttp2 >= 1.61 and the OpenSSL cryptor. HTTP/3 requires
libnghttp3 >= 1.12 and OpenSSL >= 3.5 built with server QUIC support.
`pkg-config` must find these libraries. `--enable-http2` and `--enable-http3`
make missing dependencies a configure error; `--disable-http2` and
`--disable-http3` produce a build without the corresponding adapter. The
default configure setting detects available dependencies automatically.

In Cherokee Admin, select the libssl TLS backend and enable TLS on the port
under General, then enable the HTTP/2 and HTTP/3 checkboxes for that port.
The controls use the running binary's build capabilities, and the apply
endpoint validates the proposed configuration before saving it. Configure
virtual server certificates using the existing TLS settings. The equivalent
listener configuration is:

```
server!tls = libssl
server!bind!1!port = 443
server!bind!1!tls = 1
server!bind!1!http2 = 1
server!bind!1!http3 = 1
```

Allow TCP and UDP on this port. TLS responses advertise HTTP/3 with Alt-Svc.
Clients without the new protocols continue to use HTTP/1 on TCP.

Resource and security policy
----------------------------

Requests have at most 64 KiB of decoded fields including per-field overhead,
1 MiB of body, and 100 concurrent streams per protocol connection. Bodies
are buffered before handler dispatch. Each worker has a 16 MiB aggregate
budget for retained request fields and bodies, including HTTP/3 encoded field
sections while decoding. This budget measures input bytes rather than total
process memory; buffer capacity, handler memory and library transport buffers
are additional. Response adapters retain at most 64 KiB per stream and pause
handler reads when flow control prevents transmission. Kernel socket buffers
and OpenSSL's QUIC retransmission buffers also apply backpressure.

Field validation rejects prohibited connection fields, unsafe characters,
ambiguous lengths, pseudo-field ordering errors and unsupported tunnels before
handler dispatch. Cookies are joined with `; `, framing is synthesized from
the actual body, and trailers cannot supply routing or authentication fields.
Responses remove both fixed hop-by-hop fields and fields nominated by
Connection. nghttp2 bounds CONTINUATION and outstanding ACK processing;
nghttp3 receives monotonic timestamps for its protocol abuse limiter.

HTTP/3 additionally validates every decoded QPACK field before nghttp3's
messaging filter can discard an invalid optional field. Request dynamic table
capacity and blocked streams are advertised as zero; static QPACK and Huffman
coding work normally, and response encoding follows the client's settings.
This adds a second field-section decode until nghttp3 exposes strict validation
of fields it would otherwise discard. Compression/framing errors terminate
the connection with the corresponding HTTP/3 error; Cherokee field-limit
violations reset the request stream. Closed connections retain QUIC state
briefly to allow the close packet to be transmitted while handler state is
released immediately.

HTTP/2 uses TLS >= 1.2 with authenticated ephemeral key exchange and AEAD.
HTTP/3 uses QUIC's TLS 1.3 and OpenSSL address validation. Early data is disabled
so replay-sensitive handlers cannot execute before authentication completes.
Virtual server certificate selection and client verification policies are
copied from the existing libssl contexts.

HTTP/3 uses the main thread's poller and caps its polling interval at 10 ms to
service QUIC timers. It currently has 128 source endpoints and accepted QUIC
connections per listener, with at most eight connections per source endpoint.
Per-source listeners preserve peer identity on OpenSSL 3.x, which has no QUIC
peer-address getter. Active migration is disabled; NAT rebinding is not
supported by this endpoint mapping. HTTP/3 work is not distributed across
workers. The socket-pair bridge adds copies and file descriptors per request;
this implementation has correctness QA but has not been benchmarked or
independently security audited. CONNECT, h2c, server push and 0-RTT are outside
the supported scope.

Validation
----------

After building, install the Python client dependencies and run:

```
python3 -m pip install -r qa/requirements-http23.txt
CHEROKEE_TEST_HTTP3=1 python3 qa/test_http23.py
python3 qa/test_http23_admin.py
```

The wire tests start a temporary server with generated certificates. They
exercise ALPN and HTTP/1 fallback, multiplexed transfers beyond flow-control
windows, CGI POST and protocol/peer metadata, HEAD and ranges, cookies,
cancellation, malformed fields and lengths, repeated HTTP/3 stream cleanup,
QUIC loss/retransmission, critical stream errors and invalid startup settings.
`CHEROKEE_TEST_THREADS=1` selects the single-thread server path; the default is
two threads. `make -C qa test-http23 PYTHON=/path/to/python` runs both suites.

References:
* https://www.rfc-editor.org/rfc/rfc9113
* https://www.rfc-editor.org/rfc/rfc9114
* https://docs.openssl.org/3.6/man3/SSL_new_listener/
* https://nghttp2.org/documentation/
* https://nghttp2.org/nghttp3/
