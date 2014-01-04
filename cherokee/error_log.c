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
#include "error_log.h"
#include "nullable.h"
#include "init.h"
#include "util.h"

static cherokee_logger_writer_t *default_error_writer = NULL;


/* Include the error information */
#include "errors.h"


ret_t
cherokee_error_set_default (cherokee_logger_writer_t *writer)
{
	default_error_writer = writer;
	return ret_ok;
}


static void
skip_args (va_list ap, const char *prev_string)
{
	const char *p = prev_string;

	p = strchr (prev_string, '%');
	while (p) {
		p++;
		switch (*p) {
		case 's':
			va_arg (ap, char *);
			break;
		case 'd':
			va_arg (ap, int);
			break;
		default:
//			LOG_CRITICAL (CHEROKEE_ERROR_ERRORLOG_PARAM, p);
			break;
		}

		p = strchr (p, '%');
	}

}


static ret_t
report_error (cherokee_buffer_t *buf)
{
	cherokee_logger_writer_t *writer = NULL;

	/* 1st Option, the thread variable
	 */
	writer = LOGGER_WRITER (CHEROKEE_THREAD_PROP_GET (thread_error_writer_ptr));

	/* 2nd, the default error logger
	 */
	if (writer == NULL) {
		writer = default_error_writer;
	}

	/* Last resource: Print it to stderr.
	 */
	if ((writer == NULL) || (! writer->initialized)) {
		fprintf (stderr, "%s\n", buf->buf);
		fflush (stderr);
		return ret_ok;
	}

	/* Do logging
	 */
	if (writer->initialized) {
		cherokee_buffer_t *writer_log = NULL;

		cherokee_logger_writer_get_buf (writer, &writer_log);
		cherokee_buffer_add_buffer (writer_log, buf);
		cherokee_logger_writer_flush (writer, true);
		cherokee_logger_writer_release_buf (writer);

		return ret_ok;
	}

	return ret_ok;
}


static void
render_python_error (cherokee_error_type_t   type,
                     const char             *filename,
                     int                     line,
                     int                     error_num,
                     const cherokee_error_t *error,
                     cherokee_buffer_t      *output,
                     va_list                 ap)
{
	va_list           ap_tmp;
	cherokee_buffer_t tmp     = CHEROKEE_BUF_INIT;

	/* Dict: open */
	cherokee_buffer_add_char (output, '{');

	/* Error type */
	cherokee_buffer_add_str (output, "'type': \"");

	switch (type) {
	case cherokee_err_warning:
		cherokee_buffer_add_str (output, "warning");
		break;
	case cherokee_err_error:
		cherokee_buffer_add_str (output, "error");
		break;
	case cherokee_err_critical:
		cherokee_buffer_add_str (output, "critical");
		break;
	default:
		SHOULDNT_HAPPEN;
	}
	cherokee_buffer_add_str (output, "\", ");

	/* Time */
	cherokee_buffer_add_str  (output, "'time': \"");
	cherokee_buf_add_bogonow (output, false);
	cherokee_buffer_add_str  (output, "\", ");

	/* Render the title */
	va_copy (ap_tmp, ap);
	cherokee_buffer_add_str     (output, "'title': \"");
	cherokee_buffer_add_va_list (output, error->title, ap_tmp);
	cherokee_buffer_add_str     (output, "\", ");
	skip_args (ap, error->title);

	/* File and line*/
	cherokee_buffer_add_str (output, "'code': \"");
	cherokee_buffer_add_va  (output, "%s:%d", filename, line);
	cherokee_buffer_add_str (output, "\", ");

	/* Error number */
	cherokee_buffer_add_str (output, "'error': \"");
	cherokee_buffer_add_va  (output, "%d", error_num);
	cherokee_buffer_add_str (output, "\", ");

	/* Description */
	if (error->description) {
		va_copy (ap_tmp, ap);
		cherokee_buffer_clean           (&tmp);
		cherokee_buffer_add_str         (output, "'description': \"");
		cherokee_buffer_add_va_list     (&tmp, error->description, ap_tmp);
		cherokee_buffer_add_escape_html (output, &tmp);
		cherokee_buffer_add_str         (output, "\", ");

		/* ARGS: Skip 'description' */
		skip_args (ap, error->description);
	}

	/* Admin URL */
	if (error->admin_url) {
		va_copy (ap_tmp, ap);
		cherokee_buffer_add_str     (output, "'admin_url': \"");
		cherokee_buffer_add_va_list (output, error->admin_url, ap_tmp);
		cherokee_buffer_add_str     (output, "\", ");

		/* ARGS: Skip 'admin_url' */
		skip_args (ap, error->admin_url);
	}

	/* Debug information */
	if (error->debug) {
		va_copy (ap_tmp, ap);
		cherokee_buffer_clean           (&tmp);
		cherokee_buffer_add_str         (output, "'debug': \"");
		cherokee_buffer_add_va_list     (&tmp, error->debug, ap_tmp);
		cherokee_buffer_add_escape_html (output, &tmp);
		cherokee_buffer_add_str         (output, "\", ");

		/* ARGS: Skip 'debug' */
		skip_args (ap, error->debug);
	}

	/* Version */
	cherokee_buffer_add_str (output, "'version': \"");
	cherokee_buffer_add_str (output, PACKAGE_VERSION);
	cherokee_buffer_add_str (output, "\", ");

	cherokee_buffer_add_str (output, "'compilation_date': \"");
	cherokee_buffer_add_str (output, __DATE__ " " __TIME__);
	cherokee_buffer_add_str (output, "\", ");

	cherokee_buffer_add_str (output, "'configure_args': \"");
	cherokee_buffer_clean   (&tmp);
	cherokee_buffer_add_str (&tmp, CHEROKEE_CONFIG_ARGS);
	cherokee_buffer_add_escape_html (output, &tmp);
	cherokee_buffer_add_buffer (output, &tmp);
	cherokee_buffer_add_str (output, "\", ");

	/* Backtrace */
	if (error->show_backtrace) {
		cherokee_buffer_add_str (output, "'backtrace': \"");
#ifdef BACKTRACES_ENABLED
		cherokee_buffer_clean (&tmp);
		cherokee_buf_add_backtrace (&tmp, 2, "\\n", "");
		cherokee_buffer_add_escape_html (output, &tmp);
#endif
		cherokee_buffer_add_str (output, "\", ");
	}

	/* Let's finish here.. */
	if (strcmp (output->buf + output->len - 2, ", ") == 0) {
		cherokee_buffer_drop_ending (output, 2);
	}
	cherokee_buffer_add_str (output, "}\n");

	/* Clean up */
	cherokee_buffer_mrproper (&tmp);
}


