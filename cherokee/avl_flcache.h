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

#ifndef CHEROKEE_FLCACHE_AVL_H
#define CHEROKEE_FLCACHE_AVL_H

#include <cherokee/common.h>
#include <cherokee/avl_generic.h>
#include <cherokee/connection.h>

CHEROKEE_BEGIN_DECLS

typedef enum {
	flcache_status_undef,
	flcache_status_storing,
	flcache_status_ready
} cherokee_flcache_status_t;

/* AVL Tree Node
 */
typedef struct {
	cherokee_avl_generic_node_t  base;
	cherokee_list_t              to_del;

	cint_t                       ref_count;
	CHEROKEE_MUTEX_T            (ref_count_mutex);

	cherokee_buffer_t            request;
	cherokee_buffer_t            query_string;
	cherokee_buffer_t            content_encoding;

	cherokee_connection_t       *conn_ref;

	cherokee_flcache_status_t    status;
	cherokee_buffer_t            file;
	cullong_t                    file_size;

	time_t                       created_at;
	time_t                       valid_until;
} cherokee_avl_flcache_node_t;


/* AVL Tree
 */
typedef struct {
	cherokee_avl_generic_t  base;
	CHEROKEE_RWLOCK_T      (base_rwlock);
} cherokee_avl_flcache_t;


#define AVL_FLCACHE(a)      ((cherokee_avl_flcache_t *)(a))
#define AVL_FLCACHE_NODE(a) ((cherokee_avl_flcache_node_t *)(a))


ret_t cherokee_avl_flcache_init     (cherokee_avl_flcache_t *avl);
ret_t cherokee_avl_flcache_mrproper (cherokee_avl_flcache_t *avl, cherokee_func_free_t free_value);
ret_t cherokee_avl_flcache_cleanup  (cherokee_avl_flcache_t *avl);

ret_t cherokee_avl_flcache_add        (cherokee_avl_flcache_t       *avl,
                                       cherokee_connection_t        *conn,
                                       cherokee_avl_flcache_node_t **node);

ret_t cherokee_avl_flcache_get        (cherokee_avl_flcache_t       *avl,
                                       cherokee_connection_t        *conn,
                                       cherokee_avl_flcache_node_t **node);

ret_t cherokee_avl_flcache_del        (cherokee_avl_flcache_t       *avl,
                                       cherokee_avl_flcache_node_t  *node);

ret_t cherokee_avl_flcache_purge_path (cherokee_avl_flcache_t       *avl,
                                       cherokee_buffer_t            *path);

CHEROKEE_END_DECLS

#endif /* CHEROKEE_FLCACHE_AVL_H */
