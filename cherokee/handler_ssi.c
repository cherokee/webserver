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
#include "handler_ssi.h"

#include <sys/stat.h>

#include "util.h"
#include "server-protected.h"
#include "connection-protected.h"

#define ENTRIES "handler,ssi"


/* Plug-in initialization
 */
PLUGIN_INFO_HANDLER_EASIEST_INIT (ssi, http_get | http_head);


ret_t
cherokee_handler_ssi_new (cherokee_handler_t     **hdl,
			  cherokee_connection_t   *cnt,
			  cherokee_module_props_t *props)
{
	CHEROKEE_NEW_STRUCT (n, handler_ssi);

	/* Init the base class object
	 */
	cherokee_handler_init_base (HANDLER(n), cnt, HANDLER_PROPS(props), PLUGIN_INFO_HANDLER_PTR(ssi));

	MODULE(n)->free         = (module_func_free_t) cherokee_handler_ssi_free;
	MODULE(n)->init         = (handler_func_init_t) cherokee_handler_ssi_init;
	HANDLER(n)->step        = (handler_func_step_t) cherokee_handler_ssi_step;
	HANDLER(n)->add_headers = (handler_func_add_headers_t) cherokee_handler_ssi_add_headers;

	/* Supported features
	 */
	HANDLER(n)->support     = hsupport_length;

	/* Init
	 */
	n->mime = NULL;
	cherokee_buffer_init (&n->dir);
	cherokee_buffer_init (&n->source);
	cherokee_buffer_init (&n->render);

	/* Return the object
	 */
	*hdl = HANDLER(n);
	return ret_ok;
}


ret_t
cherokee_handler_ssi_configure (cherokee_config_node_t   *conf,
				cherokee_server_t        *srv,
				cherokee_module_props_t **_props)
{
	return ret_ok;
}


ret_t
cherokee_handler_ssi_props_free (cherokee_handler_ssi_props_t *props)
{
	return cherokee_handler_props_free_base (HANDLER_PROPS(props));
}


static ret_t
get_pair (cherokee_buffer_t *key,
	  cherokee_buffer_t *pair)
{
	char *i = key->buf;
	
	while ((*i != ' ') && *i)
		i++;

	cherokee_buffer_add (pair, key->buf, i - key->buf);
	return ret_ok;
}

static ret_t
get_val (char              *begin,
	 cherokee_buffer_t *val)
{
	char    *j;
	char    *i = begin;

	/* Skip whites
	 */
	while (*i == ' ')
		i++;

	/* Read value
	 */
	if (*i == '"') {
		/* The value is quoted */
		i++;
		j = i;
		while ((*j != '"') && *j)
			j++;
	} else {
		/* The value is not quoted */
		j = i;
		while ((*j != ' ') && *j)
			j++;
	}

	cherokee_buffer_add (val, i, j-i);
	return ret_ok;
}


static ret_t
parse (cherokee_handler_ssi_t *hdl,
       cherokee_buffer_t      *in,
       cherokee_buffer_t      *out)
{
	char              *p, *q;
	char              *begin;
	cuint_t            len;
	cherokee_buffer_t  key  = CHEROKEE_BUF_INIT;
	cherokee_buffer_t  val  = CHEROKEE_BUF_INIT;
	cherokee_buffer_t  pair = CHEROKEE_BUF_INIT;

	q = in->buf;

	while (true) {
		begin = q;

		/* Check the end 
		 */
		if (q >= in->buf + in->len)
			break;

		/* Find next SSI tag
		 */
		p = strstr (q, "<!--#");
		if (p == NULL)
			return ret_ok;

		q = strstr (p + 5, "-->");
		if (q == NULL)
			return ret_error;
		
		len = q - p;
		len -= 5;

		cherokee_buffer_clean (&key);
		cherokee_buffer_add (&key, p+5, len);
		cherokee_buffer_trim (&key);

		q += 3;
		TRACE(ENTRIES, "Found key '%s'", key.buf);

		/* Add the previous chunk
		 */
		cherokee_buffer_add (out, begin, p - begin);

		/* Check element
		 */
		if (strncmp (key.buf, "include", 7) == 0) {
			cherokee_buffer_move_to_begin (&key, 7);
			cherokee_buffer_trim (&key);

			cherokee_buffer_clean (&pair);
			get_pair (&key, &pair);
			
			cherokee_buffer_drop_ending (&key, pair.len);
			cherokee_buffer_trim (&key);

			if (strncmp (pair.buf, "file=", 5) == 0) {
				cherokee_buffer_clean (&val);
				get_val (pair.buf + 5, &val);
			  
				/* Build the path */
				cherokee_buffer_add_char   (&hdl->dir, '/');
				cherokee_buffer_add_buffer (&hdl->dir, &val);

				/* Read the file */
				TRACE(ENTRIES, "Including file '%s'\n", hdl->dir.buf);
				cherokee_buffer_read_file (out, hdl->dir.buf);

				/* Clean up */
				cherokee_buffer_drop_ending (&hdl->dir, val.len+1);

			} else {
				PRINT_MSG ("Unknown SSI include property: '%s'\n", pair.buf);
			}
		} else {
			PRINT_MSG ("Unknown SSI tag: '%s'\n", key.buf);
		}
	}

	return ret_ok;
}


