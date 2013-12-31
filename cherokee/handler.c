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
#include "handler.h"

#include <stdlib.h>
#include <string.h>

#include "connection.h"


ret_t
cherokee_handler_init_base (cherokee_handler_t             *hdl,
                            void                           *conn,
                            cherokee_handler_props_t       *props,
                            cherokee_plugin_info_handler_t *info)
{
	/* Init the base class
	 */
	cherokee_module_init_base (MODULE(hdl), MODULE_PROPS(props), PLUGIN_INFO(info));

	/* Pure virtual methods
	 */
	hdl->read_post     = NULL;
	hdl->add_headers   = NULL;
	hdl->step          = NULL;

	/* Parent reference
	 */
	hdl->connection    = conn;

	return ret_ok;
}


/* Virtual method hiding layer
 */

ret_t
cherokee_handler_free (cherokee_handler_t *hdl)
{
	/* Sanity check
	 */
	return_if_fail (hdl != NULL, ret_error);

	if (MODULE(hdl)->free == NULL) {
		return ret_error;
	}

	MODULE(hdl)->free (hdl);

	/* Free the handler memory
	 */
	free (hdl);
	return ret_ok;
}


ret_t
cherokee_handler_init (cherokee_handler_t *hdl)
{
	handler_func_init_t init_func;

	/* Sanity check
	 */
	return_if_fail (hdl != NULL, ret_error);

	init_func = (handler_func_init_t) MODULE(hdl)->init;

	if (init_func) {
		return init_func(hdl);
	}

	return ret_error;
}


ret_t
cherokee_handler_read_post (cherokee_handler_t *hdl)
{
	if (unlikely (hdl->read_post == NULL)) {
		return ret_error;
	}

	return hdl->read_post(hdl);
}


ret_t
cherokee_handler_add_headers (cherokee_handler_t *hdl,
                              cherokee_buffer_t  *buffer)
{
	/* Sanity check
	 */
	return_if_fail (hdl != NULL, ret_error);

	if (hdl->add_headers) {
		return hdl->add_headers (hdl, buffer);
	}

	return ret_error;
}


ret_t
cherokee_handler_step (cherokee_handler_t *hdl,
                       cherokee_buffer_t  *buffer)
{
	/* Sanity check
	 */
	return_if_fail (hdl != NULL, ret_error);

	if (hdl->step) {
		return hdl->step (hdl, buffer);
	}

	return ret_error;
}



/* Handler properties methods
 */

ret_t
cherokee_handler_props_init_base (cherokee_handler_props_t *props,
                                  module_func_props_free_t  free_func)
{
	props->valid_methods = http_unknown;

	return cherokee_module_props_init_base (MODULE_PROPS(props), free_func);
}


ret_t
cherokee_handler_props_free_base (cherokee_handler_props_t *props)
{
	return cherokee_module_props_free_base (MODULE_PROPS(props));
}


/* Utilities
 */
ret_t
cherokee_handler_add_header_options (cherokee_handler_t *hdl,
                                     cherokee_buffer_t  *buffer)
{
	ret_t                   ret;
	int                     http_n;
	const char             *method_name;
	cuint_t                 method_name_len;
	cherokee_http_method_t  method;
	cherokee_http_method_t  supported_methods;
	cherokee_boolean_t      first              = true;

	/* Introspect the handler
	 */
	supported_methods = PLUGIN_INFO_HANDLER (MODULE (hdl)->info)->valid_methods;

	/* Build the response string
	 */
	cherokee_buffer_add_str (buffer, "Allow: ");

	for (http_n=0; http_n < cherokee_http_method_LENGTH; http_n++)
	{
		method = HTTP_METHOD (1LL << http_n);

		if (supported_methods & method) {
			method_name     = NULL;
			method_name_len = 0;

			if (! first) {
				cherokee_buffer_add_str (buffer, ", ");
			} else {
				first = false;
			}

			ret = cherokee_http_method_to_string (method, &method_name, &method_name_len);
			if (unlikely ((ret != ret_ok) || (! method_name))) {
				continue;
			}

			cherokee_buffer_add (buffer, method_name, method_name_len);
		}
	}

	cherokee_buffer_add_str (buffer, CRLF);
	return ret_ok;

}
