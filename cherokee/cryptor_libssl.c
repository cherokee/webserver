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

/* IN CASE THIS PLUG-IN IS COMPILED WITH OPENSSL:
 *
 * In addition, as a special exception, the copyright holders give
 * permission to link the code of portions of this program with the
 * OpenSSL library under certain conditions as described in each
 * individual source file, and distribute linked combinations
 * including the two.
 *
 * You must obey the GNU General Public License in all respects for
 * all of the code used other than OpenSSL.  If you modify file(s)
 * with this exception, you may extend this exception to your version
 * of the file(s), but you are not obligated to do so.  If you do not
 * wish to do so, delete this exception statement from your version.
 * If you delete this exception statement from all source files in the
 * program, then also delete it here.
 */

#include "common-internal.h"
#include "cryptor_libssl.h"
#include "virtual_server.h"
#include "socket.h"
#include "util.h"
#include "server-protected.h"

#define ENTRIES "crypto,ssl"

static ret_t
_free (cherokee_cryptor_libssl_t *cryp)
{
	return ret_ok;
}


static ret_t
_configure (cherokee_cryptor_t     *cryp,
	    cherokee_config_node_t *conf,
	    cherokee_server_t      *srv)
{
	return ret_ok;
}

static ret_t
_vserver_free (cherokee_cryptor_vserver_libssl_t *cryp_vsrv)
{
	cherokee_avl_r_mrproper (&cryp_vsrv->session_cache, NULL);

	if (cryp_vsrv->context != NULL) {
		SSL_CTX_free (cryp_vsrv->context);
		cryp_vsrv->context = NULL;
	}

	free (cryp_vsrv);
	return ret_ok;
}

#ifndef OPENSSL_NO_TLSEXT
static int
openssl_sni_servername_cb (SSL *ssl, int *ad, void *arg)
{
	ret_t                      ret;
	const char                *servername;
	cherokee_socket_t         *socket;
	cherokee_buffer_t          tmp;
	SSL_CTX                   *ctx;
	cherokee_server_t         *srv       = SRV(arg);
	cherokee_virtual_server_t *vsrv      = NULL;

	/* Get the pointer to the socket 
	 */
	socket = SSL_get_app_data (ssl); 
	if (unlikely (socket == NULL)) {
		PRINT_ERROR ("Could not get the socket struct: %p\n", ssl);
		return SSL_TLSEXT_ERR_ALERT_FATAL; 
	}

	/* Read the SNI server name
	 */
	servername = SSL_get_servername (ssl, TLSEXT_NAMETYPE_host_name);
	if (servername == NULL) {
		TRACE (ENTRIES, "No SNI: Did not provide a server name%s", "\n");
		return SSL_TLSEXT_ERR_NOACK;
	}

	TRACE (ENTRIES, "SNI: Switching to servername='%s'\n", servername);

	/* Try to match the name
	 */
	cherokee_buffer_fake (&tmp, servername, strlen(servername));
	ret = cherokee_server_get_vserver (srv, &tmp, &vsrv);
	if ((ret != ret_ok) || (vsrv == NULL)) {
		PRINT_ERROR ("Servername did not match: '%s'\n", servername);
		return SSL_TLSEXT_ERR_NOACK; 
	}

	TRACE (ENTRIES, "SNI: Setting new TLS context. Virtual host='%s'\n",
	       vsrv->name.buf);

	/* Set the new SSL context
	 */
	ctx = SSL_set_SSL_CTX (ssl, CRYPTOR_VSRV_SSL(vsrv->cryptor)->context);
	if (ctx != CRYPTOR_VSRV_SSL(vsrv->cryptor)->context) {
		PRINT_ERROR ("Could change the SSL context: servername='%s'\n", servername);
	}

	return SSL_TLSEXT_ERR_OK; 
}
#endif

