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

#include "common-internal.h"
#include "connection.h"
#include "connection-protected.h"

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
#include "dirs_table.h"
#include "handler_error.h"
#include "buffer.h"
#include "buffer_escape.h"
#include "config_entry.h"
#include "encoder_table.h"
#include "server-protected.h"
#include "access.h"
#include "virtual_server.h"
#include "socket.h"
#include "header.h"
#include "header-protected.h"
#include "iocache.h"
#include "reqs_list.h"

#define ENTRIES "core,connection"


ret_t
cherokee_connection_new  (cherokee_connection_t **cnt)
{
	CHEROKEE_NEW_STRUCT(n, connection);
	   
	INIT_LIST_HEAD(&n->list_entry);

	n->tcp_cork          = 0;
	n->error_code        = http_ok;
	n->phase             = phase_reading_header;
	n->phase_return      = phase_nothing;
	n->auth_type         = http_auth_nothing;
	n->req_auth_type     = http_auth_nothing;
	n->upgrade           = http_upgrade_nothing;
	n->handler           = NULL; 
	n->encoder           = NULL;
	n->logger_ref        = NULL;
	n->keepalive         = 0;
	n->range_start       = 0;
	n->range_end         = 0;
	n->vserver           = NULL;
	n->log_at_end        = 1;
	n->arguments         = NULL;
	n->realm_ref         = NULL;
	n->mmaped            = NULL;
	n->io_entry_ref      = NULL;
	n->thread            = NULL;
	n->rx                = 0;	
	n->tx                = 0;
	n->rx_partial        = 0;
	n->tx_partial        = 0;
	n->traffic_next      = 0;
	n->validator         = NULL;
	n->req_matched_ref   = NULL;
	n->timeout           = -1;
	n->polling_fd        = -1;
	n->polling_multiple  = false;

	cherokee_buffer_init (&n->buffer);
	cherokee_buffer_init (&n->header_buffer);
	cherokee_buffer_init (&n->incoming_header);
	cherokee_buffer_init (&n->encoder_buffer);

	cherokee_buffer_init (&n->local_directory);
	cherokee_buffer_init (&n->web_directory);
	cherokee_buffer_init (&n->effective_directory);
	cherokee_buffer_init (&n->userdir);
	cherokee_buffer_init (&n->request);
	cherokee_buffer_init (&n->pathinfo);
	cherokee_buffer_init (&n->redirect);
	cherokee_buffer_init (&n->host);

	cherokee_buffer_init (&n->query_string);
	cherokee_buffer_init (&n->request_original);

	cherokee_buffer_escape_new (&n->request_escape);
	cherokee_buffer_escape_set_ref (n->request_escape, &n->request);

	cherokee_socket_init (&n->socket);
	cherokee_header_init (&n->header);
	cherokee_post_init (&n->post);

	*cnt = n;
	return ret_ok;
}


ret_t
cherokee_connection_free (cherokee_connection_t  *cnt)
{
	cherokee_header_mrproper (&cnt->header);
	cherokee_socket_mrproper (&cnt->socket);
	
	if (cnt->handler != NULL) {
		cherokee_handler_free (cnt->handler);
		cnt->handler = NULL;
	}

	if (cnt->encoder != NULL) {
		cherokee_encoder_free (cnt->encoder);
		cnt->encoder = NULL;
	}

	cherokee_post_mrproper (&cnt->post);
	
	cherokee_buffer_escape_free (cnt->request_escape);
	cherokee_buffer_mrproper (&cnt->request);
	cherokee_buffer_mrproper (&cnt->request_original);

	cherokee_buffer_mrproper (&cnt->pathinfo);
	cherokee_buffer_mrproper (&cnt->buffer);
	cherokee_buffer_mrproper (&cnt->header_buffer);
	cherokee_buffer_mrproper (&cnt->incoming_header);
	cherokee_buffer_mrproper (&cnt->query_string);
	cherokee_buffer_mrproper (&cnt->encoder_buffer);

	cherokee_buffer_mrproper (&cnt->local_directory);
	cherokee_buffer_mrproper (&cnt->web_directory);
	cherokee_buffer_mrproper (&cnt->effective_directory);
	cherokee_buffer_mrproper (&cnt->userdir);
	cherokee_buffer_mrproper (&cnt->redirect);
	cherokee_buffer_mrproper (&cnt->host);

	if (cnt->validator != NULL) {
		cherokee_validator_free (cnt->validator);
		cnt->validator = NULL;
	}

	if (cnt->arguments != NULL) {
		cherokee_table_free2 (cnt->arguments, free);
		cnt->arguments = NULL;
	}

        if (cnt->polling_fd != -1) {
                close (cnt->polling_fd);
                cnt->polling_fd = -1;
        }
	
	free (cnt);
	return ret_ok;
}


