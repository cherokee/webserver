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
#include "cache.h"
#include "util.h"

#define ENTRIES "cache"

/* must be multiple of 4 */
#define DEFAULT_MAX_SIZE 4 * 10

struct cherokee_cache_priv {
#ifdef HAVE_PTHREAD
	pthread_mutex_t mutex;
#endif
	int foo;
};

#define cache_list_add(list,entry)                                  \
	do  {                                                       \
		cherokee_list_add (LIST(entry), &cache->_ ## list); \
		cache->len_ ## list += 1;                           \
		CACHE_ENTRY(entry)->in_list = cache_ ## list;       \
	} while (false)

#define cache_list_del(list,entry)                              \
	do {                                                    \
		CACHE_ENTRY(entry)->in_list = cache_no_list;    \
		cherokee_list_del (LIST(entry));                \
		cache->len_ ## list -= 1;                       \
	} while (false)

#define cache_list_get_lru(list,ret_entry)                               \
	do {                                                             \
		if (! cherokee_list_empty (&cache->_## list)) {          \
			ret_entry = CACHE_ENTRY((cache->_## list).prev); \
		}                                                        \
	} while (false)

#define cache_list_del_lru(list,ret_entry)                               \
	do {                                                             \
		if (! cherokee_list_empty (&cache->_## list)) {          \
			ret_entry = CACHE_ENTRY((cache->_## list).prev); \
			cache_list_del (list,ret_entry);                 \
		}                                                        \
	} while (false)

#define cache_list_make_first(list,entry)                           \
	do {                                                        \
		cherokee_list_del (LIST(entry));                    \
		cherokee_list_add (LIST(entry), &cache->_ ## list); \
	} while (false)

#define cache_list_swap(from,to,entry)       \
	do {                                 \
		cache_list_del(from, entry); \
		cache_list_add(to, entry);   \
	} while (false)


/* Cache entry
 */
ret_t
cherokee_cache_entry_init (cherokee_cache_entry_t *entry,
                           cherokee_buffer_t      *key,
                           cherokee_cache_t       *cache,
                           void                   *mutex)
{
	entry->in_list   = cache_no_list;
	entry->ref_count = 0;

	entry->cache     = cache;
	entry->mutex     = mutex;

	INIT_LIST_HEAD(&entry->listed);

	cherokee_buffer_init (&entry->key);
	cherokee_buffer_add_buffer (&entry->key, key);

	return ret_ok;
}

static void
entry_free (cherokee_cache_entry_t *entry)
{
	/* Call free virtual method
	 */
	entry->free_cb (entry);

	/* Clean itself
	 */
	entry->in_list = cache_no_list;
	cherokee_buffer_mrproper (&entry->key);

	free (entry);
}

static ret_t
entry_parent_info_clean (cherokee_cache_entry_t *entry)
{
	/* entry->mutex       is LOCKED
	 */
	ret_t ret;

	if (entry->ref_count > 0)
		return ret_ok;

	ret = entry->clean_cb (entry);

	TRACE(ENTRIES, "Evincing: '%s' ret=%d\n", entry->key.buf, ret);
	return ret;
}

static ret_t
entry_parent_info_fetch (cherokee_cache_entry_t *entry)
{
	/* cache->priv->mutex is LOCKED
	 * entry->mutex       is LOCKED
	 */
	if (entry->fetch_cb == NULL)
		return ret_error;

	return entry->fetch_cb (entry);
}

static void
entry_ref (cherokee_cache_entry_t *entry)
{
	/* cache->priv->mutex is LOCKED
	 * entry->mutex       is LOCKED
	 */
	entry->ref_count++;
}


static ret_t
entry_unref_guts (cherokee_cache_entry_t **entry_p,
                  cherokee_boolean_t       lock_object)
{
	cherokee_cache_t       *cache;
	cherokee_cache_entry_t *entry;

	/* Sanity check
	 */
	if (*entry_p == NULL)
		return ret_ok;

	entry = (*entry_p);

	/* Lock the cache entry object
	 */
	if (lock_object) {
		CHEROKEE_MUTEX_LOCK (entry->mutex);
	}

	entry->ref_count--;

	/* The entry is still being used
	 */
	if (entry->ref_count > 0) {
		if (lock_object) {
			CHEROKEE_MUTEX_UNLOCK (entry->mutex);
		}

		*entry_p = NULL;
		return ret_eagain;
	}

	/* Clean cache references
	 */
	cache = (*entry_p)->cache;

	/* Is it in the AVL? */
	cherokee_avl_del (&cache->map, &entry->key, NULL);

	/* Is it listed? */
	switch (entry->in_list) {
	case cache_t1:
		cache_list_del (t1, entry);
		break;
	case cache_t2:
		cache_list_del (t2, entry);
		break;
	case cache_b1:
		cache_list_del (b1, entry);
		break;
	case cache_b2:
		cache_list_del (b2, entry);
		break;
	default:
		break;
	}

	/* Clean parent class information
	 */
	entry_parent_info_clean (entry);

	if (lock_object) {
		CHEROKEE_MUTEX_UNLOCK (entry->mutex);
	}

	entry_free (entry);
	*entry_p = NULL;

	return ret_ok;
}


ret_t
cherokee_cache_entry_unref (cherokee_cache_entry_t **entry)
{
	ret_t             ret;
	cherokee_cache_t *cache;

	if (*entry == NULL)
		return ret_ok;

	cache = (*entry)->cache;

	CHEROKEE_MUTEX_LOCK (&cache->priv->mutex);
	ret = entry_unref_guts (entry, true);
	CHEROKEE_MUTEX_UNLOCK (&cache->priv->mutex);

	return ret;
}


/* Cache
 */

ret_t
cherokee_cache_configure (cherokee_cache_t       *cache,
                          cherokee_config_node_t *conf)
{
	ret_t            ret;
	cherokee_list_t *i;

	cherokee_config_node_foreach (i, conf) {
		cherokee_config_node_t *subconf = CONFIG_NODE(i);

		if (equal_buf_str (&subconf->key, "max_size")) {
			ret = cherokee_atoi (subconf->val.buf, &cache->max_size);
			if (unlikely (ret != ret_ok)) {
				return ret_error;
			}
		}
	}

	/* max_size must be divisible by 4
	 */
	while (cache->max_size % 4 != 0)
		cache->max_size++;

	return ret_ok;
}

ret_t
cherokee_cache_init (cherokee_cache_t *cache)
{
	/* Private
	 */
	cache->priv = malloc (sizeof(cherokee_cache_priv_t));
	if (cache->priv == NULL)
		return ret_nomem;

	CHEROKEE_MUTEX_INIT (&cache->priv->mutex, CHEROKEE_MUTEX_FAST);
	CHEROKEE_MUTEX_LOCK (&cache->priv->mutex);

	/* Public
	 */
	INIT_LIST_HEAD (&cache->_t1);
	INIT_LIST_HEAD (&cache->_t2);
	INIT_LIST_HEAD (&cache->_b1);
	INIT_LIST_HEAD (&cache->_b2);

	cherokee_avl_init (&cache->map);

	cache->len_t1       = 0;
	cache->len_t2       = 0;
	cache->len_b1       = 0;
	cache->len_b2       = 0;

	cache->count        = 0;
	cache->count_hit    = 0;
	cache->count_miss   = 0;

	cache->max_size     = DEFAULT_MAX_SIZE;
	cache->target_t1    = 0;

	cache->new_cb       = NULL;
	cache->new_cb_param = NULL;

	CHEROKEE_MUTEX_UNLOCK (&cache->priv->mutex);
	return ret_ok;
}


ret_t
cherokee_cache_mrproper (cherokee_cache_t *cache)
{
	if (cache->priv) {
		CHEROKEE_MUTEX_DESTROY (&cache->priv->mutex);
		free (cache->priv);
		cache->priv = NULL;
	}

	cherokee_avl_mrproper (AVL_GENERIC(&cache->map), (cherokee_func_free_t)entry_free);
	return ret_ok;
}


static ret_t
replace (cherokee_cache_t       *cache,
         cherokee_cache_entry_t *x)
{
	/* cache->priv->mutex is LOCKED
	 * x->mutex           is LOCKED
	 */
	cint_t                  p   = cache->target_t1;
	cherokee_cache_entry_t *tmp = NULL;

	if ((cache->len_t1 >= 1) &&
	    ((cache->len_t1  > p) ||
	     (cache->len_t1 == p  && (x && x->in_list == cache_b2))))
	{
		cache_list_get_lru (t1, tmp);

		if (tmp) {
			CHEROKEE_MUTEX_LOCK (tmp->mutex);
			cache_list_del (t1, tmp);
			entry_parent_info_clean (tmp);
			cache_list_add (b1, tmp);
			CHEROKEE_MUTEX_UNLOCK (tmp->mutex);
		}
	} else {
		cache_list_get_lru (t2, tmp);

		if (tmp) {
			CHEROKEE_MUTEX_LOCK (tmp->mutex);
			cache_list_del (t2, tmp);
			entry_parent_info_clean (tmp);
			cache_list_add (b2, tmp);
			CHEROKEE_MUTEX_UNLOCK (tmp->mutex);
		}
	}

	return ret_ok;
}

static ret_t
on_new_added (cherokee_cache_t *cache)
{
	/* cache->priv->mutex is LOCKED */

	cuint_t                 c         = cache->max_size;
	cuint_t                 len_l1    = cache->len_t1 + cache->len_b1;
	cuint_t                 len_l2    = cache->len_t2 + cache->len_b2;
	cuint_t                 total_len = len_l1 + len_l2;
	cherokee_cache_entry_t *tmp       = NULL;

	cache->count_miss += 1;

	/* Free some room for the new obj that is about to be added to T1
	 */
	if (len_l1 >= c) {
		if (cache->len_t1 < c) {
			/* Remove LRU from B1 */
			cache_list_get_lru (b1, tmp);
			cache_list_del (b1, tmp);
			entry_unref_guts (&tmp, true);

			replace (cache, NULL);
		} else {
			/* Evict T1  */
			cache_list_get_lru (t1, tmp);
			cache_list_del (t1, tmp);
			entry_unref_guts (&tmp, true);
		}

	} else if ((len_l1 < c) && (total_len >= c)) {
		if (total_len >= (2 * c)) {
			/* The cache is full: Remove from B2 */
			cache_list_get_lru (b2, tmp);
			cache_list_del (b2, tmp);
			entry_unref_guts (&tmp, true);
		}
		replace (cache, NULL);
	}

	return ret_ok;
}

static ret_t
update_ghost_b1 (cherokee_cache_t       *cache,
                 cherokee_cache_entry_t *entry)
{
	/* cache->priv->mutex is LOCKED
	 * entry->mutex       is LOCKED
	 */
	ret_t ret;

	/* B1 hit: favour recency
	 */
	cache->target_t1 = MIN (cache->max_size,
	                        (cache->target_t1 + MAX (1, (cache->len_b2 / cache->len_b1))));

	/* Replace a page if needed
	 */
	replace (cache, entry);

	/* Re-fetch the information
	 */
	ret = entry_parent_info_fetch (entry);
	switch (ret) {
	case ret_ok:
	case ret_deny:
	case ret_ok_and_sent:
		break;
	case ret_error:
	case ret_no_sys:
	case ret_not_found:
		return ret;
	default:
		RET_UNKNOWN(ret);
		return ret_error;
	}

	/* Move 'entry' to the top of T2, and place it in the cache
	 */
	cache_list_swap (b1, t2, entry);
	return ret_ok;
}

static ret_t
update_ghost_b2 (cherokee_cache_t       *cache,
                 cherokee_cache_entry_t *entry)
{
	/* cache->priv->mutex is LOCKED
	 * entry->mutex       is LOCKED
	 */
	ret_t ret;

	/* Adapt the target size
	 */
	cache->target_t1 = MAX (0, (cache->target_t1 - MAX(1, (cache->len_b1 / cache->len_b2))));

	/* Replace a page if needed
	 */
	replace (cache, entry);

	/* Re-fetch the information
	 */
	ret = entry_parent_info_fetch (entry);
	switch (ret) {
	case ret_ok:
	case ret_deny:
		break;
	case ret_error:
	case ret_no_sys:
		return ret;
	default:
		RET_UNKNOWN(ret);
		return ret_error;
	}

	/* Move 'entry' to the top of T2, and place it in the cache
	 */
	cache_list_swap (b2, t2, entry);
	return ret_ok;
}

static ret_t
update_cache (cherokee_cache_t       *cache,
              cherokee_cache_entry_t *entry)
{
	/* cache->priv->mutex is LOCKED
	 * entry->mutex       is LOCKED
	 */
	ret_t ret;

	cache->count_hit += 1;

	switch (entry->in_list) {
	case cache_t1:
		/* LRU (Least Recently Used) cache hit
		 */
		TRACE(ENTRIES, "T1 hit: '%s'\n", entry->key.buf);

		cache_list_swap (t1, t2, entry);
		break;

	case cache_t2:
		/* LFU (Least Frequently Used)
		 */
		TRACE(ENTRIES, "T2 hit: '%s'\n", entry->key.buf);

		cache_list_make_first (t2, entry);
		break;

	case cache_b1:
		/* Ghost 'Recently' hit
		 */
		TRACE(ENTRIES, "B1 ghost hit: '%s' (refs=%d)\n",
		      entry->key.buf, entry->ref_count);

		ret = update_ghost_b1 (cache, entry);
		switch (ret) {
		case ret_ok:
		case ret_no_sys:
			return ret;
		default:
			return ret_error;
		}
		break;

	case cache_b2:
		/* Ghost 'Frequently' hit
		 */
		TRACE(ENTRIES, "B2 ghost hit: '%s' (refs=%d)\n",
		      entry->key.buf, entry->ref_count);

		ret = update_ghost_b2 (cache, entry);
		switch (ret) {
		case ret_ok:
		case ret_no_sys:
			return ret;
		default:
			return ret_error;
		}
		break;

	default:
		return ret_error;
	}

	return ret_ok;
}


ret_t
cherokee_cache_get (cherokee_cache_t        *cache,
                    cherokee_buffer_t       *key,
                    cherokee_cache_entry_t **ret_entry)
{
	ret_t ret;

	CHEROKEE_MUTEX_LOCK (&cache->priv->mutex);
	cache->count += 1;

#ifdef TRACE_ENABLED
	if (cache->count % 100 == 0) {
		cherokee_buffer_t tmp = CHEROKEE_BUF_INIT;

		cherokee_cache_get_stats (cache, &tmp);
		TRACE(ENTRIES, "Cache stats:\n%s", tmp.buf);
		cherokee_buffer_mrproper (&tmp);
	}
#endif

	/* Find inside the cache
	 */
	ret = cherokee_avl_get (&cache->map, key, (void **)ret_entry);
	switch (ret) {
	case ret_ok:
		CHEROKEE_MUTEX_LOCK ((*ret_entry)->mutex);

		/* Lingering object: evinced, but already in the AVL
		 */
		if ((*ret_entry)->in_list == cache_no_list) {
			TRACE(ENTRIES, "Found in AVL, not listed: '%s'\n", key->buf);
			entry_ref (*ret_entry);
			goto add_list;
		}

		/* It's in the cache; update it
		 */
		ret = update_cache (cache, *ret_entry);
		entry_ref (*ret_entry);

		CHEROKEE_MUTEX_UNLOCK ((*ret_entry)->mutex);
		goto out;

	case ret_not_found:
		break;

	default:
		SHOULDNT_HAPPEN;
		ret = ret_error;
		goto out;
	}

	/* Might need to free some room for the new page
	 */
	on_new_added (cache);

	/* Instance new page and add it to the AVL
	 */
	cache->new_cb (cache, key, cache->new_cb_param, ret_entry);
	if (*ret_entry == NULL) {
		SHOULDNT_HAPPEN;
		CHEROKEE_MUTEX_UNLOCK (&cache->priv->mutex);
		return ret_error;
	}

	TRACE(ENTRIES, "Miss (adding): '%s'\n", key->buf);
	CHEROKEE_MUTEX_LOCK ((*ret_entry)->mutex);

	cherokee_avl_add (&cache->map, key, *ret_entry);
	entry_ref (*ret_entry); /* cache */

	/* Add the new object to T1
	 */
add_list:
	cache_list_add (t1, *ret_entry);
	entry_ref (*ret_entry); /* client */

	CHEROKEE_MUTEX_UNLOCK ((*ret_entry)->mutex);
	ret = ret_ok;
out:
	CHEROKEE_MUTEX_UNLOCK (&cache->priv->mutex);
	return ret;
}


ret_t
cherokee_cache_get_stats (cherokee_cache_t  *cache,
                          cherokee_buffer_t *info)
{
	size_t len  = 0;
	float  rate = 0;

	cherokee_list_get_len (&cache->_t1, &len);
	cherokee_buffer_add_va (info, "T1 size: %d (real=%d)\n", cache->len_t1, len);

	cherokee_list_get_len (&cache->_b1, &len);
	cherokee_buffer_add_va (info, "B1 size: %d (real=%d)\n", cache->len_b1, len);

	cherokee_list_get_len (&cache->_t2, &len);
	cherokee_buffer_add_va (info, "T2 size: %d (real=%d)\n", cache->len_t2, len);

	cherokee_list_get_len (&cache->_b2, &len);
	cherokee_buffer_add_va (info, "B2 size: %d (real=%d)\n", cache->len_b2, len);

	cherokee_buffer_add_va (info, "Max size: %d\n", cache->max_size);
	cherokee_buffer_add_va (info, "Target T1 size: %d\n", cache->target_t1);

	cherokee_avl_len (AVL_GENERIC(&cache->map), &len);
	cherokee_buffer_add_va (info, "AVL size: %d\n", len);

	cherokee_buffer_add_va (info, "Total count: %d\n", cache->count);
	cherokee_buffer_add_va (info, "Hit count: %d\n", cache->count_hit);
	cherokee_buffer_add_va (info, "Miss count: %d\n", cache->count_miss);

	if (cache->count_hit + cache->count_miss > 0) {
		rate = (float) ((cache->count_hit * 100) /
				(cache->count_hit + cache->count_miss));
	}
	cherokee_buffer_add_va (info, "Hit Rate: %.2f%%\n", rate);

	if (cache->stats_cb != NULL) {
		cache->stats_cb (cache, info);
	}

	return ret_ok;
}


