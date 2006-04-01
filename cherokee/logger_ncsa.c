/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/* Cherokee
 *
 * Authors:
 *      Alvaro Lopez Ortega <alvaro@alobbs.com>
 * 
 * Some pieces of code by:
 *      Miguel Angel Ajo Pelayo <ajo@godsmaze.org> 
 *      Pablo Neira Ayuso <pneira@optimat.com>
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
#include "logger_ncsa.h"

#include <stdarg.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>

#ifdef HAVE_SYSLOG_H
# include <syslog.h>
#endif

#ifdef HAVE_SYS_TIME_H
# include <sys/time.h>
#else 
# include <time.h>
#endif

#include "util.h"
#include "connection.h"
#include "connection-protected.h"
#include "header.h"
#include "header-protected.h"
#include "server.h"
#include "server-protected.h"
#include "connection.h"
#include "module.h"


/* Some constants
 */
static char *month[] = {
	"Jan", "Feb", "Mar", "Apr", "May", "Jun",
	"Jul", "Aug", "Sep", "Oct", "Nov", "Dec", 
	NULL
};


cherokee_module_info_t MODULE_INFO(ncsa) = {
	cherokee_logger,            /* type     */
	cherokee_logger_ncsa_new    /* new func */
};

ret_t
cherokee_logger_ncsa_new (cherokee_logger_t **logger, cherokee_table_t *properties)
{
	ret_t ret;
	CHEROKEE_NEW_STRUCT (n, logger_ncsa);
	
	/* Init the base class object
	 */
	cherokee_logger_init_base(LOGGER(n));

	MODULE(n)->init         = (logger_func_init_t) cherokee_logger_ncsa_init;
	MODULE(n)->free         = (logger_func_free_t) cherokee_logger_ncsa_free;
	LOGGER(n)->flush        = (logger_func_flush_t) cherokee_logger_ncsa_flush;
	LOGGER(n)->reopen       = (logger_func_reopen_t) cherokee_logger_ncsa_reopen;
	LOGGER(n)->write_error  = (logger_func_write_error_t)  cherokee_logger_ncsa_write_error;
	LOGGER(n)->write_access = (logger_func_write_access_t) cherokee_logger_ncsa_write_access;
	LOGGER(n)->write_string = (logger_func_write_string_t) cherokee_logger_ncsa_write_string;

	ret = cherokee_logger_ncsa_init_base (n, properties);
	if (unlikely(ret < ret_ok)) return ret;

	/* Return the object
	 */
	*logger = LOGGER(n);
	return ret_ok;
}


ret_t 
cherokee_logger_ncsa_init_base (cherokee_logger_ncsa_t *logger, cherokee_table_t *properties)
{
	/* Init
	 */
	logger->errorlog_fd        = NULL;
	logger->accesslog_fd       = NULL;
	logger->accesslog_filename = NULL;
	logger->errorlog_filename  = NULL;
	logger->combined           = false;
	
	if (properties != NULL) {
		cherokee_typed_table_get_str (properties, "AccessLog", &logger->accesslog_filename);
		cherokee_typed_table_get_str (properties, "ErrorLog", &logger->errorlog_filename);
	}
	
	return ret_ok;
}


static ret_t 
open_output (cherokee_logger_ncsa_t *logger)
{
	if ((logger->accesslog_filename == NULL) ||
	    (logger->errorlog_filename == NULL))
	{
		openlog ("Cherokee", LOG_CONS | LOG_PID | LOG_NDELAY, LOG_LOCAL1);
		return ret_ok;
	}

	logger->accesslog_fd = fopen (logger->accesslog_filename, "a+");
	if (logger->accesslog_fd == NULL) {
		PRINT_ERROR("cherokee_logger_ncsa: error opening %s for append\n", logger->accesslog_filename); 
		return ret_error;
	}
#ifndef _WIN32
	fcntl (fileno (logger->accesslog_fd), F_SETFD, 1);
#endif	

        logger->errorlog_fd  = fopen (logger->errorlog_filename, "a+");
	if (logger->errorlog_fd == NULL) {
		PRINT_ERROR("cherokee_logger_ncsa: error opening %s for append\n", logger->errorlog_filename); 
		return ret_error;
	}
#ifndef _WIN32
	fcntl (fileno (logger->errorlog_fd), F_SETFD, 1);
#endif

	return ret_ok;
}


