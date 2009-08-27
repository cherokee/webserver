/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/* Cherokee
 *
 * Authors:
 *      Alvaro Lopez Ortega <alvaro@alobbs.com>
 *
 * Copyright (C) 2001-2009 Alvaro Lopez Ortega
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
#include "error_log.h"
#include "util.h"

static cherokee_logger_t *default_error_logger = NULL;

ret_t
cherokee_error_log_set_log (cherokee_logger_t *logger)
{
	default_error_logger = logger;
	return ret_ok;
}

ret_t
cherokee_error_log (cherokee_error_type_t type,
		    const char *format, ...)
{
	va_list                   ap;
	cherokee_logger_t        *logger;
	cherokee_logger_writer_t *writer = NULL;
	cherokee_buffer_t         tmp    = CHEROKEE_BUF_INIT;

	/* Error message formatting
	 */
	cherokee_buf_add_bogonow (&tmp, false);

	switch (type) {
	case cherokee_err_warning:
		cherokee_buffer_add_str (&tmp, " (warning) ");
		break;
	case cherokee_err_error:
		cherokee_buffer_add_str (&tmp, " (error) ");
		break;
	case cherokee_err_critical:
		cherokee_buffer_add_str (&tmp, " (critical) ");
		break;
	}

	va_start (ap, format);
	cherokee_buffer_add_va_list (&tmp, format, ap);
	va_end (ap);

	/* Backtrace
	 */
#ifdef BACKTRACES_ENABLED
	cherokee_buf_add_backtrace (&tmp, 2);
#endif

	/* Logging: 1st option - connection's logger
	 */
	logger = LOGGER (CHEROKEE_THREAD_PROP_GET (thread_logger_error_ptr));

	/* Logging: 2nd option - default logger
	 */
	if (logger == NULL) {
		logger = default_error_logger;
	}

	/* Do logging
	 */
	if (logger) {
		cherokee_logger_get_error_writer (logger, &writer);
		if ((writer) && (writer->initialized)) {
			cherokee_logger_write_error (logger, &tmp);
		}
	}

	if ((logger == NULL) ||
	    (! writer) ||
	    ((writer) && (! writer->initialized)) ||
	    (type == cherokee_err_critical))
	{
		fprintf (stderr, "%s", tmp.buf);
	}

	cherokee_buffer_mrproper (&tmp);
	return ret_ok;
}


ret_t
cherokee_error_errno_log (int                    error,
			  cherokee_error_type_t  type,
			  const char            *format, ...)
{
	va_list           ap;
	const char       *errstr;
	char              err_tmp[ERROR_MAX_BUFSIZE];
	cherokee_buffer_t buffer = CHEROKEE_BUF_INIT;

	/* Get the error string
	 */
	errstr = cherokee_strerror_r (error, err_tmp, sizeof(err_tmp));
	if (errstr == NULL)
		errstr = "unknwon error (?)";

	/* Render
	 */
	cherokee_buffer_ensure_size (&buffer, 128);
	va_start (ap, format);
	cherokee_buffer_add_va_list (&buffer, format, ap);
	va_end (ap);

	/* Replace error
	 */
	cherokee_buffer_replace_string (&buffer, (char *)"${errno}", 8,
					(char *) errstr, strlen(errstr));

	/* Log & clean up
	 */
	cherokee_error_log (type, "%s", buffer.buf);
	cherokee_buffer_mrproper (&buffer);

	return ret_ok;
}
