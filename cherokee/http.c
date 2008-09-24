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

#include "common-internal.h"
#include "http.h"

#include <stdio.h>

#define entry(name,name_str)                           \
	    case name:	                               \
	         if (len) *len = sizeof(name_str) - 1; \
	         *str = name_str;                      \
 		 return ret_ok;


ret_t 
cherokee_http_method_to_string (cherokee_http_method_t method, const char **str, cuint_t *len)
{
	switch (method) {
		/* HTTP 1.1 methods
		 */
		entry (http_get, "GET");
		entry (http_post, "POST");
		entry (http_head, "HEAD");
		entry (http_put, "PUT");
		entry (http_options, "PUT");
		entry (http_delete, "DELETE");
		entry (http_trace, "TRACE");
		entry (http_connect, "CONNECT");

		/* WebDAV methods
		 */
		entry (http_copy, "COPY");
		entry (http_lock, "LOCK");
		entry (http_mkcol, "MKCOL");
		entry (http_move, "MOVE");
		entry (http_notify, "NOTIFY");
		entry (http_poll, "POLL");
		entry (http_propfind, "PROPFIND");
		entry (http_proppatch, "PROPPATCH");
		entry (http_search, "SEARCH");
		entry (http_subscribe, "SUBSCRIBE");
		entry (http_unlock, "UNLOCK");
		entry (http_unsubscribe, "UNSUBSCRIBE");

	default:
		break;
	};

	if (len) *len = 7;
	*str = "UNKNOWN";
	return ret_error;
}


ret_t 
cherokee_http_version_to_string (cherokee_http_version_t version, const char **str, cuint_t *len)
{
	switch (version) {
		entry (http_version_11, "HTTP/1.1");
		entry (http_version_10, "HTTP/1.0");
		entry (http_version_09, "HTTP/0.9");
	default:
		break;
	};

	if (len) *len = 12;
	*str = "HTTP/Unknown";
	return ret_error;
}


ret_t
cherokee_http_code_to_string (cherokee_http_t code, const char **str)
{
	switch (code) {

	/* 2xx
	 */
	case http_ok:  	                    *str = http_ok_string; break;
	case http_accepted:                 *str = http_accepted_string; break;
	case http_no_content:               *str = http_no_content_string; break;
	case http_partial_content:          *str = http_partial_content_string; break;

	/* 3xx
	 */
	case http_moved_permanently:        *str = http_moved_permanently_string; break;
	case http_moved_temporarily:        *str = http_moved_temporarily_string; break;
	case http_see_other:                *str = http_see_other_string; break;
	case http_not_modified:             *str = http_not_modified_string; break;

	/* 4xx
	 */
	case http_bad_request:              *str = http_bad_request_string; break;
	case http_unauthorized:             *str = http_unauthorized_string; break;
	case http_access_denied:            *str = http_access_denied_string; break;
	case http_not_found:                *str = http_not_found_string; break;
	case http_method_not_allowed:       *str = http_method_not_allowed_string; break;
	case http_length_required:          *str = http_length_required_string; break;
	case http_request_entity_too_large: *str = http_request_entity_too_large_string; break;
	case http_unsupported_media_type:   *str = http_unsupported_media_type_string; break;
	case http_request_uri_too_long:     *str = http_request_uri_too_long_string; break;
	case http_range_not_satisfiable:    *str = http_range_not_satisfiable_string; break;
	case http_upgrade_required:         *str = http_upgrade_required_string; break;

	/* 5xx
	 */
	case http_internal_error:           *str = http_internal_error_string; break;
	case http_not_implemented:          *str = http_not_implemented_string; break;
	case http_bad_gateway:              *str = http_bad_gateway_string; break;
	case http_service_unavailable:      *str = http_service_unavailable_string; break;
	case http_gateway_timeout:          *str = http_gateway_timeout_string; break;
	case http_version_not_supported:    *str = http_version_not_supported_string; break;

	/* 1xx
	*/
	case http_continue:                 *str = http_continue_string; break;
	case http_switching_protocols:      *str = http_switching_protocols_string; break;

	default:
		*str = "500 Unknown error";
		return ret_error;
	}

	return ret_ok;
}


ret_t 
cherokee_http_code_copy (cherokee_http_t code, cherokee_buffer_t *buf)
{
#define entry_code(c)    \
	case http_ ## c: \
	   return cherokee_buffer_add (buf, http_ ## c ## _string, sizeof(http_ ## c ## _string)-1)

	switch (code) {

		/* 2xx
		 */
		entry_code (ok);
		entry_code (accepted);
		entry_code (no_content);
		entry_code (partial_content);

		/* 3xx
		 */
		entry_code (moved_permanently);
		entry_code (moved_temporarily);
		entry_code (see_other);
		entry_code (not_modified);

		/* 4xx
		 */
		entry_code (bad_request);
		entry_code (unauthorized);
		entry_code (access_denied);
		entry_code (not_found);
		entry_code (method_not_allowed);
		entry_code (length_required);
		entry_code (request_entity_too_large);
		entry_code (request_uri_too_long);
		entry_code (unsupported_media_type);
		entry_code (range_not_satisfiable);
		entry_code (upgrade_required);

		/* 5xx
		 */
		entry_code (internal_error);
		entry_code (not_implemented);
		entry_code (bad_gateway);
		entry_code (service_unavailable);
		entry_code (gateway_timeout);
		entry_code (version_not_supported);

		/* 1xx
		 */
		entry_code (continue);
		entry_code (switching_protocols);

	default:
 		PRINT_ERROR ("ERROR: Unknown HTTP status code %d\n", code);
 		cherokee_buffer_add_str (buf, http_internal_error_string);
 		return ret_error;
	}

	SHOULDNT_HAPPEN;
	return ret_error;
}

