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
#include "trace.h"
#include "buffer.h"
#include "util.h"

#ifdef HAVE_SYSLOG_H
# include <syslog.h>
#endif

typedef struct {
	cherokee_buffer_t  modules;
	cherokee_boolean_t use_syslog;
	cherokee_boolean_t print_time;
	cherokee_boolean_t print_thread;
} cherokee_trace_t;


static cherokee_trace_t trace = {
	CHEROKEE_BUF_INIT, 
	false,
	false,
	false
};


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
	/* Store a copy of the modules
	 */
	cherokee_buffer_clean (&trace.modules);

	if (cherokee_buffer_case_cmp_str (modules, "none") != 0) {
		cherokee_buffer_add_buffer (&trace.modules, modules);
	}
	
	/* Check the special properties
	 */
	trace.use_syslog   = (strstr (modules->buf, "syslog") != NULL);
	trace.print_time   = (strstr (modules->buf, "time") != NULL);
	trace.print_thread = (strstr (modules->buf, "thread") != NULL);

	return ret_ok;
}


ret_t 
cherokee_trace_do_trace (const char *entry, const char *file, int line, const char *func, const char *fmt, ...)
{
	char               *p;
	char               *lentry;
	char               *lentry_end;
	va_list             args;
	struct tm           now;
	time_t              now_time;
	cherokee_buffer_t  *trace_modules = &trace.modules;
	cherokee_boolean_t  do_log        = false;
	cherokee_buffer_t   entries       = CHEROKEE_BUF_INIT;

	/* Return ASAP if nothing is being traced
	 */
	if (cherokee_buffer_is_empty (&trace.modules))
		return ret_ok;

	/* Parse the In-code module string
	 */
	cherokee_buffer_add (&entries, entry, strlen(entry));

	for (lentry = entries.buf;;) {
		lentry_end = strchr (lentry, ',');
		if (lentry_end) *lentry_end = '\0';

		/* Check for 'all'
		 */
		p = strstr (trace_modules->buf, "all");
		if (p) do_log = true;

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
	if (! do_log) 
		goto out;

	/* Format the message and log it:
	 * 'entries' is not needed at this stage, reuse it
	 */
	cherokee_buffer_clean (&entries);
	if (trace.print_thread) {
		cherokee_buffer_add_va (&entries, "{%p} ", CHEROKEE_THREAD_SELF);
	}

	if (trace.print_time) {
		now_time = time(NULL);
		cherokee_localtime (&now_time, &now);
		cherokee_buffer_add_va (&entries, "[%02d/%02d/%d:%02d:%02d:%02d] ",
					now.tm_mday, 
					now.tm_mon, 
					now.tm_year + 1900,
					now.tm_hour, 
					now.tm_min, 
					now.tm_sec);
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
	return ret_ok;
}


ret_t 
cherokee_trace_get_trace (cherokee_buffer_t **buf_ref)
{
	if (buf_ref == NULL)
		return ret_not_found;

	*buf_ref = &trace.modules;
	return ret_ok;
}
