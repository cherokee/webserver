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

#ifndef CHEROKEE_BALANCER_ROUND_ROBIN_H
#define CHEROKEE_BALANCER_ROUND_ROBIN_H

#include "common-internal.h"
#include "balancer.h"


typedef struct {
	cherokee_balancer_t  balancer;

	cuint_t              last_one;	
	CHEROKEE_MUTEX_T    (last_one_mutex);
} cherokee_balancer_round_robin_t;

#define BAL_RR(x)       ((cherokee_balancer_round_robin_t *)(x))


ret_t cherokee_balancer_round_robin_new       (cherokee_balancer_t **balancer);
ret_t cherokee_balancer_round_robin_free      (cherokee_balancer_round_robin_t *balancer);

#endif /* CHEROKEE_BALANCER_ROUND_ROBIN_H */
