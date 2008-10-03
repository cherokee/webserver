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

#ifndef CHEROKEE_CACHE_H
#define CHEROKEE_CACHE_H

#include <cherokee/common.h>
#include <cherokee/list.h>
#include <cherokee/avl.h>

/* Forward declaration */
typedef struct cherokee_cache       cherokee_cache_t;
typedef struct cherokee_cache_priv  cherokee_cache_priv_t;
typedef struct cherokee_cache_entry cherokee_cache_entry_t;

/* Callback prototypes */
typedef ret_t (* cherokee_cache_new_func_t)  (struct cherokee_cache        *cache,
					      cherokee_buffer_t            *key,
					      void                         *param,
					      struct cherokee_cache_entry **ret);

typedef ret_t (* cherokee_cache_get_stats_t) (struct cherokee_cache        *cache,
					      cherokee_buffer_t            *key);

typedef ret_t (* cherokee_cache_entry_clean_t) (struct cherokee_cache_entry *entry);
typedef ret_t (* cherokee_cache_entry_fetch_t) (struct cherokee_cache_entry *entry);
typedef ret_t (* cherokee_cache_entry_free_t)  (struct cherokee_cache_entry *entry);

/* Enums */
typedef enum {
	cache_no_list = 0,
	cache_t1,
	cache_t2,
	cache_b1,
	cache_b2
} cherokee_cache_list_t;

/* Classes */
struct cherokee_cache {
	/* Lookup table */
	cherokee_avl_t  map;

	/* LRU (Least Recently Used)   */
	cherokee_list_t _t1; 
	cherokee_list_t _b1;
	cint_t          len_t1;
	cint_t          len_b1;

	/* LFU (Least Frequently Used) */
	cherokee_list_t _t2;
	cherokee_list_t _b2;
	cint_t          len_t2;
	cint_t          len_b2;

	/* Stats */
	cuint_t         count;
	cuint_t         count_hit;
	cuint_t         count_miss;

	/* Configuration */
	cuint_t         max_size;
	cint_t          target_t1;

	/* Callbacks */
	cherokee_cache_new_func_t  new_cb;
	void                      *new_cb_param;
	cherokee_cache_get_stats_t stats_cb;

	/* Private properties */
	cherokee_cache_priv_t     *priv;
};

struct cherokee_cache_entry {
	/* Internal stuff */
	cherokee_list_t              listed;
	cherokee_buffer_t            key;
	cherokee_cache_list_t        in_list;

	cint_t                       ref_count;
	void                        *mutex;
	
	/* Callbacks */
	cherokee_cache_entry_clean_t clean_cb;
	cherokee_cache_entry_fetch_t fetch_cb;
	cherokee_cache_entry_free_t  free_cb;
};

/* Castings */
#define CACHE(x)       ((cherokee_cache_t *)(x))
#define CACHE_ENTRY(x) ((cherokee_cache_entry_t *)(x))


/* Cache Entries
 */
ret_t cherokee_cache_entry_init  (cherokee_cache_entry_t  *entry, 
				  cherokee_buffer_t       *key,
				  void                    *mutex);
ret_t cherokee_cache_entry_unref (cherokee_cache_entry_t **entry);

/* Cache Objects
 */
ret_t cherokee_cache_init      (cherokee_cache_t *cache);
ret_t cherokee_cache_mrproper  (cherokee_cache_t *cache);
ret_t cherokee_cache_clean     (cherokee_cache_t *cache);

/* Cache Functionality
 */
ret_t cherokee_cache_clean_up  (cherokee_cache_t        *cache);
ret_t cherokee_cache_get       (cherokee_cache_t        *cache, 
				cherokee_buffer_t       *key, 
				cherokee_cache_entry_t **entry);

ret_t cherokee_cache_get_stats (cherokee_cache_t        *cache,
				cherokee_buffer_t       *info);

#endif /* CHEROKEE_CACHE_H */
