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

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#ifdef HAVE_GETOPT_LONG
# include <getopt.h>
#else 
# include "getopt/getopt.h"
#endif

#include <cherokee/cherokee.h>

/* Notices 
 */
#define APP_NAME        \
	"Cherokee Web Server: Tweaker"

#define APP_COPY_NOTICE \
	"Written by Alvaro Lopez Ortega <alvaro@gnu.org>\n\n"                          \
	"Copyright (C) 2001-2008 Alvaro Lopez Ortega.\n"                               \
	"This is free software; see the source for copying conditions.  There is NO\n" \
	"warranty; not even for MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.\n"


#define EXIT_OK     0
#define EXIT_ERROR  1
#define WATCH_SLEEP 1000
#define ENTRIES     "tweak"

#define CHECK_ERROR(msg)                                          \
	 if (ret != ret_ok) {                                     \
	    const char      *error;                               \
	    cherokee_http_t  code;                                \
	    cherokee_admin_client_get_reply_code (client, &code); \
            if (code != http_ok) {                                \
                cherokee_http_code_to_string (code, &error);	  \
                printf ("ERROR %s\n", error);                     \
            } else {                                              \
		PRINT_ERROR ("ERROR: " msg " ret=%d\n", ret);	  \
            }                                                     \
   	    return ret;                                           \
         }


static void
print_help (void)
{
	printf (APP_NAME "\n"
		"Usage: cherokee-tweak -c command -u url [options]\n\n"
		"  -h,  --help                   Print this help\n"
		"  -V,  --version                Print version and exit\n\n"
		" Required:\n"
		"  -c,  --command=STRING         Command: logrotate, trace, info\n"
		"  -a,  --url=URL                URL to the admin interface\n\n"
		" Secutiry:\n"
		"  -u,  --user=STRING            User name\n"
		"  -p,  --password=STRING        Password\n\n"
		" Logrotate:\n"
		"  -l,  --log=PATH               Log file to be rotated\n\n"
		" Trace:\n"
		"  -t,  --trace=STRING           Modules to be traced\n\n"
		"Report bugs to " PACKAGE_BUGREPORT "\n");
}

static void
print_usage (void)
{
	printf (APP_NAME "\n"
		"Usage: cherokee-tweak -c command -u url [options]\n\n"
		"Try `cherokee-tweak --help' for more options.\n");
}

static ret_t
client_new (cherokee_admin_client_t **client_ret, 
	    cherokee_fdpoll_t       **fdpoll_ret, 
	    cherokee_buffer_t        *url,
	    cherokee_buffer_t        *user,
	    cherokee_buffer_t        *password)
{
	ret_t                    ret;
	cuint_t                  fds_num;
	cherokee_fdpoll_t       *fdpoll;
	cherokee_admin_client_t *client;

	cherokee_sys_fdlimit_get (&fds_num);

	ret = cherokee_fdpoll_best_new (&fdpoll, fds_num, fds_num);
	if (ret != ret_ok) 
		return ret;
	   
	ret = cherokee_admin_client_new (&client);
	if (ret != ret_ok) 
		return ret;

	ret = cherokee_tls_init();
	if (ret != ret_ok) 
		return ret;

	ret = cherokee_admin_client_prepare (client, fdpoll, url, user, password);
 	if (ret != ret_ok) {
		PRINT_ERROR_S ("Client prepare failed\n");
		return ret;
	}

	ret = cherokee_admin_client_connect (client);
 	if (ret != ret_ok) {
		PRINT_ERROR_S ("Couldn't connect\n");
		return ret;
	}

	TRACE(ENTRIES, "fdpoll=%p, client_admin=%p\n", fdpoll, client);

	*fdpoll_ret = fdpoll;
	*client_ret = client;
	return ret_ok;
}


static ret_t
do_trace (cherokee_buffer_t *url, 
	  cherokee_buffer_t *user, 
	  cherokee_buffer_t *pass, 
	  cherokee_buffer_t *trace)
{
	ret_t                    ret;
	cherokee_admin_client_t *client;
	cherokee_fdpoll_t       *fdpoll;

	ret = client_new (&client, &fdpoll, url, user, pass);
	if (ret != ret_ok) return ret;

	RUN_CLIENT1 (client, cherokee_admin_client_set_trace, trace);
	CHECK_ERROR ("trace");

	cherokee_admin_client_free (client);
	return ret_ok;
}


