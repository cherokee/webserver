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

#ifndef CHEROKEE_FLCACHE_H
#define CHEROKEE_FLCACHE_H

#include <cherokee/common.h>
#include <cherokee/buffer.h>
#include <cherokee/avl_flcache.h>
#include <cherokee/connection.h>
#include <cherokee/config_node.h>

CHEROKEE_BEGIN_DECLS

/* Forward declaration */
typedef struct cherokee_flcache         cherokee_flcache_t;
typedef struct cherokee_flcache_entry   cherokee_flcache_entry_t;
typedef struct cherokee_flcache_conn    cherokee_flcache_conn_t;

/* Classes
 */

typedef enum {
	flcache_mode_undef,
	flcache_mode_in,
	flcache_mode_out,
	flcache_mode_error
} cherokee_flcache_mode_t;

typedef enum {
	flcache_policy_explicitly_allowed,
	flcache_policy_all_but_forbidden
} cherokee_flcache_policy_t;


struct cherokee_flcache {
	cherokee_avl_flcache_t    request_map;
	cherokee_buffer_t         local_directory;
	culong_t                  last_file_id;
};

struct cherokee_flcache_conn {
	cherokee_flcache_mode_t      mode;
	int                          fd;

	/* Outbound */
	clong_t                      header_sent;
	clong_t                      response_sent;
	cherokee_avl_flcache_node_t *avl_node_ref;
	cherokee_buffer_t            header;
};


/* Front-line cache
 */
ret_t cherokee_flcache_new             (cherokee_flcache_t **flcache);
ret_t cherokee_flcache_free            (cherokee_flcache_t  *flcache);
ret_t cherokee_flcache_configure       (cherokee_flcache_t  *flcache, cherokee_config_node_t *conf, void *vserver);

ret_t cherokee_flcache_cleanup         (cherokee_flcache_t *flcache);
ret_t cherokee_flcache_req_get_cached  (cherokee_flcache_t *flcache, cherokee_connection_t *conn);
ret_t cherokee_flcache_req_is_storable (cherokee_flcache_t *flcache, cherokee_connection_t *conn);
ret_t cherokee_flcache_req_set_store   (cherokee_flcache_t *flcache, cherokee_connection_t *conn);
ret_t cherokee_flcache_del_entry       (cherokee_flcache_t *flcache, cherokee_avl_flcache_node_t *entry);
ret_t cherokee_flcache_purge_path      (cherokee_flcache_t *flcache, cherokee_buffer_t *path);

/* Front-line cache connection
 */
ret_t cherokee_flcache_conn_init          (cherokee_flcache_conn_t *flcache_conn);
ret_t cherokee_flcache_conn_clean         (cherokee_flcache_conn_t *flcache_conn);

ret_t cherokee_flcache_conn_write_header  (cherokee_flcache_conn_t *flcache_conn, cherokee_connection_t *conn);
ret_t cherokee_flcache_conn_write_body    (cherokee_flcache_conn_t *flcache_conn, cherokee_connection_t *conn);
ret_t cherokee_flcache_conn_commit_header (cherokee_flcache_conn_t *flcache_conn, cherokee_connection_t *conn);

ret_t cherokee_flcache_conn_send_header   (cherokee_flcache_conn_t *flcache_conn, cherokee_connection_t *conn);
ret_t cherokee_flcache_conn_send_body     (cherokee_flcache_conn_t *flcache_conn, cherokee_connection_t *conn);


CHEROKEE_END_DECLS

#endif /* CHEROKEE_FLCACHE_H */
