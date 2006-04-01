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
#include "iocache.h"

#include "table.h"
#include "table-protected.h"
#include "list.h"
#include "buffer.h"
#include "list_merge_sort.h"
#include "server-protected.h"
#include "util.h"

#ifdef HAVE_SYS_MMAN_H
# include <sys/mman.h>
#endif

#ifdef HAVE_FCNTL_H
# include <fcntl.h>
#endif

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>


#define FRESHNESS_TIME 600
#define CACHE_SIZE      10

#ifndef O_BINARY
# define O_BINARY 0
#endif

#ifdef MAP_FILE
# define MAP_OPTIONS MAP_SHARED | MAP_FILE
#else
# define MAP_OPTIONS MAP_SHARED
#endif


typedef struct {
	cherokee_iocache_entry_t base;
	time_t                   stat_update;
	time_t                   mmap_update;
	cint_t                   mmap_usage;
	cint_t                   usages;

#ifdef HAVE_PTHREAD
	pthread_mutex_t          lock;
#endif
} cherokee_iocache_entry_extension_t;


struct cherokee_iocache {
	cherokee_server_t *srv;
	cherokee_table_t   files;
	cuint_t            files_num;
	cuint_t            files_usages;

#ifdef HAVE_PTHREAD
	pthread_mutex_t    files_lock;
#endif	
};


typedef struct {
	cherokee_iocache_t *iocache;
	float               average;
	struct list_head    to_delete;
} clean_up_params_t;


typedef struct {
	list_t                    list;
	cherokee_iocache_entry_t *file;
	const char               *filename;
} to_delete_entry_t;



#define PUBL(o) ((cherokee_iocache_entry_t *)(o))
#define PRIV(o) ((cherokee_iocache_entry_extension_t *)(o))


static cherokee_iocache_t *global_io = NULL;


ret_t 
cherokee_iocache_new (cherokee_iocache_t **iocache, cherokee_server_t *srv)
{
	CHEROKEE_NEW_STRUCT (n, iocache);

	cherokee_table_init (&n->files);
	CHEROKEE_MUTEX_INIT (&n->files_lock, NULL);

	n->files_num    = 0;
	n->files_usages = 0;

	*iocache = n;
	return ret_ok;
}


ret_t 
cherokee_iocache_new_default  (cherokee_iocache_t **iocache, cherokee_server_t *srv)
{
	ret_t ret;

	ret = cherokee_iocache_get_default (iocache);
	if (unlikely (ret != ret_ok)) return ret_ok;

	(*iocache)->srv = srv;
	return ret_ok;
}


ret_t 
cherokee_iocache_get_default (cherokee_iocache_t **iocache)
{
	ret_t ret;
	
	if (global_io == NULL) {
		ret = cherokee_iocache_new (&global_io, NULL);
		if (unlikely (ret != ret_ok)) return ret;
	}
	
	*iocache = global_io;
	return ret_ok;
}


static int
iocache_clean_up_each (const char *key, void *value, void *param)
{
	clean_up_params_t        *params = param;
	cherokee_iocache_entry_t *file   = IOCACHE_ENTRY(value); 
	to_delete_entry_t        *delobj;
	float                     usage;

	/* Reset usage
	 */
	usage = (float) PRIV(file)->usages;
	PRIV(file)->usages = 0;

	/* Is it in use?
	 */
	if (PRIV(file)->mmap_usage > 0) {
		return true;
	}

	if ((usage == -1) ||
	    (usage <= params->average))
	{
		/* Add to the removal list
		 */
		delobj = malloc (sizeof (to_delete_entry_t));
		INIT_LIST_HEAD (&delobj->list);	
		
		delobj->file     = file;
		delobj->filename = key;
		
		list_add ((list_t *)delobj, &params->to_delete);
	}

	return true;
}


static ret_t
iocache_entry_new (cherokee_iocache_entry_t **entry)
{
	CHEROKEE_NEW_STRUCT(n, iocache_entry_extension);

	PRIV(n)->stat_update = 0;
	PRIV(n)->mmap_update = 0;
	PRIV(n)->mmap_usage  = 0;
	PRIV(n)->usages      = 0;
	
	CHEROKEE_MUTEX_INIT (&PRIV(n)->lock, NULL);

	PUBL(n)->mmaped      = NULL;
	PUBL(n)->mmaped_len  = 0;

	*entry = PUBL(n);
	return ret_ok;
}


static ret_t
iocache_entry_free (cherokee_iocache_entry_t *entry)
{
	if (entry->mmaped != NULL) {
		munmap (entry->mmaped, entry->mmaped_len);

		entry->mmaped     = NULL;
		entry->mmaped_len = 0;
	}

	CHEROKEE_MUTEX_DESTROY (&PRIV(entry)->lock);

	free (entry);
	return ret_ok;
}


