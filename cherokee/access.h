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

#ifndef CHEROKEE_ACCESS_H
#define CHEROKEE_ACCESS_H

#include "common-internal.h"
#include "list.h"
#include "socket.h"

typedef struct {
	cherokee_list_t list_ips;
	cherokee_list_t list_subnets;
} cherokee_access_t;

ret_t cherokee_access_new  (cherokee_access_t **entry);
ret_t cherokee_access_free (cherokee_access_t  *entry);

ret_t cherokee_access_add        (cherokee_access_t *entry, char *ip_or_subnet);
ret_t cherokee_access_ip_match   (cherokee_access_t *entry, cherokee_socket_t *sock);

ret_t cherokee_access_print_debug (cherokee_access_t *entry);

#endif /* CHEROKEE_ACCESS_H */
