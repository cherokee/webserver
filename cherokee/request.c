/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/* Cherokee
 *
 * Authors:
 *      Alvaro Lopez Ortega <alvaro@alobbs.com>
 *
 * Copyright (C) 2001-2011 Alvaro Lopez Ortega
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
#include "request.h"
#include "request-protected.h"
#include "bogotime.h"

#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>
#include <sys/types.h>

#ifdef HAVE_PWD_H
# include <pwd.h>
#endif

#ifdef HAVE_SYS_UIO_H
# include <sys/uio.h>
#endif

#ifdef HAVE_SYS_SOCKET_H
# include <sys/socket.h>
#endif

#ifdef HAVE_ARPA_INET_H
# include <arpa/inet.h>
#endif

#ifdef HAVE_NETINET_IN_H
# include <netinet/in.h>
#endif

#ifdef HAVE_NETINET_TCP_H
# include <netinet/tcp.h>
#endif

#ifdef HAVE_STRINGS_H
# include <strings.h>
#endif

#include "util.h"
#include "list.h"
#include "http.h"
#include "handler.h"
#include "thread.h"
#include "handler_error.h"
#include "buffer.h"
#include "config_entry.h"
#include "server-protected.h"
#include "access.h"
#include "virtual_server.h"
#include "socket.h"
#include "header.h"
#include "header_op.h"
#include "header-protected.h"
#include "iocache.h"
#include "dtm.h"
#include "flcache.h"

#define ENTRIES "core,request"


ret_t
cherokee_request_new  (cherokee_request_t **req)
{
	CHEROKEE_NEW_STRUCT(n, request);

	INIT_LIST_HEAD (&n->list_node);

	n->error_code                = http_ok;
	n->phase                     = phase_reading_header;
	n->auth_type                 = http_auth_nothing;
	n->req_auth_type             = http_auth_nothing;
	n->upgrade                   = http_upgrade_nothing;
	n->options                   = conn_op_nothing;
	n->handler                   = NULL;
	n->encoder                   = NULL;
	n->encoder_new_func          = NULL;
	n->encoder_props             = NULL;
	n->logger_ref                = NULL;
	n->range_start               = -1;
	n->range_end                 = -1;
	n->vserver                   = NULL;
	n->arguments                 = NULL;
	n->mmaped                    = NULL;
	n->mmaped_len                = 0;
	n->io_entry_ref              = NULL;
	n->thread                    = NULL;
	n->conn                      = NULL;
	n->rx                        = 0;
	n->tx                        = 0;
	n->rx_partial                = 0;
	n->tx_partial                = 0;
	n->traffic_next              = 0;
	n->validator                 = NULL;
	n->expiration                = cherokee_expiration_none;
	n->expiration_time           = 0;
	n->expiration_prop           = cherokee_expiration_prop_none;
	n->respins                   = 0;
	n->limit_rate                = false;
	n->limit_bps                 = 0;
	n->limit_blocked_until       = 0;

	cherokee_buffer_init (&n->buffer);
	cherokee_buffer_init (&n->header_buffer_out);
	cherokee_buffer_init (&n->encoder_buffer);
	cherokee_buffer_init (&n->logger_real_ip);

	cherokee_buffer_init (&n->local_directory);
	cherokee_buffer_init (&n->web_directory);
	cherokee_buffer_init (&n->effective_directory);
	cherokee_buffer_init (&n->userdir);
	cherokee_buffer_init (&n->request);
	cherokee_buffer_init (&n->pathinfo);
	cherokee_buffer_init (&n->redirect);
	cherokee_buffer_init (&n->host);
	cherokee_buffer_init (&n->host_port);
	cherokee_buffer_init (&n->self_trace);

	n->error_internal_code = http_unset;
	cherokee_buffer_init (&n->error_internal_url);
	cherokee_buffer_init (&n->error_internal_qs);

	cherokee_buffer_init (&n->query_string);
	cherokee_buffer_init (&n->request_original);
	cherokee_buffer_init (&n->query_string_original);

	cherokee_header_init (&n->header, header_type_request);
	cherokee_post_init (&n->post, n);

	memset (n->regex_ovector, 0, OVECTOR_LEN * sizeof(int));
	n->regex_ovecsize = 0;
	memset (n->regex_host_ovector, 0, OVECTOR_LEN * sizeof(int));
	n->regex_host_ovecsize = 0;

	n->chunked_encoding     = false;
	n->chunked_sent         = 0;
	n->chunked_last_package = false;
	cherokee_buffer_init (&n->chunked_len);

	cherokee_config_entry_ref_init (&n->config_entry);
	cherokee_flcache_conn_init (&n->flcache);

	*req = n;
	return ret_ok;
}


ret_t
cherokee_request_free (cherokee_request_t *req)
{
	cherokee_header_mrproper (&req->header);

	if (req->handler != NULL) {
		cherokee_handler_free (req->handler);
		req->handler = NULL;
	}

	if (req->encoder != NULL) {
		cherokee_encoder_free (req->encoder);
		req->encoder = NULL;
	}

	cherokee_post_mrproper (&req->post);

	cherokee_buffer_mrproper (&req->request);
	cherokee_buffer_mrproper (&req->request_original);
	cherokee_buffer_mrproper (&req->query_string_original);
	cherokee_buffer_mrproper (&req->logger_real_ip);

	cherokee_buffer_mrproper (&req->pathinfo);
	cherokee_buffer_mrproper (&req->buffer);
	cherokee_buffer_mrproper (&req->header_buffer_in);
	cherokee_buffer_mrproper (&req->header_buffer_out);
	cherokee_buffer_mrproper (&req->query_string);
	cherokee_buffer_mrproper (&req->encoder_buffer);

	cherokee_buffer_mrproper (&req->local_directory);
	cherokee_buffer_mrproper (&req->web_directory);
	cherokee_buffer_mrproper (&req->effective_directory);
	cherokee_buffer_mrproper (&req->userdir);
	cherokee_buffer_mrproper (&req->redirect);
	cherokee_buffer_mrproper (&req->host);
	cherokee_buffer_mrproper (&req->host_port);
	cherokee_buffer_mrproper (&req->self_trace);
	cherokee_buffer_mrproper (&req->chunked_len);

	cherokee_buffer_mrproper (&req->error_internal_url);
	cherokee_buffer_mrproper (&req->error_internal_qs);

	if (req->validator != NULL) {
		cherokee_validator_free (req->validator);
		req->validator = NULL;
	}

	if (req->arguments != NULL) {
		cherokee_avl_free (req->arguments,
				   (cherokee_func_free_t) cherokee_buffer_free);
		req->arguments = NULL;
	}

	free (req);
	return ret_ok;
}


ret_t
cherokee_request_clean (cherokee_request_t *req,
			cherokee_boolean_t  reuse)
{
	size_t             crlf_len;
	uint32_t           header_len;
	cherokee_server_t *srv         = REQ_SRV(req);

	/* I/O cache entry reference
	 */
	if (req->io_entry_ref != NULL) {
		cherokee_iocache_entry_unref (&req->io_entry_ref);
		req->io_entry_ref = NULL;
	}

	/* Front-line cache
	 */
	cherokee_flcache_conn_clean (&req->flcache);

	/* TCP cork
	 */
	if (req->options & conn_op_tcp_cork) {
		cherokee_request_set_cork (req, false);
		BIT_UNSET (req->options, conn_op_tcp_cork);
	}

	/* POST track
	 */
	if (srv && srv->post_track) {
		srv->post_track->func_unregister (srv->post_track, &req->post);
	}

	/* Free handlers before cleaning the conn properties. Its
	 * free() function might require to read some of them.
	 */
	if (req->handler != NULL) {
		cherokee_handler_free (req->handler);
		req->handler = NULL;
	}

	if (req->encoder != NULL) {
		cherokee_encoder_free (req->encoder);
		req->encoder = NULL;
	}
	req->encoder_new_func = NULL;
	req->encoder_props    = NULL;

	if (req->validator != NULL) {
		cherokee_validator_free (req->validator);
		req->validator = NULL;
	}

	req->phase                = phase_reading_header;
	req->auth_type            = http_auth_nothing;
	req->req_auth_type        = http_auth_nothing;
	req->upgrade              = http_upgrade_nothing;
	req->options              = conn_op_nothing;
	req->error_code           = http_ok;
	req->range_start          = -1;
	req->range_end            = -1;
	req->logger_ref           = NULL;
	req->mmaped               = NULL;
	req->mmaped_len           = 0;
	req->rx                   = 0;
	req->tx                   = 0;
	req->rx_partial           = 0;
	req->tx_partial           = 0;
	req->traffic_next         = 0;
	req->expiration           = cherokee_expiration_none;
	req->expiration_time      = 0;
	req->expiration_prop      = cherokee_expiration_prop_none;
	req->chunked_encoding     = false;
	req->chunked_sent         = 0;
	req->chunked_last_package = false;
	req->respins              = 0;
	req->limit_rate           = false;
	req->limit_bps            = 0;
	req->limit_blocked_until  = 0;

	memset (req->regex_ovector, 0, OVECTOR_LEN * sizeof(int));
	req->regex_ovecsize = 0;
	memset (req->regex_host_ovector, 0, OVECTOR_LEN * sizeof(int));
	req->regex_host_ovecsize = 0;

	cherokee_request_poll_clean (&req->polling_aim);

	cherokee_post_clean (&req->post);
	cherokee_buffer_mrproper (&req->encoder_buffer);

	cherokee_buffer_clean (&req->request);
	cherokee_buffer_clean (&req->request_original);
	cherokee_buffer_clean (&req->query_string_original);
	cherokee_buffer_clean (&req->logger_real_ip);

	cherokee_buffer_clean (&req->pathinfo);
	cherokee_buffer_clean (&req->local_directory);
	cherokee_buffer_clean (&req->web_directory);
	cherokee_buffer_clean (&req->effective_directory);
	cherokee_buffer_clean (&req->userdir);
	cherokee_buffer_clean (&req->redirect);
	cherokee_buffer_clean (&req->host);
	cherokee_buffer_clean (&req->host_port);
	cherokee_buffer_clean (&req->query_string);
	cherokee_buffer_clean (&req->self_trace);
	cherokee_buffer_clean (&req->chunked_len);
	cherokee_buffer_clean (&req->header_buffer_out);

	req->error_internal_code = http_unset;
	cherokee_buffer_clean (&req->error_internal_url);
	cherokee_buffer_clean (&req->error_internal_qs);

	cherokee_config_entry_ref_clean (&req->config_entry);

	if (req->arguments != NULL) {
		cherokee_avl_free (req->arguments,
				   (cherokee_func_free_t) cherokee_buffer_free);
		req->arguments = NULL;
	}

	/* Drop out the last incoming header
	 */
	cherokee_header_get_length (&req->header, &header_len);

	cherokee_header_clean (&req->header);
	cherokee_buffer_clean (&req->buffer);

	if (reuse) {
		/* Skip trailing CRLF (which may be sent by some HTTP clients)
		 * only if the number of CRLFs is within the predefine count
		 * limit otherwise ignore trailing CRLFs so that they will be
		 * handled in next request.  This may avoid a subsequent real
		 * move_to_begin of the contents left in the buffer.
		 */
		crlf_len = cherokee_buffer_cnt_spn (&req->header_buffer_in, header_len, CRLF);
		header_len += (crlf_len <= MAX_HEADER_CRLF) ? crlf_len : 0;

		cherokee_buffer_move_to_begin (&req->header_buffer_in, header_len);

		TRACE (ENTRIES, "conn %p, %s headers\n", req,
		       !cherokee_buffer_is_empty (&req->header_buffer_in) ? "has" : "doesn't have");
	} else {
		cherokee_buffer_clean (&req->header_buffer_in);
	}

	return ret_ok;
}


