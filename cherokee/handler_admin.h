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

#ifndef CHEROKEE_ADMIN_HANDLER_H
#define CHEROKEE_ADMIN_HANDLER_H

#include "handler.h"
#include "plugin_loader.h"


typedef enum {
	state_valid,
	state_invalid_request
} cherokee_handler_admin_state_t;

typedef struct {
	cherokee_handler_t             handler;
	cherokee_buffer_t              reply;
} cherokee_handler_admin_t;

#define PROP_ADMIN(x)      ((cherokee_handler_admin_props_t *)(x)) 
#define HDL_ADMIN(x)       ((cherokee_handler_admin_t *)(x))
#define HDL_ADMIN_PROPS(x) (PROP_ADMIN(HANDLER(x)->props))


/* Library init function
 */
void PLUGIN_INIT_NAME(admi) (cherokee_plugin_loader_t *loader);
ret_t cherokee_handler_admin_new (cherokee_handler_t **hdl, void *cnt, cherokee_module_props_t *props);

/* virtual methods implementation
 */
ret_t cherokee_handler_admin_init        (cherokee_handler_admin_t *ahdl);
ret_t cherokee_handler_admin_free        (cherokee_handler_admin_t *ahdl);
void  cherokee_handler_admin_get_name    (cherokee_handler_admin_t *ahdl, const char **name);
ret_t cherokee_handler_admin_step        (cherokee_handler_admin_t *ahdl, cherokee_buffer_t *buffer);
ret_t cherokee_handler_admin_add_headers (cherokee_handler_admin_t *ahdl, cherokee_buffer_t *buffer);


#endif /* CHEROKEE_ADMIN_HANDLER_H */
