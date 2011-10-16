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

#include "common-internal.h"
#include "handler_sphinx.h"
#include "connection-protected.h"
#include "thread.h"

PLUGIN_INFO_HANDLER_EASIEST_INIT (sphinx, http_get);

ret_t
cherokee_handler_sphinx_init (cherokee_handler_sphinx_t *hdl)
{
	ret_t                            ret;
	cherokee_connection_t *conn            = HANDLER_CONN(hdl);
	cherokee_handler_sphinx_props_t *props = HANDLER_SPHINX_PROPS(hdl);
	cherokee_buffer_t                *tmp  = &HANDLER_THREAD(hdl)->tmp_buf1;
	cuint_t                           len;
	cuint_t				  limit = props->max_results;
	

	hdl->client = sphinx_create ( SPH_TRUE );
	if (unlikely(!hdl->client))
		return ret_error;

	cherokee_connection_parse_args (conn);

	if (conn->arguments != NULL) {
		cherokee_buffer_t *value;
		cherokee_buffer_t limit_arg = CHEROKEE_BUF_INIT;
		cherokee_buffer_fake_str(&limit_arg, "max");
		ret = cherokee_avl_get (conn->arguments, &limit_arg, (void **)&value);
		if (ret == ret_ok) {
			limit = (int) strtol(value->buf, NULL, 10);
		}
	}

	sphinx_set_limits ( hdl->client, 0, limit, limit, 0);

        if ((cherokee_buffer_is_empty (&conn->web_directory)) ||
            (cherokee_buffer_is_ending (&conn->web_directory, '/')))
        {
                len = conn->web_directory.len;
        } else {
                len = conn->web_directory.len + 1;
        }

        cherokee_buffer_clean (tmp);
        cherokee_buffer_add   (tmp,
                               conn->request.buf + len,
                               conn->request.len - len);


	hdl->res = sphinx_query ( hdl->client, tmp->buf, props->index.buf, NULL );
	if (unlikely(!hdl->res))
		return ret_error;

	return ret_ok;
}


static ret_t
sphinx_add_headers (cherokee_handler_sphinx_t *hdl, 
		      cherokee_buffer_t           *buffer)
{
	switch (HANDLER_SPHINX_PROPS(hdl)->lang) {
	case dwriter_json:
		cherokee_buffer_add_str (buffer, "Content-Type: application/json" CRLF);
		break;
	case dwriter_python:
		cherokee_buffer_add_str (buffer, "Content-Type: application/x-python" CRLF);
		break;
	case dwriter_php:
		cherokee_buffer_add_str (buffer, "Content-Type: application/x-php" CRLF);
		break;
	case dwriter_ruby:
		cherokee_buffer_add_str (buffer, "Content-Type: application/x-ruby" CRLF);
		break;
	default:
		SHOULDNT_HAPPEN;
		return ret_error;
	}

	return ret_ok;
}
	

ret_t
sphinx_step (cherokee_handler_sphinx_t *hdl,
             cherokee_buffer_t         *buffer)
{
	int i;
	cherokee_dwriter_set_buffer (&hdl->writer, buffer);

	/* Open the result list */
	cherokee_dwriter_list_open (&hdl->writer);

	cherokee_dwriter_dict_open (&hdl->writer);
	cherokee_dwriter_cstring (&hdl->writer, "RESULT");
	cherokee_dwriter_list_open (&hdl->writer);
	
	if (hdl->res) {
		for ( i = 0; i < hdl->res->num_matches; i++ ) {
			cherokee_dwriter_integer (&hdl->writer, sphinx_get_id ( hdl->res, i ));
//			cherokee_dwriter_integer (&hdl->writer, sphinx_get_weight ( hdl->res, i ));
		}
	}

	cherokee_dwriter_list_close (&hdl->writer);
	cherokee_dwriter_dict_close (&hdl->writer);

	/* Close results list */
	cherokee_dwriter_list_close (&hdl->writer);

	return ret_eof_have_data;
}


