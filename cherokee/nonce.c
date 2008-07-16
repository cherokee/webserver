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

#include "common-internal.h"
#include "nonce.h"
#include "avl.h"
#include "connection-protected.h"
#include "server-protected.h"
#include "bogotime.h"

struct cherokee_nonce_table {
	cherokee_avl_t    table;
	CHEROKEE_MUTEX_T (access);
};


ret_t 
cherokee_nonce_table_new (cherokee_nonce_table_t **nonces)
{
	CHEROKEE_NEW_STRUCT(n, nonce_table);

	cherokee_avl_init (&n->table);
	CHEROKEE_MUTEX_INIT (&n->access, NULL);

	*nonces = n;
	return ret_ok;
}


ret_t 
cherokee_nonce_table_free (cherokee_nonce_table_t *nonces)
{
	CHEROKEE_MUTEX_DESTROY (&nonces->access);
	cherokee_avl_free (&nonces->table, free);

	return ret_ok;
}


ret_t 
cherokee_nonce_table_remove (cherokee_nonce_table_t *nonces, cherokee_buffer_t *nonce)
{
	ret_t  ret;
	void  *non = NULL;

	CHEROKEE_MUTEX_LOCK (&nonces->access);
	ret = cherokee_avl_get (&nonces->table, nonce, &non);
	if (ret == ret_ok) {
		cherokee_avl_del (&nonces->table, nonce, NULL);
	}
	CHEROKEE_MUTEX_UNLOCK (&nonces->access);

	if (ret != ret_ok) return ret_not_found;
	return ret_ok;
}


ret_t 
cherokee_nonce_table_check (cherokee_nonce_table_t *nonces, cherokee_buffer_t *nonce)
{
	ret_t  ret;
	void  *non = NULL;

	CHEROKEE_MUTEX_LOCK (&nonces->access);
	ret = cherokee_avl_get (&nonces->table, nonce, &non);
	CHEROKEE_MUTEX_UNLOCK (&nonces->access);

	if (ret != ret_ok) return ret_not_found;

	return ret_ok;
}


ret_t 
cherokee_nonce_table_generate (cherokee_nonce_table_t *nonces, cherokee_connection_t *conn, cherokee_buffer_t *nonce)
{
	/* Generate nonce string
	 */
	cherokee_buffer_clean (nonce);
	cherokee_buffer_add_ullong16(nonce, (cullong_t) cherokee_bogonow_now);
	cherokee_buffer_add_ulong16 (nonce, (culong_t) rand());
	cherokee_buffer_add_ulong16 (nonce, (culong_t) POINTER_TO_INT(conn));

	/* Compute MD5 and overwrite buffer content without reallocating it !
	 */
	cherokee_buffer_encode_md5_digest (nonce);

	/* Copy the nonce and add to the table
	 */
	CHEROKEE_MUTEX_LOCK (&nonces->access);
	cherokee_avl_add (&nonces->table, nonce, NULL);
	CHEROKEE_MUTEX_UNLOCK (&nonces->access);

	/* Return
	 */
	return ret_ok;
}

