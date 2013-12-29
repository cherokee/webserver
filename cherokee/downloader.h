/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/* Cherokee
 *
 * Authors:
 *      Alvaro Lopez Ortega <alvaro@alobbs.com>
 *
 * Copyright (C) 2001-2014 Alvaro Lopez Ortega
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA.
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
#include <cherokee/post.h>
#include <cherokee/cryptor.h>

CHEROKEE_BEGIN_DECLS

typedef struct cherokee_downloader cherokee_downloader_t;
#define DOWNLOADER(d) ((cherokee_downloader_t *)(d))

typedef enum {
	downloader_status_none             = 0x0,
	downloader_status_headers_sent     = 0x1,
	downloader_status_post_sent        = 0x2,
	downloader_status_headers_received = 0x4,
	downloader_status_data_available   = 0x8,
	downloader_status_finished         = 0x10
} cherokee_downloader_status_t;

ret_t cherokee_downloader_new             (cherokee_downloader_t **downloader);
ret_t cherokee_downloader_free            (cherokee_downloader_t  *downloader);

ret_t cherokee_downloader_set_url         (cherokee_downloader_t *downloader, cherokee_buffer_t *url);
ret_t cherokee_downloader_set_keepalive   (cherokee_downloader_t *downloader, cherokee_boolean_t active);
ret_t cherokee_downloader_set_proxy       (cherokee_downloader_t *downloader, cherokee_buffer_t *proxy, cuint_t port);
ret_t cherokee_downloader_set_auth        (cherokee_downloader_t *downloader, cherokee_buffer_t *user, cherokee_buffer_t *password);
ret_t cherokee_downloader_set_cryptor     (cherokee_downloader_t *downloader, cherokee_cryptor_t *cryptor);

ret_t cherokee_downloader_get_reply_code  (cherokee_downloader_t *downloader, cherokee_http_t *code);
ret_t cherokee_downloader_get_reply_hdr   (cherokee_downloader_t *downloader, cherokee_buffer_t **header);

ret_t cherokee_downloader_post_reset      (cherokee_downloader_t *downloader);

ret_t cherokee_downloader_step            (cherokee_downloader_t *downloader, cherokee_buffer_t *tmp1, cherokee_buffer_t *tmp2);
ret_t cherokee_downloader_reuse           (cherokee_downloader_t *downloader);
ret_t cherokee_downloader_connect         (cherokee_downloader_t *downloader);

ret_t cherokee_downloader_get_status      (cherokee_downloader_t *downloader, cherokee_downloader_status_t *status);
ret_t cherokee_downloader_is_request_sent (cherokee_downloader_t *downloader);

CHEROKEE_END_DECLS

#endif /* CHEROKEE_DOWNLOADER_H */
