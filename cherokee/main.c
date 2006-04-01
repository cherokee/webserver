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

#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <string.h>

#ifdef HAVE_GETOPT_H
# include <getopt.h>
#endif

#include "server.h"

#ifndef CHEROKEE_EMBEDDED
# define GETOPT_OPT  "C:b"
# define CONFIG_FILE "[-C configfile] "
#else
# define GETOPT_OPT  "b"
# define CONFIG_FILE ""
#endif 


static cherokee_server_t  *srv         = NULL;
static char               *config_file = NULL;
static cherokee_boolean_t  daemon_mode = false;

static ret_t common_server_initialization (cherokee_server_t *srv);


static void
panic_handler (int code)
{
	cherokee_server_handle_panic (srv);
}


static void
restart_server_cb (cherokee_server_t *new_srv)
{
	srv = new_srv;
	common_server_initialization (srv);
}


static void
restart_server (int code)
{	
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

	ret = cherokee_server_read_config_file (srv, config_file);
	if (ret != ret_ok) {
		PRINT_MSG_S ("Couldn't read the config file\n");
		return ret_error;
	}
		
	ret = cherokee_server_init (srv);
	if (ret != ret_ok) return ret_error;

	if (daemon_mode) {
		cherokee_server_daemonize (srv);
	}

	cherokee_server_write_pidfile (srv);
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

		default:
			fprintf (stderr, "Usage: %s " CONFIG_FILE "[-b]\n", argv[0]);
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

	for (;;) {
		cherokee_server_step (srv);
	}
	
	cherokee_server_free (srv);

	if (config_file)
		free (config_file);

	return 0;
}
