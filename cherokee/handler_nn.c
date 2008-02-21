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
#include "handler_nn.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>

#include "thread.h"
#include "connection.h"
#include "connection-protected.h"
#include "module.h"
#include "plugin_loader.h"
#include "connection.h"
#include "handler_common.h"
#include "handler_redir.h"
#include "levenshtein_distance.h"


ret_t 
cherokee_handler_nn_configure (cherokee_config_node_t *conf, cherokee_server_t *srv, cherokee_module_props_t **_props)
{
	return cherokee_handler_common_configure (conf, srv, _props);
}


static ret_t
get_nearest_from_directory (char *directory, char *request, cherokee_buffer_t *output)
{
	DIR               *dir;
	struct dirent     *entry;
	int                min_diff = 9999;
	cherokee_boolean_t found    = false;

	dir = opendir(directory);
	if (dir == NULL)
		goto go_out;

	while ((entry = readdir (dir)) != NULL) { 
		int dis;
			 
		if (!strncmp (entry->d_name, ".", 1)) continue;
		if (!strncmp (entry->d_name, "..",2)) continue;

		dis = distance ((char *)request, entry->d_name);
		if (dis < min_diff) {
			min_diff = dis;
			found    = true;
			
			cherokee_buffer_clean (output);
			cherokee_buffer_add (output, entry->d_name, strlen(entry->d_name));
		}
			 
	}
	closedir (dir);

go_out:
	return (found) ? ret_ok : ret_error;
}


static ret_t
get_nearest_name (
			cherokee_connection_t *conn,
			cherokee_buffer_t *local_dir,
			cherokee_buffer_t *request,
			cherokee_buffer_t *output)
{
	char              *rest;
	ret_t              ret = ret_ok;
	cherokee_thread_t *thread = CONN_THREAD(conn);
	cherokee_buffer_t *req = THREAD_TMP_BUF1(thread);/* Request w/o last word */

	/* Build the local request path
	 */
	rest = strrchr (request->buf, '/');
	if (rest == NULL) {
		return ret_error;
	}
	rest++;

	cherokee_buffer_clean (req);
	cherokee_buffer_add_buffer (req, local_dir);
	cherokee_buffer_add (req, request->buf, rest - request->buf);
	
	/* Copy the new filename to the output buffer
	 */
	ret = get_nearest_from_directory (req->buf, rest, output);

	if (unlikely (ret != ret_ok)) {
		return ret_error;
	}

	/* Prepend the rest of the old request to the new filename
	 */
	cherokee_buffer_prepend (output, request->buf, rest - request->buf);
	return ret_ok;
}


ret_t 
cherokee_handler_nn_new (cherokee_handler_t **hdl, void *cnt, cherokee_module_props_t *props)
{
	ret_t                  ret;
	struct stat            info;
	int                    stat_ret;
	cherokee_connection_t *conn   = CONN(cnt);

	cherokee_buffer_add (&conn->local_directory, conn->request.buf, conn->request.len);
	stat_ret = stat (conn->local_directory.buf, &info);
	cherokee_buffer_drop_endding (&conn->local_directory, conn->request.len);
	
	/* Maybe the file/dir exists
	 */
	if (stat_ret == 0) {
		return cherokee_handler_common_new (hdl, cnt, props);
	} 

	/* It doesn't exists, let's redirect it..
	 */
	cherokee_buffer_clean (&conn->redirect);

	ret = get_nearest_name (conn, &conn->local_directory, &conn->request, &conn->redirect);
	if (unlikely (ret != ret_ok)) {
		conn->error_code = http_not_found;
		return ret_error;
	}

	cherokee_buffer_swap_buffers (&conn->request, &conn->redirect);
	cherokee_buffer_clean (&conn->redirect);

	return ret_eagain;
}


/*   Library init function
 */
static cherokee_boolean_t _nn_is_init = false;

void
PLUGIN_INIT_NAME(nn) (cherokee_plugin_loader_t *loader)
{
	/* Is init?
	 */
	if (_nn_is_init)
		return;
	_nn_is_init = true;
	   
	/* Load the dependences
	 */
	cherokee_plugin_loader_load (loader, "common");
}

PLUGIN_INFO_HANDLER_EASY_INIT (nn, http_all_methods);

