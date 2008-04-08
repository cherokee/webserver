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

#if !defined (CHEROKEE_INSIDE_CHEROKEE_H) && !defined (CHEROKEE_COMPILATION)
# error "Only <cherokee/cherokee.h> can be included directly, this file may disappear or change contents."
#endif

#ifndef CHEROKEE_CONNECTION_INFO_H
#define CHEROKEE_CONNECTION_INFO_H

#include <cherokee/common.h>
#include <cherokee/list.h>
#include <cherokee/buffer.h>
#include <cherokee/connection.h>
#include <cherokee/server.h>
#include <cherokee/handler.h>


CHEROKEE_BEGIN_DECLS

typedef struct {
	cherokee_list_t      list_node;
	cherokee_buffer_t    id;              /* ID */
	cherokee_buffer_t    phase;           /* Current task */
	cherokee_buffer_t    request;         /* Request string */
	cherokee_buffer_t    rx;              /* Data received */
	cherokee_buffer_t    tx;              /* Data transmited */
	cherokee_buffer_t    total_size;      /* Size of data to be sent */
	cherokee_buffer_t    ip;              /* Remote IP */
	cherokee_buffer_t    percent;         /* tx * 100 / total_size */
	cherokee_buffer_t    handler;         /* Connection handler */
	cherokee_buffer_t    icon;            /* Icon filename */
} cherokee_connection_info_t;

#define CONN_INFO(i)  ((cherokee_connection_info_t *)(i))


ret_t cherokee_connection_info_new      (cherokee_connection_info_t **info);
ret_t cherokee_connection_info_free     (cherokee_connection_info_t  *info);

ret_t cherokee_connection_info_fill_up     (cherokee_connection_info_t *info, cherokee_connection_t *conn);

ret_t cherokee_connection_info_list_thread (cherokee_list_t *infos_list, void *thread, cherokee_handler_t *self);
ret_t cherokee_connection_info_list_server (cherokee_list_t *infos_list, cherokee_server_t *server, cherokee_handler_t *self);

CHEROKEE_END_DECLS

#endif /* CHEROKEE_CONNECTION_INFO_H */
