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
	http_unknown      = 0LL,
	http_all_methods  = 0x1FFFFFFFFLL,

	http_get              = 1,
	http_post             = 1LL << 1,
	http_head             = 1LL << 2,
	http_put              = 1LL << 3,
	http_options          = 1LL << 4,
	http_delete           = 1LL << 5,
	http_trace            = 1LL << 6,
	http_connect          = 1LL << 7,

	http_copy             = 1LL << 8,
	http_lock             = 1LL << 9,
	http_mkcol            = 1LL << 10,
	http_move             = 1LL << 11,
	http_notify           = 1LL << 12,
	http_poll             = 1LL << 13,
	http_propfind         = 1LL << 14,
	http_proppatch        = 1LL << 15,
	http_search           = 1LL << 16,
	http_subscribe        = 1LL << 17,
	http_unlock           = 1LL << 18,
	http_unsubscribe      = 1LL << 19,
	http_report           = 1LL << 20,
	http_patch            = 1LL << 21,
	http_version_control  = 1LL << 22,
	http_checkout         = 1LL << 23,
	http_uncheckout       = 1LL << 24,
	http_checkin          = 1LL << 25,
	http_update           = 1LL << 26,
	http_label            = 1LL << 27,
	http_mkworkspace      = 1LL << 28,
	http_mkactivity       = 1LL << 29,
	http_baseline_control = 1LL << 30,
	http_merge            = 1LL << 31,
	http_invalid          = 1LL << 32,
	http_purge            = 1LL << 33
} cherokee_http_method_t;

#define cherokee_http_method_LENGTH 33
#define HTTP_METHOD(x) ((cherokee_http_method_t)(x))


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

typedef enum {                               /* Protocol   RFC  Section */
	http_unset                    = 0,
	http_continue                 = 100, /* HTTP/1.1  2616  10.1.1  */
	http_switching_protocols      = 101, /* HTTP/1.1  2616  10.1.2  */
	http_processing               = 102, /*   WebDAV  2518  10.1    */
	http_ok                       = 200, /* HTTP/1.1  2616  10.2.1  */
	http_created                  = 201, /* HTTP/1.1  2616  10.2.2  */
	http_accepted                 = 202, /* HTTP/1.1  2616  10.2.3  */
	http_non_authoritative_info   = 203, /* HTTP/1.1  2616  10.2.4  */
	http_no_content               = 204, /* HTTP/1.1  2616  10.2.5  */
	http_reset_content            = 205, /* HTTP/1.1  2616  10.2.6  */
	http_partial_content          = 206, /* HTTP/1.1  2616  10.2.6  */
	http_multi_status             = 207, /*   WebDAV  2518  10.2    */
	http_im_used                  = 226, /*    Delta  3229  10.4.1  */
	http_multiple_choices         = 300, /* HTTP/1.1  2616  10.3.1  */
	http_moved_permanently        = 301, /* HTTP/1.1  2616  10.3.2  */
	http_moved_temporarily        = 302, /* HTTP/1.1  2616  10.3.3  */
	http_see_other                = 303, /* HTTP/1.1  2616  10.3.4  */
	http_not_modified             = 304, /* HTTP/1.1  2616  10.3.5  */
	http_use_proxy                = 305, /* HTTP/1.1  2616  10.3.6  */
	http_temporary_redirect       = 307, /* HTTP/1.1  2616  10.3.8  */
	http_bad_request              = 400, /* HTTP/1.1  2616  10.4.1  */
	http_unauthorized             = 401, /* HTTP/1.1  2616  10.4.2  */
	http_payment_required         = 402, /* HTTP/1.1  2616  10.4.3  */
	http_access_denied            = 403, /* HTTP/1.1  2616  10.4.4  */
	http_not_found                = 404, /* HTTP/1.1  2616  10.4.5  */
	http_method_not_allowed       = 405, /* HTTP/1.1  2616  10.4.6  */
	http_not_acceptable           = 406, /* HTTP/1.1  2616  10.4.7  */
	http_proxy_auth_required      = 407, /* HTTP/1.1  2616  10.4.8  */
	http_request_timeout          = 408, /* HTTP/1.1  2616  10.4.9  */
	http_conflict                 = 409, /* HTTP/1.1  2616  10.4.10 */
	http_gone                     = 410, /* HTTP/1.1  2616  10.4.11 */
	http_length_required          = 411, /* HTTP/1.1  2616  10.4.12 */
	http_precondition_failed      = 412, /* HTTP/1.1  2616  10.4.13 */
	http_request_entity_too_large = 413, /* HTTP/1.1  2616  10.4.14 */
	http_request_uri_too_long     = 414, /* HTTP/1.1  2616  10.4.15 */
	http_unsupported_media_type   = 415, /* HTTP/1.1  2616  10.4.16 */
	http_range_not_satisfiable    = 416, /* HTTP/1.1  2616  10.4.17 */
	http_expectation_failed       = 417, /* HTTP/1.1  2616  10.4.18 */
	http_unprocessable_entity     = 422, /*   WebDAV  2518  10.3    */
	http_locked                   = 423, /*   WebDAV  2518  10.4    */
	http_failed_dependency        = 424, /*   WebDAV  2518  10.5    */
	http_unordered_collection     = 425, /*   WebDAV  3648          */
	http_upgrade_required         = 426, /* TLS upgr  2817   6      */
	http_retry_with               = 449, /* Microsoft extension     */
	http_internal_error           = 500, /* HTTP/1.1  2616  10.5.1  */
	http_not_implemented          = 501, /* HTTP/1.1  2616  10.5.2  */
	http_bad_gateway              = 502, /* HTTP/1.1  2616  10.5.3  */
	http_service_unavailable      = 503, /* HTTP/1.1  2616  10.5.4  */
	http_gateway_timeout          = 504, /* HTTP/1.1  2616  10.5.5  */
	http_version_not_supported    = 505, /* HTTP/1.1  2616  10.5.6  */
	http_variant_also_negotiates  = 506, /* HTTP/1.1  2295   8.1    */
	http_insufficient_storage     = 507, /* HTTP/1.1  2616  10.6    */
	http_bandwidth_limit_exceeded = 509, /* Apache extension        */
	http_not_extended             = 510  /* HTTP Ext  2774   7      */
} cherokee_http_t;

