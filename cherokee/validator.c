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
#include "validator.h"

#include "connection.h"
#include "connection-protected.h"
#include "header-protected.h"

ret_t 
cherokee_validator_init_base (cherokee_validator_t *validator)
{
	cherokee_module_init_base (MODULE(validator));
	
	validator->check = NULL;

	cherokee_buffer_init (&validator->user);
	cherokee_buffer_init (&validator->passwd);
	cherokee_buffer_init (&validator->realm);
	cherokee_buffer_init (&validator->response);
	cherokee_buffer_init (&validator->uri);
	cherokee_buffer_init (&validator->qop);
	cherokee_buffer_init (&validator->nonce);
	cherokee_buffer_init (&validator->cnonce);
	cherokee_buffer_init (&validator->algorithm);
	cherokee_buffer_init (&validator->nc);

	return ret_ok;
}


ret_t 
cherokee_validator_free_base (cherokee_validator_t *validator)
{
	cherokee_buffer_mrproper (&validator->user);
	cherokee_buffer_mrproper (&validator->passwd);
	cherokee_buffer_mrproper (&validator->realm);
	cherokee_buffer_mrproper (&validator->response);
	cherokee_buffer_mrproper (&validator->uri);
	cherokee_buffer_mrproper (&validator->qop);
	cherokee_buffer_mrproper (&validator->nonce);
	cherokee_buffer_mrproper (&validator->cnonce);
	cherokee_buffer_mrproper (&validator->algorithm);
	cherokee_buffer_mrproper (&validator->nc);

	free (validator);
	return ret_ok;
}


ret_t 
cherokee_validator_free (cherokee_validator_t *validator)
{
	module_func_free_t func;
	
	return_if_fail (validator!=NULL, ret_error);

	func = (module_func_free_t) MODULE(validator)->free;

	if (func == NULL) {
		return ret_error;
	}
	
	return func (validator);
}


ret_t 
cherokee_validator_check (cherokee_validator_t *validator, void *conn)
{
	if (validator->check == NULL) {
		return ret_error;
	}
	
	return validator->check (validator, CONN(conn));
}


ret_t 
cherokee_validator_add_headers (cherokee_validator_t *validator, void *conn, cherokee_buffer_t *buf)
{
	if (validator->add_headers == NULL) {
		return ret_error;
	}
	
	return validator->add_headers (validator, CONN(conn), buf);	
}


ret_t 
cherokee_validator_parse_basic (cherokee_validator_t *validator, char *str, cuint_t str_len)
{
	char              *colon;
	cherokee_buffer_t  auth = CHEROKEE_BUF_INIT;
	
	/* Decode base64
	 */
	cherokee_buffer_add (&auth, str, str_len);
	cherokee_buffer_decode_base64 (&auth);

	/* Look for the user:passwd structure
	 */
	colon = strchr (auth.buf, ':');
	if (colon == NULL) goto error;

	/* Copy user and password
	 */
	cherokee_buffer_add (&validator->user, auth.buf, colon - auth.buf);
	cherokee_buffer_add (&validator->passwd, colon+1, auth.len  - ((colon+1) - auth.buf));		
	
	/* Clean up and exit
	 */
	cherokee_buffer_mrproper (&auth);
	return ret_ok;

error:
	cherokee_buffer_mrproper (&auth);
	return ret_error;
}


