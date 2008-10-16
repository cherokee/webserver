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

#include "buffer.h"
#include "server-protected.h"
#include "util.h"
#include "bogotime.h"

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

#define LASTING_MMAP     (5 * 60)            /* secs */
#define LASTING_STAT     (5 * 60)            /* secs */
#define MIN_FILE_SIZE    1                   /* bytes */
#define MAX_FILE_SIZE    SENDFILE_MIN_SIZE   /* bytes */


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
	ret_t                    stat_status;
	time_t                   stat_expiration;
	time_t                   mmap_expiration;
	CHEROKEE_MUTEX_T        (updating);
	CHEROKEE_MUTEX_T        (parent_lock);
} cherokee_iocache_entry_extension_t;

#define PUBL(o) ((cherokee_iocache_entry_t *)(o))
#define PRIV(o) ((cherokee_iocache_entry_extension_t *)(o))

static ret_t fetch_info_cb (cherokee_cache_entry_t *entry);

CHEROKEE_ADD_FUNC_NEW (iocache);
CHEROKEE_ADD_FUNC_FREE (iocache);


static ret_t
clean_info_cb (cherokee_cache_entry_t *entry)
{
	cherokee_iocache_entry_t *ioentry = IOCACHE_ENTRY(entry);

	TRACE (ENTRIES, "Cleaning cached info: '%s': %s\n", 
	       entry->key.buf, ioentry->mmaped ? "mmap": "no mmap");

	/* Free the mmaped info
	 */
	if (ioentry->mmaped) {
		munmap (ioentry->mmaped, ioentry->mmaped_len);

		ioentry->mmaped     = NULL;
		ioentry->mmaped_len = 0;
	}

	/* Mark it as expired
	 */
	PRIV(entry)->mmap_expiration = 0;

	return ret_ok;
}

static ret_t
free_cb (cherokee_cache_entry_t *entry)
{
	UNUSED(entry);
	return ret_ok;
}

static ret_t
get_stats_cb (cherokee_cache_t  *cache, 
	      cherokee_buffer_t *info)
{
	size_t total = 0;

	cherokee_iocache_get_mmaped_size (IOCACHE(cache), &total);

	cherokee_buffer_add_str (info, "IOcache mappped: ");
	cherokee_buffer_add_fsize (info, total);
	cherokee_buffer_add_str (info, "\n");

	return ret_ok;
}


static ret_t
iocache_entry_new_cb (cherokee_cache_t        *cache, 
		      cherokee_buffer_t       *key,
		      void                    *param,
		      cherokee_cache_entry_t **ret_entry)
{
	CHEROKEE_NEW_STRUCT(n, iocache_entry_extension);

	UNUSED(cache);
	UNUSED(param);

	CHEROKEE_MUTEX_INIT (&PRIV(n)->updating, NULL);
	CHEROKEE_MUTEX_INIT (&PRIV(n)->parent_lock, NULL);

	/* Init its parent class
	 */
#ifdef HAVE_PTHREAD
	cherokee_cache_entry_init (CACHE_ENTRY(n), key, cache, &PRIV(n)->parent_lock);
#else
	cherokee_cache_entry_init (CACHE_ENTRY(n), key, cache, NULL);
#endif

	/* Set the virtual methods
	 */
	CACHE_ENTRY(n)->clean_cb = clean_info_cb;
	CACHE_ENTRY(n)->fetch_cb = fetch_info_cb;
	CACHE_ENTRY(n)->free_cb  = free_cb;

	/* Init its properties
	 */
	PRIV(n)->stat_status     = ret_ok;
	PRIV(n)->stat_expiration = 0;
	PRIV(n)->mmap_expiration = 0;
	PUBL(n)->mmaped          = NULL;
	PUBL(n)->mmaped_len      = 0;
	PUBL(n)->info            = 0;

	/* Return the new object
	 */
	*ret_entry = CACHE_ENTRY(n);
	return ret_ok;
}