static ret_t
init (cherokee_handler_ssi_t *hdl,
      cherokee_buffer_t      *local_path)
{
	int                    re;
	ret_t                  ret;
	cherokee_connection_t *conn = HANDLER_CONN(hdl);

	/* Stat the file
	 */
	re = stat (local_path->buf, &hdl->cache_info);
	if (re < 0) {
		switch (errno) {
		case ENOENT: 
			conn->error_code = http_not_found;
			break;
		case EACCES: 
			conn->error_code = http_access_denied;
			break;
		default:     
			conn->error_code = http_internal_error;
		}
		return ret_error;
	}

	/* Read the file
	 */
	ret = cherokee_buffer_read_file (&hdl->source, local_path->buf);
	if (ret != ret_ok)
		return ret_error;

	/* Render
	 */
	ret = parse (hdl, &hdl->source, &hdl->render);
	if (ret != ret_ok)
		return ret;

	return ret_ok;
}


ret_t
cherokee_handler_ssi_init (cherokee_handler_ssi_t *hdl)
{
	ret_t                  ret;
	cherokee_connection_t *conn = HANDLER_CONN(hdl);

	/* Build the local directory
	 */
	cherokee_buffer_add_buffer (&hdl->dir, &conn->local_directory);

	/* Real init function
	 */
	cherokee_buffer_add_buffer (&conn->local_directory, &conn->request);
	ret = init (hdl, &conn->local_directory);
	cherokee_buffer_drop_ending (&conn->local_directory, conn->request.len);	

	return ret;
}


ret_t
cherokee_handler_ssi_free (cherokee_handler_ssi_t *hdl)
{
	cherokee_buffer_mrproper (&hdl->dir);
	cherokee_buffer_mrproper (&hdl->source);
	cherokee_buffer_mrproper (&hdl->render);
	return ret_ok;
}


ret_t
cherokee_handler_ssi_step (cherokee_handler_ssi_t *hdl,
			   cherokee_buffer_t      *buffer)
{
	cherokee_buffer_add_buffer (buffer, &hdl->render);
	return ret_eof_have_data;
}


ret_t
cherokee_handler_ssi_add_headers (cherokee_handler_ssi_t *hdl,
				  cherokee_buffer_t      *buffer)
{
	char                  *ext;
	cherokee_buffer_t     *mime = NULL;
	cherokee_connection_t *conn = HANDLER_CONN(hdl);;
	cherokee_server_t     *srv  = HANDLER_SRV(hdl);

	/* MIME type
	 */
	if (srv->mime != NULL) {
		ext = strrchr (conn->request.buf, '.');
		if (ext == NULL)
			return ret_ok;

		cherokee_mime_get_by_suffix (srv->mime, ext+1, &hdl->mime);

		cherokee_mime_entry_get_type (hdl->mime, &mime);
		cherokee_buffer_add_str    (buffer, "Content-Type: ");
		cherokee_buffer_add_buffer (buffer, mime);
		cherokee_buffer_add_str    (buffer, CRLF);
	}

	/* Length
	 */
	cherokee_buffer_add_str     (buffer, "Content-Length: ");
	cherokee_buffer_add_ullong10(buffer, (cullong_t) hdl->render.len);
	cherokee_buffer_add_str     (buffer, CRLF);

	return ret_ok;
}
