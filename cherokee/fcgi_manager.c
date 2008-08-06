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
#include "fcgi_manager.h"
#include "util.h"
#include "fastcgi.h"
#include "connection-protected.h"
#include "handler_fastcgi.h"
#include "source_interpreter.h"

#include <unistd.h>


#define ENTRIES "manager,cgi"
#define CONN_STEP 10 


ret_t
cherokee_fcgi_manager_init (cherokee_fcgi_manager_t *mgr,
			    void                    *dispatcher,
			    cherokee_source_t       *src, 
			    cherokee_boolean_t       keepalive, 
			    cuint_t                  pipeline)
{
	cuint_t i;
	
	cherokee_socket_init (&mgr->socket);
	cherokee_buffer_init (&mgr->read_buffer);

	mgr->dispatcher    = dispatcher;
	mgr->keepalive     = keepalive;
	mgr->pipeline      = pipeline;
	mgr->generation    = 0;
	mgr->source        = src;
	mgr->first_connect = true;

	mgr->conn.len      = 0;
	mgr->conn.size     = CONN_STEP;

	mgr->conn.id2conn = (conn_entry_t *) malloc (CONN_STEP * sizeof (conn_entry_t));
	if (unlikely(mgr->conn.id2conn == NULL)) return ret_nomem;

	for (i=0; i<CONN_STEP; i++) {
		mgr->conn.id2conn[i].conn = NULL;
		mgr->conn.id2conn[i].eof  = true;		
	}

	TRACE (ENTRIES, "keepalive=%d pipeline=%d conn_entries=%d\n", keepalive, pipeline, mgr->conn.size);
	return ret_ok;
}


ret_t
cherokee_fcgi_manager_mrproper (cherokee_fcgi_manager_t *mgr)
{
	cherokee_socket_close (&mgr->socket);
	cherokee_socket_mrproper (&mgr->socket);

	cherokee_buffer_mrproper (&mgr->read_buffer);

	if (mgr->conn.id2conn != NULL) {
		free (mgr->conn.id2conn);
		mgr->conn.id2conn = NULL;
	}

	return ret_ok;
}


static void
update_conn_list_length (cherokee_fcgi_manager_t *mgr, cuint_t id)
{
	if (mgr->conn.id2conn[id].conn == NULL) {
		/* There is room of one connection, notify it
		 */
		cherokee_fcgi_dispatcher_end_notif (FCGI_DISPATCHER(mgr->dispatcher));
		mgr->conn.len--;
	}

/*	printf ("mgr->conn.id2conn[id].conn %p, mgr->conn.id2conn[id].eof %d -- use=%d\n",
	mgr->conn.id2conn[id].conn, mgr->conn.id2conn[id].eof, mgr->conn.len); */
}


static void
reset_connections (cherokee_fcgi_manager_t *mgr)
{
	cuint_t i;

	for (i=1; i<mgr->conn.size; i++) {
		cherokee_handler_cgi_base_t *cgi;
		
		if (mgr->conn.id2conn[i].conn == NULL)
			continue;
			
		cgi = HDL_CGI_BASE(mgr->conn.id2conn[i].conn->handler);

		if (mgr->generation != HDL_FASTCGI(cgi)->generation) {
			continue;
		}
		
		cgi->got_eof = true;
		mgr->conn.id2conn[i].conn = NULL;
		mgr->conn.id2conn[i].eof  = true;
		mgr->conn.len--;
	}
}


static ret_t 
reconnect (cherokee_fcgi_manager_t *mgr, cherokee_thread_t *thd, cherokee_boolean_t clean_up)
{
	ret_t              ret;
	cuint_t            next;
	cuint_t            try = 0;
	cherokee_source_t *src = mgr->source;

	/* Do some clean up
	 */
	if (clean_up) {
		TRACE (ENTRIES, "Cleaning up before reconecting %s", "\n");

		cherokee_thread_close_polling_connections (thd, SOCKET_FD(&mgr->socket), NULL);

		reset_connections (mgr);
		cherokee_buffer_clean (&mgr->read_buffer);
	
		/* Set the new generation number
		 */
		next = mgr->generation;
		mgr->generation = ++next % 255;

		/* Close	
		 */
		cherokee_socket_close (&mgr->socket);
	}
	
	/* If it connects we're done here..
	 */
	ret = cherokee_source_connect (src, &mgr->socket);
	if (ret != ret_ok) {
		cherokee_source_interpreter_t *src_int = SOURCE_INT(src);

		/* It didn't sucess to connect, so lets spawn a new server
		 */
		ret = cherokee_source_interpreter_spawn (src_int);
		if (ret != ret_ok) {
			if (src_int->interpreter.buf)
				TRACE (ENTRIES, "Couldn't spawn: %s\n", src_int->interpreter.buf);
			else
				TRACE (ENTRIES, "There was no interpreter to be spawned %s", "\n");
			return ret_error;
		}
		
		while (true) {
			/* Try to connect again	
			 */
			ret = cherokee_source_connect (src, &mgr->socket);
			if (ret == ret_ok) break;

			TRACE (ENTRIES, "Couldn't connect: %s, try %d\n", src->host.buf ? src->host.buf : src->unix_socket.buf, try);

			if (try++ >= 3)
				return ret;

			sleep (1);
		}
	}

	TRACE (ENTRIES, "Connected sucessfully try=%d, fd=%d\n", try, mgr->socket.socket);

	cherokee_fd_set_nonblocking (SOCKET_FD(&mgr->socket), true);
	return ret_ok;
}


