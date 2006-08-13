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
#include "handler_remote_control.h"

#include "connection.h"
#include "connection-protected.h"
#include "util.h"
#include "server.h"
#include "module_loader.h"

cherokee_module_info_t MODULE_INFO(remote_control) = {
	cherokee_handler,                   /* type     */
	cherokee_handler_remote_control_new /* new func */
};

ret_t 
cherokee_handler_remote_control_new (cherokee_handler_t **hdl, cherokee_connection_t *cnt, cherokee_table_t *properties)
{
	CHEROKEE_NEW_STRUCT (n, handler_remote_control);
	
	/* Init the base class object
	 */
	cherokee_handler_init_base(HANDLER(n), cnt);

	MODULE(n)->init         = (handler_func_init_t) cherokee_handler_remote_control_init;
	MODULE(n)->free         = (handler_func_free_t) cherokee_handler_remote_control_free;
	HANDLER(n)->step        = (handler_func_step_t) cherokee_handler_remote_control_step;
	HANDLER(n)->add_headers = (handler_func_add_headers_t) cherokee_handler_remote_control_add_headers;

	HANDLER(n)->support = hsupport_length | hsupport_range;
	
	/* Init
	 */
	cherokee_buffer_new (&n->buffer);

	*hdl = HANDLER(n);
	return ret_ok;
}


ret_t 
cherokee_handler_remote_control_free (cherokee_handler_remote_control_t *hdl)
{
	cherokee_buffer_free (hdl->buffer);
	return ret_ok;
}


ret_t 
cherokee_handler_remote_control_init (cherokee_handler_remote_control_t *hdl)
{
	ret_t  ret;
	void  *param;

	cherokee_connection_parse_args (HANDLER_CONN(hdl));

	cherokee_buffer_add_str (hdl->buffer, "<?xml version=\"1.0\"?>"CRLF); 
	cherokee_buffer_add_str (hdl->buffer, "<status>"CRLF); 
	
	/* Connections
	 */
	ret = cherokee_table_get (HANDLER_CONN(hdl)->arguments, "active", &param);
	if (ret == ret_ok) {
		int active;
		
		cherokee_server_get_active_conns (HANDLER_SRV(hdl), &active);
		cherokee_buffer_add_va (hdl->buffer, "<active>%d</active>"CRLF, active);
	}

	ret = cherokee_table_get (HANDLER_CONN(hdl)->arguments, "reusable", &param);
	if (ret == ret_ok) {
		int reusable;
		
		cherokee_server_get_reusable_conns (HANDLER_SRV(hdl), &reusable);
		cherokee_buffer_add_va (hdl->buffer, "<reusable>%d</reusable>"CRLF, reusable);
	}

	/* Traffic
	 */
	ret = cherokee_table_get (HANDLER_CONN(hdl)->arguments, "rx", &param);
	if (ret == ret_ok) {
		size_t rx, tx;
		char tmp[5];

		cherokee_server_get_total_traffic (HANDLER_SRV(hdl), &rx, &tx);
		cherokee_strfsize (rx, tmp);
		cherokee_buffer_add_va (hdl->buffer, "<rx>%s</rx>"CRLF, tmp);
	}

	ret = cherokee_table_get (HANDLER_CONN(hdl)->arguments, "tx", &param);
	if (ret == ret_ok) {
		size_t rx, tx;
		char tmp[5];

		cherokee_server_get_total_traffic (HANDLER_SRV(hdl), &rx, &tx);
		cherokee_strfsize (tx, tmp);
		cherokee_buffer_add_va (hdl->buffer, "<tx>%s</tx>"CRLF, tmp);
	}



	cherokee_buffer_add_str (hdl->buffer, "</status>"CRLF); 

	return ret_ok;
}


ret_t 
cherokee_handler_remote_control_step (cherokee_handler_remote_control_t *hdl, cherokee_buffer_t *buffer)
{
	cherokee_buffer_add_buffer (buffer, hdl->buffer);
	return ret_eof_have_data;
}


ret_t 
cherokee_handler_remote_control_add_headers (cherokee_handler_remote_control_t *hdl, cherokee_buffer_t *buffer)
{
	cherokee_buffer_add_str(buffer, "Content-Type: text/html"CRLF);
	cherokee_buffer_add_va (buffer, "Content-Length: %d"CRLF, hdl->buffer->len);

	cherokee_buffer_add_str(buffer, "Cache-Control: no-cache"CRLF);
	cherokee_buffer_add_str(buffer, "Pragma: no-cache"CRLF);

	return ret_ok;
}



/*   Library init function
 */
static cherokee_boolean_t _remote_control_is_init = false;

void
MODULE_INIT(remote_control) (cherokee_module_loader_t *loader)
{
	/* Init flag
	 */
	if (_remote_control_is_init) return;
	_remote_control_is_init = true;
}
