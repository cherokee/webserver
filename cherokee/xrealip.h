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


#if !defined (CHEROKEE_INSIDE_CHEROKEE_H) && !defined (CHEROKEE_COMPILATION)
# error "Only <cherokee/cherokee.h> can be included directly, this file may disappear or change contents."
#endif

#ifndef CHEROKEE_X_REAL_IP_H
#define CHEROKEE_X_REAL_IP_H

#include <cherokee/common.h>
#include <cherokee/buffer.h>
#include <cherokee/config_node.h>
#include <cherokee/access.h>

CHEROKEE_BEGIN_DECLS

typedef struct {
	cherokee_boolean_t  enabled;
	cherokee_access_t  *access;
	cherokee_boolean_t  access_all;
} cherokee_x_real_ip_t;

#define X_REAL_IP(o) ((cherokee_x_real_ip_t *)(o))


ret_t cherokee_x_real_ip_init       (cherokee_x_real_ip_t *real_ip);
ret_t cherokee_x_real_ip_mrproper   (cherokee_x_real_ip_t *real_ip);
ret_t cherokee_x_real_ip_configure  (cherokee_x_real_ip_t *real_ip, cherokee_config_node_t *conf);
ret_t cherokee_x_real_ip_is_allowed (cherokee_x_real_ip_t *real_ip, cherokee_socket_t *sock);

CHEROKEE_END_DECLS

#endif /* CHEROKEE_X_REAL_IP_H */
