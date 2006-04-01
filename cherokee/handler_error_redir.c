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
#include "handler_error_redir.h"

#include "connection.h"
#include "connection-protected.h"
#include "handler_redir.h"
#include "module_loader.h"


cherokee_module_info_handler_t MODULE_INFO(error_redir) = {
	.module.type     = cherokee_handler,                 /* type         */
	.module.new_func = cherokee_handler_error_redir_new, /* new func     */
	.valid_methods   = http_all_methods                  /* http methods */
};


ret_t 
cherokee_handler_error_redir_new (cherokee_handler_t **hdl, cherokee_connection_t *cnt, cherokee_table_t *properties)
{
	ret_t  ret;
	char  *dir = NULL;
	char   code[4];

	if (properties == NULL) {
		return ret_not_found;
	}

	snprintf (code, 4, "%d", cnt->error_code);
	   
	ret = cherokee_typed_table_get_str (properties, code, &dir);
	if (ret != ret_ok) return ret_error;

	cherokee_buffer_add (&cnt->redirect, dir, strlen(dir));
	cnt->error_code = http_moved_permanently;

	return cherokee_handler_redir_new (hdl, cnt, properties);
}


/* Library init function
 */
static cherokee_boolean_t _error_redir_is_init = false;

void
MODULE_INIT(error_redir) (cherokee_module_loader_t *loader)
{
	/* Is init?
	 */
	if (_error_redir_is_init) return;
	_error_redir_is_init = true;
	   
	/* Load the dependences
	 */
	cherokee_module_loader_load (loader, "redir");
}