static ret_t
look_for_logname (cherokee_buffer_t *logfile, cherokee_buffer_t *logname)
{
	DIR               *dir; 
	char              *tmp;
	struct dirent     *file;
	cuint_t            max     = 0;	
	cherokee_buffer_t  dirname = CHEROKEE_BUF_INIT;

	/* Build the directory name
	 */
	cherokee_buffer_add_buffer (&dirname, logfile);

	tmp = strrchr (dirname.buf, '/');
	if (tmp == NULL) {
		PRINT_ERROR ("Bad filename '%s'\n", logfile->buf);
		goto error;
	}

	cherokee_buffer_drop_ending (&dirname, (dirname.buf + dirname.len) - (tmp + 1));

	/* Read files
	 */
	dir = opendir (dirname.buf);
	if (dir == NULL) {
		PRINT_ERROR ("Invalid directory '%s'\n", dirname.buf);
		goto error;
	}

	while ((file = readdir (dir)) != NULL) {
		cuint_t  val;
		char    *numstr;
		cuint_t d_name_len;

		d_name_len = strlen(file->d_name);
		cherokee_buffer_add (&dirname, file->d_name, d_name_len);

		if (cherokee_buffer_cmp_buf (&dirname, logname)) {
			cherokee_buffer_drop_ending (&dirname, d_name_len);
			continue;
		}
		
		if (dirname.len >= logfile->len + 2) {
			numstr = dirname.buf + logfile->len + 1;
			val = strtol (numstr, NULL, 10);
			
			if (val > max) 
				max = val;
		}

		cherokee_buffer_drop_ending (&dirname, d_name_len);
	}

	/* Build the new filename
	 */
	cherokee_buffer_add_va (logname, "%s.%d", logfile->buf, max + 1);

	cherokee_buffer_mrproper (&dirname);
	return ret_ok;

error:
	cherokee_buffer_mrproper (&dirname);
	return ret_error;	
}


static ret_t
do_logrotate (cherokee_buffer_t *url, 
	      cherokee_buffer_t *user, 
	      cherokee_buffer_t *pass, 
	      cherokee_buffer_t *log)
{
	int                      re;
	ret_t                    ret;
	cherokee_admin_client_t *client;
	cherokee_fdpoll_t       *fdpoll;
	cherokee_buffer_t        newname = CHEROKEE_BUF_INIT;

	ret = client_new (&client, &fdpoll, url, user, pass);
	if (ret != ret_ok) return ret;

	/* Look for the log name
	 */
	ret = look_for_logname (log, &newname);
	if (ret != ret_ok) return ret;

	/* Set the server in to backup mode
	 */
	printf ("Setting backup mode.. ");
	RUN_CLIENT1 (client, cherokee_admin_client_set_backup_mode, true);
	CHECK_ERROR ("backup_mode");
	printf ("OK\n");

	/* Move logs
	 */
	re = rename (log->buf, newname.buf);
	if (re != 0) {
		PRINT_ERRNO (errno, "Could not move '%s' to '%s': '${errno}'", log->buf, newname.buf);
	}
	printf ("Log file '%s' moved to '%s' successfully\n", log->buf, newname.buf);
	
	/* Turn the backup mode off
	 */
	printf ("Restoring production mode.. ");
	RUN_CLIENT1 (client, cherokee_admin_client_set_backup_mode, false);
	CHECK_ERROR ("backup_mode");
	printf ("OK\n");
	
	cherokee_admin_client_free (client);
	cherokee_fdpoll_free (fdpoll);
	return ret_ok;
}

static void
print_entry (const char *str, char *format, ...)
{
	cuint_t i;
	va_list ap;

	printf ("%s", str);
	for (i=0; i<20-strlen(str); i++) {
		printf (" ");
	}

	va_start (ap, format);
	vfprintf (stdout, format, ap);
	va_end (ap);

	printf ("\n");
}

