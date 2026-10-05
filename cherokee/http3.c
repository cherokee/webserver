/* Cherokee native HTTP/3 (RFC 9114). GPL-2.0-only.
 * OpenSSL linking exception: the exception in cryptor_libssl.c also applies
 * to this transport, which uses the same TLS library and certificate policy.
 */
#include "common-internal.h"
#include "http3.h"
#include "http-stream.h"
#ifdef HAVE_HTTP3
#include "cryptor_libssl.h"
#include <nghttp3/nghttp3.h>
#include <openssl/quic.h>
#include <fcntl.h>
#include <errno.h>

#define HTTP3_ENDPOINT_MAX 128
#define HTTP3_QUIC_STREAM_MAX (CHEROKEE_HTTP_STREAM_MAX + 6)

typedef struct {
	cherokee_list_t list;
	SSL *ssl;
	cherokee_http_stream_t *request;
	uint64_t id;
	size_t offered, acknowledged;
	int read_fin, write_fin, closed;
} cherokee_http3_stream_t;

typedef struct {
	cherokee_list_t list;
	SSL *ssl;
	nghttp3_conn *http;
	cherokee_list_t streams;
	cherokee_bind_t *bind;
	cherokee_thread_t *thread;
	cherokee_sockaddr_t peer;
	time_t activity;
	int dead;
} cherokee_http3_connection_t;

typedef struct {
	cherokee_list_t list;
	SSL *listener;
	BIO *network;
	cherokee_sockaddr_t peer;
	socklen_t peer_len;
	cherokee_list_t connections;
	time_t activity;
	unsigned char pending[65536];
	size_t pending_len;
} cherokee_http3_endpoint_t;

typedef struct {
	cherokee_list_t list;
	cherokee_virtual_server_t *vserver;
	SSL_CTX *context;
} cherokee_http3_context_t;

typedef struct {
	int fd, registered;
	cherokee_bind_t *bind;
	cherokee_server_t *server;
	cherokee_list_t endpoints, contexts;
	size_t endpoint_count;
	SSL_CTX *default_context;
} cherokee_http3_listener_t;

static cherokee_http3_stream_t *
find_stream (cherokee_http3_connection_t *c, int64_t id)
{
	cherokee_list_t *i;
	list_for_each (i, &c->streams) {
		cherokee_http3_stream_t *s = (cherokee_http3_stream_t *)i;
		if (s->id == (uint64_t)id) return s;
	}
	return NULL;
}

static int
alpn_select (SSL *ssl, const unsigned char **out, unsigned char *outlen,
             const unsigned char *in, unsigned int len, void *arg)
{
	unsigned int off = 0;
	UNUSED(ssl); UNUSED(arg);
	while (off < len) {
		unsigned int n = in[off++];
		if (!n || n > len-off) return SSL_TLSEXT_ERR_ALERT_FATAL;
		if (n == 2 && !memcmp(in+off, "h3", 2)) {
			*out = in+off; *outlen = 2; return SSL_TLSEXT_ERR_OK;
		}
		off += n;
	}
	return SSL_TLSEXT_ERR_ALERT_FATAL;
}

