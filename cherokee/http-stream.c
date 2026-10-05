/* Cherokee native stream adapter. GPL-2.0-only. */
#include "common-internal.h"
#include "http-stream.h"
#include "util.h"
#include <ctype.h>
#include <fcntl.h>
#include <errno.h>

cherokee_http_stream_t *
cherokee_http_stream_new (cherokee_thread_t *thread, int64_t id)
{
	cherokee_http_stream_t *s = calloc (1, sizeof(*s));
	if (!s) return NULL;
	INIT_LIST_HEAD (&s->list);
	s->id = id;
	s->fd = -1;
	s->thread = thread;
	return s;
}

void
cherokee_http_stream_free (cherokee_http_stream_t *s)
{
	size_t i;
	if (!s) return;
	cherokee_list_del (&s->list);
	if (s->fd >= 0) {
		cherokee_fdpoll_del (s->thread->fdpoll, s->fd);
		close (s->fd);
	}
	cherokee_buffer_mrproper (&s->method);
	cherokee_buffer_mrproper (&s->path);
	cherokee_buffer_mrproper (&s->authority);
	cherokee_buffer_mrproper (&s->scheme);
	cherokee_buffer_mrproper (&s->headers);
	cherokee_buffer_mrproper (&s->body);
	cherokee_buffer_mrproper (&s->request);
	cherokee_buffer_mrproper (&s->response);
	for (i=0; i<s->fields_len; i++) {
		free (s->fields[i].name);
		free (s->fields[i].value);
	}
	free (s);
}

int
cherokee_http_field_hop_by_hop (const char *name)
{
	return !strcasecmp(name, "connection") || !strcasecmp(name, "keep-alive") ||
	       !strcasecmp(name, "proxy-connection") || !strcasecmp(name, "upgrade") ||
	       !strcasecmp(name, "transfer-encoding") || !strcasecmp(name, "te");
}

static int
is_token (uint8_t c)
{
	return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
	       (c >= '0' && c <= '9') || strchr("!#$%&'*+-.^_`|~", c) != NULL;
}

int
cherokee_http_stream_header (cherokee_http_stream_t *s, const uint8_t *name,
                            size_t nlen, const uint8_t *value, size_t vlen)
{
	size_t i;
	cherokee_buffer_t *pseudo = NULL;
	char key[256];
	if (!s || s->dispatched || !nlen || nlen >= sizeof(key) ||
	    nlen + vlen + 32 > CHEROKEE_HTTP_STREAM_HEADERS - s->header_bytes)
		return -1;
	s->header_bytes += nlen + vlen + 32;
	for (i=0; i<nlen; i++) {
		if ((i == 0 && name[i] == ':')) continue;
		if (!name[i] || !is_token(name[i]) || (name[i] >= 'A' && name[i] <= 'Z')) return -1;
	}
	for (i=0; i<vlen; i++)
		if (value[i] == 0 || value[i] == '\r' || value[i] == '\n' ||
		    (value[i] < 32 && value[i] != '\t') || value[i] == 127) return -1;
	if (vlen && (value[0] == ' ' || value[0] == '\t' ||
	             value[vlen-1] == ' ' || value[vlen-1] == '\t')) return -1;
	memcpy (key, name, nlen); key[nlen] = 0;
	if (name[0] == ':') {
		if (s->regular_seen) return -1;
		if (!strcmp(key, ":method")) pseudo = &s->method;
		else if (!strcmp(key, ":path")) pseudo = &s->path;
		else if (!strcmp(key, ":authority")) pseudo = &s->authority;
		else if (!strcmp(key, ":scheme")) pseudo = &s->scheme;
		else return -1;
		if (pseudo->len || !vlen) return -1;
		return cherokee_buffer_add (pseudo, (const char *)value, vlen) == ret_ok ? 0 : -1;
	}
	s->regular_seen = 1;
	if (cherokee_http_field_hop_by_hop(key)) {
		if (!strcmp(key, "te") && vlen == 8 && !memcmp(value, "trailers", 8)) return 0;
		return -1;
	}
	if (!strcmp(key, "content-length")) {
		uint64_t length = 0;
		if (s->has_length || !vlen) return -1;
		for (i=0; i<vlen; i++) {
			if (value[i] < '0' || value[i] > '9' || length > (UINT64_MAX-9)/10) return -1;
			length = length*10 + value[i]-'0';
		}
		if (length > CHEROKEE_HTTP_STREAM_BODY) return -1;
		s->has_length = 1;
		s->content_length = length;
		return 0; /* Synthesized once from the actual DATA length. */
	}
	if (!strcmp(key, "host")) {
		if (s->authority.len && (s->authority.len != vlen ||
		    strncasecmp(s->authority.buf, (const char *)value, vlen))) return -1;
		if (!s->authority.len && cherokee_buffer_add (&s->authority, (const char *)value, vlen) != ret_ok) return -1;
		return 0;
	}
	if (cherokee_buffer_add (&s->headers, key, nlen) != ret_ok ||
	    cherokee_buffer_add_str (&s->headers, ": ") != ret_ok ||
	    cherokee_buffer_add (&s->headers, (const char *)value, vlen) != ret_ok ||
	    cherokee_buffer_add_str (&s->headers, "\r\n") != ret_ok) return -1;
	return 0;
}

int
cherokee_http_stream_data (cherokee_http_stream_t *s, const uint8_t *data, size_t len)
{
	if (!s || s->dispatched || len > CHEROKEE_HTTP_STREAM_BODY - s->body.len) return -1;
	return cherokee_buffer_add (&s->body, (const char *)data, len) == ret_ok ? 0 : -1;
}