ret_t
cherokee_request_clean_close (cherokee_request_t *req)
{
	// TODO: Implement properties clean
	//
	//
	return ret_ok;


	/* /\* Close and clean the socket */
	/*  *\/ */
	/* cherokee_socket_close (&req->socket); */
	/* cherokee_socket_clean (&req->socket); */

	/* /\* Make sure the connection Keep-Alive is disabled */
	/*  *\/ */
	/* req->keepalive = 0; */
	/* cherokee_buffer_clean (&req->incoming_header); */

	/* /\* Clean the connection object */
	/*  *\/ */
	/* cherokee_request_clean (req, false); */
	/* return ret_ok; */
}


ret_t
cherokee_request_setup_error_handler (cherokee_request_t *req)
{
	ret_t                       ret;
	cherokee_server_t          *srv;
	cherokee_virtual_server_t  *vsrv;
	cherokee_config_entry_t    *entry;

	/* NOTE ABOUT THIS FUNCTION: It ought to be call only from
	 * cherokee/thread.c. Right after calling the function it must
	 * call continue (becase req->error_code might have changed).
	 */

	srv   = REQ_SRV(req);
	vsrv  = REQ_VSRV(req);
	entry = vsrv->error_handler;

	TRACE(ENTRIES, "Setting up error handler: %d (respins=%d)\n",
	      req->error_code, req->respins);

	/* It has a common handler. It has to be freed.
	 */
	if (req->handler != NULL) {
		cherokee_handler_free (req->handler);
		req->handler = NULL;
	}

	/* The rid of the encoder as well.
	 */
	if (req->encoder_new_func) {
		req->encoder_new_func = NULL;
		req->encoder_props    = NULL;
	}

	if (req->encoder != NULL) {
		cherokee_encoder_free (req->encoder);
		req->encoder = NULL;
	}

	/* Loop detection: Set the default error handler and exit.
	 */
	req->respins += 1;
	if (req->respins > RESPINS_MAX) {
		goto safe;
	}

	/* Create a new error handler
	 */
	if ((entry != NULL) && (entry->handler_new_func != NULL)) {
		ret = entry->handler_new_func ((void **) &req->handler, req, entry->handler_properties);
		switch (ret) {
		case ret_ok:
			/* At this point, two different things might happen:
			 * - It has got a common handler like handler_redir
			 * - It has got an error handler like handler_error
			 */
			req->phase = phase_init;

			TRACE(ENTRIES, "Error handler set. Phase is '%s' now.\n", "init");
			goto out;

		case ret_eagain:
			/* It's an internal error redirection
			 */
 			cherokee_buffer_clean (&req->pathinfo);
 			cherokee_buffer_clean (&req->query_string);
			cherokee_buffer_clean (&req->web_directory);
			cherokee_buffer_clean (&req->local_directory);

			req->error_code = http_ok;
			req->phase = phase_setup_connection;

			TRACE(ENTRIES, "Internal redir. Switching to phase 'setup_connection', respins=%d, request=%s\n", req->respins, req->request.buf);
			goto clean;

		default:
			TRACE(ENTRIES, "Could not set the error handler. Relying on %s\n", "the default error handler");
			break;
		}
	}

safe:
	/* If something was wrong, try with the default error handler
	 */
	ret = cherokee_handler_error_new (&req->handler, req, NULL);
	if (unlikely (ret != ret_ok))
		return ret_error;

	TRACE(ENTRIES, "Default Error handler set. Phase is '%s' now.\n", "init");
	req->phase = phase_init;

out:
#ifdef TRACE_ENABLED
	{
		const char *name = NULL;

		cherokee_module_get_name (MODULE(req->handler), &name);
		TRACE(ENTRIES, "New handler '%s'\n", name ? name : "unknown");
	}
#endif

	/* Let's keep alive 3xx, 404, and 410 responses.
	 */
	if (http_type_500 (req->error_code)) {
		req->keepalive = 0;

	} else if (! http_type_300 (req->error_code) &&   /* 3xx */
		   (req->error_code != http_gone) &&      /* 410 */
		   (req->error_code != http_not_found))   /* 404 */
	{
		req->keepalive = 0;
	}

	ret = ret_ok;

clean:
	/* Nothing should be mmaped any longer
	 */
	if (req->io_entry_ref != NULL) {
		cherokee_iocache_entry_unref (&req->io_entry_ref);
		req->io_entry_ref = NULL;
	}

	req->mmaped     = NULL;
	req->mmaped_len = 0;

	cherokee_flcache_conn_clean (&req->flcache);

	/* It does not matter whether the previous handler wanted to
	 * use Chunked encoding.
	 */
	req->chunked_encoding = false;

	return ret;
}


ret_t
cherokee_request_setup_hsts_handler (cherokee_request_t *req)
{
	ret_t              ret;
	cherokee_list_t   *i;
	int                port = -1;
	cherokee_server_t *srv  = REQ_SRV(req);

	/* Redirect to:
	 * "https://" + host + request + query_string
	 */
	cherokee_buffer_clean (&req->redirect);

	/* 1.- Proto */
	cherokee_buffer_add_str (&req->redirect, "https://");

	/* 2.- Host */
	cherokee_request_build_host_string (req, &req->redirect);

	/* 3.- Port */
	list_for_each (i, &srv->listeners) {
		if (BIND_IS_TLS(i)) {
			port = BIND(i)->port;
			break;
		}
	}

	if ((port != -1) &&
	    (! http_port_is_standard (port, true)))
	{
		cherokee_buffer_add_char    (&req->redirect, ':');
		cherokee_buffer_add_ulong10 (&req->redirect, port);
	}

	/* 4.- Request */
	cherokee_buffer_add_buffer (&req->redirect, &req->request);

	if (req->query_string.len > 0) {
		cherokee_buffer_add_char   (&req->redirect, '?');
		cherokee_buffer_add_buffer (&req->redirect, &req->query_string);
	}

	/* 301 response: Move Permanetly
	 */
	req->error_code = http_moved_permanently;

	/* Instance the handler object
	 */
	ret = cherokee_handler_error_new (&req->handler, req, NULL);
	if (unlikely (ret != ret_ok)) {
		return ret_error;
	}

	TRACE (ENTRIES, "HSTS redirection handler set. Phase is '%s' now.\n", "init");
	req->phase = phase_init;

	return ret_ok;
}


static void
build_response_header_authentication (cherokee_request_t *req, cherokee_buffer_t *buffer)
{
	ret_t ret;

	/* Basic Authenticatiom
	 * Eg: WWW-Authenticate: Basic realm=""
	 */
	if (req->auth_type & http_auth_basic) {
		cherokee_buffer_add_str (buffer, "WWW-Authenticate: Basic realm=\"");
		cherokee_buffer_add_buffer (buffer, req->config_entry.auth_realm);
		cherokee_buffer_add_str (buffer, "\""CRLF);
	}

	/* Digest Authentication, Eg:
	 * WWW-Authenticate: Digest realm="", qop="auth,auth-int",
	 *                   nonce="", opaque=""
	 */
	if (req->auth_type & http_auth_digest) {
		cherokee_thread_t *thread    = REQ_THREAD(req);
		cherokee_buffer_t *new_nonce = THREAD_TMP_BUF1(thread);

		/* Realm
		 */
		cherokee_buffer_add_str (buffer, "WWW-Authenticate: Digest realm=\"");
		cherokee_buffer_add_buffer (buffer, req->config_entry.auth_realm);
		cherokee_buffer_add_str (buffer, "\", ");

		/* Nonce:
		 * "nonce=\"%s\", "
		 */
		ret = ret_not_found;

		if ((req->validator != NULL) &&
		    (! cherokee_buffer_is_empty (&req->validator->nonce)))
		{
			ret = cherokee_nonce_table_check (REQ_SRV(req)->nonces,
							  &req->validator->nonce);
		}

		cherokee_buffer_add_str (buffer, "nonce=\"");
		if (ret != ret_ok) {
			cherokee_buffer_clean (new_nonce);
			cherokee_nonce_table_generate (REQ_SRV(req)->nonces, req, new_nonce);
			cherokee_buffer_add_buffer (buffer, new_nonce);
		} else {
			cherokee_buffer_add_buffer (buffer, &req->validator->nonce);
		}
		cherokee_buffer_add_str (buffer, "\", ");


		/* Quality of protection: auth, auth-int, auth-conf
		 * Algorithm: MD5
		 */
		cherokee_buffer_add_str (buffer, "qop=\"auth\", algorithm=\"MD5\""CRLF);
	}
}


ret_t
cherokee_request_read_post (cherokee_request_t *req)
{
	/* Shortcut
	 */
	if (req->handler->read_post == NULL) {
		return ret_ok;
	}

	return cherokee_handler_read_post (req->handler);
}


