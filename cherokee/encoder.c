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
#include "encoder.h"


ret_t 
cherokee_encoder_init_base (cherokee_encoder_t *enc, cherokee_plugin_info_t *info)
{
	cherokee_module_init_base (MODULE(enc), NULL, info);

	enc->encode      = NULL;
	enc->add_headers = NULL;
	enc->flush       = NULL;

	return ret_ok;
}


ret_t 
cherokee_encoder_free (cherokee_encoder_t *enc)
{
	ret_t ret;

	if (MODULE(enc)->free == NULL) 
		return ret_error;

	ret = MODULE(enc)->free (enc);
	return ret;
}


ret_t
cherokee_encoder_add_headers (cherokee_encoder_t *enc, cherokee_buffer_t *buf)
{
	if (unlikely (enc->add_headers == NULL))
		return ret_error;

	return enc->add_headers (enc, buf);
}


ret_t 
cherokee_encoder_init (cherokee_encoder_t *enc, void *conn)
{
	encoder_func_init_t init_func;
	
	/* Properties
	 */
	enc->conn = conn;

	/* Call the virtual method
	 */
	init_func = (encoder_func_init_t) MODULE(enc)->init;		
	if (init_func == NULL)
		return ret_error;

	return init_func (enc);
}


ret_t 
cherokee_encoder_encode (cherokee_encoder_t *enc, 
			 cherokee_buffer_t  *in, 
			 cherokee_buffer_t  *out)
{
	if (unlikely (enc->encode == NULL))
		return ret_error;

	return enc->encode (enc, in, out);
}


ret_t 
cherokee_encoder_flush (cherokee_encoder_t *enc,
			cherokee_buffer_t  *in, 
			cherokee_buffer_t  *out)
{
	if (unlikely (enc->flush == NULL))
		return ret_error;

	return enc->flush (enc, in, out);
}
