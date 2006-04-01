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

#ifndef __CHEROKEE_HANDLER_REMOTE_CONTROL_H__
#define __CHEROKEE_HANDLER_REMOTE_CONTROL_H__

#include "common.h"

#include "buffer.h"
#include "handler.h"
#include "connection.h"


typedef struct {
	cherokee_handler_t handler;
	cherokee_buffer_t *buffer;
} cherokee_handler_remote_control_t;

#define REMOTE_HANDLER(x)  ((cherokee_handler_remote_control_t *)(x))


/* Library init function
 */
void remote_control_init ();


ret_t cherokee_handler_remote_control_new   (cherokee_handler_t **hdl, cherokee_connection_t *cnt, cherokee_table_t *properties);

/* virtual methods implementation
 */
ret_t cherokee_handler_remote_control_init        (cherokee_handler_remote_control_t *hdl);
ret_t cherokee_handler_remote_control_free        (cherokee_handler_remote_control_t *hdl);
ret_t cherokee_handler_remote_control_step        (cherokee_handler_remote_control_t *hdl, cherokee_buffer_t *buffer);
ret_t cherokee_handler_remote_control_add_headers (cherokee_handler_remote_control_t *hdl, cherokee_buffer_t *buffer);

#endif /* __CHEROKEE_HANDLER_REMOTE_CONTROL_H__ */
