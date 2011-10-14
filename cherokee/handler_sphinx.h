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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 */

#ifndef CHEROKEE_HANDLER_SPHINX_H
#define CHEROKEE_HANDLER_SPHINX_H

#include "handler.h"
#include "dwriter.h"
#include "balancer.h"
#include "plugin_loader.h"
#include "source.h"

#include "sphinxclient.h"

typedef ret_t (* dbs_func_t) (void  *conn);
typedef ret_t (* dbs_func_query_t) (void  *conn, cherokee_buffer_t *buf);

typedef struct {
	cherokee_module_props_t  base;
//	cherokee_balancer_t     *balancer;
	cherokee_dwriter_lang_t  lang;
	cherokee_buffer_t	 index;
	cuint_t                  max_results;
} cherokee_handler_sphinx_props_t;

typedef struct {
	cherokee_handler_t      base;
	cherokee_dwriter_t      writer;
//	cherokee_source_t      *src_ref;
	sphinx_client          *client;
	sphinx_result          *res;
} cherokee_handler_sphinx_t;

#define HDL_SPHINX(x)           ((cherokee_handler_sphinx_t *)(x))
#define PROP_SPHINX(x)          ((cherokee_handler_sphinx_props_t *)(x))
#define HANDLER_SPHINX_PROPS(x) (PROP_SPHINX(MODULE(x)->props))

#define add_cstr_str(w,key,val,len)                                     \
        do {                                                            \
                cherokee_dwriter_string (w, key, sizeof(key)-1);        \
                cherokee_dwriter_string (w, val, len);                  \
        } while (false)

#define add_cstr_int(w,key,val)                                   \
        do {                                                      \
                cherokee_dwriter_string  (w, key, sizeof(key)-1); \
                cherokee_dwriter_integer (w, val);                \
        } while (false)


/* Library init function
 */
void PLUGIN_INIT_NAME(sphinx)      (cherokee_plugin_loader_t *loader);

/* Methods
 */
ret_t cherokee_handler_sphinx_new  (cherokee_handler_t **hdl, void *cnt, cherokee_module_props_t *props);
ret_t cherokee_handler_sphinx_free (cherokee_handler_sphinx_t *hdl);
ret_t cherokee_handler_sphinx_init (cherokee_handler_sphinx_t *hdl);

#endif /* CHEROKEE_HANDLER_SPHINX_H */
