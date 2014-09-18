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
#include <sys/stat.h>

#include "init.h"
#include "server.h"
#include "socket.h"
#include "config_reader.h"
#include "server-protected.h"
#include "util.h"

#ifdef HAVE_SYS_WAIT_H
# include <sys/wait.h>
#endif

#ifdef HAVE_GETOPT_LONG
# include <getopt.h>
#else
# include "getopt/getopt.h"
#endif

#define APP_NAME        \
	"Cherokee Web Server: Admin"

#define APP_COPY_NOTICE \
	"Written by Alvaro Lopez Ortega <alvaro@alobbs.com>\n\n"                       \
	"Copyright (C) 2001-2014 Alvaro Lopez Ortega.\n"                               \
	"This is free software; see the source for copying conditions.  There is NO\n" \
	"warranty; not even for MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.\n"

#define ALPHA_NUM            "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789"
#define PASSWORD_LEN         16
#define DEFAULT_PORT         9090
#define TIMEOUT              "25"
#define THREAD_MAX_AUTO      4
#define DEFAULT_DOCUMENTROOT CHEROKEE_DATADIR "/admin"
#define DEFAULT_CONFIG_FILE  CHEROKEE_CONFDIR "/cherokee.conf"
#define DEFAULT_UNIX_SOCKET  TMPDIR "/cherokee-admin-scgi.socket"
#define DEFAULT_BIND         "127.0.0.1"
#define RULE_PRE             "vserver!1!rule!"

static int                port          = DEFAULT_PORT;
static char              *document_root = NULL;
static char              *config_file   = NULL;
static char              *bind_to       = NULL;
static int                debug         = 0;
static int                unsecure      = 0;
static int                iocache       = 1;
static int                scgi_port     = 4000;
static int                thread_num    = -1;
static cherokee_server_t *srv           = NULL;
static cherokee_buffer_t  password      = CHEROKEE_BUF_INIT;


static ret_t
find_empty_port (int starting, int *port)
{
	ret_t             ret;
	cherokee_socket_t s;
	int               p     = starting;
	cherokee_buffer_t bind_ = CHEROKEE_BUF_INIT;

	cherokee_buffer_add_str (&bind_, "127.0.0.1");
	cherokee_socket_init (&s);

	while (true) {
		cherokee_socket_create_fd (&s, AF_INET);
		ret = cherokee_socket_bind (&s, p, &bind_);
		cherokee_socket_close (&s);

		if (ret == ret_ok)
			break;

		p += 1;
		if (unlikely (p > 0xFFFF)) {
			goto error;
		}
	}

	cherokee_socket_mrproper (&s);
	cherokee_buffer_mrproper (&bind_);

	*port = p;
	return ret_ok;

error:
	cherokee_socket_close (&s);
	cherokee_socket_mrproper (&s);
	cherokee_buffer_mrproper (&bind_);
	return ret_error;
}

static ret_t
generate_password (cherokee_buffer_t *buf)
{
	cuint_t i;
	cuint_t n;

	for (i=0; i<PASSWORD_LEN; i++) {
		n = cherokee_random()%(sizeof(ALPHA_NUM)-1);
		cherokee_buffer_add_char (buf, ALPHA_NUM[n]);
	}

	return ret_ok;
}

static ret_t
remove_old_socket (const char *path)
{
	int         re;
	struct stat info;

	/* It might not exist
	 */
	re = cherokee_stat (path, &info);
	if (re != 0) {
		return ret_ok;
	}

	/* If exist, it must be a socket
	 */
	if (! S_ISSOCK(info.st_mode)) {
		PRINT_MSG ("ERROR: Something happens; '%s' isn't a socket.\n", path);
		return ret_error;
	}

	/* Remove it
	 */
	re = cherokee_unlink (path);
	if (re != 0) {
		PRINT_MSG ("ERROR: Couldn't remove unix socket '%s'.\n", path);
		return ret_error;
	}

	return ret_ok;
}


static void
print_connection_info (void)
{
	printf ("\n");

	if (! cherokee_buffer_is_empty (&password)) {
		printf ("Login:\n"
			"  User:              admin\n"
			"  One-time Password: %s\n\n", password.buf);
	}

	printf ("Web Interface:\n"
		"  URL:               http://%s:%d/\n\n",
		(bind_to) ? bind_to : "localhost", port);

	fflush(stdout);
}