static ret_t
iocache_entry_update_stat (cherokee_iocache_entry_t *entry, char *filename, cherokee_iocache_t *iocache)
{
	int re;

	re = cherokee_stat (filename, &entry->state);
	if (re < 0) {
		switch (errno) {
		case EACCES: 
			return ret_deny;
		case ENOENT: 
			return ret_not_found;
		default:     
			break;
		}

		return ret_error;
	}

	PRIV(entry)->stat_update = iocache->srv->bogo_now;
	return ret_ok;
}


static ret_t
iocache_entry_update_mmap (cherokee_iocache_entry_t *entry, char *filename, int fd, cherokee_iocache_t *iocache)
{
	ret_t              ret;
	cherokee_boolean_t do_close = false;

	/* The stat information has to be fresh enough
	 */
	if (iocache->srv->bogo_now >= (PRIV(entry)->stat_update + FRESHNESS_TIME)) {
		ret = iocache_entry_update_stat (entry, filename, iocache);
		if (ret != ret_ok) return ret;
	}

	/* Directories can not be mmaped
	 */
	if (unlikely (S_ISDIR(entry->state.st_mode))) {
		return ret_deny;
	}

	/* Maybe open
	 */
	if (fd < 0) {
		fd = open (filename, O_RDONLY|O_BINARY);
		if (unlikely (fd < 0)) return ret_error;

		do_close = true;
	}

	/* Free previous mmap
	 */
	if (entry->mmaped != NULL) {
		munmap (entry->mmaped, entry->mmaped_len);

		entry->mmaped     = NULL;
		entry->mmaped_len = 0;
	}

	/* Do it
	 */
	entry->mmaped =
		mmap (NULL,                 /* void   *start  */
		      entry->state.st_size, /* size_t  length */
		      PROT_READ,            /* int     prot   */
		      MAP_OPTIONS,          /* int     flag   */
		      fd,                   /* int     fd     */
		      0);                   /* off_t   offset */

	if (entry->mmaped == MAP_FAILED) {
		return ret_not_found;
	}

	PUBL(entry)->mmaped_len  = entry->state.st_size;
	PRIV(entry)->mmap_update = iocache->srv->bogo_now;

	/* Has it to close
	 */
	if (do_close) {
		close (fd);
	}

	return ret_ok;
}


ret_t 
cherokee_iocache_clean_up (cherokee_iocache_t *iocache, cuint_t num)
{
	float              average;
	clean_up_params_t  params;
	list_t            *i, *tmp;
	
	if (iocache->files_num < CACHE_SIZE)
		return ret_ok;

	average = (iocache->files_usages / iocache->files_num);

	params.iocache = iocache;
	params.average = average;
	INIT_LIST_HEAD(&params.to_delete);

	/* Check entry by entry
	 */
	cherokee_table_while (&iocache->files,       /* table obj */
			      iocache_clean_up_each, /* func */
			      &params,               /* param */
			      NULL,                  /* key */
			      NULL);                 /* value */

	/* Remove some files
	 */
	list_for_each_safe (i, tmp, &params.to_delete) {
		to_delete_entry_t *delobj = (to_delete_entry_t *)i;

		cherokee_table_del (&iocache->files, (char *)delobj->filename, NULL);
		iocache->files_num--;

		iocache_entry_free (delobj->file);
		free (delobj);
	}

	/* Reset statistics values
	 */
	iocache->files_usages = 0;

	return ret_ok;
}


ret_t 
cherokee_iocache_free (cherokee_iocache_t *iocache)
{
	cherokee_table_mrproper2 (&iocache->files, (cherokee_table_free_item_t)iocache_entry_free);

	CHEROKEE_MUTEX_DESTROY (&iocache->files_lock);

	free (iocache);
	return ret_ok;
}


ret_t 
cherokee_iocache_free_default (cherokee_iocache_t *iocache)
{
	if (iocache == global_io) {
		global_io = NULL;
	}

	return cherokee_iocache_free (iocache);	
}


