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
#include "http.h"
#include "validator_pam.h"
#include "connection-protected.h"
#include "plugin_loader.h"

#include <security/pam_appl.h>

/* PAM service name
 */
#define CHEROKEE_AUTH_SERVICE "cherokee"


/* Plug-in initialization
 */
PLUGIN_INFO_VALIDATOR_EASIEST_INIT (pam, http_auth_basic);


ret_t
cherokee_validator_pam_configure (cherokee_config_node_t   *conf,
                                  cherokee_server_t        *srv,
                                  cherokee_module_props_t **props)
{
	UNUSED(conf);
	UNUSED(srv);
	UNUSED(props);

	return ret_ok;
}


ret_t
cherokee_validator_pam_new (cherokee_validator_pam_t **pam,
                            cherokee_module_props_t  *props)
{
	CHEROKEE_NEW_STRUCT(n,validator_pam);

	/* Init
	 */
	cherokee_validator_init_base (VALIDATOR(n), VALIDATOR_PROPS(props), PLUGIN_INFO_VALIDATOR_PTR(pam));
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
			response[i].resp = strdup((conn->validator->user.buf ? conn->validator->user.buf : ""));
			break;
		case PAM_PROMPT_ECHO_OFF:
			response[i].resp = strdup((conn->validator->passwd.buf ? conn->validator->passwd.buf : ""));
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
cherokee_validator_pam_check (cherokee_validator_pam_t *pam,
                              cherokee_connection_t    *conn)
{
	int                  ret;
	pam_handle_t        *pamhandle = NULL;
	struct pam_conv      pamconv   = {&auth_pam_talker, conn};

	extern int _pam_dispatch (pam_handle_t *, int, int);

	UNUSED(pam);

	/* Start the PAM query
	 */
	ret = pam_start (CHEROKEE_AUTH_SERVICE, conn->validator->user.buf, &pamconv, &pamhandle);
	if (ret != PAM_SUCCESS) {
		conn->error_code = http_internal_error;
		return ret_error;
	}

	/* Try to authenticate user:
	 */
#ifdef HAVE_PAM_FAIL_DELAY
	ret = pam_fail_delay (pamhandle, 0);
	if (ret != PAM_SUCCESS) {
		LOG_ERROR_S (CHEROKEE_ERROR_VALIDATOR_PAM_DELAY);

		conn->error_code = http_internal_error;
		return ret_error;
	}

	ret = pam_authenticate (pamhandle, 0);

#elif defined(HAVE_PAM_DISPATCH)

	/* If you can't set the delay to zero, you try to call one of
	 * the PAM internal functions. It is nasty, but reached this
	 * point it's the only thing you can do.
	 *
	 * Parameters: The second one, 0, are the flags. The third
	 * means PAM_AUTHENTICATE
	 */
	ret = _pam_dispatch (pamhandle, 0, 1);
#else

	/* If it fails, it will probably block
	 */
	ret = pam_authenticate (pamhandle, 0);
#endif

	if (ret != PAM_SUCCESS) {
		LOG_ERROR (CHEROKEE_ERROR_VALIDATOR_PAM_AUTH,
		           conn->validator->user.buf,
		           pam_strerror(pamhandle, ret));

		goto unauthorized;
	}

	/* Check that the account is healthy
	 */
	ret = pam_acct_mgmt (pamhandle, PAM_DISALLOW_NULL_AUTHTOK);
	if (ret != PAM_SUCCESS) {
		LOG_ERROR (CHEROKEE_ERROR_VALIDATOR_PAM_ACCOUNT,
		           conn->validator->user.buf,
		           pam_strerror(pamhandle, ret));

		goto unauthorized;
	}

	pam_end (pamhandle, PAM_SUCCESS);
	return ret_ok;

unauthorized:
	pam_end (pamhandle, PAM_SUCCESS);
	return ret_error;
}


ret_t
cherokee_validator_pam_add_headers (cherokee_validator_pam_t *pam,
                                    cherokee_connection_t    *conn,
                                    cherokee_buffer_t        *buf)
{
	UNUSED(pam);
	UNUSED(conn);
	UNUSED(buf);

	return ret_ok;
}

