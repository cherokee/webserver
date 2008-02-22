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

#define DEFAULT_CONFIG_FILE CHEROKEE_CONFDIR "/cherokee.conf"

#define GETOPT_OPT  "C:r:bhvg"
#define CONFIG_FILE_HELP "[-C configfile] [-r] [-g]"

#define BASIC_CONFIG                                                                         \
	"vserver!default!directory!/!handler = common\n"                                     \
	"vserver!default!directory!/!handler!iocache = 0\n"                                  \
	"vserver!default!directory!/!priority = 1\n"                                         \
	"vserver!default!directory!/icons!handler = file\n"                                  \
	"vserver!default!directory!/icons!document_root = " CHEROKEE_ICONSDIR "\n"           \
	"vserver!default!directory!/icons!priority = 2\n"                                    \
	"vserver!default!directory!/cherokee_themes!handler = file\n"                        \
	"vserver!default!directory!/cherokee_themes!document_root = " CHEROKEE_THEMEDIR "\n" \
	"vserver!default!directory!/cherokee_themes!priority = 3\n"                          \
	"icons!default = page_white.png\n"                                                   \
	"icons!directory = folder.png\n"      	                                             \
	"icons!parent_directory = arrow_turn_left.png\n"                                     \
	"try_include = " CHEROKEE_CONFDIR "/mods-enabled\n"

static cherokee_server_t  *srv           = NULL;
static char               *config_file   = NULL;
static char               *document_root = NULL;
static cherokee_boolean_t  daemon_mode   = false;

static ret_t common_server_initialization (cherokee_server_t *srv);


static void
panic_handler (int code)
{
	cherokee_server_handle_panic (srv);
}

static void
prepare_to_die (int code)
{
	cherokee_server_handle_TERM (srv);
}

static void
restart_server_cb (cherokee_server_t *new_srv)
{
	ret_t ret;

	srv = new_srv;
	ret = common_server_initialization (srv);
	if (ret != ret_ok) exit(3);
}

static void
restart_server (int code)
{	
	printf ("Handling HUP signal..\n");
	cherokee_server_handle_HUP (srv, restart_server_cb);
}


static ret_t
common_server_initialization (cherokee_server_t *srv)
{
	ret_t ret;

#ifdef SIGPIPE
        signal (SIGPIPE, SIG_IGN);
#endif
#ifdef SIGHUP
        signal (SIGHUP,  restart_server);
#endif
#ifdef SIGSEGV
        signal (SIGSEGV, panic_handler);
#endif
#ifdef SIGTERM
        signal (SIGTERM, prepare_to_die);
#endif

	if (document_root != NULL) {
		cherokee_buffer_t tmp = CHEROKEE_BUF_INIT;

		cherokee_buffer_add_va (&tmp, "vserver!default!document_root = %s\n"
					BASIC_CONFIG, document_root);

		ret = cherokee_server_read_config_string (srv, &tmp);
		cherokee_buffer_mrproper (&tmp);

		if (ret != ret_ok) {
			PRINT_MSG ("Couldn't start serving directory %s\n", document_root);
			return ret_error;
		}

	} else {
		char *config;

		config = (config_file) ? config_file : DEFAULT_CONFIG_FILE;
		ret = cherokee_server_read_config_file (srv, config);

		if (ret != ret_ok) {
			PRINT_MSG ("Couldn't read the config file: %s\n", config);
			return ret_error;
		}
	}
		
	if (daemon_mode)
		cherokee_server_daemonize (srv);

	cherokee_server_write_pidfile (srv);

	ret = cherokee_server_initialize (srv);
	if (ret != ret_ok) return ret_error;

	cherokee_server_unlock_threads (srv);
	return ret_ok;
}


static void
process_parameters (int argc, char **argv)
{
	int c;

	while ((c = getopt(argc, argv, GETOPT_OPT)) != -1) {
		switch(c) {
		case 'C':
			config_file = strdup(optarg);
			break;
		case 'b':
			daemon_mode = true;
			break;
		case 'r':
			document_root = strdup(optarg);
			break;
		case 'v':
			fprintf (stdout, "%s\n", PACKAGE_STRING);
			exit(1);
			break;
		case 'h':
		default:
			fprintf (stderr, "Usage: %s " CONFIG_FILE_HELP " [-b] -h -v\n", argv[0]);
			exit(1);
		}
	}
}


int
main (int argc, char **argv)
{
	ret_t ret;

#ifdef _WIN32
	init_win32();
#endif	

	ret = cherokee_server_new (&srv);
	if (ret < ret_ok) return 1;
	
	process_parameters (argc, argv);

	ret = common_server_initialization (srv);
	if (ret < ret_ok) return 2;

	do {
		ret = cherokee_server_step (srv);
	} while (ret == ret_eagain);
	
	cherokee_server_stop (srv);
	cherokee_server_free (srv);

	if (config_file)
		free (config_file);

	return 0;
}
