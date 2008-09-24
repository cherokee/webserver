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

#ifndef CHEROKEE_HTTP_H
#define CHEROKEE_HTTP_H

#include <cherokee/common.h>
#include <cherokee/buffer.h>


CHEROKEE_BEGIN_DECLS

typedef enum {
	http_version_09,
	http_version_10,
	http_version_11,
	http_version_unknown
} cherokee_http_version_t;

typedef enum {
	http_unknown      = 0,
	http_all_methods  = 0xFFFFFFF,

	http_get          = 1,
	http_post         = 1 << 1,
	http_head         = 1 << 2,
	http_put          = 1 << 3,
	http_options      = 1 << 4,
	http_delete       = 1 << 5,
	http_trace        = 1 << 6,
	http_connect      = 1 << 7,

	http_copy         = 1 << 8,
	http_lock         = 1 << 9,
	http_mkcol        = 1 << 10,
	http_move         = 1 << 11,
	http_notify       = 1 << 12,
	http_poll         = 1 << 13,
	http_propfind     = 1 << 14,
	http_proppatch    = 1 << 15,
	http_search       = 1 << 16,
	http_subscribe    = 1 << 17,
	http_unlock       = 1 << 18,
	http_unsubscribe  = 1 << 19
} cherokee_http_method_t;

typedef enum {
	http_auth_nothing = 0,
	http_auth_basic   = 1,
	http_auth_digest  = 1 << 1
} cherokee_http_auth_t;

typedef enum {
	http_upgrade_nothing,
	http_upgrade_http11,
	http_upgrade_tls10
} cherokee_http_upgrade_t;

typedef enum {
	http_unset                    = 0,
	http_continue                 = 100,
	http_switching_protocols      = 101,
	http_ok                       = 200,
	http_accepted                 = 202,
	http_no_content               = 204,
	http_partial_content          = 206,
	http_moved_permanently        = 301,
	http_moved_temporarily        = 302,
 	http_see_other                = 303,
	http_not_modified             = 304,
	http_bad_request              = 400,
	http_unauthorized             = 401,
	http_access_denied            = 403,
	http_not_found                = 404,
	http_method_not_allowed       = 405,
	http_length_required          = 411,
 	http_request_entity_too_large = 413,
	http_request_uri_too_long     = 414,
	http_unsupported_media_type   = 415,
	http_range_not_satisfiable    = 416,
	http_upgrade_required         = 426,
	http_internal_error           = 500,
	http_not_implemented          = 501,
	http_bad_gateway              = 502,
	http_service_unavailable      = 503,
	http_gateway_timeout          = 504,
	http_version_not_supported    = 505
} cherokee_http_t;

#define http_continue_string                 "100 Continue"
#define http_switching_protocols_string      "101 Switching Protocols"
#define http_ok_string                       "200 OK"
#define http_accepted_string                 "202 Accepted"
#define http_no_content_string               "204 No Content"
#define http_partial_content_string          "206 Partial Content"
#define http_moved_permanently_string        "301 Moved Permanently"
#define http_moved_temporarily_string        "302 Moved Temporarily"
#define http_see_other_string                "303 See Other"
#define http_not_modified_string             "304 Not Modified"
#define http_bad_request_string              "400 Bad Request"
#define http_unauthorized_string             "401 Authorization Required"
#define http_access_denied_string            "403 Forbidden"
#define http_not_found_string                "404 Not Found"
#define http_method_not_allowed_string       "405 Method Not Allowed"
#define http_length_required_string          "411 Length Required"
#define http_request_entity_too_large_string "413 Request Entity too large"
#define http_request_uri_too_long_string     "414 Request-URI too long"
#define http_unsupported_media_type_string   "415 Unsupported Media Type"
#define http_range_not_satisfiable_string    "416 Requested range not satisfiable"
#define http_upgrade_required_string         "426 Upgrade Required"
#define http_internal_error_string           "500 Internal Server Error"
#define http_not_implemented_string          "501 Not Implemented"
#define http_bad_gateway_string              "502 Bad gateway"
#define http_service_unavailable_string      "503 Service Unavailable"
#define http_gateway_timeout_string          "504 Gateway Timeout"
#define http_version_not_supported_string    "505 HTTP Version Not Supported"

#define http_type_200_max 206
#define http_type_300_max 307
#define http_type_400_max 417
#define http_type_500_max 505

#define http_type_200(c)  ((c >= 200) && (c <= http_type_200_max))
#define http_type_300(c)  ((c >= 300) && (c <= http_type_300_max))
#define http_type_400(c)  ((c >= 400) && (c <= http_type_400_max))
#define http_type_500(c)  ((c >= 500) && (c <= http_type_500_max))

#define http_method_with_body(m)  ((m != http_head) && (m != http_options))
#define http_method_with_input(m) ((m == http_post)     || \
				   (m == http_put)      || \
				   (m == http_mkcol)    || \
				   (m == http_search)   || \
				   (m == http_propfind) || \
				   (m == http_proppatch))

#define http_code_with_body(e)   ((e != http_continue)            && \
				  (e != http_not_modified)        && \
				  (e != http_switching_protocols))

ret_t cherokee_http_method_to_string  (cherokee_http_method_t  method,  const char **str, cuint_t *str_len);
ret_t cherokee_http_version_to_string (cherokee_http_version_t version, const char **str, cuint_t *str_len);
ret_t cherokee_http_code_to_string    (cherokee_http_t code, const char **str);
ret_t cherokee_http_code_copy         (cherokee_http_t code, cherokee_buffer_t *buf);


CHEROKEE_END_DECLS

#endif /* CHEROKEE_HTTP_H */