ret_t 
cherokee_fcgi_manager_ensure_is_connected (cherokee_fcgi_manager_t *mgr, cherokee_thread_t *thd)
{
	ret_t              ret;
	cherokee_socket_t *socket = &mgr->socket;

	if (socket->status == socket_closed) {
		TRACE (ENTRIES, "mgr=%p reconnecting\n", mgr);
		
		ret = reconnect (mgr, thd, !mgr->first_connect);
		if (ret != ret_ok) return ret;

 		if (mgr->first_connect) 
 			mgr->first_connect = false; 
	}
	
	return ret_ok;
}


static ret_t
process_package (cherokee_fcgi_manager_t *mgr, cherokee_buffer_t *inbuf)
{
	FCGI_Header                *header;
	FCGI_EndRequestBody        *ending;
	cherokee_buffer_t          *outbuf;
	cherokee_connection_t      *conn;
	cherokee_handler_fastcgi_t *hdl;

	cuint_t  len;
	char    *data;
	cint_t   return_val;
	cuint_t  type;
	cuint_t  id;
	cuint_t  padding;
		
	/* Is there enough information?
	 */
	if (inbuf->len < sizeof(FCGI_Header))
		return ret_ok;

	/* At least there is a header
	 */
	header = (FCGI_Header *)inbuf->buf;

	if (header->version != 1) {
		cherokee_buffer_print_debug (inbuf, -1);
		PRINT_ERROR_S ("Parsing error: unknown version\n");
		return ret_error;
	}
	
	if (header->type != FCGI_STDERR &&
	    header->type != FCGI_STDOUT && 
	    header->type != FCGI_END_REQUEST)
	{
		cherokee_buffer_print_debug (inbuf, -1);
		PRINT_ERROR_S ("Parsing error: unknown type\n");
		return ret_error;
	}
	
	/* Read the header
	 */
	type    =  header->type;
	padding =  header->paddingLength;
	id      = (header->requestIdB0     | (header->requestIdB1 << 8));
	len     = (header->contentLengthB0 | (header->contentLengthB1 << 8));

	/* Is the package complete?
	 */
	if (len > inbuf->len - (FCGI_HEADER_LEN + padding))
		return ret_ok;

	/* Locate the connection
	 */
	conn = mgr->conn.id2conn[id].conn;
	if (conn == NULL) {
		if (mgr->conn.id2conn[id].eof)
			goto error;

		goto go_out;
	}

	hdl      = HDL_FASTCGI(conn->handler);
	outbuf   = &HDL_CGI_BASE(hdl)->data;
	data     = inbuf->buf +  FCGI_HEADER_LEN;

	/* It has received the full package content
	 */
	switch (type) {
	case FCGI_STDERR: 
		if (CONN_VSRV(conn)->logger != NULL) {
			cherokee_buffer_t tmp = CHEROKEE_BUF_INIT;
			
			cherokee_buffer_add (&tmp, data, len);
			cherokee_logger_write_string (CONN_VSRV(conn)->logger, "%s", tmp.buf);
			cherokee_buffer_mrproper (&tmp);		
		}
		exit(1);
		break;

	case FCGI_STDOUT:
/* 		printf ("READ:STDOUT id=%d gen=%d eof=%d (%s): %d", id, hdl->generation, CGI_BASE(hdl)->got_eof, conn->query_string.buf, len); */
		cherokee_buffer_add (outbuf, data, len);
		break;

	case FCGI_END_REQUEST: 
		ending = (FCGI_EndRequestBody *)data;

		return_val = ((ending->appStatusB0)       | 
			      (ending->appStatusB0 << 8)  | 
			      (ending->appStatusB0 << 16) | 
			      (ending->appStatusB0 << 24));

		HDL_CGI_BASE(hdl)->got_eof    = true;
		mgr->conn.id2conn[id].eof = true;

		update_conn_list_length (mgr, id);

/* 		printf ("READ:END id=%d gen=%d", id, hdl->generation); */
		break;

	default:
		SHOULDNT_HAPPEN;
	}

go_out:
	cherokee_buffer_move_to_begin (inbuf, len + FCGI_HEADER_LEN + padding);
/* 	printf ("- FCGI quedan %d\n", inbuf->len); */
	return ret_eagain;

error:
	cherokee_buffer_move_to_begin (inbuf, len + FCGI_HEADER_LEN + padding);
	return ret_error;
}