void
cherokee_request_add_expiration_header (cherokee_request_t *req,
					cherokee_buffer_t  *buffer,
					cherokee_boolean_t  use_maxage)
{
	time_t             exp_time;
	struct tm          exp_tm;
	char               bufstr[DTM_SIZE_GMTTM_STR + 2];
	size_t             szlen               = 0;
	cherokee_boolean_t first_prop          = true;

	/* Expires, and Cache-Control: max-age
	 */
	switch (req->expiration) {
	case cherokee_expiration_epoch:
		cherokee_buffer_add_str (buffer, "Expires: Tue, 01 Jan 1970 00:00:01 GMT" CRLF);
		if (req->expiration_prop != cherokee_expiration_prop_none) {
			cherokee_buffer_add_str (buffer, "Cache-Control: ");
		}
		break;

	case cherokee_expiration_max:
		cherokee_buffer_add_str (buffer, "Expires: Thu, 31 Dec 2037 23:55:55 GMT" CRLF);
		cherokee_buffer_add_str (buffer, "Cache-Control: max-age=315360000");
		first_prop = false;
		break;

	case cherokee_expiration_time:
		exp_time = (cherokee_bogonow_now + req->expiration_time);
		cherokee_gmtime (&exp_time, &exp_tm);
		szlen = cherokee_dtm_gmttm2str (bufstr, sizeof(bufstr), &exp_tm);

		cherokee_buffer_add_str (buffer, "Expires: ");
		cherokee_buffer_add (buffer, bufstr, szlen);
		cherokee_buffer_add_str (buffer, CRLF);

		if (use_maxage) {
			cherokee_buffer_add_str (buffer, "Cache-Control: max-age=");
			cherokee_buffer_add_long10 (buffer, req->expiration_time);
			first_prop = false;
		} else if (req->expiration_prop) {
			cherokee_buffer_add_str (buffer, "Cache-Control: ");
		}
		break;

	default:
		SHOULDNT_HAPPEN;
	}

	/* No properties shortcut
	 */
	if (req->expiration_prop == cherokee_expiration_prop_none) {
		if (! first_prop)
			cherokee_buffer_add_str (buffer, CRLF);
		return;
	}

	/* Caching related properties
	 */
#define handle_comma						\
	do {							\
		if (first_prop) {				\
			first_prop = false;			\
		} else {					\
			cherokee_buffer_add_str (buffer, ", "); \
		}						\
	} while (false)

	if (req->expiration_prop & cherokee_expiration_prop_public) {
		handle_comma;
		cherokee_buffer_add_str (buffer, "public");
	} else if (req->expiration_prop & cherokee_expiration_prop_private) {
		handle_comma;
		cherokee_buffer_add_str (buffer, "private");
	} else if (req->expiration_prop & cherokee_expiration_prop_no_cache) {
		handle_comma;
		cherokee_buffer_add_str (buffer, "no-cache");
	}

	if (req->expiration_prop & cherokee_expiration_prop_no_store) {
		handle_comma;
		cherokee_buffer_add_str (buffer, "no-store");
	}
	if (req->expiration_prop & cherokee_expiration_prop_no_transform) {
		handle_comma;
		cherokee_buffer_add_str (buffer, "no-transform");
	}
	if (req->expiration_prop & cherokee_expiration_prop_must_revalidate) {
		handle_comma;
		cherokee_buffer_add_str (buffer, "must-revalidate");
	}
	if (req->expiration_prop & cherokee_expiration_prop_proxy_revalidate) {
		handle_comma;
		cherokee_buffer_add_str (buffer, "proxy-revalidate");
	}

#undef handle_comma

	cherokee_buffer_add_str (buffer, CRLF);
}


static void
build_response_header (cherokee_request_t *req,
		       cherokee_buffer_t  *buffer)
{
	cherokee_buffer_t *tmp1 = THREAD_TMP_BUF1(REQ_THREAD(req));

	/* Build the response header.
	 */
	cherokee_buffer_clean (buffer);

	/* Add protocol string
	 */
	switch (req->header.version) {
	case http_version_10:
		cherokee_buffer_add_str (buffer, "HTTP/1.0 ");
		break;
	case http_version_11:
	default:
		cherokee_buffer_add_str (buffer, "HTTP/1.1 ");
		break;
	}

	/* HTTP response code
	 */
	if (unlikely (req->error_internal_code != http_unset)) {
		cherokee_http_code_copy (req->error_internal_code, buffer);
	} else {
		cherokee_http_code_copy (req->error_code, buffer);
	}

	cherokee_buffer_add_str (buffer, CRLF);

	/* Add the "Connection:" header
	 */
	if (req->upgrade != http_upgrade_nothing) {
		cherokee_buffer_add_str (buffer, "Connection: Upgrade"CRLF);

	} else if (req->keepalive > 1) {
		if (req->header.version < http_version_11) {
			cherokee_buffer_add_str     (buffer, "Connection: Keep-Alive"CRLF);
			cherokee_buffer_add_buffer  (buffer, req->timeout_header);
			cherokee_buffer_add_str     (buffer, ", max=");
			cherokee_buffer_add_ulong10 (buffer, req->keepalive);
			cherokee_buffer_add_str     (buffer, CRLF);
		}
	} else {
		cherokee_buffer_add_str (buffer, "Connection: close"CRLF);
	}

	/* Chunked transfer
	 */
	if (req->chunked_encoding) {
		cherokee_buffer_add_str (buffer, "Transfer-Encoding: chunked" CRLF);
	}

	/* Exit now if the handler already added all the headers
	 */
	if ((req->handler != NULL) &&
	    (HANDLER_SUPPORTS (req->handler, hsupport_full_headers)))
		return;

	/* Date
	 */
	cherokee_buffer_add_str (buffer, "Date: ");
	cherokee_buffer_add_buffer (buffer, &cherokee_bogonow_strgmt);
	cherokee_buffer_add_str (buffer, CRLF);

	/* Add the Server header
	 */
	cherokee_buffer_add_str (buffer, "Server: ");
	cherokee_buffer_add_buffer (buffer, &REQ_BIND(req)->server_string);
	cherokee_buffer_add_str (buffer, CRLF);

	/* Authentication
	 */
	if (req->config_entry.auth_realm != NULL) {
		if ((req->error_code          == http_unauthorized) ||
		    (req->error_internal_code == http_unauthorized))
		{
			build_response_header_authentication (req, buffer);
		}
	}

	/* Redirected connections
	 */
	if (req->redirect.len >= 1) {
		cherokee_buffer_add_str (buffer, "Location: ");
		cherokee_buffer_add_buffer (buffer, &req->redirect);
		cherokee_buffer_add_str (buffer, CRLF);
	}

	/* Encoder headers
	 */
	if (req->encoder) {
		/* Front-line Cache (in) */
		if (req->flcache.mode == flcache_mode_in) {
			cherokee_buffer_clean (tmp1);
			cherokee_encoder_add_headers (req->encoder, tmp1);
			cherokee_buffer_add_buffer (buffer, tmp1);
			cherokee_buffer_add_buffer (&req->flcache.header, tmp1);
		}
		/* Straight */
		else {
			cherokee_encoder_add_headers (req->encoder, buffer);
		}
	}

	/* HSTS support
	 */
	if ((req->socket.is_tls == TLS)    &&
	    (REQ_VSRV(req)->hsts.enabled))
	{
		cherokee_buffer_add_str     (buffer, "Strict-Transport-Security: ");
		cherokee_buffer_add_str     (buffer, "max-age=");
		cherokee_buffer_add_ulong10 (buffer, (culong_t) REQ_VSRV(req)->hsts.max_age);

		if (REQ_VSRV(req)->hsts.subdomains) {
			cherokee_buffer_add_str (buffer, "; includeSubdomains");
		}

		cherokee_buffer_add_str (buffer, CRLF);
	}
}


static void
build_response_header_final (cherokee_request_t *req,
			     cherokee_buffer_t  *buffer)
{
	cherokee_buffer_t *tmp1 = THREAD_TMP_BUF1(REQ_THREAD(req));

	/* Expiration
	 */
	if (req->expiration != cherokee_expiration_none) {
		/* Add expiration headers if Front-Line Cache is not
		 * enabled. If it were, the header was already stored
		 * in the response file.
		 */
		if (req->flcache.mode == flcache_mode_undef) {
			cherokee_request_add_expiration_header (req, buffer, true);
		} else if (req->flcache.mode == flcache_mode_in) {
			cherokee_header_del_entry (buffer, "Cache-Control", 13);
			cherokee_request_add_expiration_header (req, buffer, true);
		}
	}

	/* Headers ops:
	 * This must be the last operation
	 */
	if (req->config_entry.header_ops) {
		cherokee_header_op_render (req->config_entry.header_ops, &req->buffer);
	}
}


ret_t
cherokee_request_build_header (cherokee_request_t *req)
{
	ret_t ret;

	/* Front-line cache
	 */
	if (req->flcache.mode == flcache_mode_out) {
		ret = cherokee_flcache_conn_send_header (&req->flcache, req);
		if (ret != ret_ok) {
			return ret;
		}

		goto out;
	}

	/* Try to get the headers from the handler
	 */
	ret = cherokee_handler_add_headers (req->handler, &req->header_buffer_out);
	if (unlikely (ret != ret_ok)) {
		switch (ret) {
		case ret_eof:
		case ret_error:
		case ret_eagain:
			return ret;
		default:
			RET_UNKNOWN(ret);
			return ret_error;
		}
	}

	/* Front-line Cache (in): Handler headers
	 */
	if (req->flcache.mode == flcache_mode_in) {
		cherokee_buffer_add_buffer (&req->flcache.header, &req->header_buffer_out);

		/* Add X-Cache miss
		 */
		cherokee_buffer_add_str (&req->header_buffer_out, "X-Cache: MISS from ");
		cherokee_request_build_host_port_string (req, &req->header_buffer_out);
		cherokee_buffer_add_str (&req->header_buffer_out, CRLF);
	}

	/* Replies with no body cannot use chunked encoding
	 */
	if ((! http_code_with_body (req->error_code)) ||
	    (! http_method_with_body (req->header.method)))
	{
		req->chunked_encoding = false;
	}

	/* If the connection is using Keep-Alive, it must either know
	 * the length or use chunked encoding. Otherwise, Keep-Alive
	 * has to be turned off.
	 */
	if (req->keepalive != 0) {
		if (! HANDLER_SUPPORTS (req->handler, hsupport_length)) {
			if (! req->chunked_encoding) {
				req->keepalive = 0;
			}
		} else {
			req->chunked_encoding = false;
		}
	}

	/* Instance an encoder if needed. Special-case: the proxy
	 * handler might have instanced a encoded at this stage.
	 */
	if ((req->encoder == NULL) &&
	    (req->encoder_new_func))
	{
		/* Internally checks conn_op_cant_encoder */
		cherokee_request_instance_encoder (req);
	}

out:
	/* Add the server headers (phase 1)
	 */
	build_response_header (req, &req->buffer);

	/* Add handler headers
	 */
	cherokee_buffer_add_buffer (&req->buffer, &req->header_buffer_out);

	/* Add the server headers (phase 2)
	 */
	build_response_header_final (req, &req->buffer);

	/* EOH
	 */
	cherokee_buffer_add_str (&req->buffer, CRLF);
	TRACE(ENTRIES, "Replying:\n%s", req->buffer.buf);

	return ret_ok;
}


