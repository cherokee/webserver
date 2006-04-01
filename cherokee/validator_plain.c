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


cherokee_module_info_validator_t MODULE_INFO(plain) = {
	.module.type     = cherokee_validator,                 /* type     */
	.module.new_func = cherokee_validator_plain_new,       /* new func */
	.valid_methods   = http_auth_basic | http_auth_digest  /* methods  */
};

ret_t 
cherokee_validator_plain_new (cherokee_validator_plain_t **plain, cherokee_table_t *properties)
{	
	ret_t ret;
	CHEROKEE_NEW_STRUCT(n,validator_plain);

	/* Init 		
	 */
	cherokee_validator_init_base (VALIDATOR(n));
	VALIDATOR(n)->support = http_auth_basic | http_auth_digest;

	MODULE(n)->free           = (module_func_free_t)           cherokee_validator_plain_free;
	VALIDATOR(n)->check       = (validator_func_check_t)       cherokee_validator_plain_check;
	VALIDATOR(n)->add_headers = (validator_func_add_headers_t) cherokee_validator_plain_add_headers;
	   
	n->file_ref = NULL;

	/* Get the properties
	 */
	if (properties) {
		ret = cherokee_typed_table_get_str (properties, "file", (char **)&n->file_ref);
		if (ret < ret_ok) {
			PRINT_MSG_S ("plain validator needs a \"File\" property\n");
			return ret_error;
		}
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

	f = fopen (plain->file_ref, "r");
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
static cherokee_boolean_t _plain_is_init = false;

void
MODULE_INIT(plain) (cherokee_module_loader_t *loader)
{
	/* Init flag
	 */
	if (_plain_is_init == true) return;
	_plain_is_init = true;

	/* Other stuff
	 */
}

