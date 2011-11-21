/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/* Cherokee
 *
 * Authors:
 *      Alvaro Lopez Ortega <alvaro@alobbs.com>
 *      Stefan de Konink <stefan@konink.de>
 *
 * Copyright (C) 2001-2011 Alvaro Lopez Ortega
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

#ifndef CHEROKEE_HANDLER_XSLT_H
#define CHEROKEE_HANDLER_XSLT_H

#include "common-internal.h"
#include "buffer.h"
#include "handler.h"
#include "connection.h"
#include "plugin_loader.h"

#include <libxslt/security.h>

/* Data types
 */
typedef struct {
	/* The foundation of the handler is here */
	cherokee_handler_props_t base;

	/* Our own configuration parameters */
	int                      options;
	xsltSecurityPrefsPtr     sec;
	xmlExternalEntityLoader  defaultEntityLoader;
	xsltStylesheetPtr        global;
	xmlDocPtr                globalDoc;
	cherokee_boolean_t       xinclude;
	cherokee_boolean_t       xincludestyle;
	cherokee_buffer_t        content_type;
	cherokee_buffer_t	 stylesheet;
} cherokee_handler_xslt_props_t;

typedef struct {
	/* Shared structures */
	cherokee_handler_t       handler;

	/* A buffer is your output 'to be' */
	cherokee_buffer_t        buffer;
} cherokee_handler_xslt_t;

/* Easy access methods, ALWAYS implement them for your own handler! */
#define HDL_XSLT(x)       ((cherokee_handler_xslt_t *)(x))
#define PROP_XSLT(x)      ((cherokee_handler_xslt_props_t *)(x))
#define HDL_XSLT_PROPS(x) (PROP_XSLT(MODULE(x)->props))


/* Library init function
 */
void  PLUGIN_INIT_NAME(xslt)    (cherokee_plugin_loader_t *loader);
ret_t cherokee_handler_xslt_new (cherokee_handler_t **hdl, cherokee_connection_t *cnt, cherokee_module_props_t *props);

/* virtual methods implementation
 */
ret_t cherokee_handler_xslt_init        (cherokee_handler_xslt_t *hdl);
ret_t cherokee_handler_xslt_free        (cherokee_handler_xslt_t *hdl);
ret_t cherokee_handler_xslt_step        (cherokee_handler_xslt_t *hdl, cherokee_buffer_t *buffer);
ret_t cherokee_handler_xslt_add_headers (cherokee_handler_xslt_t *hdl, cherokee_buffer_t *buffer);

#endif /* CHEROKEE_HANDLER_XSLT_H */