ret_t
cherokee_iocache_configure (cherokee_iocache_t     *iocache,
			    cherokee_config_node_t *conf)
{
	ret_t            ret;
	cherokee_list_t *i;

	/* Configure parent class
	 */
	ret = cherokee_cache_configure (CACHE(iocache), conf);
	if (ret != ret_ok)
		return ret;

	/* Configure it own properties
	 */
	cherokee_config_node_foreach (i, conf) {
		cherokee_config_node_t *subconf = CONFIG_NODE(i);
		
		if (equal_buf_str (&subconf->key, "max_file_size")) {
			iocache->max_file_size = atoi(subconf->val.buf);
		} else if (equal_buf_str (&subconf->key, "min_file_size")) {
			iocache->min_file_size = atoi(subconf->val.buf);

		} else if (equal_buf_str (&subconf->key, "lasting_stat")) {
			iocache->lasting_stat = atoi(subconf->val.buf);
		} else if (equal_buf_str (&subconf->key, "lasting_mmap")) {
			iocache->lasting_mmap = atoi(subconf->val.buf);
		}
	}

	return ret_ok;
}

ret_t 
cherokee_iocache_init (cherokee_iocache_t *iocache)
{
	ret_t ret;

	/* Init the parent (cache policy) class
	 */
	ret = cherokee_cache_init (CACHE(iocache));
	if (ret != ret_ok)
		return ret;
	
	/* Init its virtual methods
	 */
	CACHE(iocache)->new_cb       = iocache_entry_new_cb;
	CACHE(iocache)->new_cb_param = NULL;
	CACHE(iocache)->stats_cb     = get_stats_cb;

	iocache->max_file_size = MAX_FILE_SIZE;
	iocache->min_file_size = MIN_FILE_SIZE;
	iocache->lasting_stat  = LASTING_STAT;
	iocache->lasting_mmap  = LASTING_MMAP;

	return ret_ok;
}

ret_t 
cherokee_iocache_mrproper (cherokee_iocache_t *iocache)
{
	return cherokee_cache_mrproper (CACHE(iocache));
}


/* Cache entry objects
 */

ret_t
cherokee_iocache_entry_unref (cherokee_iocache_entry_t **entry)
{
	return cherokee_cache_entry_unref ((cherokee_cache_entry_t **)entry);
}

static ret_t
ioentry_update_stat (cherokee_iocache_entry_t *entry)
{
	int                 re;
	ret_t               ret;
	cherokee_iocache_t *iocache = IOCACHE(CACHE_ENTRY(entry)->cache);

	if (PRIV(entry)->stat_expiration >= cherokee_bogonow_now) {
		TRACE (ENTRIES, "Update stat: %s: updated - skipped\n", 
		       CACHE_ENTRY(entry)->key.buf);
		return PRIV(entry)->stat_status;
	}

	/* Update stat
	 */
	re = cherokee_stat (CACHE_ENTRY(entry)->key.buf, &entry->state);
	if (re < 0) {
		TRACE(ENTRIES, "Couldn't update stat: %s: errno=%d\n", 
		      CACHE_ENTRY(entry)->key.buf, errno);

		switch (errno) {
		case EACCES:
			ret = ret_deny;
			break;
		case ENOENT:
		case ENOTDIR:
			ret = ret_not_found;
			break;
		default:
			ret = ret_error;
			break;
		}

		goto out;
	}

	ret = ret_ok;
	BIT_SET (PUBL(entry)->info, iocache_stat);

	TRACE (ENTRIES, "Updated stat: %s\n", CACHE_ENTRY(entry)->key.buf);

out:
	PRIV(entry)->stat_expiration = cherokee_bogonow_now + iocache->lasting_stat;
	PRIV(entry)->stat_status     = ret;

	BIT_UNSET (PUBL(entry)->info, iocache_stat);
	return ret;
}

