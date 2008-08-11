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

#include "common-internal.h"
#include "handler.h"

#include <stdlib.h>
#include <string.h>

#include "connection.h"


ret_t
cherokee_handler_init_base (cherokee_handler_t *hdl, void *conn, cherokee_handler_props_t *props, cherokee_plugin_info_handler_t *info)
{
	/* Init the base class
	 */
	cherokee_module_init_base (MODULE(hdl), MODULE_PROPS(props), PLUGIN_INFO(info));

	/* Pure virtual methods
	 */
	hdl->step          = NULL;
	hdl->add_headers   = NULL;

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
cherokee_handler_add_headers (cherokee_handler_t *hdl, cherokee_buffer_t *buffer)
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
cherokee_handler_step (cherokee_handler_t *hdl, cherokee_buffer_t *buffer)
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
cherokee_handler_props_init_base (cherokee_handler_props_t *props, module_func_props_free_t free_func)
{
	props->valid_methods = http_unknown;

	return cherokee_module_props_init_base (MODULE_PROPS(props), free_func);
}


ret_t 
cherokee_handler_props_free_base (cherokee_handler_props_t *props)
{
	return cherokee_module_props_free_base (MODULE_PROPS(props));
}