static ret_t
_vserver_new (cherokee_cryptor_t          *cryp,
	      cherokee_virtual_server_t   *vsrv,
	      cherokee_cryptor_vserver_t **cryp_vsrv)
{
	ret_t  ret;
	int    rc;
	char  *error;
	CHEROKEE_NEW_STRUCT (n, cryptor_vserver_libssl);
	
	/* Init
	 */
	ret = cherokee_cryptor_vserver_init_base (CRYPTOR_VSRV(n));
	if (ret != ret_ok)
		return ret;

	CRYPTOR_VSRV(n)->free = (cryptor_vsrv_func_free_t) _vserver_free;

	/* Properties
	 */
	ret = cherokee_avl_r_init (&n->session_cache);
	if (ret != ret_ok)
		return ret;

	/* Init the OpenSSL context
	 */
	n->context = SSL_CTX_new (SSLv23_server_method());
	if (n->context == NULL) {
		PRINT_ERROR_S("ERROR: OpenSSL: Couldn't allocate OpenSSL context\n");
		return ret_error;
	}

	/* Set OpenSSL context options
	 */

	/* Client-side options */
	SSL_CTX_set_options (n->context, SSL_OP_MICROSOFT_SESS_ID_BUG);
	SSL_CTX_set_options (n->context, SSL_OP_NETSCAPE_CHALLENGE_BUG);
	SSL_CTX_set_options (n->context, SSL_OP_NETSCAPE_REUSE_CIPHER_CHANGE_BUG);

	/* Server-side options */
	SSL_CTX_set_options (n->context, SSL_OP_SSLREF2_REUSE_CERT_TYPE_BUG);
	SSL_CTX_set_options (n->context, SSL_OP_MICROSOFT_BIG_SSLV3_BUFFER);
	SSL_CTX_set_options (n->context, SSL_OP_MSIE_SSLV2_RSA_PADDING);
	SSL_CTX_set_options (n->context, SSL_OP_SSLEAY_080_CLIENT_DH_BUG);
	SSL_CTX_set_options (n->context, SSL_OP_TLS_D5_BUG);
	SSL_CTX_set_options (n->context, SSL_OP_TLS_BLOCK_PADDING_BUG);
	SSL_CTX_set_options (n->context, SSL_OP_SINGLE_DH_USE);

#ifdef SSL_OP_DONT_INSERT_EMPTY_FRAGMENTS
	SSL_CTX_set_options (n->context, SSL_OP_DONT_INSERT_EMPTY_FRAGMENTS);
#endif

	/* Trusted CA certificates
	 */
	if (! cherokee_buffer_is_empty (&vsrv->ca_cert)) {
		rc = SSL_CTX_load_verify_locations (n->context, vsrv->ca_cert.buf, NULL);
		if (rc != 1) {
			OPENSSL_LAST_ERROR(error);
			PRINT_ERROR("ERROR: OpenSSL: Can't read trusted CA list '%s': %s\n", 
				    vsrv->server_key.buf, error);
			return ret_error;
		}
	}

	/* Certificate
	 */
	rc = SSL_CTX_use_certificate_chain_file (n->context, vsrv->server_cert.buf);
	if (rc != 1) {
		OPENSSL_LAST_ERROR(error);
		PRINT_ERROR("ERROR: OpenSSL: Can not use certificate file '%s':  %s\n", 
			    vsrv->server_cert.buf, error);
		return ret_error;
	}

	/* Private key
	 */
	rc = SSL_CTX_use_PrivateKey_file (n->context, vsrv->server_key.buf, SSL_FILETYPE_PEM);
	if (rc != 1) {
		OPENSSL_LAST_ERROR(error);
		PRINT_ERROR("ERROR: OpenSSL: Can not use private key file '%s': %s\n", 
			    vsrv->server_key.buf, error);
		return ret_error;
	}

	/* Check private key
	 */
	rc = SSL_CTX_check_private_key (n->context);
	if (rc != 1) {
		PRINT_ERROR_S("ERROR: OpenSSL: Private key does not match the certificate public key\n");
		return ret_error;
	}

#ifndef OPENSSL_NO_TLSEXT
	/* Enable SNI
	 */
	rc = SSL_CTX_set_tlsext_servername_callback (n->context, openssl_sni_servername_cb);
	if (rc != 1) {
		OPENSSL_LAST_ERROR(error);
		PRINT_ERROR ("Could activate TLS SNI for '%s': %s\n", vsrv->name.buf, error);
		return ret_error;
	}

	rc = SSL_CTX_set_tlsext_servername_arg (n->context, VSERVER_SRV(vsrv));
	if (rc != 1) {
		OPENSSL_LAST_ERROR(error);
		PRINT_ERROR ("Could activate TLS SNI for '%s': %s\n", vsrv->name.buf, error);
		return ret_error;
	}
#endif /* OPENSSL_NO_TLSEXT */

	*cryp_vsrv = CRYPTOR_VSRV(n);
	return ret_ok;
}


