/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/* Cherokee
 *
 * Authors:
 *      Alvaro Lopez Ortega <alvaro@alobbs.com>
 *      Stefan de Konink <stefan@konink.de>
 *
 * Copyright (C) 2001-2013 Alvaro Lopez Ortega
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

#ifndef CHEROKEE_HANDLER_ZEROMQ_H
#define CHEROKEE_HANDLER_ZEROMQ_H

#include "common-internal.h"
#include "handler.h"
#include "encoder.h"
#include "plugin_loader.h"
#include "connection.h"
#include "server-protected.h"

#include "zmq-shim.h"

typedef struct {
	cherokee_module_props_t   base;
	cherokee_buffer_t         endpoint;
	cuint_t                   io_threads;
	void *                    context;
	void                     *socket;
	CHEROKEE_MUTEX_T         (mutex);
	cherokee_encoder_props_t *encoder_props;
} cherokee_handler_zeromq_props_t;

typedef struct {
	cherokee_handler_t        base;
	cherokee_encoder_t       *encoder;
	cherokee_buffer_t         output;
	cherokee_buffer_t         encoded;
} cherokee_handler_zeromq_t;

#define HDL_ZEROMQ(x)           ((cherokee_handler_zeromq_t *)(x))
#define PROP_ZEROMQ(x)          ((cherokee_handler_zeromq_props_t *)(x))
#define HANDLER_ZEROMQ_PROPS(x) (PROP_ZEROMQ(MODULE(x)->props))

/* Library init function
 */
void PLUGIN_INIT_NAME(zeromq)      (cherokee_plugin_loader_t *loader);

/* Methods
 */
ret_t cherokee_handler_zeromq_new  (cherokee_handler_t **hdl, void *cnt, cherokee_module_props_t *props);
ret_t cherokee_handler_zeromq_free (cherokee_handler_zeromq_t *hdl);
ret_t cherokee_handler_zeromq_init (cherokee_handler_zeromq_t *hdl);

#endif /* CHEROKEE_HANDLER_ZEROMQ_H */
