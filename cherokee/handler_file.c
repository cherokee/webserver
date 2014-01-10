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

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>

#include "server.h"
#include "server-protected.h"
#include "dtm.h"
#include "mime.h"
#include "header.h"
#include "header-protected.h"
#include "handler_file.h"
#include "connection.h"
#include "connection-protected.h"
#include "module.h"
#include "iocache.h"
#include "util.h"
#include "handler_dirlist.h"
#include "error_log.h"

#define ENTRIES "handler,file"


/* Plug-in initialization
 */
PLUGIN_INFO_HANDLER_EASIEST_INIT (file, http_get | http_head | http_options);


/* Methods implementation
 */
ret_t
cherokee_handler_file_props_free (cherokee_handler_file_props_t *props)
{
	return cherokee_handler_props_free_base (HANDLER_PROPS(props));
}


ret_t
cherokee_handler_file_configure (cherokee_config_node_t   *conf,
				 cherokee_server_t        *srv,
				 cherokee_module_props_t **_props)
{
	ret_t                          ret;
	cherokee_list_t               *i;
	cherokee_handler_file_props_t *props;

	UNUSED(srv);

	if (*_props == NULL) {
		CHEROKEE_NEW_STRUCT (n, handler_file_props);

		cherokee_handler_props_init_base (HANDLER_PROPS(n),
						  MODULE_PROPS_FREE(cherokee_handler_file_props_free));

		n->use_cache = true;
		n->send_symlinks = true;
		*_props = MODULE_PROPS(n);
	}

	props = PROP_FILE(*_props);

	cherokee_config_node_foreach (i, conf) {
		cherokee_config_node_t *subconf = CONFIG_NODE(i);

		if (equal_buf_str (&subconf->key, "iocache")) {
			ret = cherokee_atob (subconf->val.buf, &props->use_cache);
			if (ret != ret_ok) return ret;
		} else if (equal_buf_str (&subconf->key, "symlinks")) {
			ret = cherokee_atob (subconf->val.buf, &props->send_symlinks);
			if (ret != ret_ok) return ret;
		}
	}

	return ret_ok;
}


ret_t
cherokee_handler_file_new (cherokee_handler_t     **hdl,
			   cherokee_connection_t   *cnt,
			   cherokee_module_props_t *props)
{
	CHEROKEE_NEW_STRUCT (n, handler_file);

	TRACE_CONN(cnt);

	/* Init the base class object
	 */
	cherokee_handler_init_base (HANDLER(n), cnt, HANDLER_PROPS(props), PLUGIN_INFO_HANDLER_PTR(file));

	MODULE(n)->free         = (module_func_free_t) cherokee_handler_file_free;
	MODULE(n)->init         = (handler_func_init_t) cherokee_handler_file_init;
	HANDLER(n)->step        = (handler_func_step_t) cherokee_handler_file_step;
	HANDLER(n)->add_headers = (handler_func_add_headers_t) cherokee_handler_file_add_headers;

	/* Supported features
	 */
	HANDLER(n)->support     = hsupport_range;

	/* Init
	 */
	n->fd             = -1;
	n->offset         = 0;
	n->mime           = NULL;
	n->info           = NULL;
	n->using_sendfile = false;
	n->not_modified   = false;

	/* Return the object
	 */
	*hdl = HANDLER(n);
	return ret_ok;
}


ret_t
cherokee_handler_file_free (cherokee_handler_file_t *fhdl)
{
	if (fhdl->fd != -1) {
		cherokee_fd_close (fhdl->fd);
		fhdl->fd = -1;
	}

	return ret_ok;
}


