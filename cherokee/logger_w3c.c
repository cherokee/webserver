/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/* Cherokee
 *
 * Authors:
 *      Alvaro Lopez Ortega <alvaro@alobbs.com>
 *
 * Copyright (C) 2001-2007 Alvaro Lopez Ortega
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
#include "logger_w3c.h"

#include <stdio.h>
#include <stdlib.h>
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
#include "module.h"
#include "server.h"
#include "server-protected.h"
#include "header.h"
#include "header-protected.h"

/* Plug-in initialization
 */
PLUGIN_INFO_LOGGER_EASIEST_INIT (w3c);


/* Documentation:
 * - http://www.w3.org/TR/WD-logfile
 */

/* Some constants
 */
static char *month[]   = {"Jan", "Feb", "Mar", "Apr", "May", "Jun",
			  "Jul", "Aug", "Sep", "Oct", "Nov", "Dec", NULL};

#define IN_ADDR(c) ((struct in_addr) (c).sin_addr)


ret_t
cherokee_logger_w3c_new  (cherokee_logger_t **logger, cherokee_config_node_t *config)
{
	ret_t                   ret;
	cherokee_config_node_t *subconf;
	CHEROKEE_NEW_STRUCT (n, logger_w3c);
	
	/* Init the base class object
	 */
	cherokee_logger_init_base (LOGGER(n), PLUGIN_INFO_PTR(w3c));

	MODULE(n)->init         = (logger_func_init_t) cherokee_logger_w3c_init;
	MODULE(n)->free         = (logger_func_free_t) cherokee_logger_w3c_free;
	LOGGER(n)->flush        = (logger_func_flush_t) cherokee_logger_w3c_flush;
	LOGGER(n)->write_error  = (logger_func_write_error_t) cherokee_logger_w3c_write_error;
	LOGGER(n)->write_access = (logger_func_write_access_t) cherokee_logger_w3c_write_access;
	LOGGER(n)->write_string = (logger_func_write_string_t) cherokee_logger_w3c_write_string;
	
	/* Init properties
	 */
	n->header_added = false;

	/* Init logger writer
	 */
	ret = cherokee_logger_writer_init (&n->writer);
	if (ret != ret_ok) return ret;
	
	ret = cherokee_config_node_get (config, "all", &subconf);
	if (ret == ret_ok) {
		ret = cherokee_logger_writer_configure (&n->writer, subconf);
		if (ret != ret_ok) return ret;
	}

	*logger = LOGGER(n);
	return ret_ok;
}


ret_t 
cherokee_logger_w3c_init (cherokee_logger_w3c_t *logger)
{
	ret_t ret;

	ret = cherokee_logger_writer_open (&logger->writer);
	if (ret != ret_ok) return ret;

	return ret_ok;
}


ret_t
cherokee_logger_w3c_free (cherokee_logger_w3c_t *logger)
{
	cherokee_logger_writer_mrproper (&logger->writer);
	return ret_ok;
}


ret_t 
cherokee_logger_w3c_reopen (cherokee_logger_w3c_t *logger)
{
	return cherokee_logger_writer_reopen (&logger->writer);
}


ret_t
cherokee_logger_w3c_flush (cherokee_logger_w3c_t *logger)
{
	return cherokee_logger_writer_flush (&logger->writer);
}


ret_t 
cherokee_logger_w3c_write_error  (cherokee_logger_w3c_t *logger, cherokee_connection_t *cnt)
{
	ret_t              ret;
	long int           z;
	const char        *method;
	struct tm         *conn_time;
	cherokee_buffer_t *log;
	cherokee_buffer_t *request;
	static long       *this_timezone = NULL;

	/* Get the logger writer buffer
	 */
	ret = cherokee_logger_writer_get_buf (&logger->writer, &log);
	if (unlikely (ret != ret_ok)) return ret;

	/* Read the bogonow value from the server
	 */
	conn_time = &CONN_THREAD(cnt)->bogo_now_tm;

	/* Get the timezone reference
	 */
	if (this_timezone == NULL) {
		this_timezone = cherokee_get_timezone_ref();
	}
	
	z = - (*this_timezone / 60);

	/* The method
	 */
	cherokee_http_method_to_string (cnt->header.method, &method, NULL);

	/* Build the string
	 */
	request = cherokee_buffer_is_empty(&cnt->request_original) ? 
		&cnt->request : &cnt->request_original;

	cherokee_buffer_add_va (log, 
				"%02d:%02d:%02d [error] %s %s\n",
				conn_time->tm_hour, 
				conn_time->tm_min, 
				conn_time->tm_sec,
				method,
				request->buf);
	return ret_ok;
}


ret_t 
cherokee_logger_w3c_write_string (cherokee_logger_w3c_t *logger, const char *string)
{
	ret_t              ret;
	cherokee_buffer_t *log;

	ret = cherokee_logger_writer_get_buf (&logger->writer, &log);
	if (unlikely (ret != ret_ok)) return ret;

	cherokee_buffer_add (log, string, strlen(string));
	return ret_ok;
}


ret_t
cherokee_logger_w3c_write_access (cherokee_logger_w3c_t *logger, cherokee_connection_t *cnt)
{
	ret_t              ret;
	long int           z;
	const char        *method;
	struct tm         *conn_time;
	cherokee_buffer_t *log;
	cherokee_buffer_t *request;
	static long       *this_timezone = NULL;

	/* Get the logger writer buffer
	 */
	ret = cherokee_logger_writer_get_buf (&logger->writer, &log);
	if (unlikely (ret != ret_ok)) return ret;

	/* Read the bogonow value from the server
	 */
	conn_time = &CONN_THREAD(cnt)->bogo_now_tm;

	if (! logger->header_added) {
		cherokee_buffer_add_va (log,
					"#Version 1.0\n"
					"#Date: %d02-%s-%4d %02d:%02d:%02d\n"
					"#Fields: time cs-method cs-uri\n",
					conn_time->tm_mday, 
					month[conn_time->tm_mon], 
					1900 + conn_time->tm_year,
					conn_time->tm_hour, 
					conn_time->tm_min, 
					conn_time->tm_sec);
		logger->header_added = true;
	}
	
	/* Get the timezone reference
	 */
	if (this_timezone == NULL) {
		this_timezone = cherokee_get_timezone_ref();
	}
	
	z = - (*this_timezone / 60);

	/* HTTP Method
	 */
	cherokee_http_method_to_string (cnt->header.method, &method, NULL);

	/* Build the string
	 */
	request = cherokee_buffer_is_empty(&cnt->request_original) ? 
		&cnt->request : &cnt->request_original;

	cherokee_buffer_add_va (log,
				"%02d:%02d:%02d %s %s\n",
				conn_time->tm_hour, 
				conn_time->tm_min, 
				conn_time->tm_sec,
				method,
				request->buf);
	return ret_ok;
}

