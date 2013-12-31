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

#include "common-internal.h"
#include "avl_flcache.h"
#include "connection.h"
#include "connection-protected.h"
#include "list.h"
#include "util.h"
#include "bogotime.h"

#define ENTRIES "avl,flcache"


static ret_t
conn_to_node (cherokee_connection_t       *conn,
              cherokee_avl_flcache_node_t *node)
{
	cherokee_avl_generic_node_init (AVL_GENERIC_NODE(node));

	/* Request */
	cherokee_buffer_init (&node->request);

	if (conn->request_original.len > 0) {
		cherokee_buffer_add_buffer (&node->request, &conn->request_original);
	} else {
		cherokee_buffer_add_buffer (&node->request, &conn->request);
	}

	/* Query string */
	cherokee_buffer_init (&node->query_string);
	cherokee_buffer_add_buffer (&node->query_string, &conn->query_string);

	/* Encoder:
	 * It will be filled in when the encoder object is instanced
	 */
	cherokee_buffer_init (&node->content_encoding);

	return ret_ok;
}


static ret_t
node_new (cherokee_avl_flcache_node_t **node,
          cherokee_connection_t        *conn)
{
	CHEROKEE_NEW_STRUCT (n, avl_flcache_node);

	INIT_LIST_HEAD (&n->to_del);
	cherokee_buffer_init (&n->file);
	CHEROKEE_MUTEX_INIT (&n->ref_count_mutex, CHEROKEE_MUTEX_FAST);

	n->conn_ref    = NULL;
	n->status      = flcache_status_undef;
	n->ref_count   = 0;
	n->file_size   = 0;
	n->valid_until = TIME_MAX;
	n->created_at  = cherokee_bogonow_now;

	conn_to_node (conn, n);

	*node = n;
	return ret_ok;
}

static ret_t
node_mrproper (cherokee_avl_flcache_node_t *node)
{
	CHEROKEE_MUTEX_DESTROY (&node->ref_count_mutex);

	cherokee_buffer_mrproper (&node->request);
	cherokee_buffer_mrproper (&node->query_string);
	cherokee_buffer_mrproper (&node->content_encoding);
	cherokee_buffer_mrproper (&node->file);

	return ret_ok;
}

static ret_t
node_free (cherokee_avl_flcache_node_t *node)
{
	if (node == NULL) {
		return ret_ok;
	}

	node_mrproper (node);
	free (node);

	return ret_ok;
}

static int
cmp_request (cherokee_avl_flcache_node_t *A,
             cherokee_avl_flcache_node_t *B)
{
	int                          re;
	cherokee_connection_t       *conn;
	cherokee_avl_flcache_node_t *node;
	cherokee_boolean_t           invert;

	/* Comparing against a cherokee_connection_t
	 */
	if (A->conn_ref || B->conn_ref) {
		if (A->conn_ref) {
			conn   = A->conn_ref;
			node   = B;
			invert = false;
		} else {
			conn = B->conn_ref;
			node = A;
			invert = true;
		}

		if (conn->request_original.len > 0) {
			re = cherokee_buffer_case_cmp_buf (&conn->request_original, &node->request);
		} else {
			re = cherokee_buffer_case_cmp_buf (&conn->request, &node->request);
		}

		return (invert) ? -re : re;
	}

	/* Comparing two nodes
	 */
	return cherokee_buffer_case_cmp_buf (&A->request, &B->request);
}

static int
cmp_query_string (cherokee_avl_flcache_node_t *A,
                  cherokee_avl_flcache_node_t *B)
{
	int                          re;
	cherokee_connection_t       *conn;
	cherokee_avl_flcache_node_t *node;
	cherokee_boolean_t           invert;

	/* Comparing against a cherokee_connection_t
	 */
	if (A->conn_ref || B->conn_ref) {
		if (A->conn_ref) {
			conn   = A->conn_ref;
			node   = B;
			invert = false;
		} else {
			conn = B->conn_ref;
			node = A;
			invert = true;
		}

		if (conn->query_string_original.len > 0) {
			re = cherokee_buffer_case_cmp_buf (&conn->query_string_original, &node->query_string);
		} else {
			re = cherokee_buffer_case_cmp_buf (&conn->query_string, &node->query_string);
		}

		return (invert) ? -re : re;
	}

	/* Comparing two nodes
	 */
	return cherokee_buffer_case_cmp_buf (&A->query_string, &B->query_string);
}

