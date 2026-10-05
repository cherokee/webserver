/* Cherokee native HTTP/2 (RFC 9113). GPL-2.0-only. */
#include "common-internal.h"
#include "http2.h"
#include "http-stream.h"
#ifdef HAVE_HTTP2
#include <nghttp2/nghttp2.h>

typedef struct {
	nghttp2_session *session;
	cherokee_connection_t *connection;
	cherokee_list_t streams;
	cherokee_buffer_t output;
	size_t stream_count;
	cherokee_sockaddr_t local;
} cherokee_http2_t;

static cherokee_http_stream_t *
find_stream (cherokee_http2_t *h, int32_t id)
{
	return nghttp2_session_get_stream_user_data(h->session, id);
}

static int
begin_headers (nghttp2_session *session, const nghttp2_frame *frame, void *data)
{
	cherokee_http2_t *h = data;
	cherokee_http_stream_t *s;
	if (frame->headers.cat != NGHTTP2_HCAT_REQUEST) return 0;
	if (h->stream_count >= CHEROKEE_HTTP_STREAM_MAX) return NGHTTP2_ERR_TEMPORAL_CALLBACK_FAILURE;
	s = cherokee_http_stream_new (CONN_THREAD(h->connection), frame->hd.stream_id);
	if (!s) return NGHTTP2_ERR_CALLBACK_FAILURE;
	cherokee_list_add_tail (&s->list, &h->streams);
	h->stream_count++;
	nghttp2_session_set_stream_user_data(session, frame->hd.stream_id, s);
	return 0;
}

static int
recv_header (nghttp2_session *session, const nghttp2_frame *frame,
             const uint8_t *name, size_t nlen, const uint8_t *value,
             size_t vlen, uint8_t flags, void *data)
{
	cherokee_http2_t *h = data;
	cherokee_http_stream_t *s = find_stream(h, frame->hd.stream_id);
	UNUSED(session); UNUSED(flags);
	if (frame->headers.cat != NGHTTP2_HCAT_REQUEST && frame->headers.cat != NGHTTP2_HCAT_HEADERS) return 0;
	if (cherokee_http_stream_header(s, name, nlen, value, vlen)) {
		if (s) s->invalid = 1;
		return NGHTTP2_ERR_TEMPORAL_CALLBACK_FAILURE;
	}
	return 0;
}

static int
invalid_header (nghttp2_session *session, const nghttp2_frame *frame,
                const uint8_t *name, size_t nlen, const uint8_t *value,
                size_t vlen, uint8_t flags, void *data)
{
	cherokee_http_stream_t *s = find_stream(data, frame->hd.stream_id);
	UNUSED(session); UNUSED(name); UNUSED(nlen); UNUSED(value); UNUSED(vlen); UNUSED(flags);
	if (s) s->invalid = 1;
	return NGHTTP2_ERR_TEMPORAL_CALLBACK_FAILURE;
}

static int
recv_data (nghttp2_session *session, uint8_t flags, int32_t id,
           const uint8_t *data, size_t len, void *user)
{
	cherokee_http2_t *h = user;
	cherokee_http_stream_t *s = find_stream(h, id);
	UNUSED(flags);
	if (cherokee_http_stream_data(s, data, len)) {
		if (s) s->invalid = 1;
		nghttp2_submit_rst_stream(session, NGHTTP2_FLAG_NONE, id, NGHTTP2_ENHANCE_YOUR_CALM);
	}
	return 0;
}

static int
recv_frame (nghttp2_session *session, const nghttp2_frame *frame, void *data)
{
	cherokee_http2_t *h = data;
	cherokee_http_stream_t *s;
	if (frame->hd.type == NGHTTP2_HEADERS && frame->headers.cat == NGHTTP2_HCAT_REQUEST) {
		s = find_stream(h, frame->hd.stream_id);
		if (s) s->headers_complete = 1;
	}
	if ((frame->hd.type != NGHTTP2_HEADERS && frame->hd.type != NGHTTP2_DATA) ||
	    !(frame->hd.flags & NGHTTP2_FLAG_END_STREAM)) return 0;
	s = find_stream(h, frame->hd.stream_id);
	if (!s || s->dispatched) return 0;
	if (cherokee_http_stream_dispatch(s, h->connection->bind,
	    &h->connection->socket.client_addr, &h->local, http_version_20, h->connection->socket.cryptor->vserver_ref)) {
		nghttp2_submit_rst_stream(session, NGHTTP2_FLAG_NONE, frame->hd.stream_id, NGHTTP2_PROTOCOL_ERROR);
	}
	return 0;
}

