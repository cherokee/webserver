/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/* Cherokee
 *
 * Authors:
 *      Alvaro Lopez Ortega <alvaro@alobbs.com>
 *
 * Copyright (C) 2001-2006 Alvaro Lopez Ortega
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of version 2 of the GNU General Public
 * License as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
 * USA
 */

#if !defined (CHEROKEE_INSIDE_CHEROKEE_H) && !defined (CHEROKEE_COMPILATION)
# error "Only <cherokee/cherokee.h> can be included directly, this file may disappear or change contents."
#endif

#ifndef CHEROKEE_DOWNLOADER_H
#define CHEROKEE_DOWNLOADER_H

#include <cherokee/common.h>
#include <cherokee/fdpoll.h>
#include <cherokee/buffer.h>
#include <cherokee/http.h>


CHEROKEE_BEGIN_DECLS

typedef enum {
	downloader_event_init,
	downloader_event_has_headers,
	downloader_event_read_body,
	downloader_event_finish,
	downloader_event_NUMBER
} cherokee_downloader_event_t;

typedef ret_t (* cherokee_downloader_init_t)        (void *downloader, void *param);
typedef ret_t (* cherokee_downloader_has_headers_t) (void *downloader, void *param);
typedef ret_t (* cherokee_downloader_read_body_t)   (void *downloader, void *param);
typedef ret_t (* cherokee_downloader_finish_t)      (void *downloader, void *param);

typedef struct cherokee_downloader cherokee_downloader_t;
#define DOWNLOADER(d) ((cherokee_downloader_t *)(d))


ret_t cherokee_downloader_new           (cherokee_downloader_t **downloader);
ret_t cherokee_downloader_free          (cherokee_downloader_t  *downloader);

ret_t cherokee_downloader_set_fdpoll     (cherokee_downloader_t *downloader, cherokee_fdpoll_t *fdpoll);
ret_t cherokee_downloader_set_url        (cherokee_downloader_t *downloader, cherokee_buffer_t *url);
ret_t cherokee_downloader_set_keepalive  (cherokee_downloader_t *downloader, cherokee_boolean_t active);
ret_t cherokee_downloader_get_reply_code (cherokee_downloader_t *downloader, cherokee_http_t *code);

ret_t cherokee_downloader_post_set      (cherokee_downloader_t *downloader, cherokee_buffer_t *post);
ret_t cherokee_downloader_post_reset    (cherokee_downloader_t *downloader);

ret_t cherokee_downloader_step          (cherokee_downloader_t *downloader);
ret_t cherokee_downloader_reuse         (cherokee_downloader_t *downloader);
ret_t cherokee_downloader_connect       (cherokee_downloader_t *downloader);
ret_t cherokee_downloader_connect_event (cherokee_downloader_t *downloader, cherokee_downloader_event_t event, void *func, void *param);


CHEROKEE_END_DECLS

#endif /* CHEROKEE_DOWNLOADER_H */
