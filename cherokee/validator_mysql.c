/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/* Cherokee
 *
 * Authors:
 *      Brian Rosner <brosner@gmail.com>
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
#include "validator_mysql.h"

#include "connection.h"
#include "connection-protected.h"
#include "plugin_loader.h"
#include "util.h"

#define ENTRIES "validator,mysql"
#define MYSQL_DEFAULT_PORT 3306


/* Plug-in initialization
 */
PLUGIN_INFO_VALIDATOR_EASIEST_INIT (mysql, http_auth_basic | http_auth_digest);


static ret_t
props_free (cherokee_validator_mysql_props_t *props)
{
	cherokee_buffer_mrproper (&props->host);
	cherokee_buffer_mrproper (&props->unix_socket);
	cherokee_buffer_mrproper (&props->user);
	cherokee_buffer_mrproper (&props->passwd);
	cherokee_buffer_mrproper (&props->database);
	cherokee_buffer_mrproper (&props->query);

	return cherokee_validator_props_free_base (VALIDATOR_PROPS(props));
}


ret_t 
cherokee_validator_mysql_configure (cherokee_config_node_t *conf, cherokee_server_t *srv, cherokee_module_props_t **_props)
{
	cherokee_list_t			 *i;
	cherokee_validator_mysql_props_t *props;
	
	if(*_props == NULL) {
		CHEROKEE_NEW_STRUCT (n, validator_mysql_props);
		
		cherokee_validator_props_init_base (VALIDATOR_PROPS (n), MODULE_PROPS_FREE(props_free));
		
		cherokee_buffer_init (&n->host);
		cherokee_buffer_init (&n->unix_socket);		
		cherokee_buffer_init (&n->user);
		cherokee_buffer_init (&n->passwd);
		cherokee_buffer_init (&n->database);
		cherokee_buffer_init (&n->query);

		n->port           = MYSQL_DEFAULT_PORT;
		n->use_md5_passwd = false;

		*_props = MODULE_PROPS (n);
	}
	
	props = PROP_MYSQL(*_props);
	
	cherokee_config_node_foreach (i, conf) {
		cherokee_config_node_t *subconf = CONFIG_NODE(i);
		
		if (equal_buf_str (&subconf->key, "host")) {
			cherokee_buffer_add_buffer (&props->host, &subconf->val);

		} else if (equal_buf_str (&subconf->key, "port")) {
			props->port = atoi (subconf->val.buf);

		} else if (equal_buf_str (&subconf->key, "unix_socket")) {
			cherokee_buffer_add_buffer (&props->unix_socket, &subconf->val);

		} else if (equal_buf_str (&subconf->key, "user")) {
			cherokee_buffer_add_buffer (&props->user, &subconf->val);

		} else if (equal_buf_str (&subconf->key, "passwd")) {
			cherokee_buffer_add_buffer (&props->passwd, &subconf->val);			

		} else if (equal_buf_str (&subconf->key, "database")) {
			cherokee_buffer_add_buffer (&props->database, &subconf->val);

		} else if (equal_buf_str (&subconf->key, "query")) {
			cherokee_buffer_add_buffer (&props->query, &subconf->val);

		} else if (equal_buf_str (&subconf->key, "use_md5_passwd")) {
			props->use_md5_passwd = !!atoi (subconf->val.buf);

		} else if ((equal_buf_str (&subconf->key, "methods") || 
			    equal_buf_str (&subconf->key, "realm"))) 
		{
			/* not handled here
			 */
		} else {
			PRINT_MSG ("ERROR: Validator MySQL: Unknown key: '%s'\n", subconf->key.buf);
			return ret_error;
		}
	}

	/* Checks
	 */
	if (cherokee_buffer_is_empty (&props->user)) {
		PRINT_ERROR_S ("ERROR: MySQL validator: an 'user' entry is needed\n");
		return ret_error;
	}
	if (cherokee_buffer_is_empty (&props->passwd)) {
		PRINT_ERROR_S ("ERROR: MySQL validator: an 'passwd' entry is needed\n");
		return ret_error;
	}
	if (cherokee_buffer_is_empty (&props->database)) {
		PRINT_ERROR_S ("ERROR: MySQL validator: an 'database' entry is needed\n");
		return ret_error;
	}
	if (cherokee_buffer_is_empty (&props->query)) {
		PRINT_ERROR_S ("ERROR: MySQL validator: an 'query' entry is needed\n");
		return ret_error;
	}

	return ret_ok;
}


static ret_t
init_mysql_connection (cherokee_validator_mysql_t *mysql, cherokee_validator_mysql_props_t *props)
{
	MYSQL *conn;

	if (unlikely ((props->host.buf == NULL) &&
		      (props->unix_socket.buf == NULL))) {
		PRINT_ERROR_S ("ERROR: MySQL validator misconfigured: A Host or Unix socket is needed.");
		return ret_error;
	}

	mysql->conn = mysql_init (NULL);
	if (mysql->conn == NULL)
		return ret_nomem;

	conn = mysql_real_connect (mysql->conn, 
				   props->host.buf, 
				   props->user.buf, 
				   props->passwd.buf, 
				   props->database.buf, 
				   props->port, 
				   props->unix_socket.buf, 0);
	if (conn == NULL) {
		PRINT_ERROR ("Unable to connect to MySQL server: %s:%d %s\n", 
			     props->host.buf, props->port, mysql_error (mysql->conn));
		return ret_error;
	}

	TRACE (ENTRIES, "Connected to (%s:%d)\n", props->host.buf, props->port);
	return ret_ok;
}


