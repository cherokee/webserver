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
#include "init.h"
#include "server.h"

#ifdef HAVE_GETOPT_H
# include <getopt.h>
#endif

#define APP_NAME        \
	"Cherokee Web Server: Admin"

#define APP_COPY_NOTICE \
	"Written by Alvaro Lopez Ortega <alvaro@gnu.org>\n\n"                          \
	"Copyright (C) 2001-2008 Alvaro Lopez Ortega.\n"                               \
	"This is free software; see the source for copying conditions.  There is NO\n" \
	"warranty; not even for MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.\n"

#define DEFAULT_PORT         9090
#define DEFAULT_DOCUMENTROOT CHEROKEE_DATADIR "/admin"
#define DEFAULT_CONFIG_FILE  CHEROKEE_CONFDIR "/cherokee.conf"
#define DEFAULT_BIND         "127.0.0.1"
#define RULE                 "vserver!default!rule!"
 
static int   port          = DEFAULT_PORT;
static char *document_root = DEFAULT_DOCUMENTROOT;
static char *config_file   = DEFAULT_CONFIG_FILE;
static char *bind_to       = DEFAULT_BIND;

static ret_t
config_server (cherokee_server_t *srv) 
{
	ret_t             ret;
	cherokee_buffer_t buf = CHEROKEE_BUF_INIT;

	cherokee_buffer_add_va  (&buf, "server!port = %d\n", port);
	cherokee_buffer_add_str (&buf, "server!ipv6 = 0\n");
	cherokee_buffer_add_str (&buf, "server!max_connection_reuse = 0\n");

	if (bind_to)
		cherokee_buffer_add_va (&buf, "server!listen = %s\n", bind_to);

	cherokee_buffer_add_va  (&buf, "vserver!default!document_root = %s\n", document_root);

	cherokee_buffer_add_va  (&buf, 
				 RULE "1!match = default\n"
				 RULE "1!handler = scgi\n"
				 RULE "1!handler!balancer = round_robin\n"
				 RULE "1!handler!balancer!type = interpreter\n"
				 RULE "1!handler!balancer!local1!host = localhost:4000\n"
				 RULE "1!handler!balancer!local1!interpreter = %s/server.py %s\n", document_root, config_file);

	cherokee_buffer_add_str (&buf, 
				 RULE "2!match = directory\n"
				 RULE "2!match!directory = /about\n"
				 RULE "2!handler = server_info\n");

	cherokee_buffer_add_str (&buf, 
				 RULE "3!match = directory\n"
				 RULE "3!match!directory = /static\n"
				 RULE "3!handler = file\n"
				 RULE "3!handler!iocache = 0\n");

	cherokee_buffer_add_str (&buf, 
				 RULE "4!match = request\n"
				 RULE "4!match!request = ^/favicon.ico$\n"
				 RULE "4!handler = file\n");

	cherokee_buffer_add_va  (&buf, 
				 RULE "5!match = directory\n"
				 RULE "5!match!directory = /icons_local\n"
				 RULE "5!handler = file\n"
				 RULE "5!handler!iocache = 0\n"
				 RULE "5!document_root = %s\n", CHEROKEE_ICONSDIR);

	ret = cherokee_server_read_config_string (srv, &buf);
	if (ret != ret_ok) return ret;

	cherokee_buffer_mrproper (&buf);
	return ret_ok;
}

static void
print_help (void)
{
	printf (APP_NAME "\n"
		"Usage: cherokee-admin [options]\n\n"
		"  -h,  --help                   Print this help\n"
		"  -V,  --version                Print version and exit\n"
		"  -l,  --listen[=IP]            Bind net iface; no arg means all\n"
		"  -d,  --appdir=DIR             Application directory\n"
		"  -p,  --port=NUM               TCP port\n"
		"  -C,  --target=PATH            Configuration file to modify\n\n"
		"Report bugs to " PACKAGE_BUGREPORT "\n");
}

static void
process_parameters (int argc, char **argv)
{
	int c;

	struct option long_options[] = {
		{"help",    no_argument,       NULL, 'h'},
		{"version", no_argument,       NULL, 'V'},
		{"bind",    optional_argument, NULL, 'b'},
		{"appdir",  required_argument, NULL, 'd'},
		{"port",    required_argument, NULL, 'p'},
		{"target",  required_argument, NULL, 'C'},
		{NULL, 0, NULL, 0}
	};

	while ((c = getopt_long(argc, argv, "hVb::d:p:C:", long_options, NULL)) != -1) {
		switch(c) {
		case 'b':
			if (optarg)
				bind_to = strdup(optarg);
			else
				bind_to = NULL;
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
		case 'V':
			printf (APP_NAME " " PACKAGE_VERSION "\n" APP_COPY_NOTICE);
			exit(0);
		case 'h':
		case '?':
		default:
			print_help();
			exit(0);
		}
	}
}


int
main (int argc, char **argv)
{
	ret_t              ret;
	cherokee_server_t *srv;

#ifdef SIGPIPE
        signal (SIGPIPE, SIG_IGN);
#endif

	cherokee_init();
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