static ret_t
do_print_info (cherokee_buffer_t *url,
	       cherokee_buffer_t *user, 
	       cherokee_buffer_t *pass)
{
	ret_t                    ret;
	cherokee_admin_client_t *client;
	cherokee_fdpoll_t       *fdpoll;
	cherokee_list_t         *i, *tmp;

	cuint_t                  port;
	cherokee_buffer_t        buf   = CHEROKEE_BUF_INIT;
	cherokee_list_t          conns = LIST_HEAD_INIT(conns);

	ret = client_new (&client, &fdpoll, url, user, pass);
	if (ret != ret_ok) return ret;

	RUN_CLIENT1 (client, cherokee_admin_client_ask_port, &port);
	CHECK_ERROR ("port");
	print_entry ("HTTP port", "%d", port);

	RUN_CLIENT1 (client, cherokee_admin_client_ask_port_tls, &port);
	CHECK_ERROR ("port_tls");
	print_entry ("HTTPS port", "%d", port);

	RUN_CLIENT1 (client, cherokee_admin_client_ask_thread_num, &buf);
	CHECK_ERROR ("thread_num");
	print_entry ("Threads", "%s", buf.buf);
	cherokee_buffer_clean (&buf);
	
	RUN_CLIENT1 (client, cherokee_admin_client_ask_rx, &buf);
	CHECK_ERROR ("rx");
	print_entry ("Received", "%s", buf.buf);
	cherokee_buffer_clean (&buf);

	RUN_CLIENT1 (client, cherokee_admin_client_ask_tx, &buf);
	CHECK_ERROR ("tx");
	print_entry ("Transfered", "%s", buf.buf);

	RUN_CLIENT1 (client, cherokee_admin_client_ask_connections, &conns);
	CHECK_ERROR ("conns");

	list_for_each (i, &conns) {
		cherokee_connection_info_t *conn = CONN_INFO(i);
		
		printf ("Request: '%s', phase: '%s', rx: '%s', tx: '%s', size: '%s'\n", 
			conn->request.buf, conn->phase.buf, conn->rx.buf, 
			conn->tx.buf, conn->total_size.buf);
	}
	
	list_for_each_safe (i, tmp, &conns) {
		cherokee_connection_info_free (CONN_INFO(i));
	}

	/* Clean up
	 */
	cherokee_buffer_mrproper (&buf);
	cherokee_admin_client_free (client);
	cherokee_fdpoll_free (fdpoll);

	return ret_ok;
}


int 
main (int argc, char *argv[]) 
{
	int                c;
	cherokee_buffer_t  command  = CHEROKEE_BUF_INIT;
	cherokee_buffer_t  url      = CHEROKEE_BUF_INIT;
	cherokee_buffer_t  trace    = CHEROKEE_BUF_INIT;
	cherokee_buffer_t  log      = CHEROKEE_BUF_INIT;
	cherokee_buffer_t  user     = CHEROKEE_BUF_INIT;
	cherokee_buffer_t  password = CHEROKEE_BUF_INIT;

	struct option long_options[] = {
		{"help",         no_argument,       NULL, 'h'},
		{"version",      no_argument,       NULL, 'V'},
		{"command",      required_argument, NULL, 'c'},
		{"url",          required_argument, NULL, 'a'},
		{"user",         required_argument, NULL, 'u'},
		{"password",     required_argument, NULL, 'p'},
		{"log",          required_argument, NULL, 'l'},
		{"trace",        required_argument, NULL, 't'},
		{NULL, 0, NULL, 0}
	};

	/* Initialize the library
	 */
	cherokee_init();
	TRACE(ENTRIES, "Starts %d args\n", argc-1);

	/* Parse the parameters
	 */
	while ((c = getopt_long(argc, argv, "hVc:a:u:p:l:t:", long_options, NULL)) != -1) {
		switch(c) {
		case 'u':
			cherokee_buffer_add (&user, optarg, strlen(optarg));
			break;
		case 'p':
			cherokee_buffer_add (&password, optarg, strlen(optarg));
			break;
		case 'c':
			cherokee_buffer_add (&command, optarg, strlen(optarg));
			break;
		case 't':
			cherokee_buffer_add (&trace, optarg, strlen(optarg));
			break;
		case 'a':
			cherokee_buffer_add (&url, optarg, strlen(optarg));
			break;
		case 'l':
			cherokee_buffer_add (&log, optarg, strlen(optarg));
			break;
		case 'V':
			printf (APP_NAME " " PACKAGE_VERSION "\n" APP_COPY_NOTICE);
			exit (EXIT_OK);
		case 'h':
		case '?':
		default:
			print_help();
			exit (EXIT_OK);
		}
	}

	/* Ensure that there's a command
	 */
	if ((command.len <= 0) || (url.len <= 0)) {
		PRINT_MSG_S ("ERROR: A command and the administration URL are needed\n\n");
		print_usage();
		exit (EXIT_ERROR);
	}

	/* Check the command and perform
	 */
	if (cherokee_buffer_cmp_str (&command, "trace") == 0) {
		if (trace.len <= 0) {
			PRINT_MSG_S ("ERROR: Trace needs a -t option\n\n");
			print_usage();
			exit (EXIT_ERROR);
		}
		do_trace (&url, &user, &password, &trace);

	} else if (cherokee_buffer_cmp_str (&command, "logrotate") == 0) {
		if (log.len <= 0) {
			PRINT_MSG_S ("ERROR: Logrotate needs -u and -l options\n\n");
			print_usage();
			exit (EXIT_ERROR);
		}
		do_logrotate (&url, &user, &password, &log);

	} else if (cherokee_buffer_cmp_str (&command, "info") == 0) {
		do_print_info (&url, &user, &password);

	} else {
		PRINT_MSG_S ("ERROR: Command not recognized\n\n");
		print_help();
		exit (EXIT_ERROR);		
	}

	return EXIT_OK;
}
