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
#include "handler_dbslayer.h"
#include "connection-protected.h"
#include "thread.h"
#include "util.h"

#define add_cstr_str(w,key,val,len)                               \
	do {                                                      \
		cherokee_dwriter_string (w, key, sizeof(key)-1);  \
		cherokee_dwriter_string (w, val, len);            \
	} while (false)

#define add_cstr_int(w,key,val)                                   \
	do {                                                      \
		cherokee_dwriter_string  (w, key, sizeof(key)-1); \
		cherokee_dwriter_unsigned (w, val);                \
	} while (false)


PLUGIN_INFO_HANDLER_EASIEST_INIT (dbslayer, http_get);


static ret_t
connect_to_database (cherokee_handler_dbslayer_t *hdl)
{
	MYSQL                             *conn;
	cherokee_handler_dbslayer_props_t *props      = HANDLER_DBSLAYER_PROPS(hdl);
	cherokee_connection_t             *connection = HANDLER_CONN(hdl);

	conn = mysql_real_connect (hdl->conn,
	                           hdl->src_ref->host.buf,
	                           props->user.buf,
	                           props->password.buf,
	                           props->db.buf,
	                           hdl->src_ref->port,
	                           hdl->src_ref->unix_socket.buf,
	                           CLIENT_MULTI_STATEMENTS | CLIENT_MULTI_RESULTS);
	if (conn == NULL) {
		cherokee_balancer_report_fail (props->balancer, connection, hdl->src_ref);

		connection->error_code = http_bad_gateway;
		return ret_error;
	}

	return ret_ok;
}

static ret_t
send_query (cherokee_handler_dbslayer_t *hdl)
{
	int                    re;
	cuint_t                len;
	cherokee_connection_t *conn = HANDLER_CONN(hdl);
	cherokee_buffer_t     *tmp  = &HANDLER_THREAD(hdl)->tmp_buf1;

	/* Extract the SQL query
	 */
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

	cherokee_buffer_unescape_uri (tmp);

	/* Send the query
	 */
	re = mysql_real_query (hdl->conn, tmp->buf, tmp->len);
	if (re != 0)
		return ret_error;

	return ret_ok;
}

static ret_t
cherokee_client_headers (cherokee_handler_dbslayer_t *hdl)
{
	ret_t                  ret;
	char                  *hdr   = NULL;
	cuint_t                len   = 0;
	cherokee_connection_t *conn  = HANDLER_CONN(hdl);

	ret = cherokee_header_get_unknown (&conn->header, "X-Beautify", 10, &hdr, &len);
	if ((ret == ret_ok) && hdr) {
		ret = cherokee_atob (hdr, &hdl->writer.pretty);
		if (ret != ret_ok) return ret;
	}

	hdr = NULL;
	ret = cherokee_header_get_unknown (&conn->header, "X-Rollback", 10, &hdr, &len);
	if ((ret == ret_ok) && hdr) {
		ret = cherokee_atob (hdr, &hdl->rollback);
		if (ret != ret_ok) return ret;
	}

	return ret_ok;
}


static ret_t
handle_error (cherokee_handler_dbslayer_t *hdl)
{
	int   re;
	char *tmp;

	/* Render error reply
	 */
	cherokee_dwriter_dict_open (&hdl->writer);
	add_cstr_int (&hdl->writer, "SUCCESS", false);
	add_cstr_int (&hdl->writer, "MYSQL_ERRNO", mysql_errno(hdl->conn));

	tmp = (char *)mysql_error (hdl->conn);
	add_cstr_str (&hdl->writer, "MYSQL_ERROR", tmp, strlen(tmp));

	/* Issue a rollback if needed
	 */
	if (hdl->rollback) {
		re = mysql_rollback (hdl->conn);

		add_cstr_int (&hdl->writer, "ROLLBACK_ON_ERROR", true);
		add_cstr_int (&hdl->writer, "ROLLBACK_ON_ERROR_SUCCESS", !re);
	}

	cherokee_dwriter_dict_close (&hdl->writer);
	return ret_ok;
}