ret_t
cherokee_connection_clean (cherokee_connection_t *cnt)
{	   
	uint32_t           header_len;
	cherokee_server_t *srv = CONN_SRV(cnt);

#ifndef CHEROKEE_EMBEDDED
	if (cnt->io_entry_ref != NULL) {
		cherokee_iocache_mmap_release (srv->iocache, cnt->io_entry_ref);
		cnt->io_entry_ref = NULL;		
	}
#endif

	cnt->timeout           = -1;
	cnt->phase             = phase_reading_header;
	cnt->phase_return      = phase_nothing;
	cnt->auth_type         = http_auth_nothing;
	cnt->req_auth_type     = http_auth_nothing;
	cnt->upgrade           = http_upgrade_nothing;
	cnt->error_code        = http_ok;
	cnt->range_start       = 0;
	cnt->range_end         = 0;
	cnt->logger_ref        = NULL;
	cnt->tcp_cork          = 0;
	cnt->log_at_end        = 1;
	cnt->realm_ref         = NULL;
	cnt->mmaped            = NULL;
	cnt->rx                = 0;	
	cnt->tx                = 0;
	cnt->rx_partial        = 0;	
	cnt->tx_partial        = 0;
	cnt->traffic_next      = 0;
	cnt->req_matched_ref   = NULL;
	cnt->polling_multiple  = false;

	if (cnt->handler != NULL) {
		cherokee_handler_free (cnt->handler);
		cnt->handler = NULL;
	}
	
	if (cnt->encoder != NULL) {
		cherokee_encoder_free (cnt->encoder);
		cnt->encoder = NULL;
	}

        if (cnt->polling_fd != -1) {
                close (cnt->polling_fd);
                cnt->polling_fd = -1;
        }

	cherokee_post_mrproper (&cnt->post);

	cherokee_buffer_clean (&cnt->request);
	cherokee_buffer_escape_clean (cnt->request_escape);
	cherokee_buffer_escape_set_ref (cnt->request_escape, &cnt->request);

	cherokee_buffer_mrproper (&cnt->request_original);
	cherokee_buffer_mrproper (&cnt->encoder_buffer);

	cherokee_buffer_clean (&cnt->pathinfo);
	cherokee_buffer_clean (&cnt->local_directory);
	cherokee_buffer_clean (&cnt->web_directory);
	cherokee_buffer_clean (&cnt->effective_directory);
	cherokee_buffer_clean (&cnt->userdir);
	cherokee_buffer_clean (&cnt->redirect);
	cherokee_buffer_clean (&cnt->host);
	cherokee_buffer_clean (&cnt->query_string);
	
	if (cnt->validator != NULL) {
		cherokee_validator_free (cnt->validator);
		cnt->validator = NULL;
	}

	if (cnt->arguments != NULL) {
		cherokee_table_free2 (cnt->arguments, free);
		cnt->arguments = NULL;
	}

	/* Drop out the loast incoming header
	 */
	cherokee_header_get_length (&cnt->header, &header_len);

	cherokee_header_clean (&cnt->header);
	cherokee_buffer_clean (&cnt->buffer);
	cherokee_buffer_clean (&cnt->header_buffer);
	cherokee_buffer_move_to_begin (&cnt->incoming_header, header_len);

	/* If the connection has incoming headers to be processed,
	 * then increment the pending counter from the thread
	 */	 
	if (! cherokee_buffer_is_empty (&cnt->incoming_header)) {
		CONN_THREAD(cnt)->pending_conns_num++;
	}

	TRACE (ENTRIES, "conn %p, has headers %d\n", cnt, 
	       !cherokee_buffer_is_empty (&cnt->incoming_header));

	return ret_ok;
}


ret_t 
cherokee_connection_mrproper (cherokee_connection_t *cnt)
{
	ret_t ret;

	cnt->keepalive = 0;
	
	/* Close and clean the socket
	 */
	ret = cherokee_socket_close (&cnt->socket);
	if (unlikely(ret < ret_ok)) return ret;

	ret = cherokee_socket_clean (&cnt->socket);
	if (unlikely(ret < ret_ok)) return ret;

	/* Clean the connection object
	 */
	cherokee_connection_clean (cnt);

	/* It is not a keep-alive connection, so we shouldn't
	 * keep any previous header
	 */
	cherokee_buffer_clean (&cnt->incoming_header);

	return ret_ok;
}



ret_t
cherokee_connection_setup_error_handler (cherokee_connection_t *cnt)
{
	ret_t                       ret;
	cherokee_server_t          *srv;
	cherokee_virtual_server_t  *vsrv;
	cherokee_config_entry_t    *entry;	

	srv   = CONN_SRV(cnt);
	vsrv  = CONN_VSRV(cnt);
	entry = vsrv->error_handler;

	/* On error, it will close the socket
	 */
	cnt->keepalive = 0;

	/* It has a common handler. It has to be freed.
	 */
	if (cnt->handler != NULL) {
		cherokee_handler_free (cnt->handler);
		cnt->handler = NULL;
	}

	/* Create a new error handler
	 */
	if ((entry != NULL) && (entry->handler_new_func != NULL)) {
		ret = entry->handler_new_func ((void **) &cnt->handler, cnt, entry->handler_properties);
		if (ret == ret_ok) goto out;

#ifdef TRACE_ENABLED
		{ 
			const char *name = NULL;
			cherokee_module_get_name (MODULE(cnt->handler), &name);
			TRACE(ENTRIES, "New handler %s\n", name);
		}
#endif
	} 

	/* If something was wrong, try with the default error handler
	 */
	ret = cherokee_handler_error_new (&cnt->handler, cnt, NULL);

out:
	/* Nothing should be mmaped any longer
	 */
	if (cnt->mmaped != NULL) {
#ifndef CHEROKEE_EMBEDDED
		ret = cherokee_iocache_mmap_release (srv->iocache, cnt->io_entry_ref);
		cnt->mmaped       = NULL;
		cnt->io_entry_ref = NULL;
#endif
	}

	return ret;
}


static void
build_response_header__authenticate (cherokee_connection_t *cnt, cherokee_buffer_t *buffer)
{
	/* Basic Authenticatiom
	 * Eg: WWW-Authenticate: Basic realm=""
	 */
	if (cnt->auth_type & http_auth_basic) {
		cherokee_buffer_ensure_size (buffer, 31 + cnt->realm_ref->len + 4);
		cherokee_buffer_add_str (buffer, "WWW-Authenticate: Basic realm=\"");
		cherokee_buffer_add_buffer (buffer, cnt->realm_ref);
		cherokee_buffer_add_str (buffer, "\""CRLF);
	}

	/* Digest Authenticatiom
	 * Eg: WWW-Authenticate: Digest realm="", qop="auth,auth-int", nonce="", opaque=""
	 */
	if (cnt->auth_type & http_auth_digest) {
		cherokee_buffer_t new_nonce = CHEROKEE_BUF_INIT;
		/* Realm
		 */
		cherokee_buffer_ensure_size (buffer, 32 + cnt->realm_ref->len + 4);
		cherokee_buffer_add_str (buffer, "WWW-Authenticate: Digest realm=\"");
		cherokee_buffer_add_buffer (buffer, cnt->realm_ref);
		cherokee_buffer_add_str (buffer, "\", ");

		/* Nonce
		 */
		cherokee_nonce_table_generate (CONN_SRV(cnt)->nonces, cnt, &new_nonce);
		cherokee_buffer_add_va (buffer, "nonce=\"%s\", ", new_nonce.buf);
				
		/* Quality of protection: auth, auth-int, auth-conf
		 * Algorithm: MD5
		 */
		cherokee_buffer_add_str (buffer, "qop=\"auth\", algorithm=\"MD5\""CRLF);

		cherokee_buffer_mrproper (&new_nonce);
	}
}