#define http_continue_string                 "100 Continue"
#define http_switching_protocols_string      "101 Switching Protocols"
#define http_processing_string               "102 Processing"
#define http_ok_string                       "200 OK"
#define http_created_string                  "201 Created"
#define http_accepted_string                 "202 Accepted"
#define http_non_authoritative_info_string   "203 Non-Authoritative Information"
#define http_no_content_string               "204 No Content"
#define http_reset_content_string            "205 Reset Content"
#define http_partial_content_string          "206 Partial Content"
#define http_multi_status_string             "207 Multi-Status"
#define http_im_used_string                  "226 IM Used"
#define http_multiple_choices_string         "300 Multiple Choices"
#define http_moved_permanently_string        "301 Moved Permanently"
#define http_moved_temporarily_string        "302 Moved Temporarily"
#define http_see_other_string                "303 See Other"
#define http_not_modified_string             "304 Not Modified"
#define http_use_proxy_string                "305 Use Proxy"
#define http_temporary_redirect_string       "307 Temporary Redirect"
#define http_bad_request_string              "400 Bad Request"
#define http_unauthorized_string             "401 Authorization Required"
#define http_payment_required_string         "402 Payment Required"
#define http_access_denied_string            "403 Forbidden"
#define http_not_found_string                "404 Not Found"
#define http_method_not_allowed_string       "405 Method Not Allowed"
#define http_not_acceptable_string           "406 Not Acceptable"
#define http_proxy_auth_required_string      "407 Proxy Authentication Required"
#define http_request_timeout_string          "408 Request Time-out"
#define http_conflict_string                 "409 Conflict"
#define http_gone_string                     "410 Gone"
#define http_length_required_string          "411 Length Required"
#define http_precondition_failed_string      "412 Precondition Failed"
#define http_request_entity_too_large_string "413 Request Entity too large"
#define http_request_uri_too_long_string     "414 Request-URI too long"
#define http_unsupported_media_type_string   "415 Unsupported Media Type"
#define http_range_not_satisfiable_string    "416 Requested range not satisfiable"
#define http_expectation_failed_string       "417 Expectation Failed"
#define http_unprocessable_entity_string     "422 Unprocessable Entity"
#define http_locked_string                   "423 Locked"
#define http_failed_dependency_string        "424 Failed Dependency"
#define http_unordered_collection_string     "425 Unordered Collection"
#define http_upgrade_required_string         "426 Upgrade Required"
#define http_retry_with_string               "449 Retry With"
#define http_internal_error_string           "500 Internal Server Error"
#define http_not_implemented_string          "501 Not Implemented"
#define http_bad_gateway_string              "502 Bad gateway"
#define http_service_unavailable_string      "503 Service Unavailable"
#define http_gateway_timeout_string          "504 Gateway Timeout"
#define http_version_not_supported_string    "505 HTTP Version Not Supported"
#define http_variant_also_negotiates_string  "506 Variant Also Negotiates"
#define http_insufficient_storage_string     "507 Insufficient Storage"
#define http_bandwidth_limit_exceeded_string "509 Bandwidth Limit Exceeded"
#define http_not_extended_string             "510 Not Extended"