static int
sni_select (SSL *ssl, int *alert, void *arg)
{
	cherokee_http3_listener_t *h = arg;
	cherokee_virtual_server_t *vsrv;
	cherokee_list_t *i;
	cherokee_connection_t conn;
	cherokee_buffer_t host = CHEROKEE_BUF_INIT;
	const char *name = SSL_get_servername(ssl, TLSEXT_NAMETYPE_host_name);
	SSL_CTX *ctx = NULL;
	if (!name) return SSL_TLSEXT_ERR_OK;
	memset(&conn, 0, sizeof(conn));
	conn.server = h->server;
	conn.bind = h->bind;
	conn.socket.is_tls = TLS;
	list_for_each (i, &h->endpoints) {
		cherokee_http3_endpoint_t *e = (cherokee_http3_endpoint_t *)i;
		if (SSL_get0_domain(ssl) == SSL_get0_domain(e->listener)) {
			conn.socket.client_addr = e->peer;
			break;
		}
	}
	if (cherokee_buffer_add(&host, name, strlen(name)) != ret_ok) return SSL_TLSEXT_ERR_ALERT_FATAL;
	if (cherokee_server_get_vserver(h->server, &host, &conn, &vsrv) == ret_ok) {
		list_for_each (i, &h->contexts) {
			cherokee_http3_context_t *v = (cherokee_http3_context_t *)i;
			if (v->vserver == vsrv) { ctx = v->context; break; }
		}
	}
	cherokee_buffer_mrproper(&host);
	if (!ctx) { *alert = SSL_AD_UNRECOGNIZED_NAME; return SSL_TLSEXT_ERR_ALERT_FATAL; }
	/* The SNI callback receives OpenSSL's inner TLS object. QUIC contexts
	 * cannot replace its TLS method; install the selected credentials and
	 * verification policy on that object instead. */
	{
		STACK_OF(X509) *chain = NULL;
		if (!SSL_use_certificate(ssl, SSL_CTX_get0_certificate(ctx)) ||
		    !SSL_use_PrivateKey(ssl, SSL_CTX_get0_privatekey(ctx))) return SSL_TLSEXT_ERR_ALERT_FATAL;
		SSL_CTX_get0_chain_certs(ctx, &chain);
		if (chain && !SSL_set1_chain(ssl, chain)) return SSL_TLSEXT_ERR_ALERT_FATAL;
		if (!SSL_set1_verify_cert_store(ssl, SSL_CTX_get_cert_store(ctx))) return SSL_TLSEXT_ERR_ALERT_FATAL;
	}
	SSL_set_verify(ssl, SSL_CTX_get_verify_mode(ctx), SSL_CTX_get_verify_callback(ctx));
	SSL_set_verify_depth(ssl, SSL_CTX_get_verify_depth(ctx));
	return SSL_TLSEXT_ERR_OK;
}

static int
begin_headers (nghttp3_conn *http, int64_t id, void *user, void *stream_user)
{
	cherokee_http3_connection_t *c = user;
	cherokee_http3_stream_t *s = find_stream(c, id);
	UNUSED(stream_user);
	if (!s || s->request) return NGHTTP3_ERR_CALLBACK_FAILURE;
	s->request = cherokee_http_stream_new(c->thread, id);
	if (!s->request) return NGHTTP3_ERR_CALLBACK_FAILURE;
	nghttp3_conn_set_stream_user_data(http, id, s);
	return 0;
}

static int
recv_header (nghttp3_conn *http, int64_t id, int32_t token, nghttp3_rcbuf *name,
             nghttp3_rcbuf *value, uint8_t flags, void *user, void *stream_user)
{
	cherokee_http3_stream_t *s = find_stream(user, id);
	nghttp3_vec n = nghttp3_rcbuf_get_buf(name), v = nghttp3_rcbuf_get_buf(value);
	UNUSED(http); UNUSED(token); UNUSED(flags); UNUSED(stream_user);
	if (!s || cherokee_http_stream_header(s->request, n.base, n.len, v.base, v.len)) return NGHTTP3_ERR_CALLBACK_FAILURE;
	return 0;
}

static int
recv_data (nghttp3_conn *http, int64_t id, const uint8_t *data, size_t len,
           void *user, void *stream_user)
{
	cherokee_http3_stream_t *s = find_stream(user, id);
	UNUSED(http); UNUSED(stream_user);
	if (!s || cherokee_http_stream_data(s->request, data, len)) return NGHTTP3_ERR_CALLBACK_FAILURE;
	return 0;
}

static int
end_stream (nghttp3_conn *http, int64_t id, void *user, void *stream_user)
{
	cherokee_http3_connection_t *c = user;
	cherokee_http3_stream_t *s = find_stream(c, id);
	UNUSED(http); UNUSED(stream_user);
	if (!s || cherokee_http_stream_dispatch(s->request, c->bind, &c->peer, http_version_30)) return NGHTTP3_ERR_CALLBACK_FAILURE;
	return 0;
}

