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

#ifndef CHEROKEE_RESOLV_CACHE_H
#define CHEROKEE_RESOLV_CACHE_H

#include <cherokee/common.h>


CHEROKEE_BEGIN_DECLS

typedef struct cherokee_resolv_cache cherokee_resolv_cache_t;
#define RESOLV(x) ((cherokee_resolv_cache_t *)(x))


ret_t cherokee_resolv_cache_get_default (cherokee_resolv_cache_t **resolv);

ret_t cherokee_resolv_cache_init      (cherokee_resolv_cache_t *resolv);
ret_t cherokee_resolv_cache_mrproper  (cherokee_resolv_cache_t *resolv);
ret_t cherokee_resolv_cache_clean     (cherokee_resolv_cache_t *resolv);

ret_t cherokee_resolv_cache_get_ipstr (cherokee_resolv_cache_t *resolv,  const char *domain, const char **ip);
ret_t cherokee_resolv_cache_get_host  (cherokee_resolv_cache_t *resolv,  const char *domain, void *sock);

CHEROKEE_END_DECLS

#endif /* CHEROKEE_RESOLV_CACHE_H */
