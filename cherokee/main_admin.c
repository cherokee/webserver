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
	ret_t             ret;
	cherokee_buffer_t buf = CHEROKEE_BUF_INIT;

	cherokee_buffer_add_va  (&buf, "server!port = %d\n", DEFAULT_PORT);
	cherokee_buffer_add_str (&buf, "server!ipv6 = 0\n");
	cherokee_buffer_add_str (&buf, "server!listen = 127.0.0.1\n");
	cherokee_buffer_add_str (&buf, "server!max_connection_reuse = 0\n");

	cherokee_buffer_add_va  (&buf, "vserver!default!document_root = %s\n", DEFAULT_DOCUMENTROOT);
	cherokee_buffer_add_str (&buf, "vserver!default!directory_index = index.php\n");
	cherokee_buffer_add_str (&buf, "vserver!default!directory!/!handler = common\n");
	cherokee_buffer_add_str (&buf, "vserver!default!directory!/!priority = 1\n");

	cherokee_buffer_add_str (&buf, "vserver!default!directory!/about!handler = server_info\n");
	cherokee_buffer_add_str (&buf, "vserver!default!directory!/about!priority = 2\n");

	cherokee_buffer_add_str (&buf, "vserver!default!extensions!php!handler = phpcgi\n");
	cherokee_buffer_add_str (&buf, "vserver!default!extensions!php!priority = 3\n");

	ret = cherokee_server_read_config_string (srv, &buf);
	if (ret != ret_ok) return ret;

	cherokee_buffer_mrproper (&buf);
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