ret_t 
cherokee_validator_mysql_new (cherokee_validator_mysql_t **mysql, cherokee_module_props_t *props)
{
	ret_t ret;
	CHEROKEE_NEW_STRUCT (n, validator_mysql);

	cherokee_validator_init_base (VALIDATOR(n), VALIDATOR_PROPS(props), PLUGIN_INFO_VALIDATOR_PTR(mysql));
	VALIDATOR(n)->support = http_auth_basic | http_auth_digest;

	MODULE(n)->free           = (module_func_free_t)           cherokee_validator_mysql_free;
	VALIDATOR(n)->check       = (validator_func_check_t)       cherokee_validator_mysql_check;
	VALIDATOR(n)->add_headers = (validator_func_add_headers_t) cherokee_validator_mysql_add_headers;

	/* Initialization
	 */
	ret = init_mysql_connection (n, PROP_MYSQL(props));
	if (ret != ret_ok) {
		cherokee_validator_mysql_free (n);
		return ret;
	}

	/* Return obj
	 */
	*mysql = n;
	return ret_ok;
}


ret_t 
cherokee_validator_mysql_free (cherokee_validator_mysql_t *mysql)
{
	if (mysql->conn != NULL) {
		mysql_close (mysql->conn);
	}

	return ret_ok;
}


ret_t 
cherokee_validator_mysql_check (cherokee_validator_mysql_t *mysql, cherokee_connection_t *conn)
{
	int                               re;
	ret_t                             ret;
	MYSQL_ROW	                  row;
	MYSQL_RES	                 *result;
	unsigned long                    *lengths;
	cherokee_buffer_t                 db_passwd   = CHEROKEE_BUF_INIT;
	cherokee_buffer_t                 user_passwd = CHEROKEE_BUF_INIT;
	cherokee_buffer_t                 query       = CHEROKEE_BUF_INIT;
	cherokee_validator_mysql_props_t *props	      = VAL_MYSQL_PROP(mysql);

	/* Sanity checks
	 */
	if (unlikely ((conn->validator == NULL) || 
		      cherokee_buffer_is_empty (&conn->validator->user))) {
		return ret_error;
	}

	if (unlikely (strcasestr (conn->validator->user.buf, " or ")))
		return ret_error;

	re = cherokee_buffer_cnt_cspn (&conn->validator->user, 0, "'\";");
	if (re != conn->validator->user.len)
		return ret_error;

	/* Build query
	 */
	cherokee_buffer_add_buffer (&query, &props->query);
	cherokee_buffer_replace_string (&query, "${user}", 7, conn->validator->user.buf, conn->validator->user.len);

	/* Execute query
	 */
	re = mysql_query (mysql->conn, query.buf); 
	if (re != 0) {
		TRACE (ENTRIES, "Unable to execute authenication query: %s\n", mysql_error (mysql->conn));
		ret = ret_error;
		goto error;
	}

	result = mysql_store_result (mysql->conn);

	re = mysql_num_rows (result); 
	if (re <= 0) {
		TRACE (ENTRIES, "User %s was not found\n", conn->validator->user.buf);
		ret = ret_not_found;
		goto error;
	} else if  (re > 1) {
		TRACE (ENTRIES, "The user %s is not unique in the DB\n", conn->validator->user.buf);
		ret = ret_deny;
		goto error;
	}

	/* Copy the user information
	 */
	row     = mysql_fetch_row (result);
	lengths = mysql_fetch_lengths (result);

	if ((props->use_md5_passwd) || 
	    (conn->req_auth_type == http_auth_digest)) {
		cherokee_buffer_add_buffer (&user_passwd, &conn->validator->passwd);
		cherokee_buffer_encode_md5_digest (&user_passwd);
	} else {
		cherokee_buffer_add_buffer (&user_passwd, &conn->validator->passwd);
	}
	cherokee_buffer_add (&db_passwd, row[0], (size_t) lengths[0]);

	/* Check it out
	 */
	switch (conn->req_auth_type) {
	case http_auth_basic:
		re = cherokee_buffer_case_cmp_buf (&user_passwd, &db_passwd);
		ret = (re == 0) ? ret_ok : ret_deny;
		break;

	case http_auth_digest:
		ret = cherokee_validator_digest_check (VALIDATOR(mysql), &db_passwd, conn);
		break;

	default:
		SHOULDNT_HAPPEN;
		ret = ret_error;
		goto error;
	}

	if (ret != ret_ok) {
		TRACE (ENTRIES, "User %s did not properly authenticate.\n", conn->validator->user.buf);
		ret = ret_not_found;
		goto error;
	}

	TRACE (ENTRIES, "Access to user %s has been granted\n", conn->validator->user.buf);

	/* Clean-up
	 */
	cherokee_buffer_mrproper (&query);
	cherokee_buffer_mrproper (&db_passwd);
	cherokee_buffer_mrproper (&user_passwd);

	mysql_free_result (result);
	return ret_ok;

error:
	cherokee_buffer_mrproper (&query);
	cherokee_buffer_mrproper (&db_passwd);
	cherokee_buffer_mrproper (&user_passwd);
	return ret;
}


ret_t 
cherokee_validator_mysql_add_headers (cherokee_validator_mysql_t *mysql, cherokee_connection_t *conn, cherokee_buffer_t *buf)
{
	return ret_ok;
}

