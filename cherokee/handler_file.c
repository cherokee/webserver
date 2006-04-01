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

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>

#include "server.h"
#include "server-protected.h"
#include "tdate_parse.h"
#include "mime.h"
#include "header.h"
#include "header-protected.h"
#include "handler_file.h"
#include "connection.h"
#include "connection-protected.h"
#include "module.h"
#include "iocache.h"
#include "util.h"


cherokee_module_info_handler_t MODULE_INFO(file) = {
	.module.type     = cherokee_handler,            /* type         */
	.module.new_func = cherokee_handler_file_new,   /* new func     */
	.valid_methods   = http_get | http_head         /* http methods */
};

ret_t
cherokee_handler_file_new  (cherokee_handler_t **hdl, cherokee_connection_t *cnt, cherokee_table_t *properties)
{
	CHEROKEE_NEW_STRUCT (n, handler_file);
	
	/* Init the base class object
	 */
	cherokee_handler_init_base (HANDLER(n), cnt);

	MODULE(n)->free         = (module_func_free_t) cherokee_handler_file_free;
	MODULE(n)->get_name     = (module_func_get_name_t) cherokee_handler_file_get_name;
	MODULE(n)->init         = (handler_func_init_t) cherokee_handler_file_init;
	HANDLER(n)->step        = (handler_func_step_t) cherokee_handler_file_step;
	HANDLER(n)->add_headers = (handler_func_add_headers_t) cherokee_handler_file_add_headers;

	/* Supported features
	 */
	HANDLER(n)->support     = hsupport_length | hsupport_range;

	/* Init
	 */
	n->fd             = -1;
	n->offset         = 0;
	n->mime           = NULL;
	n->using_sendfile = false;
	n->info           = NULL;

#ifdef CHEROKEE_EMBEDDED
	n->use_cache      = true;
#else
	n->use_cache      = false;
#endif

	/* Check some properties
	 */
	if (properties != NULL) {
		cherokee_typed_table_get_int (properties, "cache", &n->use_cache);
	}

	/* Return the object
	 */
	*hdl = HANDLER(n);
	return ret_ok;
}


ret_t
cherokee_handler_file_free (cherokee_handler_file_t *fhdl)
{
	if (fhdl->fd != -1) {
		close (fhdl->fd);
		fhdl->fd = -1;
	}

	return ret_ok;
}


void  
cherokee_handler_file_get_name (cherokee_handler_file_t *module, const char **name)
{
	*name = "file";
}


static ret_t
check_cached (cherokee_handler_file_t *n)
{
	ret_t                  ret;
	char                  *header;
	cuint_t                header_len;
	cherokee_connection_t *conn = HANDLER_CONN(n);

	/* Based in time
	 */
	ret = cherokee_header_get_known (&conn->header, header_if_modified_since, &header, &header_len);
	if (ret == ret_ok)  {
		time_t req_time;
		char   tmp;
		char  *end = header + header_len;

		tmp = *end;    /* save */
		*end = '\0';   /* set  */

		req_time = tdate_parse (header);			
		if (req_time == -1) {
			cherokee_logger_write_string (
				CONN_VSRV(conn)->logger, 
				"Warning: Unparseable time '%s'",
				header);
		}
		*end = tmp;   /* restore */
		
		if (req_time == -1) {
			return ret_ok;
		}
		
		/* The file is cached in the client
		 */
		if (n->info->st_mtime <= req_time) {
			conn->error_code = http_not_modified;
			return ret_error;	
		}
	}
	
	/* HTTP/1.1 only headers
	 */
	if (conn->header.version < http_version_11)
		return ret_ok;

	/* Based in ETag
	 */
	ret = cherokee_header_get_known (&conn->header, header_if_none_match, &header, &header_len);
	if (ret == ret_ok)  {
		int    tmp_len;
		CHEROKEE_TEMP(tmp,100);
		
		tmp_len = snprintf (tmp, tmp_size, "%lx=" FMT_OFFSET "x", n->info->st_mtime, n->info->st_size);
		
		if (strncmp (header, tmp, tmp_len) == 0) {
			conn->error_code = http_not_modified;
			return ret_error;				
		}
	}
	
	/* If-Range
	 */
	ret = cherokee_header_get_known (&conn->header, header_if_range, &header, &header_len);
	if (ret == ret_ok)  {
		time_t req_time;
		char   tmp;
		char  *end = header + header_len;
		
		tmp = *end; 
		*end = '\0'; 
		
		req_time = tdate_parse (header);			
		if (req_time == -1) {
			cherokee_logger_write_string (
				CONN_VSRV(conn)->logger, 
				"Warning: Unparseable time '%s'",
				header);
		}
		*end = tmp;
		
		if (req_time == -1) {
			return ret_ok;
		}

		/* If the entity tag given in the If-Range header
		 * matches the current entity tag for the entity, then
		 * the server SHOULD provide the specified sub-range
		 * of the entity using a 206 (Partial content)
		 * response. If the entity tag does not match, then
		 * the server SHOULD return the entire entity using a
		 * 200 (OK) response.
		 */
		if (n->info->st_mtime > req_time) {
			conn->error_code = http_ok;

			conn->range_start = 0;
			conn->range_end = 0;
		}
	}

	return ret_ok;
}


