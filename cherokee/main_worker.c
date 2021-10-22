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

#include <signal.h>
#include <unistd.h>
#include <sys/types.h>

#include "init.h"
#include "server.h"
#include "info.h"
#include "server-protected.h"
#include "services.h"
#include "util.h"

#ifdef HAVE_SYS_WAIT_H
# include <sys/wait.h>
#endif

#ifdef HAVE_GETOPT_LONG
# include <getopt.h>
#else
# include "getopt/getopt.h"
#endif

#ifdef HAVE_OPENSSL
# include <openssl/ssl.h>
# include "pcre/pcre.h"
#endif

/* Notices
 */
#define APP_NAME        \
	"Cherokee Web Server"

#define APP_COPY_NOTICE \
	"Written by Alvaro Lopez Ortega <alvaro@alobbs.com>\n\n"                       \
	"Copyright (C) 2001-2014 Alvaro Lopez Ortega.\n"                               \
	"This is free software; see the source for copying conditions.  There is NO\n" \
	"warranty; not even for MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.\n"


/* Default configuration
 */
#define DEFAULT_CONFIG_FILE CHEROKEE_CONFDIR "/cherokee.conf"

#define BASIC_CONFIG                                               \
	"vserver!1!nick = default\n"                               \
	"vserver!1!error_writer!type = stderr\n"                   \
	"vserver!1!rule!3!match = directory\n"                     \
	"vserver!1!rule!3!match!directory = /cherokee_themes\n"    \
	"vserver!1!rule!3!handler = file\n"                        \
	"vserver!1!rule!3!document_root = " CHEROKEE_THEMEDIR "\n" \
	"vserver!1!rule!2!match = directory\n"                     \
	"vserver!1!rule!2!match!directory = /cherokee_icons\n"     \
	"vserver!1!rule!2!handler = file\n"                        \
	"vserver!1!rule!2!document_root = " CHEROKEE_ICONSDIR "\n" \
	"vserver!1!rule!1!match = default\n"                       \
	"vserver!1!rule!1!handler = common\n"                      \
	"vserver!1!rule!1!handler!iocache = 0\n"                   \
	"icons!default = page_white.png\n"                         \
	"icons!directory = folder.png\n"                           \
	"icons!parent_directory = arrow_turn_left.png\n"

#define ENTRIES "main"

static cherokee_server_t  *srv           = NULL;
static char               *config_file   = NULL;
static char               *document_root = NULL;
static cherokee_boolean_t  daemon_mode   = false;
static cherokee_boolean_t  just_test     = false;
static cherokee_boolean_t  print_modules = false;
static cherokee_boolean_t  port_set      = false;
static cuint_t             port          = 80;
static int                 services_fd   = -1;

static ret_t common_server_initialization (cherokee_server_t *srv);


#ifdef HAVE_OPENSSL
static int
check_for_tls_protocols(cherokee_buffer_t *buf, const char *regex, int options,
	cherokee_buffer_t *protocol_option_list) {
	pcre *reCompiled;
	const char *pcreErrorStr;
	const char *psubStrMatchStr;
	int pcreErrorOffset;
	int subStrVec[30];
	int match_count = 0;
	int start_offset = 0;
	int j;
	int ret = 0;

	if (!buf)
		goto out;
	reCompiled = pcre_compile(regex, options, &pcreErrorStr,
	                          &pcreErrorOffset, NULL);
	if (reCompiled == NULL) {
		TRACE(ENTRIES, "Regex error compiling '%s': %s\n", regex, pcreErrorStr);
		ret = PCRE_ERROR_NULL;
		goto out;
	}
	while (1) {
		ret = pcre_exec(reCompiled, NULL, buf->buf, buf->len,
			start_offset, 0, subStrVec, 30);
		if (ret < 0) {
			switch (ret) {
				case PCRE_ERROR_NOMATCH:
					ret = match_count;
					break;
				default:
					TRACE(ENTRIES, "Regex matching failed with error code %d\n", ret);
			}
			goto out;
		}
		if (ret == 0) {
			// Too many substrings were found to fit in subStrVec!");
			// Set rc to the max number of substring matches possible.
			ret = 30 / 3;
		}
		pcre_get_substring(buf->buf, subStrVec, ret, 1,
		                   &(psubStrMatchStr));
		if (!cherokee_buffer_is_empty(protocol_option_list)) {
			cherokee_buffer_add_str(protocol_option_list, ",");
		}
		cherokee_buffer_add(protocol_option_list,
		                    psubStrMatchStr, strlen(psubStrMatchStr));
		match_count++;
		pcre_free_substring(psubStrMatchStr);
		start_offset = subStrVec[1];
	}

out:
	if (reCompiled == NULL) {
		pcre_free(reCompiled);
	}
	return ret;
}

