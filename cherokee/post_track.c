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
#include "post_track.h"
#include "plugin_loader.h"
#include "connection-protected.h"
#include "util.h"

#define ENTRIES "post,track"
#define TIMEOUT 60

PLUGIN_INFO_EASY_INIT (cherokee_generic, post_track);
PLUGIN_EMPTY_INIT_FUNCTION(post_track);


/* Private type
 */

typedef struct {
	cherokee_list_t    listed;
	cherokee_post_t   *post;
	cherokee_buffer_t  progress_id;
	time_t             unregistered_at;
} cherokee_post_track_entry_t;


static ret_t
entry_new (cherokee_post_track_entry_t **entry)
{
	CHEROKEE_NEW_STRUCT(n,post_track_entry);

	n->post            = NULL;
	n->unregistered_at = 0;

	INIT_LIST_HEAD (&n->listed);
	cherokee_buffer_init (&n->progress_id);

	*entry = n;
	return ret_ok;
}

static void
entry_free (void *p)
{
	cherokee_post_track_entry_t *entry = p;

	cherokee_buffer_mrproper (&entry->progress_id);
	free (entry);
}


/* Main class
 */

static void
_free (cherokee_post_track_t *track)
{
	/* Rest of the properties
	 */
	cherokee_avl_mrproper (AVL_GENERIC(&track->posts_lookup), entry_free);
	CHEROKEE_MUTEX_DESTROY (&track->lock);
	free (track);
}


static ret_t
_figure_x_progress_id (cherokee_connection_t *conn,
                       cherokee_buffer_t     *track_id)
{
	ret_t              ret;
	cherokee_buffer_t *tmp;

	/* Check the query-string
	 */
	ret = cherokee_connection_parse_args (conn);
	if (ret == ret_ok) {
		ret = cherokee_avl_get_ptr (conn->arguments, "X-Progress-ID", (void **)&tmp);
		if ((ret == ret_ok) && (tmp != NULL) && (tmp->len > 0)) {
			cherokee_buffer_add_buffer (track_id, tmp);

			TRACE (ENTRIES, "X-Progress-ID in query-string: '%s'\n", track_id->buf);
			return ret_ok;
		}
	}

	/* By any chance, it's been sent a header entry
	 */
	ret = cherokee_header_copy_unknown (&conn->header, "X-Progress-ID", 13, track_id);
	if ((ret == ret_ok) && (track_id->len > 0)) {
		TRACE (ENTRIES, "X-Progress-ID in header: '%s'\n", track_id->buf);
		return ret_ok;
	}

	TRACE (ENTRIES, "X-Progress-ID %s\n", "not found");
	return ret_not_found;
}


static ret_t
_purge_unreg (cherokee_post_track_t *track)
{
	cherokee_list_t             *i, *j;
	cherokee_post_track_entry_t *entry;

	CHEROKEE_MUTEX_LOCK (&track->lock);

	/* Iterate through the entries
	 */
	list_for_each_safe (i, j, &track->posts_list) {
		entry = (cherokee_post_track_entry_t *)i;

		/* It still in use
		 */
		if (entry->unregistered_at == 0) {
			continue;
		}

		/* It has not timed out
		 */
		if (cherokee_bogonow_now < entry->unregistered_at + TIMEOUT) {
			continue;
		}

		TRACE (ENTRIES, "Removing reference to X-Progress-ID '%s'\n",
		       entry->progress_id.buf);

		cherokee_avl_del (&track->posts_lookup, &entry->progress_id, NULL);
		cherokee_list_del (i);
		entry_free (entry);
	}

	CHEROKEE_MUTEX_UNLOCK (&track->lock);
	return ret_ok;
}


