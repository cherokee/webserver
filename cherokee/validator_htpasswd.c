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
#include "validator_htpasswd.h"

#ifdef HAVE_CRYPT_H
# include <crypt.h>
#endif

#include "plugin_loader.h"
#include "thread.h"
#include "connection.h"
#include "connection-protected.h"
#include "sha1.h"
#include "md5crypt.h"

#define CRYPT_SALT_LENGTH 2


/* Plug-in initialization
 */
PLUGIN_INFO_VALIDATOR_EASIEST_INIT (htpasswd, http_auth_basic);


static ret_t 
props_free (cherokee_validator_htpasswd_props_t *props)
{
	cherokee_buffer_mrproper (&props->password_file);
	return cherokee_validator_props_free_base (VALIDATOR_PROPS(props));
}


ret_t 
cherokee_validator_htpasswd_configure (cherokee_config_node_t *conf, cherokee_server_t *srv, cherokee_module_props_t **_props)
{
	ret_t                                ret;
	cherokee_config_node_t              *subconf;
	cherokee_validator_htpasswd_props_t *props;

	UNUSED(srv);

	if (*_props == NULL) {
		CHEROKEE_NEW_STRUCT (n, validator_htpasswd_props);

		cherokee_validator_props_init_base (VALIDATOR_PROPS(n), MODULE_PROPS_FREE(props_free));
		cherokee_buffer_init (&n->password_file);

		*_props = MODULE_PROPS(n);
	}

	props = PROP_HTPASSWD(*_props);

	ret = cherokee_config_node_get (conf, "passwdfile", &subconf);
	if (ret == ret_ok) {
		cherokee_buffer_add_buffer (&props->password_file, &subconf->val);
	}

	return ret_ok;
}

ret_t 
cherokee_validator_htpasswd_new (cherokee_validator_htpasswd_t **htpasswd, cherokee_module_props_t *props)
{	  
	CHEROKEE_NEW_STRUCT(n,validator_htpasswd);

	/* Init 	
	 */
	cherokee_validator_init_base (VALIDATOR(n), VALIDATOR_PROPS(props), PLUGIN_INFO_VALIDATOR_PTR(htpasswd));
	VALIDATOR(n)->support = http_auth_basic;

	MODULE(n)->free           = (module_func_free_t)           cherokee_validator_htpasswd_free;
	VALIDATOR(n)->check       = (validator_func_check_t)       cherokee_validator_htpasswd_check;
	VALIDATOR(n)->add_headers = (validator_func_add_headers_t) cherokee_validator_htpasswd_add_headers;
	   
	/* Checks
	 */
	if (cherokee_buffer_is_empty (&VAL_HTPASSWD_PROP(n)->password_file)) {
		PRINT_MSG_S ("htpasswd validator needs a password file\n");
		return ret_error;
	}

	*htpasswd = n;
	return ret_ok;
}


ret_t 
cherokee_validator_htpasswd_free (cherokee_validator_htpasswd_t *htpasswd)
{
	cherokee_validator_free_base (VALIDATOR(htpasswd));
	return ret_ok;
}


#if !defined(HAVE_CRYPT_R) && defined(HAVE_PTHREAD)
# define USE_CRYPT_R_EMU
static pthread_mutex_t __global_crypt_r_emu_mutex = PTHREAD_MUTEX_INITIALIZER;

static ret_t
crypt_r_emu (const char *key, const char *salt, const char *compared)
{
	ret_t  ret;
	char  *tmp;

	CHEROKEE_MUTEX_LOCK(&__global_crypt_r_emu_mutex);
	tmp = crypt (key, salt);
	ret = (strcmp (tmp, compared) == 0) ? ret_ok : ret_deny;
	CHEROKEE_MUTEX_UNLOCK(&__global_crypt_r_emu_mutex);	

	return ret;
}

#endif


ret_t
check_crypt (char *passwd, char *salt, const char *compared)
{
	ret_t ret;

#ifndef HAVE_PTHREAD
	char *tmp;

	tmp = crypt (passwd, salt);
	ret = (strcmp (tmp, compared) == 0) ? ret_ok : ret_deny;
#else
# ifdef HAVE_CRYPT_R
	char              *tmp;
	struct crypt_data  data;

	memset (&data, 0, sizeof(data));
	tmp = crypt_r (passwd, salt, &data);
	ret = (strcmp (tmp, compared) == 0) ? ret_ok : ret_deny;

# elif defined(USE_CRYPT_R_EMU)
	ret = crypt_r_emu (passwd, salt, compared);
# else
# error "No crypt() implementation found"
# endif
#endif

	return ret;
}



static ret_t
validate_plain (cherokee_connection_t *conn, const char *crypted)
{
	if (cherokee_buffer_is_empty (&conn->validator->passwd))
		return ret_error;

	return (strcmp (conn->validator->passwd.buf, crypted) == 0) ? ret_ok : ret_error;
}


static ret_t
validate_crypt (cherokee_connection_t *conn, const char *crypted)
{
	ret_t ret;
	char  salt[CRYPT_SALT_LENGTH];

	if (cherokee_buffer_is_empty (&conn->validator->passwd))
		return ret_error;

	memcpy (salt, crypted, CRYPT_SALT_LENGTH);

	ret = check_crypt (conn->validator->passwd.buf, salt, crypted);

	return ret;
}


