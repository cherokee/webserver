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

#ifndef CHEROKEE_LIMITER_H
#define CHEROKEE_LIMITER_H

#include "common.h"
#include "list.h"
#include "bogotime.h"

typedef struct {
	cherokee_list_t conns;
	cuint_t         conns_num;
} cherokee_limiter_t;

ret_t cherokee_limiter_init     (cherokee_limiter_t *limiter);
ret_t cherokee_limiter_mrproper (cherokee_limiter_t *limiter);

ret_t cherokee_limiter_add_conn (cherokee_limiter_t *limiter,
				 void               *conn);

cherokee_msec_t
cherokee_limiter_get_time_limit (cherokee_limiter_t *limiter,
				 cherokee_msec_t     fdwatch_msecs);

ret_t cherokee_limiter_reactive (cherokee_limiter_t *limiter,
				 void               *thread);

#endif /* CHEROKEE_LIMITER_H */