static int
stream_close (nghttp3_conn *http, int64_t id, uint64_t error, void *user, void *stream_user)
{
	cherokee_http3_stream_t *s = find_stream(user, id);
	UNUSED(http); UNUSED(error); UNUSED(stream_user);
	if (s) s->closed = 1;
	return 0;
}

static int
reset_stream (nghttp3_conn *http, int64_t id, uint64_t error, void *user, void *stream_user)
{
	cherokee_http3_stream_t *s = find_stream(user, id);
	SSL_STREAM_RESET_ARGS args = {error};
	UNUSED(http); UNUSED(stream_user);
	if (s) {
		if (!SSL_stream_reset(s->ssl, &args, sizeof(args))) return NGHTTP3_ERR_CALLBACK_FAILURE;
		s->closed = 1;
	}
	return 0;
}

static int
acked_data (nghttp3_conn *http, int64_t id, uint64_t len, void *user, void *stream_user)
{
	cherokee_http3_stream_t *s = find_stream(user, id);
	UNUSED(http); UNUSED(stream_user);
	if (!s || len > s->offered-s->acknowledged) return NGHTTP3_ERR_CALLBACK_FAILURE;
	s->acknowledged += len;
	if (s->acknowledged == s->offered) {
		cherokee_buffer_clean(&s->request->response);
		s->offered = s->acknowledged = 0;
	}
	return 0;
}

static nghttp3_ssize
read_response (nghttp3_conn *http, int64_t id, nghttp3_vec *vec, size_t veccnt,
               uint32_t *flags, void *user, void *stream_user)
{
	cherokee_http3_stream_t *s = find_stream(user, id);
	cherokee_http_stream_t *request;
	UNUSED(http); UNUSED(veccnt); UNUSED(stream_user);
	if (!s || !s->request) return NGHTTP3_ERR_CALLBACK_FAILURE;
	request = s->request;
	if (s->offered || (!request->response.len && !request->eof)) return NGHTTP3_ERR_WOULDBLOCK;
	if (request->eof) *flags |= NGHTTP3_DATA_FLAG_EOF;
	if (!request->response.len) return 0;
	s->offered = request->response.len;
	vec[0].base = (uint8_t *)request->response.buf;
	vec[0].len = request->response.len;
	return 1;
}

static int
submit_response (cherokee_http3_connection_t *c, cherokee_http3_stream_t *s)
{
	cherokee_http_stream_t *r = s->request;
	nghttp3_nv fields[129];
	nghttp3_data_reader reader = {read_response};
	size_t i;
	fields[0] = (nghttp3_nv){(uint8_t *)":status", (uint8_t *)r->status, 7, 3, NGHTTP3_NV_FLAG_NONE};
	for (i=0; i<r->fields_len; i++) {
		fields[i+1] = (nghttp3_nv){(uint8_t *)r->fields[i].name, (uint8_t *)r->fields[i].value,
			strlen(r->fields[i].name), strlen(r->fields[i].value), NGHTTP3_NV_FLAG_NONE};
	}
	if (nghttp3_conn_submit_response(c->http, s->id, fields, r->fields_len+1, &reader)) return -1;
	r->submitted = 1;
	return 0;
}

static void
free_stream (cherokee_http3_stream_t *s)
{
	cherokee_list_del(&s->list);
	SSL_free(s->ssl);
	cherokee_http_stream_free(s->request);
	free(s);
}

static void
free_connection (cherokee_http3_connection_t *c)
{
	cherokee_list_t *i, *tmp;
	cherokee_list_del(&c->list);
	nghttp3_conn_del(c->http);
	list_for_each_safe (i, tmp, &c->streams) free_stream((cherokee_http3_stream_t *)i);
	SSL_free(c->ssl);
	free(c);
}

