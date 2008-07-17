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

#ifndef CHEROKEE_CONNECTOR_H
#define CHEROKEE_CONNECTOR_H

#include <cherokee/common.h>

typedef struct {
	cuint_t  connects;
	cuint_t  max;
	void    *priv;
} cherokee_connector_t;

#define CONNECTOR(c) ((cherokee_connector_t *)(c))


/* Constructor & Destructor
 */
ret_t cherokee_connector_init        (cherokee_connector_t  *contor,
				      cuint_t                max);
ret_t cherokee_connector_mrproper    (cherokee_connector_t  *contor);

/* Kind of factory
 */
ret_t cherokee_connector_set_default (cherokee_connector_t  *contor);
ret_t cherokee_connector_get_default (cherokee_connector_t **contor);

/* Methods
 */
ret_t cherokee_connector_connect     (cherokee_connector_t *contor,
				      void                 *socket);

#endif /* CHEROKEE_CACHE_H */
