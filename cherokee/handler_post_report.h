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

#ifndef CHEROKEE_HANDLER_POST_REPORT_H
#define CHEROKEE_HANDLER_POST_REPORT_H

#include "common-internal.h"
#include "buffer.h"
#include "handler.h"
#include "dwriter.h"
#include "plugin_loader.h"
#include "connection.h"


/* Data types
 */
typedef struct {
	cherokee_handler_props_t base;
	cherokee_dwriter_lang_t  lang;
} cherokee_handler_post_report_props_t;

typedef struct {
	cherokee_handler_t       handler;
	cherokee_buffer_t        buffer;
	cherokee_dwriter_t       writer;
} cherokee_handler_post_report_t;


#define HDL_POST_REPORT(x)       ((cherokee_handler_post_report_t *)(x))
#define PROP_POST_REPORT(x)      ((cherokee_handler_post_report_props_t *)(x))
#define HDL_POST_REPORT_PROPS(x) (PROP_POST_REPORT(MODULE(x)->props))

/* Library init function
 */
void  PLUGIN_INIT_NAME(post_report)      (cherokee_plugin_loader_t *loader);
ret_t cherokee_handler_post_report_new   (cherokee_handler_t **hdl, cherokee_connection_t *cnt, cherokee_module_props_t *props);

/* virtual methods implementation
 */
ret_t cherokee_handler_post_report_init        (cherokee_handler_post_report_t *hdl);
ret_t cherokee_handler_post_report_free        (cherokee_handler_post_report_t *hdl);
ret_t cherokee_handler_post_report_step        (cherokee_handler_post_report_t *hdl, cherokee_buffer_t *buffer);
ret_t cherokee_handler_post_report_add_headers (cherokee_handler_post_report_t *hdl, cherokee_buffer_t *buffer);

#endif /* CHEROKEE_HANDLER_POST_REPORT_H */
