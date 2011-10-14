/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/* Cherokee
 *
 * Authors:
 *      Alvaro Lopez Ortega <alvaro@alobbs.com>
 *      Stefan de Konink <stefan@konink.de>
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

#ifndef CHEROKEE_HANDLER_TILE_H
#define CHEROKEE_HANDLER_TILE_H

#include "common-internal.h"

#include "handler.h"
#include "buffer.h"
#include "socket.h"
#include "balancer.h"
#include "util.h"
#include "plugin_loader.h"
#include "handler_file.h"

#include <sys/time.h>

/* This we should load from protocol.h */
#define XMLCONFIG_MAX 41
enum protoCmd { cmdIgnore, cmdRender, cmdDirty, cmdDone, cmdNotDone, cmdRenderPrio, cmdRenderBulk };

struct protocol {
    int ver;
    enum protoCmd cmd;
    int x;
    int y;
    int z;
    char xmlname[XMLCONFIG_MAX];
};

/* Data types
 */
typedef struct {
	cherokee_module_props_t        base;
	cherokee_balancer_t           *balancer;
	cherokee_handler_file_props_t *file_props;
	time_t			       expiration_time;
	int			       timeout;
} cherokee_handler_tile_props_t;

typedef struct {
    /* Shared structures */
	cherokee_handler_t       handler;
        cherokee_source_t       *src_ref;
        cherokee_socket_t        socket;
	enum { parsing,
	       sending,
	       senddone,
	       timeout
	} mystatus;
	struct protocol          cmd;
	cherokee_handler_file_t *file_hdl;
} cherokee_handler_tile_t;


#define HDL_TILE(x)       ((cherokee_handler_tile_t *)(x))
#define PROP_TILE(x)      ((cherokee_handler_tile_props_t *)(x))
#define HDL_TILE_PROPS(x) (PROP_TILE(MODULE(x)->props))


/* Library init function
 */
void  PLUGIN_INIT_NAME(tile)      (cherokee_plugin_loader_t *loader);
ret_t cherokee_handler_tile_new   (cherokee_handler_t **hdl, cherokee_connection_t *cnt, cherokee_module_props_t *props);

/* virtual methods implementation
 */
ret_t cherokee_handler_tile_init        (cherokee_handler_tile_t *hdl);
ret_t cherokee_handler_tile_free        (cherokee_handler_tile_t *hdl);
ret_t cherokee_handler_tile_step        (cherokee_handler_tile_t *hdl, cherokee_buffer_t *buffer);
ret_t cherokee_handler_tile_add_headers (cherokee_handler_tile_t *hdl, cherokee_buffer_t *buffer);

#endif /* CHEROKEE_HANDLER_TILE_H */