static ret_t
validate_md5 (cherokee_connection_t *conn, const char *magic, char *crypted)
{
	ret_t  ret;
	char  *new_md5_crypt;
	char   space[120];

	new_md5_crypt = md5_crypt (conn->validator->passwd.buf, crypted, magic, space);
	if (new_md5_crypt == NULL)
		return ret_error;
	
	ret = (strcmp (new_md5_crypt, crypted) == 0) ? ret_ok : ret_error;

	/* There is no need to free new_md5_crypt, it is 'space' (in the stack)..
	 */
	return ret;
}


static ret_t
validate_non_salted_sha (cherokee_connection_t *conn, char *crypted)
{	
	cuint_t           c_len       = strlen (crypted);
	cherokee_thread_t *thread = CONN_THREAD(conn);
	cherokee_buffer_t *sha1_buf1 = THREAD_TMP_BUF1(thread);
	cherokee_buffer_t *sha1_buf2 = THREAD_TMP_BUF2(thread);

	/* Check the size. It should be: "{SHA1}" + Base64(SHA1(info))
	 */
	if (c_len != 28)
		return ret_error;

	if (cherokee_buffer_is_empty (&conn->validator->passwd))
		return ret_error;
 
	/* Decode user
	 */
	cherokee_buffer_clean (sha1_buf1);
	cherokee_buffer_clean (sha1_buf2);
	cherokee_buffer_add_buffer (sha1_buf1, &conn->validator->passwd);
	cherokee_buffer_encode_sha1_base64 (sha1_buf1, sha1_buf2);

	if (strcmp (sha1_buf2->buf, crypted) == 0)
		return ret_ok;

	return ret_error;
}


static ret_t
request_isnt_passwd_file (cherokee_validator_htpasswd_t *htpasswd, cherokee_connection_t *conn)
{
	ret_t              ret;
	cherokee_buffer_t *pfile;

	pfile = &VAL_HTPASSWD_PROP(htpasswd)->password_file;

	if (conn->request.len > 0)
		cherokee_buffer_add (&conn->local_directory, conn->request.buf+1, conn->request.len-1);  /* 1: add    */

	ret = ret_ok;

	if (pfile->len == conn->local_directory.len) {
		ret = (strncmp (pfile->buf, 
				conn->local_directory.buf, 
				conn->local_directory.len) == 0) ? ret_error : ret_ok;
	}

	if (conn->request.len > 0)
		cherokee_buffer_drop_ending (&conn->local_directory, conn->request.len-1);              /* 1: remove */

	return ret;
}


ret_t 
cherokee_validator_htpasswd_check (cherokee_validator_htpasswd_t *htpasswd, cherokee_connection_t *conn)
{
	FILE *f;
	int   len;
	char *cryp;
	int   cryp_len;
	ret_t ret;
	ret_t ret_auth;
	CHEROKEE_TEMP(line, 128);

	/* Sanity check
	 */
	if ((conn->validator == NULL) || cherokee_buffer_is_empty (&conn->validator->user))
		return ret_error;

	/* 1.- Check the login/passwd
	 */	  
	f = fopen (VAL_HTPASSWD_PROP(htpasswd)->password_file.buf, "r");
	if (f == NULL) {
		return ret_error;
	}

	ret      = ret_error;
	ret_auth = ret_error;

	while (!feof(f)) {
		if (fgets (line, line_size, f) == NULL)
			continue;

		len = strlen (line);

		if (len <= 0) 
			continue;

		if (line[0] == '#')
			continue;

		if (line[len-1] == '\n') 
			line[len-1] = '\0';
			 
		/* Split into user and encrypted password. 
		 */
		cryp = strchr (line, ':');
		if (cryp == NULL) continue;
		*cryp++ = '\0';
		cryp_len = strlen(cryp);

		/* Is this the right user? 
		 */
		if (strcmp (conn->validator->user.buf, line) != 0) {
			continue;
		}

		/* Check the type of the crypted password:
		 * It recognizes: Apache MD5, MD5, SHA, old crypt and plain text
		 */
		if (strncmp (cryp, "$apr1$", 6) == 0) {
			const char *magic = "$apr1$";
			ret_auth = validate_md5 (conn, magic, cryp);

		} else if (strncmp (cryp, "$1$", 3) == 0) {
			const char *magic = "$1$";
			ret_auth = validate_md5 (conn, magic, cryp);

		} else if (strncmp (cryp, "{SHA}", 5) == 0) {
			ret_auth = validate_non_salted_sha (conn, cryp + 5);

		} else if (cryp_len == 13) {
			ret_auth = validate_crypt (conn, cryp);

			if (ret_auth != ret_ok)
				ret_auth = validate_plain (conn, cryp);

		} else {
			ret_auth = validate_plain (conn, cryp);
		}

		if (ret_auth == ret_ok)
			break;
	}

	fclose(f);

	/* Check the authentication returned value
	 */
	if (ret_auth < ret_ok)
		return ret_auth;

	/* 2.- Security check:
	 * Is the client trying to download the passwd file?
	 */
	ret = request_isnt_passwd_file (htpasswd, conn);	
	if (ret != ret_ok)
		return ret;

	return ret_ok;
}


ret_t 
cherokee_validator_htpasswd_add_headers (cherokee_validator_htpasswd_t *htpasswd, cherokee_connection_t *conn, cherokee_buffer_t *buf)
{
	UNUSED(htpasswd);
	UNUSED(conn);
	UNUSED(buf);

	return ret_ok;
}

