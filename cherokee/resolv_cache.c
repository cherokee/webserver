/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/* Cherokee
 *
 * Authors:
 *      Alvaro Lopez Ortega <alvaro@alobbs.com>
 *
 * Copyright (C) 2001-2011 Alvaro Lopez Ortega
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

#ifndef _WIN32
# include <sys/socket.h>
# include <netinet/in.h>
# include <arpa/inet.h>
# include <netdb.h>
#endif

#include "resolv_cache.h"
#include "util.h"
#include "avl.h"
#include "socket.h"
#include "bogotime.h"

#define ENTRIES "resolve"


typedef struct {
	struct in_addr    addr;
	cherokee_buffer_t ip_str;
} cherokee_resolv_cache_entry_t;

struct cherokee_resolv_cache {
	cherokee_avl_t     table;
	CHEROKEE_RWLOCK_T (lock);
};

static cherokee_resolv_cache_t *__global_resolv = NULL;


/* Entries
 */
static ret_t
entry_new (cherokee_resolv_cache_entry_t **entry)
{
	CHEROKEE_NEW_STRUCT(n, resolv_cache_entry);

	cherokee_buffer_init (&n->ip_str);
	memset (&n->addr, 0, sizeof(n->addr));

	*entry = n;
	return ret_ok;
}


static void
entry_free (void *entry)
{
	cherokee_resolv_cache_entry_t *e = entry;

	cherokee_buffer_mrproper (&e->ip_str);
	free(entry);
}


static ret_t
entry_fill_up (cherokee_resolv_cache_entry_t *entry,
	       cherokee_buffer_t             *domain)
{
	ret_t   ret;
	char   *tmp;
	time_t  eagain_at = 0;

	while (true) {
		ret = cherokee_gethostbyname (domain->buf, &entry->addr);
		if (ret == ret_ok) {
			break;

		} else if (ret == ret_eagain) {
			if (eagain_at == 0) {
				eagain_at = cherokee_bogonow_now;

			} else if (cherokee_bogonow_now > eagain_at + 3) {
			      	LOG_WARNING (CHEROKEE_ERROR_RESOLVE_TIMEOUT, domain->buf);
				return ret_error;
			}

			CHEROKEE_THREAD_YIELD;
			continue;

		} else {
			return ret_error;
		}
	}

	tmp = inet_ntoa (entry->addr);
	if (tmp == NULL) {
		return ret_error;
	}

	cherokee_buffer_add (&entry->ip_str, tmp, strlen(tmp));
	return ret_ok;
}



/* Table
 */
ret_t
cherokee_resolv_cache_init (cherokee_resolv_cache_t *resolv)
{
	ret_t ret;

	ret = cherokee_avl_init (&resolv->table);
	if (unlikely (ret != ret_ok)) return ret;

	ret = cherokee_avl_set_case (&resolv->table, true);
	if (unlikely (ret != ret_ok)) return ret;

	CHEROKEE_RWLOCK_INIT (&resolv->lock, NULL);
	return ret_ok;
}


ret_t
cherokee_resolv_cache_mrproper (cherokee_resolv_cache_t *resolv)
{
	cherokee_avl_mrproper (&resolv->table, entry_free);
	CHEROKEE_RWLOCK_DESTROY (&resolv->lock);

	return ret_ok;
}


ret_t
cherokee_resolv_cache_get_default (cherokee_resolv_cache_t **resolv)
{
	ret_t ret;

	if (unlikely (__global_resolv == NULL)) {
		CHEROKEE_NEW_STRUCT (n, resolv_cache);

		ret = cherokee_resolv_cache_init (n);
		if (ret != ret_ok) return ret;

		__global_resolv = n;
	}

	*resolv = __global_resolv;
	return ret_ok;
}


ret_t
cherokee_resolv_cache_clean (cherokee_resolv_cache_t *resolv)
{
	CHEROKEE_RWLOCK_WRITER (&resolv->lock);
	cherokee_avl_mrproper (&resolv->table, entry_free);
	CHEROKEE_RWLOCK_UNLOCK (&resolv->lock);

	return ret_ok;
}


static ret_t
table_add_new_entry (cherokee_resolv_cache_t        *resolv,
		     cherokee_buffer_t              *domain,
		     cherokee_resolv_cache_entry_t **entry)
{
	ret_t                          ret;
	cherokee_resolv_cache_entry_t *n    = NULL;

	/* Instance the entry
	 */
	ret = entry_new (&n);
	if (unlikely (ret != ret_ok)) {
		return ret;
	}

	/* Fill it up
	 */
	ret = entry_fill_up (n, domain);
	if (unlikely (ret != ret_ok)) {
		entry_free (n);
		return ret;
	}

	/* Add it to the table
	 */
	CHEROKEE_RWLOCK_WRITER (&resolv->lock);
	ret = cherokee_avl_add (&resolv->table, domain, (void **)n);
	CHEROKEE_RWLOCK_UNLOCK (&resolv->lock);

	*entry = n;
	return ret_ok;
}


ret_t
cherokee_resolv_cache_get_ipstr (cherokee_resolv_cache_t  *resolv,
				 cherokee_buffer_t        *domain,
				 const char              **ip)
{
	ret_t                          ret;
	cherokee_resolv_cache_entry_t *entry = NULL;

	/* Look for the name in the cache
	 */
	CHEROKEE_RWLOCK_READER (&resolv->lock);
	ret = cherokee_avl_get (&resolv->table, domain, (void **)&entry);
	CHEROKEE_RWLOCK_UNLOCK (&resolv->lock);

	if (ret != ret_ok) {
		TRACE (ENTRIES, "Resolve '%s': missed.\n", domain->buf);

		/* Bad luck: it wasn't cached
		 */
		ret = table_add_new_entry (resolv, domain, &entry);
		if (ret != ret_ok) {
			return ret;
		}
	} else {
		TRACE (ENTRIES, "Resolve '%s': hit.\n", domain->buf);
	}

	/* Return the ip string
	 */
	if (ip != NULL) {
		*ip = entry->ip_str.buf;
	}

	return ret_ok;
}


ret_t
cherokee_resolv_cache_get_host (cherokee_resolv_cache_t *resolv,
				cherokee_buffer_t       *domain,
				void                    *sock_)
{
	ret_t                          ret;
	cherokee_socket_t             *sock  = sock_;
	cherokee_resolv_cache_entry_t *entry = NULL;

	/* Look for the name in the cache
	 */
	CHEROKEE_RWLOCK_READER (&resolv->lock);
	ret = cherokee_avl_get (&resolv->table, domain, (void **)&entry);
	CHEROKEE_RWLOCK_UNLOCK (&resolv->lock);

	if (ret != ret_ok) {
		TRACE (ENTRIES, "Resolve '%s': missed.\n", domain->buf);

		/* Bad luck: it wasn't cached
		 */
		ret = table_add_new_entry (resolv, domain, &entry);
		if (ret != ret_ok) {
			return ret;
		}
	} else {
		TRACE (ENTRIES, "Resolve '%s': hit.\n", domain->buf);
	}

	/* Copy the address
	 */
	memcpy (&SOCKET_SIN_ADDR(sock), &entry->addr, sizeof(entry->addr));
	return ret_ok;
}