static ret_t
socket_initialize (cherokee_cryptor_socket_libssl_t *cryp,
		   cherokee_socket_t                *socket,
		   cherokee_virtual_server_t        *vserver)
{
	int                                re;
	cherokee_cryptor_vserver_libssl_t *vsrv_crytor = CRYPTOR_VSRV_SSL(vserver->cryptor);

	/* Set the virtual server object reference
	 */
	CRYPTOR_SOCKET(cryp)->vserver_ref = vserver;

	/* Check whether the virtual server supports SSL
	 */
	if (vserver->cryptor == NULL) {
		return ret_not_found;
	}

	if (vsrv_crytor->context == NULL) {
		return ret_not_found;
	}

	/* New session
	 */
	cryp->session = SSL_new (vsrv_crytor->context);
	if (cryp->session == NULL) {
		char *error;

		OPENSSL_LAST_ERROR(error);
		PRINT_ERROR ("ERROR: OpenSSL: Unable to create a new SSL "
			     "connection from the SSL context: %s\n", error);
		return ret_error;
	}
	
	/* Set the socket file descriptor
	 */
	re = SSL_set_fd (cryp->session, socket->socket);
	if (re != 1) {
		char *error;

		OPENSSL_LAST_ERROR(error);
		PRINT_ERROR ("ERROR: OpenSSL: Can not set fd(%d): %s\n", 
			     socket->socket, error);
		return ret_error;
	}

#ifndef OPENSSL_NO_TLSEXT 
	SSL_set_app_data (cryp->session, socket);
#endif

	/* Set the SSL context cache
	 */
	re = SSL_CTX_set_session_id_context (vsrv_crytor->context, 
					     (const unsigned char *)"SSL", 3);
	if (re != 1) {
		PRINT_ERROR_S("ERROR: OpenSSL: Unable to set SSL session-id context\n");
	}

	return ret_ok;
}
	

static ret_t
_socket_init_tls (cherokee_cryptor_socket_libssl_t *cryp,
		  cherokee_socket_t                *sock,
		  cherokee_virtual_server_t        *vsrv)
{
	int   re;
	ret_t ret;

	/* Initialize
	 */
	if (CRYPTOR_SOCKET(cryp)->initialized == false) {
		ret = socket_initialize (cryp, sock, vsrv);
		if (ret != ret_ok) {
			return ret_error;
		}

		CRYPTOR_SOCKET(cryp)->initialized = true;
	}

	/* TLS Handshake
	 */
	re = SSL_accept (cryp->session);
	if (re <= 0) {
		char *error;

		switch (SSL_get_error (cryp->session, re)) {
		case SSL_ERROR_WANT_READ:
		case SSL_ERROR_WANT_WRITE:
		case SSL_ERROR_WANT_CONNECT:
			return ret_eagain;
		default: 
			OPENSSL_LAST_ERROR(error);
			PRINT_ERROR ("ERROR: Init OpenSSL: %s\n", error);
			return ret_error;
		}
	}

	return ret_ok;
}

static ret_t
_socket_close (cherokee_cryptor_socket_libssl_t *cryp)
{
	SSL_shutdown (cryp->session);
	return ret_ok;
}

