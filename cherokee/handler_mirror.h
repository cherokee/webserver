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

#ifndef CHEROKEE_HANDLER_MIRROR_H
#define CHEROKEE_HANDLER_MIRROR_H

#include "common-internal.h"
#include "balancer.h"
#include "plugin_loader.h"
#include "list.h"
#include "util.h"

typedef enum {
	hmirror_phase_connect,
	hmirror_phase_send_headers,
	hmirror_phase_send_post
} cherokee_handler_mirror_phase_t;

typedef struct {
	cherokee_module_props_t  base;
	cherokee_balancer_t     *balancer;
} cherokee_handler_mirror_props_t;

typedef struct {
	cherokee_handler_t              base;
	cherokee_socket_t               socket;
	cherokee_source_t              *src_ref;

	cherokee_handler_mirror_phase_t phase;
	off_t                           header_sent;
	off_t                           post_sent;
	off_t                           post_len;
} cherokee_handler_mirror_t;

#define HDL_MIRROR(x)       ((cherokee_handler_mirror_t *)(x))
#define PROP_MIRROR(x)      ((cherokee_handler_mirror_props_t *)(x))
#define HDL_MIRROR_PROPS(x) (PROP_MIRROR(MODULE(x)->props))

 
/* Library init function
 */
void  PLUGIN_INIT_NAME(mirror)     (cherokee_plugin_loader_t *loader);

/* Methods
 */
ret_t cherokee_handler_mirror_new         (cherokee_handler_t       **hdl, void *cnt, cherokee_module_props_t *props);
ret_t cherokee_handler_mirror_free        (cherokee_handler_mirror_t *hdl);
ret_t cherokee_handler_mirror_init        (cherokee_handler_mirror_t *hdl);
ret_t cherokee_handler_mirror_step        (cherokee_handler_mirror_t *hdl, cherokee_buffer_t *buffer);
ret_t cherokee_handler_mirror_add_headers (cherokee_handler_mirror_t *hdl, cherokee_buffer_t *buffer);

#endif /* CHEROKEE_HANDLER_MIRROR_H */