int
cherokee_http_stream_dispatch (cherokee_http_stream_t *s, cherokee_bind_t *bind,
                              cherokee_sockaddr_t *peer, cherokee_http_version_t version)
{
	int fds[2];
	size_t i;
	if (!s || s->invalid || s->dispatched || !s->method.len || !s->path.len ||
	    !s->authority.len || !s->scheme.len || strcmp(s->scheme.buf, "https") ||
	    (s->has_length && s->content_length != s->body.len)) return -1;
	for (i=0; i<s->method.len; i++)
		if (!is_token((uint8_t)s->method.buf[i])) return -1;
	/* Tunnels need a separate handler interface; never translate CONNECT. */
	if (!strcmp(s->method.buf, "CONNECT")) return -1;
	if (s->path.buf[0] != '/' && (strcmp(s->path.buf, "*") || strcmp(s->method.buf, "OPTIONS"))) return -1;
	for (i=0; i<s->path.len; i++) if ((unsigned char)s->path.buf[i] <= 32 || s->path.buf[i] == '#') return -1;
	for (i=0; i<s->authority.len; i++) if ((unsigned char)s->authority.buf[i] <= 32 || strchr("/@?#\\", s->authority.buf[i])) return -1;
	if (cherokee_buffer_add_va (&s->request, "%s %s HTTP/1.0\r\nHost: %s\r\nConnection: close\r\nContent-Length: %u\r\n",
	    s->method.buf, s->path.buf, s->authority.buf, s->body.len) != ret_ok ||
	    cherokee_buffer_add_buffer (&s->request, &s->headers) != ret_ok ||
	    cherokee_buffer_add_str (&s->request, "\r\n") != ret_ok ||
	    cherokee_buffer_add_buffer (&s->request, &s->body) != ret_ok) return -1;
	if (socketpair(AF_UNIX, SOCK_STREAM, 0, fds)) return -1;
	for (i=0; i<2; i++) {
		if (fcntl(fds[i], F_SETFL, O_NONBLOCK) < 0 || fcntl(fds[i], F_SETFD, FD_CLOEXEC) < 0) {
			close(fds[0]); close(fds[1]); return -1;
		}
	}
	if (cherokee_thread_add_stream (s->thread, bind, fds[1], peer, version) != ret_ok) {
		close(fds[0]); close(fds[1]); return -1;
	}
	s->fd = fds[0];
	if (cherokee_fdpoll_add(s->thread->fdpoll, s->fd, FDPOLL_MODE_WRITE) != ret_ok) {
		close(s->fd); s->fd = -1; return -1;
	}
	s->dispatched = 1;
	cherokee_buffer_mrproper (&s->body);
	return 0;
}

static int
parse_response (cherokee_http_stream_t *s)
{
	char *end = strstr(s->response.buf, "\r\n\r\n");
	char *line, *next, *colon;
	size_t header_len, i;
	if (!end) return s->response.len >= CHEROKEE_HTTP_STREAM_HEADERS ? -1 : 0;
	header_len = end - s->response.buf + 4;
	if (s->response.len < 12 || strncmp(s->response.buf, "HTTP/1.", 7) || s->response.buf[8] != ' ') return -1;
	for (i=0; i<3; i++) if (!isdigit((unsigned char)s->response.buf[9+i])) return -1;
	memcpy(s->status, s->response.buf+9, 3); s->status[3] = 0;
	line = strstr(s->response.buf, "\r\n");
	if (!line) return -1;
	/* Informational responses do not end the response. */
	if (s->status[0] == '1') {
		if (!strcmp(s->status, "101")) return -1;
		cherokee_buffer_move_to_begin (&s->response, header_len);
		return 0;
	}
	line += 2;
	while (line < end) {
		char *value;
		next = strstr(line, "\r\n");
		colon = memchr(line, ':', next-line);
		if (!colon || colon == line) return -1;
		*colon = 0; *next = 0;
		for (i=0; line[i]; i++) {
			if (!is_token((uint8_t)line[i])) return -1;
			line[i] = tolower((unsigned char)line[i]);
		}
		value = colon+1;
		while (*value == ' ' || *value == '\t') value++;
		if (!cherokee_http_field_hop_by_hop(line)) {
			cherokee_http_field_t *f;
			if (s->fields_len == 128) return -1;
			f = &s->fields[s->fields_len++];
			f->name = strdup(line); f->value = strdup(value);
			if (!f->name || !f->value) return -1;
		}
		line = next+2;
	}
	cherokee_buffer_move_to_begin (&s->response, header_len);
	return 1;
}

int
cherokee_http_stream_step (cherokee_http_stream_t *s)
{
	char buf[16384];
	ssize_t n;
	if (s->fd < 0) return 0;
	if (s->request.len) {
		n = send(s->fd, s->request.buf, s->request.len, MSG_NOSIGNAL);
		if (n > 0) cherokee_buffer_move_to_begin (&s->request, n);
		else if (n < 0 && errno != EAGAIN && errno != EWOULDBLOCK && errno != EINTR) return -1;
		if (!s->request.len) {
			cherokee_fdpoll_set_mode (s->thread->fdpoll, s->fd, FDPOLL_MODE_READ);
			shutdown(s->fd, SHUT_WR);
		}
	}
	/* Leave the socket readable while bounded DATA awaits peer flow credit. */
	if (s->response.len > CHEROKEE_HTTP_STREAM_WINDOW - sizeof(buf) || s->eof) return 0;
	n = recv(s->fd, buf, sizeof(buf), 0);
	if (n > 0) {
		if (cherokee_buffer_add (&s->response, buf, n) != ret_ok) return -1;
	} else if (n == 0) s->eof = 1;
	else if (errno != EAGAIN && errno != EWOULDBLOCK && errno != EINTR) return -1;
	if (!s->submitted && s->response.len) return parse_response(s);
	if (!s->submitted && s->eof) return -1;
	return 0;
}
