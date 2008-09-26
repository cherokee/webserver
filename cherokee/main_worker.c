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
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "init.h"
#include "server.h"
#include "info.h"
#include "server-protected.h"
#include "util.h"

#ifdef HAVE_GETOPT_LONG
# include <getopt.h>
#else 
# include "getopt/getopt.h"
#endif

/* Notices 
 */
#define APP_NAME        \
	"Cherokee Web Server"

#define APP_COPY_NOTICE \
	"Written by Alvaro Lopez Ortega <alvaro@gnu.org>\n\n"                          \
	"Copyright (C) 2001-2008 Alvaro Lopez Ortega.\n"                               \
	"This is free software; see the source for copying conditions.  There is NO\n" \
	"warranty; not even for MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.\n"


/* Default configuration
 */
#define DEFAULT_CONFIG_FILE CHEROKEE_CONFDIR "/cherokee.conf"

#define BASIC_CONFIG							\
	"vserver!1!nick = default\n"					\
	"vserver!1!rule!1!match = default\n"				\
	"vserver!1!rule!1!handler = common\n"				\
	"vserver!1!rule!1!handler!iocache = 0\n"			\
	"vserver!1!rule!2!match = directory\n"			\
	"vserver!1!rule!2!match!directory = /icons\n"			\
	"vserver!1!rule!2!handler = file\n"				\
	"vserver!1!rule!2!document_root = " CHEROKEE_ICONSDIR "\n"	\
	"vserver!1!rule!3!match = directory\n"			\
	"vserver!1!rule!3!match!directory = /cherokee_themes\n"	\
	"vserver!1!rule!3!handler = file\n"				\
	"vserver!1!rule!3!document_root = " CHEROKEE_THEMEDIR "\n"	\
	"icons!default = page_white.png\n"				\
	"icons!directory = folder.png\n"				\
	"icons!parent_directory = arrow_turn_left.png\n"		\
	"try_include = " CHEROKEE_CONFDIR "/mods-enabled\n"

static cherokee_server_t  *srv           = NULL;
static char               *config_file   = NULL;
static char               *document_root = NULL;
static cherokee_boolean_t  daemon_mode   = false;
static cherokee_boolean_t  just_test     = false;
static cherokee_boolean_t  print_modules = false;
static cuint_t             port          = 80;

static ret_t common_server_initialization (cherokee_server_t *srv);


static void
wait_process (pid_t pid)
{
	int re;
	int exitcode;

	while (true) {
		re = waitpid (pid, &exitcode, 0);
		if (re > 0)
			break;
		else if (errno == ECHILD) 
			break;
		else if (errno == EINTR) 
			continue;
		else {
			PRINT_ERRNO (errno, "ERROR: waitting PID %d: ${errno}\n", pid);
			return;
		}
	}
}

static void 
signals_handler (int sig, siginfo_t *si, void *context) 
{
	UNUSED(context);

	switch (sig) {
	case SIGHUP:
		printf ("Handling Graceful Restart..\n");
		cherokee_server_handle_HUP (srv);
		break;

	case SIGUSR2:
		printf ("Reopening log files..\n");
		cherokee_server_log_reopen (srv);
		break;

	case SIGINT:
	case SIGTERM:
		printf ("Server is exiting..\n");
		cherokee_server_handle_TERM (srv);
		break;

	case SIGCHLD:
		wait_process (si->si_pid);
		break;

	case SIGSEGV:
#ifdef SIGBUS
	case SIGBUS:
#endif
		cherokee_server_handle_panic (srv);
		break;

	default:
		PRINT_ERROR ("Unknown signal: %d\n", sig);
	}
}


static ret_t
test_configuration_file (void)
{
	ret_t  ret;
	char  *config;

	config = (config_file) ? config_file : DEFAULT_CONFIG_FILE;
	ret = cherokee_server_read_config_file (srv, config);

	PRINT_MSG ("Test on %s: %s\n", config, (ret == ret_ok)? "OK": "Failed"); 
	return ret;
}


