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
#include <time.h>

#define HTTP3_ENDPOINT_MAX 128
#define HTTP3_QUIC_STREAM_MAX (CHEROKEE_HTTP_STREAM_MAX + 6)

typedef struct {
	cherokee_list_t list;
	SSL *ssl;
	cherokee_http_stream_t *request;
	uint64_t id;
	size_t offered, acknowledged;
	int read_fin, write_fin, closed, write_blocked;
	uint64_t reset_error;
	nghttp3_qpack_stream_context *fields_context;
	cherokee_buffer_t encoded_fields;
	size_t encoded_charge;
	uint64_t frame_type, frame_left, varint;
	unsigned int varint_left, frame_phase;
} cherokee_http3_stream_t;

typedef struct {
	cherokee_list_t list;
	SSL *ssl;
	nghttp3_conn *http;
	nghttp3_qpack_decoder *field_decoder;
	cherokee_list_t streams;
	cherokee_bind_t *bind;
	cherokee_thread_t *thread;
	cherokee_sockaddr_t peer;
	cherokee_sockaddr_t local;
	time_t activity;
	time_t closing_until;
	uint64_t close_error;
	void *tls_vserver;
	void *owner;
} cherokee_http3_connection_t;

typedef struct {
	cherokee_list_t list;
	SSL *listener;
	BIO *network;
	cherokee_sockaddr_t peer;
	cherokee_sockaddr_t local;
	unsigned int interface_index;
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
	cherokee_http3_endpoint_t *current_endpoint;
	size_t connection_count;
	cherokee_sockaddr_t local;
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
	conn.socket.native_stream = true;
	/* Callbacks run synchronously while the main thread services this
	 * endpoint, including callbacks on OpenSSL's inner TLS object. */
	if (h->current_endpoint) {
		conn.socket.client_addr = h->current_endpoint->peer;
		conn.socket.native_local_addr = h->current_endpoint->local;
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
		STACK_OF(X509_NAME) *authorities = SSL_CTX_get_client_CA_list(ctx);
		STACK_OF(X509_NAME) *copy = authorities ? SSL_dup_CA_list(authorities) : NULL;
		if (authorities && !copy) return SSL_TLSEXT_ERR_ALERT_FATAL;
		SSL_set_client_CA_list(ssl, copy);
		if (!SSL_use_certificate(ssl, SSL_CTX_get0_certificate(ctx)) ||
		    !SSL_use_PrivateKey(ssl, SSL_CTX_get0_privatekey(ctx))) return SSL_TLSEXT_ERR_ALERT_FATAL;
		SSL_CTX_get0_chain_certs(ctx, &chain);
		if (!SSL_set1_chain(ssl, chain)) return SSL_TLSEXT_ERR_ALERT_FATAL;
		if (!SSL_set1_verify_cert_store(ssl, SSL_CTX_get_cert_store(ctx))) return SSL_TLSEXT_ERR_ALERT_FATAL;
		if (!SSL_set1_param(ssl, SSL_CTX_get0_param(ctx))) return SSL_TLSEXT_ERR_ALERT_FATAL;
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
	if (!s || !s->request) return NGHTTP3_ERR_CALLBACK_FAILURE;
	nghttp3_conn_set_stream_user_data(http, id, s);
	return 0;
}

static int
recv_header (nghttp3_conn *http, int64_t id, int32_t token, nghttp3_rcbuf *name,
             nghttp3_rcbuf *value, uint8_t flags, void *user, void *stream_user)
{
	cherokee_http3_stream_t *s = find_stream(user, id);
	UNUSED(http); UNUSED(token); UNUSED(name); UNUSED(value); UNUSED(flags); UNUSED(stream_user);
	if (!s || !s->request) return NGHTTP3_ERR_CALLBACK_FAILURE;
	/* All decoded fields, including ones nghttp3 might discard, have
	 * already passed the strict field-section validator below. */
	return 0;
}

static int
end_headers (nghttp3_conn *http, int64_t id, int fin, void *user, void *stream_user)
{
	cherokee_http3_stream_t *s = find_stream(user, id);
	UNUSED(http); UNUSED(fin); UNUSED(stream_user);
	if (!s || !s->request) return NGHTTP3_ERR_CALLBACK_FAILURE;
	return 0;
}

static int
recv_data (nghttp3_conn *http, int64_t id, const uint8_t *data, size_t len,
           void *user, void *stream_user)
{
	cherokee_http3_stream_t *s = find_stream(user, id);
	UNUSED(http); UNUSED(stream_user);
	if (!s || !s->request) return NGHTTP3_ERR_CALLBACK_FAILURE;
	if (s->request->invalid || cherokee_http_stream_data(s->request, data, len)) s->request->invalid = 1;
	return 0;
}

static int
end_stream (nghttp3_conn *http, int64_t id, void *user, void *stream_user)
{
	cherokee_http3_connection_t *c = user;
	cherokee_http3_stream_t *s = find_stream(c, id);
	UNUSED(http); UNUSED(stream_user);
	if (!s || !s->request) return NGHTTP3_ERR_CALLBACK_FAILURE;
	if (s->request->invalid || cherokee_http_stream_dispatch(s->request, c->bind, &c->peer, &c->local, http_version_30, c->tls_vserver)) s->request->invalid = 1;
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
		s->reset_error = error;
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
	nghttp3_qpack_stream_context_del(s->fields_context);
	cherokee_buffer_mrproper(&s->encoded_fields);
	cherokee_http_stream_free(s->request);
	free(s);
}

static void
free_connection (cherokee_http3_connection_t *c)
{
	cherokee_list_t *i, *tmp;
	cherokee_list_del(&c->list);
	if (c->owner) ((cherokee_http3_listener_t *)c->owner)->connection_count--;
	nghttp3_conn_del(c->http);
	nghttp3_qpack_decoder_del(c->field_decoder);
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
	SSL_set_mode(ssl, SSL_MODE_ENABLE_PARTIAL_WRITE | SSL_MODE_ACCEPT_MOVING_WRITE_BUFFER);
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
	c->local = e->local;
	c->activity = cherokee_bogonow_now;
	c->close_error = NGHTTP3_H3_INTERNAL_ERROR;
	if (!SSL_set_blocking_mode(ssl, 0) ||
	    !SSL_set_default_stream_mode(ssl, SSL_DEFAULT_STREAM_MODE_NONE) ||
	    !SSL_set_incoming_stream_policy(ssl, SSL_INCOMING_STREAM_POLICY_ACCEPT, 0)) {
		free_connection(c); return NULL;
	}
	c->owner = h;
	h->connection_count++;
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
	callbacks.end_headers = end_headers;
	callbacks.recv_trailer = recv_header;
	callbacks.recv_data = recv_data;
	callbacks.end_stream = end_stream;
	callbacks.stream_close = stream_close;
	callbacks.reset_stream = reset_stream;
	callbacks.stop_sending = reset_stream;
	callbacks.acked_stream_data = acked_data;
	nghttp3_settings_default(&settings);
	settings.max_field_section_size = CHEROKEE_HTTP_STREAM_HEADERS;
	/* No dynamic request table: it lets the strict validator inspect each
	 * complete field section before nghttp3's HTTP messaging filter can
	 * silently discard invalid optional fields. Static QPACK and Huffman
	 * coding remain supported. Response compression uses nghttp3 normally. */
	settings.qpack_max_dtable_capacity = 0;
	settings.qpack_blocked_streams = 0;
	if (!SSL_set_blocking_mode(ssl, 0) ||
	    !SSL_set_default_stream_mode(ssl, SSL_DEFAULT_STREAM_MODE_NONE) ||
	    !SSL_set_incoming_stream_policy(ssl, SSL_INCOMING_STREAM_POLICY_ACCEPT, 0) ||
	    nghttp3_conn_server_new(&c->http, &callbacks, &settings, NULL, c) ||
	    nghttp3_qpack_decoder_new(&c->field_decoder, 0, 0, nghttp3_mem_default())) goto error;
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
protocol_error (cherokee_http3_connection_t *c, int error)
{
	c->close_error = nghttp3_err_infer_quic_app_error_code(error);
	return -1;
}

static int
validate_fields (cherokee_http3_connection_t *c, cherokee_http3_stream_t *s)
{
	size_t offset = 0;
	const uint8_t *bytes = (const uint8_t *)(s->encoded_fields.buf ? s->encoded_fields.buf : "");
	s->request->charged -= s->encoded_charge;
	c->thread->native_request_bytes -= s->encoded_charge;
	s->encoded_charge = 0;
	for (;;) {
		nghttp3_qpack_nv field;
		uint8_t flags;
		nghttp3_ssize n = nghttp3_qpack_decoder_read_request(c->field_decoder,
		    s->fields_context, &field, &flags, bytes+offset, s->encoded_fields.len-offset, 1);
		if (n < 0) return protocol_error(c, n);
		offset += n;
		if (flags & NGHTTP3_QPACK_DECODE_FLAG_EMIT) {
			nghttp3_vec name = nghttp3_rcbuf_get_buf(field.name);
			nghttp3_vec value = nghttp3_rcbuf_get_buf(field.value);
			int invalid = cherokee_http_stream_header(s->request, name.base, name.len, value.base, value.len);
			nghttp3_rcbuf_decref(field.name); nghttp3_rcbuf_decref(field.value);
			if (invalid) { s->request->invalid = 1; return 1; }
		}
		if (flags & NGHTTP3_QPACK_DECODE_FLAG_FINAL) break;
		if ((flags & NGHTTP3_QPACK_DECODE_FLAG_BLOCKED) || (!n && !flags))
			return protocol_error(c, NGHTTP3_ERR_QPACK_DECOMPRESSION_FAILED);
	}
	s->request->headers_complete = 1;
	nghttp3_qpack_stream_context_reset(s->fields_context);
	cherokee_buffer_mrproper(&s->encoded_fields);
	return 0;
}

/* Scan only the frame envelope on remote request streams. nghttp3 retains
 * responsibility for HTTP/3 frame sequencing and all control streams. */
static int
validate_request_bytes (cherokee_http3_connection_t *c, cherokee_http3_stream_t *s,
                        const uint8_t *bytes, size_t length)
{
	if ((s->id & 3) != 0) return 0;
	while (length) {
		if (s->frame_phase < 2) {
			if (!s->varint_left) {
				s->varint_left = (1U << (bytes[0] >> 6)) - 1;
				s->varint = bytes[0] & 63;
				bytes++; length--;
				if (s->varint_left) continue;
			} else {
				s->varint = (s->varint << 8) | *bytes++;
				length--;
				if (--s->varint_left) continue;
			}
			if (s->frame_phase == 0) { s->frame_type = s->varint; s->frame_phase = 1; }
			else {
				s->frame_left = s->varint; s->frame_phase = 2;
				if (s->frame_type == 1) {
					if (!s->request) s->request = cherokee_http_stream_new(c->thread, s->id);
					if (!s->request) return -1;
					if (!s->fields_context && nghttp3_qpack_stream_context_new(&s->fields_context, s->id, nghttp3_mem_default())) return -1;
					if (s->frame_left > CHEROKEE_HTTP_STREAM_HEADERS) { s->request->invalid = 1; return 1; }
				}
				if (!s->frame_left) {
					if (s->frame_type == 1) { int ret = validate_fields(c, s); if (ret) return ret; }
					s->frame_phase = 0;
				}
			}
		} else {
			size_t n = length < s->frame_left ? length : (size_t)s->frame_left;
			if (s->frame_type == 1) {
				if (n > CHEROKEE_HTTP_STREAM_BUDGET - c->thread->native_request_bytes) { s->request->invalid = 1; return 1; }
				s->encoded_charge += n; s->request->charged += n;
				c->thread->native_request_bytes += n;
				if (cherokee_buffer_add(&s->encoded_fields, (const char *)bytes, n) != ret_ok) return -1;
			}
			bytes += n; length -= n; s->frame_left -= n;
			if (!s->frame_left) {
				if (s->frame_type == 1) { int ret = validate_fields(c, s); if (ret) return ret; }
				s->frame_phase = 0;
			}
		}
	}
	return 0;
}

static int
step_connection (cherokee_http3_connection_t *c)
{
	cherokee_list_t *i, *tmp;
	unsigned char buf[16384];
	SSL *ssl;
	SSL_CONN_CLOSE_INFO info;
	int k;
	struct timespec now;
	nghttp3_tstamp timestamp;
	if (clock_gettime(CLOCK_MONOTONIC, &now)) return -1;
	timestamp = (nghttp3_tstamp)now.tv_sec * 1000000000ULL + now.tv_nsec;
	if (SSL_get_conn_close_info(c->ssl, &info, sizeof(info))) return -1;
	if (!c->http) {
		int result;
		ERR_clear_error();
		result = SSL_do_handshake(c->ssl);
		if (result <= 0) return want_io(c->ssl, result) ? 0 : -1;
		{
			cherokee_connection_t conn;
			cherokee_buffer_t host = CHEROKEE_BUF_INIT;
			cherokee_virtual_server_t *vsrv;
			const char *name = SSL_get_servername(c->ssl, TLSEXT_NAMETYPE_host_name);
			memset(&conn, 0, sizeof(conn));
			conn.server = THREAD_SRV(c->thread); conn.bind = c->bind;
			conn.socket.is_tls = TLS; conn.socket.client_addr = c->peer;
			conn.socket.native_stream = true; conn.socket.native_local_addr = c->local;
			c->tls_vserver = VSERVER(THREAD_SRV(c->thread)->vservers.prev);
			if (name) {
				int failed = cherokee_buffer_add(&host, name, strlen(name)) != ret_ok ||
				    cherokee_server_get_vserver(THREAD_SRV(c->thread), &host, &conn, &vsrv) != ret_ok;
				cherokee_buffer_mrproper(&host);
				if (failed) return -1;
				c->tls_vserver = vsrv;
			}
		}
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
		int state, write_state;
		if (s->closed) continue;
		state = SSL_get_stream_read_state(s->ssl);
		write_state = SSL_get_stream_write_state(s->ssl);
		if (state == SSL_STREAM_STATE_RESET_REMOTE || state == SSL_STREAM_STATE_RESET_LOCAL ||
		    write_state == SSL_STREAM_STATE_RESET_REMOTE || write_state == SSL_STREAM_STATE_RESET_LOCAL) {
			int error = nghttp3_conn_close_stream(c->http, s->id, NGHTTP3_H3_REQUEST_CANCELLED);
			if (error < 0 && error != NGHTTP3_ERR_STREAM_NOT_FOUND) return protocol_error(c, error);
			s->closed = 1;
			continue;
		}
		if (s->write_blocked) {
			SSL_POLL_ITEM item;
			struct timeval timeout = {0, 0};
			memset(&item, 0, sizeof(item));
			item.desc = SSL_as_poll_descriptor(s->ssl);
			item.events = SSL_POLL_EVENT_WE;
			if (!SSL_poll(&item, 1, sizeof(item), &timeout, SSL_POLL_FLAG_NO_HANDLE_EVENTS, NULL)) return -1;
			if (item.revents & SSL_POLL_EVENT_W) {
				int error = nghttp3_conn_unblock_stream(c->http, s->id);
				if (error < 0) return protocol_error(c, error);
				s->write_blocked = 0;
			}
		}
		if (!s->read_fin && !SSL_is_stream_local(s->ssl)) {
			for (k=0; k<4; k++) {
				size_t n = 0;
				int result;
				ERR_clear_error();
				result = SSL_read_ex(s->ssl, buf, sizeof(buf), &n);
				int fin = !result && SSL_get_error(s->ssl, result) == SSL_ERROR_ZERO_RETURN;
				if (!result && !fin) {
					if (!want_io(s->ssl, result)) return -1;
					break;
				}
				{
					int validation = validate_request_bytes(c, s, buf, n);
					if (validation < 0) return -1;
					if (validation > 0) break;
					nghttp3_ssize consumed = nghttp3_conn_read_stream2(c->http, s->id, buf, n, fin, timestamp);
					if (consumed < 0) return protocol_error(c, consumed);
				}
				if (s->closed || s->reset_error || (s->request && s->request->invalid)) break;
				c->activity = cherokee_bogonow_now;
				if (fin) { s->read_fin = 1; break; }
			}
		}
		if (!s->closed && (s->reset_error || (s->request && s->request->invalid))) {
			SSL_STREAM_RESET_ARGS args = {s->reset_error ? s->reset_error : NGHTTP3_H3_MESSAGE_ERROR};
			int error;
			if (!SSL_stream_reset(s->ssl, &args, sizeof(args))) return -1;
			error = nghttp3_conn_close_stream(c->http, s->id, args.quic_error_code);
			if (error < 0 && error != NGHTTP3_ERR_STREAM_NOT_FOUND) return protocol_error(c, error);
			s->closed = 1;
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
		if (count < 0) return protocol_error(c, count);
		if (id < 0) break;
		s = find_stream(c, id);
		if (!s) return -1;
		for (j=0; j<count; j++) {
			size_t n = 0;
			int result;
			ERR_clear_error();
			result = SSL_write_ex(s->ssl, vec[j].base, vec[j].len, &n);
			written += n;
			if (!result) {
				if (!want_io(s->ssl, result)) return -1;
				s->write_blocked = 1;
				nghttp3_conn_block_stream(c->http, id);
				break;
			}
			if (n < vec[j].len) {
				s->write_blocked = 1;
				nghttp3_conn_block_stream(c->http, id);
				break;
			}
		}
		if (nghttp3_conn_add_write_offset(c->http, id, written) ||
		    nghttp3_conn_add_ack_offset(c->http, id, written)) return -1;
		if (written) c->activity = cherokee_bogonow_now;
		if (j < count) continue;
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
		else if (s->request && cherokee_http_stream_update_poll(s->request, !s->offered)) return -1;
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
new_endpoint (cherokee_http3_listener_t *h, cherokee_sockaddr_t *peer, socklen_t peer_len,
              cherokee_sockaddr_t *local, unsigned int interface_index)
{
	cherokee_http3_endpoint_t *e = calloc(1, sizeof(*e));
	BIO *internal = NULL;
	BIO_ADDR *addr = BIO_ADDR_new();
	if (!e || !addr) { free(e); BIO_ADDR_free(addr); return NULL; }
	INIT_LIST_HEAD(&e->list); INIT_LIST_HEAD(&e->connections);
	e->peer = *peer; e->peer_len = peer_len; e->activity = cherokee_bogonow_now;
	e->local = *local; e->interface_index = interface_index;
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
		if (!SSL_CTX_set1_param(v->context, SSL_CTX_get0_param(original))) goto error;
		{
			STACK_OF(X509_NAME) *authorities = SSL_CTX_get_client_CA_list(original);
			STACK_OF(X509_NAME) *copy = authorities ? SSL_dup_CA_list(authorities) : NULL;
			if (authorities && !copy) goto error;
			SSL_CTX_set_client_CA_list(v->context, copy);
		}
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
	h->local = local;
	h->fd = socket(local.sa.sa_family, SOCK_DGRAM, 0);
#if defined(HAVE_IPV6) && defined(IPV6_V6ONLY)
	if (h->fd >= 0 && local.sa.sa_family == AF_INET6) {
		int only;
		socklen_t option_len = sizeof(only);
		if (getsockopt(listener->socket.socket, IPPROTO_IPV6, IPV6_V6ONLY, &only, &option_len) ||
		    setsockopt(h->fd, IPPROTO_IPV6, IPV6_V6ONLY, &only, sizeof(only))) goto error;
	}
#endif
	if (h->fd < 0 || fcntl(h->fd, F_SETFL, O_NONBLOCK) < 0 || fcntl(h->fd, F_SETFD, FD_CLOEXEC) < 0 ||
	    bind(h->fd, &local.sa, local_len)) goto error;
	{
		int enabled = 1;
#ifdef IP_PKTINFO
		if (local.sa.sa_family == AF_INET && setsockopt(h->fd, IPPROTO_IP, IP_PKTINFO, &enabled, sizeof(enabled))) goto error;
#endif
#if defined(HAVE_IPV6) && defined(IPV6_RECVPKTINFO)
		if (local.sa.sa_family == AF_INET6 && setsockopt(h->fd, IPPROTO_IPV6, IPV6_RECVPKTINFO, &enabled, sizeof(enabled))) goto error;
#endif
		UNUSED(enabled);
	}
	return ret_ok;
error:
	fprintf(stderr, "Cannot initialize native HTTP/3 listener\n");
	ERR_print_errors_fp(stderr);
	cherokee_http3_free(listener);
	return ret_error;
}

static ssize_t
receive_packet (cherokee_http3_listener_t *h, unsigned char *packet, size_t size,
                cherokee_sockaddr_t *peer, socklen_t *peer_len,
                cherokee_sockaddr_t *local, unsigned int *interface_index)
{
	union { struct cmsghdr alignment; unsigned char bytes[256]; } control;
	struct msghdr message;
	struct iovec vector = {packet, size};
	struct cmsghdr *field;
	ssize_t n;
	memset(&message, 0, sizeof(message));
	message.msg_name = &peer->sa; message.msg_namelen = *peer_len;
	message.msg_iov = &vector; message.msg_iovlen = 1;
	message.msg_control = control.bytes; message.msg_controllen = sizeof(control.bytes);
	n = recvmsg(h->fd, &message, 0);
	if (n < 0) return n;
	if (message.msg_flags & (MSG_TRUNC | MSG_CTRUNC)) return 0;
	*peer_len = message.msg_namelen; *local = h->local; *interface_index = 0;
	for (field = CMSG_FIRSTHDR(&message); field; field = CMSG_NXTHDR(&message, field)) {
#ifdef IP_PKTINFO
		if (field->cmsg_level == IPPROTO_IP && field->cmsg_type == IP_PKTINFO &&
		    field->cmsg_len >= CMSG_LEN(sizeof(struct in_pktinfo)) && local->sa.sa_family == AF_INET) {
			struct in_pktinfo info;
			memcpy(&info, CMSG_DATA(field), sizeof(info));
			local->sa_in.sin_addr = info.ipi_addr;
			*interface_index = info.ipi_ifindex;
		}
#endif
#if defined(HAVE_IPV6) && defined(IPV6_PKTINFO)
		if (field->cmsg_level == IPPROTO_IPV6 && field->cmsg_type == IPV6_PKTINFO &&
		    field->cmsg_len >= CMSG_LEN(sizeof(struct in6_pktinfo)) && local->sa.sa_family == AF_INET6) {
			struct in6_pktinfo info;
			memcpy(&info, CMSG_DATA(field), sizeof(info));
			local->sa_in6.sin6_addr = info.ipi6_addr;
			local->sa_in6.sin6_scope_id = IN6_IS_ADDR_LINKLOCAL(&info.ipi6_addr) ? info.ipi6_ifindex : 0;
			*interface_index = info.ipi6_ifindex;
		}
#endif
	}
	return n;
}

static ssize_t
send_packet (cherokee_http3_listener_t *h, cherokee_http3_endpoint_t *e)
{
	union { struct cmsghdr alignment; unsigned char bytes[256]; } control;
	struct msghdr message;
	struct iovec vector = {e->pending, e->pending_len};
	memset(&message, 0, sizeof(message));
	memset(&control, 0, sizeof(control));
	message.msg_name = &e->peer.sa; message.msg_namelen = e->peer_len;
	message.msg_iov = &vector; message.msg_iovlen = 1;
#ifdef IP_PKTINFO
	if (e->local.sa.sa_family == AF_INET) {
		struct in_pktinfo info;
		struct cmsghdr *field;
		memset(&info, 0, sizeof(info));
		info.ipi_spec_dst = e->local.sa_in.sin_addr; info.ipi_ifindex = e->interface_index;
		message.msg_control = control.bytes; message.msg_controllen = CMSG_SPACE(sizeof(info));
		field = CMSG_FIRSTHDR(&message);
		field->cmsg_level = IPPROTO_IP; field->cmsg_type = IP_PKTINFO; field->cmsg_len = CMSG_LEN(sizeof(info));
		memcpy(CMSG_DATA(field), &info, sizeof(info));
	}
#endif
#if defined(HAVE_IPV6) && defined(IPV6_PKTINFO)
	if (e->local.sa.sa_family == AF_INET6) {
		struct in6_pktinfo info;
		struct cmsghdr *field;
		memset(&info, 0, sizeof(info));
		info.ipi6_addr = e->local.sa_in6.sin6_addr; info.ipi6_ifindex = e->interface_index;
		message.msg_control = control.bytes; message.msg_controllen = CMSG_SPACE(sizeof(info));
		field = CMSG_FIRSTHDR(&message);
		field->cmsg_level = IPPROTO_IPV6; field->cmsg_type = IPV6_PKTINFO; field->cmsg_len = CMSG_LEN(sizeof(info));
		memcpy(CMSG_DATA(field), &info, sizeof(info));
	}
#endif
	return sendmsg(h->fd, &message, 0);
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
		cherokee_sockaddr_t local;
		unsigned int interface_index;
		socklen_t len = sizeof(peer);
		ssize_t n = receive_packet(h, packet, sizeof(packet), &peer, &len, &local, &interface_index);
		cherokee_http3_endpoint_t *e = NULL;
		if (n < 0) break;
		if (!n) continue;
		list_for_each(i, &h->endpoints) {
			cherokee_http3_endpoint_t *candidate = (cherokee_http3_endpoint_t *)i;
			if (same_peer(&candidate->peer, &peer) && same_peer(&candidate->local, &local)) { e = candidate; break; }
		}
		if (!e) {
			/* Do not allocate state for short, undersized or non-Initial traffic. */
			if (n < 1200 || (packet[0] & 0xf0) != 0xc0 || packet[1] || packet[2] || packet[3] || packet[4] != 1 ||
			    h->endpoint_count >= HTTP3_ENDPOINT_MAX) continue;
			e = new_endpoint(h, &peer, len, &local, interface_index);
			if (!e) continue;
		}
		if (BIO_write(e->network, packet, n) > 0) e->activity = cherokee_bogonow_now;
	}
	list_for_each_safe(i, tmp, &h->endpoints) {
		cherokee_http3_endpoint_t *e = (cherokee_http3_endpoint_t *)i;
		cherokee_list_t *j, *next;
		SSL *ssl;
		h->current_endpoint = e;
		ERR_clear_error();
		SSL_handle_events(e->listener);
		for (k=0; k<8; k++) {
			size_t count;
			ssl = SSL_accept_connection(e->listener, SSL_ACCEPT_CONNECTION_NO_BLOCK);
			if (!ssl) break;
			cherokee_list_get_len(&e->connections, &count);
			if (count >= 8 || h->connection_count >= HTTP3_ENDPOINT_MAX) {
				SSL_free(ssl); break;
			}
			if (!new_connection(ssl, h, e, thread)) break;
		}
		list_for_each_safe(j, next, &e->connections) {
			cherokee_http3_connection_t *c = (cherokee_http3_connection_t *)j;
			if (c->closing_until) {
				SSL_handle_events(c->ssl);
				if (cherokee_bogonow_now >= c->closing_until) free_connection(c);
				continue;
			}
			if (step_connection(c) || c->activity + h->server->timeout < cherokee_bogonow_now) {
				cherokee_list_t *stream, *stream_next;
				SSL_SHUTDOWN_EX_ARGS args = {c->close_error, "HTTP/3 session closed"};
				SSL_shutdown_ex(c->ssl, SSL_SHUTDOWN_FLAG_NO_BLOCK | SSL_SHUTDOWN_FLAG_RAPID, &args, sizeof(args));
				/* Retain QUIC state briefly so OpenSSL can transmit the close
				 * packet. Immediate SSL_free can discard that queued packet.
				 * Release application state and handlers immediately. */
				nghttp3_conn_del(c->http); c->http = NULL;
				list_for_each_safe(stream, stream_next, &c->streams) {
					cherokee_http3_stream_t *s = (cherokee_http3_stream_t *)stream;
					SSL_STREAM_RESET_ARGS reset = {c->close_error};
					if (!s->write_fin && SSL_get_stream_write_state(s->ssl) == SSL_STREAM_STATE_OK)
						SSL_stream_reset(s->ssl, &reset, sizeof(reset));
					free_stream(s);
				}
				c->closing_until = cherokee_bogonow_now + 3;
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
			sent = send_packet(h, e);
			if (sent < 0 && (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR)) break;
			e->pending_len = 0;
		}
		h->current_endpoint = NULL;
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
