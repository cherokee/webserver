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

#ifndef _WIN32
# include <sys/socket.h>
# include <netinet/in.h>
# include <arpa/inet.h>
# include <netdb.h>
#endif

#include "resolv_cache.h"
#include "util.h"
#include "table.h"
#include "table-protected.h"


struct cherokee_resolv_cache {
	   cherokee_table_t table;

#ifdef HAVE_PTHREAD
	   pthread_rwlock_t lock;
#endif
};


static cherokee_resolv_cache_t *__global_resolv = NULL;


ret_t 
cherokee_resolv_cache_init (cherokee_resolv_cache_t *resolv)
{
	   ret_t ret;

	   ret = cherokee_table_init (&resolv->table);
	   if (unlikely (ret != ret_ok)) return ret;

	   CHEROKEE_RWLOCK_INIT (&resolv->lock, NULL);

	   return ret_ok;
}


ret_t 
cherokee_resolv_cache_mrproper (cherokee_resolv_cache_t *resolv)
{
	   cherokee_table_mrproper (&resolv->table);
	   CHEROKEE_RWLOCK_DESTROY (&resolv->lock);

	   return ret_ok;
}


ret_t 
cherokee_resolv_cache_get_default (cherokee_resolv_cache_t **resolv)
{
	if (__global_resolv != NULL) {
		*resolv = __global_resolv;
		return ret_ok;
	}
	
	*resolv = (cherokee_resolv_cache_t *) malloc (sizeof(cherokee_resolv_cache_t));
	return cherokee_resolv_cache_init (*resolv);
}


static ret_t
resolve (const char *domain, const char **ip)
{
	ret_t           ret;
	char           *tmp;
	struct in_addr  addr;
	
	ret = cherokee_gethostbyname (domain, &addr); 
	if (unlikely (ret != ret_ok)) return ret;

	tmp = inet_ntoa (addr);
	*ip = strdup (tmp);

	return ret_ok;
}


ret_t 
cherokee_resolv_cache_resolve (cherokee_resolv_cache_t *resolv, const char *domain, const char **ip)
{
	   ret_t ret;

	   /* Look for the name in the cache
	    */
	   CHEROKEE_RWLOCK_WRITER (&resolv->lock);
	   ret = cherokee_table_get (&resolv->table, (char *)domain, (void **)ip);
	   CHEROKEE_RWLOCK_UNLOCK (&resolv->lock);
	   if (ret == ret_ok) return ret_ok;

	   /* Damn it! It isn't there..
	    */
	   ret = resolve (domain, ip);
	   if (ret != ret_ok) return ret;

	   CHEROKEE_RWLOCK_WRITER (&resolv->lock);
	   ret = cherokee_table_add (&resolv->table, (char *)domain, (void **)*ip);
	   CHEROKEE_RWLOCK_UNLOCK (&resolv->lock);

	   return ret;
}


ret_t 
cherokee_resolv_cache_clean (cherokee_resolv_cache_t *resolv)
{
	   CHEROKEE_RWLOCK_WRITER (&resolv->lock);
	   cherokee_table_clean2 (&resolv->table, free);
	   CHEROKEE_RWLOCK_UNLOCK (&resolv->lock);

	   return ret_ok;
}
