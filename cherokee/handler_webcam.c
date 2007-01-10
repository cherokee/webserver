/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/* Cherokee
 *
 * Authors:
 *      Miguel Angel Ajo <ajo@alobbs.com>
 *      Alvaro Lopez Ortega <alvaro@alobbs.com>
 *
 * Copyright (C) 2001-2007 Alvaro Lopez Ortega
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

#include "handler_webcam.h"


ret_t 
cherokee_handler_webcam_new (cherokee_handler_t **hdl, void *cnt, cherokee_table_t *properties)
{
	CHEROKEE_NEW_STRUCT (n, handler_webcam);
	
	/* Init the base class object
	 */
	cherokee_handler_init_base(HANDLER(n), cnt);

	HANDLER(n)->support = hsupport_length;

	MODULE(n)->init         = (handler_func_init_t) cherokee_handler_webcam_init;
	MODULE(n)->free         = (module_func_free_t) cherokee_handler_webcam_free;
	HANDLER(n)->step        = (handler_func_step_t) cherokee_handler_webcam_step;
	HANDLER(n)->add_headers = (handler_func_add_headers_t) cherokee_handler_webcam_add_headers;
	
	/* Init
	 */
	n->jpeg     = NULL;
	n->jpeg_len = 0;

	/* Return the object
	 */
	*hdl = HANDLER(n);
	return ret_ok;
}


static ret_t
grab_image (cherokee_handler_webcam_t *hdl)
{
	   /* TODO
	    */
	   return ret_ok;
}


ret_t 
cherokee_handler_webcam_init (cherokee_handler_webcam_t *hdl)
{
	   ret_t ret;

	   ret = grab_image (hdl);
	   if (ret < ret_ok) return ret;

	   return ret_ok;
}


ret_t 
cherokee_handler_webcam_free (cherokee_handler_webcam_t *hdl)
{
	   if (hdl->jpeg != NULL) {
			 free (hdl->jpeg);
	   }

	   return ret_ok;
}


ret_t 
cherokee_handler_webcam_add_headers (cherokee_handler_webcam_t *hdl, cherokee_buffer_t *buffer)
{
	   cherokee_buffer_add_str (buffer, "Content-Type: image/jpeg"CRLF);
	   return ret_ok;
}


ret_t 
cherokee_handler_webcam_step (cherokee_handler_webcam_t *hdl, cherokee_buffer_t *buffer)
{
	   ret_t ret;

	   ret = cherokee_buffer_add (buffer, hdl->jpeg, hdl->jpeg_len);
	   if (ret < ret_ok) return ret;

	   return ret_eof_have_data;	   
}


/* Library init
 */
static cherokee_boolean_t _is_init = false;

void 
webcam_init (cherokee_module_loader_t *loader)
{
	   if (_is_init) return;
	   _is_init = true;
}
