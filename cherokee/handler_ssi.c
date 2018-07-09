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
#include "handler_ssi.h"

#include <sys/stat.h>

#include "util.h"
#include "server-protected.h"
#include "connection-protected.h"
#include "virtual_server.h"

#define ENTRIES "handler,ssi"

/* Plug-in initialization
 */
PLUGIN_INFO_HANDLER_EASIEST_INIT (ssi, http_get | http_head);


typedef enum {
	op_none,
	op_include,
	op_size,
	op_lastmod
} operations_t;

typedef enum {
	path_none,
	path_file,
	path_virtual
} path_type_t;


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
	HANDLER(n)->support     = hsupport_nothing;

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


static ret_t
props_free (cherokee_handler_ssi_props_t *props)
{
	return cherokee_handler_props_free_base (HANDLER_PROPS(props));
}

ret_t
cherokee_handler_ssi_configure (cherokee_config_node_t   *conf,
                                cherokee_server_t        *srv,
                                cherokee_module_props_t **_props)
{
	cherokee_handler_ssi_props_t *props;

	UNUSED(srv);
	UNUSED(conf);
	UNUSED(props);

	if (*_props == NULL) {
		CHEROKEE_NEW_STRUCT (n, handler_ssi_props);

		cherokee_module_props_init_base (MODULE_PROPS(n),
		                                 MODULE_PROPS_FREE(props_free));
		n->foo = 1;
		*_props = MODULE_PROPS(n);
	}

	props = PROP_SSI(*_props);
	return ret_ok;
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
	ret_t              ret;
	char              *p, *q;
	char              *begin;
	int                re;
	cuint_t            len;
	operations_t       op;
	path_type_t        path;
	struct stat        info;
	cherokee_boolean_t ignore;
	cherokee_buffer_t  key     = CHEROKEE_BUF_INIT;
	cherokee_buffer_t  val     = CHEROKEE_BUF_INIT;
	cherokee_buffer_t  pair    = CHEROKEE_BUF_INIT;
	cherokee_buffer_t  fpath   = CHEROKEE_BUF_INIT;

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
		if (p == NULL) {
			cherokee_buffer_add (out, begin, (in->buf + in->len) - begin);
			ret = ret_ok;
			goto out;
		}

		q = strstr (p + 5, "-->");
		if (q == NULL) {
			ret = ret_error;
			goto out;
		}

		len = q - p;
		len -= 5;

		cherokee_buffer_clean (&key);
		cherokee_buffer_add (&key, p+5, len);
		cherokee_buffer_trim (&key);

		q += 3;
		TRACE(ENTRIES, "Found key '%s'\n", key.buf);

		/* Add the previous chunk
		 */
		cherokee_buffer_add (out, begin, p - begin);

		/* Check element
		 */
		op     = op_none;
		ignore = false;

		if (strncmp (key.buf, "include", 7) == 0) {
			op  = op_include;
			len = 7;
		} else if (strncmp (key.buf, "fsize", 5) == 0) {
			op  = op_size;
			len = 5;
		} else if (strncmp (key.buf, "flastmod", 8) == 0) {
			op  = op_lastmod;
			len = 8;
		} else {
			LOG_ERROR (CHEROKEE_ERROR_HANDLER_SSI_PROPERTY, key.buf);
		}

		/* Deeper parsing
		 */
		path = path_none;

		switch (op) {
		case op_size:
		case op_include:
		case op_lastmod:
			/* Read a property key
			 */
			cherokee_buffer_move_to_begin (&key, len);
			cherokee_buffer_trim (&key);

			cherokee_buffer_clean (&pair);
			get_pair (&key, &pair);

			cherokee_buffer_drop_ending (&key, pair.len);
			cherokee_buffer_trim (&key);

			/* Parse the property
			 */
			if (strncmp (pair.buf, "file=", 5) == 0) {
				path = path_file;
				len  = 5;
			} else if (strncmp (pair.buf, "virtual=", 8) == 0) {
				path = path_virtual;
				len  = 8;
			}

			cherokee_buffer_clean (&val);
			get_val (pair.buf + len, &val);

			cherokee_buffer_clean (&fpath);

			switch (path) {
			case path_file:
				cherokee_buffer_add_buffer (&fpath, &hdl->dir);
				cherokee_buffer_add_char   (&fpath, '/');
				cherokee_buffer_add_buffer (&fpath, &val);

				TRACE(ENTRIES, "Path: file '%s'\n", fpath.buf);
				break;
			case path_virtual:
				cherokee_buffer_add_buffer (&fpath, &HANDLER_VSRV(hdl)->root);
				cherokee_buffer_add_char   (&fpath, '/');
				cherokee_buffer_add_buffer (&fpath, &val);

				TRACE(ENTRIES, "Path: virtual '%s'\n", fpath.buf);
				break;
			default:
				ignore = true;
				SHOULDNT_HAPPEN;
			}

			/* Path security check: ensure that the file
			 * to include is inside the document root.
			 */
			if (! cherokee_buffer_is_empty (&fpath)) {
				cherokee_path_short (&fpath);

				if (fpath.len < HANDLER_VSRV(hdl)->root.len) {
					ignore = true;

				}  else {
					re = strncmp (fpath.buf,
					              HANDLER_VSRV(hdl)->root.buf,
					              HANDLER_VSRV(hdl)->root.len);
					if (re != 0) {
						ignore = true;
					}
				}
			}

			/* Perform the operation
			 */
			if (! ignore) {
				switch (op) {
				case op_include: {
					cherokee_buffer_t file_content = CHEROKEE_BUF_INIT;

					ret = cherokee_buffer_read_file (&file_content, fpath.buf);
					if (unlikely (ret != ret_ok)) {
						cherokee_buffer_mrproper (&file_content);
						ret = ret_error;
						goto out;
					}

					TRACE(ENTRIES, "Including file '%s'\n", fpath.buf);

					ret = parse (hdl, &file_content, out);
					if (unlikely (ret != ret_ok)) {
						cherokee_buffer_mrproper (&file_content);
						ret = ret_error;
						goto out;
					}

					cherokee_buffer_mrproper (&file_content);
					break;
				}

				case op_size:
					TRACE(ENTRIES, "Including file size '%s'\n", fpath.buf);
					re = cherokee_stat (fpath.buf, &info);
					if (re >=0) {
						cherokee_buffer_add_ullong10 (out, info.st_size);
					}
					break;

				case op_lastmod:
					TRACE(ENTRIES, "Including file modification date '%s'\n", fpath.buf);
					re = cherokee_stat (fpath.buf, &info);
					if (re >= 0) {
						struct tm *ltime;
						struct tm  ltime_buf;
						char       tmp[50];

						ltime = cherokee_localtime (&info.st_mtime, &ltime_buf);
						if (ltime != NULL) {
							strftime (tmp, sizeof(tmp), "%d-%b-%Y %H:%M", ltime);
							cherokee_buffer_add (out, tmp, strlen(tmp));
						}
					}
					break;
				default:
					SHOULDNT_HAPPEN;
				}
			} /* !ignore */

			break;
		default:
			SHOULDNT_HAPPEN;
		} /* switch(op) */
	} /* while */

	ret = ret_ok;