static ret_t
close_output (cherokee_logger_ncsa_t *logger)
{
	int n   = 2;
	int ret = 0;

	if (logger->errorlog_fd != NULL) {
		ret = fclose (logger->errorlog_fd);
		logger->errorlog_fd = NULL;
		n--;
	}
	
	if (logger->accesslog_fd != NULL) {
		ret |= fclose (logger->accesslog_fd);
		logger->accesslog_fd = NULL;
		n--;
	}
	
	if (n != 0) {
		closelog();
	}
	
	return (ret == 0) ? ret_ok : ret_error;
}


ret_t 
cherokee_logger_ncsa_init (cherokee_logger_ncsa_t *logger)
{
	return open_output (logger);
}


ret_t
cherokee_logger_ncsa_free (cherokee_logger_ncsa_t *logger)
{
	return close_output (logger);
}


ret_t 
cherokee_logger_ncsa_flush (cherokee_logger_ncsa_t *logger)
{
	int tmp;

	if (cherokee_buffer_is_empty (LOGGER_BUFFER(logger))) {
		return ret_ok;
	}

	if (logger->accesslog_fd == NULL) {
		cherokee_syslog (LOG_INFO, LOGGER_BUFFER(logger));
		return cherokee_buffer_clean (LOGGER_BUFFER(logger));
	}


	tmp = fwrite (LOGGER_BUFFER(logger)->buf, 1, LOGGER_BUFFER(logger)->len, logger->accesslog_fd); 
	fflush (logger->accesslog_fd);
	if (tmp < 0) {
		return ret_error;
	}

	if (tmp == LOGGER_BUFFER(logger)->len) {
		return cherokee_buffer_clean (LOGGER_BUFFER(logger));
	} 

	return cherokee_buffer_drop_endding (LOGGER_BUFFER(logger), tmp);
}


static ret_t
build_log_string (cherokee_logger_ncsa_t *logger, cherokee_connection_t *cnt, cherokee_buffer_t *buf)
{
  	long int           z;
	ret_t              ret;
	char              *username;
	const char        *method;
        const char        *version;
	struct tm         *conn_time;
	char               ipaddr[CHE_INET_ADDRSTRLEN];
	static long       *this_timezone = NULL;
	cherokee_buffer_t *combined_info = NULL;
	cherokee_buffer_t *request;

	/* Read the bogonow value from the server
	 */
	conn_time = &CONN_THREAD(cnt)->bogo_now_tm;

	/* Get the timezone reference
	 */
	if (this_timezone == NULL) {
		this_timezone = cherokee_get_timezone_ref();
	}
	
	z = - (*this_timezone / 60);

	memset (ipaddr, 0, sizeof(ipaddr));
	cherokee_socket_ntop (&cnt->socket, ipaddr, sizeof(ipaddr)-1);

	/* Look for the user
	 */
	if (cnt->validator && !cherokee_buffer_is_empty (&cnt->validator->user)) {
		username = cnt->validator->user.buf;
	} else {
		username = "-";
	}

	/* Get the method and version strings
	 */
	ret = cherokee_http_method_to_string (cnt->header.method, &method, NULL);
	if (unlikely(ret < ret_ok)) return ret;

	ret = cherokee_http_version_to_string (cnt->header.version, &version, NULL);
	if (unlikely(ret < ret_ok)) return ret;

	/* Look for the "combined" information
	 */
	if (logger->combined) {
		char *ref;
		char *usr;
		CHEROKEE_NEW2(referer, useragent, buffer);

		cherokee_header_copy_known (&cnt->header, header_referer, referer);
		cherokee_header_copy_known (&cnt->header, header_user_agent, useragent);

		ref = (referer->buf) ? referer->buf : "-";
		usr = (useragent->buf) ? useragent->buf : "";

		cherokee_buffer_new (&combined_info);
		cherokee_buffer_add_va (combined_info, " \"%s\" \"%s\"", ref, usr);

		cherokee_buffer_free (referer);
		cherokee_buffer_free (useragent);
	}

	request = cherokee_buffer_is_empty(&cnt->request_original) ? 
		&cnt->request : &cnt->request_original;

	/* Build the log string
	 */
	cherokee_buffer_add_va (buf, 
				"%s - %s [%02d/%s/%d:%02d:%02d:%02d %c%02d%02d] \"%s %s %s\" %d " FMT_OFFSET "%s\n",
				ipaddr,
				username, 
				conn_time->tm_mday, 
				month[conn_time->tm_mon], 
				1900 + conn_time->tm_year,
				conn_time->tm_hour, 
				conn_time->tm_min, 
				conn_time->tm_sec,
				(z < 0) ? '-' : '+', 
				(int) z/60, 
				(int) z%60, 
				method,
				request->buf, 
				version, 
				cnt->error_code,
				(CST_OFFSET) (cnt->range_end - cnt->range_start),
				(logger->combined) ? combined_info->buf : "");
	
	/* Maybe free some memory..
	 */
	if (combined_info != NULL) {
		cherokee_buffer_free (combined_info);
	}

	return ret_ok;

}


