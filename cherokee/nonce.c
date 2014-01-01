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
#include "nonce.h"
#include "avl.h"
#include "connection-protected.h"
#include "server-protected.h"
#include "bogotime.h"

struct cherokee_nonce_table {
	cherokee_avl_t    table;
	cherokee_list_t   list;
	CHEROKEE_MUTEX_T (access);
};

typedef struct {
	cherokee_list_t   listed;
	cherokee_buffer_t nonce;
	time_t            accessed;
} entry_t;


static entry_t *
entry_new (void)
{
	entry_t *entry;

	entry = (entry_t *) malloc(sizeof(entry_t));
	if (unlikely (entry == NULL))
		return NULL;

	entry->accessed = cherokee_bogonow_now;
	INIT_LIST_HEAD (&entry->listed);
	cherokee_buffer_init (&entry->nonce);

	return entry;
}

static void
entry_free (cherokee_nonce_table_t *nonces,
	    entry_t                *entry)
{
	cherokee_list_del (&entry->listed);
	cherokee_avl_del (&nonces->table, &entry->nonce, NULL);
	cherokee_buffer_mrproper (&entry->nonce);
	free (entry);
}


ret_t
cherokee_nonce_table_new (cherokee_nonce_table_t **nonces)
{
	CHEROKEE_NEW_STRUCT(n, nonce_table);

	cherokee_avl_init (&n->table);
	CHEROKEE_MUTEX_INIT (&n->access, NULL);
	INIT_LIST_HEAD (&n->list);

	*nonces = n;
	return ret_ok;
}


ret_t
cherokee_nonce_table_free (cherokee_nonce_table_t *nonces)
{
	cherokee_list_t *i, *j;

	list_for_each_safe (i, j, &nonces->list) {
		entry_free (nonces, (entry_t *)i);
	}

	cherokee_avl_mrproper (AVL_GENERIC(&nonces->table), NULL);
	CHEROKEE_MUTEX_DESTROY (&nonces->access);

	free (nonces);
	return ret_ok;
}


ret_t
cherokee_nonce_table_remove (cherokee_nonce_table_t *nonces,
                             cherokee_buffer_t      *nonce)
{
	ret_t    ret;
	entry_t *entry = NULL;

	CHEROKEE_MUTEX_LOCK (&nonces->access);
	ret = cherokee_avl_get (&nonces->table, nonce, (void **)&entry);
	if (ret == ret_ok) {
		entry_free (nonces, entry);
	}
	CHEROKEE_MUTEX_UNLOCK (&nonces->access);

	if (ret != ret_ok) return ret_not_found;
	return ret_ok;
}


ret_t
cherokee_nonce_table_check (cherokee_nonce_table_t *nonces,
                            cherokee_buffer_t      *nonce)
{
	ret_t    ret;
	entry_t *entry = NULL;

	if (cherokee_buffer_is_empty (nonce))
		return ret_not_found;

	CHEROKEE_MUTEX_LOCK (&nonces->access);
	ret = cherokee_avl_get (&nonces->table, nonce, (void **)&entry);
	CHEROKEE_MUTEX_UNLOCK (&nonces->access);

	if ((ret != ret_ok) || (entry == NULL))
		return ret_not_found;

	entry->accessed = cherokee_bogonow_now;
	return ret_ok;
}


ret_t
cherokee_nonce_table_generate (cherokee_nonce_table_t *nonces,
                               cherokee_connection_t  *conn,
                               cherokee_buffer_t      *nonce)
{
	entry_t *entry;

	/* Generate nonce string
	 */
	cherokee_buffer_add_ullong16(nonce, (cullong_t) cherokee_bogonow_now);
	cherokee_buffer_add_ulong16 (nonce, (culong_t) rand());
	cherokee_buffer_add_ulong16 (nonce, (culong_t) POINTER_TO_INT(conn));

	/* Compute MD5 and overwrite buffer content without reallocating it !
	 */
	cherokee_buffer_encode_md5_digest (nonce);

	/* Copy the nonce and add to the table
	 */
	entry = entry_new();
	if (unlikely (entry == NULL))
		return ret_nomem;

	cherokee_buffer_add_buffer (&entry->nonce, nonce);

	CHEROKEE_MUTEX_LOCK (&nonces->access);
	cherokee_avl_add (&nonces->table, nonce, entry);
	cherokee_list_add (&entry->listed, &nonces->list);
	CHEROKEE_MUTEX_UNLOCK (&nonces->access);

	/* Return
	 */
	return ret_ok;
}


ret_t
cherokee_nonce_table_cleanup (cherokee_nonce_table_t *nonces)
{
	cherokee_list_t *i, *j;
	entry_t         *entry;

	CHEROKEE_MUTEX_LOCK (&nonces->access);
	list_for_each_safe (i, j, &nonces->list) {
		entry = (entry_t *)i;
		if (entry->accessed + NONCE_EXPIRATION < cherokee_bogonow_now) {
			entry_free (nonces, entry);
		}
	}
	CHEROKEE_MUTEX_UNLOCK (&nonces->access);

	return ret_ok;
}
