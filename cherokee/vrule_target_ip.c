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
#include "vrule_target_ip.h"
#include "plugin_loader.h"
#include "connection.h"
#include "connection-protected.h"
#include "util.h"

#define ENTRIES "vrule,target_ip,ip"

PLUGIN_INFO_VRULE_EASIEST_INIT(target_ip);


static ret_t
match (cherokee_vrule_target_ip_t *vrule,
       cherokee_buffer_t          *host,
       cherokee_connection_t      *conn)
{
	int               re;
	ret_t             ret;
	cherokee_socket_t sock;

	UNUSED (host);

	/* There might not be a connection
	 */
	if (conn == NULL)
		return ret_deny;

	/* Use a temporal socket object
	 */
	ret = cherokee_socket_init (&sock);
	if (unlikely(ret != ret_ok))
		return ret_error;

	/* Copy the server IP
	 */
	sock.client_addr_len = conn->socket.client_addr_len;

	re = getsockname (SOCKET_FD(&conn->socket),
	                  (struct sockaddr *) &(sock.client_addr),
	                  &sock.client_addr_len);
	if (re != 0) {
		TRACE(ENTRIES, "VRule target_ip could %s get the server IP\n", "not");
		goto deny;
	}

	/* Validate it
	 */
	ret = cherokee_access_ip_match (&vrule->access, &sock);
	if (ret != ret_ok) {
		TRACE(ENTRIES, "VRule target_ip did %s match any\n", "not");
		goto deny;
	}

	cherokee_socket_mrproper (&sock);
	TRACE(ENTRIES, "Rule from matched %s", "\n");
	return ret_ok;
deny:

	cherokee_socket_mrproper (&sock);
	return ret_deny;
}

static ret_t
configure (cherokee_vrule_target_ip_t   *vrule,
           cherokee_config_node_t       *conf,
           cherokee_virtual_server_t    *vsrv)
{
	ret_t                   ret;
	cherokee_list_t        *i;
	cherokee_config_node_t *subconf;

	UNUSED(vsrv);

	ret = cherokee_config_node_get (conf, "to", &subconf);
	if (ret != ret_ok) {
		LOG_CRITICAL (CHEROKEE_ERROR_VRULE_NO_PROPERTY, VRULE(vrule)->priority, "to");
		return ret_error;
	}

	cherokee_config_node_foreach (i, subconf) {
		cherokee_config_node_t *subconf2 = CONFIG_NODE(i);

		ret = cherokee_access_add (&vrule->access, subconf2->val.buf);
		if (ret != ret_ok) {
			LOG_ERROR (CHEROKEE_ERROR_VRULE_TARGET_IP_PARSE, subconf2->val.buf);
			return ret_error;
		}
	}

	return ret_ok;
}

static ret_t
_free (void *p)
{
	cherokee_vrule_target_ip_t *vrule = p;

	cherokee_access_mrproper (&vrule->access);
	return ret_ok;
}


ret_t
cherokee_vrule_target_ip_new (cherokee_vrule_t **vrule)
{
	ret_t ret;

	CHEROKEE_NEW_STRUCT (n, vrule_target_ip);

	/* Parent class constructor
	 */
	cherokee_vrule_init_base (VRULE(n), PLUGIN_INFO_PTR(target_ip));

	/* Virtual methods
	 */
	VRULE(n)->match     = (vrule_func_match_t) match;
	VRULE(n)->configure = (vrule_func_configure_t) configure;
	MODULE(n)->free     = (module_func_free_t) _free;

	/* Properties
	 */
	ret = cherokee_access_init (&n->access);
	if (ret != ret_ok) {
		return ret_error;
	}

	*vrule = VRULE(n);
	return ret_ok;
}
