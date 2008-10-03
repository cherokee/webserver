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

#ifndef CHEROKEE_IOCACHE_H
#define CHEROKEE_IOCACHE_H

#include <cherokee/common.h>
#include <cherokee/server.h>
#include <cherokee/cache.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#define IOCACHE_MAX_FILE_SIZE  80000
#define IOCACHE_MIN_FILE_SIZE      1

typedef struct {
	cherokee_cache_t       cache;
} cherokee_iocache_t;

typedef struct {
	/* Inheritance */
	cherokee_cache_entry_t base;

	/* Information to cache */
	struct stat            state;
	void                  *mmaped;
	size_t                 mmaped_len;
} cherokee_iocache_entry_t;

typedef enum {
	iocache_nothing = 0,
	iocache_stat    = 1,
	iocache_mmap    = 1 << 1
} cherokee_iocache_info_t;

#define IOCACHE(x)       ((cherokee_iocache_t *)(x))
#define IOCACHE_ENTRY(x) ((cherokee_iocache_entry_t *)(x))

/* I/O cache
 */
ret_t cherokee_iocache_init            (cherokee_iocache_t  *iocache);
ret_t cherokee_iocache_get_default     (cherokee_iocache_t **iocache);
ret_t cherokee_iocache_mrproper        (cherokee_iocache_t  *iocache);
ret_t cherokee_iocache_free_default    (void);

ret_t cherokee_iocache_get_mmaped_size (cherokee_iocache_t  *iocache, size_t *total);

/* I/O cache entry
 */
ret_t cherokee_iocache_entry_update_fd (cherokee_iocache_entry_t  *entry,
					cherokee_iocache_info_t    info,
					int                       *fd);
ret_t cherokee_iocache_entry_update    (cherokee_iocache_entry_t  *entry,
				        cherokee_iocache_info_t    info);

ret_t cherokee_iocache_entry_unref     (cherokee_iocache_entry_t **entry);

/* Autoget: Get or Update */
ret_t cherokee_iocache_autoget       (cherokee_iocache_t        *iocache,
				      cherokee_buffer_t         *file,
				      cherokee_iocache_info_t    info,
				      cherokee_iocache_entry_t **ret_io);

ret_t cherokee_iocache_autoget_fd    (cherokee_iocache_t        *iocache,
				      cherokee_buffer_t         *file,
				      cherokee_iocache_info_t    info,
				      int                       *fd, 
				      cherokee_iocache_entry_t **ret_io);

#endif /* CHEROKEE_IOCACHE_H */
