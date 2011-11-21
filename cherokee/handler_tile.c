/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/* Cherokee
 *
 * Authors:
 *      Alvaro Lopez Ortega <alvaro@alobbs.com>
 *      Stefan de Konink <stefan@konink.de>
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

#include "handler_tile.h"

#include "connection-protected.h"
#include "source_interpreter.h"
#include "thread.h"
#include <sys/stat.h>

#define ENTRIES "handler,tile"

/* Plug-in initialization
 *
 * In this function you can use any of these:
 * http_delete | http_get | http_post | http_put
 *
 * For a full list: cherokee_http_method_t
 *
 * It is what your handler to be implements.
 *
 */
PLUGIN_INFO_HANDLER_EASIEST_INIT (tile, http_get);


/* Methods implementation
 */
static ret_t 
props_free (cherokee_handler_tile_props_t *props)
{
	if (props->file_props != NULL) {
		cherokee_handler_file_props_free (props->file_props);
	}

	return cherokee_module_props_free_base (MODULE_PROPS(props));
}


ret_t 
cherokee_handler_tile_configure (cherokee_config_node_t *conf, cherokee_server_t *srv, cherokee_module_props_t **_props)
{
	cherokee_list_t               *i;
	cherokee_handler_tile_props_t *props;

	if (*_props == NULL) {
		CHEROKEE_NEW_STRUCT (n, handler_tile_props);

		cherokee_module_props_init_base (MODULE_PROPS(n), 
						 MODULE_PROPS_FREE(props_free));

		n->timeout = 3;
		n->expiration_time = 0;

		/* Sub-handler properties */
		n->file_props = NULL;
		cherokee_handler_file_configure (conf, srv, (cherokee_module_props_t **) &n->file_props);
		n->file_props->use_cache = false;
        
		*_props = MODULE_PROPS(n);
	}

	props = PROP_TILE(*_props);

	cherokee_config_node_foreach (i, conf) {
		cherokee_config_node_t *subconf = CONFIG_NODE(i);

                if (equal_buf_str (&subconf->key, "balancer")) {
                        ret_t ret = cherokee_balancer_instance (&subconf->val, subconf, srv, &props->balancer);
                        if (ret != ret_ok)
                                return ret;
		} else if (equal_buf_str (&subconf->key, "timeout")) {
			props->timeout = atoi (subconf->val.buf);
		} else if (equal_buf_str (&subconf->key, "expiration")) {
			props->expiration_time = cherokee_eval_formated_time (&subconf->val);
		} else {
			PRINT_MSG ("ERROR: Handler file: Unknown key: '%s'\n", subconf->key.buf);
			return ret_error;
		}
	}

        /* Final checks
         */
        if (props->balancer == NULL) {
                return ret_error;
        }

	return ret_ok;
}

ret_t
cherokee_handler_tile_new  (cherokee_handler_t **hdl, cherokee_connection_t *cnt, cherokee_module_props_t *props)
{
	CHEROKEE_NEW_STRUCT (n, handler_tile);
	ret_t ret;
	
	/* Init the base class object
	 */
	cherokee_handler_init_base(HANDLER(n), cnt, HANDLER_PROPS(props), PLUGIN_INFO_HANDLER_PTR(tile));
	   
	MODULE(n)->init         = (handler_func_init_t) cherokee_handler_tile_init;
	MODULE(n)->free         = (module_func_free_t)  cherokee_handler_tile_free;
	HANDLER(n)->step        = (handler_func_step_t) cherokee_handler_tile_step;
	HANDLER(n)->add_headers = (handler_func_add_headers_t) cherokee_handler_tile_add_headers;

	HANDLER(n)->support = hsupport_nothing;

	n->src_ref     = NULL;
	n->mystatus    = parsing;
	bzero(&n->cmd, sizeof(struct protocol));
	cherokee_socket_init (&n->socket);
	
	/* Instance file sub-handler
	 */
	ret = cherokee_handler_file_new ((cherokee_handler_t **)&n->file_hdl, cnt, MODULE_PROPS(PROP_TILE(props)->file_props));
	if (ret != ret_ok) {
		return ret_ok;
	}
	HANDLER(n)->support = HANDLER(n->file_hdl)->support;

	/* Init
	 */
	*hdl = HANDLER(n);
	return ret_ok;
}


ret_t 
cherokee_handler_tile_free (cherokee_handler_tile_t *hdl)
{
	TRACE(ENTRIES, "Close...");

	cherokee_socket_close(&hdl->socket);
	cherokee_socket_mrproper(&hdl->socket);

	if (hdl->file_hdl != NULL) {
		cherokee_handler_file_free (hdl->file_hdl);
	}

	return ret_ok;
}


static ret_t
connect_to_server (cherokee_handler_tile_t *hdl)
{
	ret_t                          ret;
	cherokee_connection_t         *conn  = HANDLER_CONN(hdl);
	cherokee_handler_tile_props_t *props = HDL_TILE_PROPS(hdl);

	/* Get a reference to the target host
	*/
	if (hdl->src_ref == NULL) {
		ret = cherokee_balancer_dispatch (props->balancer, conn, &hdl->src_ref);
		if (ret != ret_ok)
			return ret;
	}

	/* Try to connect
	*/
	if (hdl->src_ref->type == source_host) {
		ret = cherokee_source_connect_polling (hdl->src_ref, &hdl->socket, conn);
		if ((ret == ret_deny) || (ret == ret_error))
		{
			cherokee_balancer_report_fail (props->balancer, conn, hdl->src_ref);
		}
	} else {
		ret = cherokee_source_interpreter_connect_polling (SOURCE_INT(hdl->src_ref), &hdl->socket, conn);
	}

	return ret;
}


