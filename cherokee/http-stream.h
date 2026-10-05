/* Cherokee native stream adapter. GPL-2.0-only. */
#ifndef CHEROKEE_HTTP_STREAM_H
#define CHEROKEE_HTTP_STREAM_H
#include "thread.h"

#define CHEROKEE_HTTP_STREAM_HEADERS 65536
#define CHEROKEE_HTTP_STREAM_BODY (1024 * 1024)
#define CHEROKEE_HTTP_STREAM_WINDOW 65536
#define CHEROKEE_HTTP_STREAM_MAX 100

typedef struct {
	char *name;
	char *value;
} cherokee_http_field_t;

typedef struct {
	cherokee_list_t list;
	int64_t id;
	int fd;
	cherokee_thread_t *thread;
	cherokee_buffer_t method, path, authority, scheme, headers, body;
	cherokee_buffer_t request, response;
	size_t header_bytes;
	int regular_seen, invalid, dispatched, submitted, eof;
	int has_length;
	uint64_t content_length;
	char status[4];
	cherokee_http_field_t fields[128];
	size_t fields_len;
} cherokee_http_stream_t;

cherokee_http_stream_t *cherokee_http_stream_new (cherokee_thread_t *, int64_t);
void cherokee_http_stream_free (cherokee_http_stream_t *);
int cherokee_http_stream_header (cherokee_http_stream_t *, const uint8_t *, size_t, const uint8_t *, size_t);
int cherokee_http_stream_data (cherokee_http_stream_t *, const uint8_t *, size_t);
int cherokee_http_stream_dispatch (cherokee_http_stream_t *, cherokee_bind_t *, cherokee_sockaddr_t *, cherokee_http_version_t);
/* Returns 1 when response headers are ready, 0 while waiting, -1 on error. */
int cherokee_http_stream_step (cherokee_http_stream_t *);
int cherokee_http_field_hop_by_hop (const char *);
#endif