static ret_t
check_cached (cherokee_handler_file_t *fhdl)
{
	ret_t                  ret;
	char                  *header;
	cuint_t                header_len;
	cherokee_boolean_t     has_modified_since = false;
	cherokee_boolean_t     has_etag           = false;
	cherokee_boolean_t     not_modified_ms    = false;
	cherokee_boolean_t     not_modified_etag  = false;
	cherokee_connection_t *conn               = HANDLER_CONN(fhdl);
	cherokee_thread_t     *thread             = HANDLER_THREAD(fhdl);
	cherokee_buffer_t     *etag_local         = THREAD_TMP_BUF1(thread);

	cherokee_buffer_clean (etag_local);

	/* Based in time
	 */
	ret = cherokee_header_get_known (&conn->header, header_if_modified_since, &header, &header_len);
	if (ret == ret_ok)  {
		char   tmp;
		time_t req_time = 0;
		char  *end      = header + header_len;

		has_modified_since = true;

		/* Set EOL
		 */
		tmp = *end;
		*end = '\0';

		/* Parse the Date string
		 */
		ret = cherokee_dtm_str2time (header, header_len, &req_time);
		if (unlikely (ret == ret_error)) {
			LOG_WARNING (CHEROKEE_ERROR_HANDLER_FILE_TIME_PARSE, header);

		} else if (likely (ret == ret_ok)) {
			/* The file is cached in the client
			 */
			if (fhdl->info->st_mtime <= req_time) {
				not_modified_ms = true;
			}
		}

		/* Restore EOL
		 */
		*end = tmp;
	}

	/* HTTP/1.1 only headers from now on
	 */
	if (conn->header.version < http_version_11) {
		fhdl->not_modified = not_modified_ms;
		return ret_ok;
	}

	/* Based in ETag
	 */
	ret = cherokee_header_get_known (&conn->header, header_if_none_match, &header, &header_len);
	if (ret == ret_ok)  {
		/* Build local ETag
		 */
		if (cherokee_buffer_is_empty (etag_local)) {
			cherokee_buffer_add_char     (etag_local, '"');
			cherokee_buffer_add_ullong16 (etag_local, (cullong_t) fhdl->info->st_mtime);
			cherokee_buffer_add_str      (etag_local, "=");
			cherokee_buffer_add_ullong16 (etag_local, (cullong_t) fhdl->info->st_size);
			cherokee_buffer_add_char     (etag_local, '"');
		}

		/* Compare ETag(s)
		 */
		if ((header_len == etag_local->len) &&
		    (strncmp (header, etag_local->buf, etag_local->len) == 0))
		{
			not_modified_etag = true;
		}

		has_etag = true;
	}

	/* If-Range
	 */
	ret = cherokee_header_get_known (&conn->header, header_if_range, &header, &header_len);
	if (ret == ret_ok)  {
		char   tmp;
		char  *end = header + header_len;

		/* Set EOL
		 */
		tmp = *end;
		*end = '\0';

		/* If-Range: "<Etag>"
		 */
		if (strchr (header, '=')) {
			/* Build local ETag if needed
			 */
			if (cherokee_buffer_is_empty (etag_local)) {
				cherokee_buffer_add_char     (etag_local, '"');
				cherokee_buffer_add_ullong16 (etag_local, (cullong_t) fhdl->info->st_mtime);
				cherokee_buffer_add_str      (etag_local, "=");
				cherokee_buffer_add_ullong16 (etag_local, (cullong_t) fhdl->info->st_size);
				cherokee_buffer_add_char     (etag_local, '"');
			}

			/* Compare ETags
			 */
			if ((header_len == etag_local->len) &&
			    (strncmp (header, etag_local->buf, etag_local->len) == 0))
			{
				not_modified_etag = true;
			}

			has_etag = true;
		}

		/* "If-Range: Sun, 14 Nov 2010 17:17:13 GMT"
		 */
		else {
			time_t req_time = 0;

			ret = cherokee_dtm_str2time (header, header_len, &req_time);
			if (unlikely (ret == ret_error)) {
				LOG_WARNING (CHEROKEE_ERROR_HANDLER_FILE_TIME_PARSE, header);

			} else if (likely (ret == ret_ok)) {
				/* If the entity tag given in the If-Range
				 * header matches the current entity tag for
				 * the entity, then the server SHOULD provide
				 * the specified sub-range of the entity using
				 * a 206 (Partial content) response. If the
				 * entity tag does not match, then the server
				 * SHOULD return the entire entity using a 200
				 * (OK) response.
				 */
				if (fhdl->info->st_mtime > req_time) {
					conn->error_code = http_ok;

					conn->range_start = -1;
					conn->range_end   = -1;
				}
			}
		}

		/* Restore EOL
		 */
		*end = tmp;
	}

	/* If both If-Modified-Since and ETag have been found then
	 * both must match in order to return a not_modified response.
	 */
	if (has_modified_since && has_etag) {
		if (not_modified_ms && not_modified_etag) {
			fhdl->not_modified = true;
			return ret_ok;
		}
	} else  {
		if (not_modified_ms || not_modified_etag) {
			fhdl->not_modified = true;
			return ret_ok;
		}
	}

	return ret_ok;
}