ret_t 
cherokee_logger_ncsa_write_string (cherokee_logger_ncsa_t *logger, const char *string)
{
	/* Write it down in the file..
	 */
	if (logger->accesslog_fd != NULL) {
		int ret;
		ret = fprintf (logger->accesslog_fd, "%s", string);

		return (ret > 0) ? ret_ok : ret_error;
	} 

	/* .. or, send it to syslog
	 */
	syslog (LOG_INFO, "%s", string);
	return ret_ok;
}


ret_t
cherokee_logger_ncsa_write_access (cherokee_logger_ncsa_t *logger, cherokee_connection_t *cnt)
{
	ret_t             ret;
	cherokee_buffer_t tmp = CHEROKEE_BUF_INIT;

	/* Build the log string
	 */
	ret = build_log_string (logger, cnt, &tmp);
	if (unlikely(ret < ret_ok)) return ret;
	
	/* Add it to the logger buffer
	 */
	ret = cherokee_buffer_add_buffer (LOGGER_BUFFER(logger), &tmp);
	if (unlikely(ret < ret_ok)) return ret;

	cherokee_buffer_mrproper (&tmp);
	return ret_ok;
}


ret_t 
cherokee_logger_ncsa_write_error (cherokee_logger_ncsa_t *logger, cherokee_connection_t *cnt)
{
	ret_t             ret;
	cherokee_buffer_t tmp = CHEROKEE_BUF_INIT;

	/* Build the log string
	 */
	ret = build_log_string (logger, cnt, &tmp);
	if (unlikely(ret < ret_ok)) return ret;

	/* Write it in the error file
	 */
	if (logger->errorlog_fd != NULL) {
		cuint_t wrote;

		do {
			wrote = fwrite (tmp.buf, 1, tmp.len, logger->errorlog_fd); 
			if (wrote < 0) break;

			cherokee_buffer_move_to_begin (&tmp, wrote);
		} while (! cherokee_buffer_is_empty(&tmp));

                fflush(logger->errorlog_fd);

		return (wrote > 0) ? ret_ok : ret_error;
	}

	/* or, send it to syslog
	 */
	cherokee_syslog (LOG_ERR, &tmp);
	cherokee_buffer_mrproper (&tmp);

	return ret_ok;
}


ret_t 
cherokee_logger_ncsa_reopen (cherokee_logger_ncsa_t *logger)
{
	ret_t ret;

	ret = close_output (logger);
	if (unlikely(ret != ret_ok)) return ret;

	return open_output (logger);
}



/*   Library init function
 */

static cherokee_boolean_t _ncsa_is_init = false;

void
MODULE_INIT(ncsa) (cherokee_module_loader_t *loader)
{
	/* Init flag
	 */
	if (_ncsa_is_init) return;
	_ncsa_is_init = true;
}