/*
 * More and more Linux distributions deactivate insecure SSL/TLS protocols
 * at build time using configure options. OpenSSL 1.1.1 silently blocks the
 * deactivated protocols, but they can still be configured using the libssl
 * API. Build-time deactivation is documented in opensslconf.h, which in
 * general requires libssl-dev package installation. Currently, the only
 * feasible way to identify deactivated SSL/TLS protocols is to try to start
 * an OpenSSL SSL/TLS server with the protocol that is subject to test.
 * OpenSSL will terminate with an error if the protocol has been deactivated
 * at build time,
 */
static ret_t
add_hardcoded_ssl_options (cherokee_server_t *srv)
{
	unsigned int tls_protocols[] = {
#ifdef SSL_OP_NO_SSLv2
	                                SSL_OP_NO_SSLv2,
#endif
#ifdef SSL_OP_NO_SSLv3
	                                SSL_OP_NO_SSLv3,
#endif
#ifdef SSL_OP_NO_TLSv1
	                                SSL_OP_NO_TLSv1,
#endif
#ifdef SSL_OP_NO_TLSv1_1
	                                SSL_OP_NO_TLSv1_1,
#endif
#ifdef SSL_OP_NO_TLSv1_2
	                                SSL_OP_NO_TLSv1_2,
#endif
#ifdef SSL_OP_NO_TLSv1_3
	                                SSL_OP_NO_TLSv1_3,
#endif
	                                0,
	                               };
	const char *openssl_tls_options[] = {
#ifdef SSL_OP_NO_SSLv2
	                                     "-ssl2",
#endif
#ifdef SSL_OP_NO_SSLv3
	                                     "-ssl3",
#endif
#ifdef SSL_OP_NO_TLSv1
	                                     "-tls1",
#endif
#ifdef SSL_OP_NO_TLSv1_1
	                                     "-tls1_1",
#endif
#ifdef SSL_OP_NO_TLSv1_2
	                                     "-tls1_2",
#endif
#ifdef SSL_OP_NO_TLSv1_3
	                                     "-tls1_3",
#endif
	                                     NULL
	                                    };
	const char *pattern = "(-(ssl|tls)[[:digit:]]+_*[[:digit:]]*)";
	FILE *f;
	char tmp[256];
	char *line;
	int retVal, i;
	long options = 0;
	cherokee_boolean_t known_protocol;
	cherokee_buffer_t protocol_option_list = CHEROKEE_BUF_INIT;
	cherokee_buffer_t buf = CHEROKEE_BUF_INIT;
	ret_t ret;

	for (i = 0; openssl_tls_options[i]; i++) {
#ifdef PATH_OPENSSL_BIN
		snprintf(tmp, sizeof (tmp), "%s s_server %s 2>&1",
		         PATH_OPENSSL_BIN,
		         openssl_tls_options[i]);
#else
		snprintf(tmp, sizeof (tmp), "openssl s_server -%s 2>&1",
		         openssl_tls_options[i]);
#endif
		f = popen(tmp, "r");
		if (f == NULL) {
			PRINT_MSG("(critical) Cannot execute '%s'\n", tmp);
			ret = ret_error;
			goto out;
		}
		while (!feof(f)) {
			/* Skip line until it found the version entry
			 */
			line = fgets(tmp, sizeof (tmp), f);
			if (line == NULL)
				continue;
			line = strstr(line, openssl_tls_options[i]);
			if (line == NULL)
				continue;
			options |= tls_protocols[i];
			break;
		}
		pclose(f);
	}
	if (srv->cryptor != NULL) {
		srv->cryptor->hardcoded_ssl_options = options;
	}

#ifdef PATH_OPENSSL_BIN
	snprintf(tmp, sizeof (tmp), "%s s_server -help 2>&1",
	         PATH_OPENSSL_BIN);
#else
	snprintf(tmp, sizeof (tmp), "openssl s_server -help 2>&1");
#endif
	snprintf(tmp, sizeof (tmp), "openssl s_server -help 2>&1");
	f = popen(tmp, "r");
	if (f == NULL) {
		PRINT_MSG("(critical) Cannot execute '%s'\n", tmp);
		ret = ret_error;
		goto out;
	}
	while (!feof(f)) {
		line = fgets(tmp, sizeof (tmp), f);
		if (line == NULL)
			continue;
		cherokee_buffer_add(&buf, line, strlen(line));
	}
	pclose(f);
	retVal = check_for_tls_protocols(&buf, pattern, 0, &protocol_option_list);
	if (retVal > 0) {
		char *begin;
		cherokee_buffer_clean(&buf);
		while ((begin = (char *) strsep(&protocol_option_list.buf, ",")) != NULL) {
			known_protocol = false;
			for (i = 0; openssl_tls_options[i]; i++) {
				if (strstr(openssl_tls_options[i], begin)) {
					known_protocol = true;
					break;
				}
			}
			if (known_protocol == false) {
				if (!cherokee_buffer_is_empty(&buf)) {
					cherokee_buffer_add_str(&buf, ", ");
				}
				cherokee_buffer_add(&buf, (begin + 1), strlen(begin) - 1);
			}
		}
		if (!cherokee_buffer_is_empty(&buf)) {
			LOG_WARNING(CHEROKEE_ERROR_SERVER_UNKNOWN_TLS_PROTOCOL, buf.buf);
		}
	} else {
		PRINT_MSG("ERROR: No TLS protocol options in 'openssl s_server -help' output. "
		          "Please issue a bug report.\n");
	}
	ret = ret_ok;

out:
	cherokee_buffer_mrproper(&buf);
	cherokee_buffer_mrproper(&protocol_option_list);
	return ret;
}
#endif