out:
	cherokee_buffer_mrproper (&key);
	cherokee_buffer_mrproper (&val);
	cherokee_buffer_mrproper (&pair);
	cherokee_buffer_mrproper (&fpath);

	return ret;
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
	re = cherokee_stat (local_path->buf, &hdl->cache_info);
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
	cherokee_buffer_add_buffer (&hdl->dir, &conn->request);

	while (true) {
		if (cherokee_buffer_is_empty (&hdl->dir))
			return ret_error;

		if (cherokee_buffer_is_ending (&hdl->dir, '/'))
			break;

		cherokee_buffer_drop_ending (&hdl->dir, 1);
	}


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
	ret_t                  ret;
	char                  *ext;
	cherokee_buffer_t     *mime = NULL;
	cherokee_server_t     *srv  = HANDLER_SRV(hdl);
	cherokee_connection_t *conn = HANDLER_CONN(hdl);

	/* MIME type
	 */
	if (srv->mime != NULL) {
		ext = strrchr (conn->request.buf, '.');
		if (ext == NULL)
			return ret_ok;

		ret = cherokee_mime_get_by_suffix (srv->mime, ext+1, &hdl->mime);
		if (ret == ret_ok) {
			cherokee_mime_entry_get_type (hdl->mime, &mime);

			cherokee_buffer_add_str    (buffer, "Content-Type: ");
			cherokee_buffer_add_buffer (buffer, mime);
			cherokee_buffer_add_str    (buffer, CRLF);
		}
	}

	/* Length
	 */
	if (cherokee_connection_should_include_length(conn)) {
		HANDLER(hdl)->support = hsupport_length;

		cherokee_buffer_add_str     (buffer, "Content-Length: ");
		cherokee_buffer_add_ullong10(buffer, (cullong_t) hdl->render.len);
		cherokee_buffer_add_str     (buffer, CRLF);
	}

	return ret_ok;
}
