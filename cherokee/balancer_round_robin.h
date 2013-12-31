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

#ifndef CHEROKEE_BALANCER_ROUND_ROBIN_H
#define CHEROKEE_BALANCER_ROUND_ROBIN_H

#include "common-internal.h"
#include "balancer.h"
#include "plugin_loader.h"

typedef struct {
	cherokee_balancer_t  balancer;

	cherokee_list_t     *last_one;
	CHEROKEE_MUTEX_T    (mutex);
} cherokee_balancer_round_robin_t;

#define BAL_RR(x) ((cherokee_balancer_round_robin_t *)(x))

void PLUGIN_INIT_NAME(round_robin) (cherokee_plugin_loader_t *loader);

ret_t cherokee_balancer_round_robin_new  (cherokee_balancer_t **balancer);
ret_t cherokee_balancer_round_robin_free (cherokee_balancer_round_robin_t *balancer);

#endif /* CHEROKEE_BALANCER_ROUND_ROBIN_H */