static int
cmp_encoding (cherokee_avl_flcache_node_t *A,
              cherokee_avl_flcache_node_t *B)
{
	ret_t                        ret;
	int                          re;
	char                        *p;
	cherokee_connection_t       *conn;
	cherokee_avl_flcache_node_t *node;
	cherokee_boolean_t           invert;
	char                        *header      = NULL;
	cuint_t                      header_len;

	/* Comparing against a cherokee_connection_t
	 */
	if (A->conn_ref || B->conn_ref) {
		if (A->conn_ref) {
			conn   = A->conn_ref;
			node   = B;
			invert = false;
		} else {
			conn = B->conn_ref;
			node = A;
			invert = true;
		}

		/* No "Content-encoding:"
		 */
		ret = cherokee_header_get_known (&conn->header, header_accept_encoding, &header, &header_len);
		if (ret != ret_ok) {
			/* Node not encoded */
			if (cherokee_buffer_is_empty (&node->content_encoding)) {
				return 0;
			}

			/* Node encoded */
			return (invert) ? 1 : -1;
		}

		/* For each entry of "Content-encoding:", Encoded node
		 */
		if (! cherokee_buffer_is_empty (&node->content_encoding)) {
			while ((p = (char *) strsep (&header, ",")) != NULL) {
				cuint_t len, total_len;

				if (*p == '\0') {
					*p = ',';
					continue;
				}

				total_len = strlen(p);

				/* Skip training CR, LF chars */
				len = total_len;
				while ((len > 1) && ((p[len -1] == '\r')||
				                     (p[len -1] == '\n')))
					len--;

				/* Compare */
				if (len <= 0) {
					re = -1;
				} else {
					re = strncasecmp (p, node->content_encoding.buf,
					                  MIN(len, node->content_encoding.len));
				}

				/* Restore original string */
				p[total_len] = ',';

				if (re == 0) {
					/* Matched */
					return 0;
				}
			}
		}

		/* Node isn't encoded
		 */
		if (cherokee_buffer_is_empty (&node->content_encoding)) {
			return 0;
		}

		/* Not supported
		 */
		return (invert) ? 1 : -1;
	}

	/* Comparing two nodes
	 */
	return cherokee_buffer_case_cmp_buf (&A->content_encoding, &B->content_encoding);
}

static int
node_cmp (cherokee_avl_flcache_node_t *A,
          cherokee_avl_flcache_node_t *B,
          cherokee_avl_flcache_t      *avl)
{
	int re;

	UNUSED (avl);

	/* Comparing with itself
	 */
	if (A == B)
		return 0;

	/* 1.- Request
	 */
	re = cmp_request (A, B);
	if (re != 0) {
		return re;
	}

	/* 2.- Query String
	 */
	re = cmp_query_string (A, B);
	if (re != 0) {
		return re;
	}

	/* 3.- Encoding
	 */
	re = cmp_encoding (A, B);
	if (re != 0) {
		return re;
	}

	return 0;
}

static int
node_is_empty (cherokee_avl_flcache_node_t *key)
{
	/* The key object can be either be:
	 * 1.- A reference to a cherokee_connection_t object
	 * 2.- An object storing information
	 */
	if (key->conn_ref)
		return false;

	return cherokee_buffer_is_empty (&key->request);
}


ret_t
cherokee_avl_flcache_init (cherokee_avl_flcache_t *avl)
{
	cherokee_avl_generic_t *gen = AVL_GENERIC(avl);

	cherokee_avl_generic_init (gen);
	CHEROKEE_RWLOCK_INIT (&avl->base_rwlock, NULL);

	gen->node_mrproper = (avl_gen_node_mrproper_t) node_mrproper;
	gen->node_cmp      = (avl_gen_node_cmp_t) node_cmp;
	gen->node_is_empty = (avl_gen_node_is_empty_t) node_is_empty;

	return ret_ok;
}


ret_t
cherokee_avl_flcache_mrproper (cherokee_avl_flcache_t *avl,
                               cherokee_func_free_t    free_value)
{
	CHEROKEE_RWLOCK_DESTROY (&avl->base_rwlock);
	cherokee_avl_mrproper (AVL_GENERIC(avl), free_value);
	return ret_ok;
}


ret_t
cherokee_avl_flcache_add (cherokee_avl_flcache_t       *avl,
                          cherokee_connection_t        *conn,
                          cherokee_avl_flcache_node_t **node)
{
	ret_t                       ret;
	cherokee_avl_flcache_node_t *n   = NULL;

	ret = node_new (&n, conn);
	if ((ret != ret_ok) || (n == NULL)) {
		node_free (n);
		return ret;
	}

	CHEROKEE_RWLOCK_WRITER (&avl->base_rwlock);
	ret = cherokee_avl_generic_add (AVL_GENERIC(avl), AVL_GENERIC_NODE(n), n);
	CHEROKEE_RWLOCK_UNLOCK (&avl->base_rwlock);

	if (ret != ret_ok) {
		/* n was already freed by add() */
		return ret;
	}

	*node = n;
	return ret;
}


ret_t
cherokee_avl_flcache_get (cherokee_avl_flcache_t       *avl,
                          cherokee_connection_t        *conn,
                          cherokee_avl_flcache_node_t **node)
{
	ret_t                       ret;
	cherokee_avl_flcache_node_t tmp;

	tmp.conn_ref = conn;

	CHEROKEE_RWLOCK_READER (&avl->base_rwlock);
	ret = cherokee_avl_generic_get (AVL_GENERIC(avl), AVL_GENERIC_NODE(&tmp), (void **)node);
	CHEROKEE_RWLOCK_UNLOCK (&avl->base_rwlock);

	return ret;
}