static void
build_response_header (cherokee_connection_t *cnt, cherokee_buffer_t *buffer)
{	
	/* Build the response header
	 */
	cherokee_buffer_clean (buffer);

	/* Add protocol string + error_code
	 */
	switch (cnt->header.version) {
	case http_version_09:
		cherokee_buffer_add_str (buffer, "HTTP/0.9 "); 
		break;
	case http_version_10:
		cherokee_buffer_add_str (buffer, "HTTP/1.0 "); 
		break;
	case http_version_11:
	default:
		cherokee_buffer_add_str (buffer, "HTTP/1.1 "); 
		break;
	}
	
	cherokee_http_code_copy (cnt->error_code, buffer);
	cherokee_buffer_add (buffer, CRLF, 2);

	/* Add the "Connection:" header
	 */
	if (cnt->upgrade != http_upgrade_nothing) {
		cherokee_buffer_add_str (buffer, "Connection: Upgrade"CRLF);

	} else if (cnt->handler && (cnt->keepalive > 0)) {
		cherokee_buffer_add_str (buffer, "Connection: Keep-Alive"CRLF);
		cherokee_buffer_add_buffer (buffer, CONN_SRV(cnt)->timeout_header);

	} else {
		cherokee_buffer_add_str (buffer, "Connection: Close"CRLF);
	}

	/* Date
	 */
	cherokee_buffer_add_str (buffer, "Date: ");
	cherokee_buffer_add_buffer (buffer, CONN_SRV(cnt)->bogo_now_string);
	cherokee_buffer_add (buffer, CRLF, 2);

	/* Add the Server header
	 */
	cherokee_buffer_add_str (buffer, "Server: ");
	cherokee_buffer_add_buffer (buffer, CONN_SRV(cnt)->server_string);
	cherokee_buffer_add_str (buffer, CRLF);

	/* Authentication
	 */
	if ((cnt->realm_ref != NULL) && (cnt->error_code == http_unauthorized))
	{
		build_response_header__authenticate (cnt, buffer);
	}

	/* Redirected connections
	 */
	if (cnt->redirect.len >= 1) {
		cherokee_buffer_add_str (buffer, "Location: ");
		cherokee_buffer_add_buffer (buffer, &cnt->redirect);
		cherokee_buffer_add_str (buffer, CRLF);
	}

	/* Encoder headers
	 */
	if (cnt->encoder) {
		cherokee_encoder_add_headers (cnt->encoder, buffer);
		
		/* Keep-alive is not possible w/o a file cache
		 */
		cnt->keepalive = 0;
		if (cnt->handler->support & hsupport_length) {
			cnt->handler->support ^= hsupport_length;
		}
	}

	/* Unusual methods
	 */
	if (cnt->header.method == http_options) {
		cherokee_buffer_add_str (buffer, "Allow: GET, HEAD, POST, OPTIONS"CRLF);
	}
	
	/* Maybe the handler add some headers from the handler
	 */
	if (! cherokee_buffer_is_empty (&cnt->header_buffer)) {
		cherokee_buffer_add_buffer (buffer, &cnt->header_buffer);
	}

	/* Add the response header ends
	 */
	cherokee_buffer_add_str (buffer, CRLF);
}


ret_t 
cherokee_connection_build_header (cherokee_connection_t *cnt)
{
	ret_t ret;

	/* Try to get the headers from the handler
	 */
	ret = cherokee_handler_add_headers (cnt->handler, &cnt->header_buffer);
	switch (ret) {
	case ret_ok:
		break;

	case ret_eof:
	case ret_error:
	case ret_eagain:
		return ret;

	default:
		RET_UNKNOWN(ret);
		return ret_error;
	}

	if (HANDLER_SUPPORT_MAYBE_LENGTH(cnt->handler)) {
		if (strcasestr (cnt->header_buffer.buf, "Content-length: ") == NULL) {
			cnt->keepalive = 0;
		}
	}
	
	/* Add the server headers	
	 */
	build_response_header (cnt, &cnt->buffer);

	return ret_ok;
}


ret_t 
cherokee_connection_send_header_and_mmaped (cherokee_connection_t *cnt)
{
	size_t  re;
	ret_t   ret;
	struct iovec bufs[2];

	/* 1.- Special case: There is not header to send
	 * It is becase it has been sent in a writev()
	 */
	if (cherokee_buffer_is_empty (&cnt->buffer)) {
		ret = cherokee_write (&cnt->socket, cnt->mmaped, cnt->mmaped_len, &re);
		switch (ret) {
		case ret_eof:
		case ret_eagain:
			return ret;

		case ret_error:
			cnt->keepalive = 0;
			return ret_error;

		default:
			break;
		}
		
		cherokee_connection_tx_add (cnt, re);

		cnt->mmaped_len -= re;
		cnt->mmaped     += re;

		return (cnt->mmaped_len <= 0) ? ret_ok : ret_eagain;
	}

	/* 2.- There are header and mmaped content to send
	 */
	bufs[0].iov_base = cnt->buffer.buf;
	bufs[0].iov_len  = cnt->buffer.len;
	bufs[1].iov_base = cnt->mmaped;
	bufs[1].iov_len  = cnt->mmaped_len;

	ret = cherokee_writev (&cnt->socket, bufs, 2, &re);

	switch (ret) {
	case ret_ok: 
		break;

	case ret_eof:
	case ret_eagain: 
		return ret;

	case ret_error:
		cnt->keepalive = 0;
		return ret_error;

	default:
		RET_UNKNOWN(ret);
	}
	
	/* If writev() has not sent all data
	 */
	if (re < cnt->buffer.len + cnt->mmaped_len) {
		int offset;

		if (re <= cnt->buffer.len) {
			cherokee_buffer_move_to_begin (&cnt->buffer, re);
			return ret_eagain;
		}
		
		offset = re - cnt->buffer.len;
		cnt->mmaped     += offset;
		cnt->mmaped_len -= offset;

		cherokee_buffer_clean (&cnt->buffer);

		return ret_eagain;
	}

	/* Add to the connection traffic counter
	 */
	cherokee_connection_tx_add (cnt, re);

	return ret_ok;
}


void
cherokee_connection_rx_add (cherokee_connection_t *cnt, ssize_t rx)
{
	cnt->rx += rx;
	cnt->rx_partial += rx;
}


void
cherokee_connection_tx_add (cherokee_connection_t *cnt, ssize_t tx)
{
	cnt->tx += tx;
	cnt->tx_partial += tx;
}


