/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/* Cherokee
 *
 * Authors:
 *      Alvaro Lopez Ortega <alvaro@alobbs.com>
 *
 * Copyright (C) 2001-2011 Alvaro Lopez Ortega
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

#ifndef CHEROKEE_CONNECTION_POLL_H
#define CHEROKEE_CONNECTION_POLL_H

#include "common.h"
#include "fdpoll.h"


typedef struct {
	cherokee_poll_mode_t mode;
	int                  fd;
} cherokee_connection_pool_t;


ret_t cherokee_connection_poll_init     (cherokee_connection_pool_t *conn_poll);
ret_t cherokee_connection_poll_clean    (cherokee_connection_pool_t *conn_poll);
ret_t cherokee_connection_poll_mrproper (cherokee_connection_pool_t *conn_poll);
int   cherokee_connection_poll_is_set   (cherokee_connection_pool_t *conn_poll);

#endif /* CHEROKEE_CONNECTION_POLL_H */
