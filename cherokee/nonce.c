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

#include "common-internal.h"
#include "nonce.h"
#include "connection-protected.h"
#include "server-protected.h"

struct cherokee_nonce_table {
	cherokee_table_t table;

#ifdef HAVE_PTHREAD
	pthread_mutex_t access;
#endif
};


ret_t 
cherokee_nonce_table_new (cherokee_nonce_table_t **nonces)
{
	CHEROKEE_NEW_STRUCT(n, nonce_table);

	cherokee_table_init (TABLE(n));
	CHEROKEE_MUTEX_INIT (&n->access, NULL);

	*nonces = n;
	return ret_ok;
}


ret_t 
cherokee_nonce_table_free (cherokee_nonce_table_t *nonces)
{
	CHEROKEE_MUTEX_DESTROY (&nonces->access);
	cherokee_table_free2 (TABLE(nonces), free);

	return ret_ok;
}


ret_t 
cherokee_nonce_table_remove (cherokee_nonce_table_t *nonces, cherokee_buffer_t *nonce)
{
	ret_t  ret;
	void  *non = NULL;

	CHEROKEE_MUTEX_LOCK (&nonces->access);
	ret = cherokee_table_get (TABLE(nonces), nonce->buf, &non);
	if (ret == ret_ok) {
		cherokee_table_del (TABLE(nonces), nonce->buf, NULL);
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
	ret = cherokee_table_get (TABLE(nonces), nonce->buf, &non);
	CHEROKEE_MUTEX_UNLOCK (&nonces->access);

	if (ret != ret_ok) return ret_not_found;

	return ret_ok;
}


ret_t 
cherokee_nonce_table_generate (cherokee_nonce_table_t *nonces, cherokee_connection_t *conn, cherokee_buffer_t *nonce)
{
	cherokee_buffer_t crc = CHEROKEE_BUF_INIT;

	/* Generate nonce string
	 */
	cherokee_buffer_add_va (&crc, "%x", POINTER_TO_INT(conn));
	cherokee_buffer_crc32 (&crc);

	cherokee_buffer_add_va (nonce, "%x%x%s", CONN_SRV(conn)->bogo_now, rand(), crc.buf);
	cherokee_buffer_encode_md5_digest (nonce);

	/* Copy the nonce and add to the table
	 */
	CHEROKEE_MUTEX_LOCK (&nonces->access);
	cherokee_table_add (TABLE(nonces), nonce->buf, NULL);
	CHEROKEE_MUTEX_UNLOCK (&nonces->access);

	/* Clean up
	 */
	cherokee_buffer_mrproper (&crc);
	return ret_ok;
}