ret_t
cherokee_request_send_header_and_mmaped (cherokee_request_t *req)
{
	size_t  re = 0;
	ret_t   ret;
	int16_t	nvec = 1;
	struct iovec bufs[2];

	/* 1.- Special case: There is not header to send
	 * because it has been sent by writev() (see below)
	 */
	if (cherokee_buffer_is_empty (&req->buffer)) {
		ret = cherokee_socket_write (&req->socket, req->mmaped, req->mmaped_len, &re);
		if (unlikely (ret != ret_ok) ) {
			switch (ret) {
			case ret_eof:
				return ret_eof;

			case ret_eagain:
				req->polling_aim.fd   = req->socket.socket;
				req->polling_aim.mode = poll_mode_write;
				return ret_eagain;

			case ret_error:
				req->keepalive = 0;
				return ret;

			default:
				req->keepalive = 0;
				RET_UNKNOWN(ret);
				return ret_error;
			}
		}
		cherokee_request_tx_add (req, re);

		/* NOTE: req->mmaped is a ptr. to void
		 * so we have to apply ptr. math carefully.
		 */
		req->mmaped      = (void *) ( ((char *)req->mmaped) + re );
		req->mmaped_len -= (off_t) re;

		return (req->mmaped_len > 0) ? ret_eagain : ret_ok;
	}

	/* 2.- There are header and mmaped content to send
	 */
	bufs[0].iov_base = req->buffer.buf;
	bufs[0].iov_len  = req->buffer.len;
	if (likely (req->mmaped_len > 0)) {
		bufs[1].iov_base = req->mmaped;
		bufs[1].iov_len  = req->mmaped_len;
		nvec = 2;
	}
	ret = cherokee_socket_writev (&req->socket, bufs, nvec, &re);
	if (unlikely (ret != ret_ok)) {
		switch (ret) {

		case ret_eof:
			return ret_eof;

		case ret_eagain:
			req->polling_aim.fd   = req->socket.socket;
			req->polling_aim.mode = poll_mode_write;
			return ret_eagain;

		case ret_error:
			req->keepalive = 0;
			return ret_error;

		default:
			RET_UNKNOWN(ret);
			return ret_error;
		}
	}
	/* Add to the connection traffic counter
	 */
	cherokee_request_tx_add (req, re);

	/* writev() may not have sent all headers data.
	 */
	if (unlikely (re < (size_t) req->buffer.len)) {
		/* Partial header data sent.
		 */
		cherokee_buffer_move_to_begin (&req->buffer, re);
		return ret_eagain;
	}

	/* OK, all headers have been sent,
	 * subtract from amount sent and clean header buffer.
	 */
	re -= (size_t) req->buffer.len;
	cherokee_buffer_clean (&req->buffer);

	/* NOTE: req->mmaped is a ptr. to void
	 * so we have to apply ptr. math carefully.
	 */
	req->mmaped      = (void *) ( ((char *)req->mmaped) + re );
	req->mmaped_len -= (off_t) re;

	return (req->mmaped_len > 0) ? ret_eagain : ret_ok;
}


void
cherokee_request_rx_add (cherokee_request_t *req, ssize_t rx)
{
	if (likely (rx > 0)) {
		req->rx += rx;
		req->rx_partial += rx;
	}
}


void
cherokee_request_tx_add (cherokee_request_t *req, ssize_t tx)
{
	cuint_t to_sleep;

	/* Count it
	 */
	req->tx += tx;
	req->tx_partial += tx;

	/* Traffic shaping
	 */
	if (req->limit_rate) {
		to_sleep = (tx * 1000) / req->limit_bps;

		/* It's meant to sleep for a while */
		req->limit_blocked_until = cherokee_bogonow_msec + to_sleep;
	}
}


ret_t
cherokee_request_recv (cherokee_request_t *req,
		       cherokee_buffer_t  *buffer,
		       off_t               to_read,
		       off_t              *len)
{
	ret_t  ret;
	size_t cnt_read = 0;

	ret = cherokee_socket_bufread (&req->socket, buffer, to_read, &cnt_read);
	switch (ret) {
	case ret_ok:
		cherokee_request_rx_add (req, cnt_read);
		*len = cnt_read;
		return ret_ok;

	case ret_eof:
	case ret_error:
		return ret;

	case ret_eagain:
		if (cnt_read > 0) {
			cherokee_request_rx_add (req, cnt_read);
			*len = cnt_read;
			return ret_ok;
		}

		req->polling_aim.fd   = req->socket.socket;
		req->polling_aim.mode = poll_mode_read;

		return ret_eagain;

	default:
		RET_UNKNOWN(ret);
		return ret_error;
	}
	/* NOTREACHED */
}


ret_t
cherokee_request_reading_check (cherokee_request_t *req)
{
	/* Check for too long headers
	 */
	if (req->header_buffer_in.len > MAX_HEADER_LEN) {
		req->error_code = http_request_entity_too_large;
		return ret_error;
	}

	return ret_ok;
}


ret_t
cherokee_request_set_cork (cherokee_request_t *req,
			   cherokee_boolean_t  enable)
{
	ret_t ret;

	ret = cherokee_socket_set_cork (&req->socket, enable);
	if (unlikely (ret != ret_ok)) {
		return ret;
	}

	if (enable) {
		BIT_SET (req->options, conn_op_tcp_cork);
	} else {
		BIT_UNSET (req->options, conn_op_tcp_cork);
	}

	return ret_ok;
}


ret_t
cherokee_request_send_header (cherokee_request_t *req)
{
	ret_t  ret;
	size_t sent = 0;

	if (cherokee_buffer_is_empty (&req->buffer))
		return ret_ok;

	/* Send the buffer content
	 */
	ret = cherokee_socket_bufwrite (&req->socket, &req->buffer, &sent);
	switch (ret) {
	case ret_ok:
		break;

	case ret_eagain:
		req->polling_aim.fd   = req->socket.socket;
		req->polling_aim.mode = poll_mode_write;
		return ret_eagain;

	default:
		return ret;
	}

	/* Add to the connection traffic counter
	 */
	cherokee_request_tx_add (req, sent);

	/* Drop out the sent data
	 */
	if (sent == req->buffer.len) {
		cherokee_buffer_clean (&req->buffer);
		return ret_ok;
	}

	/* There is still some data waiting to be sent
	 */
	cherokee_buffer_move_to_begin (&req->buffer, sent);
	return ret_eagain;
}


ret_t
cherokee_request_send (cherokee_request_t *req)
{
	ret_t  ret;
	size_t to_send;
	size_t sent     = 0;

	/* EAGAIN handle of chunked responses over SSL. Due a libssl
	 * restriction, the cryptor will NOT report ret_ok until the
	 * full content is sent (the buffer can not be touched while
	 * it is being sent). We generate the plain chunk in-memory in
	 * order to let this mechanism handle the I/O events.
	 */
	if ((req->chunked_encoding) &&
	    (req->socket.is_tls == TLS))
	{
		if (! (req->options & conn_op_chunked_formatted)) {
			cherokee_buffer_prepend_buf (&req->buffer, &req->chunked_len);
			cherokee_buffer_add_str     (&req->buffer, CRLF);

			if (req->chunked_last_package) {
				cherokee_buffer_add_str (&req->buffer, "0" CRLF CRLF);
			}

			BIT_SET (req->options, conn_op_chunked_formatted);
		}
	}

	/* Use writev() to send the chunk-begin mark
	 */
	else if (req->chunked_encoding)
	{
		struct iovec tmp[3];

		/* Build the data vector */
		tmp[0].iov_base = req->chunked_len.buf;
		tmp[0].iov_len  = req->chunked_len.len;
		tmp[1].iov_base = req->buffer.buf;
		tmp[1].iov_len  = req->buffer.len;

		/* Traffic shaping */
		if ((req->limit_bps > 0) &&
		    (req->limit_bps < req->buffer.len))
		{
			tmp[1].iov_len = req->limit_bps;
		}

		/* Trailer */
		if (req->chunked_last_package) {
			tmp[2].iov_base = CRLF "0" CRLF CRLF;
			tmp[2].iov_len  = 7;
		} else {
			tmp[2].iov_base = CRLF;
			tmp[2].iov_len  = 2;
		}

		cherokee_iovec_skip_sent (tmp, 3,
					  req->chunks, &req->chunksn,
					  req->chunked_sent);

		ret = cherokee_socket_writev (&req->socket, req->chunks, req->chunksn, &sent);
		switch (ret) {
		case ret_ok:
			break;

		case ret_eof:
			return ret_eof;

		case ret_eagain:
			req->polling_aim.fd   = req->socket.socket;
			req->polling_aim.mode = poll_mode_write;
			return ret_eagain;

		case ret_error:
			req->keepalive = 0;
			return ret_error;

		default:
			RET_UNKNOWN(ret);
			return ret_error;
		}

		req->chunked_sent += sent;

		/* Clean the information buffer only when everything
		 * has been sent.
		 */
		ret = cherokee_iovec_was_sent (tmp, 3, req->chunked_sent);
		if (ret == ret_ok) {
			cherokee_buffer_clean (&req->chunked_len);
			cherokee_buffer_clean (&req->buffer);
			ret = ret_ok;
		} else {
			ret = ret_eagain;
		}

		goto out;
	}

	/* Send the buffer content
	 */
	to_send = req->buffer.len;
	if ((req->limit_bps > 0) &&
	    (req->limit_bps < to_send))
	{
		to_send = req->limit_bps;
	}

	ret = cherokee_socket_write (&req->socket, req->buffer.buf, to_send, &sent);
	switch (ret) {
	case ret_ok:
		break;

	case ret_eagain:
		req->polling_aim.fd   = req->socket.socket;
		req->polling_aim.mode = poll_mode_write;
		return ret_eagain;

	default:
		return ret;
	}

	/* Drop out the sent info
	 */
	if (sent == req->buffer.len) {
		BIT_UNSET (req->options, conn_op_chunked_formatted);
		cherokee_buffer_clean (&req->buffer);
		ret = ret_ok;
	} else if (sent != 0) {
		cherokee_buffer_move_to_begin (&req->buffer, sent);
		ret = ret_eagain;
	}

out:
	/* Add to the connection traffic counter
	 */
	cherokee_request_tx_add (req, sent);

	/* If this connection has a handler without Content-Length support
	 * it has to count the bytes sent
	 */
	if ((req->handler) &&
	    (! HANDLER_SUPPORTS (req->handler, hsupport_length))) {
		req->range_end += sent;
	}

	return ret;
}

int
cherokee_request_should_include_length (cherokee_request_t *req)
{
	if (req->encoder_new_func) {
		return false;
	}
	if (req->encoder) {
		return false;
	}

	return true;
}