static ret_t
open_local_directory (cherokee_handler_file_t *fhdl, cherokee_buffer_t *local_file)
{
	cherokee_connection_t *conn = HANDLER_CONN(fhdl);

	/* Check if it is already open
	 */
	if (fhdl->fd > 0)
		return ret_ok;

	/* Open it
	 */
	fhdl->fd = cherokee_open (local_file->buf, O_RDONLY | O_BINARY, 0);
	if (fhdl->fd > 0) {
		cherokee_fd_set_closexec (fhdl->fd);
		return ret_ok;
	}

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
stat_local_directory (cherokee_handler_file_t   *fhdl,
		      cherokee_buffer_t         *local_file,
		      cherokee_iocache_entry_t **io_entry,
		      struct stat              **info)
{
	int                    re;
	ret_t                  ret;
	cherokee_connection_t *conn = HANDLER_CONN(fhdl);
	cherokee_server_t     *srv  = CONN_SRV(conn);

	/* I/O cache
	 */
	if ((srv->iocache != NULL) &&
	    (HDL_FILE_PROP(fhdl)->use_cache))
	{
		ret = cherokee_iocache_autoget (srv->iocache, local_file, iocache_stat, io_entry);
		TRACE (ENTRIES, "%s, use_iocache=1 ret=%d\n", local_file->buf, ret);

		switch (ret) {
		case ret_ok:
		case ret_ok_and_sent:
			*info = &(*io_entry)->state;
			return (*io_entry)->state_ret;

		case ret_no_sys:
			goto without;

		case ret_deny:
			conn->error_code = http_access_denied;
			break;
		case ret_not_found:
			conn->error_code = http_not_found;
			break;
		default:
			RET_UNKNOWN(ret);
			conn->error_code = http_internal_error;
		}

		return ret_error;
	}

	/* Without cache
	 */
without:
	re = cherokee_stat (local_file->buf, &fhdl->cache_info);
	TRACE (ENTRIES, "%s, use_iocache=0 ret=%d\n", local_file->buf, re);

	if (re >= 0) {
		*info = &fhdl->cache_info;
		return ret_ok;
	}

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

	return ret_error;
}


ret_t
cherokee_handler_file_custom_init (cherokee_handler_file_t *fhdl,
				   cherokee_buffer_t       *local_file)
{
	ret_t                     ret;
	char                     *ext;
	cherokee_iocache_entry_t *io_entry = NULL;
	cherokee_boolean_t        use_io   = false;
	cherokee_connection_t    *conn     = HANDLER_CONN(fhdl);
	cherokee_server_t        *srv      = HANDLER_SRV(fhdl);

	/* Query the I/O cache
	 */
	ret = stat_local_directory (fhdl, local_file, &io_entry, &fhdl->info);
	switch (ret) {
	case ret_ok:
		break;
	case ret_not_found:
		conn->error_code = http_not_found;
		ret = ret_error;
		goto out;
	default:
		goto out;
	}

	/* Ensure it is a file
	 */
	if (S_ISDIR(fhdl->info->st_mode)) {
		conn->error_code = http_access_denied;
		ret = ret_error;
		goto out;
	}

	/* Are we allowed to send symlinks?
	 */
	if (!HDL_FILE_PROP(fhdl)->send_symlinks) {
		struct stat stat;
		int re;

		re = cherokee_lstat (local_file->buf, &stat);
		if (re < 0) {
			ret = ret_error;
			goto out;
		}

		if (S_ISLNK(stat.st_mode)) {
			conn->error_code = http_not_found;
			ret = ret_error;
			goto out;
		}
	}

	/* Look for the mime type
	 */
	if (srv->mime != NULL) {
		ext = (local_file->buf + local_file->len) - 1;
		while (ext > local_file->buf) {
			if (*ext == '.') {
				ret = cherokee_mime_get_by_suffix (srv->mime, ext+1, &fhdl->mime);
				if (ret == ret_ok)
					break;
			}
			ext--;
		}
	}

	/* Is it cached on the client?
	 */
	ret = check_cached (fhdl);
	if ((ret != ret_ok) || (fhdl->not_modified)) {
		/* Set both ranges to zero to avoid file size errors in loggers
		 */
		conn->range_start  = 0;
		conn->range_end    = 0;
		goto out;
	}

	/* Is this file cached in the io cache?
	 */
	use_io = ((srv->iocache != NULL) &&
		  (conn->encoder_new_func == NULL) &&
		  (HDL_FILE_PROP(fhdl)->use_cache) &&
		  (conn->socket.is_tls == non_TLS) &&
		  (conn->flcache.mode == flcache_mode_undef) &&
		  (http_method_with_body (conn->header.method)) &&
		  (fhdl->info->st_size <= srv->iocache->max_file_size) &&
		  (fhdl->info->st_size >= srv->iocache->min_file_size));

	TRACE(ENTRIES, "Using iocache %d\n", use_io);

	if (use_io) {
		ret = cherokee_iocache_autoget_fd (srv->iocache,
						   local_file,
						   iocache_mmap,
						   &fhdl->fd,
						   &io_entry);

		TRACE (ENTRIES, "iocache looked up, local=%s ret=%d\n",
		       local_file->buf, ret);

		switch (ret) {
		case ret_ok:
		case ret_ok_and_sent:
			break;
		case ret_no_sys:
			use_io = false;
			break;
		case ret_deny:
			conn->error_code = http_access_denied;
			ret = ret_error;
			goto out;
		case ret_not_found:
			conn->error_code = http_not_found;
			ret = ret_error;
			goto out;
		default:
			goto out;
		}

		/* Ensure the mmap content is ready
		 */
		if (io_entry->mmaped == NULL)
			use_io = false;
	}

	/* Maybe open the file
	 */
	if (! use_io) {
		ret = open_local_directory (fhdl, local_file);
		if (ret != ret_ok) {
			goto out;
		}
	}

	/* Is it a directory?
	 */
	if (S_ISDIR (fhdl->info->st_mode)) {
		conn->error_code = http_access_denied;
		ret = ret_error;
		goto out;
	}

	/* Range 1: Check the range and file size
	 */
	if (unlikely (conn->range_start >= fhdl->info->st_size))
	{
		/* Sets the range limits, so the error handler can
		 * report the error properly.
		 */
		conn->range_start = 0;
		conn->range_end   = fhdl->info->st_size;
		conn->error_code  = http_range_not_satisfiable;

		ret = ret_error;
		goto out;
	}

	if (unlikely ((conn->range_end >= fhdl->info->st_size)))
	{
		/* Range-end out of bounds. Set it to -1 so it is
		 * updated with the file size later on.
		 */
		conn->range_end = -1;
	}

	/* Set the error code
	 */
	if ((conn->range_start > -1) ||
	    (conn->range_end   > -1))
	{
		conn->error_code = http_partial_content;
	}

	/* Range 2: Set the file length as the range end
	 */
	if (conn->range_end == -1) {
		if  (conn->range_start == -1) {
			conn->range_start = 0;
		}
		conn->range_end = fhdl->info->st_size - 1;
	} else {
		if (conn->range_start == -1) {
			conn->range_start = fhdl->info->st_size - conn->range_end;
			conn->range_end = fhdl->info->st_size - 1;
		}
	}

	/* Set mmap or file position
	 */
	if ((use_io) &&
	    (io_entry != NULL) &&
	    (io_entry->mmaped != NULL))
	{
		off_t len;

		/* Set the mmap info
		 */
		conn->io_entry_ref = io_entry;

		len = conn->range_end - conn->range_start + 1;

		conn->mmaped     = ((char *)io_entry->mmaped) + conn->range_start;
		conn->mmaped_len = len;
	} else {
		/* Does no longer care about the io_entry
		 */
		cherokee_iocache_entry_unref (&io_entry);

		/* Seek the file if needed
		 */
		if ((conn->range_start != 0) && (conn->mmaped == NULL)) {
			fhdl->offset = conn->range_start;
			lseek (fhdl->fd, fhdl->offset, SEEK_SET);
		}
	}

	/* Maybe use sendfile
	 */
#ifdef WITH_SENDFILE
	fhdl->using_sendfile = ((conn->mmaped == NULL) &&
				(conn->encoder == NULL) &&
				(conn->encoder_new_func == NULL) &&
				(conn->socket.is_tls == non_TLS) &&
				(conn->flcache.mode == flcache_mode_undef) &&
				(fhdl->info->st_size >= srv->sendfile.min) &&
				(fhdl->info->st_size <  srv->sendfile.max));

	if (fhdl->using_sendfile) {
		cherokee_connection_set_cork (conn, true);
		BIT_SET (conn->options, conn_op_tcp_cork);
	}
#endif

	return ret_ok;

out:
	cherokee_iocache_entry_unref (&io_entry);
	return ret;
}


ret_t
cherokee_handler_file_init (cherokee_handler_file_t *fhdl)
{
	ret_t                  ret;
	cherokee_connection_t *conn = HANDLER_CONN(fhdl);

	/* OPTIONS request
	 */
	if (unlikely (HANDLER_CONN(fhdl)->header.method == http_options)) {
		return ret_ok;
	}

	/* Build the local file path
	 */
	cherokee_buffer_add_buffer (&conn->local_directory, &conn->request);
	ret = cherokee_handler_file_custom_init (fhdl, &conn->local_directory);

	/* Undo the local directory
	 */
	cherokee_buffer_drop_ending (&conn->local_directory, conn->request.len);
	return ret;
}


ret_t
cherokee_handler_file_add_headers (cherokee_handler_file_t *fhdl,
				   cherokee_buffer_t       *buffer)
{
	ret_t                  ret;
	char                   bufstr[DTM_SIZE_GMTTM_STR];
	size_t                 szlen          = 0;
	off_t                  content_length = 0;
	cherokee_connection_t *conn           = HANDLER_CONN(fhdl);

	/* OPTIONS request
	 */
	if (unlikely (HANDLER_CONN(fhdl)->header.method == http_options)) {
		cherokee_buffer_add_str (buffer, "Content-Length: 0"CRLF);
		cherokee_handler_add_header_options (HANDLER(fhdl), buffer);
		return ret_ok;
	}

	/* Regular request
	 */

	/* ETag: "<etag>"
	 */
	if (conn->header.version >= http_version_11) {
		/* ETag: "%lx= FMT_OFFSET_HEX" CRLF
		 */
		cherokee_buffer_add_str     (buffer, "ETag: \"");
		cherokee_buffer_add_ullong16(buffer, (cullong_t) fhdl->info->st_mtime);
		cherokee_buffer_add_str     (buffer, "=");
		cherokee_buffer_add_ullong16(buffer, (cullong_t) fhdl->info->st_size);
		cherokee_buffer_add_str     (buffer, "\"" CRLF);
	}

	/* Last-Modified:
	 */
	if (!(fhdl->not_modified)) {
		struct tm modified_tm;

		memset (&modified_tm, 0, sizeof(struct tm));
		cherokee_gmtime (&fhdl->info->st_mtime, &modified_tm);

		szlen = cherokee_dtm_gmttm2str(bufstr, DTM_SIZE_GMTTM_STR, &modified_tm);

		cherokee_buffer_add_str(buffer, "Last-Modified: ");
		cherokee_buffer_add    (buffer, bufstr, szlen);
		cherokee_buffer_add_str(buffer, CRLF);
	}

	/* Add MIME related headers:
	 * "Content-Type:" and "Cache-Control: max-age="
	 */
	if (fhdl->mime != NULL) {
		cuint_t            maxage;
		if (!(fhdl->not_modified)) {
			cherokee_buffer_t *mime   = NULL;

			cherokee_mime_entry_get_type (fhdl->mime, &mime);
			cherokee_buffer_add_str    (buffer, "Content-Type: ");
			cherokee_buffer_add_buffer (buffer, mime);
			cherokee_buffer_add_str    (buffer, CRLF);
		}
		ret = cherokee_mime_entry_get_maxage (fhdl->mime, &maxage);
		if (ret == ret_ok) {
			/* Set the expiration if there wasn't a
			 * previous val. connection.c will render it.
			 */
			if (conn->expiration == cherokee_expiration_none) {
				conn->expiration      = cherokee_expiration_time;
				conn->expiration_time = maxage;
			}
		}
	}

	/* If it's replying "304 Not Modified", we're done here
	 */
	if (fhdl->not_modified) {
		/* The handler will manage this special reply
		 */
		HANDLER(fhdl)->support |= hsupport_error;

		conn->error_code = http_not_modified;
		return ret_ok;
	}


	if (cherokee_connection_should_include_length(conn)) {

		HANDLER(fhdl)->support |= hsupport_length;

		/* Content Length
		 */
		content_length = conn->range_end - conn->range_start + 1;
		if (unlikely (content_length < 0)) {
			content_length = 0;
		}

		if (conn->error_code == http_partial_content) {
			/*
			 * "Content-Range: bytes " FMT_OFFSET "-" FMT_OFFSET
			 *                                    "/" FMT_OFFSET CRLF
			 */
			cherokee_buffer_add_str     (buffer, "Content-Range: bytes ");
			cherokee_buffer_add_ullong10(buffer, (cullong_t)conn->range_start);
			cherokee_buffer_add_str     (buffer, "-");
			cherokee_buffer_add_ullong10(buffer, (cullong_t)(conn->range_end));
			cherokee_buffer_add_str     (buffer, "/");
			cherokee_buffer_add_ullong10(buffer, (cullong_t)fhdl->info->st_size);
			cherokee_buffer_add_str     (buffer, CRLF);
		}

		/*
		 * "Content-Length: " FMT_OFFSET CRLF
		 */
		cherokee_buffer_add_str     (buffer, "Content-Length: ");
		cherokee_buffer_add_ullong10(buffer, (cullong_t) content_length);
		cherokee_buffer_add_str     (buffer, CRLF);
	}

	return ret_ok;
}


ret_t
cherokee_handler_file_step (cherokee_handler_file_t *fhdl, cherokee_buffer_t *buffer)
{
	off_t                  total;
	size_t                 size;
	cherokee_connection_t *conn = HANDLER_CONN(fhdl);

	/* OPTIONS request
	 */
	if (unlikely (HANDLER_CONN(fhdl)->header.method == http_options)) {
		return ret_eof;
	}

#ifdef WITH_SENDFILE
	if (fhdl->using_sendfile) {
		ret_t   ret;
		ssize_t sent;
		off_t   to_send;

		to_send = conn->range_end - fhdl->offset + 1;
		if ((conn->limit_bps > 0) &&
		    (conn->limit_bps < to_send))
		{
			to_send = conn->limit_bps;
		}

		ret = cherokee_socket_sendfile (&conn->socket,    /* cherokee_socket_t *socket */
						fhdl->fd,         /* int                fd     */
						to_send,          /* size_t             size   */
						&fhdl->offset,    /* off_t             *offset */
						&sent);           /* ssize_t           *sent   */

		/* cherokee_handler_file_init() activated the TCP_CORK
		 * flags. Then the response header was sent. Now, the
		 * first chunk of the file with sendfile(), so it's
		 * time to turn off the TCP_CORK flag again.
		 */
		if (conn->options & conn_op_tcp_cork) {
			cherokee_connection_set_cork (conn, false);
			BIT_UNSET (conn->options, conn_op_tcp_cork);
		}

		if (ret == ret_no_sys) {
			fhdl->using_sendfile = false;
			goto exit_sendfile;
		}

		if (ret != ret_ok) {
			return ret;
		}

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
	size = MIN (DEFAULT_READ_SIZE, (conn->range_end - fhdl->offset + 1));

	/* Ensure there's enough memory
	 */
	cherokee_buffer_ensure_size (buffer, size + 1);

	/* Read
	 */
	do {
		total = read (fhdl->fd, buffer->buf, size);
	} while ((total == -1) && (errno == EINTR));

	switch (total) {
	case 0:
		return ret_eof;
	case -1:
		return ret_error;
	default:
		buffer->len = total;
		buffer->buf[buffer->len] = '\0';
		fhdl->offset += total;
	}

	/* Maybe it was the last file chunk
	 */
	if (fhdl->offset >= conn->range_end) {
		return ret_eof_have_data;
	}

	return ret_ok;
}

ret_t
cherokee_handler_file_seek (cherokee_handler_file_t *hdl,
			    off_t                    start)
{
	cherokee_connection_t *conn = HANDLER_CONN(hdl);

	conn->range_start = start;
	hdl->offset       = start;

	return ret_ok;
}
