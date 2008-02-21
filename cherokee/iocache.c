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

#include "common-internal.h"
#include "iocache.h"

#include "avl.h"
#include "list.h"
#include "buffer.h"
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

#define ENTRIES "iocache"

#define FRESHNESS_TIME_STAT 300
#define FRESHNESS_TIME_MMAP 600
#define CACHE_SIZE          10
#define CACHE_SIZE_MAX      50

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
	cint_t                   ref_counter;
	cint_t                   usages;

	/* unref */
	cherokee_list_t          to_be_deleted;
	cherokee_buffer_t       *name_ref;
} cherokee_iocache_entry_extension_t;


struct cherokee_iocache {
	cherokee_server_t *srv;
	cherokee_avl_t     files;
	cuint_t            files_num;
	cuint_t            files_max;
	cuint_t            files_usages;
	CHEROKEE_MUTEX_T  (files_lock);

	/* cleaning up stuff */
	float              average;
	cherokee_list_t    to_delete;
};


#define PUBL(o) ((cherokee_iocache_entry_t *)(o))
#define PRIV(o) ((cherokee_iocache_entry_extension_t *)(o))


static cherokee_iocache_t *global_io = NULL;


static ret_t 
cherokee_iocache_new (cherokee_iocache_t **iocache, cherokee_server_t *srv)
{
	CHEROKEE_NEW_STRUCT (n, iocache);

	cherokee_avl_init   (&n->files);
	CHEROKEE_MUTEX_INIT (&n->files_lock, NULL);

	n->files_num     = 0;
	n->files_max     = CACHE_SIZE_MAX;
	n->files_usages  = 0;
	n->srv           = srv;

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


static ret_t
iocache_entry_new (cherokee_iocache_entry_t **entry)
{
	CHEROKEE_NEW_STRUCT(n, iocache_entry_extension);

	PRIV(n)->stat_update = 0;
	PRIV(n)->mmap_update = 0;
	PRIV(n)->usages      = 0;
	PRIV(n)->ref_counter = 0;

	PRIV(n)->name_ref    = NULL;
	INIT_LIST_HEAD(&PRIV(n)->to_be_deleted);

	PUBL(n)->mmaped      = NULL;
	PUBL(n)->mmaped_len  = 0;

	*entry = PUBL(n);
	return ret_ok;
}

static ret_t
iocache_entry_free (cherokee_iocache_entry_t *entry)
{
	/* Free the object
	 */
	if (entry->mmaped != NULL) {
		munmap (entry->mmaped, entry->mmaped_len);

		entry->mmaped     = NULL;
		entry->mmaped_len = 0;
	}

	/* Free the entry object
	 */
	free (entry);
	return ret_ok;
}

static ret_t
iocache_entry_ref (cherokee_iocache_entry_t *entry)
{
	PRIV(entry)->ref_counter++;
	return ret_ok;
}


static ret_t
iocache_entry_unref (cherokee_iocache_entry_t *entry)
{
	cherokee_iocache_entry_extension_t *priv = PRIV(entry);

	priv->ref_counter--;	
	if (priv->ref_counter > 0)
		return ret_eagain;

	return ret_ok;
}


static ret_t
iocache_free_entry (cherokee_iocache_t *iocache, cherokee_iocache_entry_t *entry)
{
	ret_t ret;

	ret = iocache_entry_free (entry);

	/* Update the obj counter
	 */
	iocache->files_num--;

	return ret;
}


static ret_t
iocache_entry_update_stat (cherokee_iocache_t *iocache, cherokee_iocache_entry_t *entry, cherokee_buffer_t *filename)
{
	int re;

	re = cherokee_stat (filename->buf, &entry->state);
	if (re < 0) {
		TRACE(ENTRIES, "Couldn't update stat: errno=%d\n", re);

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
iocache_entry_update_mmap (cherokee_iocache_t *iocache, cherokee_iocache_entry_t *entry, cherokee_buffer_t *filename, int fd, int *ret_fd)
{
	ret_t ret;

	TRACE(ENTRIES, "Update mmap: %s\n", filename->buf);

	/* The stat information has to be fresh enough
	 */
	if (iocache->srv->bogo_now >= (PRIV(entry)->stat_update + FRESHNESS_TIME_STAT)) {
		ret = iocache_entry_update_stat (iocache, entry, filename);
		if (ret != ret_ok) return ret;
	}

	/* Only map regular files
	 */
	if (unlikely (! S_ISREG(entry->state.st_mode))) {
		TRACE(ENTRIES, "Not a regular file: %s\n", filename->buf);
		return ret_deny;
	}

	/* Maybe it is already opened
	 */
	if (fd < 0) {
		fd = open (filename->buf, O_RDONLY|O_BINARY);
		if (unlikely (fd < 0)) {
			TRACE(ENTRIES, "Couldn't open(): %s\n", filename->buf);
			return ret_error;
		}
	}

	*ret_fd = fd;

	/* Might need to free the previous mmap
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
		int err = errno;
		TRACE(ENTRIES, "%s mmap() failed: errno=%d\n", filename->buf, err);

		switch (err) {
		case EAGAIN:
		case ENOMEM:
		case ENFILE:
			return ret_eagain;
		default: 
			return ret_not_found;
		}
	}

	PUBL(entry)->mmaped_len  = entry->state.st_size;
	PRIV(entry)->mmap_update = iocache->srv->bogo_now;

	return ret_ok;
}


static ret_t
iocache_entry_maybe_update_mmap (cherokee_iocache_t *iocache, cherokee_iocache_entry_t *entry, cherokee_buffer_t *filename, int fd, int *ret_fd)
{
	cherokee_boolean_t update;

	/* Update mmap only if..
	 * - It is not refered and either:
	 *   - It has no mmap
	 *   - It is old
	 */
	update  = (entry->mmaped == NULL);
	update |= (iocache->srv->bogo_now >= (PRIV(entry)->mmap_update + FRESHNESS_TIME_MMAP));
	update &= (PRIV(entry)->ref_counter <= 1);

	if (! update)
		return ret_deny;

	return iocache_entry_update_mmap (iocache, entry, filename, fd, ret_fd);
}


static int
iocache_clean_up_each (cherokee_buffer_t *key, void *value, void *param)
{
	float                               usage;
	cherokee_iocache_t                 *iocache    = IOCACHE(param);
	cherokee_iocache_entry_extension_t *entry_priv = PRIV(value);

	/* Reset usage value
	 */
	usage = (float) entry_priv->usages;
	entry_priv->usages = 0;

	/* Is it in use or worth keeping?
	 */
	if ((entry_priv->ref_counter > 0) ||
	    (usage > iocache->average)) 
	{
		goto out;
	}

	/* Then, it should be deleted
	 */
	entry_priv->name_ref = key;
	cherokee_list_add_tail (&entry_priv->to_be_deleted, &iocache->to_delete);

out:
	return false;
}


ret_t 
cherokee_iocache_clean_up (cherokee_iocache_t *iocache, cuint_t num)
{
	ret_t              ret;
	float              average;
	cherokee_list_t   *i, *tmp;

	CHEROKEE_MUTEX_LOCK (&iocache->files_lock);
	TRACE(ENTRIES, "Clean-up: %d files\n", iocache->files_num);

	if (iocache->files_num < CACHE_SIZE)
		goto ok;

	average = (iocache->files_usages / iocache->files_num) + 1;

	iocache->average = average;
	INIT_LIST_HEAD(&iocache->to_delete);

	/* Check entry by entry
	 */
	cherokee_avl_while (&iocache->files,       /* table obj */
			    iocache_clean_up_each, /* func */
			    iocache,               /* param */
			    NULL,                  /* key */
			    NULL);                 /* value */

#ifdef TRACE_ENABLED
	{ 
		int n = 0;
		list_for_each_safe (i, tmp, &iocache->to_delete) { n++; }
		TRACE(ENTRIES, "Clean-up list length: %d\n", n);
	}
#endif

	/* Remove some files
	 */
	list_for_each_safe (i, tmp, &iocache->to_delete) {
		cherokee_iocache_entry_extension_t *ret_entry = NULL;
		cherokee_iocache_entry_extension_t *entry;

		entry = list_entry (i, cherokee_iocache_entry_extension_t, to_be_deleted);

		ret = cherokee_avl_del (&iocache->files, entry->name_ref, (void **)&ret_entry);
		if (unlikely (ret != ret_ok)) return ret;

		entry->name_ref = NULL; /* freed in table::del() */

		cherokee_list_del (&entry->to_be_deleted);
		iocache_free_entry (iocache, IOCACHE_ENTRY(entry));
	}

	/* Reset statistics values
	 */
	iocache->files_usages = 0;

ok:
	CHEROKEE_MUTEX_UNLOCK (&iocache->files_lock);
	return ret_ok;
}


static ret_t 
cherokee_iocache_free (cherokee_iocache_t *iocache)
{
	cherokee_avl_mrproper (&iocache->files, 
			       (cherokee_func_free_t) iocache_entry_free);
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

static void
hit (cherokee_iocache_t *iocache, cherokee_iocache_entry_t *file)
{
	PRIV(file)->usages++;
	iocache->files_usages++;
}


ret_t
cherokee_iocache_get_or_create_w_stat (cherokee_iocache_t *iocache, cherokee_buffer_t *filename, cherokee_iocache_entry_t **ret_file)
{
	ret_t                     ret;
	cherokee_iocache_entry_t *entry;

	CHEROKEE_MUTEX_LOCK (&iocache->files_lock);
	TRACE(ENTRIES, "With stat: %s\n", filename->buf);

	/* Look inside the table
	 */
	ret = cherokee_avl_get (&iocache->files, filename, (void **)ret_file);
	if (ret != ret_ok) {
		if (iocache->files_num >= iocache->files_max) {
			*ret_file = NULL;
			ret = ret_no_sys;
			goto error;
		}

		ret = iocache_entry_new (ret_file);
		if (unlikely (ret != ret_ok)) 
			goto error;

		ret = cherokee_avl_add (&iocache->files, filename, *ret_file);
		if (unlikely (ret != ret_ok))
			goto error_free;

		TRACE(ENTRIES, "Added new '%s': %p\n", filename->buf, ret_file);

		iocache->files_num++;
	} 
#ifdef TRACE_ENABLED
	else {
		TRACE(ENTRIES, "Found in cache '%s': %p\n", filename->buf, ret_file);
	}
#endif

	/* Reference it
	 */
	entry = *ret_file;
	hit (iocache, entry);
	iocache_entry_ref(entry);
	
	/* May update the stat info
	 */
	if (iocache->srv->bogo_now >= (PRIV(entry)->stat_update + FRESHNESS_TIME_STAT)) {
		ret = iocache_entry_update_stat (iocache, entry, filename);
		if (unlikely (ret != ret_ok)) goto error;
	}

	CHEROKEE_MUTEX_UNLOCK (&iocache->files_lock);
	return ret_ok;

error_free:
	free (*ret_file);
	*ret_file = NULL;

error:
	CHEROKEE_MUTEX_UNLOCK (&iocache->files_lock);
	return ret;
}


ret_t 
cherokee_iocache_get_or_create_w_mmap (cherokee_iocache_t *iocache, cherokee_buffer_t *filename, cherokee_iocache_entry_t **ret_file, int *ret_fd)
{
	ret_t                     ret;
	cherokee_iocache_entry_t *entry;

	CHEROKEE_MUTEX_LOCK (&iocache->files_lock);
	TRACE(ENTRIES, "With mmap: %s\n", filename->buf);

	/* Fetch or create the entry object
	 */
	if (*ret_file == NULL) {
		ret = cherokee_avl_get (&iocache->files, filename, (void **)ret_file);
		if (ret != ret_ok) {
			if (iocache->files_num >= iocache->files_max) {
				ret = ret_no_sys;
				goto error;
			}
			
			ret = iocache_entry_new (ret_file);
			if (unlikely (ret != ret_ok)) goto error;
			
			ret = cherokee_avl_add (&iocache->files, filename, *ret_file);
			if (unlikely (ret != ret_ok)) {
				goto error_free;
			}

			TRACE(ENTRIES, "Added new '%s': %p\n", filename->buf, ret_file);
			
			iocache->files_num++;
		}

		/* Reference the entry
		 */
		iocache_entry_ref(*ret_file);
	}
#ifdef TRACE_ENABLED
	else {
		TRACE(ENTRIES, "Found in cache '%s': %p\n", filename->buf, ret_file);
	}
#endif

	/* Update statistics
	 */
	entry = *ret_file;
	hit (iocache, entry);

	/* Try to update the mmaped memory
	 */
	ret = iocache_entry_maybe_update_mmap (iocache, entry, filename, -1, ret_fd);
	switch (ret) {
	case ret_ok:
		/* Updated */ 
		ret = ret_ok;
		break;
	case ret_deny:
		/* Not updated */
		ret = ret_ok;
		break;
	case ret_eagain:
		/* Tried, but failed  */
		ret = ret_no_sys;
		break;
	default:
		goto error;
	}

	CHEROKEE_MUTEX_UNLOCK (&iocache->files_lock);
	return ret;

error_free:
	free (*ret_file);
	*ret_file = NULL;

error:
	CHEROKEE_MUTEX_UNLOCK (&iocache->files_lock);
	return ret;
}


ret_t 
cherokee_iocache_mmap_release (cherokee_iocache_t *iocache, cherokee_iocache_entry_t *file)
{
	ret_t ret;

	if (file == NULL) {
		TRACE(ENTRIES, "Release: %s\n", "NULL");
		return ret_not_found;
	}

#ifdef TRACE_ENABLED
	if (PRIV(file)->name_ref)
		TRACE(ENTRIES, "Release: %s\n", PRIV(file)->name_ref->buf);
	else
		TRACE(ENTRIES, "Releasing obj with no name_ref %s", "\n");
#endif

	CHEROKEE_MUTEX_LOCK (&iocache->files_lock);
	ret = iocache_entry_unref (file);
	CHEROKEE_MUTEX_UNLOCK (&iocache->files_lock);

	return ret;
}
