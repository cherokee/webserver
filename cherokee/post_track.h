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

#ifndef CHEROKEE_POST_TRACK_H
#define CHEROKEE_POST_TRACK_H

#include "common.h"
#include "post.h"
#include "module.h"
#include "avl.h"
#include "plugin_loader.h"

CHEROKEE_BEGIN_DECLS

typedef ret_t (* post_track_new_t)        (void **track);
typedef ret_t (* post_track_configure_t)  (void *track, cherokee_config_node_t *config);
typedef ret_t (* post_track_register_t)   (void *track, cherokee_connection_t *);
typedef ret_t (* post_track_unregister_t) (void *track, cherokee_post_t *);

typedef struct {
	/* Object */
	cherokee_module_t        module;

	/* Methods */
	post_track_configure_t   func_configure;
	post_track_register_t    func_register;
	post_track_unregister_t  func_unregister;

	/* Concurrency */
	CHEROKEE_MUTEX_T        (lock);

	/* Properties */
	cherokee_avl_t           posts_lookup;
	cherokee_list_t          posts_list;
	time_t                   last_purge;
} cherokee_post_track_t;

#define POST_TRACK(x) ((cherokee_post_track_t *)(x))

void  PLUGIN_INIT_NAME(post_track) (cherokee_plugin_loader_t *loader);

ret_t cherokee_generic_post_track_new       (cherokee_post_track_t **track);
ret_t cherokee_generic_post_track_configure (cherokee_post_track_t  *track,
                                             cherokee_config_node_t *config);

ret_t cherokee_generic_post_track_get       (cherokee_post_track_t  *track,
                                             cherokee_buffer_t      *progress_id,
                                             const char            **out_status,
                                             off_t                  *out_size,
                                             off_t                  *out_received);

CHEROKEE_END_DECLS

#endif /* CHEROKEE_POST_TRACK_H */
