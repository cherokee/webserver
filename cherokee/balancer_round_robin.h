/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/* Cherokee
 *
 * Authors:
 *      Alvaro Lopez Ortega <alvaro@alobbs.com>
 *
 * Copyright (C) 2001-2006 Alvaro Lopez Ortega
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
	cherokee_balancer_t balancer;
} cherokee_balancer_round_robin_t;


#define PROP_RR(x)      ((cherokee_balancer_round_robin_props_t *)(x))
#define BAL_RR(x)       ((cherokee_balancer_round_robin_file_t *)(x))
#define BAL_RR_PROP(x)  (PROP_RR(BALANCER(x)->props))


ret_t cherokee_balancer_round_robin_new       (cherokee_balancer_t **hdl, cherokee_connection_t *cnt, cherokee_balancer_props_t *props);
ret_t cherokee_balancer_round_robin_configure (cherokee_config_node_t *conf, cherokee_server_t *srv, void **props);

#endif /* CHEROKEE_BALANCER_ROUND_ROBIN_H */
