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
#include "handler_common.h"

#ifdef HAVE_SYS_TYPES_H
# include <sys/types.h>
#endif

#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>

#ifdef HAVE_STRINGS_H
# include <strings.h>
#endif

#include "util.h"
#include "connection.h"
#include "connection-protected.h"
#include "server.h"
#include "server-protected.h"
#include "module.h"
#include "connection.h"

#define ENTRIES "handler,common"
#define DEFAULT_ALLOW_PATHINFO true

ret_t
cherokee_handler_common_props_free (cherokee_handler_common_props_t *props)
{
	if (props->props_file != NULL) {
		cherokee_handler_file_props_free (props->props_file);
		props->props_file = NULL;
	}
	
	if (props->props_dirlist != NULL) {
		cherokee_handler_dirlist_props_free (props->props_dirlist);
		props->props_dirlist = NULL;
	}

	return cherokee_handler_props_free_base (HANDLER_PROPS(props));
}


ret_t 
cherokee_handler_common_configure (cherokee_config_node_t *conf, cherokee_server_t *srv, cherokee_module_props_t **_props)
{
	ret_t                            ret;
	cherokee_config_node_t          *subconf;
	cherokee_handler_common_props_t *props;

	if (*_props == NULL) {
		CHEROKEE_NEW_STRUCT (n, handler_common_props);

		cherokee_handler_props_init_base (HANDLER_PROPS(n),
			MODULE_PROPS_FREE(cherokee_handler_common_props_free));

		n->props_file     = NULL;
		n->props_dirlist  = NULL;
		n->allow_pathinfo = DEFAULT_ALLOW_PATHINFO;

		*_props = MODULE_PROPS(n);
	}

	props = PROP_COMMON(*_props);

	/* Parse 'common' parameters
	 */
	ret = cherokee_config_node_get (conf, "allow_pathinfo", &subconf);
	if (ret == ret_ok) {
		props->allow_pathinfo = atoi(subconf->val.buf);
	}

	/* Parse 'file' parameters
	 */
	ret = cherokee_handler_file_configure (conf, srv, (cherokee_module_props_t **)&props->props_file);
	if ((ret != ret_ok) && (ret != ret_deny)) return ret;

	/* Parse 'dirlist' parameters
	 */
	return cherokee_handler_dirlist_configure (conf, srv, (cherokee_module_props_t **)&props->props_dirlist);
}


static ret_t
stat_file (cherokee_boolean_t         useit, 
	   cherokee_iocache_t        *iocache, 
	   struct stat               *nocache_info, 
	   cherokee_buffer_t         *path, 
	   cherokee_iocache_entry_t **io_entry,
	   struct stat              **info)
{	
	ret_t ret;
	int   re  = -1;

	/* I/O cache
	 */
	if (useit) {
		ret = cherokee_iocache_autoget (iocache, path, iocache_stat, io_entry);
		TRACE (ENTRIES, "%s, use_iocache=1 ret=%d\n", path->buf, ret);

		switch (ret) {
		case ret_ok:
		case ret_ok_and_sent:
			*info = &(*io_entry)->state;
			return ret_ok;

		case ret_no_sys:
			goto without;

		case ret_deny:
		case ret_not_found:
			return ret;
		default:
			return ret_error;
		}
	}

	/* Without cache
	 */
without:
	re = cherokee_stat (path->buf, nocache_info);
	TRACE (ENTRIES, "%s, use_iocache=0 re=%d\n", path->buf, re);

	if (re >= 0) {
		*info = nocache_info;
		return ret_ok;
	}
	
	switch (errno) {
	case ENOENT: 
		return ret_not_found;
	case EACCES: 
		return ret_deny;
	default:
		return ret_error;
	}

	return ret_error;
}