static ret_t 
open_local_directory (cherokee_handler_file_t *n, cherokee_connection_t *conn)
{
	/* Check if it is already open
	 */
	if (n->fd > 0)
		return ret_ok;

	/* Open it
	 */
	n->fd = open (conn->local_directory.buf, CHE_O_READ);
	if (n->fd > 0) return ret_ok;

	/* Manage errors
	 */
	switch (errno) {
	case EACCES:
		conn->error_code = http_access_denied;
		break;
	case ENOENT:
		conn->error_code = http_not_found;
		break;
	default:
		conn->error_code = http_internal_error;
	}
	return ret_error;
}


static ret_t
stat_local_directory (cherokee_handler_file_t *n, cherokee_connection_t *conn, cherokee_iocache_entry_t **io_entry, struct stat **info)
{	
	int                re;
	ret_t              ret;
	cherokee_server_t *srv = CONN_SRV(conn);

	/* Without cache
	 */
	if (! n->use_cache) {
		re = stat (conn->local_directory.buf, &n->cache_info);
		if (re < 0) {
			switch (errno) {
			case ENOENT: 
				conn->error_code = http_not_found;
				break;
			case EACCES: 
				conn->error_code = http_access_denied;
				break;
			default:     
				conn->error_code = http_internal_error;
			}

			cherokee_buffer_drop_endding (&conn->local_directory, conn->request.len);
			return ret_error;
		}

		*info = &n->cache_info;		
		return ret_ok;
	} 

	/* I/O cache
	 */
#ifndef CHEROKEE_EMBEDDED
	ret = cherokee_iocache_stat_get (srv->iocache, conn->local_directory.buf, io_entry);
	if (ret != ret_ok) {
		switch (ret) {
		case ret_not_found:
			conn->error_code = http_not_found;
			break;
		case ret_deny:
			conn->error_code = http_access_denied;
			break;
		default:
			conn->error_code = http_internal_error;
		}
		
		cherokee_buffer_drop_endding (&conn->local_directory, conn->request.len);
		return ret_error;		
	}

	*info = &(*io_entry)->state;
	return ret_ok;
#endif 

	return ret_error;
}


