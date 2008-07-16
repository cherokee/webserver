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
#include "init.h"
#include "cacheline.h"
#include "trace.h"
#include "ncpus.h"
#include "util.h"
#include "bogotime.h"


cuint_t cherokee_cacheline_size;
cint_t  cherokee_cpu_number;
cuint_t cherokee_fdlimit;

static cherokee_boolean_t _cherokee_init = false;

ret_t 
cherokee_init (void)
{
	ret_t ret;

	if (_cherokee_init)
		return ret_ok;

#ifdef _WIN32
	init_win32();
#endif	

	/* Init the tracing facility
	 */
	cherokee_trace_init();

	/* Init the bogotime mechanism
	 */
	cherokee_bogotime_init();

	/* Get the CPU number
	 */
	dcc_ncpus (&cherokee_cpu_number);
	if (cherokee_cpu_number < 1) {
		PRINT_ERROR ("Bad CPU number: %d, using 1\n", cherokee_cpu_number);
		cherokee_cpu_number = 1;
	}

	/* Try to figure the L2 cache line size
	 */
	cherokee_cacheline_size_get (&cherokee_cacheline_size);

	/* Get the file descriptor number limit
	 */
	ret = cherokee_sys_fdlimit_get (&cherokee_fdlimit);
	if (ret < ret_ok) {
		PRINT_ERROR_S ("ERROR: Unable to get file descriptor limit\n");
		return ret;
	}

	_cherokee_init = true;
	return ret_ok;
}
