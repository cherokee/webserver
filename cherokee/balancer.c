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
#include "balancer.h"
#include "plugin_loader.h"
#include "server-protected.h"
#include "source_interpreter.h"

#define DEFAULT_SOURCES_ALLOCATION 5


ret_t 
cherokee_balancer_init_base (cherokee_balancer_t *balancer, cherokee_plugin_info_t *info)
{
	/* Init the base class
	 */
	cherokee_module_init_base (MODULE(balancer), NULL, info);

	/* Virtual methods
	 */
	balancer->dispatch     = NULL;
	balancer->configure    = NULL;
	
	/* Sources
	 */
	balancer->sources_len  = 0;
	balancer->sources_size = 0;
	balancer->sources      = NULL;

	return ret_ok;
}


ret_t 
cherokee_balancer_mrproper (cherokee_balancer_t *balancer)
{
	/* There is nothing to free here
	 */
	return ret_ok;
}


ret_t 
cherokee_balancer_configure_base (cherokee_balancer_t    *balancer, 
				  cherokee_server_t      *srv,
				  cherokee_config_node_t *conf)
{
	ret_t                   ret;
	cherokee_list_t        *i;
	cherokee_source_t      *src;
	cherokee_config_node_t *subconf  = NULL;
	cherokee_config_node_t *subconf2 = NULL;

	/* Look for the type of the source objects
	 */
	ret = cherokee_config_node_get (conf, "source", &subconf);
	if (ret != ret_ok) {
		PRINT_ERROR_S ("ERROR: Balancer: An entry 'source' is needed\n");
		return ret_error;
	}

	cherokee_config_node_foreach (i, subconf) {
		subconf2 = CONFIG_NODE(i);

		ret = cherokee_avl_get (&srv->sources, &subconf2->val, (void **)&src);
		if (ret != ret_ok) {
			PRINT_ERROR ("Could not find source '%s'\n", subconf2->val.buf);
			return ret_error;
		}

		cherokee_balancer_add_source (balancer, src);
	}

	return ret_ok;
}


static ret_t
alloc_more_sources (cherokee_balancer_t *balancer)
{
	size_t size;

	if (balancer->sources == NULL) {
		size = DEFAULT_SOURCES_ALLOCATION * sizeof(cherokee_source_t *);
		balancer->sources = (cherokee_source_t **) malloc (size);
	} else {
		size = (balancer->sources_size + DEFAULT_SOURCES_ALLOCATION ) * sizeof(cherokee_source_t *);
		balancer->sources = (cherokee_source_t **) realloc (balancer->sources, size);
	}
	
	if (balancer->sources == NULL) 
		return ret_nomem;
	
	memset (balancer->sources + balancer->sources_len, 0, DEFAULT_SOURCES_ALLOCATION);

	balancer->sources_size += DEFAULT_SOURCES_ALLOCATION;
	return ret_ok;
}


ret_t 
cherokee_balancer_add_source (cherokee_balancer_t *balancer, cherokee_source_t *source)
{
	ret_t ret;
	
	if (balancer->sources_len >= balancer->sources_size) {
		ret = alloc_more_sources (balancer);
		if (ret != ret_ok) return ret;
	}

	balancer->sources[balancer->sources_len] = source;
	balancer->sources_len++;

	return ret_ok;
}


ret_t 
cherokee_balancer_dispatch (cherokee_balancer_t *balancer, cherokee_connection_t *conn, cherokee_source_t **source)
{
	if (balancer->dispatch == NULL)
		return ret_error;

	return balancer->dispatch (balancer, conn, source);
}


ret_t 
cherokee_balancer_free (cherokee_balancer_t *bal)
{
	ret_t              ret;
	cherokee_module_t *module = MODULE(bal);

	/* Sanity check
	 */
	if (module->free == NULL) {
		return ret_error;
	}
	
	/* Call the virtual method implementation
	 */
	ret = MODULE(bal)->free (bal); 
	if (unlikely (ret < ret_ok)) return ret;
	
	/* Free the rest
	 */
	cherokee_balancer_mrproper(bal);

	free (bal);
	return ret_ok;
}


ret_t 
cherokee_balancer_instance (cherokee_buffer_t       *name, 
			    cherokee_config_node_t  *conf, 
			    cherokee_server_t       *srv, 
			    cherokee_balancer_t    **balancer)
{
	ret_t                      ret;
	balancer_new_func_t        new_func;
	balancer_configure_func_t  config_func;
	cherokee_plugin_info_t    *info = NULL;
	
	if (cherokee_buffer_is_empty (name)) {
		PRINT_ERROR_S ("Balancer defined without a value\n");
		return ret_error;
	}

	ret = cherokee_plugin_loader_get (&srv->loader, name->buf, &info);
	if (ret != ret_ok) return ret;
	
	new_func = (balancer_new_func_t) info->instance;
	ret = new_func (balancer);
	if (ret != ret_ok) return ret;


	config_func = (balancer_configure_func_t) info->configure;
	ret = config_func (*balancer, srv, conf);
	if (ret != ret_ok) return ret;

	return ret_ok;
}