static ret_t
process_buffer (cherokee_fcgi_manager_t *mgr)
{
	ret_t ret;
	
	do {
		ret = process_package (mgr, &mgr->read_buffer);
	} while (ret == ret_eagain);

	/* ok, error
	 */
	return ret;
}


ret_t 
cherokee_fcgi_manager_register (cherokee_fcgi_manager_t *mgr, cherokee_connection_t *conn, cuint_t *id, cuchar_t *gen)
{
	cuint_t            slot;
	cherokee_boolean_t found = false;

	/* Look for a available slot
	 */
	for (slot = 1; slot < mgr->conn.size; slot++) {
		if ((mgr->conn.id2conn[slot].eof) &&
		    (mgr->conn.id2conn[slot].conn == NULL)) 
		{
			found = true;
			break;
		}
	}
	
	/* Do we need more memory for the index?
	 */
	if (found == false) {
		cuint_t i;

		mgr->conn.id2conn = (conn_entry_t *) realloc (
			mgr->conn.id2conn, (mgr->conn.size + CONN_STEP) * sizeof(conn_entry_t));

                if (unlikely (mgr->conn.id2conn == NULL)) 
			return ret_nomem;

		for (i = 0; i < CONN_STEP; i++) {
			mgr->conn.id2conn[i + mgr->conn.size].eof  = true;
			mgr->conn.id2conn[i + mgr->conn.size].conn = NULL;			
		}
		
		slot = mgr->conn.size;
		mgr->conn.size += CONN_STEP;
	}

	/* Register the connection
	 */
	mgr->conn.id2conn[slot].conn = conn;
	mgr->conn.id2conn[slot].eof  = false;
	mgr->conn.len++;

	*gen = mgr->generation;
	*id  = slot;
	
	TRACE (ENTRIES, "registered id=%d (gen=%d)\n", slot, mgr->generation);
	return ret_ok;
}


ret_t 
cherokee_fcgi_manager_unregister (cherokee_fcgi_manager_t *mgr, cherokee_connection_t *conn)
{
	cherokee_handler_fastcgi_t *hdl = HDL_FASTCGI(conn->handler);

	if (hdl->generation != mgr->generation) {
		TRACE (ENTRIES, "Unregister: Different generation id=%d gen=%d, mgr=%d\n",
		       hdl->id, mgr->generation, mgr->generation);
		return ret_ok;
	}
 	
	if (mgr->conn.id2conn[hdl->id].conn != conn) {
		SHOULDNT_HAPPEN;
		return ret_error;
	}
 	
	TRACE (ENTRIES, "UNregistered id=%d (gen=%d)\n", hdl->id, mgr->generation);

	if (! mgr->keepalive) {
		cherokee_socket_close (&mgr->socket);
		cherokee_socket_clean (&mgr->socket);
	}

	mgr->conn.id2conn[hdl->id].conn = NULL;       
	update_conn_list_length (mgr, hdl->id);

	return ret_ok;
}


ret_t 
cherokee_fcgi_manager_step (cherokee_fcgi_manager_t *mgr)
{
	ret_t  ret;
	size_t size = 0; 

	if (mgr->read_buffer.len < sizeof(FCGI_Header)) {
		ret = cherokee_socket_bufread (&mgr->socket, &mgr->read_buffer, DEFAULT_READ_SIZE, &size);
		switch (ret) {
		case ret_ok:
			TRACE (ENTRIES, "%d bytes read\n", size);
			break;

		case ret_eof:
			TRACE (ENTRIES, "%s\n", "EOF");
			return ret_eof;

		case ret_error:
		case ret_eagain:
			return ret;

		default:
			RET_UNKNOWN(ret);
			return ret_error;	
		}
	}

	/* Process the new chunk
	 */
	ret = process_buffer (mgr);
	if (unlikely (ret != ret_ok)) return ret;

	return ret_ok;
}


ret_t 
cherokee_fcgi_manager_send_remove (cherokee_fcgi_manager_t *mgr, cherokee_buffer_t *buf)
{
	ret_t  ret;
	size_t written = 0;
	
	ret = cherokee_socket_bufwrite (&mgr->socket, buf, &written);
	switch (ret) {
	case ret_ok:
		TRACE (ENTRIES, "Sent %db\n", written);
		cherokee_buffer_move_to_begin (buf, written);
		return ret_ok;
	case ret_error:
		return ret;
	case ret_eof:
	case ret_eagain:
		return ret;
	default:
		RET_UNKNOWN(ret);
		return ret;
	}
}


char
cherokee_fcgi_manager_supports_pipelining (cherokee_fcgi_manager_t *mgr)
{
	return (mgr->pipeline > 0);
}