static ret_t
_socket_write (cherokee_cryptor_socket_libssl_t *cryp,
	       char                             *buf,
	       int                               buf_len,
	       size_t                           *pcnt_written)
{
	int     re;
	ssize_t len;

	len = SSL_write (cryp->session, buf, buf_len);
	if (likely (len > 0) ) {
		/* Return info
		 */
		*pcnt_written = len;
		return ret_ok;
	}

	if (len == 0) {
		/* maybe socket was closed by client, no write was performed
		int re = SSL_get_error (socket->session, len);
		if (re == SSL_ERROR_ZERO_RETURN)
			socket->status = socket_closed;
		 */
		return ret_eof;
	}

	/* len < 0 */
	re = SSL_get_error (cryp->session, len);
	switch (re) {
	case SSL_ERROR_WANT_READ:
	case SSL_ERROR_WANT_WRITE:
		return ret_eagain;
	case SSL_ERROR_SSL:
		return ret_error;
	}
	
	PRINT_ERROR ("ERROR: SSL_write (%d, ..) -> err=%d '%s'\n", 
		     SOCKET_FD(socket), (int)len, ERR_error_string(re, NULL));

	return ret_error;
}


static ret_t
_socket_read (cherokee_cryptor_socket_libssl_t *cryp,
	      char                             *buf,
	      int                               buf_size,
	      size_t                           *pcnt_read)
{
	int     re;
	int     error;
	ssize_t len;

	len = SSL_read (cryp->session, buf, buf_size);
	if (likely (len > 0)) {
		*pcnt_read = len;
		if (SSL_pending (cryp->session))
			return ret_eagain;
		return ret_ok;
	}

	if (len == 0) {
		return ret_eof;
	}

	/* len < 0 */
	error = errno;
	re = SSL_get_error (cryp->session, len);
	switch (re) {
	case SSL_ERROR_WANT_READ:   
	case SSL_ERROR_WANT_WRITE:   
		return ret_eagain;
	case SSL_ERROR_ZERO_RETURN: 
		return ret_eof;
	case SSL_ERROR_SSL:         
		return ret_error;
	case SSL_ERROR_SYSCALL:
		switch (error) {
		case EPIPE:
		case ECONNRESET:
			return ret_eof;
		default:
			PRINT_ERRNO_S (error, "SSL_read: unknown errno: ${errno}\n");
		}
		return ret_error;
	}

	PRINT_ERROR ("ERROR: OpenSSL: SSL_read (%d, ..) -> err=%d '%s'\n",
		     SOCKET_FD(socket), (int)len, ERR_error_string(re, NULL));
	return ret_error;
}

static int
_socket_pending (cherokee_cryptor_socket_libssl_t *cryp)
{
	return (SSL_pending (cryp->session) > 0);
}

static ret_t
_socket_clean (cherokee_cryptor_socket_libssl_t *cryp_socket)
{
	cherokee_cryptor_socket_clean_base (CRYPTOR_SOCKET(cryp_socket));

	if (cryp_socket->session != NULL) {
		SSL_free (cryp_socket->session);
		cryp_socket->session = NULL;
	}
	
	return ret_ok;
}

static ret_t
_socket_free (cherokee_cryptor_socket_libssl_t *cryp_socket)
{
	_socket_clean (cryp_socket);

	free (cryp_socket);
	return ret_ok;
}

static ret_t
_socket_new (cherokee_cryptor_libssl_t         *cryp,
	     cherokee_cryptor_socket_libssl_t **cryp_socket)
{
	ret_t ret;
	CHEROKEE_NEW_STRUCT (n, cryptor_socket_libssl);
	
	ret = cherokee_cryptor_socket_init_base (CRYPTOR_SOCKET(n));
	if (unlikely (ret != ret_ok))
		return ret;

	/* Socket properties */
	n->session = NULL;
	n->ssl_ctx = NULL;

	/* Virtual methods */
	CRYPTOR_SOCKET(n)->free     = (cryptor_socket_func_free_t) _socket_free;
	CRYPTOR_SOCKET(n)->clean    = (cryptor_socket_func_clean_t) _socket_clean;
	CRYPTOR_SOCKET(n)->init_tls = (cryptor_socket_func_init_tls_t) _socket_init_tls;
	CRYPTOR_SOCKET(n)->close    = (cryptor_socket_func_close_t) _socket_close;
	CRYPTOR_SOCKET(n)->read     = (cryptor_socket_func_read_t) _socket_read;
	CRYPTOR_SOCKET(n)->write    = (cryptor_socket_func_write_t) _socket_write;
	CRYPTOR_SOCKET(n)->pending  = (cryptor_socket_func_pending_t) _socket_pending;

	*cryp_socket = n;
	return ret_ok;
}

