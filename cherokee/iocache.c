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
# define MAP_OPTIONS MAP_PRIVATE | MAP_FILE
#else
# define MAP_OPTIONS MAP_PRIVATE
#endif


typedef struct {
	cherokee_iocache_entry_t base;
	time_t                   stat_expiration;
	time_t                   mmap_expiration;
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

	/* Nothing will be left
	 */
	ioentry->info      = iocache_nothing;
	ioentry->state_ret = 123456;

	/* Free the mmaped info
	 */
	if (ioentry->mmaped != NULL) {
		munmap (ioentry->mmaped, ioentry->mmaped_len);

		ioentry->mmaped     = NULL;
		ioentry->mmaped_len = 0;
	}

	/* Mark it as expired
	 */
	PRIV(entry)->mmap_expiration = 0;
	PRIV(entry)->stat_expiration = 0;

	return ret_ok;
}

static ret_t
free_cb (cherokee_cache_entry_t *entry)
{
	CHEROKEE_MUTEX_DESTROY (&PRIV(entry)->parent_lock);
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

	UNUSED(param);

	CHEROKEE_MUTEX_INIT (&PRIV(n)->parent_lock, CHEROKEE_MUTEX_FAST);

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
	PRIV(n)->stat_expiration = 0;
	PRIV(n)->mmap_expiration = 0;
	PUBL(n)->mmaped          = NULL;
	PUBL(n)->mmaped_len      = 0;
	PUBL(n)->info            = 0;
	PUBL(n)->state_ret       = ret_ok;

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
		int                     val;
		cherokee_config_node_t *subconf = CONFIG_NODE(i);

		if (equal_buf_str (&subconf->key, "max_file_size")) {
			ret = cherokee_atoi (subconf->val.buf, &val);
			if (ret != ret_ok) return ret_error;
			iocache->max_file_size = val;

		} else if (equal_buf_str (&subconf->key, "min_file_size")) {
			ret = cherokee_atoi (subconf->val.buf, &val);
			if (ret != ret_ok) return ret_error;
			iocache->min_file_size = val;

		} else if (equal_buf_str (&subconf->key, "lasting_stat")) {
			ret = cherokee_atoi (subconf->val.buf, &val);
			if (ret != ret_ok) return ret_error;
			iocache->lasting_stat = val;

		} else if (equal_buf_str (&subconf->key, "lasting_mmap")) {
			ret = cherokee_atoi (subconf->val.buf, &val);
			if (ret != ret_ok) return ret_error;
			iocache->lasting_mmap = val;
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

	/* Returns:
	 * ret_ok          - Ok, updated
	 * ret_ok_and_sent - It's still fresh
	 * ret_deny        - No info about the file
	 */

	if (PRIV(entry)->stat_expiration >= cherokee_bogonow_now) {
		TRACE (ENTRIES, "Update stat: %s: updated - skipped\n",
		       CACHE_ENTRY(entry)->key.buf);

		/* Checked, but file didn't exist */
		if (PUBL(entry)->state_ret != ret_ok) {
			return ret_deny;
		}

		return ret_ok_and_sent;
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
	}
	else {
		ret = ret_ok;
	}

	TRACE (ENTRIES, "Updated stat: %s, ret=%d\n", CACHE_ENTRY(entry)->key.buf, ret);

	PRIV(entry)->stat_expiration = cherokee_bogonow_now + iocache->lasting_stat;
	PUBL(entry)->state_ret       = ret;

	BIT_SET (PUBL(entry)->info, iocache_stat);
	return (ret == ret_ok) ? ret_ok : ret_deny;
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
		fd_local = cherokee_open (filename->buf, (O_RDONLY | O_BINARY), 0);
		if (unlikely (fd_local < 0)) {
			TRACE(ENTRIES, "Couldn't open(%s) = %s\n", filename->buf, strerror(errno));

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

			PRIV(entry)->stat_expiration = cherokee_bogonow_now + iocache->lasting_stat;
			PUBL(entry)->state_ret       = ret;

			goto error;
		}

		cherokee_fd_set_closexec (fd_local);

		if (fd != NULL) {
			*fd = fd_local;
		}
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

	if ((fd == NULL) && (fd_local != -1)) {
		cherokee_fd_close (fd_local);
	}

	BIT_SET (PUBL(entry)->info, iocache_mmap);
	return ret_ok;

error:
	if ((fd == NULL) && (fd_local != -1)) {
		cherokee_fd_close (fd_local);
	}

	BIT_UNSET (PUBL(entry)->info, iocache_mmap);
	return ret;
}


static ret_t
entry_update_fd (cherokee_iocache_entry_t *entry,
                 cherokee_iocache_info_t   info,
                 int                      *fd)
{
	ret_t               ret;
	cherokee_iocache_t *iocache = IOCACHE(CACHE_ENTRY(entry)->cache);

	/* Returns:
	 * ret_ok          - Okay
	 * ret_ok_and_sent - Ok but couldn't update it
	 *
	 * ret_deny        - Couldn't stat the file
	 * ret_not_found   - File not found
	 * ret_error       - Something bad happened
	 */

	/* - CACHE_ENTRY(entry)->ref_count > 2:
	 * It shouldn't update the object if somebody else
	 * is using it.
	 *
	 * - entry->info != iocache_nothing:
	 * However, it might happen that a few threads acquired
	 * the object at the 'same' time and they are trying to
	 * update it now.
	 */
	if ((CACHE_ENTRY(entry)->ref_count > 2) &&
	    (entry->info != iocache_nothing))
	{
		TRACE(ENTRIES, "Cannot update '%s', refs=%d\n",
		      CACHE_ENTRY(entry)->key.buf,
		      CACHE_ENTRY(entry)->ref_count);

		return ret_ok_and_sent;
	}

	/* Check the required info
	 */
	if (info & iocache_stat) {
		ioentry_update_stat (entry);
	}

	if (info & iocache_mmap) {
		/* Update mmap
		 */
		ret = ioentry_update_stat (entry);
		if ((ret != ret_ok) &&
		    (ret != ret_ok_and_sent))
		{
			/* stat() did not success
			 */
			return ret_deny;
		}

		/* Check the size before mapping it
		 */
		if ((entry->state.st_size < iocache->min_file_size) ||
		    (entry->state.st_size > iocache->max_file_size))
		{
			return ret_no_sys;
		}

		/* Go ahead
		 */
		ret = ioentry_update_mmap (entry, fd);
		if (ret != ret_ok) {
			return ret;
		}
	}

	return ret_ok;
}


static ret_t
entry_update (cherokee_iocache_entry_t *entry,
              cherokee_iocache_info_t    info)
{
	return entry_update_fd (entry, info, NULL);
}


static ret_t
fetch_info_cb (cherokee_cache_entry_t *entry)
{
	/* cache->priv->mutex is LOCKED
	 * entry->mutex       is LOCKED
	 */
	entry_update (IOCACHE_ENTRY(entry),
	              (iocache_stat | iocache_mmap));
	return ret_ok;
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
		*ret_io = IOCACHE_ENTRY(entry);
		break;
	default:
		SHOULDNT_HAPPEN;
		return ret_error;
	}

	/* Update the cached info
	 */
	CHEROKEE_MUTEX_LOCK (entry->mutex);
	if (fd) {
		ret = entry_update_fd (*ret_io, info, fd);
	} else {
		ret = entry_update (*ret_io, info);
	}
	CHEROKEE_MUTEX_UNLOCK (entry->mutex);

	if (unlikely (PUBL(entry)->info == 0)) {
		SHOULDNT_HAPPEN;
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
	ret_t                     ret;
	cherokee_iocache_entry_t *entry = *ret_io;

	TRACE (ENTRIES, "file=%s\n", file->buf);

	if (entry) {
		if (unlikely (cherokee_buffer_cmp_buf (file, &CACHE_ENTRY(entry)->key) != 0))
			SHOULDNT_HAPPEN;

		CHEROKEE_MUTEX_LOCK (CACHE_ENTRY(entry)->mutex);
		ret = entry_update_fd (entry, info, fd);
		CHEROKEE_MUTEX_UNLOCK (CACHE_ENTRY(entry)->mutex);

		return ret;
	}

	return iocache_get (iocache, file, info, fd, ret_io);
}

ret_t
cherokee_iocache_autoget (cherokee_iocache_t        *iocache,
			  cherokee_buffer_t         *file,
			  cherokee_iocache_info_t    info,
			  cherokee_iocache_entry_t **ret_io)
{
	ret_t                     ret;
	cherokee_iocache_entry_t *entry = *ret_io;

	TRACE (ENTRIES, "file=%s\n", file->buf);

	if (entry) {
		if (unlikely (cherokee_buffer_cmp_buf (file, &CACHE_ENTRY(entry)->key) != 0))
			SHOULDNT_HAPPEN;

		CHEROKEE_MUTEX_LOCK (CACHE_ENTRY(entry)->mutex);
		ret = entry_update (entry, info);
		CHEROKEE_MUTEX_UNLOCK (CACHE_ENTRY(entry)->mutex);

		return ret;
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
