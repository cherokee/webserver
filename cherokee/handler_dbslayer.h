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

#ifndef CHEROKEE_HANDLER_DBSLAYER_H
#define CHEROKEE_HANDLER_DBSLAYER_H

#include "handler.h"
#include "dwriter.h"
#include "balancer.h"
#include "plugin_loader.h"
#include "source.h"

#include <mysql.h>

typedef struct {
	cherokee_module_props_t  base;
	cherokee_balancer_t     *balancer;
	cherokee_buffer_t        user;
	cherokee_buffer_t        password;
	cherokee_buffer_t        db;
	cherokee_dwriter_lang_t  lang;
} cherokee_handler_dbslayer_props_t;

typedef struct {
	cherokee_handler_t      base;
	cherokee_dwriter_t      writer;
	cherokee_source_t      *src_ref;
	MYSQL		       *conn;
	cherokee_boolean_t      rollback;
} cherokee_handler_dbslayer_t;

#define HDL_DBSLAYER(x)           ((cherokee_handler_dbslayer_t *)(x))
#define PROP_DBSLAYER(x)          ((cherokee_handler_dbslayer_props_t *)(x))
#define HANDLER_DBSLAYER_PROPS(x) (PROP_DBSLAYER(MODULE(x)->props))

/* Library init function
 */
void PLUGIN_INIT_NAME(dbslayer)      (cherokee_plugin_loader_t *loader);

/* Methods
 */
ret_t cherokee_handler_dbslayer_new  (cherokee_handler_t **hdl, void *cnt, cherokee_module_props_t *props);
ret_t cherokee_handler_dbslayer_free (cherokee_handler_dbslayer_t *hdl);
ret_t cherokee_handler_dbslayer_init (cherokee_handler_dbslayer_t *hdl);

#endif /* CHEROKEE_HANDLER_DBSLAYER_H */
