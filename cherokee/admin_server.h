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

#ifndef CHEROKEE_ADMIN_SERVER_H
#define CHEROKEE_ADMIN_SERVER_H

#include "handler_admin.h"

ret_t cherokee_admin_server_reply_get_port (cherokee_handler_admin_t *ahdl, cherokee_buffer_t *question, cherokee_buffer_t *reply);
ret_t cherokee_admin_server_reply_set_port (cherokee_handler_admin_t *ahdl, cherokee_buffer_t *question, cherokee_buffer_t *reply);

ret_t cherokee_admin_server_reply_get_port_tls (cherokee_handler_admin_t *ahdl, cherokee_buffer_t *question, cherokee_buffer_t *reply);
ret_t cherokee_admin_server_reply_set_port_tls (cherokee_handler_admin_t *ahdl, cherokee_buffer_t *question, cherokee_buffer_t *reply);

ret_t cherokee_admin_server_reply_get_tx (cherokee_handler_admin_t *ahdl, cherokee_buffer_t *question, cherokee_buffer_t *reply);
ret_t cherokee_admin_server_reply_get_rx (cherokee_handler_admin_t *ahdl, cherokee_buffer_t *question, cherokee_buffer_t *reply);

ret_t cherokee_admin_server_reply_get_connections (cherokee_handler_admin_t *ahdl, cherokee_buffer_t *question, cherokee_buffer_t *reply);
ret_t cherokee_admin_server_reply_del_connection  (cherokee_handler_admin_t *ahdl, cherokee_buffer_t *question, cherokee_buffer_t *reply);

ret_t cherokee_admin_server_reply_get_trace (cherokee_handler_admin_t *ahdl, cherokee_buffer_t *question, cherokee_buffer_t *reply);
ret_t cherokee_admin_server_reply_set_trace (cherokee_handler_admin_t *ahdl, cherokee_buffer_t *question, cherokee_buffer_t *reply);

ret_t cherokee_admin_server_reply_get_thread_num  (cherokee_handler_admin_t *ahdl, cherokee_buffer_t *question, cherokee_buffer_t *reply);
ret_t cherokee_admin_server_reply_set_backup_mode (cherokee_handler_admin_t *ahdl, cherokee_buffer_t *question, cherokee_buffer_t *reply);

#endif /* CHEROKEE_ADMIN_SERVER_H */