ret_t
cherokee_connection_recv (cherokee_connection_t *cnt, cherokee_buffer_t *buffer, off_t *len)
{
	ret_t  ret;
	size_t readed = 0;
	
	ret = cherokee_socket_read (&cnt->socket, buffer, DEFAULT_RECV_SIZE, &readed);

	switch (ret) {
	case ret_ok:
		cherokee_connection_rx_add (cnt, readed);
		*len = readed;
		return ret_ok;

	case ret_eof:
	case ret_error:
	case ret_eagain:
		return ret;

	default:
		RET_UNKNOWN(ret);		
	}

	return ret_error;
}


ret_t 
cherokee_connection_reading_check (cherokee_connection_t *cnt)
{
	/* Check for too long headers
	 */
	if (cnt->incoming_header.len > MAX_HEADER_LEN) {
		cnt->error_code = http_request_uri_too_long;
		return ret_error;
	}

	return ret_ok;
}


ret_t 
cherokee_connection_set_cork (cherokee_connection_t *cnt, int enable)
{
	int on = 0;
	int fd;

#ifdef HAVE_TCP_CORK
	fd = SOCKET_FD(&cnt->socket);
	if (enable) {
		setsockopt(fd, IPPROTO_TCP, TCP_NODELAY,  &on, sizeof on);

		on = 1;
		setsockopt(fd, IPPROTO_TCP, TCP_CORK,  &on, sizeof on);
	} else {
		setsockopt(fd, IPPROTO_TCP, TCP_CORK,  &on, sizeof on);

		on = 1;
		setsockopt(fd, IPPROTO_TCP, TCP_NODELAY,  &on, sizeof on);
	}

	cnt->tcp_cork = enable;
#endif

	return ret_ok;
}


ret_t
cherokee_connection_send_header (cherokee_connection_t *cnt)
{
	ret_t  ret;
	size_t sent = 0;

	/* Send the buffer content
	 */
	ret = cherokee_socket_write (&cnt->socket, &cnt->buffer, &sent);
	if (unlikely(ret != ret_ok)) return ret;
	
	/* Add to the connection traffic counter
	 */
	cherokee_connection_tx_add (cnt, sent);

	/* Drop out the sent info
	 */
	if (sent == cnt->buffer.len) {
		cherokee_buffer_clean (&cnt->buffer);
		return ret_ok;
	}

	/* There are still info waiting to be sent
	 */
	cherokee_buffer_move_to_begin (&cnt->buffer, sent);
	return ret_eagain;
}


ret_t
cherokee_connection_send (cherokee_connection_t *cnt)
{
	ret_t  ret;
	size_t sent = 0;

	/* Send the buffer content
	 */
	ret = cherokee_socket_write (&cnt->socket, &cnt->buffer, &sent);
	if (unlikely(ret != ret_ok)) return ret;

	/* Add to the connection traffic counter
	 */
	cherokee_connection_tx_add (cnt, sent);

	/* Drop out the sent info
	 */
	if (sent == cnt->buffer.len) {
		cherokee_buffer_clean (&cnt->buffer);
	} else if (sent != 0) {
		cherokee_buffer_move_to_begin (&cnt->buffer, sent);
	}
	
	/* If this connection has a handler without content-length support
	 * it has to count the bytes sent
	 */
	if (!HANDLER_SUPPORT_LENGTH(cnt->handler)) {
		cnt->range_end += sent;
	}

	return ret_ok;
}


ret_t 
cherokee_connection_pre_lingering_close (cherokee_connection_t *cnt)
{
	ret_t  ret;
	size_t readed = 0;

	/* At this point, we don't want to follow the TLS protocol
	 * any longer.
	 */
	cnt->socket.is_tls = non_TLS;

	/* Shut down the socket for write, which will send a FIN
	 * to the peer.
	 */
	ret = cherokee_socket_shutdown (&cnt->socket, SHUT_WR);
	if (unlikely (ret != ret_ok)) return ret_ok;

	/* Set the timeout
	 */
	ret = cherokee_socket_set_timeout (&cnt->socket, MSECONS_TO_LINGER);	
	if (unlikely (ret != ret_ok)) return ret_ok;

	/* Read from the socket to nowhere
	 */
	ret = cherokee_socket_read (&cnt->socket, NULL, DEFAULT_RECV_SIZE, &readed);
	switch (ret) {
	case ret_eof:
	case ret_error:
	case ret_eagain:
		TRACE(ENTRIES, "%s\n", "ok");
		return ret_ok;
	case ret_ok:
		TRACE(ENTRIES, "readed %d, eagain\n", readed);
		return ret_eagain;
	default:
		RET_UNKNOWN(ret);		
	}

	return ret_error;
}


ret_t
cherokee_connection_step (cherokee_connection_t *cnt)
{
	ret_t step_ret = ret_ok;

	return_if_fail (cnt->handler != NULL, ret_error);

	/* Need to 'read' from handler ?
	 */
	if (cnt->buffer.len > 0) {
		return ret_ok;
	}

	/* Do a step in the handler
	 */
	step_ret = cherokee_handler_step (cnt->handler, &cnt->buffer);
	switch (step_ret) {
	case ret_ok:
	case ret_eof:
	case ret_eof_have_data:
		break;

	case ret_error:
	case ret_eagain:
	case ret_ok_and_sent:
		return step_ret;

	default:
		RET_UNKNOWN(step_ret);
	}

	/* May be encode..
	 */
	if (cnt->encoder != NULL) {
		ret_t ret;

		/* Encode
		 */
		switch (step_ret) {
		case ret_eof:
		case ret_eof_have_data:
			ret = cherokee_encoder_flush (cnt->encoder, &cnt->buffer, &cnt->encoder_buffer);			
			step_ret = (cnt->encoder_buffer.len == 0)? ret_eof : ret_eof_have_data;
			break;
		default:
			ret = cherokee_encoder_encode (cnt->encoder, &cnt->buffer, &cnt->encoder_buffer);
			break;
		}
		if (ret < ret_ok) return ret;

		/* Swap buffers	
		 */
		cherokee_buffer_swap_buffers (&cnt->buffer, &cnt->encoder_buffer);		
		cherokee_buffer_clean (&cnt->encoder_buffer);
	}
	
	return step_ret;
}


