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

References:
* https://www.rfc-editor.org/rfc/rfc9113
* https://www.rfc-editor.org/rfc/rfc9114
* https://docs.openssl.org/3.6/man3/SSL_new_listener/
* https://nghttp2.org/documentation/
* https://nghttp2.org/nghttp3/