static ret_t
config_server (cherokee_server_t *srv)
{
	ret_t                  ret;
	cherokee_config_node_t conf;
	cuint_t                nthreads;
	cherokee_buffer_t      buf       = CHEROKEE_BUF_INIT;
	cherokee_buffer_t      rrd_dir   = CHEROKEE_BUF_INIT;
	cherokee_buffer_t      rrd_bin   = CHEROKEE_BUF_INIT;
	cherokee_buffer_t      fake;

	/* Generate the password
	 */
	if (unsecure == 0) {
		ret = generate_password (&password);
		if (ret != ret_ok)
			return ret;
	}

	/* Configure the embedded server
	 */
	if (scgi_port > 0) {
		ret = find_empty_port (scgi_port, &scgi_port);
	} else {
		ret = remove_old_socket (DEFAULT_UNIX_SOCKET);
	}
	if (ret != ret_ok) {
		return ret_error;
	}

	cherokee_buffer_add_va  (&buf, "server!bind!1!port = %d\n", port);
	cherokee_buffer_add_str (&buf, "server!ipv6 = 1\n");
	cherokee_buffer_add_str (&buf, "server!max_connection_reuse = 0\n");
	cherokee_buffer_add_va  (&buf, "server!iocache = %d\n", iocache);

	if (bind_to) {
		cherokee_buffer_add_va (&buf, "server!bind!1!interface = %s\n", bind_to);
	}

	if (thread_num != -1) {
		/* Manually set
		 */
		cherokee_buffer_add_va (&buf, "server!thread_number = %d\n", thread_num);
	} else {
		/* Automatically set
		 */
		nthreads = MIN (cherokee_cpu_number, THREAD_MAX_AUTO);
		cherokee_buffer_add_va (&buf, "server!thread_number = %d\n", nthreads);
	}

	cherokee_buffer_add_str (&buf, "vserver!1!nick = default\n");
	cherokee_buffer_add_va  (&buf, "vserver!1!document_root = %s\n", document_root);

	if (scgi_port <= 0) {
		cherokee_buffer_add_va  (&buf,
		                         "source!1!nick = app-logic\n"
		                         "source!1!type = interpreter\n"
		                         "source!1!timeout = " TIMEOUT "\n"
		                         "source!1!host = %s\n"
		                         "source!1!interpreter = %s/server.py %s %s %s\n"
		                         "source!1!env_inherited = 1\n",
		                         DEFAULT_UNIX_SOCKET, document_root,
		                         DEFAULT_UNIX_SOCKET, config_file,
		                         (debug) ? "-x" : "");

	} else {
		cherokee_buffer_add_va  (&buf,
		                         "source!1!nick = app-logic\n"
		                         "source!1!type = interpreter\n"
		                         "source!1!timeout = " TIMEOUT "\n"
		                         "source!1!host = localhost:%d\n"
		                         "source!1!interpreter = %s/server.py %d %s %s\n"
		                         "source!1!env_inherited = 1\n",
		                         scgi_port, document_root,
		                         scgi_port, config_file,
		                         (debug) ? "-x" : "");
	}

	if (debug) {
		cherokee_buffer_add_str  (&buf, "source!1!debug = 1\n");
	}

	cherokee_buffer_add_str  (&buf,
	                          RULE_PRE "1!match = default\n"
	                          RULE_PRE "1!handler = scgi\n"
	                          RULE_PRE "1!timeout = " TIMEOUT "\n"
	                          RULE_PRE "1!handler!balancer = round_robin\n"
	                          RULE_PRE "1!handler!balancer!source!1 = 1\n");

	cherokee_buffer_add_str  (&buf, RULE_PRE "1!handler!env!CTK_COOKIE = ");
	generate_password  (&buf);
	cherokee_buffer_add_char (&buf, '\n');

	cherokee_buffer_add_str  (&buf, RULE_PRE "1!handler!env!CTK_SUBMITTER_SECRET = ");
	generate_password  (&buf);
	cherokee_buffer_add_char (&buf, '\n');

	if (! debug) {
		cherokee_buffer_add_str (&buf, RULE_PRE "1!encoder!gzip = 1\n");
	}

	cherokee_buffer_add_str (&buf,
	                         RULE_PRE "2!match = directory\n"
	                         RULE_PRE "2!match!directory = /about\n"
	                         RULE_PRE "2!handler = server_info\n");

	cherokee_buffer_add_str (&buf,
	                         RULE_PRE "3!match = directory\n"
	                         RULE_PRE "3!match!directory = /static\n"
	                         RULE_PRE "3!handler = file\n"
	                         RULE_PRE "3!expiration = time\n"
	                         RULE_PRE "3!expiration!time = 30d\n");

	cherokee_buffer_add_va  (&buf,
	                         RULE_PRE "4!match = request\n"
	                         RULE_PRE "4!match!request = ^/favicon.ico$\n"
	                         RULE_PRE "4!document_root = %s/static/images\n"
	                         RULE_PRE "4!handler = file\n"
	                         RULE_PRE "4!expiration = time\n"
	                         RULE_PRE "4!expiration!time = 30d\n",
	                         document_root);

	cherokee_buffer_add_va  (&buf,
	                         RULE_PRE "5!match = directory\n"
	                         RULE_PRE "5!match!directory = /icons_local\n"
	                         RULE_PRE "5!handler = file\n"
	                         RULE_PRE "5!document_root = %s\n"
	                         RULE_PRE "5!expiration = time\n"
	                         RULE_PRE "5!expiration!time = 30d\n",
	                         CHEROKEE_ICONSDIR);

	cherokee_buffer_add_va  (&buf,
	                         RULE_PRE "6!match = directory\n"
	                         RULE_PRE "6!match!directory = /CTK\n"
	                         RULE_PRE "6!handler = file\n"
	                         RULE_PRE "6!document_root = %s/CTK/static\n"
	                         RULE_PRE "6!expiration = time\n"
	                         RULE_PRE "6!expiration!time = 30d\n",
	                         document_root);

	/* Embedded help
	 */
	cherokee_buffer_add_va  (&buf,
	                         RULE_PRE "7!match = and\n"
	                         RULE_PRE "7!match!left = directory\n"
	                         RULE_PRE "7!match!left!directory = /help\n"
	                         RULE_PRE "7!match!right = not\n"
	                         RULE_PRE "7!match!right!right = extensions\n"
	                         RULE_PRE "7!match!right!right!extensions = html\n"
	                         RULE_PRE "7!handler = file\n");

	cherokee_buffer_add_va  (&buf,
	                         RULE_PRE "8!match = fullpath\n"
	                         RULE_PRE "8!match!fullpath!1 = /static/help_404.html\n"
	                         RULE_PRE "8!handler = file\n"
	                         RULE_PRE "8!document_root = %s\n", document_root);

	cherokee_buffer_add_va  (&buf,
	                         RULE_PRE "9!match = and\n"
	                         RULE_PRE "9!match!left = directory\n"
	                         RULE_PRE "9!match!left!directory = /help\n"
	                         RULE_PRE "9!match!right = not\n"
	                         RULE_PRE "9!match!right!right = exists\n"
	                         RULE_PRE "9!match!right!right!match_any = 1\n"
	                         RULE_PRE "9!handler = redir\n"
	                         RULE_PRE "9!handler!rewrite!1!show = 1\n"
	                         RULE_PRE "9!handler!rewrite!1!substring = /static/help_404.html\n");

	cherokee_buffer_add_va  (&buf,
	                         RULE_PRE "10!match = directory\n"
	                         RULE_PRE "10!match!directory = /help\n"
	                         RULE_PRE "10!match!final = 0\n"
	                         RULE_PRE "10!document_root = %s\n", CHEROKEE_DOCDIR);

	/* GZip
	 */
	if (! debug) {
		cherokee_buffer_add_va (&buf,
		                        RULE_PRE "15!match = extensions\n"
		                        RULE_PRE "15!match!extensions = css,js,html\n"
		                        RULE_PRE "15!match!final = 0\n"
		                        RULE_PRE "15!encoder!gzip = 1\n");
	}

	/* RRDtool graphs
	 */
	cherokee_config_node_init (&conf);
	cherokee_buffer_fake (&fake, config_file, strlen(config_file));

	ret = cherokee_config_reader_parse (&conf, &fake);
	if (ret == ret_ok) {
		cherokee_config_node_copy (&conf, "server!collector!rrdtool_path", &rrd_bin);
		cherokee_config_node_copy (&conf, "server!collector!database_dir", &rrd_dir);
	}

	if (! cherokee_buffer_is_empty (&rrd_bin)) {
		cherokee_buffer_add_va  (&buf,
		                         RULE_PRE "20!handler!rrdtool_path = %s\n", rrd_bin.buf);
	}

	if (! cherokee_buffer_is_empty (&rrd_dir)) {
		cherokee_buffer_add_va  (&buf,
		                         RULE_PRE "20!handler!database_dir = %s\n", rrd_dir.buf);
	}

	cherokee_buffer_add_str (&buf,
	                         RULE_PRE "20!match = directory\n"
	                         RULE_PRE "20!match!directory = /graphs\n"
	                         RULE_PRE "20!handler = render_rrd\n"
	                         RULE_PRE "20!expiration = epoch\n"
	                         RULE_PRE "20!expiration!caching = no-cache\n"
	                         RULE_PRE "20!expiration!caching!no-store = 1\n");

	cherokee_buffer_add_str    (&buf, RULE_PRE "20!document_root = ");
	cherokee_buffer_add_buffer (&buf, &cherokee_tmp_dir);
	cherokee_buffer_add_va     (&buf, "/rrd-cache\n");

	if ((unsecure == 0) &&
	    (!cherokee_buffer_is_empty (&password)))
	{
		cherokee_buffer_add_va (&buf,
		                        RULE_PRE "100!auth = authlist\n"
		                        RULE_PRE "100!auth!methods = digest\n"
		                        RULE_PRE "100!auth!realm = Cherokee-admin\n"
		                        RULE_PRE "100!auth!list!1!user = admin\n"
		                        RULE_PRE "100!auth!list!1!password = %s\n"
		                        RULE_PRE "100!match = request\n"
		                        RULE_PRE "100!match!final = 0\n"
		                        RULE_PRE "100!match!request = .*\n",
		                        password.buf);
	}

	/* MIME types
	 */
	cherokee_buffer_add_str (&buf,
	                         "mime!text/javascript!extensions = js\n"
	                         "mime!text/css!extensions = css\n"
	                         "mime!image/png!extensions = png\n"
	                         "mime!image/jpeg!extensions = jpeg,jpg\n"
	                         "mime!image/svg+xml!extensions = svg,svgz\n"
	                         "mime!image/gif!extensions = gif\n");

	ret = cherokee_server_read_config_string (srv, &buf);
	if (ret != ret_ok) {
		PRINT_ERROR_S ("Could not initialize the server\n");
		return ret;
	}

	cherokee_config_node_mrproper (&conf);

	cherokee_buffer_mrproper (&rrd_bin);
	cherokee_buffer_mrproper (&rrd_dir);
	cherokee_buffer_mrproper (&buf);

	return ret_ok;
}