ret_t 
cherokee_validator_parse_digest (cherokee_validator_t *validator, char *str, cuint_t str_len)
{
	cuint_t             len;
	char               *end;
	char               *entry;
	char               *comma;
	char               *equal;
	cherokee_buffer_t   auth = CHEROKEE_BUF_INIT;
	cherokee_buffer_t  *entry_buf;

	/* Copy authentication string
	 */
	cherokee_buffer_add (&auth, str, str_len);

	entry = str;
	end = str + str_len;
	
	do {
		/* Skip some chars
		 */
		while ((*entry == ' ')  ||
		       (*entry == '\r') || 
		       (*entry == '\n')) entry++;

		/* Check for the end
		 */
		if (entry >= end) 
			break;

		comma = strchr(entry, ',');

		if (strncasecmp (entry, "nc", 2) == 0) {
			entry_buf = &validator->nc;
		} else if (strncasecmp (entry, "uri", 3) == 0) {
			entry_buf = &validator->uri;
		} else if (strncasecmp (entry, "qop", 3) == 0) {
			entry_buf = &validator->qop;
		} else if (strncasecmp (entry, "realm", 5) == 0) {
			entry_buf = &validator->realm;
		} else if (strncasecmp (entry, "nonce", 5) == 0) {
			entry_buf = &validator->nonce;
		} else if (strncasecmp (entry, "cnonce", 6) == 0) {
			entry_buf = &validator->cnonce;
		} else if (strncasecmp (entry, "username", 8) == 0) {
			entry_buf = &validator->user;
		} else if (strncasecmp (entry, "response", 8) == 0) {
			entry_buf = &validator->response;
		} else if (strncasecmp (entry, "algorithm", 9) == 0) {
			entry_buf = &validator->algorithm;
		} else {
			entry = comma + 1;
			continue;
		}

		/* Split the string [1]
		 */
		if (comma) *comma = '\0';
	
		equal = strchr (entry, '=');
		if (equal == NULL) {
			if (comma) *comma = ',';  /* [1] */
			continue;
		}

		/* Check for "s and skip it
		 */
		equal += (equal[1] == '"') ? 2 : 1;
		len = strlen(equal);		
		while ((len > 0) &&
		       ((equal[len-1] == '"')  || 
			(equal[len-1] == '\r') || 
			(equal[len-1] == '\n'))) len--;

		/* Copy the entry value
		 */ 
		cherokee_buffer_add (entry_buf, equal, len);
		
		/* Resore [1], and prepare next loop 
		 */
		if (comma) *comma = ','; 
		entry = comma + 1;

	} while (comma != NULL);

#if 0
	printf ("nc           %s\n", validator->nc.buf);
	printf ("uri          %s\n", validator->uri.buf);
	printf ("qop          %s\n", validator->qop.buf);
	printf ("realm        %s\n", validator->realm.buf);
	printf ("nonce        %s\n", validator->nonce.buf);
	printf ("cnonce       %s\n", validator->cnonce.buf);
	printf ("username     %s\n", validator->user.buf);
	printf ("reponse      %s\n", validator->response.buf);
	printf ("algorithm    %s\n", validator->algorithm.buf);
#endif

	/* Clean up and exit
	 */
	cherokee_buffer_mrproper (&auth);
	return ret_ok;
}


static ret_t 
digest_HA2 (cherokee_validator_t *validator, cherokee_buffer_t *buf, cherokee_connection_t *conn)
{
	ret_t       ret;
	const char *method;

	/* Sanity check
	 */
	if (cherokee_buffer_is_empty (&validator->uri))
		return ret_deny;

	ret = cherokee_http_method_to_string (conn->header.method, &method, NULL);
	if (unlikely (ret != ret_ok)) return ret;

	cherokee_buffer_add_va (buf, "%s:%s", method, validator->uri.buf);
	cherokee_buffer_encode_md5_digest (buf);	

	return ret_ok;
}


ret_t 
cherokee_validator_digest_response (cherokee_validator_t  *validator, 
				    char                  *A1, 
				    cherokee_buffer_t     *buf, 
				    cherokee_connection_t *conn)
{
	ret_t              ret;
	cherokee_buffer_t a2 = CHEROKEE_BUF_INIT;

	/* A1 has to be in string of lenght 32:
	 * MD5_digest(user":"realm":"passwd)
	 */

	/* Sanity checks
	 */
	if (A1 == NULL)
		return ret_deny;

	if (cherokee_buffer_is_empty (&validator->nonce))
		return ret_deny;

	/* Build A2
	 */
	ret = digest_HA2 (validator, &a2, conn);
	if (ret != ret_ok) goto error;
	
	/* Build the final string
	 */
	cherokee_buffer_ensure_size (buf, 32 + a2.len + validator->nonce.len + 4);

	cherokee_buffer_add (buf, A1, 32);
	cherokee_buffer_add (buf, ":", 1);
	cherokee_buffer_add_buffer (buf, &validator->nonce);
	cherokee_buffer_add (buf, ":", 1);

	if (!cherokee_buffer_is_empty (&validator->qop)) {
		if (!cherokee_buffer_is_empty (&validator->nc))
			cherokee_buffer_add_buffer (buf, &validator->nc);		
		cherokee_buffer_add (buf, ":", 1);
		if (!cherokee_buffer_is_empty (&validator->cnonce))
			cherokee_buffer_add_buffer (buf, &validator->cnonce);		
		cherokee_buffer_add (buf, ":", 1);
		cherokee_buffer_add_buffer (buf, &validator->qop);		
		cherokee_buffer_add (buf, ":", 1);
	}

	cherokee_buffer_add_buffer (buf, &a2);
	cherokee_buffer_encode_md5_digest (buf);

	return ret_ok;

error:
	cherokee_buffer_mrproper (&a2);
	return ret;
}

