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
#include "downloader_manager.h"
#include "downloader-protected.h"


#define DEFAULT_MIN   0
#define DEFAULT_MAX  50
#define DEFAULT_SMAX 20

typedef struct {
	cherokee_downloader_t downloader;
	cherokee_list_t       listed;
} cherokee_downloader_entry_t;

#define DOWN_ENTRY(o) ((cherokee_downloader_entry_t *)(o))

ret_t
cherokee_downloader_mgr_init (cherokee_downloader_mgr_t *mgr)
{
	CHEROKEE_MUTEX_INIT (&mgr->mutex, NULL);

	INIT_LIST_HEAD (&mgr->available);
	INIT_LIST_HEAD (&mgr->assigned);
	
	mgr->min       = DEFAULT_MIN;
	mgr->max       = DEFAULT_MAX;
	mgr->smax      = DEFAULT_SMAX;
	mgr->keepalive = true;

	return ret_ok;
}


ret_t
cherokee_downloader_mgr_configure (cherokee_downloader_mgr_t *mgr,
				   cherokee_config_node_t    *conf)
{
	return ret_ok;
}


ret_t
cherokee_downloader_mgr_mrproper (cherokee_downloader_mgr_t *mgr)
{
	cherokee_list_t *i;

	CHEROKEE_MUTEX_DESTROY (&mgr->mutex);

	list_for_each (i, &mgr->available) {
		cherokee_downloader_free (DOWNLOADER(i));
	}

	list_for_each (i, &mgr->assigned) {
		cherokee_downloader_free (DOWNLOADER(i));
	}

	return ret_ok;
}


ret_t
cherokee_downloader_mgr_get (cherokee_downloader_mgr_t *mgr,
			     cherokee_downloader_t    **down)
{
	ret_t                        ret;
	cherokee_downloader_entry_t *down_entry;
	size_t                       len         = 0;

	CHEROKEE_MUTEX_LOCK (&mgr->mutex);

	/* A connection is available
	 */
	if (! cherokee_list_empty (&mgr->available)) {
		down_entry = list_entry (mgr->available.next,
					 cherokee_downloader_entry_t,
					 listed);

		cherokee_list_del (&down_entry->listed);
		cherokee_list_add (&down_entry->listed, &mgr->assigned);

		*down = DOWNLOADER(down_entry);

		CHEROKEE_MUTEX_UNLOCK (&mgr->mutex);
		return ret_ok;
	}

	/* Check limits
	 */
	cherokee_list_get_len (&down_entry->listed, &len);
	if (len > mgr->max) {
		ret = ret_deny;
		goto error;
	}

	/* Instance a new one
	 */
	*down = NULL;

	down_entry = (cherokee_downloader_entry_t *) 
		malloc(sizeof (cherokee_downloader_entry_t));
	if (down_entry == NULL) {
		ret = ret_nomem;
		goto error;
	}

	INIT_LIST_HEAD (&down_entry->listed);
	ret = cherokee_downloader_init (DOWNLOADER(down_entry));
	if (ret != ret_ok) {
		ret = ret_error;
		goto error;
	}

	cherokee_downloader_set_keepalive (DOWNLOADER(down_entry), mgr->keepalive);

	cherokee_list_add (&down_entry->listed, &mgr->assigned);
	*down = DOWNLOADER(down_entry);

	CHEROKEE_MUTEX_UNLOCK (&mgr->mutex);
	return ret_ok;

error:
	CHEROKEE_MUTEX_UNLOCK (&mgr->mutex);
	*down = NULL;
	return ret;
}


ret_t
cherokee_downloader_mgr_release (cherokee_downloader_mgr_t *mgr,
				 cherokee_downloader_t     *down)
{
	cherokee_downloader_entry_t *down_entry = DOWN_ENTRY(down);

	CHEROKEE_MUTEX_LOCK (&mgr->mutex);

	/* It is no longer assigned
	 */
	cherokee_list_del (&down_entry->listed);
	cherokee_list_add (&down_entry->listed, &mgr->available);

	CHEROKEE_MUTEX_UNLOCK (&mgr->mutex);
	return ret_ok;
}
