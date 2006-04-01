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

#ifndef CHEROKEE_SESSION_CACHE_H
#define CHEROKEE_SESSION_CACHE_H

#include "avl/avl.h"
#include "common.h"


typedef struct {
	struct avl_table *tree;
} cherokee_session_cache_t;

#define SESSION_CACHE(x) ((cherokee_session_cache_t *)(x))


ret_t cherokee_session_cache_new  (cherokee_session_cache_t **tab);
ret_t cherokee_session_cache_free (cherokee_session_cache_t  *tab);

ret_t cherokee_session_cache_del      (cherokee_session_cache_t *tab, unsigned char *key, int key_len);
ret_t cherokee_session_cache_add      (cherokee_session_cache_t *tab, unsigned char *key, unsigned int key_len, unsigned char *value, unsigned int value_len);
ret_t cherokee_session_cache_retrieve (cherokee_session_cache_t *tab, unsigned char *key, int key_len, void **buf, unsigned int *but_len);

#endif /* CHEROKEE_SESSION_CACHE_H */