static int
stream_close (nghttp2_session *session, int32_t id, uint32_t error, void *data)
{
	cherokee_http2_t *h = data;
	cherokee_http_stream_t *s = find_stream(h, id);
	UNUSED(session); UNUSED(error);
	if (s) { cherokee_http_stream_free(s); h->stream_count--; }
	return 0;
}

static ssize_t
read_response (nghttp2_session *session, int32_t id, uint8_t *buf,
               size_t len, uint32_t *flags, nghttp2_data_source *source, void *user)
{
	cherokee_http_stream_t *s = source->ptr;
	size_t n = MIN(len, s->response.len);
	UNUSED(session); UNUSED(id); UNUSED(user);
	if (!n && !s->eof) return NGHTTP2_ERR_DEFERRED;
	if (n) {
		memcpy(buf, s->response.buf, n);
		cherokee_buffer_move_to_begin (&s->response, n);
	}
	if (!s->response.len && s->eof) *flags |= NGHTTP2_DATA_FLAG_EOF;
	return n;
}

static int
submit_response (cherokee_http2_t *h, cherokee_http_stream_t *s)
{
	nghttp2_nv fields[129];
	nghttp2_data_provider body;
	size_t i;
	fields[0] = (nghttp2_nv){(uint8_t *)":status", (uint8_t *)s->status, 7, 3, NGHTTP2_NV_FLAG_NONE};
	for (i=0; i<s->fields_len; i++) {
		fields[i+1] = (nghttp2_nv){(uint8_t *)s->fields[i].name, (uint8_t *)s->fields[i].value,
			strlen(s->fields[i].name), strlen(s->fields[i].value), NGHTTP2_NV_FLAG_NONE};
	}
	body.source.ptr = s;
	body.read_callback = read_response;
	if (nghttp2_submit_response(h->session, s->id, fields, s->fields_len+1, &body)) return -1;
	s->submitted = 1;
	return 0;
}

ret_t
cherokee_http2_init (cherokee_connection_t *conn)
{
	cherokee_http2_t *h = calloc(1, sizeof(*h));
	nghttp2_session_callbacks *callbacks = NULL;
	nghttp2_option *option = NULL;
	nghttp2_settings_entry settings[] = {
		{NGHTTP2_SETTINGS_MAX_CONCURRENT_STREAMS, CHEROKEE_HTTP_STREAM_MAX},
		{NGHTTP2_SETTINGS_MAX_HEADER_LIST_SIZE, CHEROKEE_HTTP_STREAM_HEADERS},
		{NGHTTP2_SETTINGS_HEADER_TABLE_SIZE, 4096}
	};
	if (!h) return ret_nomem;
	h->connection = conn;
	if (cherokee_socket_get_local_address(&conn->socket, &h->local) != ret_ok) { free(h); return ret_error; }
	INIT_LIST_HEAD (&h->streams);
	if (nghttp2_session_callbacks_new(&callbacks) || nghttp2_option_new(&option)) goto error;
	nghttp2_session_callbacks_set_on_begin_headers_callback(callbacks, begin_headers);
	nghttp2_session_callbacks_set_on_header_callback(callbacks, recv_header);
	nghttp2_session_callbacks_set_on_invalid_header_callback(callbacks, invalid_header);
	nghttp2_session_callbacks_set_on_data_chunk_recv_callback(callbacks, recv_data);
	nghttp2_session_callbacks_set_on_frame_recv_callback(callbacks, recv_frame);
	nghttp2_session_callbacks_set_on_stream_close_callback(callbacks, stream_close);
	nghttp2_option_set_max_continuations(option, 16);
	nghttp2_option_set_max_outbound_ack(option, 100);
	if (nghttp2_session_server_new2(&h->session, callbacks, h, option)) goto error;
	nghttp2_session_callbacks_del(callbacks);
	nghttp2_option_del(option);
	conn->http2_session = h;
	conn->phase = phase_http2;
	if (nghttp2_submit_settings(h->session, NGHTTP2_FLAG_NONE, settings, 3)) {
		cherokee_http2_free(conn); return ret_error;
	}
	return ret_ok;
error:
	nghttp2_session_callbacks_del(callbacks);
	nghttp2_option_del(option);
	free(h);
	return ret_error;
}