static inline ret_t
get_host (cherokee_connection_t *cnt, 
	  char                  *ptr,
	  int                    size) 
{
	ret_t    ret;
	char    *i;
	char    *end = ptr + size;
	cuint_t  skip = 0;

	/* Sanity check
	 */
	if (size <= 0) return ret_error;

	/* Skip "colon + port"
	 */
	for (i=end; i>=ptr; i--) {
		if (*i == ':') {
			skip = end - i;
			break;
		}
	}

	/* Copy the string
	 */
	if (unlikely (size - skip) <= 0)
		return ret_error;

	ret = cherokee_buffer_add (&cnt->host, ptr, size - skip);
	if (unlikely(ret < ret_ok)) return ret;

	/* Security check: Hostname shouldn't start with a dot
	 */
	if ((cnt->host.len >= 1) && (*cnt->host.buf == '.')) {
		return ret_error;
	}

	/* RFC-1034: Dot ending host names
	 */
	if (cherokee_buffer_end_char (&cnt->host) == '.')
		cherokee_buffer_drop_endding (&cnt->host, 1);
	
	return ret_ok;
}


static inline ret_t
get_encoding (cherokee_connection_t    *cnt,
	      char                     *ptr,
	      cherokee_encoder_table_t *encoders) 
{
	char tmp;
	char *i1, *i2;
	char *end;
	char *ext;
	ret_t ret;

	/* ptr = Header at the "Accept-Encoding" position 
	 */
	end = strchr (ptr, '\r');
	if (end == NULL) {
		return ret_error;
	}

	/* Look for the request extension
	 */
	ext = strrchr (cnt->request.buf, '.');
	if (ext == NULL) {
		return ret_ok;
	}

	*end = '\0'; /* (1) */
	
	i1 = ptr;
	
	do {
		i2 = strchr (i1, ',');
		if (!i2) i2 = strchr (i1, ';');
		if (!i2) i2 = end;

		tmp = *i2;    /* (2) */
		*i2 = '\0';
		cherokee_encoder_table_new_encoder (encoders, i1, ext+1, &cnt->encoder);
		*i2 = tmp;    /* (2') */

		if (cnt->encoder != NULL) {
			/* Init the encoder related objects
			 */
			ret = cherokee_encoder_init (cnt->encoder, cnt);
			if (ret < ret_ok) {
				goto error;
			}
			cherokee_buffer_clean (&cnt->encoder_buffer);
			break;
		}

		if (i2 < end) {
			i1 = i2+1;
		}

	} while (i2 < end);

	*end = '\r'; /* (1') */
	return ret_ok;

error:
	*end = '\r'; /* (1') */
	return ret_error;
}


static ret_t
get_authorization (cherokee_connection_t *cnt,
		   cherokee_http_auth_t   type,
		   cherokee_validator_t  *validator,
		   char                  *ptr,
	           int                    ptr_len)
{
	ret_t    ret;
	char    *end, *end2;
	cuint_t  pre_len;

	/* It checks that the authentication send by the client is compliant
	 * with the configuration of the server.  It does not check if the 
	 * kind of validator is suitable in this case.
	 */
	if (strncasecmp(ptr, "Basic ", 6) == 0) {

		/* Check the authentication type
 		 */
		if (!(type & http_auth_basic))
			return ret_error;

		cnt->req_auth_type = http_auth_basic;
		pre_len = 6;

	} else if (strncasecmp(ptr, "Digest ", 7) == 0) {

		/* Check the authentication type
		 */
		if (!(type & http_auth_digest))
			return ret_error;

		cnt->req_auth_type = http_auth_digest;
		pre_len = 7;
	}

	/* Skip end of line
	 */
	end  = strchr (ptr, '\r');
	end2 = strchr (ptr, '\n');

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
	switch (cnt->req_auth_type) {
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

		/* If it returns ret_ok, it means that the nonce was on the table,
		 * and it removed it successfuly, otherwhise ret_not_found is returned.
		 */
		ret = cherokee_nonce_table_remove (CONN_SRV(cnt)->nonces, &validator->nonce);
		if (ret != ret_ok) return ret;
		
		break;

	default:
		PRINT_ERROR_S ("Unknown authentication method\n");
		return ret_error;
	}
	
	return ret_ok;
}


int
cherokee_connection_is_userdir (cherokee_connection_t *cnt)
{
	return ((cnt->request.len > 3) && 
		(cnt->request.buf[1] == '~'));
}


ret_t
cherokee_connection_build_local_directory (cherokee_connection_t *cnt, cherokee_virtual_server_t *vsrv, cherokee_config_entry_t *entry)
{
	ret_t ret;

	if (entry->document_root && 
	    entry->document_root->len >= 1) 
	{
		/* Have a special DocumentRoot
		 */
		ret = cherokee_buffer_add_buffer (&cnt->local_directory, entry->document_root);

		/* It has to drop the webdir from the request:
		 *	
		 * Directory /thing {
		 *    DocumentRoot /usr/share/this/rocks
		 * }	
		 *	
		 * on petition: http://server/thing/cherokee	
		 * should read: /usr/share/this/rocks/cherokee	
		 */
		cherokee_buffer_add_buffer (&cnt->request_original, &cnt->request);
		cherokee_buffer_move_to_begin (&cnt->request, cnt->web_directory.len);
	
		if ((cnt->request.len >= 2) && (strncmp(cnt->request.buf, "//", 2) == 0)) {
			cherokee_buffer_move_to_begin (&cnt->request, 1);
		}

	} else {
		/* Normal request
		 */
		ret = cherokee_buffer_add_buffer (&cnt->local_directory, vsrv->root);
	}
	
	return ret;
}


ret_t
cherokee_connection_build_local_directory_userdir (cherokee_connection_t *cnt, cherokee_virtual_server_t *vsrv, cherokee_config_entry_t *entry)
{
	struct passwd *pwd;	

	/* Has a defined DocumentRoot
	 */
	if (entry->document_root &&
	    entry->document_root->len >= 1) 
	{
		cherokee_buffer_add_buffer (&cnt->local_directory, entry->document_root);

		cherokee_buffer_add_buffer (&cnt->request_original, &cnt->request);
		cherokee_buffer_move_to_begin (&cnt->request, cnt->web_directory.len);

		if ((cnt->request.len >= 2) && (strncmp(cnt->request.buf, "//", 2) == 0)) {
			cherokee_buffer_move_to_begin (&cnt->request, 1);
		}

		return ret_ok;
	}

	/* Default: it is inside the UserDir in home
	 */
	pwd = (struct passwd *) getpwnam (cnt->userdir.buf);
	if ((pwd == NULL) || (pwd->pw_dir == NULL)) {
		cnt->error_code = http_not_found;
		return ret_error;
	}

	/* Build the local_directory:
	 */
	cherokee_buffer_add (&cnt->local_directory, pwd->pw_dir, strlen(pwd->pw_dir));
	cherokee_buffer_add (&cnt->local_directory, "/", 1);
	cherokee_buffer_add_buffer (&cnt->local_directory, vsrv->userdir);

	return ret_ok;
}


