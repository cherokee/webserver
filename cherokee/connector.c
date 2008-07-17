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
#include "connector.h"
#include "socket.h"

typedef struct {
	   CHEROKEE_MUTEX_T(lock);
	   int foo;
} cherokee_priv_t;

#define PRIV(contor) ((cherokee_priv_t *)(contor)->priv)
#define LOCK(contor) (&PRIV(contor)->lock)

static cherokee_connector_t *global_contor = NULL;

ret_t
cherokee_connector_init (cherokee_connector_t *contor,
			 cuint_t               max)
{
	CHEROKEE_NEW_STRUCT(n, priv);

	contor->connects = 0;
	contor->max      = max;
	contor->priv     = n;

	CHEROKEE_MUTEX_INIT (LOCK(contor), NULL);	
	return ret_ok;
}

ret_t
cherokee_connector_get_default (cherokee_connector_t **contor)
{
	if (unlikely (global_contor == NULL))
		return ret_error;

	*contor = global_contor;
	return ret_ok;
}

ret_t
cherokee_connector_set_default (cherokee_connector_t *contor)
{
	global_contor = contor;
	return ret_ok;
}


ret_t
cherokee_connector_mrproper (cherokee_connector_t *contor)
{
	if (contor->priv) {
		CHEROKEE_MUTEX_DESTROY (LOCK(contor));
		free (contor->priv);
		contor->priv = NULL;
	}
	
	return ret_ok;
}


ret_t
cherokee_connector_connect (cherokee_connector_t *contor,
			    void                 *socket)
{
	ret_t  ret;

	/* Check whether it has reached the limit
	 */
	CHEROKEE_MUTEX_LOCK (LOCK(contor));
	if (contor->connects > contor->max) {
		CHEROKEE_MUTEX_UNLOCK (LOCK(contor));
		return ret_deny;
	}
	contor->connects += 1;
	CHEROKEE_MUTEX_UNLOCK (LOCK(contor));

	/* Connect the socket
	 */
	ret = cherokee_socket_connect (SOCKET(socket));

	/* Update the 'connecting' counter
	 */
	CHEROKEE_MUTEX_LOCK (LOCK(contor));
	contor->connects -= 1;
	CHEROKEE_MUTEX_UNLOCK (LOCK(contor));

	return ret;
}
