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
#include "init.h"
#include "cacheline.h"
#include "trace.h"
#include "ncpus.h"
#include "util.h"
#include "bogotime.h"
#include "threading.h"

/* Global variables
 */
cuint_t              cherokee_cacheline_size;
cint_t               cherokee_cpu_number;
cuint_t              cherokee_fdlimit;
cherokee_buffer_t    cherokee_tmp_dir;
cherokee_boolean_t   cherokee_admin_child;
cherokee_null_bool_t cherokee_readable_errors;

static cherokee_boolean_t _cherokee_init = false;

static ret_t
init_tmp_dir (void)
{
	ret_t ret;

	cherokee_buffer_init    (&cherokee_tmp_dir);
	cherokee_tmp_dir_copy   (&cherokee_tmp_dir);
	cherokee_buffer_add_str (&cherokee_tmp_dir, "/cherokee.XXXXXXXXXXX");

	ret = cherokee_mkdtemp (cherokee_tmp_dir.buf);
	if (unlikely (ret != ret_ok)) {
		return ret_error;
	}

	return ret_ok;
}

ret_t
cherokee_init (void)
{
	ret_t ret;

	if (_cherokee_init)
		return ret_ok;

	/* Init the tracing facility
	 */
	cherokee_trace_init();

	/* Init the bogotime mechanism
	 */
	cherokee_bogotime_init();
	cherokee_bogotime_update();

	/* Init threading stuff
	 */
	cherokee_threading_init();

	/* Get the CPU number
	 */
	dcc_ncpus (&cherokee_cpu_number);
	if (cherokee_cpu_number < 1) {
		LOG_WARNING (CHEROKEE_ERROR_INIT_CPU_NUMBER, cherokee_cpu_number);
		cherokee_cpu_number = 1;
	}

	/* Try to figure the L2 cache line size
	 */
	cherokee_cacheline_size_get (&cherokee_cacheline_size);

	/* Get the file descriptor number limit
	 */
	ret = cherokee_sys_fdlimit_get (&cherokee_fdlimit);
	if (ret < ret_ok) {
		LOG_ERROR_S (CHEROKEE_ERROR_INIT_GET_FD_LIMIT);
		return ret;
	}

	/* Temp directory
	 */
	ret = init_tmp_dir();
	if (ret != ret_ok) {
		return ret;
	}

	cherokee_admin_child     = false;
	cherokee_readable_errors = NULLB_NULL;

	_cherokee_init = true;
	return ret_ok;
}

ret_t
cherokee_mrproper (void)
{
	cherokee_buffer_mrproper (&cherokee_tmp_dir);

	cherokee_bogotime_free();
	cherokee_threading_free();

	return ret_ok;
}