static ret_t
_register (cherokee_post_track_t *track,
           cherokee_connection_t *conn)
{
	ret_t                        ret;
	cherokee_post_track_entry_t *entry = NULL;
	cherokee_buffer_t            tmp   = CHEROKEE_BUF_INIT;

	TRACE (ENTRIES, "Register conn ID: %d\n", conn->id);

	/* Sanity check: registering twice?
	 */
	if (unlikely (! cherokee_buffer_is_empty (&conn->post.progress_id))) {
		return ret_ok;
	}

	/* Look for X-Progress-ID
	 */
	ret = _figure_x_progress_id (conn, &tmp);
	if (ret != ret_ok) {
		return ret_ok;
	}

	/* LOCK */
	CHEROKEE_MUTEX_LOCK (&track->lock);

	ret = cherokee_avl_get (&track->posts_lookup, &tmp, NULL);
	if (ret == ret_ok) {
		TRACE (ENTRIES, "Post X-Progress-ID='%s' already registered\n", tmp.buf);
		goto ok;
	}

	ret = entry_new (&entry);
	if (unlikely (ret != ret_ok)) {
		goto error;
	}

	/* Store the POST information
	 */
	entry->post = &conn->post;

	cherokee_buffer_add_buffer (&entry->progress_id, &tmp);
	cherokee_buffer_add_buffer (&conn->post.progress_id, &tmp);

	ret = cherokee_avl_add (&track->posts_lookup, &tmp, entry);
	if (unlikely (ret != ret_ok)) {
		TRACE (ENTRIES, "Could not register X-Progress-ID='%s'\n", tmp.buf);
		goto error;
	}

	cherokee_list_add (LIST(entry), &track->posts_list);

	/* UNLOCK */
ok:
	cherokee_buffer_mrproper (&tmp);
	CHEROKEE_MUTEX_UNLOCK (&track->lock);
	return ret_ok;

error:
	if (entry) {
		entry_free (entry);
	}
	cherokee_buffer_mrproper (&tmp);
	CHEROKEE_MUTEX_UNLOCK (&track->lock);
	return ret_error;
}


static ret_t
_unregister (cherokee_post_track_t *track,
             cherokee_post_t       *post)
{
	ret_t                        ret;
	cherokee_post_track_entry_t *entry;

	/* Might need to trigger the purging function
	 */
	if (cherokee_bogonow_now > track->last_purge + TIMEOUT) {
		_purge_unreg (track);
		track->last_purge = cherokee_bogonow_now;
	}

	/* Ensure post is registered
	 */
	if (cherokee_buffer_is_empty (&post->progress_id)) {
		return ret_ok;

	}

	/* Simply mark the object as unregistered, it will be removed
	 * later on by a timed event.
	 */
	CHEROKEE_MUTEX_LOCK (&track->lock);

	ret = cherokee_avl_get (&track->posts_lookup, &post->progress_id, (void **)&entry);
	if (ret == ret_ok) {
		entry->unregistered_at = cherokee_bogonow_now;
	}

	CHEROKEE_MUTEX_UNLOCK (&track->lock);
	return ret_ok;
}

ret_t
cherokee_generic_post_track_free (cherokee_post_track_t *track)
{
	if (! track || ! MODULE(track)->free) {
		return ret_ok;
	}

	return MODULE(track)->free (track);
}


ret_t
cherokee_generic_post_track_configure (cherokee_post_track_t  *track,
                                       cherokee_config_node_t *config)
{
	UNUSED (config);
	UNUSED (track);

	return ret_ok;
}


ret_t
cherokee_generic_post_track_new (cherokee_post_track_t **track)
{
	ret_t ret;
	CHEROKEE_NEW_STRUCT(n,post_track);

	/* Methods
	 */
	cherokee_module_init_base (MODULE(n), NULL, PLUGIN_INFO_PTR(post_track));

	MODULE(n)->free    = (module_func_free_t) _free;
	n->func_register   = (post_track_register_t) _register;
	n->func_unregister = (post_track_unregister_t) _unregister;

	/* Properties
	 */
	n->last_purge = cherokee_bogonow_now;

	CHEROKEE_MUTEX_INIT (&n->lock, CHEROKEE_MUTEX_FAST);
	INIT_LIST_HEAD (&n->posts_list);

	ret = cherokee_avl_init (&n->posts_lookup);
	if (unlikely (ret != ret_ok)) {
		return ret;
	}

	*track = n;
	return ret_ok;
}


ret_t
cherokee_generic_post_track_get (cherokee_post_track_t  *track,
                                 cherokee_buffer_t      *progress_id,
                                 const char            **out_state,
                                 off_t                  *out_size,
                                 off_t                  *out_received)
{
	ret_t                        ret;
	cherokee_post_track_entry_t *entry;

	/* Get the reference
	 */
	ret = cherokee_avl_get (&track->posts_lookup, progress_id, (void**)&entry);
	if (ret != ret_ok) {
		*out_state = "Not found";
		return ret_error;
	}

	/* Return the information
	 */
	*out_size     = entry->post->len;
	*out_received = entry->post->send.read;

	if (cherokee_post_read_finished (entry->post)) {
		*out_state = "done";

	} else if ((entry->post->send.read == 0) && (entry->post->chunked.processed == 0)) {
		*out_state = "starting";

	} else {
		*out_state = "uploading";
	}

	return ret_ok;
}