ret_t
cherokee_request_instance_encoder (cherokee_request_t *req)
{
	ret_t  ret;
	char  *name = NULL;

	/* Ensure that the content can be encoded
	 */
	if (req->options & conn_op_cant_encoder)
		return ret_deny;

	if ((! http_code_with_body (req->error_code)) ||
	    (! http_method_with_body (req->header.method)))
		return ret_deny;

	if (! http_type_200 (req->error_code))
		return ret_deny;

	/* Instance and initialize the encoder
	 */
	ret = req->encoder_new_func ((void **)&req->encoder, req->encoder_props);
	if (unlikely (ret != ret_ok)) {
		ret = ret_error;
		goto error;
	}

	ret = cherokee_encoder_init (req->encoder, req);
	switch (ret) {
	case ret_ok:
		break;
	case ret_deny:
		/* Refuses to work. Eg: GZip for IE */
		goto error;
	default:
		ret = ret_error;
		goto error;
	}

	/* Update Front-Line cache
	 */
	if (req->flcache.mode == flcache_mode_in) {
		ret = cherokee_module_get_name (MODULE(req->encoder), &name);
		if (likely ((ret == ret_ok) || (name != NULL))) {
			cherokee_buffer_add (&req->flcache.avl_node_ref->content_encoding, name, strlen(name));
			TRACE (ENTRIES, "New entry encoded entry '%s' - %s\n", req->request.buf, name);
		}
	}

	cherokee_buffer_clean (&req->encoder_buffer);
	return ret_ok;

error:
	if (req->encoder) {
		cherokee_encoder_free (req->encoder);
		req->encoder = NULL;
	}
	return ret;
}


ret_t
cherokee_request_shutdown_wr (cherokee_request_t *req)
{
	ret_t ret;

	/* Turn TCP-cork off
	 */
	if (req->options & conn_op_tcp_cork) {
		cherokee_socket_flush (&req->socket);
		cherokee_request_set_cork (req, false);
	}

	/* At this point, we don't want to follow the TLS protocol
	 * any longer.
	 */
	req->socket.is_tls = non_TLS;

	/* Shut down the socket for write, which will send a FIN to
	 * the peer. If shutdown fails then the socket is unusable.
         */
	ret = cherokee_socket_shutdown (&req->socket, SHUT_WR);
	if (unlikely (ret != ret_ok)) {
		TRACE (ENTRIES, "Could not shutdown (%d, SHUT_WR)\n", req->socket.socket);
		return ret_error;
	}

	TRACE (ENTRIES, "Shutdown (%d, SHUT_WR): successful\n", req->socket.socket);
	return ret_ok;
}


ret_t
cherokee_request_linger_read (cherokee_request_t *req)
{
	ret_t              ret;
	size_t             cnt_read = 0;
	int                retries  = 2;
	cherokee_thread_t *thread   = REQ_THREAD(req);
	cherokee_buffer_t *tmp1     = THREAD_TMP_BUF1(thread);

	TRACE(ENTRIES",linger", "Linger read, socket %s closed\n", req->socket.closed ? "is" : "isn't");

	while (true) {
		/* Read from the socket to nowhere
		 */
		ret = cherokee_socket_read (&req->socket, tmp1->buf, tmp1->size - 1, &cnt_read);

		if (cnt_read > 0) {
			cherokee_buffer_clean (tmp1);
		}

		switch (ret) {
		case ret_eof:
			TRACE (ENTRIES",linger", "%s\n", "EOF");
			return ret_eof;
		case ret_error:
			TRACE (ENTRIES",linger", "%s\n", "error");
			return ret_error;
		case ret_eagain:
			TRACE (ENTRIES",linger", "read %u, eagain\n", cnt_read);

			req->polling_aim.fd   = req->socket.socket;
			req->polling_aim.mode = poll_mode_read;
			return ret_eagain;
		case ret_ok:
			TRACE (ENTRIES",linger", "%u bytes tossed away\n", cnt_read);

			retries--;
			if ((cnt_read >= tmp1->size - 2) && (retries > 0))
				continue;

			return ret_eof;
		default:
			RET_UNKNOWN(ret);
			return ret_error;
		}
		/* NOTREACHED */
	}
}


ret_t
cherokee_request_step (cherokee_request_t *req)
{
	ret_t ret;
	ret_t step_ret = ret_ok;

	/* Need to 'read' from handler ?
	 */
	if (req->buffer.len > 0) {
		return ret_ok;

	} else if (unlikely (req->options & conn_op_got_eof)) {
		return ret_eof;
	}

	/* Front-line cache: Serve content
	 */
	if (req->flcache.mode == flcache_mode_out) {
		ret = cherokee_flcache_conn_send_body (&req->flcache, req);
		switch (ret) {
		case ret_ok:
			return ret_ok;

		case ret_eof:
		case ret_eof_have_data:
			BIT_SET (req->options, conn_op_got_eof);
			return ret;

		case ret_error:
		case ret_eagain:
		case ret_ok_and_sent:
			return ret;

		default:
			RET_UNKNOWN(ret);
			return ret;
		}
	}

	/* Do a step in the handler
	 */
	return_if_fail (req->handler != NULL, ret_error);

	step_ret = cherokee_handler_step (req->handler, &req->buffer);
	switch (step_ret) {
	case ret_ok:
		break;

	case ret_eof:
	case ret_eof_have_data:
		BIT_SET (req->options, conn_op_got_eof);
		break;

	case ret_error:
	case ret_eagain:
	case ret_ok_and_sent:
		return step_ret;

	default:
		RET_UNKNOWN(step_ret);
		return step_ret;
	}

	/* Return now if no encoding is needed.
	 */
	if (req->encoder != NULL) {
		/* Encode handler output.
		 */
		switch (step_ret) {
		case ret_eof:
		case ret_eof_have_data:
			ret = cherokee_encoder_flush (req->encoder, &req->buffer, &req->encoder_buffer);
			step_ret = (req->encoder_buffer.len == 0) ? ret_eof : ret_eof_have_data;

			TRACE (ENTRIES",encoder", "Flush: in=(len %d, size %d) >> out=(len %d, size %d)\n",
			       req->buffer.len, req->buffer.size,
			       req->encoder_buffer.len, req->encoder_buffer.size);
			break;
		default:
			ret = cherokee_encoder_encode (req->encoder, &req->buffer, &req->encoder_buffer);

			TRACE (ENTRIES",encoder", "Encode: in=(len %d, size %d) >> out=(len %d, size %d)\n",
			       req->buffer.len, req->buffer.size,
			       req->encoder_buffer.len, req->encoder_buffer.size);
			break;
		}
		if (ret < ret_ok)
			return ret;

		/* Swap buffers
		 */
		cherokee_buffer_swap_buffers (&req->buffer, &req->encoder_buffer);
		cherokee_buffer_clean (&req->encoder_buffer);
	}

	/* Front-line cache: Store content
	 */
	if ((req->flcache.mode == flcache_mode_in) &&
	    (! cherokee_buffer_is_empty (&req->buffer)))
	{
		cherokee_flcache_conn_write_body (&req->flcache, req);
	}

	/* Chunked encoding header
	 */
	if (req->chunked_encoding) {
		/* On EOF: Turn off chunked and send the trailer
		 */
		if (step_ret == ret_eof) {
			req->chunked_encoding = false;
			cherokee_buffer_add_str (&req->buffer, "0" CRLF CRLF);

			return ret_eof_have_data;
		}

		/* On last package: point cherokee_request_send
		 * about it.  It will add the trailer.
		 */
		if (step_ret == ret_eof_have_data)
		{
			req->chunked_last_package = true;
		}

		/* Build the Chunked package header:
		 * len(str(hex(4294967295))+"\r\n\0") = 13
		 */
		cherokee_buffer_clean       (&req->chunked_len);
		cherokee_buffer_ensure_size (&req->chunked_len, 13);
		cherokee_buffer_add_ulong16 (&req->chunked_len, req->buffer.len);
		cherokee_buffer_add_str     (&req->chunked_len, CRLF);

		req->chunked_sent = 0;
	}

	return step_ret;
}


static ret_t
get_host (cherokee_request_t *req,
	  char               *ptr,
	  int                 size)
{
	ret_t  ret;
	char  *i;
	char  *colon = NULL;
	char  *end   = ptr + size;

	/* Sanity check
	 */
	if (unlikely (size <= 0)) {
		return ret_error;
	}

	/* Look up for a colon character
	 */
	for (i=end-1; i>=ptr; i--) {
		if (*i == ':') {
			colon = i;
			break;
		}
	}

	if (unlikely ((colon != NULL) &&
		      ((colon == ptr) || (end - colon <= 0))))
	{
		return ret_error;
	}

	/* Copy the host and port
	 */
	if (colon) {
		ret = cherokee_buffer_add (&req->host_port, colon+1, end-(colon+1));
		if (unlikely (ret != ret_ok))
			return ret;

		ret = cherokee_buffer_add (&req->host, ptr, colon - ptr);
		if (unlikely (ret != ret_ok))
			return ret;
	} else {
		ret = cherokee_buffer_add (&req->host, ptr, size);
		if (unlikely (ret != ret_ok))
			return ret;
	}

	/* Security check: Hostname shouldn't start with a dot
	 */
	if ((req->host.len >= 1) && (*req->host.buf == '.')) {
		return ret_error;
	}

	/* RFC-1034: Dot ending host names
	 */
	while ((cherokee_buffer_end_char (&req->host) == '.') &&
	       (! cherokee_buffer_is_empty (&req->host)))
	{
		cherokee_buffer_drop_ending (&req->host, 1);
	}

	return ret_ok;
}


static ret_t
get_encoding (cherokee_request_t *req,
	      char               *ptr,
	      cherokee_avl_t     *encoders_accepted)
{
	ret_t                     ret;
	char                      tmp;
	char                     *i1;
	char                     *i2;
	char                     *end;
	cherokee_encoder_props_t *props = NULL;

	/* ptr = Header at the "Accept-Encoding" position
	 */
	end = strchr (ptr, CHR_CR);
	if (end == NULL) {
		return ret_error;
	}
	*end = '\0'; /* (1) */

	i1 = ptr;
	do {
		while (*i1 == ' ')
			i1++;

		i2 = strchr (i1, ',');
		if (!i2) {
			i2 = strchr (i1, ';');
			if (!i2) {
				i2 = end;
			}
		}

		/* Get the encoder configuration entry
		 */
		tmp = *i2;    /* (2) */
		*i2 = '\0';
		ret = cherokee_avl_get_ptr (encoders_accepted, i1, (void **)&props);
		*i2 = tmp;    /* (2') */

		if ((ret == ret_ok) && (props != NULL)) {
			if (props->perms == cherokee_encoder_allow) {
				/* Use encoder
				 */
				req->encoder_new_func = props->instance_func;
				req->encoder_props    = props;
				break;

			} else if (props->perms == cherokee_encoder_forbid) {
				/* Explicitly forbidden
				 */
				req->encoder_new_func = NULL;
				req->encoder_props    = NULL;
				break;

			} else if (props->perms == cherokee_encoder_unset) {
				/* Let's it be
				 */
				break;
			}

			SHOULDNT_HAPPEN;
			return ret_error;
		}

		/* Next iteration
		 */
		if (i2 < end) {
			i1 = i2+1;
		}

	} while (i2 < end);

	*end = CHR_CR; /* (1') */
	return ret_ok;
}


