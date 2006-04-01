/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/* Cherokee
 *
 * Authors:
 *      Alvaro Lopez Ortega <alvaro@alobbs.com>
 *
 * Copyright (C) 2001-2006 Alvaro Lopez Ortega
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
cherokee_handler_init_base (cherokee_handler_t *hdl, void *conn)
{
	/* Init the base class
	 */
	cherokee_module_init_base (MODULE(hdl));

	/* Pure virtual methods
	 */
	hdl->step          = NULL;
	hdl->add_headers   = NULL;

	/* Parent reference
	 */
	hdl->connection = conn;
	return ret_ok;
}


ret_t 
cherokee_handler_free_base (cherokee_handler_t *hdl)
{
	free (hdl);
	return ret_ok;
}


/* 	Virtual method hidding layer
 */
ret_t
cherokee_handler_free (cherokee_handler_t *hdl)
{
	ret_t ret;

	/* Sanity check
	 */
	return_if_fail (hdl != NULL, ret_error);

	if (MODULE(hdl)->free == NULL) {
		return ret_error;
	}

	/* Call the destructor virtual method
	 */
	ret = MODULE(hdl)->free (hdl); 
	if (unlikely(ret < ret_ok)) return ret;
	
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



