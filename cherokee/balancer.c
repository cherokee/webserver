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
#include "balancer.h"
#include "plugin_loader.h"
#include "server-protected.h"
#include "source_interpreter.h"

/* Balancer Entry
 */

static ret_t
entry_new (cherokee_balancer_entry_t **entry)
{
	CHEROKEE_NEW_STRUCT (n, balancer_entry);

	INIT_LIST_HEAD (&n->listed);

	n->source         = NULL;
	n->disabled       = false;
	n->disabled_until = 0;

	*entry = n;
	return ret_ok;
}

static ret_t
entry_free (cherokee_balancer_entry_t *entry)
{
	free (entry);
	return ret_ok;
}


/* Balancer
 */

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
	balancer->report_fail  = NULL;

	/* Properties
	 */
	INIT_LIST_HEAD (&balancer->entries);
	balancer->entries_len  = 0;

	return ret_ok;
}


ret_t
cherokee_balancer_mrproper (cherokee_balancer_t *balancer)
{
	cherokee_list_t *i, *tmp;

	list_for_each_safe (i, tmp, &balancer->entries) {
		/* It doesn't free the sources. They are rather freed
		 * from srv->sources.
		 */
		cherokee_list_del (i);
		entry_free (BAL_ENTRY(i));
	}

	return ret_ok;
}


static ret_t
add_source (cherokee_balancer_t *balancer, cherokee_source_t *source)
{
	ret_t                      ret;
	cherokee_balancer_entry_t *entry;

	/* Instance a new balancer entry
	 */
	ret = entry_new (&entry);
	if (ret != ret_ok)
		return ret;

	entry->source = source;

	/* Add it to the list
	 */
	cherokee_list_add_tail (&entry->listed, &balancer->entries);
	balancer->entries_len += 1;

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
		LOG_ERROR (CHEROKEE_ERROR_BALANCER_NO_KEY, "source");
		return ret_error;
	}

	cherokee_config_node_foreach (i, subconf) {
		subconf2 = CONFIG_NODE(i);

		ret = cherokee_avl_get (&srv->sources, &subconf2->val, (void **)&src);
		if (ret != ret_ok) {
			LOG_ERROR (CHEROKEE_ERROR_BALANCER_BAD_SOURCE, subconf2->val.buf);
			return ret_error;
		}

		add_source (balancer, src);
	}

	return ret_ok;
}


ret_t
cherokee_balancer_dispatch (cherokee_balancer_t    *balancer,
                            cherokee_connection_t  *conn,
                            cherokee_source_t     **source)
{
	if (unlikely (balancer->dispatch == NULL))
		return ret_error;

	return balancer->dispatch (balancer, conn, source);
}


ret_t
cherokee_balancer_report_fail (cherokee_balancer_t   *balancer,
                               cherokee_connection_t *conn,
                               cherokee_source_t     *source)
{
	if (unlikely (balancer->report_fail == NULL))
		return ret_error;

	return balancer->report_fail (balancer, conn, source);
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
	if (unlikely (ret < ret_ok))
		return ret;

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
		LOG_CRITICAL_S (CHEROKEE_ERROR_BALANCER_UNDEFINED);
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