ret_t
cherokee_handler_dbslayer_init (cherokee_handler_dbslayer_t *hdl)
{
	ret_t                              ret;
	cherokee_connection_t             *conn  = HANDLER_CONN(hdl);
	cherokee_handler_dbslayer_props_t *props = HANDLER_DBSLAYER_PROPS(hdl);

	/* Check client headers
	 */
	ret = cherokee_client_headers (hdl);
	if (unlikely (ret != ret_ok))
		return ret;

	/* Get a reference to the target host
	 */
	if (hdl->src_ref == NULL) {
		ret = cherokee_balancer_dispatch (props->balancer, conn, &hdl->src_ref);
		if (ret != ret_ok)
			return ret;
	}

	/* Connect to the MySQL server
	 */
	ret = connect_to_database(hdl);
	if (unlikely (ret != ret_ok))
		return ret;

	/* Send query:
	 * Do not check whether it failed, ::step() will do
	 * it and send an error message if needed.
	 */
	send_query(hdl);

	return ret_ok;
}


static ret_t
dbslayer_add_headers (cherokee_handler_dbslayer_t *hdl,
                      cherokee_buffer_t           *buffer)
{
	switch (HANDLER_DBSLAYER_PROPS(hdl)->lang) {
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


static ret_t
render_empty_result (cherokee_handler_dbslayer_t *hdl)
{
	cherokee_dwriter_dict_open (&hdl->writer);
	add_cstr_int (&hdl->writer, "SUCCESS", true);
	add_cstr_int (&hdl->writer, "AFFECTED_ROWS", mysql_affected_rows(hdl->conn));
	add_cstr_int (&hdl->writer, "INSERT_ID", mysql_insert_id (hdl->conn));
	cherokee_dwriter_dict_close (&hdl->writer);

	return ret_ok;
}

static ret_t
render_result (cherokee_handler_dbslayer_t *hdl,
               MYSQL_RES                   *result)
{
	cuint_t      i;
	cuint_t      num_fields;
	MYSQL_ROW    row;
	MYSQL_FIELD *fields;
	char        *tmp;

#define TYPE2S(n)                                                         \
	case MYSQL_TYPE_ ## n:                                            \
		cherokee_dwriter_cstring (&hdl->writer, "MYSQL_TYPE_"#n); \
	break

#define BLOB_TYPE2S(n)                                                    \
	case MYSQL_TYPE_ ## n:                                            \
		if (fields[i].charsetnr == 63)                            \
			cherokee_dwriter_cstring (&hdl->writer,           \
			                          "MYSQL_TYPE_TEXT");     \
		else                                                      \
			cherokee_dwriter_cstring (&hdl->writer,           \
			                          "MYSQL_TYPE_"#n);       \
	break

#define CHECK_NULL                                    \
	if (row[i] == NULL) {                         \
		cherokee_dwriter_null (&hdl->writer); \
		continue;                             \
	}


	cherokee_dwriter_dict_open (&hdl->writer);
	cherokee_dwriter_cstring (&hdl->writer, "RESULT");
	cherokee_dwriter_dict_open (&hdl->writer);


	num_fields = mysql_num_fields (result);
	fields     = mysql_fetch_fields (result);

	/* Types
	 * Blobs: http://www.mysql.org/doc/refman/5.1/en/c-api-datatypes.html
	 */
	cherokee_dwriter_cstring (&hdl->writer, "TYPES");
	cherokee_dwriter_list_open (&hdl->writer);
	for(i = 0; i < num_fields; i++) {
		switch(fields[i].type) {
			TYPE2S(TINY);
			TYPE2S(SHORT);
			TYPE2S(LONG);
			TYPE2S(INT24);
			TYPE2S(DECIMAL);
			TYPE2S(NEWDECIMAL);
			TYPE2S(DOUBLE);
			TYPE2S(FLOAT);
			TYPE2S(LONGLONG);
			TYPE2S(BIT);
			TYPE2S(TIMESTAMP);
			TYPE2S(DATE);
			TYPE2S(TIME);
			TYPE2S(DATETIME);
			TYPE2S(YEAR);
			TYPE2S(STRING);
			TYPE2S(VAR_STRING);
			TYPE2S(NEWDATE);
			TYPE2S(VARCHAR);
			TYPE2S(SET);
			TYPE2S(ENUM);
			TYPE2S(GEOMETRY);
			TYPE2S(NULL);
			BLOB_TYPE2S(BLOB);
			BLOB_TYPE2S(TINY_BLOB);
			BLOB_TYPE2S(MEDIUM_BLOB);
			BLOB_TYPE2S(LONG_BLOB);
		default:
			cherokee_dwriter_cstring (&hdl->writer, "MYSQL_TYPE_UNKNOWN");
		}
	}
	cherokee_dwriter_list_close (&hdl->writer);

	/* Headers
	 */
	cherokee_dwriter_cstring (&hdl->writer, "HEADER");
	cherokee_dwriter_list_open (&hdl->writer);
	for(i = 0; i < num_fields; i++) {
		tmp = fields[i].name;
		cherokee_dwriter_string (&hdl->writer, tmp, strlen(tmp));
	}
	cherokee_dwriter_list_close (&hdl->writer);

	/* Data
	 */
	cherokee_dwriter_cstring (&hdl->writer, "ROWS");
	cherokee_dwriter_list_open (&hdl->writer);

	while (true) {
		row = mysql_fetch_row (result);
		if (! row)
			break;

		cherokee_dwriter_list_open (&hdl->writer);
		for(i = 0; i < num_fields; i++) {
			switch(fields[i].type) {
			case MYSQL_TYPE_TINY:
			case MYSQL_TYPE_SHORT:
			case MYSQL_TYPE_LONG:
			case MYSQL_TYPE_INT24:
			case MYSQL_TYPE_DECIMAL:
			case MYSQL_TYPE_NEWDECIMAL:
			case MYSQL_TYPE_DOUBLE:
			case MYSQL_TYPE_FLOAT:
			case MYSQL_TYPE_LONGLONG:
				CHECK_NULL;
				cherokee_dwriter_number (&hdl->writer, row[i], strlen(row[i]));
				break;

			case MYSQL_TYPE_BIT:
			case MYSQL_TYPE_TIMESTAMP:
			case MYSQL_TYPE_DATE:
			case MYSQL_TYPE_TIME:
			case MYSQL_TYPE_DATETIME:
			case MYSQL_TYPE_YEAR:
			case MYSQL_TYPE_STRING:
			case MYSQL_TYPE_VAR_STRING:
			case MYSQL_TYPE_NEWDATE:
			case MYSQL_TYPE_VARCHAR:
				CHECK_NULL;
				cherokee_dwriter_string (&hdl->writer, row[i], strlen(row[i]));
				break;

			case MYSQL_TYPE_BLOB:
			case MYSQL_TYPE_TINY_BLOB:
			case MYSQL_TYPE_MEDIUM_BLOB:
			case MYSQL_TYPE_LONG_BLOB:
				if ((row[i] == NULL) ||
				    (fields[i].charsetnr != 63))
				{
					cherokee_dwriter_null (&hdl->writer);
					continue;
				}
				cherokee_dwriter_string (&hdl->writer, row[i], strlen(row[i]));
				break;

			case MYSQL_TYPE_SET:
			case MYSQL_TYPE_ENUM:
			case MYSQL_TYPE_GEOMETRY:
			case MYSQL_TYPE_NULL:
				cherokee_dwriter_null (&hdl->writer);
				break;
			default:
				SHOULDNT_HAPPEN;
			}
		}
		cherokee_dwriter_list_close (&hdl->writer);
	}
	cherokee_dwriter_list_close (&hdl->writer);


	cherokee_dwriter_dict_close (&hdl->writer);
	cherokee_dwriter_dict_close (&hdl->writer);

	return ret_ok;
}

static ret_t
dbslayer_step (cherokee_handler_dbslayer_t *hdl,
               cherokee_buffer_t           *buffer)
{
	int        re;
	MYSQL_RES *result;

	cherokee_dwriter_set_buffer (&hdl->writer, buffer);

	/* Open the result list */
	cherokee_dwriter_list_open (&hdl->writer);

	/* Iterate through the results
	 */
	do {
		result = mysql_store_result (hdl->conn);
		if (result == NULL) {
			/* ERROR:
			 * - Statement didn't return a result set. Eg: Insert
			 * - Reading of the result set failed
			 */
			if (mysql_errno (hdl->conn)) {
				handle_error (hdl);
			} else {
				render_empty_result (hdl);
			}
		}
		else {
			render_result (hdl, result);
			mysql_free_result (result);
		}

		re = mysql_next_result (hdl->conn);
		if (re > 0) {
			handle_error (hdl);
		}
	} while (re == 0);

	/* Close results list */
	cherokee_dwriter_list_close (&hdl->writer);

	return ret_eof_have_data;
}


static ret_t
dbslayer_free (cherokee_handler_dbslayer_t *hdl)
{
	if (hdl->conn)
		mysql_close (hdl->conn);

	cherokee_dwriter_mrproper (&hdl->writer);
	return ret_ok;
}

ret_t
cherokee_handler_dbslayer_new (cherokee_handler_t     **hdl,
                               void                    *cnt,
                               cherokee_module_props_t *props)
{
	CHEROKEE_NEW_STRUCT (n, handler_dbslayer);

	/* Init the base class object
	 */
	cherokee_handler_init_base (HANDLER(n), cnt, HANDLER_PROPS(props), PLUGIN_INFO_HANDLER_PTR(dbslayer));

	MODULE(n)->init         = (handler_func_init_t) cherokee_handler_dbslayer_init;
	MODULE(n)->free         = (module_func_free_t) dbslayer_free;
	HANDLER(n)->step        = (handler_func_step_t) dbslayer_step;
	HANDLER(n)->add_headers = (handler_func_add_headers_t) dbslayer_add_headers;

	/* Supported features
	 */
	HANDLER(n)->support     = hsupport_nothing;

	/* Properties
	 */
	n->src_ref  = NULL;
	n->rollback = false;

	/* Data writer */
	cherokee_dwriter_init (&n->writer, &CONN_THREAD(cnt)->tmp_buf1);
	n->writer.lang = PROP_DBSLAYER(props)->lang;

	/* MySQL */
	n->conn = mysql_init (NULL);
	if (unlikely (n->conn == NULL)) {
		cherokee_handler_free (HANDLER(n));
		return ret_nomem;
	}

	*hdl = HANDLER(n);
	return ret_ok;
}


static ret_t
props_free  (cherokee_handler_dbslayer_props_t *props)
{
	if (props->balancer)
		cherokee_balancer_free (props->balancer);

	cherokee_buffer_mrproper (&props->user);
	cherokee_buffer_mrproper (&props->password);
	cherokee_buffer_mrproper (&props->db);

	return ret_ok;
}


ret_t
cherokee_handler_dbslayer_configure (cherokee_config_node_t   *conf,
                                     cherokee_server_t        *srv,
                                     cherokee_module_props_t **_props)
{
	ret_t                              ret;
	cherokee_list_t                   *i;
	cherokee_handler_dbslayer_props_t *props;

	/* Instance a new property object
	 */
	if (*_props == NULL) {
		CHEROKEE_NEW_STRUCT (n, handler_dbslayer_props);

		cherokee_handler_props_init_base (HANDLER_PROPS(n),
		                                  MODULE_PROPS_FREE(props_free));
		n->balancer = NULL;
		cherokee_buffer_init (&n->user);
		cherokee_buffer_init (&n->password);
		cherokee_buffer_init (&n->db);

		*_props = MODULE_PROPS(n);
	}

	props = PROP_DBSLAYER(*_props);

	/* Parse the configuration tree
	 */
	cherokee_config_node_foreach (i, conf) {
		cherokee_config_node_t *subconf = CONFIG_NODE(i);

		if (equal_buf_str (&subconf->key, "balancer")) {
			ret = cherokee_balancer_instance (&subconf->val, subconf, srv, &props->balancer);
			if (ret != ret_ok)
				return ret;

		} else if (equal_buf_str (&subconf->key, "user")) {
			cherokee_buffer_clean (&props->user);
			cherokee_buffer_add_buffer (&props->user, &subconf->val);

		} else if (equal_buf_str (&subconf->key, "password")) {
			cherokee_buffer_clean (&props->password);
			cherokee_buffer_add_buffer (&props->password, &subconf->val);

		} else if (equal_buf_str (&subconf->key, "db")) {
			cherokee_buffer_clean (&props->db);
			cherokee_buffer_add_buffer (&props->db, &subconf->val);

		} else if (equal_buf_str (&subconf->key, "lang")) {
			ret = cherokee_dwriter_lang_to_type (&subconf->val, &props->lang);
			if (ret != ret_ok) {
				LOG_CRITICAL (CHEROKEE_ERROR_HANDLER_DBSLAYER_LANG, subconf->val.buf);
				return ret_error;
			}
		}
	}

	/* Final checks
	 */
	if (props->balancer == NULL) {
		LOG_CRITICAL_S (CHEROKEE_ERROR_HANDLER_DBSLAYER_BALANCER);
		return ret_error;
	}

	return ret_ok;
}


