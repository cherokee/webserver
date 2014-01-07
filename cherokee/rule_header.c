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
#include "rule_header.h"

#include "plugin_loader.h"
#include "server-protected.h"
#include "connection-protected.h"
#include "util.h"
#include "pcre/pcre.h"

#define ENTRIES "rule,header"

PLUGIN_INFO_RULE_EASIEST_INIT(header);


static ret_t
match_regex (cherokee_rule_header_t  *rule,
             cherokee_connection_t   *conn,
             cherokee_config_entry_t *ret_conf)
{
	int      re;
	ret_t    ret;
	char    *info     = NULL;
	cuint_t  info_len = 0;

	UNUSED(ret_conf);

	/* Find the header
	 */
	ret = cherokee_header_get_known (&conn->header,
	                                 rule->header,
	                                 &info, &info_len);

	if ((ret != ret_ok) || (info == NULL)) {
		TRACE (ENTRIES, "Request '%s'; couldn't find header(%d)\n",
		       conn->request.buf, rule->header);
		return ret_not_found;
	}

	/* Check whether it matches
	 */
	re = pcre_exec (rule->pcre, NULL,
	                info, info_len,
	                0, 0, NULL, 0);

	if (re < 0) {
		TRACE (ENTRIES, "Request '%s' didn't match header(%d) with '%s'\n",
		       conn->request.buf, rule->header, rule->match.buf);
		return ret_not_found;
	}

	TRACE (ENTRIES, "Request '%s' matched header(%d) with '%s'\n",
	       conn->request.buf, rule->header, rule->match.buf);
	return ret_ok;
}

static ret_t
match_provided (cherokee_rule_header_t  *rule,
                cherokee_connection_t   *conn,
                cherokee_config_entry_t *ret_conf)
{
	ret_t ret;

	UNUSED(ret_conf);

	/* Find the header
	 */
	ret = cherokee_header_has_known (&conn->header, rule->header);
	if (ret != ret_ok) {
		return ret_not_found;
	}

	return ret_ok;
}


static ret_t
match_complete (cherokee_rule_header_t  *rule,
                cherokee_connection_t   *conn,
                cherokee_config_entry_t *ret_conf)
{
	int re;

	UNUSED(ret_conf);

	/* Check whether it matches
	 */
	re = pcre_exec (rule->pcre, NULL,
	                conn->incoming_header.buf,
	                conn->incoming_header.len,
	                0, 0, NULL, 0);

	if (re < 0) {
		TRACE (ENTRIES, "Request '%s' didn't match complete header with '%s'\n",
		       conn->request.buf, rule->match.buf);
		return ret_not_found;
	}

	TRACE (ENTRIES, "Request '%s' matched complete header with '%s'\n",
	       conn->request.buf, rule->match.buf);
	return ret_ok;
}


static ret_t
match (cherokee_rule_header_t  *rule,
       cherokee_connection_t   *conn,
       cherokee_config_entry_t *ret_conf)
{
	/* Match regex against complete header block
	 */
	if (rule->complete_header) {
		return match_complete (rule, conn, ret_conf);
	}

	/* Check single header entry
	 */
	switch (rule->type) {
	case rule_header_type_regex:
		return match_regex (rule, conn, ret_conf);
	case rule_header_type_provided:
		return match_provided (rule, conn, ret_conf);
	}

	SHOULDNT_HAPPEN;
	return ret_error;
}


static ret_t
header_str_to_type (cherokee_buffer_t        *header,
                    cherokee_common_header_t *common_header)
{
	switch (header->buf[0]) {
	case 'A':
		if (equal_buf_str (header, "Accept-Encoding")) {
			*common_header = header_accept_encoding;
		} else if (equal_buf_str (header, "Accept-Charset")) {
			*common_header = header_accept_charset;
		} else if (equal_buf_str (header, "Accept-Language")) {
			*common_header = header_accept_language;
		} else if (equal_buf_str (header, "Accept")) {
			*common_header = header_accept;
		} else if (equal_buf_str (header, "Authorization")) {
			*common_header = header_authorization;
		} else {
			goto unknown;
		}
		break;
	case 'C':
		if (equal_buf_str (header, "Cache-Control")) {
			*common_header = header_cache_control;
		} else if (equal_buf_str (header, "Connection")) {
			*common_header = header_connection;
		} else if (equal_buf_str (header, "Content-Encoding")) {
			*common_header = header_content_encoding;
		} else if (equal_buf_str (header, "Content-Length")) {
			*common_header = header_content_length;
		} else if (equal_buf_str (header, "Content-Type")) {
			*common_header = header_content_type;
		} else if (equal_buf_str (header, "Cookie")) {
			*common_header = header_cookie;
		} else {
			goto unknown;
		}
		break;
	case 'D':
		if (equal_buf_str (header, "DNT")) {
			*common_header = header_dnt;
		} else {
			goto unknown;
		}
		break;
	case 'E':
		if (equal_buf_str (header, "Expect")) {
			*common_header = header_expect;
		} else {
			goto unknown;
		}
		break;
	case 'H':
		if (equal_buf_str (header, "Host")) {
			*common_header = header_host;
		} else {
			goto unknown;
		}
		break;
	case 'I':
		if (equal_buf_str (header, "If-Modified-Since")) {
			*common_header = header_if_modified_since;
		} else if (equal_buf_str (header, "If-None-Match")) {
			*common_header = header_if_none_match;
		} else if (equal_buf_str (header, "If-Range")) {
			*common_header = header_if_range;
		} else {
			goto unknown;
		}
		break;
	case 'K':
		if (equal_buf_str (header, "Keep-Alive")) {
			*common_header = header_keepalive;
		} else {
			goto unknown;
		}
		break;
	case 'L':
		if (equal_buf_str (header, "Location")) {
			*common_header = header_location;
		} else {
			goto unknown;
		}
		break;
	case 'R':
		if (equal_buf_str (header, "Range")) {
			*common_header = header_range;
		} else if (equal_buf_str (header, "Referer")) {
			*common_header = header_referer;
		} else {
			goto unknown;
		}
		break;
	case 'S':
		if (equal_buf_str (header, "Set-Cookie")) {
			*common_header = header_set_cookie;
		} else {
			goto unknown;
		}
		break;
	case 'T':
		if (equal_buf_str (header, "Transfer-Encoding")) {
			*common_header = header_transfer_encoding;
		} else {
			goto unknown;
		}
		break;
	case 'U':
		if (equal_buf_str (header, "Upgrade")) {
			*common_header = header_upgrade;
		} else if (equal_buf_str (header, "User-Agent")) {
			*common_header = header_user_agent;
		} else {
			goto unknown;
		}
		break;
	case 'X':
		if (equal_buf_str (header, "X-Forwarded-For")) {
			*common_header = header_x_forwarded_for;
		} else if (equal_buf_str (header, "X-Forwarded-Host")) {
			*common_header = header_x_forwarded_host;
		} else if (equal_buf_str (header, "X-Real-IP")) {
			*common_header = header_x_real_ip;
		} else {
			goto unknown;
		}
		break;
	default:
	unknown:
		LOG_CRITICAL (CHEROKEE_ERROR_RULE_HEADER_UNKNOWN_HEADER, header->buf);
		return ret_error;
	}

	return ret_ok;
}