static void
wait_process (pid_t pid)
{
	int re;
	int exitcode;

	TRACE(ENTRIES",signal", "Handling SIGCHLD, waiting PID %d\n", pid);

	while (true) {
		re = waitpid (pid, &exitcode, 0);
		if (re > 0)
			break;
		else if (errno == ECHILD)
			break;
		else if (errno == EINTR)
			continue;
		else {
			PRINT_ERROR ("ERROR: waiting PID %d, error %d\n", pid, errno);
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
		if (srv->wanna_exit) {
			break;
		}

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
	ret_t ret;

	ret = cherokee_server_read_config_file (srv, config_file);
	PRINT_MSG ("Test on %s: %s\n", config_file, (ret == ret_ok)? "OK": "Failed");

	return ret;
}


static ret_t
fake_tls_server_initialization (cherokee_server_t *srv)
{
	ret_t            ret;

	cherokee_buffer_t tmp   = CHEROKEE_BUF_INIT;
	cherokee_buffer_t droot = CHEROKEE_BUF_INIT;

	/* Sanity checka
	 */
	if (port > 0xFFFF) {
		PRINT_ERROR ("Port %d is out of limits\n", port);
		return ret_error;
	}
	if (document_root == NULL) {
		PRINT_ERROR ("Document root is not specified\n");
		return ret_error;
	}

	/* Build the configuration string
	 */
	cherokee_buffer_add (&droot, document_root, strlen(document_root));
	cherokee_path_arg_eval (&droot);

	cherokee_buffer_add_va (&tmp,
	                        "server!bind!1!port = %d\n"
#ifdef HAVE_OPENSSL
	                        "server!tls = libssl\n"
#endif
	                        "vserver!1!document_root = %s\n"
	                        BASIC_CONFIG, port, droot.buf);

	/* Apply it
	 */
	ret = cherokee_server_read_config_string (srv, &tmp);

	cherokee_buffer_mrproper (&tmp);
	cherokee_buffer_mrproper (&droot);

	if (ret != ret_ok) {
		PRINT_MSG ("Couldn't initialize HTTPS test server\n");
		return ret_error;
	}
	srv->tls_enabled = true;

	return ret_ok;
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
		                        "server!bind!1!port = %d\n"
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
		/* Check parameter inconsistencies */
		if (port_set) {
			PRINT_MSG ("The -p parameter can only be used in conjunction with -r.\n");
			return ret_error;
		}

		/* Read the configuration file
		 */
		ret = cherokee_server_read_config_file (srv, config_file);
		if (ret != ret_ok) {
			PRINT_MSG ("Couldn't read the config file: %s\n", config_file);
			return ret_error;
		}
	}

#ifdef HAVE_OPENSSL
	if (add_hardcoded_ssl_options (srv) != ret_ok) {
		PRINT_MSG ("Couldn't identify deactivated SSL/TLS protocols\n");
	}
#endif
	if (daemon_mode)
		cherokee_server_daemonize (srv);

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
	        "  -h,       --help                  Print this help\n"
	        "  -V,       --version               Print version and exit\n"
	        "  -t,       --test                  Just test configuration file\n"
	        "  -d,       --detach                Detach from the console\n"
	        "  -C<PATH>, --config=<PATH>         Configuration file\n"
	        "  -p<NUM>,  --port=<NUM>            TCP port number\n"
	        "  -r<PATH>, --documentroot=<PATH>   Server directory content\n"
	        "  -i,       --print-server-info     Print server technical information\n"
	        "  -v,       --valgrind              Execute the worker process under valgrind\n"
	        "  -s,       --services-fd           FD for host process services\n\n"
		"Report bugs to " PACKAGE_BUGREPORT "\n");
}