static ret_t
get_authorization (cherokee_request_t   *req,
		   cherokee_http_auth_t  type,
		   cherokee_validator_t *validator,
		   char                 *ptr,
	           int                   ptr_len)
{
	ret_t    ret;
	char    *end, *end2;
	cuint_t  pre_len = 0;

	/* It checks that the authentication send by the client is compliant
	 * with the configuration of the server.  It does not check if the
	 * kind of validator is suitable in this case.
	 */
	if (equal_str (ptr, "Basic ")) {

		/* Check the authentication type
 		 */
		if (!(type & http_auth_basic))
			return ret_error;

		req->req_auth_type = http_auth_basic;
		pre_len = 6;

	} else if (equal_str (ptr, "Digest ")) {

		/* Check the authentication type
		 */
		if (!(type & http_auth_digest))
			return ret_error;

		req->req_auth_type = http_auth_digest;
		pre_len = 7;
	}

	/* Skip end of line
	 */
	end  = strchr (ptr, CHR_CR);
	end2 = strchr (ptr, CHR_LF);

	end = cherokee_min_str (end, end2);
	if (end == NULL)
		return ret_error;

	ptr_len -= (ptr + ptr_len) - end;

	/* Skip "Basic " or "Digest "
	 */
	ptr += pre_len;
	ptr_len -= pre_len;

	/* Parse the request
	 */
	switch (req->req_auth_type) {
	case http_auth_basic:
		ret = cherokee_validator_parse_basic (validator, ptr, ptr_len);
		if (ret != ret_ok) return ret;
		break;

	case http_auth_digest:
		ret = cherokee_validator_parse_digest (validator, ptr, ptr_len);
		if (ret != ret_ok) return ret;

		/* Check nonce value
		 */
		if (cherokee_buffer_is_empty(&validator->nonce))
			return ret_error;

		break;

	default:
		LOG_ERROR_S (CHEROKEE_ERROR_CONNECTION_AUTH);
		return ret_error;
	}

	return ret_ok;
}


int
cherokee_request_is_userdir (cherokee_request_t *req)
{
	return ((req->request.len > 3) &&
		(req->request.buf[1] == '~'));
}


ret_t
cherokee_request_set_custom_droot (cherokee_request_t      *req,
				   cherokee_config_entry_t *entry)
{
	/* Shortcut
	 */
	if ((entry->document_root == NULL) ||
	    (entry->document_root->len <= 0))
		return ret_ok;

	/* Have a special DocumentRoot
	 */
	BIT_SET (req->options, conn_op_document_root);

	cherokee_buffer_clean (&req->local_directory);
	cherokee_buffer_add_buffer (&req->local_directory, entry->document_root);

	/* It has to drop the webdir from the request:
	 *
	 * Directory /thing {
	 *    DocumentRoot /usr/share/this/rocks
	 * }
	 *
	 * on petition: http://server/thing/cherokee
	 * should read: /usr/share/this/rocks/cherokee
	 */
	if (cherokee_buffer_is_empty (&req->request_original)) {
		cherokee_buffer_add_buffer (&req->request_original, &req->request);
		if (! cherokee_buffer_is_empty (&req->query_string)) {
			cherokee_buffer_add_buffer (&req->query_string_original, &req->query_string);
		}
	}

	if (req->web_directory.len > 1) {
		cherokee_buffer_move_to_begin (&req->request, req->web_directory.len);
	}

	if ((req->request.len >= 2) && (strncmp(req->request.buf, "//", 2) == 0)) {
		cherokee_buffer_move_to_begin (&req->request, 1);
	}

	TRACE(ENTRIES, "Set Custom Local Directory: '%s'\n", req->local_directory.buf);
	return ret_ok;
}


ret_t
cherokee_request_build_local_directory (cherokee_request_t        *req,
					cherokee_virtual_server_t *vsrv)
{
	ret_t ret;

	/* Enhanced Virtual-Hosting
	 */
	if (vsrv->evhost != NULL) {
		TRACE(ENTRIES, "About to evaluate EVHost. Now Document root is: '%s'\n",
		      req->local_directory.buf ? req->local_directory.buf : "");

		ret = EVHOST(vsrv->evhost)->func_document_root (vsrv->evhost, req);
		if (ret == ret_ok)
			goto ok;

		/* Fall to use the default document root
		 */
		cherokee_buffer_clean (&req->local_directory);
	}

	/* Regular request
	 */
	ret = cherokee_buffer_add_buffer (&req->local_directory, &vsrv->root);
	if (unlikely (ret != ret_ok)) {
		goto error;
	}

ok:
	TRACE(ENTRIES, "Set Local Directory: '%s'\n", req->local_directory.buf);
	return ret_ok;

error:
	LOG_ERROR_S (CHEROKEE_ERROR_CONNECTION_LOCAL_DIR);
	return ret_error;
}


ret_t
cherokee_request_build_local_directory_userdir (cherokee_request_t        *req,
						cherokee_virtual_server_t *vsrv)
{
	ret_t         ret;
	struct passwd pwd;
	char          tmp[1024];

	/* Default: it is inside the UserDir in home
	 */
	ret = cherokee_getpwnam (req->userdir.buf, &pwd, tmp, sizeof(tmp));
	if ((ret != ret_ok) || (pwd.pw_dir == NULL)) {
		req->error_code = http_not_found;
		return ret_error;
	}

	/* Build the local_directory:
	 */
	cherokee_buffer_add (&req->local_directory, pwd.pw_dir, strlen(pwd.pw_dir));
	cherokee_buffer_add_char   (&req->local_directory, '/');
	cherokee_buffer_add_buffer (&req->local_directory, &vsrv->userdir);

	return ret_ok;
}


static ret_t
get_range (cherokee_request_t *req, char *ptr, int ptr_len)
{
	cuint_t num_len = 0;
	CHEROKEE_TEMP(tmp, ptr_len+1);

	/* Read the start position
	 */
	while ((ptr[num_len] != '-') &&
	       (ptr[num_len] != '\0') &&
	       (num_len < tmp_size-1)) {
		tmp[num_len] = ptr[num_len];
		num_len++;
	}
	tmp[num_len] = '\0';
	if (num_len != 0) {
		req->range_start = strtoll (tmp, (char **)NULL, 10);
		if (req->range_start < 0) {
			return ret_error;
		}
	}

	/* Advance the pointer
	 */
	ptr += num_len;
	if (*ptr != '-') {
		return ret_error;
	}
	ptr++;

	/* Maybe there're an ending position
	 */
	if ((*ptr != '\0') && (*ptr != CHR_CR) && (*ptr != CHR_LF)) {
		num_len = 0;

		/* Read the end
		 */
		while ((ptr[num_len] >= '0') && (ptr[num_len] <= '9') && (num_len < tmp_size-1)) {
			tmp[num_len] = ptr[num_len];
			num_len++;
		}
		tmp[num_len] = '\0';
		req->range_end = strtoll (tmp, (char **)NULL, 10);
		if (req->range_end < 0){
			return ret_error;
		}
	}

	/* Sanity check: switched range
	 */
	if ((req->range_start != -1) && (req->range_end != -1)) {
		if (req->range_start > req->range_end) {
			req->error_code = http_range_not_satisfiable;
			return ret_error;
		}
	}

	return ret_ok;
}


static ret_t
parse_userdir (cherokee_request_t *req)
{
	char *begin;
	char *end_username;

	/* Find user name ending:
	 */
	begin = &req->request.buf[2];

	end_username = strchr (begin, '/');
	if (end_username == NULL) {

		/* It has to be redirected
		 *
		 * from http://www.alobbs.com/~alo
		 * to   http://www.alobbs.com/~alo/
		 */
		cherokee_buffer_add_buffer (&req->redirect, &req->request);
		cherokee_buffer_add_char   (&req->redirect, '/');

		req->error_code = http_moved_permanently;
		return ret_error;
	}

	/* Sanity check:
	 * The username has to be at least a char long
	 */
	if ((end_username - begin) <= 0) {
		req->error_code = http_bad_request;
		return ret_error;
	}

	/* Get the user home directory
	 */
	cherokee_buffer_add (&req->userdir, begin, end_username - begin);

	/* Drop username from the request
	 */
	cherokee_buffer_move_to_begin (&req->request, (end_username - req->request.buf));

	return ret_ok;
}


ret_t
cherokee_request_get_request (cherokee_request_t *req)
{
	ret_t               ret;
	char               *host;
	cuint_t             host_len;
	cherokee_http_t     error_code = http_bad_request;
	cherokee_boolean_t  read_post  = false;

	/* Header parsing
	 */
	ret = cherokee_header_parse (&req->header, &req->header_buffer_in, &error_code);
	if (unlikely (ret < ret_ok)) {
		goto error;
	}

	/* Check is request body is present
	 */
	if (http_method_with_input (req->header.method)) {
		read_post = true;
	}
	else if (http_method_with_optional_input (req->header.method)) {
		ret = cherokee_header_has_known (&req->header, header_content_length);
		if (ret == ret_ok) {
			read_post = true;
		} else {
			ret = cherokee_header_has_known (&req->header, header_transfer_encoding);
			if (ret == ret_ok) {
				read_post = true;
			}
		}
	}

	/* Read POST
	 */
	if (read_post)
	{
		ret = cherokee_post_read_header (&req->post);
		if (unlikely (ret != ret_ok)) {
			return ret;
		}
	}

	/* Copy the request and query string
	 */
	ret = cherokee_header_copy_request (&req->header, &req->request);
	if (unlikely (ret < ret_ok))
		goto error;

	ret = cherokee_header_copy_query_string (&req->header, &req->query_string);
	if (unlikely (ret < ret_ok))
		goto error;


	/* "OPTIONS *" special case
	 */
	if (unlikely ((req->header.method == http_options) &&
		      (cherokee_buffer_cmp_str (&req->request, "*") == 0)))
	{
		cherokee_buffer_add_buffer (&req->request_original, &req->request);

		cherokee_buffer_clean   (&req->request);
		cherokee_buffer_add_str (&req->request, "/");
	}

	/* Look for starting '/' in the request
	 */
	if (unlikely (req->request.buf[0] != '/')) {
		goto error;
	}

#ifdef _WIN32
	/* Prevent back-slashes in the request on Windows
	 */
	TRACE (ENTRIES, "Win32 req before: %s\n", req->request.buf);
	cherokee_buffer_swap_chars (&req->request, '\\', '/');
	TRACE (ENTRIES, "Win32 req after: %s\n", req->request.buf);
#endif

	/* Short the path. It transforms the request:
	 * /dir1/dir2/../file in /dir1/file
	 */
	cherokee_path_short (&req->request);

	/* Look for "//"
	 */
	cherokee_buffer_remove_dups (&req->request, '/');

	/* Read the Host header
	 */
	ret = cherokee_header_get_known (&req->header, header_host, &host, &host_len);
	switch (ret) {
	case ret_error:
	case ret_not_found:
		if (req->header.version == http_version_11) {
			/* It is needed in HTTP/1.1
			 */
			TRACE(ENTRIES, "conn %p, HTTP/1.1 with no Host header entry\n", req);
			goto error;
		}
		break;

	case ret_ok:
		ret = get_host (req, host, host_len);
		if (unlikely(ret < ret_ok)) goto error;

		/* Set the virtual server reference
		 */
		ret = cherokee_server_get_vserver (REQ_SRV(req), &req->host, req,
						   (cherokee_virtual_server_t **)&req->vserver);
		if (unlikely (ret != ret_ok)) {
			LOG_ERROR (CHEROKEE_ERROR_CONNECTION_GET_VSERVER, req->host.buf);
			return ret_error;
		}
		break;

	default:
		RET_UNKNOWN(ret);
		return ret_error;
	}

	/* Userdir requests
	 */
	if ((!cherokee_buffer_is_empty (&REQ_VSRV(req)->userdir)) &&
	    (cherokee_request_is_userdir (req)))
	{
		ret = parse_userdir (req);
		if (ret != ret_ok) return ret;
	}

	/* Check Upload limit
	 */
	if ((REQ_VSRV(req)->post_max_len > 0) &&
	    (req->post.len > REQ_VSRV(req)->post_max_len))
	{
		req->error_code = http_request_entity_too_large;
		return ret_error;
	}

	req->error_code = http_ok;
	return ret_ok;

error:
	req->error_code = error_code;

	/* Since the request could not be parsed, the connection is
	 * about to be closed. Log the error now before it's too late.
	 */
	if (REQ_VSRV(req)->logger) {
		cherokee_logger_write_access (REQ_VSRV(req)->logger, req);
	}
	return ret_error;
}