ret_t 
cherokee_handler_common_new (cherokee_handler_t **hdl, void *cnt, cherokee_module_props_t *props)
{
	ret_t                     ret;
	int                       exists;
	struct stat               nocache_info;
	struct stat              *info;
	cherokee_iocache_entry_t *io_entry    = NULL;
	cherokee_iocache_t       *iocache     = NULL;
 	cherokee_boolean_t        use_iocache = true;
	cherokee_connection_t    *conn        = CONN(cnt);

	TRACE_CONN(conn);

	/* Check some properties
	 */
	if (PROP_COMMON(props)->props_file != NULL) {
		use_iocache = PROP_COMMON(props)->props_file->use_cache;
	}

	/* Check the request
	 */
	cherokee_buffer_add_buffer (&conn->local_directory, &conn->request);
	
	if (use_iocache)
		cherokee_iocache_get_default (&iocache);

	ret = stat_file (use_iocache, iocache, &nocache_info, &conn->local_directory, &io_entry, &info);
	exists = (ret == ret_ok);

	TRACE (ENTRIES, "request: '%s', local: '%s', exists %d\n", 
	       conn->request.buf, conn->local_directory.buf, exists);
	
	if (!exists) {
		ret_t  ret;
		char  *pathinfo;
		int    pathinfo_len;
		int    begin;

		/* If PathInfo is not allowed just return 'Not Found'
		 */
		if (! PROP_COMMON(props)->allow_pathinfo) {
			TRACE(ENTRIES, "Returns conn->error_code: %s\n", "http_not_found");
			cherokee_iocache_entry_unref (&io_entry);

			conn->error_code = http_not_found; 
			return ret_error;
		}

		/* Maybe it could stat() the file because the request contains
		 * a PathInfo string at the end..
		 */
		begin = conn->local_directory.len - conn->request.len;
		
		ret = cherokee_split_pathinfo (&conn->local_directory, begin, true, &pathinfo, &pathinfo_len);
		if ((ret == ret_not_found) || (pathinfo_len <= 0)) {
			TRACE(ENTRIES, "Returns conn->error_code: %s\n", "http_not_found");
			cherokee_iocache_entry_unref (&io_entry);

			conn->error_code = http_not_found;
			return ret_error;
		}
		
		/* Copy the PathInfo and clean the request 
		 */
		cherokee_buffer_add (&conn->pathinfo, pathinfo, pathinfo_len);
		cherokee_buffer_drop_ending (&conn->request, pathinfo_len);

		/* Clean the local_directory, this connection is going
		 * to restart the connection setup phase
		 */
		cherokee_buffer_clean (&conn->local_directory);
		cherokee_iocache_entry_unref (&io_entry);

		TRACE_CONN(conn);
		return ret_eagain;
	}	

	cherokee_buffer_drop_ending (&conn->local_directory, conn->request.len);

	/* Is it a file?
	 */
	if (S_ISREG(info->st_mode)) {
		TRACE (ENTRIES, "going for %s\n", "handler_file");
		cherokee_iocache_entry_unref (&io_entry);

		return cherokee_handler_file_new (hdl, cnt, MODULE_PROPS(PROP_COMMON(props)->props_file));
	}

	/* Is it a directory
	 */
	if (S_ISDIR(info->st_mode)) {
		cherokee_thread_t *thread = CONN_THREAD(conn);
		cherokee_list_t   *i;

		cherokee_iocache_entry_unref (&io_entry);

		/* Maybe it has to be redirected
		 */
		if (cherokee_buffer_end_char (&conn->request) != '/') {
			TRACE (ENTRIES, "going for %s\n", "handler_dir");
			return cherokee_handler_dirlist_new (hdl, cnt, MODULE_PROPS(PROP_COMMON(props)->props_dirlist));
		}

		/* Add the request
		 */
		cherokee_buffer_add_buffer (&conn->local_directory, &conn->request);

		/* Have an index file inside?
		 */
		list_for_each (i, &CONN_VSRV(conn)->index_list) {
			int   is_dir;
			char *index     = LIST_ITEM_INFO(i);
			int   index_len = strlen (index);

			/* Check if the index is fullpath
			 */
			if (*index == '/') {
				cherokee_buffer_t *new_local_dir = THREAD_TMP_BUF1(thread);

				/* This means there is a configuration entry like:
				 * 'DirectoryIndex index.php, /index_default.php'
				 * and it's proceesing '/index_default.php'.
				 */ 

				/* Build the secondary path
				 */
				cherokee_buffer_add_buffer (&conn->effective_directory, &conn->local_directory);

				/* Lets reconstruct the local directory
				 */
				cherokee_buffer_clean (new_local_dir);
				cherokee_buffer_add_buffer (new_local_dir, &CONN_VSRV(conn)->root);
				cherokee_buffer_add (new_local_dir, index, index_len);
				
				ret = stat_file (use_iocache, iocache, &nocache_info, new_local_dir, &io_entry, &info);
				exists = (ret == ret_ok);
				cherokee_iocache_entry_unref (&io_entry);

				if (!exists)
					continue;

				/* Build the new request before respin
				 */
				cherokee_buffer_clean (&conn->local_directory);
				cherokee_buffer_clean (&conn->request_original);
				cherokee_buffer_add_buffer (&conn->request_original, &conn->request);

				cherokee_buffer_clean (&conn->request);
				cherokee_buffer_add (&conn->request, index, index_len);				

				TRACE (ENTRIES, "top level index matched %s\n", index);

				BIT_SET (conn->options, conn_op_root_index);

				TRACE_CONN(conn);
				return ret_eagain;
			}

			/* stat() the possible new path
			 */
			cherokee_buffer_add (&conn->local_directory, index, index_len);
			ret = stat_file (use_iocache, iocache, &nocache_info, &conn->local_directory, &io_entry, &info);

			exists =  (ret == ret_ok);
			is_dir = ((ret == ret_ok) && S_ISDIR(info->st_mode));

			cherokee_iocache_entry_unref (&io_entry);
			cherokee_buffer_drop_ending (&conn->local_directory, index_len);

			TRACE (ENTRIES, "trying index '%s', exists %d\n", index, exists);

			/* If the file doesn't exist or it is a directory, try with the next one
			 */
			if ((!exists) || (is_dir)) 
				continue;
			
			/* Add the index file to the request and clean up
			 */
			cherokee_buffer_drop_ending (&conn->local_directory, conn->request.len);
			cherokee_buffer_add (&conn->request, index, index_len); 

			TRACE_CONN(conn);
			return ret_eagain;
		}

		/* If the dir hasn't a index file, it uses dirlist
		 */
		cherokee_buffer_drop_ending (&conn->local_directory, conn->request.len);
		return cherokee_handler_dirlist_new (hdl, cnt, MODULE_PROPS(PROP_COMMON(props)->props_dirlist));
	}

	/* Unknown request type
	 */
	TRACE(ENTRIES, "Returns conn->error_code: %s\n", "http_unsupported_media_type");

	conn->error_code = http_unsupported_media_type;
	return ret_error;
}



/* Library init function
 */
static cherokee_boolean_t _common_is_init = false;

void  
PLUGIN_INIT_NAME(common) (cherokee_plugin_loader_t *loader)
{
	if (_common_is_init) return;
	_common_is_init = true;

	/* Load the dependences
	 */
	cherokee_plugin_loader_load (loader, "file");
	cherokee_plugin_loader_load (loader, "dirlist");
}

PLUGIN_INFO_HANDLER_EASY_INIT (common, http_all_methods);