static ret_t
process_parameters (int argc, char **argv)
{
	int c;

	/* NOTE 1: If any of these parameters change, main.c may need
	 * to be updated.
	 *
	 * NOTE 2: The -v / --valgrind parameter is handled by main.c.
	 */
	struct option long_options[] = {
		{"help",              no_argument,       NULL, 'h'},
		{"version",           no_argument,       NULL, 'V'},
		{"detach",            no_argument,       NULL, 'd'},
		{"test",              no_argument,       NULL, 't'},
		{"print-server-info", no_argument,       NULL, 'i'},
		{"admin_child",       no_argument,       NULL, 'a'},
		{"port",              required_argument, NULL, 'p'},
		{"documentroot",      required_argument, NULL, 'r'},
		{"config",            required_argument, NULL, 'C'},
		{"services-fd",       required_argument, NULL, 's'},
		{NULL, 0, NULL, 0}
	};

	while ((c = getopt_long(argc, argv, "hVdtiap:r:C:s:", long_options, NULL)) != -1) {
		switch(c) {
		case 'C':
			free (config_file);
			config_file = strdup(optarg);
			break;
		case 'd':
			daemon_mode = true;
			break;
		case 'r':
			free (document_root);
			document_root = strdup(optarg);
			break;
		case 'p':
			port = atoi(optarg);
			port_set = true;
			break;
		case 't':
			just_test = true;
			break;
		case 'i':
			print_modules = true;
			break;
		case 'a':
			cherokee_admin_child = true;
			break;
		case 'V':
			printf (APP_NAME " " PACKAGE_VERSION "\n" APP_COPY_NOTICE);
			return ret_eof;
		case 's':
			services_fd = atoi(optarg);
			break;
		case 'h':
		case '?':
		default:
			print_help();
			return ret_eof;
		}
	}

	/* Check for trailing parameters
	 */
	for (c = optind; c < argc; c++) {
		if ((argv[c] != NULL) && (strlen(argv[c]) > 0)) {
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

	config_file = strdup (DEFAULT_CONFIG_FILE);
	if (config_file == NULL) {
		PRINT_MSG ("ERROR: Couldn't allocate memory.\n");
		exit (EXIT_ERROR);
	}

	cherokee_init();

	ret = cherokee_server_new (&srv);
	if (ret < ret_ok) {
		exit (EXIT_ERROR_FATAL);
	}

	ret = process_parameters (argc, argv);
	if (ret != ret_ok) {
		exit (EXIT_ERROR_FATAL);
	}

	if (print_modules) {
		if (document_root == NULL) {
			document_root = WWW_ROOT;
		}
		if (!port_set) {
			port = 443;
		}
		ret = fake_tls_server_initialization (srv);
		if (ret != ret_ok) {
			exit (EXIT_ERROR);
		}
#ifdef HAVE_OPENSSL
		if (add_hardcoded_ssl_options (srv) != ret_ok) {
			PRINT_MSG ("Couldn't identify deactivated SSL/TLS protocols\n");
		}
#endif
		cherokee_info_build_print (srv);
		exit (EXIT_OK_ONCE);
	}

	if (just_test) {
		ret = test_configuration_file();
		if (ret != ret_ok) {
			exit (EXIT_ERROR);
		}
		exit (EXIT_OK_ONCE);
	}

	ret = common_server_initialization (srv);
	if (ret < ret_ok) {
		exit (EXIT_ERROR_FATAL);
	}

	if (services_fd != -1) {
		ret = cherokee_services_client_init(services_fd);
		if (unlikely(ret < ret_ok)) {
			exit (EXIT_ERROR_FATAL);
		}
	}


	do {
		ret = cherokee_server_step (srv);
	} while (ret == ret_eagain);

	cherokee_server_stop (srv);
	cherokee_server_free (srv);

	free (config_file);
	cherokee_mrproper();

	return EXIT_OK;
}
