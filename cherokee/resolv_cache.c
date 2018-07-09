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
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include "resolv_cache.h"
#include "socket_lowlevel.h"
#include "util.h"
#include "avl.h"
#include "socket.h"
#include "bogotime.h"


#define ENTRIES "resolve"


typedef struct {
	struct addrinfo    *addr;
	cherokee_buffer_t  ip_str;
	cherokee_buffer_t  ip_str_all;
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

	n->addr = NULL;
	cherokee_buffer_init (&n->ip_str);
	cherokee_buffer_init (&n->ip_str_all);

	*entry = n;
	return ret_ok;
}


static void
entry_free (void *entry)
{
	cherokee_resolv_cache_entry_t *e = entry;

	if (e->addr) {
		freeaddrinfo (e->addr);
	}

	cherokee_buffer_mrproper (&e->ip_str);
	cherokee_buffer_mrproper (&e->ip_str_all);
	free(entry);
}


static ret_t
entry_fill_up (cherokee_resolv_cache_entry_t *entry,
               cherokee_buffer_t             *domain)
{
	ret_t            ret;
	char             tmp[46];       // Max IPv6 length is 45
	struct addrinfo *addr;
	time_t           eagain_at = 0;

	while (true) {
		ret = cherokee_gethostbyname (domain, &entry->addr);
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

	if (unlikely (entry->addr == NULL)) {
		return ret_error;
	}

	/* Render the text representation
	 */
	ret = cherokee_ntop (entry->addr->ai_family, entry->addr->ai_addr, tmp, sizeof(tmp));
	if (ret != ret_ok) {
		return ret_error;
	}

	cherokee_buffer_add (&entry->ip_str, tmp, strlen(tmp));

	/* Render the text representation (all the IPs)
	 */
	cherokee_buffer_add_buffer (&entry->ip_str_all, &entry->ip_str);

	addr = entry->addr->ai_next;
	while (addr != NULL) {
		ret = cherokee_ntop (addr->ai_family, addr->ai_addr, tmp, sizeof(tmp));
		if (ret != ret_ok) {
			return ret_error;
		}

		cherokee_buffer_add_char (&entry->ip_str_all, ',');
		cherokee_buffer_add      (&entry->ip_str_all, tmp, strlen(tmp));

		addr = addr->ai_next;
	}

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
	cherokee_avl_mrproper (AVL_GENERIC(&resolv->table), entry_free);
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
	cherokee_avl_mrproper (AVL_GENERIC(&resolv->table), entry_free);
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
		TRACE (ENTRIES, "Resolve '%s': added succesfuly as '%s'.\n", domain->buf, entry->ip_str_all.buf);
	} else {
		TRACE (ENTRIES, "Resolve '%s': hit: %s\n", domain->buf, entry->ip_str_all.buf);
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
	ret_t                  ret;
	const struct addrinfo *addr = NULL;
	cherokee_socket_t     *sock = sock_;

	/* Get addrinfo
	 */
	ret = cherokee_resolv_cache_get_addrinfo (resolv, domain, &addr);
	if (ret != ret_ok) {
		return ret;
	}

	/* Copy it to the socket object
	 */
	ret = cherokee_socket_update_from_addrinfo (sock, addr, 0);
	if (ret != ret_ok) {
		return ret;
	}

	return ret_ok;
}


ret_t
cherokee_resolv_cache_get_addrinfo (cherokee_resolv_cache_t *resolv,
                                    cherokee_buffer_t       *domain,
                                    const struct addrinfo  **addr_info)
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
			TRACE (ENTRIES, "Resolve '%s': error ret=%d.\n", domain->buf, ret);
			return ret;
		}
		TRACE (ENTRIES, "Resolve '%s': added succesfuly as '%s'.\n", domain->buf, entry->ip_str.buf);
	} else {
		TRACE (ENTRIES, "Resolve '%s': hit.\n", domain->buf);
	}

	*addr_info = entry->addr;
	return ret_ok;
}
