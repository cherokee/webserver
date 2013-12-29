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

#if !defined (CHEROKEE_INSIDE_CHEROKEE_H) && !defined (CHEROKEE_COMPILATION)
# error "Only <cherokee/cherokee.h> can be included directly, this file may disappear or change contents."
#endif

#ifndef CHEROKEE_IOCACHE_H
#define CHEROKEE_IOCACHE_H

#include <cherokee/common.h>
#include <cherokee/server.h>
#include <cherokee/cache.h>
#include <cherokee/config_node.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

typedef struct {
	cherokee_cache_t        cache;

	/* Limits */
	cuint_t                 max_file_size;
	cuint_t                 min_file_size;
	cuint_t                 lasting_mmap;
	cuint_t                 lasting_stat;
} cherokee_iocache_t;

typedef enum {
	iocache_nothing = 0,
	iocache_stat    = 1,
	iocache_mmap    = 1 << 1
} cherokee_iocache_info_t;

typedef struct {
	/* Inheritance */
	cherokee_cache_entry_t  base;
	cherokee_iocache_info_t info;

	/* Information to cache */
	struct stat             state;
	ret_t                   state_ret;
	void                   *mmaped;
	size_t                  mmaped_len;
} cherokee_iocache_entry_t;

#define IOCACHE(x)       ((cherokee_iocache_t *)(x))
#define IOCACHE_ENTRY(x) ((cherokee_iocache_entry_t *)(x))


/* I/O cache
 */
ret_t cherokee_iocache_new             (cherokee_iocache_t **iocache);
ret_t cherokee_iocache_free            (cherokee_iocache_t  *iocache);

ret_t cherokee_iocache_init            (cherokee_iocache_t  *iocache);
ret_t cherokee_iocache_mrproper        (cherokee_iocache_t  *iocache);

ret_t cherokee_iocache_configure       (cherokee_iocache_t     *iocache,
					cherokee_config_node_t *conf);

/* I/O cache entry
 */
ret_t cherokee_iocache_entry_unref     (cherokee_iocache_entry_t **entry);

/* Autoget: Get or Update
 */
ret_t cherokee_iocache_autoget         (cherokee_iocache_t        *iocache,
					cherokee_buffer_t         *file,
					cherokee_iocache_info_t    info,
					cherokee_iocache_entry_t **ret_io);

ret_t cherokee_iocache_autoget_fd      (cherokee_iocache_t        *iocache,
					cherokee_buffer_t         *file,
					cherokee_iocache_info_t    info,
					int                       *fd,
					cherokee_iocache_entry_t **ret_io);

/* Misc */
ret_t cherokee_iocache_get_mmaped_size (cherokee_iocache_t *iocache, size_t *total);

#endif /* CHEROKEE_IOCACHE_H */
