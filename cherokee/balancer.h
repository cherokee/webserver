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

#if !defined (CHEROKEE_INSIDE_CHEROKEE_H) && !defined (CHEROKEE_COMPILATION)
# error "Only <cherokee/cherokee.h> can be included directly, this file may disappear or change contents."
#endif

#ifndef CHEROKEE_BALANCER_H
#define CHEROKEE_BALANCER_H

#include <cherokee/common.h>
#include <cherokee/module.h>
#include <cherokee/connection.h>

CHEROKEE_BEGIN_DECLS


/* Hosts
 */
typedef struct {
	int foo;
} cherokee_balancer_host_t;


/* Handler properties
 */
typedef ret_t (* balancer_props_func_free_t)  (void  *balancerp);

typedef struct {
	balancer_props_func_free_t free;
} cherokee_balancer_props_t;


/* Balancer
 */
typedef ret_t (* balancer_dispatch_func_t) (void *balancer, cherokee_connection_t *conn, cherokee_balancer_host_t **hosts);
typedef ret_t (* balancer_free_func_t)     (void *balancer);

typedef struct {
	cherokee_module_t          module;

	/* Properties */
	cherokee_balancer_host_t **hosts;
	cuint_t                    hosts_len;
	cuint_t                    hosts_size;

	/* Virtual methods */
	balancer_dispatch_func_t   dispatch;
} cherokee_balancer_t;

#define BAL(b)  ((cherokee_balancer_t *)(b))

ret_t cherokee_balancer_init_base  (cherokee_balancer_t *balancer);
ret_t cherokee_balancer_mrproper   (cherokee_balancer_t *balancer);

ret_t cherokee_balancer_add_host   (cherokee_balancer_t *balancer, cherokee_balancer_host_t *host);

/* Virtual methods
 */
ret_t cherokee_balancer_dispatch   (cherokee_balancer_t *balancer, cherokee_connection_t *conn, cherokee_balancer_host_t **host);
ret_t cherokee_balancer_free       (cherokee_balancer_t *balancer);

CHEROKEE_END_DECLS

#endif /* CHEROKEE_BALANCER_H */