static void
print_help (void)
{
	printf (APP_NAME "\n"
	        "Usage: cherokee-admin [options]\n\n"
	        "  -h,        --help                 Print this help\n"
	        "  -V,        --version              Print version and exit\n"
	        "  -x,        --debug                Enables debug\n"
	        "  -u,        --unsecure             Turn off the authentication\n"
	        "  -b[<IP>],  --bind[=<IP>]          Bind net iface; no arg means all\n"
	        "  -d<DIR>,   --appdir=<DIR>         Application directory\n"
	        "  -p<NUM>,   --port=<NUM>           TCP port\n"
	        "  -t,        --internal-unix        Use a Unix domain socket internally\n"
	        "  -i,        --disable-iocache      Disable I/O cache: reduces mem usage\n"
	        "  -T<NUM>,   --threads=<NUM>        Threads number\n"
	        "  -C<PATH>,  --target=<PATH>        Configuration file to modify\n\n"
	        "Report bugs to " PACKAGE_BUGREPORT "\n");
}

static void
process_parameters (int argc, char **argv)
{
	ret_t              ret;
	int                c;
	cherokee_boolean_t error = false;

	struct option long_options[] = {
		{"help",            no_argument,       NULL, 'h'},
		{"version",         no_argument,       NULL, 'V'},
		{"debug",           no_argument,       NULL, 'x'},
		{"unsecure",        no_argument,       NULL, 'u'},
		{"internal-tcp",    no_argument,       NULL, 't'},
		{"disable-iocache", no_argument,       NULL, 'i'},
		{"bind",            optional_argument, NULL, 'b'},
		{"appdir",          required_argument, NULL, 'd'},
		{"port",            required_argument, NULL, 'p'},
		{"target",          required_argument, NULL, 'C'},
		{"threads",         required_argument, NULL, 'T'},
		{NULL, 0, NULL, 0}
	};

	while ((c = getopt_long(argc, argv, "hVxutib::d:p:C:T:", long_options, NULL)) != -1) {
		switch(c) {
		case 'b':
			free (bind_to);
			if (optarg) {
				bind_to = strdup(optarg);
			} else if (argv[optind] && argv[optind][0] != '-') {
				bind_to = strdup(argv[optind]);
				optind++;
			} else {
				bind_to = NULL;
			}
			break;
		case 'p':
			ret = cherokee_atoi (optarg, &port);
			if (ret != ret_ok) {
				error = true;
			}
			break;
		case 'T':
			ret = cherokee_atoi (optarg, &thread_num);
			if (ret != ret_ok) {
				error = true;
			}
			break;
		case 'd':
			free (document_root);
			document_root = strdup(optarg);
			break;
		case 'C':
			free (config_file);
			config_file = strdup(optarg);
			break;
		case 'x':
			debug   = 1;
			iocache = 0;
			break;
		case 'u':
			unsecure = 1;
			break;
		case 'i':
			iocache = 0;
			break;
		case 't':
			scgi_port = 0;
			break;
		case 'V':
			printf (APP_NAME " " PACKAGE_VERSION "\n" APP_COPY_NOTICE);
			exit(0);
		case 'h':
		case '?':
		default:
			error = true;
		}

		if (error) {
			print_help();
			exit (1);
		}
	}

	/* Check for trailing parameters
	 */
	for (c = optind; c < argc; c++) {
		if ((argv[c] != NULL) && (strlen(argv[c]) > 0)) {
			print_help();
			exit (1);
		}
	}
}