static ret_t
sphinx_free (cherokee_handler_sphinx_t *hdl)
{
	if (hdl->client)
		sphinx_destroy(hdl->client);

	cherokee_dwriter_mrproper (&hdl->writer);

	return ret_ok;
}

ret_t
cherokee_handler_sphinx_new (cherokee_handler_t     **hdl, 
		             void                    *cnt,
			     cherokee_module_props_t *props)
{
	CHEROKEE_NEW_STRUCT (n, handler_sphinx);
	
	/* Init the base class object
	 */
	cherokee_handler_init_base (HANDLER(n), cnt, HANDLER_PROPS(props), PLUGIN_INFO_HANDLER_PTR(sphinx));

	MODULE(n)->init         = (handler_func_init_t) cherokee_handler_sphinx_init;
	MODULE(n)->free         = (module_func_free_t) sphinx_free;
	HANDLER(n)->step        = (handler_func_step_t) sphinx_step;
	HANDLER(n)->add_headers = (handler_func_add_headers_t) sphinx_add_headers; 

	/* Supported features
	 */
	HANDLER(n)->support     = hsupport_nothing;

	/* Data writer */
	cherokee_dwriter_init (&n->writer, &CONN_THREAD(cnt)->tmp_buf1);
	n->writer.lang = PROP_SPHINX(props)->lang;

	*hdl = HANDLER(n);
	return ret_ok;
}


static ret_t 
props_free  (cherokee_handler_sphinx_props_t *props)
{
/*	if (props->balancer)
		cherokee_balancer_free (props->balancer);*/
	
	return ret_ok;
}


ret_t 
cherokee_handler_sphinx_configure (cherokee_config_node_t  *conf, 
				     cherokee_server_t       *srv,
				     cherokee_module_props_t **_props)
{
	ret_t                              ret;
	cherokee_list_t                   *i;
	cherokee_handler_sphinx_props_t *props;
	
	/* Instance a new property object
	 */
	if (*_props == NULL) {
		CHEROKEE_NEW_STRUCT (n, handler_sphinx_props);

		cherokee_handler_props_init_base (HANDLER_PROPS(n), 
						  MODULE_PROPS_FREE(props_free));
/*		n->balancer = NULL;*/

		cherokee_buffer_init (&n->index);
		cherokee_buffer_add_str (&n->index, "*"); /* default value for all indices */
		n->max_results = 20;

		*_props = MODULE_PROPS(n);
	}

	props = PROP_SPHINX(*_props);	

	/* Parse the configuration tree
	 */
	cherokee_config_node_foreach (i, conf) {
		cherokee_config_node_t *subconf = CONFIG_NODE(i);

/*		if (equal_buf_str (&subconf->key, "balancer")) {
			ret = cherokee_balancer_instance (&subconf->val, subconf, srv, &props->balancer); 
			if (ret != ret_ok)
				return ret;

		} else */
		if (equal_buf_str (&subconf->key, "lang")) {

			if (equal_buf_str (&subconf->val, "json")) {
				props->lang = dwriter_json;
			} else if (equal_buf_str (&subconf->val, "python")) {
				props->lang = dwriter_python;
			} else if (equal_buf_str (&subconf->val, "php")) {
				props->lang = dwriter_php;
			} else if (equal_buf_str (&subconf->val, "ruby")) {
				props->lang = dwriter_ruby;

			} else {
				return ret_error;
			}
		} else if (equal_buf_str (&subconf->key, "index")) {
			cherokee_buffer_clean (&props->index);
			cherokee_buffer_add_buffer (&props->index, &subconf->val);
		} else if (equal_buf_str (&subconf->key, "max")) {
			props->max_results = atoi(subconf->val.buf);
		}

	}

	/* Final checks
	 */
/*	if (props->balancer == NULL) {
		return ret_error;
	}*/

	return ret_ok;
}