static ret_t
ioentry_update_mmap (cherokee_iocache_entry_t *entry, 
		     int                      *fd)
{
	ret_t              ret;
	int                fd_local = -1;
	cherokee_buffer_t *filename = &CACHE_ENTRY(entry)->key;
	cherokee_iocache_t *iocache = IOCACHE(CACHE_ENTRY(entry)->cache);

	/* Short path
	 */
	if (PRIV(entry)->mmap_expiration >= cherokee_bogonow_now) {
		TRACE(ENTRIES, "Update mmap: %s: updated - skipped\n", filename->buf);
		return ret_ok;
	}

	/* Check the fd
	 */
	if (fd != NULL)
		fd_local = *fd;

	/* Only map regular files
	 */
	if (unlikely (! S_ISREG(entry->state.st_mode))) {
		TRACE(ENTRIES, "Not a regular file: %s\n", filename->buf);
		ret = ret_deny;
		goto error;
	}

	/* Maybe it is already opened
	 */
	if (fd_local < 0) {
		fd_local = open (filename->buf, (O_RDONLY | O_BINARY));
		if (unlikely (fd_local < 0)) {
			TRACE(ENTRIES, "Couldn't open(): %s\n", filename->buf);
			ret = ret_error;
			goto error;
		}

		if (fd != NULL)
			*fd = fd_local;
	}

	/* Might need to free the previous mmap
	 */
	if (entry->mmaped != NULL) {
		TRACE(ENTRIES, "Cleaning previos mmap: %s\n", filename->buf);
		munmap (entry->mmaped, entry->mmaped_len);

		entry->mmaped     = NULL;
		entry->mmaped_len = 0;
	}

	/* Map the file into memory
	 */
	entry->mmaped =
		mmap (NULL,                 /* void   *start  */
		      entry->state.st_size, /* size_t  length */
		      PROT_READ,            /* int     prot   */
		      MAP_OPTIONS,          /* int     flag   */
		      fd_local,             /* int     fd     */
		      0);                   /* off_t   offset */

	if (entry->mmaped == MAP_FAILED) {
		int err = errno;
		TRACE (ENTRIES, "%s mmap() failed: errno=%d\n", filename->buf, err);

		switch (err) {
		case EAGAIN:
		case ENOMEM:
		case ENFILE:
			ret = ret_eagain;
			goto error;
		default:
			ret = ret_not_found;
			goto error;
		}
	}

	TRACE(ENTRIES, "Updated mmap: %s\n", filename->buf);

	PUBL(entry)->mmaped_len      = entry->state.st_size;
	PRIV(entry)->mmap_expiration = cherokee_bogonow_now + iocache->lasting_mmap;

	if ((fd == NULL) && (fd_local != -1))
		close (fd_local);

	BIT_SET (PUBL(entry)->info, iocache_mmap);
	return ret_ok;

error:
	if ((fd == NULL) && (fd_local != -1))
		close (fd_local);

	BIT_UNSET (PUBL(entry)->info, iocache_mmap);
	return ret;
}


static ret_t
fetch_info_cb (cherokee_cache_entry_t *entry)
{
	ret_t ret;

	ret = cherokee_iocache_entry_update (IOCACHE_ENTRY(entry),
					     (iocache_stat | iocache_mmap));
	switch(ret) {
	case ret_ok:
	case ret_not_found:
	case ret_ok_and_sent:
		return ret_ok;
	default:
		return ret;
	}

	SHOULDNT_HAPPEN;
	return ret_error;
}


ret_t
cherokee_iocache_entry_update_fd (cherokee_iocache_entry_t  *entry,
				  cherokee_iocache_info_t    info,
				  int                       *fd)
{
	ret_t               ret;
	cherokee_iocache_t *iocache = IOCACHE(CACHE_ENTRY(entry)->cache);

	/* Cannot update the entry when someone uses it.
	 * At this point the object has 2 references: 
	 * the cache, and ourselves after the fetch.
	 */
	if (CACHE_ENTRY(entry)->ref_count > 2) {
		TRACE(ENTRIES, "Cannot update '%s', refs=%d\n",
		      CACHE_ENTRY(entry)->key.buf,
		      CACHE_ENTRY(entry)->ref_count);

		return ret_ok_and_sent;
	}

	CHEROKEE_MUTEX_LOCK (&PRIV(entry)->updating);

	/* Check the required info
	 */
	if (info & iocache_stat) {
		ret = ioentry_update_stat (entry);
		if (ret != ret_ok) {
			goto out;
		}
	}

	if (info & iocache_mmap) {
		/* Update mmap
		 */
		ret = ioentry_update_stat (entry);
		if (ret != ret_ok) {
			goto out;
		}

		/* Check the size before mapping it
		 */
		if ((entry->state.st_size < iocache->min_file_size) ||
		    (entry->state.st_size > iocache->max_file_size))
		{
			ret = ret_no_sys;
			goto out;
		}

		/* Go ahead
		 */
		ret = ioentry_update_mmap (entry, fd);
		if (ret != ret_ok) {
			goto out;
		}
	}

	ret = ret_ok;
out:

	CHEROKEE_MUTEX_UNLOCK (&PRIV(entry)->updating);
	return ret;
}


