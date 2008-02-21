/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/* Cherokee
 *
 * Authors:
 *      Alvaro Lopez Ortega <alvaro@alobbs.com>
 *
 * Copyright (C) 2001-2008 Alvaro Lopez Ortega
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


#ifndef CHEROKEE_HEADER_PROTECTED_H
#define CHEROKEE_HEADER_PROTECTED_H

#include "buffer.h"
#include "http.h"


CHEROKEE_BEGIN_DECLS

/* NOTE:
 * Keep it sync with known_headers_names of header.c
 */
typedef enum {
	header_accept,
	header_accept_charset,
	header_accept_encoding,
	header_accept_language,
	header_authorization,
	header_connection,
	header_content_length,
	header_cookie,
	header_host,
	header_if_modified_since,
	header_if_none_match,
	header_if_range,
	header_keepalive,
	header_location,
	header_range,
	header_referer,
	header_upgrade,
	header_user_agent,
	HEADER_LENGTH
} cherokee_common_header_t;


typedef enum {
	header_type_request,
	header_type_response, 
	header_type_basic
} cherokee_header_type_t;


typedef struct cherokee_header cherokee_header_t;
#define HDR(h) ((cherokee_header_t *)(h))


typedef ret_t (* cherokee_header_foreach_func_t) (cherokee_buffer_t *header, cherokee_buffer_t *content, void *data);

ret_t cherokee_header_new                 (cherokee_header_t **hdr, cherokee_header_type_t type);
ret_t cherokee_header_init                (cherokee_header_t  *hdr, cherokee_header_type_t type);
ret_t cherokee_header_free                (cherokee_header_t  *hdr);
ret_t cherokee_header_mrproper            (cherokee_header_t  *hdr);

ret_t cherokee_header_clean               (cherokee_header_t *hdr);
ret_t cherokee_header_parse               (cherokee_header_t *hdr, cherokee_buffer_t *buffer, cherokee_http_t *error_code);
ret_t cherokee_header_has_header          (cherokee_header_t *hdr, cherokee_buffer_t *buffer, int tail_len);

ret_t cherokee_header_get_length          (cherokee_header_t *hdr, cuint_t *len);
ret_t cherokee_header_foreach_unknown     (cherokee_header_t *hdr, cherokee_header_foreach_func_t func, void *data);

ret_t cherokee_header_copy_request        (cherokee_header_t *hdr, cherokee_buffer_t *request);
ret_t cherokee_header_copy_query_string   (cherokee_header_t *hdr, cherokee_buffer_t *query_string);
ret_t cherokee_header_copy_request_w_args (cherokee_header_t *hdr, cherokee_buffer_t *request);
ret_t cherokee_header_get_request_w_args  (cherokee_header_t *hdr, char **req, int *req_len);

ret_t cherokee_header_has_known           (cherokee_header_t *hdr, cherokee_common_header_t header);
ret_t cherokee_header_get_known           (cherokee_header_t *hdr, cherokee_common_header_t header, char **info, cuint_t *info_len);
ret_t cherokee_header_get_unknown         (cherokee_header_t *hdr, char *name, int name_len, char **header, cuint_t *header_len);

ret_t cherokee_header_copy_known          (cherokee_header_t *hdr, cherokee_common_header_t header, cherokee_buffer_t *buf);
ret_t cherokee_header_copy_unknown        (cherokee_header_t *hdr, char *name, int name_len, cherokee_buffer_t *buf);


CHEROKEE_END_DECLS

#endif /* CHEROKEE_HEADER_PROTECTED_H  */