static int
_client_init_tls (cherokee_cryptor_client_libssl_t *cryp,
		  cherokee_buffer_t                *host,
		  cherokee_socket_t                *socket)
{
	int   re;
	char *error;

	/* New context
	 */
	cryp->ssl_ctx = SSL_CTX_new (SSLv23_client_method());
	if (cryp->ssl_ctx == NULL) {
		OPENSSL_LAST_ERROR(error);
		PRINT_ERROR ("ERROR: OpenSSL: Unable to create a new SSL context: %s\n", error);
		return ret_error;
	}
	
	/* CA verifications
	re = cherokee_buffer_is_empty (&cryp->vserver_ref->ca_cert);
	if (! re) {
		re = SSL_CTX_load_verify_locations (socket->ssl_ctx, 
						    socket->vserver_ref->ca_cert.buf, NULL);
		if (! re) {
			OPENSSL_LAST_ERROR(error);
			PRINT_ERROR ("ERROR: OpenSSL: '%s': %s\n", 
				     socket->vserver_ref->ca_cert.buf, error);
			return ret_error;
		}

		re = SSL_CTX_set_default_verify_paths (socket->ssl_ctx);
		if (! re) {
			OPENSSL_LAST_ERROR(error);
			PRINT_ERROR ("ERROR: OpenSSL: cannot set certificate "
				     "verification paths: %s\n", error);
			return ret_error;
		}
	}
	 */


	SSL_CTX_set_verify (cryp->ssl_ctx, SSL_VERIFY_NONE, NULL);
	SSL_CTX_set_mode (cryp->ssl_ctx, SSL_MODE_ENABLE_PARTIAL_WRITE);

	/* New session
	 */
	cryp->session = SSL_new (cryp->ssl_ctx);
	if (cryp->session == NULL) {
		OPENSSL_LAST_ERROR(error);
		PRINT_ERROR ("ERROR: OpenSSL: Unable to create a new SSL connection "
			     "from the SSL context: %s\n", error);
		return ret_error;
	}

	/* Set the socket file descriptor
	 */
	re = SSL_set_fd (cryp->session, socket->socket);
	if (re != 1) {
		OPENSSL_LAST_ERROR(error);
		PRINT_ERROR ("ERROR: OpenSSL: Can not set fd(%d): %s\n", socket->socket, error);
		return ret_error;
	}

	SSL_set_connect_state (cryp->session); 

#ifndef OPENSSL_NO_TLSEXT 
	if ((host != NULL) &&
	    (! cherokee_buffer_is_empty (host)))
	{
		re = SSL_set_tlsext_host_name (cryp->session, host->buf);
		if (re <= 0) {
			OPENSSL_LAST_ERROR(error);
			PRINT_ERROR ("ERROR: OpenSSL: Could set SNI server name: %s\n", error);
			return ret_error;
		}
	}
#endif

	re = SSL_connect (cryp->session);
	if (re <= 0) {
		OPENSSL_LAST_ERROR(error);
		PRINT_ERROR ("ERROR: OpenSSL: Can not connect: %s\n", error);
		return ret_error;
	}

	return ret_ok;
}

static ret_t
_client_free (cherokee_cryptor_client_libssl_t *cryp)
{
	if (cryp->session != NULL) {
		SSL_free (cryp->session);
		cryp->session = NULL;
	}

	if (cryp->ssl_ctx != NULL) {
		SSL_CTX_free (cryp->ssl_ctx);
		cryp->ssl_ctx = NULL;
	}

	free (cryp);
	return ret_ok;
}