ret_t
cherokee_http2_step (cherokee_connection_t *conn)
{
	cherokee_http2_t *h = conn->http2_session;
	cherokee_list_t *i, *tmp;
	cherokee_thread_t *thread = CONN_THREAD(conn);
	uint8_t buf[16384];
	size_t n;
	int k;
	ret_t ret;
	if (!h) return ret_error;
	/* A bounded burst ensures one connection cannot starve other clients. */
	for (k=0; k<8; k++) {
		ret = cherokee_socket_read(&conn->socket, (char *)buf, sizeof(buf), &n);
		if (ret == ret_eof || ret == ret_error) return ret;
		if (ret == ret_eagain) break;
		if (n && nghttp2_session_mem_recv(h->session, buf, n) != (ssize_t)n) return ret_error;
		if (!n) break;
		cherokee_connection_update_timeout(conn);
	}
	list_for_each_safe (i, tmp, &h->streams) {
		cherokee_http_stream_t *s = (cherokee_http_stream_t *)i;
		int ready = cherokee_http_stream_step(s);
		if (ready < 0 || (ready > 0 && submit_response(h, s))) {
			nghttp2_submit_rst_stream(h->session, NGHTTP2_FLAG_NONE, s->id, NGHTTP2_INTERNAL_ERROR);
		} else if (s->submitted && (s->response.len || s->eof)) {
			nghttp2_session_resume_data(h->session, s->id);
		}
	}
	for (k=0; k<8; k++) {
		if (!h->output.len) {
			const uint8_t *data;
			ssize_t len = nghttp2_session_mem_send(h->session, &data);
			if (len < 0) return ret_error;
			if (!len) break;
			if (cherokee_buffer_add(&h->output, (const char *)data, len) != ret_ok) return ret_nomem;
		}
		ret = cherokee_socket_write(&conn->socket, h->output.buf, h->output.len, &n);
		if (ret == ret_error || ret == ret_eof) return ret;
		if (n) {
			cherokee_buffer_move_to_begin(&h->output, n);
			cherokee_connection_update_timeout(conn);
		}
		if (ret == ret_eagain || !n) break;
	}
	list_for_each (i, &h->streams) {
		if (cherokee_http_stream_update_poll((cherokee_http_stream_t *)i, 1)) return ret_error;
	}
	conn->socket.status = h->output.len || nghttp2_session_want_write(h->session) ? socket_writing : socket_reading;
	cherokee_fdpoll_set_mode(thread->fdpoll, SOCKET_FD(&conn->socket), conn->socket.status);
	if (!nghttp2_session_want_read(h->session) && !nghttp2_session_want_write(h->session) && !h->output.len) return ret_eof;
	return ret_ok;
}

void
cherokee_http2_free (cherokee_connection_t *conn)
{
	cherokee_http2_t *h = conn->http2_session;
	cherokee_list_t *i, *tmp;
	if (!h) return;
	conn->http2_session = NULL;
	nghttp2_session_del(h->session);
	list_for_each_safe (i, tmp, &h->streams) cherokee_http_stream_free((cherokee_http_stream_t *)i);
	cherokee_buffer_mrproper(&h->output);
	free(h);
}
#else
ret_t cherokee_http2_init (cherokee_connection_t *c) { UNUSED(c); return ret_no_sys; }
ret_t cherokee_http2_step (cherokee_connection_t *c) { UNUSED(c); return ret_no_sys; }
void cherokee_http2_free (cherokee_connection_t *c) { UNUSED(c); }
#endif