ret_t
cherokee_iocache_entry_update (cherokee_iocache_entry_t *entry,
			       cherokee_iocache_info_t    info)
{
	return cherokee_iocache_entry_update_fd (entry, info, NULL);
}


/* I/O cache
 */
static ret_t
iocache_get (cherokee_iocache_t        *iocache,
	     cherokee_buffer_t         *file,
	     cherokee_iocache_info_t    info,
	     int                       *fd, 
	     cherokee_iocache_entry_t **ret_io)
{
	ret_t                   ret;
	cherokee_cache_entry_t *entry = NULL;

	/* Request the element to the underlining cache
	 */
	ret = cherokee_cache_get (CACHE(iocache), file, &entry);
	switch (ret) {
	case ret_ok:
	case ret_ok_and_sent:
	case ret_no_sys:
		*ret_io = IOCACHE_ENTRY(entry);
		break;		
	default:
		return ret_error;
	}

	/* Update the cached info
	 */
	if (fd)
		ret = cherokee_iocache_entry_update_fd (*ret_io, info, fd);
	else
		ret = cherokee_iocache_entry_update (*ret_io, info);		

	/* Check whether cache page was updated
         */
	if ((info == iocache_stat) &&
	    ((*ret_io)->info & iocache_stat))
        {
                return PRIV(*ret_io)->stat_status;
        }

	if ((info & iocache_mmap) &&
	    ((*ret_io)->info & iocache_mmap))
        {
		return ret_no_sys;
	}

	return ret;
}

ret_t
cherokee_iocache_autoget_fd (cherokee_iocache_t        *iocache,
			     cherokee_buffer_t         *file,
			     cherokee_iocache_info_t    info,
			     int                       *fd, 
			     cherokee_iocache_entry_t **ret_io)
{
	cherokee_iocache_entry_t *entry = *ret_io;
	
	TRACE (ENTRIES, "file=%s\n", file->buf);

	if (entry) {
		if (unlikely (cherokee_buffer_cmp_buf (file, &CACHE_ENTRY(entry)->key) != 0))
			SHOULDNT_HAPPEN;

		return cherokee_iocache_entry_update_fd (entry, info, fd);
	}

	return iocache_get (iocache, file, info, fd, ret_io);
}

ret_t
cherokee_iocache_autoget (cherokee_iocache_t        *iocache,
			  cherokee_buffer_t         *file,
			  cherokee_iocache_info_t    info,
			  cherokee_iocache_entry_t **ret_io)
{
	cherokee_iocache_entry_t *entry = *ret_io;
	
	TRACE (ENTRIES, "file=%s\n", file->buf);

	if (entry) {
		if (unlikely (cherokee_buffer_cmp_buf (file, &CACHE_ENTRY(entry)->key) != 0))
			SHOULDNT_HAPPEN;

		return cherokee_iocache_entry_update (entry, info);
	}

	return iocache_get (iocache, file, info, NULL, ret_io);
}


ret_t
cherokee_iocache_get_mmaped_size (cherokee_iocache_t *cache, size_t *total)
{
	cuint_t          n;
	cherokee_list_t *i;
	cherokee_list_t *lists[4] = {&CACHE(cache)->_t1, &CACHE(cache)->_t2};

	*total = 0;

	for (n=0; n<2; n++) {
		list_for_each (i, lists[n]) {
			*total += IOCACHE_ENTRY(i)->mmaped_len;
		}
	}

	return ret_ok;
}