static ret_t
_client_new (cherokee_cryptor_t         *cryp,
	     cherokee_cryptor_client_t **cryp_client)
{
	ret_t ret;
	CHEROKEE_NEW_STRUCT (n, cryptor_client_libssl);

	ret = cherokee_cryptor_socket_init_base (CRYPTOR_SOCKET(n));
	if (unlikely (ret != ret_ok))
		return ret;

	/* Client properties */
	n->session = NULL;
	n->ssl_ctx = NULL;

	/* Socket */
	CRYPTOR_SOCKET(n)->close    = (cryptor_socket_func_close_t) _socket_close;
	CRYPTOR_SOCKET(n)->read     = (cryptor_socket_func_read_t) _socket_read;
	CRYPTOR_SOCKET(n)->write    = (cryptor_socket_func_write_t) _socket_write;
	CRYPTOR_SOCKET(n)->pending  = (cryptor_socket_func_pending_t) _socket_pending;
	CRYPTOR_SOCKET(n)->clean    = (cryptor_socket_func_clean_t) _socket_clean;

	/* Client */
	CRYPTOR_SOCKET(n)->free     = (cryptor_socket_func_free_t) _client_free;
	CRYPTOR_SOCKET(n)->init_tls = (cryptor_client_func_init_t) _client_init_tls;

	*cryp_client = CRYPTOR_CLIENT(n);
	return ret_ok;
}


PLUGIN_INFO_INIT (libssl,
		  cherokee_cryptor,
		  cherokee_cryptor_libssl_new,
		  NULL);


ret_t
cherokee_cryptor_libssl_new (cherokee_cryptor_libssl_t **cryp)
{
	ret_t ret;
	CHEROKEE_NEW_STRUCT (n, cryptor_libssl);

	/* Init
	 */
	ret = cherokee_cryptor_init_base (CRYPTOR(n), PLUGIN_INFO_PTR(libssl));
	if (ret != ret_ok)
		return ret;

	MODULE(n)->free         = (module_func_free_t) _free;
	CRYPTOR(n)->configure   = (cryptor_func_configure_t) _configure;
	CRYPTOR(n)->vserver_new = (cryptor_func_vserver_new_t) _vserver_new;
	CRYPTOR(n)->socket_new  = (cryptor_func_socket_new_t) _socket_new;
	CRYPTOR(n)->client_new  = (cryptor_func_client_new_t) _client_new;

	*cryp = n;
	return ret_ok;
}





/* Plug-in initialization
 */

cherokee_boolean_t PLUGIN_IS_INIT(libssl) = false;

void
PLUGIN_INIT_NAME(libssl) (cherokee_plugin_loader_t *loader)
{
#if HAVE_OPENSSL_ENGINE_H
	ENGINE *e;
#endif
	UNUSED(loader);
	
	/* Do not initialize the library twice */
	PLUGIN_INIT_ONCE_CHECK (libssl);

# if HAVE_OPENSSL_ENGINE_H
#  if OPENSSL_VERSION_NUMBER >= 0x00907000L
        ENGINE_load_builtin_engines();
#  endif
        e = ENGINE_by_id("pkcs11");
        while (e != NULL) {
                if(! ENGINE_init(e)) {
                        ENGINE_free (e);
                        PRINT_ERROR_S ("ERROR: Could not init pkcs11 engine");
			break;
                }
                
                if(! ENGINE_set_default(e, ENGINE_METHOD_ALL)) {
                        ENGINE_free (e);
                        PRINT_ERROR_S ("ERROR: Could not set all defaults");
			break;
                }
                
                ENGINE_finish(e);
                ENGINE_free(e);
		break;
        }
#endif

        SSL_load_error_strings();
        SSL_library_init();
        
        SSLeay_add_all_algorithms ();
        SSLeay_add_ssl_algorithms ();
}