static ret_t
del_list_of_entries (cherokee_avl_flcache_t *avl,
                     cherokee_list_t        *to_delete)
{
	ret_t               ret;
	cherokee_list_t    *i, *j;
	cherokee_boolean_t  error = false;

	list_for_each_safe (i, j, to_delete) {
		cherokee_avl_flcache_node_t *node = list_entry (i, cherokee_avl_flcache_node_t, to_del);

		if (node->ref_count > 0)
			continue;

		TRACE (ENTRIES, "Removing Front-line cache file: '%s'\n", node->file.buf ? node->file.buf : "");

		/* Delete local file */
		if (! cherokee_buffer_is_empty (&node->file)) {
			cherokee_unlink (node->file.buf);
		}

		/* Delete try from the AVL tree*/
		ret = cherokee_avl_generic_del (AVL_GENERIC(avl), AVL_GENERIC_NODE(node), NULL);
		if (unlikely (ret != ret_ok)) {
			error = true;
		}
	}

	return (error) ? ret_error : ret_ok;
}


static ret_t
cleanup_while_func (cherokee_avl_generic_node_t *node_generic, void *value, void *param)
{
	cherokee_list_t             *to_delete = LIST(param);
	cherokee_avl_flcache_node_t *node      = AVL_FLCACHE_NODE(node_generic);

	UNUSED(value);

	if (node->valid_until >= cherokee_bogonow_now)
		return ret_ok;

	if (node->ref_count > 0)
		return ret_ok;

	cherokee_list_add (&node->to_del, to_delete);
	return ret_ok;
}


ret_t
cherokee_avl_flcache_cleanup (cherokee_avl_flcache_t *avl)
{
	cherokee_list_t  to_delete = LIST_HEAD_INIT(to_delete);

	CHEROKEE_RWLOCK_WRITER (&avl->base_rwlock);

	/* Add expired entries to 'to_delete'
	 */
	cherokee_avl_generic_while (AVL_GENERIC(avl), cleanup_while_func, &to_delete, NULL, NULL);

	/* Delete entries
	 */
	del_list_of_entries (avl, &to_delete);

	CHEROKEE_RWLOCK_UNLOCK (&avl->base_rwlock);

	return ret_ok;
}


ret_t
cherokee_avl_flcache_del (cherokee_avl_flcache_t      *avl,
                          cherokee_avl_flcache_node_t *node)
{
	ret_t ret = ret_ok;

	CHEROKEE_RWLOCK_WRITER (&avl->base_rwlock);
	if (node->ref_count == 0) {
		ret = cherokee_avl_generic_del (AVL_GENERIC(avl), AVL_GENERIC_NODE(node), NULL);
	}
	CHEROKEE_RWLOCK_UNLOCK (&avl->base_rwlock);

	return ret;
}


static ret_t
purge_while_func (cherokee_avl_generic_node_t *node_generic, void *value, void **params)
{
	cherokee_avl_flcache_node_t *node       = AVL_FLCACHE_NODE(node_generic);
	cherokee_buffer_t           *path       = (cherokee_buffer_t *)  (params[0]);
	cuint_t                     *purged_num = (cuint_t *)            (params[1]);
	cherokee_list_t             *to_delete  = (cherokee_list_t *)    (params[2]);

	UNUSED(value);

	/* Expire entries that match the path
	 */
	if (cherokee_buffer_cmp_buf (path, &node->request) == 0) {
		*purged_num += 1;

		/* Expire the object
		 */
		node->valid_until = 0;

		/* Whenever possible, remove it from the cache
		 */
		cherokee_list_add (&node->to_del, to_delete);
	}

	return ret_ok;
}

ret_t
cherokee_avl_flcache_purge_path (cherokee_avl_flcache_t *avl,
                                 cherokee_buffer_t      *path)
{
	ret_t            ret;
	cuint_t          purged_num = 0;
	cherokee_list_t  to_delete  = LIST_HEAD_INIT(to_delete);
	void            *params[]   = {path, &purged_num, &to_delete};

	/* FIXME: O(N) function. This is clearly sub-optimal.
	 */
	CHEROKEE_RWLOCK_WRITER (&avl->base_rwlock);

	ret = cherokee_avl_generic_while (AVL_GENERIC(avl),
	                                  (cherokee_avl_generic_while_func_t) purge_while_func,
	                                  params, NULL, NULL);

	del_list_of_entries (avl, &to_delete);
	TRACE (ENTRIES, "Purging '%s' - %d objects were expired\n", path->buf, purged_num);

	CHEROKEE_RWLOCK_UNLOCK (&avl->base_rwlock);

	if (purged_num == 0) {
		return ret_not_found;
	}

	return ret;
}