static cherokee_http3_stream_t *
add_stream (cherokee_http3_connection_t *c, SSL *ssl)
{
	cherokee_http3_stream_t *s = calloc(1, sizeof(*s));
	if (!s || !ssl) { free(s); SSL_free(ssl); return NULL; }
	INIT_LIST_HEAD(&s->list);
	s->id = SSL_get_stream_id(ssl);
	s->ssl = ssl;
	if (!SSL_set_blocking_mode(ssl, 0)) { free_stream(s); return NULL; }
	cherokee_list_add_tail(&s->list, &c->streams);
	return s;
}

static cherokee_http3_connection_t *
new_connection (SSL *ssl, cherokee_http3_listener_t *h, cherokee_http3_endpoint_t *e, cherokee_thread_t *thread)
{
	cherokee_http3_connection_t *c = calloc(1, sizeof(*c));
	if (!c) { SSL_free(ssl); return NULL; }
	INIT_LIST_HEAD(&c->list); INIT_LIST_HEAD(&c->streams);
	c->ssl = ssl; c->bind = h->bind; c->thread = thread; c->peer = e->peer;
	c->activity = cherokee_bogonow_now;
	if (!SSL_set_blocking_mode(ssl, 0) ||
	    !SSL_set_default_stream_mode(ssl, SSL_DEFAULT_STREAM_MODE_NONE) ||
	    !SSL_set_incoming_stream_policy(ssl, SSL_INCOMING_STREAM_POLICY_ACCEPT, 0)) {
		free_connection(c); return NULL;
	}
	cherokee_list_add_tail(&c->list, &e->connections);
	return c;
}

static int
init_http (cherokee_http3_connection_t *c)
{
	SSL *ssl = c->ssl;
	cherokee_http3_stream_t *control, *encoder, *decoder;
	nghttp3_settings settings;
	nghttp3_callbacks callbacks;
	memset(&callbacks, 0, sizeof(callbacks));
	callbacks.begin_headers = begin_headers;
	callbacks.recv_header = recv_header;
	callbacks.recv_data = recv_data;
	callbacks.end_stream = end_stream;
	callbacks.stream_close = stream_close;
	callbacks.reset_stream = reset_stream;
	callbacks.stop_sending = reset_stream;
	callbacks.acked_stream_data = acked_data;
	nghttp3_settings_default(&settings);
	settings.max_field_section_size = CHEROKEE_HTTP_STREAM_HEADERS;
	settings.qpack_max_dtable_capacity = 4096;
	settings.qpack_blocked_streams = 16;
	if (!SSL_set_blocking_mode(ssl, 0) ||
	    !SSL_set_default_stream_mode(ssl, SSL_DEFAULT_STREAM_MODE_NONE) ||
	    !SSL_set_incoming_stream_policy(ssl, SSL_INCOMING_STREAM_POLICY_ACCEPT, 0) ||
	    nghttp3_conn_server_new(&c->http, &callbacks, &settings, NULL, c)) goto error;
	nghttp3_conn_set_max_client_streams_bidi(c->http, CHEROKEE_HTTP_STREAM_MAX);
	control = add_stream(c, SSL_new_stream(ssl, SSL_STREAM_FLAG_UNI | SSL_STREAM_FLAG_NO_BLOCK));
	encoder = add_stream(c, SSL_new_stream(ssl, SSL_STREAM_FLAG_UNI | SSL_STREAM_FLAG_NO_BLOCK));
	decoder = add_stream(c, SSL_new_stream(ssl, SSL_STREAM_FLAG_UNI | SSL_STREAM_FLAG_NO_BLOCK));
	if (!control || !encoder || !decoder ||
	    nghttp3_conn_bind_control_stream(c->http, control->id) ||
	    nghttp3_conn_bind_qpack_streams(c->http, encoder->id, decoder->id)) goto error;
	return 0;
error:
	return -1;
}

static int
want_io (SSL *ssl, int result)
{
	int error = SSL_get_error(ssl, result);
	return error == SSL_ERROR_WANT_READ || error == SSL_ERROR_WANT_WRITE;
}

