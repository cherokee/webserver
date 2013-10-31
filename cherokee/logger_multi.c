/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/* Cherokee
 *
 * Authors:
 *      Sylvain Munaut <s.munaut@whatever-company.com>
 *
 * Copyright (C) 2013 Whatever s.a.
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
#include "logger_multi.h"

#include "util.h"
#include "connection.h"
#include "connection-protected.h"
#include "module.h"
#include "server.h"
#include "server-protected.h"


/* Plug-in definition
 */
PLUGIN_INFO_LOGGER_EASIEST_INIT(multi);


static ret_t
create_sub_logger (cherokee_logger_t         **logger,
		   cherokee_config_node_t    *config,
		   cherokee_virtual_server_t *vsrv)
{
	ret_t                   ret;
	logger_func_new_t       func_new;
	cherokee_plugin_info_t *info      = NULL;
	cherokee_server_t      *srv       = SRV(vsrv->server_ref);

	/* Ensure there is a logger to instance..
	 */
	if (cherokee_buffer_is_empty (&config->val)) {
		return ret_ok;
	}

	/* Instance a new logger
	 */
	ret = cherokee_plugin_loader_get (&srv->loader, config->val.buf, &info);
	if (ret < ret_ok) {
		LOG_CRITICAL (CHEROKEE_ERROR_VSERVER_LOAD_MODULE,
				config->val.buf, vsrv->priority);
		return ret_error;
	}

	func_new = (logger_func_new_t) info->instance;
	if (func_new == NULL)
		return ret_error;

	ret = func_new ((void **) logger, vsrv, config);
	if (ret != ret_ok)
		return ret;

	return ret_ok;
}

ret_t
cherokee_logger_multi_new (cherokee_logger_t          **logger,
			   cherokee_virtual_server_t  *vsrv,
			   cherokee_config_node_t     *config)
{
	ret_t ret;
	cherokee_list_t *i;
	int j;
	cherokee_config_node_t *children_conf;
	CHEROKEE_NEW_STRUCT(n, logger_multi);

	/* Init the base class object
	 */
	cherokee_logger_init_base (LOGGER(n), PLUGIN_INFO_PTR(multi), config);

	MODULE(n)->init         = (logger_func_init_t) cherokee_logger_multi_init;
	MODULE(n)->free         = (logger_func_free_t) cherokee_logger_multi_free;
	LOGGER(n)->flush        = (logger_func_flush_t) cherokee_logger_multi_flush;
	LOGGER(n)->reopen       = (logger_func_flush_t) cherokee_logger_multi_reopen;
	LOGGER(n)->write_access = (logger_func_write_access_t) cherokee_logger_multi_write_access;

	n->n_children = 0;

	/* Get config
	 */
	ret = cherokee_config_node_get (config, "child", &children_conf);
	if (ret != ret_ok) {
		LOG_CRITICAL (CHEROKEE_ERROR_LOGGER_NO_KEY, "child");
		goto error;
	}

	/* Create all sub-loggers
	 */
	cherokee_config_node_foreach (i, children_conf) {
		cherokee_config_node_t *child_subconf = CONFIG_NODE(i);
		cherokee_logger_t *child_logger;

		ret = create_sub_logger(&n->child[n->n_children], child_subconf, vsrv);
		if (ret != ret_ok)
			goto error;

		n->n_children++;

		if (n->n_children == LOG_MULTI_MAX_CHILDREN)
			break;
	}

	/* Return the object
	 */
	*logger = LOGGER(n);
	return ret_ok;

error:
	for (j=0; j<n->n_children; j++)
		cherokee_logger_free (n->child[j]);

	cherokee_logger_free (LOGGER(n));
	return ret_error;
}

ret_t
cherokee_logger_multi_init (cherokee_logger_multi_t *logger)
{
	ret_t ret = ret_ok;
	int i;

	for (i=0; i<logger->n_children; i++)
		if (cherokee_logger_init (logger->child[i]) != ret_ok)
			ret = ret_error;

	return ret;
}

ret_t
cherokee_logger_multi_free (cherokee_logger_multi_t *logger)
{
	ret_t ret = ret_ok;
	int i;

	for (i=0; i<logger->n_children; i++)
		if (cherokee_logger_free (logger->child[i]) != ret_ok)
			ret = ret_error;

	return ret;
}

ret_t
cherokee_logger_multi_flush (cherokee_logger_multi_t *logger)
{
	ret_t ret = ret_ok;
	int i;

	for (i=0; i<logger->n_children; i++)
		if (cherokee_logger_flush (logger->child[i]) != ret_ok)
			ret = ret_error;

	return ret;
}

ret_t
cherokee_logger_multi_reopen (cherokee_logger_multi_t *logger)
{
	ret_t ret = ret_ok;
	int i;

	for (i=0; i<logger->n_children; i++)
		if (cherokee_logger_reopen (logger->child[i]) != ret_ok)
			ret = ret_error;

	return ret;
}

ret_t
cherokee_logger_multi_write_access (cherokee_logger_multi_t *logger,
				    cherokee_connection_t *conn)
{
	ret_t ret = ret_ok;
	int i;

	for (i=0; i<logger->n_children; i++)
		if (cherokee_logger_write_access (logger->child[i], conn) != ret_ok)
			ret = ret_error;

	return ret;
}