ret_t 
cherokee_handler_file_init (cherokee_handler_file_t *n)
{
	int   ret;
	char *ext;
	cherokee_boolean_t        use_io   = false;
	cherokee_iocache_entry_t *io_entry = NULL;
	cherokee_connection_t    *conn     = HANDLER_CONN(n);
	cherokee_server_t        *srv      = HANDLER_SRV(n);

	/* Build the local file path                         1.- BUILD
	 * Take care with "return"s until 2.
	 */
	if (conn->request.len > 1) {
		cherokee_buffer_add (&conn->local_directory, conn->request.buf+1, conn->request.len-1); 
	}

	/* Query the I/O cache
	 */
	ret = stat_local_directory (n, conn, &io_entry, &n->info);
	if (ret != ret_ok) return ret;

	/* Ensure it is a file
	 */
	if (S_ISDIR(n->info->st_mode)) {
		conn->error_code = http_access_denied;
		return ret_error;
	}

	/* Is it cached on the client?
	 */
	ret = check_cached(n);
	if (ret != ret_ok) {
		cherokee_buffer_drop_endding (&conn->local_directory, conn->request.len);
		return ret;
	}

	/* Is this file cached in the io cache?
	 */
#ifndef CHEROKEE_EMBEDDED
	use_io = ((!n->use_cache) &&
		  (conn->encoder == NULL) &&
		  (conn->socket.is_tls == non_TLS) &&
		  (n->info->st_size <= IOCACHE_MAX_FILE_SIZE) &&
		  (http_method_with_body (conn->header.method)));
	
	if (use_io) {
		ret = cherokee_iocache_mmap_lookup (srv->iocache,
						    conn->local_directory.buf,
						    &io_entry);
		if (ret != ret_ok) {
			ret = open_local_directory (n, conn);
			if (ret != ret_ok) {
				cherokee_buffer_drop_endding (&conn->local_directory, conn->request.len);
				return ret;
			}

			ret = cherokee_iocache_mmap_get_w_fd (srv->iocache,
							      conn->local_directory.buf,
							      n->fd,
							      &io_entry);
		}

		if (ret == ret_ok) {
			conn->io_entry_ref = io_entry;
		}
	}
#endif

	/* Maybe open the file
	 */
	if ((n->fd < 0) && (!use_io)) {
		ret = open_local_directory (n, conn);
		if (ret != ret_ok) {
			cherokee_buffer_drop_endding (&conn->local_directory, conn->request.len);
			return ret;
		}
	}

	/* Undo the local file path                          2.- UNDO
	 * "return" is allowed again
	 */
	cherokee_buffer_drop_endding (&conn->local_directory, conn->request.len);

	/* Is it a directory?
	 */
	if (S_ISDIR(n->info->st_mode)) {
		conn->error_code = http_access_denied;
		return ret_error;		
	}

	/* Range 1: Check the range and file size
	 */
	if ((conn->range_start > n->info->st_size) ||
	    (conn->range_end   > n->info->st_size)) 
	{
		conn->error_code = http_range_not_satisfiable;
		return ret_error;
	}

	/* Set the error code
	 */
	if (conn->range_start != 0 || conn->range_end != 0) {
		conn->error_code = http_partial_content;
	}

	/* Range 2: Set the file length as the range end
	 */
	if (conn->range_end == 0) {
		conn->range_end = n->info->st_size;
	} 

	/* Set mmap or file position
	 */
	if (conn->io_entry_ref != NULL) {
		/* Set the mmap info
		 */
		conn->mmaped     = conn->io_entry_ref->mmaped +  conn->range_start;
		conn->mmaped_len = conn->io_entry_ref->mmaped_len - (
			conn->range_start + (conn->io_entry_ref->mmaped_len - conn->range_end));
	} else {
		/* Seek the file is needed
		 */
		if ((conn->range_start != 0) && (conn->mmaped == NULL)) {
			n->offset = conn->range_start;
			lseek (n->fd, n->offset, SEEK_SET);
		}
	}

	/* Look for the mime type
	 */
#ifndef CHEROKEE_EMBEDDED
	if (srv->mime != NULL) {
		ext = strrchr (conn->request.buf, '.');
		if (ext != NULL) {
			ret = cherokee_mime_get_by_suffix (srv->mime, ext+1, &n->mime);
		}
	}
#endif

	/* Maybe use sendfile
	 */
#ifdef HAVE_SENDFILE
	n->using_sendfile = ((conn->mmaped == NULL) &&
			     (conn->encoder == NULL) &&
			     (n->info->st_size >= srv->sendfile.min) && 
			     (n->info->st_size <  srv->sendfile.max) &&
			     (conn->socket.is_tls == non_TLS));

# ifdef HAVE_SENDFILE_BROKEN
	n->using_sendfile = false;
# endif

	if (n->using_sendfile) {
		cherokee_connection_set_cork(conn, 1);
	}
#endif
	
	return ret_ok;
}