ret_t
cherokee_request_check_authentication (cherokee_request_t *req, cherokee_config_entry_t *config_entry)
{
	ret_t    ret;
	char    *ptr;
	cuint_t  len;

	/* Return, there is nothing to do here
	 */
	if (config_entry->validator_new_func == NULL)
		return ret_ok;

	/* Look for authentication in the headers:
	 * It's done on demand because the directory maybe don't have protection
	 */
	ret = cherokee_header_get_known (&req->header, header_authorization, &ptr, &len);
	if (ret != ret_ok) {
		goto unauthorized;
	}

	/* Create the validator object
	 */
	ret = config_entry->validator_new_func ((void **) &req->validator,
						config_entry->validator_properties);
	if (ret != ret_ok) {
		goto error;
	}

	/* Read the header information
	 */
	ret = get_authorization (req, config_entry->authentication, req->validator, ptr, len);
	if (ret != ret_ok) {
		goto unauthorized;
	}

	/* Check if the user is in the list
	 */
	if (config_entry->users != NULL) {
		void *foo;

		if (cherokee_buffer_is_empty (&req->validator->user)) {
			goto unauthorized;
		}

		ret = cherokee_avl_get (config_entry->users, &req->validator->user, &foo);
		if (ret != ret_ok) {
			goto unauthorized;
		}
	}

	/* Check if the validator is suitable
	 */
	if ((req->validator->support & req->req_auth_type) == 0) {
		goto error;
	}

	/* Check the login/password
	 */
	ret = cherokee_validator_check (req->validator, req);

	if (ret != ret_ok) {
		goto unauthorized;
	}

	return ret_ok;

unauthorized:
	req->keepalive = 0;
	req->error_code = http_unauthorized;
	return ret_error;

error:
	req->keepalive = 0;
	req->error_code = http_internal_error;
	return ret_error;
}


ret_t
cherokee_request_check_ip_validation (cherokee_request_t *req, cherokee_config_entry_t *config_entry)
{
	ret_t ret;

	if (config_entry->access == NULL) {
		return ret_ok;
	}

	ret = cherokee_access_ip_match (config_entry->access, &req->socket);
	if (ret == ret_ok) {
		return ret_ok;
	}

	req->error_code = http_access_denied;
	return ret_error;
}


ret_t
cherokee_request_check_only_secure (cherokee_request_t *req, cherokee_config_entry_t *config_entry)
{
	if (config_entry->only_secure == false) {
		/* No Only-Secure connection..
		 */
		return ret_ok;
	}

	if (req->socket.is_tls == TLS) {
		/* It is secure
		 */
		return ret_ok;
	}

	req->error_code = http_upgrade_required;
	req->upgrade    = http_upgrade_tls10;
	return ret_error;
}


ret_t
cherokee_request_check_http_method (cherokee_request_t *req, cherokee_config_entry_t *config_entry)
{
	if (config_entry->handler_methods & req->header.method)
		return ret_ok;

	if (config_entry->handler_methods == http_options)
		return ret_ok;

	/* Set the error
	 */
	req->error_code = http_method_not_allowed;

	/* If the HTTP method was not detected, set a safe one
	 */
	if (req->header.method == http_unknown) {
		req->header.method = http_get;
	}

	return ret_error;
}


ret_t
cherokee_request_create_handler (cherokee_request_t *req, cherokee_config_entry_t *config_entry)
{
	ret_t ret;

	return_if_fail (config_entry->handler_new_func != NULL, ret_error);

	/* Create and assign a handler object
	 */
	ret = (config_entry->handler_new_func) ((void **)&req->handler, req, config_entry->handler_properties);
	switch (ret) {
	case ret_ok:
	case ret_eof:
	case ret_eagain:
		/* Trace the name of the new handler
		 */
#ifdef TRACE_ENABLED
		if ((req->handler != NULL) &&
		    (cherokee_trace_is_tracing()))
		{
			char *name = NULL;

			ret = cherokee_module_get_name (MODULE (req->handler), &name);
			if ((ret == ret_ok) && (name != NULL)) {
				TRACE(ENTRIES",handler", "Instanced handler: '%s'\n", name);
			}
		}
#endif
		return ret;
	default:
		if ((req->handler == NULL) &&
		    (req->error_code == http_ok))
			req->error_code = http_internal_error;
		return ret_error;
	}

	return ret_ok;
}


void
cherokee_request_set_keepalive (cherokee_request_t *req)
{
	ret_t              ret;
	char              *ptr;
	cuint_t            ptr_len;
	cherokee_server_t *srv    = REQ_SRV(req);
	cherokee_thread_t *thread = REQ_THREAD(req);

	/* Check whether server allows keep-alive
	 */
	if (srv->keepalive == false)
		goto denied;

	/* Check the Max concurrent Keep-Alive limit on this thread
	 */
	if (thread->conns_num >= thread->conns_keepalive_max)
		goto denied;

	/* Does the virtual server support Keep-Alive?
	 */
	if (REQ_VSRV(req)->keepalive == false)
		goto denied;

	/* Set Keep-alive according with the 'Connection' header
	 * HTTP 1.1 uses Keep-Alive by default: rfc2616 sec8.1.2
	 */
	ret = cherokee_header_get_known (&req->header, header_connection, &ptr, &ptr_len);
	if (ret == ret_ok) {
		if (req->header.version == http_version_11) {
			if (strncasestrn (ptr, ptr_len, "close", 5) == NULL)
				goto granted;
			else
				goto denied;

		} else {
			if (strncasestrn (ptr, ptr_len, "keep-alive", 10) != NULL)
				goto granted;
			else
				goto denied;
		}
		return;
	}

	if (req->header.version == http_version_11)
		goto granted;

	if (req->flcache.mode == flcache_mode_out)
		goto granted;

denied:
	TRACE (ENTRIES, "Keep-alive %s\n", "denied");
	req->keepalive = 0;
	return;

granted:
	if (req->keepalive == 0)
		req->keepalive = REQ_SRV(req)->keepalive_max;

	TRACE (ENTRIES, "Keep-alive %d\n", req->keepalive);
}


ret_t
cherokee_request_set_rate (cherokee_request_t      *req,
			   cherokee_config_entry_t *entry)
{
	if (entry->limit_bps <= 0)
		return ret_ok;

	req->limit_rate = true;
	req->limit_bps  = entry->limit_bps;

	return ret_ok;
}


void
cherokee_request_set_chunked_encoding (cherokee_request_t *req)
{
	/* Conditions to activate Chunked-Encodiog:
	 *  - The server configuration allows to use it
	 *  - It's a keep-alive connection
	 *  - It's a HTTP/1.1 connection
	 *  - It's not serving content from the Front-line cache
	 */
	req->chunked_encoding = ((REQ_SRV(req)->chunked_encoding) &&
				  (req->keepalive > 0) &&
				  (req->header.version == http_version_11) &&
				  (req->flcache.mode != flcache_mode_out));
}

ret_t
cherokee_request_parse_range (cherokee_request_t *req)
{
	ret_t    ret;
	char    *ptr;
	cuint_t  ptr_len;

	/* Look for "Range:"
	 */
	if (HANDLER_SUPPORTS (req->handler, hsupport_range)) {
		ret = cherokee_header_get_known (&req->header, header_range, &ptr, &ptr_len);
		if (ret == ret_ok) {
			if (strncmp (ptr, "bytes=", 6) == 0) {
				ret = get_range (req, ptr+6, ptr_len-6);
				if (ret < ret_ok) {
					req->error_code = http_range_not_satisfiable;
					return ret;
				}
			}
		}
	}

	return ret_ok;
}

ret_t
cherokee_request_create_encoder (cherokee_request_t *req,
				 cherokee_avl_t     *encoders_accepted)
{
	ret_t    ret;
	char    *ptr;
	cuint_t  ptr_len;

	/* No encoders are accepted
	 */
	if ((encoders_accepted == NULL) ||
	    (cherokee_avl_is_empty (AVL_GENERIC(encoders_accepted))))
	{
		return ret_ok;
	}

	/* Keepalive (Content-Length) connections cannot use encoders
	 * because the transferred information length would change.
	 */
	if ((req->keepalive) &&
	    (! req->chunked_encoding) &&
	    HANDLER_SUPPORTS (req->handler, hsupport_length))
	{
		return ret_ok;
	}

	/* Process the "Accept-Encoding" header
	 */
	ret = cherokee_header_get_known (&req->header, header_accept_encoding, &ptr, &ptr_len);
	if (ret == ret_ok) {
		ret = get_encoding (req, ptr, encoders_accepted);
		if (ret < ret_ok) {
			return ret;
		}
	}

	return ret_ok;
}


ret_t
cherokee_request_parse_args (cherokee_request_t *req)
{
	ret_t ret;

	/* Parse arguments only once
	 */
	if (req->arguments != NULL) {
		return ret_ok;
	}

	/* Build a new table
	 */
	ret = cherokee_avl_new (&req->arguments);
	if (unlikely(ret != ret_ok)) {
		return ret;
	}

	/* Parse the header
	 */
	ret = cherokee_parse_query_string (&req->query_string, req->arguments);
	if (unlikely(ret != ret_ok)) {
		return ret;
	}

	return ret_ok;
}


