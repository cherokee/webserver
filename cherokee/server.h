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

#ifndef CHEROKEE_SERVER_H
#define CHEROKEE_SERVER_H

#include <cherokee/common.h>
#include <cherokee/buffer.h>
#include <stddef.h>

CHEROKEE_BEGIN_DECLS


typedef struct cherokee_server cherokee_server_t;
#define SRV(x) ((cherokee_server_t *)(x))

ret_t cherokee_server_new                (cherokee_server_t **srv);
ret_t cherokee_server_free               (cherokee_server_t  *srv);
ret_t cherokee_server_clean              (cherokee_server_t  *srv);

ret_t cherokee_server_initialize         (cherokee_server_t *srv);
ret_t cherokee_server_step               (cherokee_server_t *srv);
ret_t cherokee_server_stop               (cherokee_server_t *srv);

void  cherokee_server_set_min_latency    (cherokee_server_t *srv, int msecs);
ret_t cherokee_server_unlock_threads     (cherokee_server_t *srv);

ret_t cherokee_server_read_config_file   (cherokee_server_t *srv, char *filename);
ret_t cherokee_server_read_config_string (cherokee_server_t *srv, cherokee_buffer_t *string);

ret_t cherokee_server_daemonize          (cherokee_server_t *srv);
ret_t cherokee_server_write_pidfile      (cherokee_server_t *srv);

ret_t cherokee_server_get_conns_num      (cherokee_server_t *srv, cuint_t *num);
ret_t cherokee_server_get_active_conns   (cherokee_server_t *srv, cuint_t *num);
ret_t cherokee_server_get_reusable_conns (cherokee_server_t *srv, cuint_t *num);
ret_t cherokee_server_get_total_traffic  (cherokee_server_t *srv, size_t *rx, size_t *tx);

ret_t cherokee_server_set_backup_mode    (cherokee_server_t *srv, cherokee_boolean_t active);
ret_t cherokee_server_get_backup_mode    (cherokee_server_t *srv, cherokee_boolean_t *active);
ret_t cherokee_server_log_reopen         (cherokee_server_t *srv);

/* System signal callback
 */
ret_t cherokee_server_handle_HUP   (cherokee_server_t *srv);
ret_t cherokee_server_handle_TERM  (cherokee_server_t *srv);
ret_t cherokee_server_handle_panic (cherokee_server_t *srv);


CHEROKEE_END_DECLS

#endif /* CHEROKEE_SERVER_H */
