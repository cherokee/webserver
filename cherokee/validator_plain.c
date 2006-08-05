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
#include "validator_plain.h"

#include "connection.h"
#include "connection-protected.h"
#include "module_loader.h"


static ret_t 
props_free (cherokee_validator_plain_props_t *props)
{
	cherokee_buffer_mrproper (&props->password_file);
	return cherokee_validator_props_free_base (VALIDATOR_PROPS(props));
}

ret_t 
cherokee_validator_plain_configure (cherokee_config_node_t *conf, cherokee_server_t *srv, cherokee_validator_props_t **_props)
{
	ret_t                             ret;
	cherokee_config_node_t           *subconf;
	cherokee_validator_plain_props_t *props;

	if (*_props == NULL) {
		CHEROKEE_NEW_STRUCT (n, validator_plain_props);

		cherokee_validator_props_init_base (VALIDATOR_PROPS(n), 
						    VALIDATOR_PROPS_FREE(props_free));
		cherokee_buffer_init (&n->password_file);
		
		*_props = VALIDATOR_PROPS(n);
	}

	props = PROP_PLAIN(*_props);

	ret = cherokee_config_node_get (conf, "passwdfile", &subconf);
	if (ret == ret_ok) {
		cherokee_buffer_add_buffer (&props->password_file, &subconf->val);
	}

	return ret_ok;
}


ret_t 
cherokee_validator_plain_new (cherokee_validator_plain_t **plain, cherokee_validator_props_t *props)
{	
	CHEROKEE_NEW_STRUCT(n,validator_plain);

	/* Init 		
	 */
	cherokee_validator_init_base (VALIDATOR(n), props);
	VALIDATOR(n)->support = http_auth_basic | http_auth_digest;

	MODULE(n)->free           = (module_func_free_t)           cherokee_validator_plain_free;
	VALIDATOR(n)->check       = (validator_func_check_t)       cherokee_validator_plain_check;
	VALIDATOR(n)->add_headers = (validator_func_add_headers_t) cherokee_validator_plain_add_headers;
	   
	/* Checks
	 */
	if (cherokee_buffer_is_empty (&VAL_PLAIN_PROP(n)->password_file)) {
		PRINT_MSG_S ("plain validator needs a password file\n");
		return ret_error;
	}
	   
	*plain = n;
	return ret_ok;
}


ret_t 
cherokee_validator_plain_free (cherokee_validator_plain_t *plain)
{
	cherokee_validator_free_base (VALIDATOR(plain));
	return ret_ok;
}


static ret_t
check_digest (cherokee_validator_plain_t *plain, char *passwd, cherokee_connection_t *conn)
{			
	ret_t                 ret;
	cherokee_buffer_t     a1        = CHEROKEE_BUF_INIT;
	cherokee_buffer_t     buf       = CHEROKEE_BUF_INIT;
	cherokee_validator_t *validator = conn->validator;

	/* Sanity check
	 */
	if (cherokee_buffer_is_empty (&validator->user) ||
	    cherokee_buffer_is_empty (&validator->realm)) 
		return ret_deny;

	/* Build A1
	 */
	cherokee_buffer_add_va (&a1, "%s:%s:%s", 
				validator->user.buf,
				validator->realm.buf,
				passwd);

	cherokee_buffer_encode_md5_digest (&a1);

	/* Build a possible response
	 */
	cherokee_validator_digest_response (VALIDATOR(plain), a1.buf, &buf, conn);

	/* Check the response
	 */
	if (cherokee_buffer_is_empty (&conn->validator->response)) {
		ret = ret_error;
		goto go_out;
	}
	
	/* Compare and return
	 */
	ret = (strcmp (conn->validator->response.buf, buf.buf) == 0) ? ret_ok : ret_error;

go_out:
	cherokee_buffer_mrproper (&a1);
	cherokee_buffer_mrproper (&buf);
	return ret;
}

ret_t 
cherokee_validator_plain_check (cherokee_validator_plain_t *plain, cherokee_connection_t *conn)
{
	FILE  *f;
	ret_t  ret;

        if ((conn->validator == NULL) || cherokee_buffer_is_empty(&conn->validator->user)) {
                return ret_error;
        }

	f = fopen (VAL_PLAIN_PROP(plain)->password_file.buf, "r"); 
//	f = fopen (plain->file_ref, "r");
	if (f == NULL) {
		return ret_error;
	}

	ret = ret_error;
	while (!feof(f)) {
		int   len;
		char *pass;
		CHEROKEE_TEMP(line, 256);

		if (fgets (line, line_size, f) == NULL)
			continue;

		len = strlen (line);

		if (len <= 3) 
			continue;
			 
		if (line[0] == '#')
			continue;

		if (line[len-1] == '\n') 
			line[len-1] = '\0';
			 
		/* Split into user and encrypted password. 
		 */
		pass = strchr (line, ':');
		if (pass == NULL) continue;
		*pass++ = '\0';
			 
		/* Is this the right user? 
		 */
		if (strncmp (conn->validator->user.buf, line, 
			     conn->validator->user.len) != 0) {
			continue;
		}

		/* Validate it
		 */
		switch (conn->req_auth_type) {
		case http_auth_basic:
			/* Empty password
			 */
			if (conn->validator->passwd.len <= 0) {
				if (strlen(pass) == 0) {
					ret = ret_ok;
					goto go_out;
				}
				continue;
			}

			/* Check the passwd
			 */
			if (strcmp (conn->validator->passwd.buf, pass) == 0) {
				ret = ret_ok;
				goto go_out;
			}
			break;

		case http_auth_digest:
			ret = check_digest (plain, pass, conn);
			if (ret == ret_ok) goto go_out;
			break;

		default:
			SHOULDNT_HAPPEN;
		}
	}

go_out:
	fclose(f);
	return ret;
}


ret_t 
cherokee_validator_plain_add_headers (cherokee_validator_plain_t *plain, cherokee_connection_t *conn, cherokee_buffer_t *buf)
{
	return ret_ok;
}


/*   Library init function
 */
void
MODULE_INIT(plain) (cherokee_module_loader_t *loader)
{
}

VALIDATOR_MODULE_INFO_INIT_EASY (plain, http_auth_basic | http_auth_digest);