ret_t 
cherokee_iocache_stat_get (cherokee_iocache_t *iocache, char *filename, cherokee_iocache_entry_t **file)
{
	ret_t                     ret;
	cherokee_iocache_entry_t *new;

	CHEROKEE_MUTEX_LOCK (&iocache->files_lock);

	/* Look inside the table
	 */
	ret = cherokee_table_get (&iocache->files, filename, (void **)file);
	if (ret == ret_ok) {
		new = *file;

		if (iocache->srv->bogo_now >= (PRIV(new)->stat_update + FRESHNESS_TIME)) {
			ret = iocache_entry_update_stat (new, filename, iocache);
			if (ret != ret_ok) {
				CHEROKEE_MUTEX_UNLOCK (&iocache->files_lock);
				return ret;
			}
		}

		PRIV(new)->usages++;
		iocache->files_usages++;

		CHEROKEE_MUTEX_UNLOCK (&iocache->files_lock);
		return ret_ok;
	}

	/* Create a new entry
	 */
	iocache_entry_new (&new);

	ret = iocache_entry_update_stat (new, filename, iocache);
	if (ret != ret_ok) {
		CHEROKEE_MUTEX_UNLOCK (&iocache->files_lock);
		return ret;
	}

	/* Add to the table
	 */
	cherokee_table_add (&iocache->files, filename, (void *)new);

	*file = new;
	PRIV(new)->usages++;
	iocache->files_num++;
	iocache->files_usages++;

	CHEROKEE_MUTEX_UNLOCK (&iocache->files_lock);
	return ret_ok;
}


ret_t 
cherokee_iocache_mmap_get_w_fd (cherokee_iocache_t *iocache, char *filename, int fd, cherokee_iocache_entry_t **file)
{
	ret_t                     ret;
	cherokee_iocache_entry_t *new;

	CHEROKEE_MUTEX_LOCK (&iocache->files_lock);

	/* Look inside the table
	 */
	ret = cherokee_table_get (&iocache->files, filename, (void **)file);
	if (ret == ret_ok) {
		new = *file;

		if (iocache->srv->bogo_now >= (PRIV(new)->stat_update + FRESHNESS_TIME)) {
			ret = iocache_entry_update_stat (new, filename, iocache);
			if (ret != ret_ok) {
				CHEROKEE_MUTEX_UNLOCK (&iocache->files_lock);
				return ret;
			}
		}

		if (iocache->srv->bogo_now >= (PRIV(new)->mmap_update + FRESHNESS_TIME)) {
			ret = iocache_entry_update_mmap (new, filename, fd, iocache);
			if (ret != ret_ok) {
				CHEROKEE_MUTEX_UNLOCK (&iocache->files_lock);
				return ret;
			}
		}

		PRIV(new)->usages++;
		iocache->files_usages++;

		CHEROKEE_MUTEX_UNLOCK (&iocache->files_lock);
		return ret_ok;
	}

	/* Create a new entry
	 */
	iocache_entry_new (&new);

	PRIV(new)->mmap_usage++;
	iocache_entry_update_mmap (new, filename, fd, iocache);

	/* Add to the table
	 */
	cherokee_table_add (&iocache->files, filename, new);

	*file = new;
	PRIV(new)->usages++;
	iocache->files_num++;
	iocache->files_usages++;

	CHEROKEE_MUTEX_UNLOCK (&iocache->files_lock);
	return ret_ok;
}


ret_t 
cherokee_iocache_mmap_lookup (cherokee_iocache_t *iocache, char *filename, cherokee_iocache_entry_t **file)
{
	ret_t                     ret;
	cherokee_iocache_entry_t *new;

	CHEROKEE_MUTEX_LOCK (&iocache->files_lock);

	/* Look in the table
	 */
	ret = cherokee_table_get (&iocache->files, filename, (void **)file);
	if (ret != ret_ok) {
		CHEROKEE_MUTEX_UNLOCK (&iocache->files_lock);
		return ret;
	}

	new = *file;

	/* Is it old?
	 */
	if (iocache->srv->bogo_now >= (PRIV(new)->mmap_update + FRESHNESS_TIME)) {
		CHEROKEE_MUTEX_UNLOCK (&iocache->files_lock);
		return ret_eagain;
	}

	/* Return it
	 */
	PRIV(new)->usages++;
	iocache->files_usages++;

	CHEROKEE_MUTEX_UNLOCK (&iocache->files_lock);
	return ret_ok;
}


ret_t 
cherokee_iocache_mmap_get (cherokee_iocache_t *iocache, char *filename, cherokee_iocache_entry_t **file)
{
	return cherokee_iocache_mmap_get_w_fd (iocache, filename, -1, file);
}


ret_t 
cherokee_iocache_mmap_release (cherokee_iocache_t *iocache, cherokee_iocache_entry_t *file)
{
	CHEROKEE_MUTEX_LOCK (&iocache->files_lock);
	PRIV(file)->mmap_usage--;
	CHEROKEE_MUTEX_UNLOCK (&iocache->files_lock);

	return ret_ok;
}
