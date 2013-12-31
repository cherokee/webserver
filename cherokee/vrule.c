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
#include "vrule.h"
#include "virtual_server.h"
#include "util.h"

#define ENTRIES "vrule"

ret_t
cherokee_vrule_init_base (cherokee_vrule_t       *vrule,
                          cherokee_plugin_info_t *info)
{
	cherokee_module_init_base (MODULE(vrule), NULL, info);
	INIT_LIST_HEAD (&vrule->list_node);

	vrule->virtual_server = NULL;
	vrule->match          = NULL;
	vrule->priority       = CHEROKEE_VRULE_PRIO_NONE;

	return ret_ok;
}


ret_t
cherokee_vrule_free (cherokee_vrule_t *vrule)
{
	if (MODULE(vrule)->free) {
		MODULE(vrule)->free (vrule);
	}

	free (vrule);
	return ret_ok;
}


static ret_t
configure_base (cherokee_vrule_t       *vrule,
                cherokee_config_node_t *conf)
{
	UNUSED(vrule);
	UNUSED(conf);

	return ret_ok;
}


ret_t
cherokee_vrule_configure (cherokee_vrule_t       *vrule,
                          cherokee_config_node_t *conf,
                          void                   *vsrv)
{
	ret_t ret;

	return_if_fail (vrule, ret_error);
	return_if_fail (vrule->configure, ret_error);

	ret = configure_base (vrule, conf);
	if (ret != ret_ok) return ret;

	/* Call the real method
	 */
	return vrule->configure (vrule, conf, VSERVER(vsrv));
}


ret_t
cherokee_vrule_match (cherokee_vrule_t  *vrule,
                      cherokee_buffer_t *buffer,
                      void              *conn)
{
	return_if_fail (vrule, ret_error);
	return_if_fail (vrule->match, ret_error);

	/* Call the real method
	 */
	return vrule->match (vrule, buffer, conn);
}

