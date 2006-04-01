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

#include <cherokee/cherokee.h>

#include <string.h>
#include <sys/types.h>
#include <dirent.h>
#include <errno.h>

#define ERROR       1
#define WATCH_SLEEP 1000


static ret_t
look_for_logname (const char *logfile, cherokee_buffer_t *logname)
{
	DIR               *dir; 
	char              *tmp;
	cuint_t            max     = 0;	
	struct dirent     *file;
	cuint_t            logfile_len;
	cherokee_buffer_t  dirname = CHEROKEE_BUF_INIT;

	/* Build the directory name
	 */
	logfile_len = strlen(logfile);
	cherokee_buffer_add (&dirname, (char *)logfile, logfile_len);

	tmp = strrchr(dirname.buf, '/');
	if (tmp == NULL) {
		PRINT_ERROR ("Bad filename '%s'\n", logfile);
		return ret_error;
	}

	cherokee_buffer_drop_endding (&dirname, (dirname.buf + dirname.len) - (tmp + 1));

	/* Read files
	 */
	dir = opendir (dirname.buf);
	if (dir == NULL) {
		PRINT_ERROR ("Invalid directory '%s'\n", dirname.buf);
		return ret_error;
	}

	while ((file = readdir (dir)) != NULL) {
		cuint_t  val;
		char    *numstr;
		cuint_t d_name_len;

		d_name_len = strlen(file->d_name);
		cherokee_buffer_add (&dirname, file->d_name, d_name_len);

		if (strncmp (dirname.buf, logfile, logfile_len)) {
			cherokee_buffer_drop_endding (&dirname, d_name_len);
			continue;
		}
		
		if (dirname.len >= logfile_len + 2) {
			numstr = dirname.buf + logfile_len + 1;
			val = strtol (numstr, NULL, 10);
			
			if (val > max) 
				max = val;
		}

		cherokee_buffer_drop_endding (&dirname, d_name_len);
	}

	/* Build the new filename
	 */
	cherokee_buffer_add_va (logname, "%s.%d", logfile, max + 1);

	cherokee_buffer_mrproper (&dirname);
	return ret_ok;
}


int 
main (int argc, char *argv[]) 
{
	int                      re;
	ret_t                    ret;
	cuint_t                  fds_num;
	cherokee_fdpoll_t       *fdpoll;
	cherokee_admin_client_t *client;
	cherokee_buffer_t        url;
	cherokee_http_t          code;
	cherokee_buffer_t        logname = CHEROKEE_BUF_INIT;

	if (argc <= 2) {
		fprintf (stderr, "Usage:\n    %s URL Logfile\n", argv[0]);
		fprintf (stderr, "Example:\n    %s http://localhost/admin/ /var/log/cherokee.access\n\n", argv[0]);
		return 1;
	}

	/* Build everyhing
	 */
	cherokee_sys_fdlimit_get (&fds_num);

	ret = cherokee_fdpoll_best_new (&fdpoll, fds_num, fds_num);
	if (ret != ret_ok) return ERROR;
	   
	ret = cherokee_admin_client_new (&client);
	if (ret != ret_ok) return ERROR;

	ret = cherokee_tls_init ();
	if (ret != ret_ok) return ERROR;

	/* Set the request
	 */
	cherokee_buffer_init (&url);
	cherokee_buffer_add (&url, argv[1], strlen(argv[1]));

	/* Prepare the admin client object
	 */
	ret = cherokee_admin_client_prepare (client, fdpoll, &url);
 	if (ret != ret_ok) {
		PRINT_ERROR_S ("Client prepare failed\n");
		return ERROR;
	}

	ret = cherokee_admin_client_connect (client);
 	if (ret != ret_ok) {
		PRINT_ERROR_S ("Couldn't connect\n");
		return ERROR;
	}
	
	/* Look for the log name
	 */
	ret = look_for_logname (argv[2], &logname);
	if (ret != ret_ok) return ERROR;

	/* Set the server in to backup mode
	 */
	printf ("Setting backup mode.. ");
	RUN_CLIENT1 (client, fdpoll, cherokee_admin_client_set_backup_mode, true);
	
	if (ret != ret_ok) {
		const char *error;

		cherokee_admin_client_get_reply_code (client, &code);
		cherokee_http_code_to_string (code, &error);

		printf ("%s\n", error);
		return ERROR;
	}
	printf ("OK\n");

	/* Move logs
	 */
	re = rename (argv[2], logname.buf);
	if (re != 0) {
		PRINT_ERROR("Could not move '%s' to '%s': %s\n", argv[2], logname.buf, strerror(errno));
	}
	printf ("Log file '%s' moved to '%s' successfully\n", argv[2], logname.buf);
	
	/* Turn the backup mode off
	 */
	printf ("Restoring production mode.. ");
	RUN_CLIENT1 (client, fdpoll, cherokee_admin_client_set_backup_mode, false);
	if (ret != ret_ok) {
		const char *error;

		cherokee_admin_client_get_reply_code (client, &code);
		cherokee_http_code_to_string (code, &error);

		printf ("%s\n", error);
		return ERROR;
	}
	printf ("OK\n");

	/* Clean up
	 */
	cherokee_admin_client_free (client);
	cherokee_fdpoll_free (fdpoll);

	return 0;
}