static int
step_connection (cherokee_http3_connection_t *c)
{
	cherokee_list_t *i, *tmp;
	unsigned char buf[16384];
	SSL *ssl;
	SSL_CONN_CLOSE_INFO info;
	int k;
	if (SSL_get_conn_close_info(c->ssl, &info, sizeof(info))) return -1;
	if (!c->http) {
		int result = SSL_do_handshake(c->ssl);
		if (result <= 0) return want_io(c->ssl, result) ? 0 : -1;
		if (init_http(c)) return -1;
	}
	for (k=0; k<16; k++) {
		ssl = SSL_accept_stream(c->ssl, SSL_ACCEPT_STREAM_NO_BLOCK);
		if (!ssl) break;
		{
			size_t count;
			cherokee_list_get_len(&c->streams, &count);
			if (count >= HTTP3_QUIC_STREAM_MAX) { SSL_free(ssl); return -1; }
		}
		if (!add_stream(c, ssl)) return -1;
	}
	list_for_each_safe (i, tmp, &c->streams) {
		cherokee_http3_stream_t *s = (cherokee_http3_stream_t *)i;
		int state;
		if (s->closed) continue;
		state = SSL_get_stream_read_state(s->ssl);
		if (state == SSL_STREAM_STATE_RESET_REMOTE || state == SSL_STREAM_STATE_RESET_LOCAL) {
			nghttp3_conn_close_stream(c->http, s->id, NGHTTP3_H3_REQUEST_CANCELLED);
			s->closed = 1;
			continue;
		}
		if (!s->read_fin && !SSL_is_stream_local(s->ssl)) {
			for (k=0; k<4; k++) {
				size_t n = 0;
				int result = SSL_read_ex(s->ssl, buf, sizeof(buf), &n);
				int fin = !result && SSL_get_error(s->ssl, result) == SSL_ERROR_ZERO_RETURN;
				if (!result && !fin) {
					if (!want_io(s->ssl, result)) return -1;
					break;
				}
				if (nghttp3_conn_read_stream(c->http, s->id, buf, n, fin) < 0) return -1;
				c->activity = cherokee_bogonow_now;
				if (fin) { s->read_fin = 1; break; }
			}
		}
		if (s->request && !s->offered && !s->closed) {
			int ready = cherokee_http_stream_step(s->request);
			if (ready < 0 || (ready > 0 && submit_response(c, s))) return -1;
			if (s->request->submitted && (s->request->response.len || s->request->eof))
				nghttp3_conn_resume_stream(c->http, s->id);
		}
	}
	/* OpenSSL copies accepted bytes into its bounded retransmission buffer.
	 * nghttp3 can release them at that ownership transfer; QUIC remains solely
	 * responsible for ACKs, packet loss, congestion control and retransmission.
	 */
	for (k=0; k<64; k++) {
		nghttp3_vec vec[16];
		int64_t id;
		int fin, j;
		size_t written = 0;
		nghttp3_ssize count = nghttp3_conn_writev_stream(c->http, &id, &fin, vec, 16);
		cherokee_http3_stream_t *s;
		if (count < 0) return -1;
		if (id < 0) break;
		s = find_stream(c, id);
		if (!s) return -1;
		for (j=0; j<count; j++) {
			size_t n = 0;
			int result = SSL_write_ex(s->ssl, vec[j].base, vec[j].len, &n);
			written += n;
			if (!result) {
				if (!want_io(s->ssl, result)) return -1;
				break;
			}
		}
		if (nghttp3_conn_add_write_offset(c->http, id, written) ||
		    nghttp3_conn_add_ack_offset(c->http, id, written)) return -1;
		if (written) c->activity = cherokee_bogonow_now;
		if (j < count) break;
		if (fin && !s->write_fin) {
			if (!SSL_stream_conclude(s->ssl, 0)) return -1;
			s->write_fin = 1;
			if (s->read_fin && s->request) {
				nghttp3_conn_close_stream(c->http, id, NGHTTP3_H3_NO_ERROR);
				s->closed = 1;
			}
		}
	}
	list_for_each_safe (i, tmp, &c->streams) {
		cherokee_http3_stream_t *s = (cherokee_http3_stream_t *)i;
		if (s->closed) free_stream(s);
	}
	return 0;
}