#define http_type_100_max 102
#define http_type_200_max 207
#define http_type_300_max 307
#define http_type_400_max 449
#define http_type_500_max 510

#define http_type_100(c)  ((c >= 100) && (c <= http_type_100_max))
#define http_type_200(c)  ((c >= 200) && (c <= http_type_200_max))
#define http_type_300(c)  ((c >= 300) && (c <= http_type_300_max))
#define http_type_400(c)  ((c >= 400) && (c <= http_type_400_max))
#define http_type_500(c)  ((c >= 500) && (c <= http_type_500_max))

#define http_method_with_body(m)  ((m) != http_head)

/* RFC 2518, RFC 2616, RFC3253, RFC 5789 */
#define http_method_with_input(m) ((m == http_post)      || \
                                   (m == http_put)       || \
                                   (m == http_merge)     || \
                                   (m == http_search)    || \
                                   (m == http_report)    || \
                                   (m == http_patch)     || \
                                   (m == http_propfind)  || \
                                   (m == http_proppatch) || \
                                   (m == http_update)    || \
                                   (m == http_label))

#define http_method_with_optional_input(m) ((m == http_options)           || \
                                            (m == http_delete)            || \
                                            (m == http_mkcol)             || \
                                            (m == http_copy)              || \
                                            (m == http_move)              || \
                                            (m == http_lock)              || \
                                            (m == http_unlock)            || \
                                            (m == http_version_control)   || \
                                            (m == http_checkout)          || \
                                            (m == http_uncheckout)        || \
                                            (m == http_checkin)           || \
                                            (m == http_mkworkspace)       || \
                                            (m == http_mkactivity)        || \
                                            (m == http_baseline_control))


/* RFC 2616: Section 4.3 */
#define http_code_with_body(e)    ((! http_type_100(e))            /* 1xx */ && \
                                   ((e) != http_no_content)        /* 204 */ && \
                                   ((e) != http_not_modified))     /* 304 */

#define http_port_is_standard(port,is_tls)  (((! is_tls) && (port == 80)) || \
                                             ((  is_tls) && (port == 443)))

ret_t cherokee_http_method_to_string  (cherokee_http_method_t  method,  const char **str, cuint_t *str_len);
ret_t cherokee_http_string_to_method  (cherokee_buffer_t *string, cherokee_http_method_t *method);
ret_t cherokee_http_version_to_string (cherokee_http_version_t version, const char **str, cuint_t *str_len);
ret_t cherokee_http_code_to_string    (cherokee_http_t code, const char **str);
ret_t cherokee_http_code_copy         (cherokee_http_t code, cherokee_buffer_t *buf);

CHEROKEE_END_DECLS

#endif /* CHEROKEE_HTTP_H */