ret_t
cherokee_handler_file_step (cherokee_handler_file_t *fhdl, 
			    cherokee_buffer_t       *buffer)
{
	off_t                  total;
	int                    size;
	cherokee_connection_t *conn = HANDLER_CONN(fhdl);

#if HAVE_SENDFILE
	if (fhdl->using_sendfile) {
		ret_t   ret;
		ssize_t sent;
		
		ret = cherokee_socket_sendfile (&conn->socket,                     /* cherokee_socket_t *socket */
						fhdl->fd,                          /* int                fd     */
						conn->range_end - fhdl->offset,    /* size_t             size   */
						&fhdl->offset,                     /* off_t             *offset */
						&sent);                            /* ssize_t           *sent   */

		if (ret == ret_no_sys) {
			fhdl->using_sendfile = false;
			goto exit_sendfile;
		}

		/* cherokee_handler_file_init() activated the TCP_CORK flags.
		 * After it, the header was sent.  And now, the first
		 * chunk of the file with sendfile().  It's time to turn
		 * off again the TCP_CORK flag
		 */
		if (conn->tcp_cork) {
			cherokee_connection_set_cork (conn, 0);
		}

		if (ret < ret_ok) return ret;

		/* This connection is not using the cherokee_connection_send() method,
		 * so we have to update the connection traffic counter here.
		 */
		cherokee_connection_tx_add (conn, sent);

		if (fhdl->offset >= conn->range_end) {
			return ret_eof;
		}

		return ret_ok_and_sent;
	}

exit_sendfile:
#endif

	/* Check the amount to read
	 */
	if ((fhdl->offset + buffer->size) > conn->range_end) {
		size = conn->range_end - fhdl->offset;
	} else {
		size = buffer->size;
	}

	/* Read
	 */
	total = read (fhdl->fd, buffer->buf, size);	
	switch (total) {
	case 0:
		return ret_eof;
	case -1:
		return ret_error;
	default:
		buffer->len = total;
		fhdl->offset += total;
	}	

	/* Maybe it was the last file chunk
	 */
	if (fhdl->offset >= HANDLER_CONN(fhdl)->range_end) {
		return ret_eof_have_data;
	}

	return ret_ok;
}


ret_t
cherokee_handler_file_add_headers (cherokee_handler_file_t *fhdl,
				   cherokee_buffer_t       *buffer)
{
	ret_t     ret;
	off_t     length;
	struct tm modified_tm;

	/* We stat()'ed the file in the handler constructor
	 */
	length = HANDLER_CONN(fhdl)->range_end - HANDLER_CONN(fhdl)->range_start;		
	if (length < 0) {
		length = 0;
	}

	if (HANDLER_CONN(fhdl)->encoder == NULL) {
		if(HANDLER_CONN(fhdl)->error_code == http_partial_content) {
			cherokee_buffer_add_va (buffer,
						"Content-Range: bytes " FMT_OFFSET "-"
						FMT_OFFSET "/" FMT_OFFSET CRLF,
						HANDLER_CONN(fhdl)->range_start,
						HANDLER_CONN(fhdl)->range_end - 1,
						fhdl->info->st_size);
		}
		
		cherokee_buffer_add_va (buffer, "Content-Length: " FMT_OFFSET CRLF, length);

	} else {
		/* Can't use Keep-alive w/o "Content-length:", so
		 */
		HANDLER_CONN(fhdl)->keepalive = 0;
	}

	/* Add MIME related headers: 
	 * "Content-Type:" and "Cache-Control: max-age="
	 */
#ifndef CHEROKEE_EMBEDDED
	if (fhdl->mime != NULL) {
		cherokee_buffer_t *mime;
		cuint_t            maxage;

		cherokee_mime_entry_get_type (fhdl->mime, &mime);
		cherokee_buffer_add_str (buffer, "Content-Type: ");
		cherokee_buffer_add_buffer (buffer, mime);
		cherokee_buffer_add_str (buffer, CRLF);

		ret = cherokee_mime_entry_get_maxage (fhdl->mime, &maxage);		
		if (ret == ret_ok) {
			cherokee_buffer_add_va (buffer, "Cache-Control: max-age=%d"CRLF, maxage);
		}
	}
#endif

	/* Etag
	 */
	if (HANDLER_CONN(fhdl)->header.version >= http_version_11) { 
		cherokee_buffer_add_va (buffer, "Etag: %lx=%lx"CRLF, fhdl->info->st_mtime, fhdl->info->st_size);
	}

	/* Last-Modified:
	 */
	cherokee_gmtime (&fhdl->info->st_mtime, &modified_tm);

	cherokee_buffer_add_va (buffer, "Last-Modified: %s, %02d %s %d %02d:%02d:%02d GMT"CRLF,
				cherokee_weekdays[modified_tm.tm_wday],
				modified_tm.tm_mday,
				cherokee_months[modified_tm.tm_mon], 
				modified_tm.tm_year + 1900,
				modified_tm.tm_hour,
				modified_tm.tm_min,
				modified_tm.tm_sec);

	return ret_ok;
}



/*   Library init function
 */
static cherokee_boolean_t _file_is_init = false;

void
MODULE_INIT(file) (cherokee_module_loader_t *loader)
{
	/* Init flag
	 */
	if (_file_is_init == true) {
		return;
	}
	_file_is_init = true;

	/* Init something more..
	 */
}
