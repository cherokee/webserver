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
#include <signal.h>
#include "server.h"

#ifdef HAVE_GETOPT_H
# include <getopt.h>
#endif

#define GETOPT_OPT           "d:p:aC:"
#define CONFIG_FILE_HELP     "[-d DIR] [-p PORT] [-C FILE] [-a]"

#define DEFAULT_PORT         9090
#define DEFAULT_DOCUMENTROOT CHEROKEE_DATADIR "/admin"
#define DEFAULT_CONFIG_FILE  CHEROKEE_CONFDIR "/cherokee.conf"
 
static int                 port          = DEFAULT_PORT;
static char               *document_root = DEFAULT_DOCUMENTROOT;
static char               *config_file   = DEFAULT_CONFIG_FILE;
static cherokee_boolean_t  bind_local    = true;


static ret_t
config_server (cherokee_server_t *srv) 
{
	ret_t             ret;
	cherokee_buffer_t buf = CHEROKEE_BUF_INIT;

	cherokee_buffer_add_va  (&buf, "server!port = %d\n", port);
	cherokee_buffer_add_str (&buf, "server!ipv6 = 0\n");
	cherokee_buffer_add_str (&buf, "server!max_connection_reuse = 0\n");

	if (bind_local)
		cherokee_buffer_add_str (&buf, "server!listen = 127.0.0.1\n");

	cherokee_buffer_add_va  (&buf, "vserver!default!document_root = %s\n", document_root);

	cherokee_buffer_add_str (&buf, "vserver!default!directory!/about!handler = server_info\n");
	cherokee_buffer_add_str (&buf, "vserver!default!directory!/about!priority = 2\n");

	cherokee_buffer_add_str (&buf, "vserver!default!directory!/static!handler = file\n");
	cherokee_buffer_add_str (&buf, "vserver!default!directory!/static!handler!iocache = 0\n");
	cherokee_buffer_add_str (&buf, "vserver!default!directory!/static!priority = 3\n");

	cherokee_buffer_add_str (&buf, "vserver!default!request!^/favicon.ico$!handler = file\n");
	cherokee_buffer_add_str (&buf, "vserver!default!request!^/favicon.ico$!priority = 4\n");

	cherokee_buffer_add_str (&buf, "vserver!default!directory!/icons_local!handler = file\n");
	cherokee_buffer_add_str (&buf, "vserver!default!directory!/icons_local!handler!iocache = 0\n");
	cherokee_buffer_add_va  (&buf, "vserver!default!directory!/icons_local!document_root = %s\n", CHEROKEE_ICONSDIR);
	cherokee_buffer_add_str (&buf, "vserver!default!directory!/icons_local!priority = 5\n");

	cherokee_buffer_add_str (&buf, "vserver!default!directory!/!handler = scgi\n");
	cherokee_buffer_add_str (&buf, "vserver!default!directory!/!handler!balancer = round_robin\n");
	cherokee_buffer_add_str (&buf, "vserver!default!directory!/!handler!balancer!type = interpreter\n");
	cherokee_buffer_add_str (&buf, "vserver!default!directory!/!handler!balancer!local1!host = localhost:4000\n");
	cherokee_buffer_add_va  (&buf, "vserver!default!directory!/!handler!balancer!local1!interpreter = python %s/server.py %s\n", document_root, config_file);
	cherokee_buffer_add_str (&buf, "vserver!default!directory!/!priority = 1000\n");

	ret = cherokee_server_read_config_string (srv, &buf);
	if (ret != ret_ok) return ret;

	cherokee_buffer_mrproper (&buf);
	return ret_ok;
}


static void
process_parameters (int argc, char **argv)
{
	int c;

	while ((c = getopt(argc, argv, GETOPT_OPT)) != -1) {
		switch(c) {
		case 'a':
			bind_local = false;
			break;
		case 'p':
			port = atoi(optarg);
			break;
		case 'd':
			document_root = strdup(optarg);
			break;
		case 'C':
			config_file = strdup(optarg);
			break;
		default:
			fprintf (stderr, "Usage: %s " CONFIG_FILE_HELP "\n", argv[0]);
			exit(1);
		}
	}
}


int
main (int argc, char **argv)
{
	ret_t              ret;
	cherokee_server_t *srv;

#ifdef _WIN32
	init_win32();
#endif	

#ifdef SIGPIPE
        signal (SIGPIPE, SIG_IGN);
#endif

	process_parameters (argc, argv);

	ret = cherokee_server_new (&srv);
	if (ret != ret_ok) return 1;

	ret = config_server (srv);
	if (ret != ret_ok) return 2;

	ret = cherokee_server_initialize (srv);
	if (ret != ret_ok) return 3;

	for (;;) {
		cherokee_server_step (srv);
	}
	
	cherokee_server_stop (srv);
	cherokee_server_free (srv);

	return 0;
}