static ret_t
type_str_to_type (cherokee_buffer_t           *type_str,
                  cherokee_rule_header_type_t *type)
{
	if (equal_buf_str (type_str, "regex")) {
		*type = rule_header_type_regex;
	} else if (equal_buf_str (type_str, "provided")) {
		*type = rule_header_type_provided;
	} else {
		LOG_CRITICAL (CHEROKEE_ERROR_RULE_HEADER_UNKNOWN_TYPE, type_str->buf);
		return ret_error;
	}

	return ret_ok;
}


static ret_t
configure (cherokee_rule_header_t    *rule,
           cherokee_config_node_t    *conf,
           cherokee_virtual_server_t *vsrv)
{
	ret_t                   ret;
	cherokee_buffer_t      *type     = NULL;
	cherokee_buffer_t      *header   = NULL;
	cherokee_regex_table_t *regexs   = VSERVER_SRV(vsrv)->regexs;

	/* Complete header
	 */
	cherokee_config_node_read_bool (conf, "complete", &rule->complete_header);

	/* Read the header. Eg: Referer
	 */
	if (! rule->complete_header) {
		ret = cherokee_config_node_read (conf, "header", &header);
		if (ret != ret_ok) {
			LOG_ERROR (CHEROKEE_ERROR_RULE_NO_PROPERTY,
			           RULE(rule)->priority, "header");
			return ret_error;
		}

		ret = header_str_to_type (header, &rule->header);
		if (ret != ret_ok) {
			return ret;
		}

		/* Type: regex | provided
		 */
		ret = cherokee_config_node_read (conf, "type", &type);
		if (ret == ret_ok) {
			ret = type_str_to_type (type, &rule->type);
			if (ret != ret_ok) {
				return ret;
			}
		}
	}

	/* Read regex match
	 */
	ret = cherokee_config_node_copy (conf, "match", &rule->match);
	if (ret != ret_ok) {
		if ((type) && equal_buf_str (type, "regex")) {
			LOG_ERROR (CHEROKEE_ERROR_RULE_NO_PROPERTY,
			           RULE(rule)->priority, "match");
			return ret_error;
		}
	}

	/* Compile the regular expression
	 */
	if (! cherokee_buffer_is_empty (&rule->match)) {
		ret = cherokee_regex_table_add (regexs, rule->match.buf);
		if (ret != ret_ok)
			return ret;

		ret = cherokee_regex_table_get (regexs, rule->match.buf, &rule->pcre);
		if (ret != ret_ok)
			return ret;
	}

	return ret_ok;
}

static ret_t
_free (void *p)
{
	cherokee_rule_header_t *rule = RULE_HEADER(p);

	cherokee_buffer_mrproper (&rule->match);
	return ret_ok;
}


ret_t
cherokee_rule_header_new (cherokee_rule_header_t **rule)
{
	CHEROKEE_NEW_STRUCT (n, rule_header);

	/* Parent class constructor
	 */
	cherokee_rule_init_base (RULE(n), PLUGIN_INFO_PTR(header));

	/* Virtual methods
	 */
	RULE(n)->match     = (rule_func_match_t) match;
	RULE(n)->configure = (rule_func_configure_t) configure;
	MODULE(n)->free    = (module_func_free_t) _free;

	/* Properties
	 */
	n->pcre            = NULL;
	n->type            = rule_header_type_regex;
	n->complete_header = false;

	cherokee_buffer_init (&n->match);

	*rule = n;
	return ret_ok;
}
