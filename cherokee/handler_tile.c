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
        
        /* Look at handler_tile.h
         * This is an tile of configuration.
         */
		*_props = MODULE_PROPS(n);
	}

	props = PROP_TILE(*_props);

	cherokee_config_node_foreach (i, conf) {
		cherokee_config_node_t *subconf = CONFIG_NODE(i);

                if (equal_buf_str (&subconf->key, "balancer")) {
                        ret_t ret = cherokee_balancer_instance (&subconf->val, subconf, srv, &props->balancer);
                        if (ret != ret_ok)
                                return ret;
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
	
	/* Init the base class object
	 */
	cherokee_handler_init_base(HANDLER(n), cnt, HANDLER_PROPS(props), PLUGIN_INFO_HANDLER_PTR(tile));
	   
	MODULE(n)->init         = (handler_func_init_t) cherokee_handler_tile_init;
	MODULE(n)->free         = (module_func_free_t) cherokee_handler_tile_free;
	HANDLER(n)->add_headers = (handler_func_add_headers_t) cherokee_handler_tile_add_headers;

	HANDLER(n)->support = hsupport_nothing;

	n->src_ref   = NULL;
	n->send_done = false;
	cherokee_socket_init (&n->socket);

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
	struct protocol cmd;
	char   *from, *to;
	size_t  len;
	ret_t ret;
	cherokee_connection_t *conn = HANDLER_CONN(hdl);
	bzero(&cmd, sizeof(struct protocol));

	from = conn->request.buf+conn->web_directory.len+1;
	to   = strchr(from, '/');

	if (to && (len = (to - from)) < (XMLCONFIG_MAX - 1)) {
		strncpy(cmd.xmlname, from, len);
	} else {
		TRACE(ENTRIES, "Could not extract xmlname from %s\n", from);
		return ret_error;
	}

	from = to + 1;
	cmd.z = (int) strtol(from, &to, 10);

	from = to + 1;
	cmd.x = (int) strtol(from, &to, 10); 

	from = to + 1;
	cmd.y = (int) strtol(from, &to, 10); 

	TRACE(ENTRIES, "Found parameters: %d %d %d\n", cmd.z, cmd.x, cmd.y);

	if (cmd.z < 0 || cmd.x < 0 || cmd.y < 0 ||
			cmd.z == INT_MAX || cmd.x == INT_MAX || cmd.y == INT_MAX) {
		TRACE(ENTRIES, "Found parameters exceed specifications\n");
		return ret_error;
	}

	cmd.ver = 2;
	cmd.cmd = cmdRenderPrio;

	/* Connect
	*/
	ret = connect_to_server (hdl);
	switch (ret) {
		case ret_ok:
			break;
		case ret_eagain:
			return ret_eagain;
		case ret_deny:
//			conn->error_code = http_gateway_timeout;
			conn->error_code = http_not_found;
			return ret_error;
		default:
			conn->error_code = http_service_unavailable;
			return ret_error;
	}
	
	if (hdl->send_done == false) {
		TRACE(ENTRIES, "Writing to socket...\n");
		cherokee_socket_write (&hdl->socket, (const char *) &cmd, sizeof(struct protocol), &len);
		if (len != sizeof(struct protocol)) {
			TRACE(ENTRIES, "Unexpected return size!\n");
			return ret_error;
		}

	}

	TRACE(ENTRIES, "Reading from socket...\n");
        
	struct protocol resp;
	bzero(&resp, sizeof(struct protocol));
	
	ret = cherokee_socket_read(&hdl->socket, (char *) &resp, sizeof(struct protocol), &len);

	if (ret == ret_eagain) {
		cherokee_thread_deactive_to_polling (HANDLER_THREAD(hdl),
						     HANDLER_CONN(hdl),
						     S_SOCKET_FD(hdl->socket),
						     FDPOLL_MODE_READ, false);
		conn->error_code = http_not_found;
		return ret_eagain;
	}

	conn->error_code = http_unset;

	if (ret != ret_ok || len != sizeof(struct protocol)) {
		TRACE(ENTRIES, "Unexpected return size!\n");
		return ret_error;
	}
	
	if (cmd.x == resp.x && cmd.y == resp.y && cmd.z == resp.z &&
	    !strcmp(cmd.xmlname, resp.xmlname)) {
		if (resp.cmd == cmdDone) {
			TRACE(ENTRIES, "Command succesful\n");
			cherokee_buffer_add_buffer (&conn->redirect, &conn->request);
			conn->error_code = http_moved_temporarily;
			return ret_ok;
		} else {
			TRACE(ENTRIES, "The command was not done\n");
			conn->error_code = http_not_found;
			return ret_error;
		}
	} else {
		TRACE(ENTRIES, "We didn't get back what we asked for\n");
		return ret_error;
	}

	SHOULDNT_HAPPEN;
	return ret_error;
}


ret_t 
cherokee_handler_tile_add_headers (cherokee_handler_tile_t *hdl, cherokee_buffer_t *buffer)
{
	UNUSED(hdl);
	UNUSED(buffer);

	return ret_ok;
}
