/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/* Cherokee
 *
 * Authors:
 *      Alvaro Lopez Ortega <alvaro@alobbs.com>
 *
 * Copyright (C) 2001-2006 Alvaro Lopez Ortega
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

#include "read_config_embedded.h"

#include "module.h"
#include "dirs_table.h"
#include "server-protected.h"
#include "virtual_server.h"

#define DEFAULT_DOCUMENTROOT "/var/www/"


ret_t
cherokee_embedded_read_config (cherokee_server_t *srv)
{
	   ret_t                        ret;
	   cherokee_module_info_t      *info;
	   cherokee_config_entry_t     *entry;
	   cherokee_virtual_server_t   *vserver;

	   vserver = srv->vserver_default;

	   /* Root directory
	    */
	   cherokee_buffer_add_str (vserver->root, DEFAULT_DOCUMENTROOT);

	   /* Default handler
	    */
	   cherokee_module_loader_load (&srv->loader, "common");
	   cherokee_module_loader_get_info (&srv->loader, "common", &info);
	   
	   cherokee_config_entry_new (&entry);
	   cherokee_config_entry_set_handler (entry, info);

	   vserver->default_handler = entry;
	   vserver->default_handler->priority = CHEROKEE_CONFIG_PRIORITY_DEFAULT;
	   
	   return ret_ok;
}