static void
free_endpoint (cherokee_http3_endpoint_t *e)
{
	cherokee_list_t *i, *tmp;
	cherokee_list_del(&e->list);
	list_for_each_safe(i, tmp, &e->connections) free_connection((cherokee_http3_connection_t *)i);
	SSL_free(e->listener);
	BIO_free(e->network);
	free(e);
}

static int
same_peer (cherokee_sockaddr_t *a, cherokee_sockaddr_t *b)
{
	if (a->sa.sa_family != b->sa.sa_family) return 0;
	if (a->sa.sa_family == AF_INET) return a->sa_in.sin_port == b->sa_in.sin_port && a->sa_in.sin_addr.s_addr == b->sa_in.sin_addr.s_addr;
#ifdef HAVE_IPV6
	if (a->sa.sa_family == AF_INET6) return a->sa_in6.sin6_port == b->sa_in6.sin6_port &&
		a->sa_in6.sin6_scope_id == b->sa_in6.sin6_scope_id && !memcmp(&a->sa_in6.sin6_addr, &b->sa_in6.sin6_addr, 16);
#endif
	return 0;
}

static cherokee_http3_endpoint_t *
new_endpoint (cherokee_http3_listener_t *h, cherokee_sockaddr_t *peer, socklen_t peer_len)
{
	cherokee_http3_endpoint_t *e = calloc(1, sizeof(*e));
	BIO *internal = NULL;
	BIO_ADDR *addr = BIO_ADDR_new();
	if (!e || !addr) { free(e); BIO_ADDR_free(addr); return NULL; }
	INIT_LIST_HEAD(&e->list); INIT_LIST_HEAD(&e->connections);
	e->peer = *peer; e->peer_len = peer_len; e->activity = cherokee_bogonow_now;
	/* OpenSSL 3.x has no QUIC peer-address getter. A bounded datagram BIO
	 * endpoint per source address preserves the authenticated peer identity.
	 * The TLS context is shared, including Retry token/address validation.
	 */
	if (peer->sa.sa_family == AF_INET) {
		if (!BIO_ADDR_rawmake(addr, AF_INET, &peer->sa_in.sin_addr, 4, peer->sa_in.sin_port)) goto error;
	} else {
#ifdef HAVE_IPV6
		if (!BIO_ADDR_rawmake(addr, AF_INET6, &peer->sa_in6.sin6_addr, 16, peer->sa_in6.sin6_port)) goto error;
#else
		goto error;
#endif
	}
	if (!BIO_new_bio_dgram_pair(&internal, 65536, &e->network, 65536) ||
	    !BIO_dgram_set_caps(e->network, BIO_DGRAM_CAP_HANDLES_DST_ADDR | BIO_DGRAM_CAP_PROVIDES_SRC_ADDR) ||
	    !BIO_dgram_set0_local_addr(e->network, addr)) goto error;
	addr = NULL; /* BIO owns it. */
	e->listener = SSL_new_listener(h->default_context, 0);
	if (!e->listener) goto error;
	SSL_set_bio(e->listener, internal, internal); internal = NULL;
	if (!SSL_set_blocking_mode(e->listener, 0) || !SSL_listen(e->listener)) goto error;
	cherokee_list_add_tail(&e->list, &h->endpoints);
	h->endpoint_count++;
	return e;
error:
	BIO_ADDR_free(addr); BIO_free(internal); free_endpoint(e);
	return NULL;
}