static ret_t
check_for_python (void)
{
	int         re;
	pid_t       pid;
	int         exitcode = -1;
	char const *args[]   = {"env", "python2", "-c", "raise SystemExit", NULL};

	pid = fork();
	if (pid == -1) {
		return ret_error;

	} else if (pid == 0) {
		execv ("/usr/bin/env", (char * const *) args);

	} else {
		do {
			re = waitpid (pid, &exitcode, 0);
		} while (re == -1 && errno == EINTR);

		return (WEXITSTATUS(exitcode) == 0)? ret_ok : ret_error;
	}

	SHOULDNT_HAPPEN;
	return ret_error;
}

static void
signals_handler (int sig, siginfo_t *si, void *context)
{
	ret_t ret;
	int   retcode;

	UNUSED (context);

	switch (sig) {
	case SIGCHLD:
		ret = cherokee_wait_pid (si->si_pid, &retcode);
		if (ret == ret_ok) {
			UNUSED (retcode);
		}
		break;

	case SIGINT:
	case SIGTERM:
		if (srv->wanna_exit) {
			break;
		}

		printf ("Cherokee-admin is exiting..\n");
		cherokee_server_handle_TERM (srv);
		break;

	default:
		PRINT_ERROR ("Unknown signal: %d\n", sig);
	}
}

