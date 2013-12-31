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
		entry (http_options, "OPTIONS");
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
		entry (http_report, "REPORT");
		entry (http_patch, "PATCH");
		entry (http_version_control, "VERSION-CONTROL");
		entry (http_checkout, "CHECKOUT");
		entry (http_uncheckout, "UNCHECKOUT");
		entry (http_checkin, "CHECKIN");
		entry (http_update, "UPDATE");
		entry (http_label, "LABEL");
		entry (http_mkworkspace, "MKWORKSPACE");
		entry (http_mkactivity, "MKACTIVITY");
		entry (http_baseline_control, "BASELINE-CONTROL");
		entry (http_merge, "MERGE");
		entry (http_invalid, "INVALID");

	default:
		break;
	};

	if (len) *len = 7;
	*str = "UNKNOWN";
	return ret_error;
}


ret_t
cherokee_http_string_to_method (cherokee_buffer_t      *string,
                                cherokee_http_method_t *method)
{
	if (cherokee_buffer_case_cmp_str (string, "get") == 0)
		*method = http_get;
	else if (cherokee_buffer_case_cmp_str (string, "post") == 0)
		*method = http_post;
	else if (cherokee_buffer_case_cmp_str (string, "head") == 0)
		*method = http_head;
	else if (cherokee_buffer_case_cmp_str (string, "put") == 0)
		*method = http_put;
	else if (cherokee_buffer_case_cmp_str (string, "options") == 0)
		*method = http_options;
	else if (cherokee_buffer_case_cmp_str (string, "delete") == 0)
		*method = http_delete;
	else if (cherokee_buffer_case_cmp_str (string, "trace") == 0)
		*method = http_trace;
	else if (cherokee_buffer_case_cmp_str (string, "connect") == 0)
		*method = http_connect;
	else if (cherokee_buffer_case_cmp_str (string, "copy") == 0)
		*method = http_copy;
	else if (cherokee_buffer_case_cmp_str (string, "lock") == 0)
		*method = http_lock;
	else if (cherokee_buffer_case_cmp_str (string, "mkcol") == 0)
		*method = http_mkcol;
	else if (cherokee_buffer_case_cmp_str (string, "move") == 0)
		*method = http_move;
	else if (cherokee_buffer_case_cmp_str (string, "notify") == 0)
		*method = http_notify;
	else if (cherokee_buffer_case_cmp_str (string, "poll") == 0)
		*method = http_poll;
	else if (cherokee_buffer_case_cmp_str (string, "propfind") == 0)
		*method = http_propfind;
	else if (cherokee_buffer_case_cmp_str (string, "proppatch") == 0)
		*method = http_proppatch;
	else if (cherokee_buffer_case_cmp_str (string, "search") == 0)
		*method = http_search;
	else if (cherokee_buffer_case_cmp_str (string, "subscribe") == 0)
		*method = http_subscribe;
	else if (cherokee_buffer_case_cmp_str (string, "unlock") == 0)
		*method = http_unlock;
	else if (cherokee_buffer_case_cmp_str (string, "unsubscribe") == 0)
		*method = http_unsubscribe;
	else if (cherokee_buffer_case_cmp_str (string, "report") == 0)
		*method = http_report;
	else if (cherokee_buffer_case_cmp_str (string, "patch") == 0)
		*method = http_patch;
	else if (cherokee_buffer_case_cmp_str (string, "version-control") == 0)
		*method = http_version_control;
	else if (cherokee_buffer_case_cmp_str (string, "checkout") == 0)
		*method = http_checkout;
	else if (cherokee_buffer_case_cmp_str (string, "uncheckout") == 0)
		*method = http_uncheckout;
	else if (cherokee_buffer_case_cmp_str (string, "checkin") == 0)
		*method = http_checkin;
	else if (cherokee_buffer_case_cmp_str (string, "update") == 0)
		*method = http_update;
	else if (cherokee_buffer_case_cmp_str (string, "label") == 0)
		*method = http_label;
	else if (cherokee_buffer_case_cmp_str (string, "mkworkspace") == 0)
		*method = http_mkworkspace;
	else if (cherokee_buffer_case_cmp_str (string, "mkactivity") == 0)
		*method = http_mkactivity;
	else if (cherokee_buffer_case_cmp_str (string, "baseline-control") == 0)
		*method = http_baseline_control;
	else if (cherokee_buffer_case_cmp_str (string, "merge") == 0)
		*method = http_merge;
	else if (cherokee_buffer_case_cmp_str (string, "invalid") == 0)
		*method = http_invalid;
	else if (cherokee_buffer_case_cmp_str (string, "purge") == 0)
		*method = http_purge;
	else {
		*method = http_unknown;
		return ret_not_found;
	}

	return ret_ok;
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
	case http_created:                  *str = http_created_string; break;
	case http_accepted:                 *str = http_accepted_string; break;
	case http_non_authoritative_info:   *str = http_non_authoritative_info_string; break;
	case http_no_content:               *str = http_no_content_string; break;
	case http_reset_content:            *str = http_reset_content_string; break;
	case http_partial_content:          *str = http_partial_content_string; break;
	case http_multi_status:             *str = http_multi_status_string; break;
	case http_im_used:                  *str = http_im_used_string; break;

	/* 3xx
	 */
	case http_moved_permanently:        *str = http_moved_permanently_string; break;
	case http_moved_temporarily:        *str = http_moved_temporarily_string; break;
	case http_see_other:                *str = http_see_other_string; break;
	case http_not_modified:             *str = http_not_modified_string; break;
	case http_temporary_redirect:       *str = http_temporary_redirect_string; break;
	case http_multiple_choices:         *str = http_multiple_choices_string; break;
	case http_use_proxy:                *str = http_use_proxy_string; break;

	/* 4xx
	 */
	case http_bad_request:              *str = http_bad_request_string; break;
	case http_unauthorized:             *str = http_unauthorized_string; break;
	case http_access_denied:            *str = http_access_denied_string; break;
	case http_not_found:                *str = http_not_found_string; break;
	case http_method_not_allowed:       *str = http_method_not_allowed_string; break;
	case http_not_acceptable:           *str = http_not_acceptable_string; break;
	case http_request_timeout:          *str = http_request_timeout_string; break;
	case http_gone:                     *str = http_gone_string; break;
	case http_length_required:          *str = http_length_required_string; break;
	case http_request_entity_too_large: *str = http_request_entity_too_large_string; break;
	case http_unsupported_media_type:   *str = http_unsupported_media_type_string; break;
	case http_request_uri_too_long:     *str = http_request_uri_too_long_string; break;
	case http_range_not_satisfiable:    *str = http_range_not_satisfiable_string; break;
	case http_upgrade_required:         *str = http_upgrade_required_string; break;
	case http_payment_required:         *str = http_payment_required_string; break;
	case http_proxy_auth_required:      *str = http_proxy_auth_required_string; break;
	case http_conflict:                 *str = http_conflict_string; break;
	case http_precondition_failed:      *str = http_precondition_failed_string; break;
	case http_expectation_failed:       *str = http_expectation_failed_string; break;
	case http_unprocessable_entity:     *str = http_unprocessable_entity_string; break;
	case http_locked:                   *str = http_locked_string; break;
	case http_failed_dependency:        *str = http_failed_dependency_string; break;
	case http_unordered_collection:     *str = http_unordered_collection_string; break;
	case http_retry_with:               *str = http_retry_with_string; break;

	/* 5xx
	 */
	case http_internal_error:           *str = http_internal_error_string; break;
	case http_not_implemented:          *str = http_not_implemented_string; break;
	case http_bad_gateway:              *str = http_bad_gateway_string; break;
	case http_service_unavailable:      *str = http_service_unavailable_string; break;
	case http_gateway_timeout:          *str = http_gateway_timeout_string; break;
	case http_version_not_supported:    *str = http_version_not_supported_string; break;
	case http_variant_also_negotiates:  *str = http_variant_also_negotiates_string; break;
	case http_insufficient_storage:     *str = http_insufficient_storage_string; break;
	case http_bandwidth_limit_exceeded: *str = http_bandwidth_limit_exceeded_string; break;
	case http_not_extended:             *str = http_not_extended_string; break;

	/* 1xx
	*/
	case http_continue:                 *str = http_continue_string; break;
	case http_processing:               *str = http_processing_string; break;

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
		entry_code (created);
		entry_code (accepted);
		entry_code (non_authoritative_info);
		entry_code (no_content);
		entry_code (reset_content);
		entry_code (partial_content);
		entry_code (multi_status);
		entry_code (im_used);

		/* 3xx
		 */
		entry_code (moved_permanently);
		entry_code (moved_temporarily);
		entry_code (see_other);
		entry_code (not_modified);
		entry_code (temporary_redirect);
		entry_code (multiple_choices);
		entry_code (use_proxy);

		/* 4xx
		 */
		entry_code (bad_request);
		entry_code (unauthorized);
		entry_code (access_denied);
		entry_code (not_found);
		entry_code (method_not_allowed);
		entry_code (not_acceptable);
		entry_code (request_timeout);
		entry_code (length_required);
		entry_code (gone);
		entry_code (request_entity_too_large);
		entry_code (request_uri_too_long);
		entry_code (unsupported_media_type);
		entry_code (range_not_satisfiable);
		entry_code (upgrade_required);
		entry_code (payment_required);
		entry_code (proxy_auth_required);
		entry_code (conflict);
		entry_code (precondition_failed);
		entry_code (expectation_failed);
		entry_code (unprocessable_entity);
		entry_code (locked);
		entry_code (failed_dependency);
		entry_code (unordered_collection);
		entry_code (retry_with);

		/* 5xx
		 */
		entry_code (internal_error);
		entry_code (not_implemented);
		entry_code (bad_gateway);
		entry_code (service_unavailable);
		entry_code (gateway_timeout);
		entry_code (version_not_supported);
		entry_code (insufficient_storage);
		entry_code (bandwidth_limit_exceeded);
		entry_code (not_extended);
		entry_code (variant_also_negotiates);

		/* 1xx
		 */
		entry_code (continue);
		entry_code (processing);

	default:
		LOG_WARNING (CHEROKEE_ERROR_HTTP_UNKNOWN_CODE, code);
		cherokee_buffer_add_str (buf, http_internal_error_string);
		return ret_error;
	}

	SHOULDNT_HAPPEN;
	return ret_error;
}