ret_t
cherokee_http3_init (cherokee_bind_t *listener, cherokee_server_t *server)
{
	cherokee_http3_listener_t *h;
	cherokee_list_t *i;
	cherokee_sockaddr_t local;
	socklen_t local_len = sizeof(local);
	const char *backend;
	if (!listener->http3) return ret_ok;
	if (!server->cryptor || cherokee_module_get_name(MODULE(server->cryptor), &backend) != ret_ok || strcmp(backend, "libssl")) {
		fprintf(stderr, "HTTP/3 requires the libssl TLS backend\n"); return ret_error;
	}
	h = calloc(1, sizeof(*h));
	if (!h) return ret_nomem;
	h->fd = -1; h->bind = listener; h->server = server;
	INIT_LIST_HEAD(&h->endpoints); INIT_LIST_HEAD(&h->contexts);
	listener->http3_listener = h;
	list_for_each(i, &server->vservers) {
		cherokee_virtual_server_t *vsrv = VSERVER(i);
		cherokee_http3_context_t *v;
		SSL_CTX *original;
		STACK_OF(X509) *chain = NULL;
		if (!vsrv->cryptor) continue;
		v = calloc(1, sizeof(*v));
		if (!v) goto error;
		INIT_LIST_HEAD(&v->list); v->vserver = vsrv;
		cherokee_list_add_tail(&v->list, &h->contexts);
		v->context = SSL_CTX_new(OSSL_QUIC_server_method());
		original = CRYPTOR_VSRV_SSL(vsrv->cryptor)->context;
		if (!v->context || !SSL_CTX_use_certificate(v->context, SSL_CTX_get0_certificate(original)) ||
		    !SSL_CTX_use_PrivateKey(v->context, SSL_CTX_get0_privatekey(original)) ||
		    !SSL_CTX_check_private_key(v->context)) goto error;
		SSL_CTX_get0_chain_certs(original, &chain);
		if (chain && !SSL_CTX_set1_chain(v->context, chain)) goto error;
		SSL_CTX_set1_cert_store(v->context, SSL_CTX_get_cert_store(original));
		SSL_CTX_set_verify(v->context, SSL_CTX_get_verify_mode(original), SSL_CTX_get_verify_callback(original));
		SSL_CTX_set_verify_depth(v->context, SSL_CTX_get_verify_depth(original));
		SSL_CTX_set_alpn_select_cb(v->context, alpn_select, NULL);
		SSL_CTX_set_tlsext_servername_callback(v->context, sni_select);
		SSL_CTX_set_tlsext_servername_arg(v->context, h);
		/* No application early data is accepted: replay-sensitive handlers
		 * must never run before handshake authentication completes. */
		SSL_CTX_set_max_early_data(v->context, 0);
		if (i == server->vservers.prev) h->default_context = v->context;
	}
	if (!h->default_context || getsockname(listener->socket.socket, &local.sa, &local_len)) goto error;
	h->fd = socket(local.sa.sa_family, SOCK_DGRAM, 0);
	if (h->fd < 0 || fcntl(h->fd, F_SETFL, O_NONBLOCK) < 0 || fcntl(h->fd, F_SETFD, FD_CLOEXEC) < 0 ||
	    bind(h->fd, &local.sa, local_len)) goto error;
	return ret_ok;
error:
	fprintf(stderr, "Cannot initialize native HTTP/3 listener\n");
	ERR_print_errors_fp(stderr);
	cherokee_http3_free(listener);
	return ret_error;
}

