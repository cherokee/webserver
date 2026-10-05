/* Cherokee native HTTP/3. GPL-2.0-only. */
#ifndef CHEROKEE_HTTP3_H
#define CHEROKEE_HTTP3_H
#include "bind.h"
#include "server-protected.h"
ret_t cherokee_http3_init (cherokee_bind_t *, cherokee_server_t *);
void cherokee_http3_step (cherokee_bind_t *, cherokee_thread_t *);
void cherokee_http3_free (cherokee_bind_t *);
#endif