int
main (int argc, char **argv)
{
	ret_t            ret;
	struct sigaction act;

	/* Globals */
	document_root = strdup (DEFAULT_DOCUMENTROOT);
	config_file   = strdup (DEFAULT_CONFIG_FILE);
	bind_to       = strdup (DEFAULT_BIND);

	if ((!bind_to) || (!config_file) || (!document_root)) {
		PRINT_MSG ("ERROR: Couldn't allocate memory.\n");
		exit (EXIT_ERROR);
	}

	/* Python */
	ret = check_for_python();
	if (ret != ret_ok) {
		PRINT_MSG ("ERROR: Couldn't find python.\n");
		exit (EXIT_ERROR);
	}

	/* Signal handling */
	act.sa_handler = SIG_IGN;
	sigaction (SIGPIPE, &act, NULL);

	memset(&act, 0, sizeof(act));
	act.sa_sigaction = signals_handler;
	act.sa_flags     = SA_SIGINFO;
	sigaction (SIGCHLD, &act, NULL);
	sigaction (SIGINT,  &act, NULL);
	sigaction (SIGTERM, &act, NULL);

	/* Initialize the embedded server */
	cherokee_init();

	/* Seed random numbers
	 */
	cherokee_random_seed();

	process_parameters (argc, argv);

	ret = cherokee_server_new (&srv);
	if (ret != ret_ok)
		exit (EXIT_ERROR);

	ret = config_server (srv);
	if (ret != ret_ok)
		exit (EXIT_ERROR);

	ret = cherokee_server_initialize (srv);
	if (ret != ret_ok)
		exit (EXIT_ERROR);

	print_connection_info();

	ret = cherokee_server_unlock_threads (srv);
	if (ret != ret_ok)
		exit (EXIT_ERROR);

	do {
		ret = cherokee_server_step (srv);
	} while (ret == ret_eagain);

	cherokee_server_stop (srv);
	cherokee_server_free (srv);

	cherokee_mrproper();
	return EXIT_OK;
}