static ret_t
common_server_initialization (cherokee_server_t *srv)
{
	ret_t            ret;
	struct sigaction act;

	/* Signals it handles
	 */
	memset(&act, 0, sizeof(act));

	/* SIGPIPE */
	act.sa_handler = SIG_IGN;
	sigaction (SIGPIPE, &act, NULL);

	/* Signal Handler */
	act.sa_sigaction = signals_handler;
	act.sa_flags     = SA_SIGINFO;

	sigaction (SIGHUP,  &act, NULL);
	sigaction (SIGUSR2, &act, NULL);
	sigaction (SIGSEGV, &act, NULL);
	sigaction (SIGTERM, &act, NULL);
	sigaction (SIGINT,  &act, NULL);
	sigaction (SIGCHLD, &act, NULL);
#ifdef SIGBUS
	sigaction (SIGBUS,  &act, NULL);
#endif

	if (document_root != NULL) {
		cherokee_buffer_t tmp   = CHEROKEE_BUF_INIT;
		cherokee_buffer_t droot = CHEROKEE_BUF_INIT;

		/* Sanity check
		 */
		if (port > 0xFFFF) {
			PRINT_ERROR ("Port %d is out of limits\n", port);
			return ret_error;
		}

		/* Build the configuration string
		 */
		cherokee_buffer_add (&droot, document_root, strlen(document_root));
		cherokee_path_arg_eval (&droot);

		cherokee_buffer_add_va (&tmp, 
					"server!port = %d\n"
					"vserver!1!document_root = %s\n"
					BASIC_CONFIG, port, droot.buf);

		/* Apply it
		 */
		ret = cherokee_server_read_config_string (srv, &tmp);

		cherokee_buffer_mrproper (&tmp);
		cherokee_buffer_mrproper (&droot);

		if (ret != ret_ok) {
			PRINT_MSG ("Couldn't start serving directory %s\n", document_root);
			return ret_error;
		}

	} else {
		char *config;
		
		/* Read the configuration file
		 */
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
print_help (void)
{
	printf (APP_NAME "\n"
		"Usage: cherokee [options]\n\n"
		"  -h,  --help                   Print this help\n"
		"  -V,  --version                Print version and exit\n"
		"  -t,  --test                   Just test configuration file\n"
		"  -d,  --detach                 Detach from the console\n"
		"  -C,  --config=PATH            Configuration file\n"
		"  -p,  --port=NUM               TCP port number\n"
		"  -r,  --documentroot=PATH      Server directory content\n"
		"  -i,  --print-server-info      Print server technical information\n\n"
		"Report bugs to " PACKAGE_BUGREPORT "\n");
}

static ret_t
process_parameters (int argc, char **argv)
{
	int c;

	/* If any of these parameters change, main_guardian may need
	 * to be updated.
	 */
	struct option long_options[] = {
		{"help",              no_argument,       NULL, 'h'},
		{"version",           no_argument,       NULL, 'V'},
		{"detach",            no_argument,       NULL, 'd'},
		{"test",              no_argument,       NULL, 't'},
		{"print-server-info", no_argument,       NULL, 'i'},
		{"port",              required_argument, NULL, 'p'},
		{"documentroot",      required_argument, NULL, 'r'},
		{"config",            required_argument, NULL, 'C'},
		{NULL, 0, NULL, 0}
	};

	while ((c = getopt_long(argc, argv, "hVdtip:r:C:", long_options, NULL)) != -1) {
		switch(c) {
		case 'C':
			config_file = strdup(optarg);
			break;
		case 'd':
			daemon_mode = true;
			break;
		case 'r':
			document_root = strdup(optarg);
			break;
		case 'p':
			port = atoi(optarg);
			break;
		case 't':
			just_test = true;
			break;
		case 'i':
			print_modules = true;
			break;
		case 'V':
			printf (APP_NAME " " PACKAGE_VERSION "\n" APP_COPY_NOTICE);
			return ret_eof;
		case 'h':
		case '?':
		default:
			print_help();
			return ret_eof;
		}
	}
	return ret_ok;
}


int
main (int argc, char **argv)
{
	ret_t ret;

	cherokee_init();

	ret = cherokee_server_new (&srv);
	if (ret < ret_ok) return 1;
	
	ret = process_parameters (argc, argv);
	if (ret != ret_ok) 
		exit (EXIT_OK_ONCE);

	if (print_modules) {
		cherokee_info_build_print (srv);
		exit (EXIT_OK_ONCE);
	}

	if (just_test) {
		ret = test_configuration_file();
		if (ret != ret_ok) 
			exit(EXIT_ERROR);
		exit (EXIT_OK_ONCE);
	}

	ret = common_server_initialization (srv);
	if (ret < ret_ok) return 2;

	do {
		ret = cherokee_server_step (srv);
	} while (ret == ret_eagain);

	cherokee_server_stop (srv);
	cherokee_server_free (srv);

	if (config_file)
		free (config_file);

	return EXIT_OK;
}
