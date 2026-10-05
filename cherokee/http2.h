/* Cherokee native HTTP/2. GPL-2.0-only. */
#ifndef CHEROKEE_HTTP2_H
#define CHEROKEE_HTTP2_H
#include "connection-protected.h"
ret_t cherokee_http2_init (cherokee_connection_t *);
ret_t cherokee_http2_step (cherokee_connection_t *);
void cherokee_http2_free (cherokee_connection_t *);
#endif
