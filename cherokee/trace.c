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
#include "trace.h"
#include "buffer.h"
#include "util.h"
#include "bogotime.h"
#include "access.h"
#include "socket.h"
#include "connection-protected.h"

#ifdef HAVE_SYSLOG_H
# include <syslog.h>
#endif

typedef struct {
	cherokee_buffer_t   modules;
	cherokee_boolean_t  use_syslog;
	cherokee_boolean_t  print_time;
	cherokee_boolean_t  print_thread;
	cherokee_access_t  *from_filter;
} cherokee_trace_t;


static cherokee_trace_t trace = {
	CHEROKEE_BUF_INIT,
	false,
	false,
	false,
	NULL
};


/* cherokee_trace_do_trace() calls complex functions that might do
 * some tracing as well, and that would generate a loop. This flag is
 * used to disable the tracing while a previous trace call is ongoin.
 */
static cherokee_boolean_t disabled = false;


ret_t
cherokee_trace_init (void)
{
	const char        *env;
	cherokee_buffer_t  tmp = CHEROKEE_BUF_INIT;

	/* Read the environment variable
	 */
	env = getenv (TRACE_ENV);
	if (env == NULL)
		return ret_ok;

	/* Set it up
	 */
	cherokee_buffer_fake (&tmp, env, strlen(env));
	cherokee_trace_set_modules (&tmp);

	return ret_ok;
}


ret_t
cherokee_trace_set_modules (cherokee_buffer_t *modules)
{
	ret_t              ret;
	char              *p;
	char              *end;
	cherokee_buffer_t  tmp = CHEROKEE_BUF_INIT;

	/* Store a copy of the modules
	 */
	cherokee_buffer_clean (&trace.modules);

	if (trace.from_filter != NULL) {
		cherokee_access_free (trace.from_filter);
	}
	trace.from_filter = NULL;

	if (cherokee_buffer_case_cmp_str (modules, "none") != 0) {
		cherokee_buffer_add_buffer (&trace.modules, modules);
	}

	/* Check the special properties
	 */
	trace.use_syslog   = (strstr (modules->buf, "syslog") != NULL);
	trace.print_time   = (strstr (modules->buf, "time") != NULL);
	trace.print_thread = (strstr (modules->buf, "thread") != NULL);

	/* Even a more special 'from' property
	 */
	p = strstr (modules->buf, "from=");
	if (p != NULL) {
		p += 5;

		end = strchr(p, ',');
		if (end == NULL) {
			end = p + strlen(p);
		}

		if (end > p) {
			ret = cherokee_access_new (&trace.from_filter);
			if (ret != ret_ok) return ret_error;

			cherokee_buffer_add (&tmp, p, end-p);

			ret = cherokee_access_add (trace.from_filter, tmp.buf);
			if (ret != ret_ok) {
				ret = ret_error;
				goto out;
			}
		}
	}

	ret = ret_ok;

out:
	cherokee_buffer_mrproper (&tmp);
	return ret;
}


void
cherokee_trace_do_trace (const char *entry, const char *file, int line, const char *func, const char *fmt, ...)
{
	ret_t                  ret;
	char                  *p;
	char                  *lentry;
	char                  *lentry_end;
	va_list                args;
	cherokee_connection_t *conn;
	cherokee_buffer_t     *trace_modules = &trace.modules;
	cherokee_boolean_t     do_log        = false;
	cherokee_buffer_t      entries       = CHEROKEE_BUF_INIT;

	/* Prevents loops
	 */
	if (disabled) {
		return;
	}

	disabled = true;

	/* Return ASAP if nothing is being traced
	 */
	if (cherokee_buffer_is_empty (&trace.modules)) {
		goto out;
	}

	/* Check the connection source, if possible
	 */
	if (trace.from_filter != NULL) {
		conn = CONN (CHEROKEE_THREAD_PROP_GET (thread_connection_ptr));

		/* No conn, no trace entry
		 */
		if (conn == NULL) {
			goto out;
		}

		if (conn->socket.socket < 0) {
			goto out;
		}

		/* Skip the trace if the conn doesn't match
		 */
		ret = cherokee_access_ip_match (trace.from_filter, &conn->socket);
		if (ret != ret_ok) {
			goto out;
		}
	}

	/* Also, check for 'all'
	 */
	p = strstr (trace_modules->buf, "all");
	if (p == NULL) {
		/* Parse the In-code module string
		 */
		cherokee_buffer_add (&entries, entry, strlen(entry));

		for (lentry = entries.buf;;) {
			lentry_end = strchr (lentry, ',');
			if (lentry_end) *lentry_end = '\0';

			/* Check the type
			 */
			p = strstr (trace_modules->buf, lentry);
			if (p) {
				char *tmp = p + strlen(lentry);
				if ((*tmp == '\0') || (*tmp == ',') || (*tmp == ' '))
					do_log = true;
			}

			if ((lentry_end == NULL) || (do_log))
				break;

			lentry = lentry_end + 1;
		}

		/* Return if trace entry didn't match with the configured list
		 */
		if (! do_log) {
			goto out;
		}
	}

	/* Format the message and log it:
	 * 'entries' is not needed at this stage, reuse it
	 */
	cherokee_buffer_clean (&entries);
	if (trace.print_thread) {
		int        len;
		char       tmp[32+1];
		static int longest_len = 0;

		len = snprintf (tmp, 32+1, "%llX", (unsigned long long) CHEROKEE_THREAD_SELF);
		longest_len = MAX (longest_len, len);

		cherokee_buffer_add_str    (&entries, "{0x");
		cherokee_buffer_add_char_n (&entries, '0', longest_len - len);
		cherokee_buffer_add        (&entries, tmp, len);
		cherokee_buffer_add_str    (&entries, "} ");
	}

	if (trace.print_time) {
		cherokee_buffer_add_char (&entries, '[');
		cherokee_buf_add_bogonow (&entries, true);
		cherokee_buffer_add_str  (&entries, "] ");
	}

	cherokee_buffer_add_va (&entries, "%18s:%04d (%30s): ", file, line, func);

	va_start (args, fmt);
	cherokee_buffer_add_va_list (&entries, (char *)fmt, args);
	va_end (args);

	if (trace.use_syslog) {
		cherokee_syslog (LOG_DEBUG, &entries);
	} else {
#ifdef HAVE_FLOCKFILE
		flockfile (stdout);
#endif
		fprintf (stdout, "%s", entries.buf);
#ifdef HAVE_FUNLOCKFILE
		funlockfile (stdout);
#endif
	}

out:
	cherokee_buffer_mrproper (&entries);
	disabled = false;
}


ret_t
cherokee_trace_get_trace (cherokee_buffer_t **buf_ref)
{
	if (buf_ref == NULL)
		return ret_not_found;

	*buf_ref = &trace.modules;
	return ret_ok;
}


int
cherokee_trace_is_tracing (void)
{
	return (! cherokee_buffer_is_empty (&trace.modules));
}