ret_t 
cherokee_handler_tile_init (cherokee_handler_tile_t *hdl)
{
	ret_t ret;
	size_t  len;
	cherokee_connection_t *conn = HANDLER_CONN(hdl);

	if (hdl->mystatus == parsing) {
		char   *from, *to;

		from = conn->request.buf+conn->web_directory.len+1;
		to   = strchr(from, '/');

		if (to && (len = (to - from)) < (XMLCONFIG_MAX - 1)) {
			strncpy(hdl->cmd.xmlname, from, len);
		} else {
			TRACE(ENTRIES, "Could not extract xmlname from %s\n", from);
			goto besteffort;
		}

		from = to + 1;
		hdl->cmd.z = (int) strtol(from, &to, 10);

		from = to + 1;
		hdl->cmd.x = (int) strtol(from, &to, 10); 

		from = to + 1;
		hdl->cmd.y = (int) strtol(from, &to, 10); 

		TRACE(ENTRIES, "Found parameters: %d %d %d\n", hdl->cmd.z, hdl->cmd.x, hdl->cmd.y);

		if (hdl->cmd.z < 0 || hdl->cmd.x < 0 || hdl->cmd.y < 0 ||
				hdl->cmd.z == INT_MAX || hdl->cmd.x == INT_MAX || hdl->cmd.y == INT_MAX) {
			TRACE(ENTRIES, "Found parameters exceed specifications\n");
			goto besteffort;
		}

		hdl->cmd.ver = 2;
		if (HDL_TILE_PROPS(hdl)->timeout == 0) {
			hdl->cmd.cmd = cmdRender;
		} else {
			hdl->cmd.cmd = cmdRenderPrio;
		}

		hdl->mystatus = sending;
	}

	if (hdl->mystatus == sending) {
		/* Connect
		*/

		ret = connect_to_server (hdl);
		switch (ret) {
			case ret_ok:
				break;
			case ret_eagain:
				return ret_eagain;
			case ret_deny:
			default:
				goto besteffort;
		}
	
		TRACE(ENTRIES, "Writing to socket...\n");
		cherokee_socket_write (&hdl->socket, (const char *) &hdl->cmd, sizeof(struct protocol), &len);
		if (len != sizeof(struct protocol)) {
			TRACE(ENTRIES, "Unexpected return size!\n");
			goto besteffort;
		}
		hdl->mystatus = senddone;
	}

	if (HDL_TILE_PROPS(hdl)->timeout == 0) {
		TRACE(ENTRIES, "Rendering started, we continue serving...\n");
		goto besteffort;
	}

	TRACE(ENTRIES, "Reading from socket...\n");
        
	struct protocol resp;
	bzero(&resp, sizeof(struct protocol));
	
	ret = cherokee_socket_read(&hdl->socket, (char *) &resp, sizeof(struct protocol), &len);

	if (ret == ret_eagain && hdl->mystatus != timeout) {
		cherokee_thread_deactive_to_polling_timeout (HANDLER_THREAD(hdl),
						             HANDLER_CONN(hdl),
						             S_SOCKET_FD(hdl->socket),
   						             FDPOLL_MODE_READ, false,
							     HDL_TILE_PROPS(hdl)->timeout);
		hdl->mystatus = timeout;
		return ret_eagain;
	}

	if (ret != ret_ok || len != sizeof(struct protocol)) {
		TRACE(ENTRIES, "Unexpected return size!\n");
		goto besteffort;
	}
	
	if (hdl->cmd.x == resp.x && hdl->cmd.y == resp.y && hdl->cmd.z == resp.z &&
	    !strcmp(hdl->cmd.xmlname, resp.xmlname)) {
		if (resp.cmd == cmdDone) {
			TRACE(ENTRIES, "Command succesful\n");
			cherokee_buffer_add_buffer (&conn->redirect, &conn->request);
			goto succesful;
		} else {
			TRACE(ENTRIES, "The command was not done\n");
			goto besteffort;
		}
	} else {
		TRACE(ENTRIES, "We didn't get back what we asked for\n");
		goto besteffort;
	}

besteffort:
	conn->expiration = cherokee_expiration_time;
	conn->expiration_time = HDL_TILE_PROPS(hdl)->expiration_time;

succesful:
	return cherokee_handler_file_init (hdl->file_hdl);
}


ret_t 
cherokee_handler_tile_add_headers (cherokee_handler_tile_t *hdl, cherokee_buffer_t *buffer)
{
	return cherokee_handler_file_add_headers (hdl->file_hdl, buffer);
}

ret_t 
cherokee_handler_tile_step (cherokee_handler_tile_t *hdl, cherokee_buffer_t *buffer)
{
	return cherokee_handler_file_step (hdl->file_hdl, buffer);
}
