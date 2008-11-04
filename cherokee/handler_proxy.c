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
#include "handler_proxy.h"

/* Plug-in initialization
 */
PLUGIN_INFO_HANDLER_EASIEST_INIT (proxy, http_all_methods);


ret_t
cherokee_handler_proxy_new (cherokee_handler_t     **hdl,
			    cherokee_connection_t   *cnt,
			    cherokee_module_props_t *props)
{
	ret_t ret;
	CHEROKEE_NEW_STRUCT (n, handler_proxy);
	
	/* Init the base class object
	 */
	cherokee_handler_init_base(HANDLER(n), cnt, HANDLER_PROPS(props), PLUGIN_INFO_HANDLER_PTR(proxy));
	   
	MODULE(n)->init         = (handler_func_init_t) cherokee_handler_proxy_init;
	MODULE(n)->free         = (module_func_free_t) cherokee_handler_proxy_free;
	HANDLER(n)->step        = (handler_func_step_t) cherokee_handler_proxy_step;
	HANDLER(n)->add_headers = (handler_func_add_headers_t) cherokee_handler_proxy_add_headers;

	HANDLER(n)->support = hsupport_nothing;

	/* Init
	 */
	ret = cherokee_buffer_init (&n->buffer);
	if (unlikely(ret != ret_ok)) 
		return ret;

	ret = cherokee_buffer_ensure_size (&n->buffer, 4*1024);
	if (unlikely(ret != ret_ok)) 
		return ret;

	*hdl = HANDLER(n);
	return ret_ok;
}

ret_t
cherokee_handler_proxy_free (cherokee_handler_proxy_t *hdl)
{
	cherokee_buffer_mrproper (&hdl->buffer);
	return ret_ok;
}


static ret_t 
props_free (cherokee_handler_proxy_props_t *props)
{
	return cherokee_module_props_free_base (MODULE_PROPS(props));
}


ret_t 
cherokee_handler_proxy_configure (cherokee_config_node_t   *conf,
				  cherokee_server_t        *srv,
				  cherokee_module_props_t **_props)
{
	cherokee_list_t                *i;
	cherokee_handler_proxy_props_t *props;

	UNUSED(srv);

	if (*_props == NULL) {
		CHEROKEE_NEW_STRUCT (n, handler_proxy_props);

		cherokee_module_props_init_base (MODULE_PROPS(n), 
						 MODULE_PROPS_FREE(props_free));
		n->foo = 0;
		*_props = MODULE_PROPS(n);
	}

	props = PROP_PROXY(*_props);

	cherokee_config_node_foreach (i, conf) {
		cherokee_config_node_t *subconf = CONFIG_NODE(i);

		if (equal_buf_str (&subconf->key, "xxx")) {
		} 
	}

	return ret_ok;
}


ret_t
cherokee_handler_proxy_init (cherokee_handler_proxy_t *hdl)
{
	return ret_ok;
}

ret_t
cherokee_handler_proxy_step (cherokee_handler_proxy_t *hdl,
			     cherokee_buffer_t        *buf)
{
	return ret_ok;
}


ret_t
cherokee_handler_proxy_add_headers (cherokee_handler_proxy_t *hdl,
				    cherokee_buffer_t        *buf)
{
	return ret_ok;
}