void
cherokee_http3_step (cherokee_bind_t *bind, cherokee_thread_t *thread)
{
	cherokee_http3_listener_t *h = bind->http3_listener;
	cherokee_list_t *i, *tmp;
	unsigned char packet[65536];
	int k;
	if (!h) return;
	if (!h->registered) {
		if (cherokee_fdpoll_add(thread->fdpoll, h->fd, FDPOLL_MODE_READ) != ret_ok) return;
		h->registered = 1;
	}
	for (k=0; k<64; k++) {
		cherokee_sockaddr_t peer;
		socklen_t len = sizeof(peer);
		ssize_t n = recvfrom(h->fd, packet, sizeof(packet), 0, &peer.sa, &len);
		cherokee_http3_endpoint_t *e = NULL;
		if (n < 0) break;
		list_for_each(i, &h->endpoints) {
			cherokee_http3_endpoint_t *candidate = (cherokee_http3_endpoint_t *)i;
			if (same_peer(&candidate->peer, &peer)) { e = candidate; break; }
		}
		if (!e) {
			/* Do not allocate state for short, undersized or non-Initial traffic. */
			if (n < 1200 || (packet[0] & 0xf0) != 0xc0 || packet[1] || packet[2] || packet[3] || packet[4] != 1 ||
			    h->endpoint_count >= HTTP3_ENDPOINT_MAX) continue;
			e = new_endpoint(h, &peer, len);
			if (!e) continue;
		}
		if (BIO_write(e->network, packet, n) > 0) e->activity = cherokee_bogonow_now;
	}
	list_for_each_safe(i, tmp, &h->endpoints) {
		cherokee_http3_endpoint_t *e = (cherokee_http3_endpoint_t *)i;
		cherokee_list_t *j, *next;
		SSL *ssl;
		SSL_handle_events(e->listener);
		for (k=0; k<8; k++) {
			size_t count;
			ssl = SSL_accept_connection(e->listener, SSL_ACCEPT_CONNECTION_NO_BLOCK);
			if (!ssl) break;
			cherokee_list_get_len(&e->connections, &count);
			if (count >= 8 || !new_connection(ssl, h, e, thread)) { if (count >= 8) SSL_free(ssl); break; }
		}
		list_for_each_safe(j, next, &e->connections) {
			cherokee_http3_connection_t *c = (cherokee_http3_connection_t *)j;
			if (step_connection(c) || c->activity + h->server->timeout < cherokee_bogonow_now) {
				SSL_SHUTDOWN_EX_ARGS args = {NGHTTP3_H3_INTERNAL_ERROR, "HTTP/3 session closed"};
				SSL_shutdown_ex(c->ssl, SSL_SHUTDOWN_FLAG_NO_BLOCK | SSL_SHUTDOWN_FLAG_RAPID, &args, sizeof(args));
				free_connection(c);
			}
		}
		SSL_handle_events(e->listener);
		for (k=0; k<64; k++) {
			ssize_t sent;
			if (!e->pending_len) {
				int n = BIO_read(e->network, e->pending, sizeof(e->pending));
				if (n <= 0) break;
				e->pending_len = n;
			}
			sent = sendto(h->fd, e->pending, e->pending_len, 0, &e->peer.sa, e->peer_len);
			if (sent < 0 && (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR)) break;
			e->pending_len = 0;
		}
		if (cherokee_list_empty(&e->connections) && e->activity + 5 < cherokee_bogonow_now) {
			free_endpoint(e); h->endpoint_count--;
		}
	}
}

void
cherokee_http3_free (cherokee_bind_t *bind)
{
	cherokee_http3_listener_t *h = bind->http3_listener;
	cherokee_list_t *i, *tmp;
	if (!h) return;
	bind->http3_listener = NULL;
	list_for_each_safe(i, tmp, &h->endpoints) free_endpoint((cherokee_http3_endpoint_t *)i);
	list_for_each_safe(i, tmp, &h->contexts) {
		cherokee_http3_context_t *v = (cherokee_http3_context_t *)i;
		cherokee_list_del(&v->list); SSL_CTX_free(v->context); free(v);
	}
	if (h->fd >= 0) close(h->fd);
	free(h);
}
#else
ret_t cherokee_http3_init (cherokee_bind_t *b, cherokee_server_t *s) { UNUSED(s); return b->http3 ? ret_no_sys : ret_ok; }
void cherokee_http3_step (cherokee_bind_t *b, cherokee_thread_t *t) { UNUSED(b); UNUSED(t); }
void cherokee_http3_free (cherokee_bind_t *b) { UNUSED(b); }
#endif