static void
render_human_error (cherokee_error_type_t   type,
                    const char             *filename,
                    int                     line,
                    int                     error_num,
                    const cherokee_error_t *error,
                    cherokee_buffer_t      *output,
                    va_list                 ap)
{
	va_list ap_tmp;

	UNUSED (error_num);

	/* Time */
	cherokee_buffer_add_char (output, '[');
	cherokee_buf_add_bogonow (output, false);
	cherokee_buffer_add_str  (output, "] ");

	/* Error type */
	switch (type) {
	case cherokee_err_warning:
		cherokee_buffer_add_str (output, "(warning)");
		break;
	case cherokee_err_error:
		cherokee_buffer_add_str (output, "(error)");
		break;
	case cherokee_err_critical:
		cherokee_buffer_add_str (output, "(critical)");
		break;
	default:
		SHOULDNT_HAPPEN;
	}

	/* Source */
	cherokee_buffer_add_va (output, " %s:%d - ", filename, line);

	/* Error */
	va_copy (ap_tmp, ap);
	cherokee_buffer_add_va_list (output, error->title, ap_tmp);
	skip_args (ap, error->title);

	/* Description */
	if (error->description) {
		va_copy (ap_tmp, ap);
		cherokee_buffer_add_str     (output, " | ");
		cherokee_buffer_add_va_list (output, error->description, ap_tmp);
		cherokee_buffer_add_char    (output, '\n');
		skip_args (ap, error->title);
	}

	cherokee_buffer_add_char (output, '\n');
}

static ret_t
render_human_backtrace (const cherokee_error_t *error,
                        cherokee_buffer_t      *output)
{
#ifdef BACKTRACES_ENABLED
	if (error->show_backtrace) {
		cherokee_buf_add_backtrace (output, 2, "\n", "  ");
	}
#else
	UNUSED (error);
	UNUSED (output);
#endif
	return ret_ok;
}


static ret_t
render (cherokee_error_type_t  type,
        const char            *filename,
        int                    line,
        int                    error_num,
        va_list                ap,
        cherokee_buffer_t     *error_str)
{
	const cherokee_error_t *error;
	cherokee_boolean_t      readable;

	/* Get the error information
	 */
	error = &__cherokee_errors[error_num];
	if (unlikely (error->id != error_num)) {
		return ret_error;
	}

	/* Format
	 */
	if (! NULLB_IS_NULL (cherokee_readable_errors)) {
		readable = NULLB_TO_BOOL (cherokee_readable_errors);
	} else {
		readable = ! cherokee_admin_child;
	}

	/* Render
	 */
	if (readable) {
		render_human_error (type, filename, line, error_num, error, error_str, ap);
		cherokee_buffer_split_lines (error_str, TERMINAL_WIDTH, "    ");
		render_human_backtrace (error, error_str);
	} else {
		render_python_error (type, filename, line, error_num, error, error_str, ap);
	}

	return ret_ok;
}


ret_t
cherokee_error_log (cherokee_error_type_t  type,
                    const char            *filename,
                    int                    line,
                    int                    error_num, ...)
{
	va_list            ap;
	cherokee_buffer_t  error_str = CHEROKEE_BUF_INIT;

	/* Render the error message
	 */
	va_start (ap, error_num);
	render (type, filename, line, error_num, ap, &error_str);
	va_end (ap);

	/* Report it
	 */
	report_error (&error_str);

	/* Clean up
	 */
	cherokee_buffer_mrproper (&error_str);
	return ret_ok;
}

ret_t
cherokee_error_errno_log (int                    errnumber,
                          cherokee_error_type_t  type,
                          const char            *filename,
                          int                    line,
                          int                    error_num, ...)
{
	va_list            ap;
	const char        *errstr;
	char               err_tmp[ERROR_MAX_BUFSIZE];
	cherokee_buffer_t  error_str = CHEROKEE_BUF_INIT;

	/* Render the error message
	 */
	va_start (ap, error_num);
	render (type, filename, line, error_num, ap, &error_str);
	va_end (ap);

	/* Replace ${errno}
	 */
	errstr = cherokee_strerror_r (errnumber, err_tmp, sizeof(err_tmp));
	if (errstr == NULL) {
		errstr = "unknwon error (?)";
	}

	cherokee_buffer_replace_string (&error_str, (char *)"${errno}", 8,
	                                (char *) errstr, strlen(errstr));

	/* Report it
	 */
	report_error (&error_str);

	/* Clean up
	 */
	cherokee_buffer_mrproper (&error_str);
	return ret_ok;
}
