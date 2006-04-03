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

#include "common-internal.h"

#include "virtual_server.h"
#include "server-protected.h"
#include "config_entry.h"
#include "list_ext.h"
#include "typed_table.h"


#define DEFAULT_PORT         9090
#define DEFAULT_DOCUMENTROOT CHEROKEE_DATADIR "/admin/"


static ret_t
config_server (cherokee_server_t *srv) 
{
	ret_t                      ret;
	cherokee_virtual_server_t *vserver;
	cherokee_module_info_t    *info;
	cherokee_config_entry_t   *entry;

	/* Server: Port, DocumentRoot and DirectoryIndex
	 */
	srv->port      = DEFAULT_PORT;
	srv->ipv6      = false;
	srv->listen_to = strdup("127.0.0.1");

	vserver = srv->vserver_default;
	cherokee_virtual_server_set_documentroot (vserver, DEFAULT_DOCUMENTROOT);

	cherokee_list_add_tail (&vserver->index_list, "index.php");
	
	/* Default handler
	 */
	cherokee_module_loader_load (&srv->loader, "common");
	cherokee_module_loader_get_info (&srv->loader, "common", &info);
	
	cherokee_config_entry_new (&entry);
	cherokee_config_entry_set_handler (entry, info);
	entry->priority = CHEROKEE_CONFIG_PRIORITY_DEFAULT;

	vserver->default_handler = entry;

	/* About
	 */
	cherokee_module_loader_load (&srv->loader, "server_info");
	cherokee_module_loader_get_info (&srv->loader, "server_info", &info);

	cherokee_config_entry_new (&entry);
	cherokee_config_entry_set_handler (entry, info);
	entry->priority = CHEROKEE_CONFIG_PRIORITY_DEFAULT + 1;
	
	cherokee_dirs_table_add (&vserver->dirs, "/about", entry);

	/* PHP extension
	 */
	cherokee_module_loader_load (&srv->loader, "phpcgi");
	cherokee_module_loader_get_info (&srv->loader, "phpcgi", &info);

	cherokee_config_entry_new (&entry);
	cherokee_config_entry_set_handler (entry, info);
	entry->priority = CHEROKEE_CONFIG_PRIORITY_DEFAULT + 2;

	ret = cherokee_exts_table_new (&vserver->exts);
	if (ret != ret_ok) return ret;

	cherokee_exts_table_add (vserver->exts, "php", entry);
	
	return ret_ok;
}


int
main (int argc, char **argv)
{
	ret_t              ret;
	cherokee_server_t *srv;

	ret = cherokee_server_new (&srv);
	if (ret != ret_ok) return 1;

	ret = config_server (srv);
	if (ret != ret_ok) return 2;

	ret = cherokee_server_init (srv);
	if (ret != ret_ok) return 3;

	for (;;) {
		cherokee_server_step (srv);
	}
	
	cherokee_server_free (srv);
	return 0;
}