static ret_t
get_range (cherokee_connection_t *cnt, char *ptr, int ptr_len) 
{
	CHEROKEE_TEMP(tmp, ptr_len+1);
	int num_len = 0;

	/* Read the start position
	 */
	while ((ptr[num_len] != '-') && (ptr[num_len] != '\0') && (num_len < tmp_size-1)) {
		tmp[num_len] = ptr[num_len];
		num_len++;
	}
	tmp[num_len] = '\0';
	if (num_len != 0) {
		cnt->range_start = atoi (tmp);
		if (cnt->range_start < 0) {
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
	if ((*ptr != '\0') && (*ptr != '\r') && (*ptr != '\n')) {
		num_len = 0;
		
		/* Read the end
		 */
		while ((ptr[num_len] >= '0') && (ptr[num_len] <= '9') && (num_len < tmp_size-1)) {
			tmp[num_len] = ptr[num_len];
			num_len++;
		}
		tmp[num_len] = '\0';
		cnt->range_end = atoi (tmp);
		if (cnt->range_end < 1){
			return ret_error;
		}
	}

	/* Sanity check: switched range
	 */
	if ((cnt->range_start != 0) && (cnt->range_end != 0)) {
		if (cnt->range_start > cnt->range_end) {
			cnt->error_code = http_range_not_satisfiable;
			return ret_error;
		}
	}

	return ret_ok;
	
}


static ret_t
post_init (cherokee_connection_t *cnt)
{
	ret_t    ret;
	long     post_len;
	char    *info     = NULL;
	cuint_t  info_len = 0;

	CHEROKEE_TEMP(buf,64);

	/* Get the header "Content-Lenght" content
	 */
	ret = cherokee_header_get_known (&cnt->header, header_content_length, &info, &info_len);
	if (ret != ret_ok) {
		cnt->error_code = http_length_required;
		return ret_error;
	}

	/* Parse the POST length
	 */
	if ((info_len == 0) || (info_len >= buf_size) || (info == NULL)) {
		cnt->error_code = http_bad_request;
		return ret_error;
	}

	memcpy (buf, info, info_len);
	buf[info_len] = '\0';
		
	post_len = atol(buf);
	if (post_len < 0) {
		cnt->error_code = http_bad_request;
		return ret_error;
	}

	/* Set the length
	 */
	cherokee_post_set_len (&cnt->post, post_len);
	return ret_ok;
}


static ret_t
parse_userdir (cherokee_connection_t *cnt)
{
	char *begin;
	char *end_username;

	/* Find user name endding:
	 */
	begin = &cnt->request.buf[2];

	end_username = strchr (begin, '/');
	if (end_username == NULL) {

		/* It has to be redirected 
		 *
		 * from http://www.alobbs.com/~alo 
		 * to   http://www.alobbs.com/~alo/
		 */
		cherokee_buffer_add_va (&cnt->redirect, "%s/", cnt->request.buf);

		cnt->error_code = http_moved_permanently;
		return ret_error;
	}

	/* Sanity check:
	 * The username has to be at least a char long
	 */
	if ((end_username - begin) <= 0) {
		cnt->error_code = http_bad_request;
		return ret_error;
	}

	/* Get the user home directory
	 */
	cherokee_buffer_add (&cnt->userdir, begin, end_username - begin);

	/* Drop username from the request
	 */
	cherokee_buffer_move_to_begin (&cnt->request, (end_username - cnt->request.buf));

	return ret_ok;
}


ret_t 
cherokee_connection_get_request (cherokee_connection_t *cnt)
{
	ret_t    ret;
	char    *host, *upgrade, *conn;
	cuint_t  host_len, upgrade_len, conn_len;

	/* Header parsing
	 */
	ret = cherokee_header_parse (&cnt->header, &cnt->incoming_header, header_type_request);
	if (ret < ret_ok) {
		goto error;
	}

	/* Maybe read the POST data
	 */
	if (http_method_with_input (cnt->header.method)) {
		uint32_t header_len;
		uint32_t post_len;

		ret = post_init (cnt);
		if (unlikely (ret != ret_ok)) {
			return ret;
		}

		ret = cherokee_header_get_length (&cnt->header, &header_len);
		if (unlikely(ret != ret_ok)) return ret;

		post_len = cnt->incoming_header.len - header_len;

		cherokee_post_append (&cnt->post, cnt->incoming_header.buf + header_len, post_len);
		cherokee_buffer_drop_endding (&cnt->incoming_header, post_len);
	}
	
	/* Copy the request
	 */
	ret = cherokee_header_copy_request (&cnt->header, &cnt->request);
	if (ret < ret_ok) goto error;

	/* Look for starting '/' in the request
	 */
	if (cnt->request.buf[0] != '/') {
		goto error;
	}

	/* Short the path. It transforms the request:
	 * /dir1/dir2/../file in /dir1/file
	 */
	cherokee_short_path (&cnt->request);

	/* Look for "//" 
	 */
	cherokee_buffer_remove_dups (&cnt->request, '/');

	/* Read the Host header
	 */
	ret = cherokee_header_get_known (&cnt->header, header_host, &host, &host_len);
	switch (ret) {
	case ret_error:
	case ret_not_found:
		if (cnt->header.version == http_version_11) {
			/* It is needed in HTTP/1.1
			 */
			goto error;
		}
		break;

	case ret_ok:
		ret = get_host (cnt, host, host_len);
		if (unlikely(ret < ret_ok)) goto error;
		
		/* Set the virtual host reference
		 */
		cherokee_table_get (CONN_SRV(cnt)->vservers_ref, cnt->host.buf, &cnt->vserver);

		break;

	default:
		RET_UNKNOWN(ret);
	}

	/* Userdir requests
	 */
	if ((!cherokee_buffer_is_empty (CONN_VSRV(cnt)->userdir)) && 
	    cherokee_connection_is_userdir (cnt)) 
	{
		ret = parse_userdir (cnt);
		if (ret != ret_ok) return ret;
	}

	/* RFC 2817: Client Requested Upgrade to HTTP over TLS
	 *
	 * When the client sends an HTTP/1.1 request with an Upgrade header
	 * field containing the token "TLS/1.0", it is requesting the server
	 * to complete the current HTTP/1.1 request after switching to TLS/1.0
	 */
	if (CONN_SRV(cnt)->tls_enabled) {
		ret = cherokee_header_get_known (&cnt->header, header_upgrade, &upgrade, &upgrade_len);
		if (ret == ret_ok) {

			/* Note that HTTP/1.1 [1] specifies "the upgrade keyword MUST be
			 * supplied within a Connection header field (section 14.10)
			 * whenever Upgrade is present in an HTTP/1.1 message".
			 */
			ret = cherokee_header_get_known (&cnt->header, header_connection, &conn, &conn_len);
			if (ret == ret_ok) {
				if (strncasecmp("Upgrade", conn, 7) == 0) {
					if (strncasecmp("TLS", upgrade, 3) == 0) {
						cnt->phase = phase_switching_headers;
						return ret_eagain;
					}
				}
			}
		}
	}

	cnt->error_code = http_ok;
	return ret_ok;	

error:
	cnt->error_code = http_bad_request;
	return ret_error;
}


ret_t
cherokee_connection_send_switching (cherokee_connection_t *cnt)
{
	ret_t ret;

	/* Maybe build the response string
	 */
	if (cherokee_buffer_is_empty (&cnt->buffer)) {
		cnt->error_code = http_switching_protocols;
		build_response_header (cnt, &cnt->buffer);
	}

	/* Send the response
	 */
	ret = cherokee_connection_send_header (cnt);
	switch (ret) {
	case ret_ok:
		break;

	case ret_eof:		
	case ret_error:
	case ret_eagain:
		return ret;

	default:
		RET_UNKNOWN(ret);
	}

	return ret_ok;
}


ret_t 
cherokee_connection_get_dir_entry (cherokee_connection_t *cnt, cherokee_dirs_table_t *dirs, cherokee_config_entry_t *config_entry)
{
	ret_t ret;

	return_if_fail (dirs != NULL, ret_error);

	/* Look for the handler "*_new" function
	 */
	ret = cherokee_dirs_table_get (dirs, &cnt->request, config_entry, &cnt->web_directory);
	if (unlikely (ret == ret_error)) {
		cnt->error_code = http_internal_error;
		return ret_error;
	}	

	/* If the request is exactly the directory entry, and it
	 * doesn't end with a slash, it must be redirected. Eg:
	 *
	 * web_directory = "/blog"
	 * request       = "/blog"
	 *
	 * It must be redirected to "/blog/"
	 */
	if ((cnt->request.len == cnt->web_directory.len) &&
	    (cnt->request.buf[cnt->request.len-1] != '/') &&
	    (strcmp (cnt->request.buf, cnt->web_directory.buf) == 0))
	{
		cherokee_buffer_ensure_size (&cnt->redirect, cnt->request.len + 1);
		cherokee_buffer_add_buffer (&cnt->redirect, &cnt->request);
		cherokee_buffer_add (&cnt->redirect, "/", 1);

		cnt->error_code = http_moved_permanently;
		return ret_error;
	}

	/* Set the refereces
	 */
	cnt->realm_ref = config_entry->auth_realm;
	cnt->auth_type = config_entry->authentication;

	return ret_ok;
}


ret_t 
cherokee_connection_get_ext_entry (cherokee_connection_t *cnt, cherokee_exts_table_t *exts, cherokee_config_entry_t *config_entry)
{
	ret_t ret;

	return_if_fail (exts != NULL, ret_error);

	/* Look in the extension table
	 */
	ret = cherokee_exts_table_get (exts, &cnt->request, config_entry);
	if (unlikely (ret == ret_error)) {
		cnt->error_code = http_internal_error;
		return ret_error;
	}

	/* Set the refereces
	 */
	cnt->realm_ref = config_entry->auth_realm;
	cnt->auth_type = config_entry->authentication;

	return ret_ok;
}


ret_t 
cherokee_connection_get_req_entry (cherokee_connection_t *cnt, cherokee_reqs_list_t *reqs, cherokee_config_entry_t *config_entry)
{
	ret_t ret;

	return_if_fail (reqs != NULL, ret_error);

	/* Look in the extension table
	 */
#ifndef CHEROKEE_EMBEDDED
	ret = cherokee_reqs_list_get (reqs, &cnt->request, config_entry, cnt);
#else
	return ret_ok;
#endif
	switch (ret) {
	case ret_not_found:
		break;
	case ret_ok:
		cherokee_buffer_clean (&cnt->web_directory);
		break;
	case ret_error:
		cnt->error_code = http_internal_error;
		return ret_error;
	default:
		SHOULDNT_HAPPEN;
	}	

	/* Set the refereces
	 */
	cnt->realm_ref = config_entry->auth_realm;
	cnt->auth_type = config_entry->authentication;

	return ret;
}


ret_t 
cherokee_connection_check_authentication (cherokee_connection_t *cnt, cherokee_config_entry_t *config_entry)
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
	ret = cherokee_header_get_known (&cnt->header, header_authorization, &ptr, &len);
	if (ret != ret_ok) {
		goto unauthorized;
	}
	
	/* Create the validator object
	 */
	ret = config_entry->validator_new_func ((void **) &cnt->validator, 
						config_entry->validator_properties);
	if (ret != ret_ok) {
		goto error;
	}

	/* Read the header information
	 */
	ret = get_authorization (cnt, config_entry->authentication, cnt->validator, ptr, len);
	if (ret != ret_ok) {
		goto unauthorized;
	}

	/* Check if the user is in the list
	 */
	if (config_entry->users != NULL)
	{
		void *foo;

		if (cherokee_buffer_is_empty (&cnt->validator->user)) {
			goto unauthorized;			
		}

		ret = cherokee_table_get (config_entry->users, cnt->validator->user.buf, &foo);
		if (ret != ret_ok) {
			goto unauthorized;
		}
	}
	
	/* Check if the validator is suitable
	 */
	if ((cnt->validator->support & cnt->req_auth_type) == 0) {
		goto error;
	}	
	
	/* Check the login/password
	 */
	ret = cherokee_validator_check (cnt->validator, cnt);
	
	if (ret != ret_ok) {
		goto unauthorized;
	}

	return ret_ok;

unauthorized:
	cnt->keepalive = 0;
	cnt->error_code = http_unauthorized;
	return ret_error;

error:
	cnt->keepalive = 0;
	cnt->error_code = http_internal_error;
	return ret_error;
}


ret_t 
cherokee_connection_check_ip_validation (cherokee_connection_t *cnt, cherokee_config_entry_t *config_entry)
{
	ret_t ret;

	if (config_entry->access == NULL) {
		return ret_ok;
	}

	ret = cherokee_access_ip_match (config_entry->access, &cnt->socket);
	if (ret == ret_ok) {
		return ret_ok;
	}

	cnt->error_code = http_access_denied;
	return ret_error;
}


ret_t 
cherokee_connection_check_only_secure (cherokee_connection_t *cnt, cherokee_config_entry_t *config_entry)
{
	if (config_entry->only_secure == false) {
		/* No Only-Secure connection..
		 */
		return ret_ok;
	}

	if (cnt->socket.is_tls == TLS) {
		/* It is secure
		 */
		return ret_ok;
	}

	cnt->error_code = http_upgrade_required;
	cnt->upgrade    = http_upgrade_tls10;
	return ret_error;
}


ret_t 
cherokee_connection_check_http_method (cherokee_connection_t *cnt, cherokee_config_entry_t *config_entry)
{
	if (config_entry->handler_methods & cnt->header.method)
		return ret_ok;

	cnt->error_code = http_method_not_allowed;
	return ret_error;
}


ret_t 
cherokee_connection_create_handler (cherokee_connection_t *cnt, cherokee_config_entry_t *config_entry)
{
	ret_t ret;

	return_if_fail (config_entry->handler_new_func != NULL, ret_error);

	/* Create and assign a handler object
	 */
	ret = (config_entry->handler_new_func) ((void **)&cnt->handler, cnt, config_entry->handler_properties);
	if (ret == ret_eagain) return ret_eagain;
	if (ret != ret_ok) {
		if ((cnt->handler == NULL) && (cnt->error_code == http_ok)) {
			cnt->error_code = http_internal_error;
		}
		return ret_error;
	}

	return ret_ok;
}


ret_t
cherokee_connection_parse_header (cherokee_connection_t *cnt, cherokee_encoder_table_t *encoders)
{
	ret_t    ret;
	char    *ptr;
	cuint_t  ptr_len;

	/* Look for "Connection: Keep-Alive / Close"
	 */
	ret = cherokee_header_get_known (&cnt->header, header_connection, &ptr, &ptr_len);
	if (ret == ret_ok) 
	{
		if (strncasecmp (ptr, "close", 5) == 0) {
			cnt->keepalive = 0;
		}

	} else {
		cnt->keepalive = 0;
	}

	/* Look for "Range:" 
	 */
	if (HANDLER_SUPPORT_RANGE(cnt->handler)) {
		ret = cherokee_header_get_known (&cnt->header, header_range, &ptr, &ptr_len);
		if (ret == ret_ok) {
			if (strncmp (ptr, "bytes=", 6) == 0) {
				ret = get_range (cnt, ptr+6, ptr_len-6);
				if (ret < ret_ok) {
					cnt->error_code = http_range_not_satisfiable;
					return ret;
				}
			}
		}
	}

	/* Look for "Accept-Encoding:"
	 */
	ret = cherokee_header_get_known (&cnt->header, header_accept_encoding, &ptr, &ptr_len);
	if (ret == ret_ok) {
		ret = get_encoding (cnt, ptr, encoders);
		if (ret < ret_ok) {
			return ret;
		}
	}

	return ret_ok;
}


ret_t 
cherokee_connection_parse_args (cherokee_connection_t *cnt)
{
	ret_t ret;

	/* Sanity check
	 */
	return_if_fail (cnt->arguments == NULL, ret_error);

	/* Build a new table 
	 */
	ret = cherokee_table_new (&cnt->arguments);
	if (unlikely(ret < ret_ok)) return ret;

	/* Parse the header
	 */
	ret = cherokee_header_get_arguments (&cnt->header, &cnt->query_string, cnt->arguments);
	if (unlikely(ret < ret_ok)) return ret;

	return ret_ok;
}


ret_t
cherokee_connection_open_request (cherokee_connection_t *cnt)
{	
	TRACE (ENTRIES, "web_directory='%s' request='%s' local_directory='%s'\n", 
	       cnt->web_directory.buf,
	       cnt->request.buf,
	       cnt->local_directory.buf);

	/* If the connection is keep-alive, have a look at the
	 * handler to see if supports it.
	 */
	if ((HANDLER_SUPPORT_LENGTH(cnt->handler) == 0) && 
	    (HANDLER_SUPPORT_MAYBE_LENGTH(cnt->handler) == 0)) {
		cnt->keepalive = 0;
	}
	
	/* Ensure the space for writting
	 */
	cherokee_buffer_ensure_size (&cnt->buffer, DEFAULT_READ_SIZE+1);

	return cherokee_handler_init (cnt->handler);
}


ret_t 
cherokee_connection_log_or_delay (cherokee_connection_t *conn)
{
	ret_t ret = ret_ok;
	
	if (conn->handler == NULL) {
		conn->log_at_end = 0;
	} else {
		conn->log_at_end = ! HANDLER_SUPPORT_LENGTH(conn->handler);
	}

	if (conn->log_at_end == 0) {
		if (conn->logger_ref != NULL) {
			if (http_type_400(conn->error_code) ||
			    http_type_500(conn->error_code)) 
			{
				ret = cherokee_logger_write_error (conn->logger_ref, conn);				
			} else {
				ret = cherokee_logger_write_access (conn->logger_ref, conn);
			}
			conn->log_at_end = 0;
		}
	}

	return ret;
}


ret_t 
cherokee_connection_log_delayed (cherokee_connection_t *conn)
{
	ret_t ret;

	if ((conn->log_at_end) && (conn->logger_ref)) {
		ret = cherokee_logger_write_access (conn->logger_ref, conn);
		conn->log_at_end = false;
	}

	return ret;
}


ret_t 
cherokee_connection_update_vhost_traffic (cherokee_connection_t *cnt)
{
	/* Update the virtual server traffic counters
	 */
	cherokee_virtual_server_add_rx (CONN_VSRV(cnt), cnt->rx_partial);
	cherokee_virtual_server_add_tx (CONN_VSRV(cnt), cnt->tx_partial);	

	/* Reset partial counters
	 */
	cnt->rx_partial = 0;
	cnt->tx_partial = 0;

	/* Update the time for the next update
	 */
	cnt->traffic_next += DEFAULT_TRAFFIC_UPDATE;

	return ret_ok;
}


ret_t 
cherokee_connection_clean_for_respin (cherokee_connection_t *cnt)
{
	cherokee_buffer_clean (&cnt->web_directory);

	return ret_ok;
}
