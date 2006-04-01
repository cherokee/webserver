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
#include "http.h"
#include "validator_pam.h"
#include "connection-protected.h"
#include "module_loader.h"

#include <security/pam_appl.h>

/* PAM service name
 */
#define CHEROKEE_AUTH_SERVICE "cherokee"


cherokee_module_info_validator_t MODULE_INFO(pam) = {
	.module.type     = cherokee_validator,                 /* type     */
	.module.new_func = cherokee_validator_pam_new,         /* new func */
	.valid_methods   = http_auth_basic                     /* methods  */
};


ret_t 
cherokee_validator_pam_new (cherokee_validator_pam_t **pam, cherokee_table_t *properties)
{
	CHEROKEE_NEW_STRUCT(n,validator_pam);

	/* Init 
	 */
	cherokee_validator_init_base (VALIDATOR(n));
	VALIDATOR(n)->support = http_auth_basic;

	MODULE(n)->free           = (module_func_free_t)           cherokee_validator_pam_free;
	VALIDATOR(n)->check       = (validator_func_check_t)       cherokee_validator_pam_check;
	VALIDATOR(n)->add_headers = (validator_func_add_headers_t) cherokee_validator_pam_add_headers;

	*pam = n;
	return ret_ok;
}


ret_t 
cherokee_validator_pam_free (cherokee_validator_pam_t *pam)
{
	cherokee_validator_free_base (VALIDATOR(pam));
	return ret_ok;
}



/*
 * auth_pam_talker: supply authentication information to PAM when asked
 *
 * Assumptions:
 *   A password is asked for by requesting input without echoing
 *   A username is asked for by requesting input _with_ echoing
 *
 */
static int 
auth_pam_talker (int                        num_msg,
		 const struct pam_message **msg,
		 struct pam_response      **resp,
		 void                      *appdata_ptr)
{
	unsigned short         i        = 0;
	struct pam_response   *response = NULL;
	cherokee_connection_t *conn     = CONN(appdata_ptr);

	/* parameter sanity checking 
	 */
	if (!resp || !msg || !conn) {
		return PAM_CONV_ERR;
	}

	/* allocate memory to store response 
	 */
	response = malloc (num_msg * sizeof(struct pam_response));
	if (!response) {
		return PAM_CONV_ERR;
	}

	/* copy values 
	 */
	for (i = 0; i < num_msg; i++) {
		/* initialize to safe values 
		 */
		response[i].resp_retcode = 0;
		response[i].resp = 0;

		/* select response based on requested output style 
		 */
		switch (msg[i]->msg_style) {
		case PAM_PROMPT_ECHO_ON:
			response[i].resp = strdup(conn->validator->user.buf);
			break;
		case PAM_PROMPT_ECHO_OFF:
			response[i].resp = strdup(conn->validator->passwd.buf);
			break;
		default:
			if (response)
				free(response);
			return PAM_CONV_ERR;
		}
	}

	/* everything okay, set PAM response values 
	 */
	*resp = response;
	return PAM_SUCCESS;
}


ret_t 
cherokee_validator_pam_check (cherokee_validator_pam_t  *pam, cherokee_connection_t *conn)
{
	int                  ret;
	static pam_handle_t *pamhandle = NULL;
	struct pam_conv      pamconv   = {&auth_pam_talker, conn};

	extern int _pam_dispatch (pam_handle_t *, int, int);

	/* Start the PAM query
	 */
	ret = pam_start (CHEROKEE_AUTH_SERVICE, conn->validator->user.buf, &pamconv, &pamhandle);
	if (ret != PAM_SUCCESS) {
		conn->error_code = http_internal_error;
		return ret_error;
	}

	/* NOTE: 
	 * First of all, it's a really *awful* hack.  Said that, let's
	 * see the right way to authenticate a user is call:
	 *
	 * 	ret = pam_authenticate (pamhandle, 0);
	 *
	 * Instead of it, this validator is calling:
	 *
	 *	ret = _pam_dispatch (pamhandle, 0, 1);
	 * 
	 * It is because pam_uthenticate() does a long delay if the
	 * user is not authenticated sucesfuly.  It is a huge problem
	 * if Cherokee is compiled without threading support because
	 * it will be frozen for some time until pam_authenticate()
	 * comes back.
	 *
	 * The second parameter: 0, is the flags
	 * The last one: 1, is PAM_AUTHENTICATE
	 */

	/* Try to authenticate user:
	 */
	ret = _pam_dispatch (pamhandle, 0, 1);
	if (ret != PAM_SUCCESS) {
		CHEROKEE_NEW(msg, buffer);

		cherokee_buffer_add (msg, "PAM: user '", 11);
		cherokee_buffer_add_buffer (msg, &conn->validator->user);
		cherokee_buffer_add_va (msg, "' - not authenticated: %s", pam_strerror(pamhandle, ret));

		cherokee_logger_write_string (CONN_VSRV(conn)->logger, "%s", msg->buf);
		
		cherokee_buffer_free (msg);
		goto unauthorized;
	}

	/* Check that the account is healthy 
	 */
	ret = pam_acct_mgmt (pamhandle, PAM_DISALLOW_NULL_AUTHTOK); 
	if (ret != PAM_SUCCESS) {
		CHEROKEE_NEW(msg, buffer);

		cherokee_buffer_add (msg, "PAM: user '", 11);
		cherokee_buffer_add_buffer (msg, &conn->validator->user);
		cherokee_buffer_add_va (msg, "'  - invalid account: %s", pam_strerror(pamhandle, ret));

		cherokee_logger_write_string (CONN_VSRV(conn)->logger, "%s", msg->buf);

		cherokee_buffer_free (msg);
		goto unauthorized;
	}

	pam_end (pamhandle, PAM_SUCCESS);
	return ret_ok;

unauthorized:
	pam_end (pamhandle, PAM_SUCCESS);
	return ret_error;
}


ret_t 
cherokee_validator_pam_add_headers (cherokee_validator_pam_t  *pam, cherokee_connection_t *conn, cherokee_buffer_t *buf)
{
	return ret_ok;
}


/* Library init function
 */
static cherokee_boolean_t _pam_is_init = false;

void
MODULE_INIT(pam) (cherokee_module_loader_t *loader)
{
	/* Init flag
	 */
	if (_pam_is_init) return;
	_pam_is_init = true;
}