ret_t
cherokee_request_open_request (cherokee_request_t *req)
{
	TRACE (ENTRIES, "web_directory='%s' request='%s' local_directory='%s'\n",
	       req->web_directory.buf,
	       req->request.buf,
	       req->local_directory.buf);

	/* Ensure the space for headers and I/O buffer
	 */
	cherokee_buffer_ensure_size (&req->header_buffer_out, 384);
	cherokee_buffer_ensure_size (&req->buffer, DEFAULT_READ_SIZE+1);

	/* Init the connection handler object
	 */
	return cherokee_handler_init (req->handler);
}


ret_t
cherokee_request_log (cherokee_request_t *req)
{
	/* Check whether if needs to log now of not
	 */
	if (req->logger_ref == NULL) {
		return ret_ok;
	}

	/* Log it
	 */
	return cherokee_logger_write_access (req->logger_ref, req);
}


ret_t
cherokee_request_update_vhost_traffic (cherokee_request_t *req)
{
	if (REQ_VSRV(req)->collector == NULL) {
		return ret_ok;
	}

	cherokee_collector_vsrv_count (REQ_VSRV(req)->collector,
				       req->rx_partial,
				       req->tx_partial);

	/* Update the time for the next update
	 */
	req->traffic_next = cherokee_bogonow_now + DEFAULT_TRAFFIC_UPDATE;

	/* Reset partial counters
	 */
	req->rx_partial = 0;
	req->tx_partial = 0;

	return ret_ok;
}


ret_t
cherokee_request_clean_for_respin (cherokee_request_t *req)
{
	TRACE(ENTRIES, "Clean for respin: conn=%p\n", req);

	req->respins += 1;
	if (req->respins > RESPINS_MAX) {
		TRACE(ENTRIES, "Internal redirection limit (%d) exceeded\n", RESPINS_MAX);
		req->error_code = http_internal_error;
		return ret_error;
	}

	if (cherokee_request_use_webdir(req)) {
		cherokee_buffer_prepend_buf (&req->request, &req->web_directory);
		cherokee_buffer_clean (&req->local_directory);
		BIT_UNSET (req->options, conn_op_document_root);
	}

	cherokee_buffer_clean (&req->web_directory);

	TRACE_REQ(req);
	return ret_ok;
}


ret_t
cherokee_request_clean_error_headers (cherokee_request_t *req)
{
	char *begin;
	char *end;

	if (cherokee_buffer_is_empty (&req->header_buffer_out))
		return ret_ok;

	begin = strncasestrn_s (req->header_buffer_out.buf,
				req->header_buffer_out.len,
				"Content-Length: ");
	if (begin != NULL) {
		end = strchr (begin+16, CHR_CR);
		if (end == NULL)
			return ret_error;

		if (end[1] == CHR_LF)
			end++;

		cherokee_buffer_remove_chunk (&req->header_buffer_out,
					      begin - req->header_buffer_out.buf,
					      (end-begin)+1);
	}

	return ret_ok;
}


int
cherokee_request_use_webdir (cherokee_request_t *req)
{
	if (! (req->options & conn_op_document_root))
		return 0;

	if (cherokee_buffer_is_empty (&req->web_directory))
		return 0;

	return 1;
}


#ifdef TRACE_ENABLED
char *
cherokee_request_print (cherokee_request_t *req)
{
	const char        *phase;
	cherokee_buffer_t *buf    = &req->self_trace;

	/* Shortcut: Don't render if not tracing
	 */
	if (! cherokee_trace_is_tracing()) {
		return (char *)"";
	}

	/* Render
	 */
	cherokee_buffer_clean (buf);

	if (req == NULL) {
		cherokee_buffer_add_str (buf, "Connection is NULL\n");
		return buf->buf;
	}

	phase = cherokee_request_get_phase_str (req);
	cherokee_buffer_add_va (buf, "Connection %p info\n", req);

#define print_buf(title,name)						\
	cherokee_buffer_add_va (buf, "\t| %s: '%s' (%d)\n", title,	\
				(name)->buf ? (name)->buf : "",		\
				(name)->len);
#define print_cbuf(title,name)						\
	print_buf(title, &req->name)

#define print_cint(title,name)						\
	cherokee_buffer_add_va (buf, "\t| %s: %d\n", title, req->name);

#define print_str(title,name)						\
	cherokee_buffer_add_va (buf, "\t| %s: %s\n", title, name);

#define print_add(str)	                           			\
	cherokee_buffer_add_str (buf, str)

	print_cbuf ("         Request", request);
	print_cbuf ("Request Original", request_original);
	print_cbuf ("   Web Directory", web_directory);
	print_cbuf (" Local Directory", local_directory);
	print_cbuf ("        Pathinfo", pathinfo);
	print_cbuf ("        User Dir", userdir);
	print_cbuf ("    Query string", query_string);
	print_cbuf ("Query str. Orig.", query_string_original);
	print_cbuf ("            Host", host);
	print_cbuf ("        Redirect", redirect);
	print_cint ("    Redirect num", respins);
	print_cint ("       Keepalive", keepalive);
	print_cint ("Chunked-Encoding", chunked_encoding);
	print_str  ("           Phase", phase);
	print_cint ("     Range start", range_start);
	print_cint ("       Range end", range_end);
	print_cint ("            Rate", limit_rate);
	if (req->limit_rate) {
		print_cint ("        Rate BPS", limit_rate);
	}


	/* Options bit fields
	 */
	print_add ("\t|     Option bits:");
	if (req->options & conn_op_root_index)
		print_add (" root_index");
	if (req->options & conn_op_tcp_cork)
		print_add (" tcp_cork");
	if (req->options & conn_op_document_root)
		print_add (" document_root");
	if (req->options & conn_op_cant_encoder)
		print_add (" cant_encoder");
	if (req->options & conn_op_got_eof)
		print_add (" got_eof");

	print_add ("\n");

#undef print_buf
#undef print_cbuf
#undef print_cint
#undef print_str
#undef print_add

	cherokee_buffer_add_str (buf, "\t\\_\n");
	return buf->buf;
}
#endif

const char *
cherokee_request_get_phase_str (cherokee_request_t *req)
{
	switch (req->phase) {
	case phase_nothing:           return "Nothing";
	case phase_tls_handshake:     return "TLS handshake";
	case phase_reading_header:    return "Reading header";
	case phase_processing_header: return "Processing header";
	case phase_reading_post:      return "Reading POST";
	case phase_setup_connection:  return "Setting up connection";
	case phase_init:              return "Init connection";
	case phase_add_headers:       return "Add headers";
	case phase_send_headers:      return "Send headers";
	case phase_stepping:          return "Stepping";
	case phase_shutdown:          return "Shutdown connection";
	case phase_lingering:         return "Lingering close";
	default:
		SHOULDNT_HAPPEN;
	}
	return NULL;
}


ret_t
cherokee_request_set_redirect (cherokee_request_t *req, cherokee_buffer_t *address)
{
	cuint_t len;

	/* Build the redirection address
	 */
	cherokee_buffer_clean (&req->redirect);

	len = req->web_directory.len +
		address->len +
		req->userdir.len +
		req->host.len +
		sizeof(":65535") +
		sizeof("https://") + 4;

	cherokee_buffer_ensure_size (&req->redirect, len);

	if (! cherokee_buffer_is_empty (&req->host)) {
		if (req->socket.is_tls == TLS)
			cherokee_buffer_add_str (&req->redirect, "https://");
		else
			cherokee_buffer_add_str (&req->redirect, "http://");

		cherokee_buffer_add_buffer (&req->redirect, &req->host);

		if (req->host_port.len > 0) {
			cherokee_buffer_add_str    (&req->redirect, ":");
			cherokee_buffer_add_buffer (&req->redirect, &req->host_port);
		}
	}

	if (! cherokee_buffer_is_empty (&req->userdir)) {
		cherokee_buffer_add_str (&req->redirect, "/~");
		cherokee_buffer_add_buffer (&req->redirect, &req->userdir);
	}

	/* In case the connection has a custom Document Root directory,
	 * it must add the web equivalent directory to the path (web_directory).
	 */
	if (cherokee_request_use_webdir (req)) {
		cherokee_buffer_add_buffer (&req->redirect, &req->web_directory);
	}

	cherokee_buffer_add_buffer (&req->redirect, address);
	return ret_ok;
}


ret_t
cherokee_request_sleep (cherokee_request_t *req,
			cherokee_msec_t     msecs)
{
	req->limit_blocked_until = cherokee_bogonow_msec + msecs;
	return ret_ok;
}


void
cherokee_request_update_timeout (cherokee_request_t *req)
{
	if (req->timeout_lapse == -1) {
		TRACE (ENTRIES",timeout", "conn (%p, %s): Timeout = now + %d secs\n",
		       req, cherokee_request_get_phase_str (req), REQ_SRV(req)->timeout);

		req->timeout = cherokee_bogonow_now + REQ_SRV(req)->timeout;
		return;
	}

	TRACE (ENTRIES",timeout", "conn (%p, %s): Timeout = now + %d secs\n",
	       req, cherokee_request_get_phase_str (req), req->timeout_lapse);

	req->timeout = cherokee_bogonow_now + req->timeout_lapse;
}


ret_t
cherokee_request_build_host_string (cherokee_request_t *req,
				    cherokee_buffer_t  *buf)
{
	/* 1st choice: Request host */
	if (! cherokee_buffer_is_empty (&req->host)) {
		cherokee_buffer_add_buffer (buf, &req->host);
	}

	/* 2nd choice: Bound IP */
	else if ((req->bind != NULL) &&
		 (! cherokee_buffer_is_empty (&req->bind->ip)))
	{
		cherokee_buffer_add_buffer (buf, &req->bind->ip);
	}

	/* 3rd choice: Bound IP, rendered address */
	else if ((req->bind != NULL) &&
		 (! cherokee_buffer_is_empty (&req->bind->server_address)))
	{
		cherokee_buffer_add_buffer (buf, &req->bind->server_address);
	}

	return ret_ok;
}

ret_t
cherokee_request_build_host_port_string (cherokee_request_t *req,
					 cherokee_buffer_t  *buf)
{
	ret_t ret;

	/* Host
	 */
	ret = cherokee_request_build_host_string (req, buf);
	if (unlikely (ret != ret_ok)) {
		return ret_error;
	}

	/* Port
	 */
	if ((req->bind != NULL) &&
	    (! http_port_is_standard (req->bind->port, req->socket.is_tls)))
	{
		cherokee_buffer_add_char    (buf, ':');
		cherokee_buffer_add_ulong10 (buf, req->bind->port);
	}

	return ret_ok;
}
