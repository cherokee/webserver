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
#include "encoder.h"
#include "util.h"

#define ENTRIES "encoder"

/* Properties
 */
ret_t
cherokee_encoder_props_init_base (cherokee_encoder_props_t *props,
                                  module_func_props_free_t  free_func)
{
	props->perms         = cherokee_encoder_unset;
	props->instance_func = NULL;

	return cherokee_module_props_init_base (MODULE_PROPS(props), free_func);
}

ret_t
cherokee_encoder_props_free_base (cherokee_encoder_props_t *props)
{
	return cherokee_module_props_free_base (MODULE_PROPS(props));
}

ret_t
cherokee_encoder_configure (cherokee_config_node_t   *config,
                            cherokee_server_t        *srv,
                            cherokee_module_props_t **_props)
{
	cherokee_encoder_props_t *props = ENCODER_PROPS(*_props);

	UNUSED (srv);

	/* Unset
	 */
	if (equal_buf_str (&config->val, "0")) {
		TRACE (ENTRIES, "Encoder %s: unset\n", config->key.buf);

	/* Deny
	 */
	} else if (equal_buf_str (&config->val, "forbid") ||
		   equal_buf_str (&config->val, "deny")) {
		TRACE (ENTRIES, "Encoder %s: deny\n", config->key.buf);
		props->perms = cherokee_encoder_forbid;

	/* Allow
	 */
	} else if (equal_buf_str (&config->val, "1") ||
		   equal_buf_str (&config->val, "allow")) {
		TRACE (ENTRIES, "Encoder %s: allow\n", config->key.buf);
		props->perms = cherokee_encoder_allow;

	/* Error
	 */
	} else {
		LOG_ERROR (CHEROKEE_ERROR_ENCODER_NOT_SET_VALUE, config->val.buf);
		return ret_error;
	}

	return ret_ok;
}


/* Encoder
 */
ret_t
cherokee_encoder_init_base (cherokee_encoder_t       *enc,
                            cherokee_plugin_info_t   *info,
                            cherokee_encoder_props_t *props)
{
	cherokee_module_init_base (MODULE(enc), NULL, info);

	MODULE(enc)->props = MODULE_PROPS(props);
	enc->encode        = NULL;
	enc->add_headers   = NULL;
	enc->flush         = NULL;

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

	return init_func (enc, conn);
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
