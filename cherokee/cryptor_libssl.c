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

static DH *dh_param_512  = NULL;
static DH *dh_param_1024 = NULL;
static DH *dh_param_2048 = NULL;
static DH *dh_param_4096 = NULL;

#include "cryptor_libssl_dh_512.c"
#include "cryptor_libssl_dh_1024.c"
#include "cryptor_libssl_dh_2048.c"
#include "cryptor_libssl_dh_4096.c"

#define CLEAR_LIBSSL_ERRORS                                             \
	do {                                                            \
		unsigned long openssl_error;                            \
		while ((openssl_error = ERR_get_error())) {             \
			TRACE(ENTRIES, "Ignoring libssl error: %s\n",   \
			      ERR_error_string(openssl_error, NULL));   \
		}                                                       \
	} while(0)


static ret_t
_free (cherokee_cryptor_libssl_t *cryp)
{
	/* DH Parameters
	 */
	if (dh_param_512  != NULL) {
		DH_free (dh_param_512);
		dh_param_512 = NULL;
	}

	if (dh_param_1024 != NULL) {
		DH_free (dh_param_1024);
		dh_param_1024 = NULL;
	}

	if (dh_param_2048 != NULL) {
		DH_free (dh_param_2048);
		dh_param_2048 = NULL;
	}

	if (dh_param_4096 != NULL) {
		DH_free (dh_param_4096);
		dh_param_4096 = NULL;
	}

	/* Free loaded error strings
	 */
	ERR_free_strings();

	/* Free all ciphers and digests
	 */
	EVP_cleanup();

	cherokee_cryptor_free_base (CRYPTOR(cryp));
	return ret_ok;
}

static ret_t
try_read_dh_param(cherokee_config_node_t  *conf,
                  DH                     **dh,
                  int                      bitsize)
{
	ret_t              ret;
	cherokee_buffer_t *buf;
	FILE              *paramfile = NULL;
	cherokee_buffer_t  confentry = CHEROKEE_BUF_INIT;

	cherokee_buffer_add_va (&confentry, "dh_param%d", bitsize);

	/* Read the configuration parameter
	 */
	ret = cherokee_config_node_read (conf, confentry.buf, &buf);
	if (ret != ret_ok) {
		ret = ret_ok;
		goto out;
	}

	/* Read the file
	 */
	paramfile = fopen (buf->buf, "r");
	if (paramfile == NULL) {
		TRACE(ENTRIES, "Cannot open file %s\n", buf->buf);
		ret = ret_file_not_found;
		goto out;
	}

	/* Process the content
	 */
	*dh = PEM_read_DHparams (paramfile, NULL, NULL, NULL);
	if (*dh == NULL) {
		TRACE(ENTRIES, "Failed to load PEM %s\n", buf->buf);
		ret = ret_error;
		goto out;
	}

	ret = ret_ok;

out:
	/* Clean up
	 */
	if (paramfile != NULL) {
		fclose (paramfile);
	}

	cherokee_buffer_mrproper (&confentry);
	return ret;
}

static ret_t
_configure (cherokee_cryptor_t     *cryp,
            cherokee_config_node_t *conf,
            cherokee_server_t      *srv)
{
	ret_t ret;

	UNUSED(cryp);
	UNUSED(srv);

	ret = try_read_dh_param (conf, &dh_param_512, 512);
	if (ret != ret_ok)
		return ret;

	ret = try_read_dh_param (conf, &dh_param_1024, 1024);
	if (ret != ret_ok)
		return ret;

	ret = try_read_dh_param (conf, &dh_param_2048, 2048);
	if (ret != ret_ok)
		return ret;

	ret = try_read_dh_param (conf, &dh_param_4096, 4096);
	if (ret != ret_ok)
		return ret;

	return ret_ok;
}

static ret_t
_vserver_free (cherokee_cryptor_vserver_libssl_t *cryp_vsrv)
{
	if (cryp_vsrv->context != NULL) {
		SSL_CTX_free (cryp_vsrv->context);
		cryp_vsrv->context = NULL;
	}

	free (cryp_vsrv);
	return ret_ok;
}

ret_t
cherokee_cryptor_libssl_find_vserver (SSL *ssl,
                                      cherokee_server_t     *srv,
                                      cherokee_buffer_t     *servername,
                                      cherokee_connection_t *conn)
{
	ret_t                      ret;
	cherokee_virtual_server_t *vsrv = NULL;
	SSL_CTX                   *ctx;

	/* Try to match the connection to a server
	 */

	ret = cherokee_server_get_vserver(srv, servername, conn, &vsrv);
	if ((ret != ret_ok) || (vsrv == NULL)) {
		LOG_ERROR (CHEROKEE_ERROR_SSL_SRV_MATCH, servername->buf);
		return ret_error;
	}

	TRACE (ENTRIES, "Setting new TLS context. Virtual host='%s'\n",
	       vsrv->name.buf);

	/* Check whether the Virtual Server supports TLS
	 */
	if ((vsrv->cryptor == NULL) ||
	    (CRYPTOR_VSRV_SSL(vsrv->cryptor)->context == NULL))
	{
		TRACE (ENTRIES, "Virtual server '%s' does not support SSL\n", servername->buf);
		return ret_error;
	}

	/* Set the new SSL context
	 */
	ctx = SSL_set_SSL_CTX (ssl, CRYPTOR_VSRV_SSL(vsrv->cryptor)->context);
	if (ctx != CRYPTOR_VSRV_SSL(vsrv->cryptor)->context) {
		LOG_ERROR (CHEROKEE_ERROR_SSL_CHANGE_CTX, servername->buf);
	}

	/* SSL_set_SSL_CTX() only change certificates. We need to
	 * changes more options by hand.
	 */
	SSL_set_options(ssl, SSL_CTX_get_options(ssl->ctx));

	if ((SSL_get_verify_mode(ssl) == SSL_VERIFY_NONE) ||
	    (SSL_num_renegotiations(ssl) == 0)) {

		SSL_set_verify(ssl, SSL_CTX_get_verify_mode(ssl->ctx),
		               SSL_CTX_get_verify_callback(ssl->ctx));
	}

	return ret_ok;
}

#ifndef OPENSSL_NO_TLSEXT
static int
openssl_sni_servername_cb (SSL *ssl, int *ad, void *arg)
{
	ret_t                      ret;
	int                        re;
	const char                *servername;
	cherokee_connection_t     *conn;
	cherokee_buffer_t          tmp;
	cherokee_server_t         *srv       = SRV(arg);

	UNUSED(ad);

	/* Get the pointer to the socket
	 */
	conn = SSL_get_app_data (ssl);
	if (unlikely (conn == NULL)) {
		LOG_ERROR (CHEROKEE_ERROR_SSL_SOCKET, ssl);
		return SSL_TLSEXT_ERR_ALERT_FATAL;
	}

	cherokee_buffer_init(&tmp);

	/* Read the SNI server name
	 */
	servername = SSL_get_servername (ssl, TLSEXT_NAMETYPE_host_name);
	if (servername == NULL) {
		/* Set the server name to the IP address if we couldn't get the host name via SNI
		 */
		char ip_str[40];
		cherokee_sockaddr_t sa;
		socklen_t sa_len = sizeof(sa);

		re = getsockname (SOCKET_FD(&conn->socket),
		                  (struct sockaddr *) &sa,
		                  &sa_len);

		cherokee_ntop (sa.sa.sa_family, (struct sockaddr *) &sa, ip_str, sizeof(ip_str));
		cherokee_buffer_add (&tmp, ip_str, strlen(ip_str));
		TRACE (ENTRIES, "No SNI: Did not provide a server name, using IP='%s' as servername.\n", ip_str);
	} else {
		cherokee_buffer_add (&tmp, servername, strlen(servername));
		TRACE (ENTRIES, "SNI: Switching to servername='%s'\n", servername);
	}

	/* Look up and change the vserver
	 */
	ret = cherokee_cryptor_libssl_find_vserver(ssl, srv, &tmp, conn);
	if (ret != ret_ok) {
		re = SSL_TLSEXT_ERR_NOACK;
	}
	else {
		re = SSL_TLSEXT_ERR_OK;
	}

	cherokee_buffer_mrproper (&tmp);
	return re;
}
#endif

static DH *
tmp_dh_cb (SSL *ssl, int export, int keylen)
{
	UNUSED(ssl);
	UNUSED(export);

	switch (keylen) {
	case 512:
		if (dh_param_512 == NULL) {
			dh_param_512 = get_dh512();
		}
		return dh_param_512;

	case 1024:
		if (dh_param_1024 == NULL) {
			dh_param_1024 = get_dh1024();
		}
		return dh_param_1024;

	case 2048:
		if (dh_param_2048 == NULL) {
			dh_param_2048 = get_dh2048();
		}
		return dh_param_2048;

	case 4096:
		if (dh_param_4096 == NULL) {
			dh_param_4096 = get_dh4096();
		}
		return dh_param_4096;
	}
	return NULL;
}

#ifdef TRACE_ENABLED
static int
verify_trace_cb(int preverify_ok, X509_STORE_CTX *x509_store)
{
	X509 *peer_certificate = X509_STORE_CTX_get_current_cert(x509_store);
	if (peer_certificate) {
		BIO *mem = BIO_new(BIO_s_mem());
		char *ptr;
		X509_print (mem, peer_certificate);
		BIO_get_mem_data(mem, &ptr);
		TRACE (ENTRIES, "SSL: %s", ptr);
		BIO_free (mem);
	}

	return preverify_ok;
}
#endif

static int
verify_tolerate_cb(int preverify_ok, X509_STORE_CTX *x509_store)
{
#ifdef TRACE_ENABLED
	verify_trace_cb(preverify_ok, x509_store);
#endif
	return 1;
}

static ret_t
_vserver_new (cherokee_cryptor_t          *cryp,
              cherokee_virtual_server_t   *vsrv,
              cherokee_cryptor_vserver_t **cryp_vsrv)
{
	ret_t       ret;
	int         rc;
	const char *error;
	long        options;
	int         verify_mode = SSL_VERIFY_NONE;
#if !defined(OPENSSL_NO_EC) && OPENSSL_VERSION_NUMBER >= 0x0090800L && OPENSSL_VERSION_NUMBER < 0x10002000L
	EC_KEY *ecdh;
#endif

	CHEROKEE_NEW_STRUCT (n, cryptor_vserver_libssl);

	UNUSED(cryp);

	/* Init
	 */
	ret = cherokee_cryptor_vserver_init_base (CRYPTOR_VSRV(n));
	if (ret != ret_ok) {
		free (n);
		return ret;
	}

	CRYPTOR_VSRV(n)->free = (cryptor_vsrv_func_free_t) _vserver_free;

	/* Init the OpenSSL context
	 */
	n->context = SSL_CTX_new (SSLv23_server_method());
	if (n->context == NULL) {
		LOG_ERROR_S(CHEROKEE_ERROR_SSL_ALLOCATE_CTX);
		goto error;
	}

	/* Setup DH parameters
	 */
	switch (vsrv->ssl_dh_length)
	{
	case 512:
	case 1024:
	case 2048:
	case 4096:
		SSL_CTX_set_tmp_dh (n->context, tmp_dh_cb(NULL, 0, vsrv->ssl_dh_length));
		break;

	default:
		SSL_CTX_set_tmp_dh_callback (n->context, tmp_dh_cb);
	}

	/* Set ecliptic curve key parameters
	 */
#if !defined(OPENSSL_NO_EC) && OPENSSL_VERSION_NUMBER >= 0x10002000L
	/* OpenSSL >= 1.0.2 automatically handles ECDH temporary key parameter
	 * selection. */
	SSL_CTX_set_ecdh_auto(n->context, 1);
#elif !defined(OPENSSL_NO_EC) && OPENSSL_VERSION_NUMBER >= 0x0090800L
	/* For OpenSSL < 1.0.2, ECDH temporary key parameter selection must be
	 * performed manually. Default to the NIST P-384 (secp384r1) curve
	 * to be compliant with RFC 6460 when AES-256 TLS cipher suites are in
	 * use. This does make Cherokee non-compliant with RFC 6460 when
	 * AES-128 TLS cipher suites are in use as they "MUST" support
	 * NIST P-256 (prime256v1) but only "SHOULD" support NIST P-384
	 * (secp384v1). However 99.9% of clients support both or neither.
	 */
	ecdh = EC_KEY_new_by_curve_name(NID_secp384r1);
	if (ecdh != NULL) {
		SSL_CTX_set_tmp_ecdh(n->context, ecdh);
		EC_KEY_free(ecdh);
	}
#endif

	/* Set the SSL context options:
	 */
	options  = SSL_OP_ALL;
	options |= SSL_OP_SINGLE_DH_USE;
#ifdef SSL_OP_SINGLE_ECDH_USE
	options |= SSL_OP_SINGLE_ECDH_USE;
#endif

#ifdef SSL_OP_DONT_INSERT_EMPTY_FRAGMENTS
	options |= SSL_OP_DONT_INSERT_EMPTY_FRAGMENTS;
#endif

	if (! cryp->allow_SSLv2) {
		options |= SSL_OP_NO_SSLv2;
	}

	if (! cryp->allow_SSLv3) {
		options |= SSL_OP_NO_SSLv3;
	}

	if (! cryp->allow_TLSv1) {
		options |= SSL_OP_NO_TLSv1;
	}

#if OPENSSL_VERSION_NUMBER >= 0x10001000L
	if (! cryp->allow_TLSv1_1) {
		options |= SSL_OP_NO_TLSv1_1;
	}

	if (! cryp->allow_TLSv1_2) {
		options |= SSL_OP_NO_TLSv1_2;
	}
#endif

#ifdef SSL_OP_CIPHER_SERVER_PREFERENCE
	if (vsrv->cipher_server_preference) {
		options |= SSL_OP_CIPHER_SERVER_PREFERENCE;
	}
#endif

#ifndef OPENSSL_NO_COMP
	if (! vsrv->ssl_compression) {
#ifdef SSL_OP_NO_COMPRESSION
		options |= SSL_OP_NO_COMPRESSION;
#elif OPENSSL_VERSION_NUMBER >= 0x00908000L
		sk_SSL_COMP_zero(SSL_COMP_get_compression_methods());
#endif
	}
#endif

	SSL_CTX_set_options (n->context, options);

	/* Set cipher list that vserver will accept.
	 */
	if (! cherokee_buffer_is_empty (&vsrv->ciphers)) {
		rc = SSL_CTX_set_cipher_list (n->context, vsrv->ciphers.buf);
		if (rc != 1) {
			OPENSSL_LAST_ERROR(error);
			LOG_ERROR(CHEROKEE_ERROR_SSL_CIPHER,
			          vsrv->ciphers.buf, error);
			goto error;
		}
	}

	CLEAR_LIBSSL_ERRORS;

	/* Certificate
	 */
	TRACE(ENTRIES, "Vserver '%s'. Reading certificate file '%s'\n",
	      vsrv->name.buf, vsrv->server_cert.buf);

	rc = SSL_CTX_use_certificate_chain_file (n->context, vsrv->server_cert.buf);
	if (rc != 1) {
		OPENSSL_LAST_ERROR(error);
		LOG_ERROR(CHEROKEE_ERROR_SSL_CERTIFICATE,
		          vsrv->server_cert.buf, error);
		goto error;
	}

	/* Private key
	 */
	TRACE(ENTRIES, "Vserver '%s'. Reading key file '%s'\n",
	      vsrv->name.buf, vsrv->server_key.buf);

	rc = SSL_CTX_use_PrivateKey_file (n->context, vsrv->server_key.buf, SSL_FILETYPE_PEM);
	if (rc != 1) {
		OPENSSL_LAST_ERROR(error);
		LOG_ERROR(CHEROKEE_ERROR_SSL_KEY, vsrv->server_key.buf, error);
		goto error;
	}

	/* Check private key
	 */
	rc = SSL_CTX_check_private_key (n->context);
	if (rc != 1) {
		LOG_ERROR_S(CHEROKEE_ERROR_SSL_KEY_MATCH);
		goto error;
	}

	if (vsrv->req_client_certs != req_client_cert_skip) {
		STACK_OF(X509_NAME) *X509_clients;

		verify_mode = SSL_VERIFY_PEER | SSL_VERIFY_CLIENT_ONCE;
		if (vsrv->req_client_certs == req_client_cert_require) {
			verify_mode |= SSL_VERIFY_FAIL_IF_NO_PEER_CERT;
		}

		/* Trusted CA certificates
		*/
		if (! cherokee_buffer_is_empty (&vsrv->certs_ca)) {
			rc = SSL_CTX_load_verify_locations (n->context, vsrv->certs_ca.buf, NULL);
			if (rc != 1) {
				OPENSSL_LAST_ERROR(error);
				LOG_CRITICAL(CHEROKEE_ERROR_SSL_CA_READ,
				             vsrv->certs_ca.buf, error);
				goto error;
			}

			X509_clients = SSL_load_client_CA_file (vsrv->certs_ca.buf);
			if (X509_clients == NULL) {
				OPENSSL_LAST_ERROR(error);
				LOG_CRITICAL (CHEROKEE_ERROR_SSL_CA_LOAD,
				              vsrv->certs_ca.buf, error);
				goto error;
			}

			CLEAR_LIBSSL_ERRORS;

			SSL_CTX_set_client_CA_list (n->context, X509_clients);
			TRACE (ENTRIES, "Setting client CA list: %s on '%s'\n", vsrv->certs_ca.buf, vsrv->name.buf);
		} else {
			verify_mode = SSL_VERIFY_NONE;
		}

	}

	if (vsrv->req_client_certs == req_client_cert_tolerate) {
		SSL_CTX_set_verify (n->context, verify_mode, verify_tolerate_cb);
	} else {
#ifdef TRACE_ENABLED
		SSL_CTX_set_verify (n->context, verify_mode, verify_trace_cb);
#else
		SSL_CTX_set_verify (n->context, verify_mode, NULL);
#endif
	}

	SSL_CTX_set_verify_depth (n->context, vsrv->verify_depth);

	SSL_CTX_set_read_ahead (n->context, 1);
	SSL_CTX_set_mode (n->context,
	                  SSL_CTX_get_mode(n->context) | SSL_MODE_ACCEPT_MOVING_WRITE_BUFFER);

	/* Set the SSL context cache
	 */
	rc = SSL_CTX_set_session_id_context (n->context,
	                                     (unsigned char *) vsrv->name.buf,
	                                     MIN(SSL_MAX_SSL_SESSION_ID_LENGTH, (unsigned int) vsrv->name.len));
	if (rc != 1) {
		OPENSSL_LAST_ERROR(error);
		LOG_ERROR (CHEROKEE_ERROR_SSL_SESSION_ID, vsrv->name.buf, error);
	}

	SSL_CTX_set_session_cache_mode (n->context, SSL_SESS_CACHE_SERVER);


#ifndef OPENSSL_NO_TLSEXT
	/* Enable SNI
	 */
	rc = SSL_CTX_set_tlsext_servername_callback (n->context, openssl_sni_servername_cb);
	if (rc != 1) {
		OPENSSL_LAST_ERROR(error);
		LOG_WARNING (CHEROKEE_ERROR_SSL_SNI, vsrv->name.buf, error);
	} else {
		rc = SSL_CTX_set_tlsext_servername_arg (n->context, VSERVER_SRV(vsrv));
		if (rc != 1) {
			OPENSSL_LAST_ERROR(error);
			LOG_WARNING (CHEROKEE_ERROR_SSL_SNI, vsrv->name.buf, error);
		}
	}
#endif /* OPENSSL_NO_TLSEXT */

	*cryp_vsrv = CRYPTOR_VSRV(n);
	return ret_ok;

error:
	if (n->context != NULL) {
		SSL_CTX_free (n->context);
		n->context = NULL;
	}

	free (n);
	return ret_error;
}


static ret_t
socket_initialize (cherokee_cryptor_socket_libssl_t *cryp,
                   cherokee_socket_t                *socket,
                   cherokee_virtual_server_t        *vserver,
                   cherokee_connection_t            *conn)
{
	int                                re;
	const char                        *error;
	cherokee_cryptor_vserver_libssl_t *vsrv_crytor = CRYPTOR_VSRV_SSL(vserver->cryptor);

#ifdef OPENSSL_NO_TLSEXT
	cherokee_buffer_t                  servername = CHEROKEE_BUF_INIT;
#endif

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
		OPENSSL_LAST_ERROR(error);
		LOG_ERROR (CHEROKEE_ERROR_SSL_CONNECTION, error);
		return ret_error;
	}

	/* Sets ssl to work in server mode
	 */
	SSL_set_accept_state (cryp->session);

	/* Set the socket file descriptor
	 */
	re = SSL_set_fd (cryp->session, socket->socket);
	if (re != 1) {
		OPENSSL_LAST_ERROR(error);
		LOG_ERROR (CHEROKEE_ERROR_SSL_FD,
		           socket->socket, error);
		return ret_error;
	}

	cryp->is_pending = false;

#ifndef OPENSSL_NO_TLSEXT
	SSL_set_app_data (cryp->session, conn);
#else
	/* Attempt to determine the vserver without SNI.
	 */
	cherokee_buffer_ensure_size(&servername, 40);
	cherokee_socket_ntop (&conn->socket, servername.buf, servername.size);
	cherokee_cryptor_libssl_find_vserver (cryp->session, CONN_SRV(conn), &servername, conn);
	cherokee_buffer_mrproper (&servername);
#endif

	return ret_ok;
}


static ret_t
_socket_init_tls (cherokee_cryptor_socket_libssl_t *cryp,
                  cherokee_socket_t                *sock,
                  cherokee_virtual_server_t        *vsrv,
                  cherokee_connection_t            *conn,
                  cherokee_socket_status_t         *blocking)
{
	int   re;
	ret_t ret;

	/* Initialize
	 */
	if (CRYPTOR_SOCKET(cryp)->initialized == false) {
		ret = socket_initialize (cryp, sock, vsrv, conn);
		if (ret != ret_ok) {
			return ret_error;
		}

		CRYPTOR_SOCKET(cryp)->initialized = true;
	}

	/* TLS Handshake
	 */
	CLEAR_LIBSSL_ERRORS;

	re = SSL_do_handshake (cryp->session);
	if (re == 0) {
		/* The TLS/SSL handshake was not successful but was
		 * shut down controlled and by the specifications of
		 * the TLS/SSL protocol.
		 */
		return ret_eof;

	} else if (re <= 0) {
		int         err;
		const char *error;
		int         err_sys = errno;

		err = SSL_get_error (cryp->session, re);
		switch (err) {
		case SSL_ERROR_WANT_READ:
			*blocking = socket_reading;
			return ret_eagain;

		case SSL_ERROR_WANT_WRITE:
		case SSL_ERROR_WANT_CONNECT:
		case SSL_ERROR_WANT_ACCEPT:
			*blocking = socket_writing;
			return ret_eagain;

		case SSL_ERROR_SYSCALL:
			if (err_sys == EAGAIN) {
				return ret_eagain;
			}
			return ret_error;

		case SSL_ERROR_SSL:
		case SSL_ERROR_ZERO_RETURN:
			return ret_error;
		default:
			OPENSSL_LAST_ERROR(error);
			LOG_ERROR (CHEROKEE_ERROR_SSL_INIT, error);
			return ret_error;
		}
	}

	/* Report the connection details
	 */
#ifdef TRACE_ENABLED
	{
		CHEROKEE_TEMP (buf,256);
		const SSL_CIPHER *cipher = SSL_get_current_cipher (cryp->session);

		if (cipher) {
			SSL_CIPHER_description (cipher, &buf[0], buf_size-1);

			TRACE (ENTRIES, "SSL: %s, %sREUSED, Ciphers: %s",
			       SSL_get_version(cryp->session),
			       SSL_session_reused(cryp->session)? "" : "Not ", &buf[0]);
		}
	}
#endif

	/* Disable Ciphers renegotiation (CVE-2009-3555)
	 */
	if (cryp->session->s3) {
		cryp->session->s3->flags |= SSL3_FLAGS_NO_RENEGOTIATE_CIPHERS;
	}

	return ret_ok;
}

static ret_t
_socket_shutdown (cherokee_cryptor_socket_libssl_t *cryp)
{
	int re;
	int ssl_error;

	if (unlikely (cryp->session == NULL)) {
		return ret_ok;
	}

	/* Send a 'close_notify' SSL message
	 */
	errno = 0;
	CLEAR_LIBSSL_ERRORS;

	re = SSL_shutdown (cryp->session);
	if (re == 1) {
		/* The shutdown was successfully completed. */
		return ret_ok;

	} else if (re == 0) {
		/* Shutdown in progress */
		int ssl_err = SSL_get_error(cryp->session, re);

		/* According to OpenSSL bug #611, something bad might
		 * have happened during the call to SSL_shutdown()..
		 */
		switch (ssl_err) {
		case SSL_ERROR_ZERO_RETURN:
			return ret_ok;
		case SSL_ERROR_SYSCALL: {
			unsigned long e = ERR_get_error();

			if (e == 0) {
				/* EOF occurred in violation of protocol */
				return ret_eof;

			} else if (e == -1) {
				/* Underlying BIO reported an I/O error */
				switch (errno) {
				case EINTR:
				case EAGAIN:
					return ret_eagain;
				case EIO:
				case EPIPE:
				case ECONNRESET:
					return ret_eof;
				default:
					return ret_error;
				}
			}
			return ret_error;
		}
		default:
			return ret_error;
		}

	} else if (re < 0) {
		ssl_error = SSL_get_error (cryp->session, re);
		switch (ssl_error) {
		case SSL_ERROR_ZERO_RETURN:
			return ret_ok;

		case SSL_ERROR_WANT_READ:
			return ret_eagain;
		case SSL_ERROR_WANT_WRITE:
			return ret_eagain;

		case SSL_ERROR_SYSCALL:
			CLEAR_LIBSSL_ERRORS;

			switch (errno) {
			case 0:
				return ret_ok;
			case EINTR:
			case EAGAIN:
				return ret_eagain;
			default:
				return ret_error;
			}
			break;

		default:
			return ret_error;
		}
	}

	SHOULDNT_HAPPEN;
	return ret_error;
}

static ret_t
_socket_write (cherokee_cryptor_socket_libssl_t *cryp,
               char                             *buf,
               int                               buf_len,
               size_t                           *pcnt_written)
{
	int     re;
	ssize_t len;
	int     error;

	/* 'Truco del almendruco': This is a method to bypass the
	 * limitations of the libssl sockets regard to SSL_write.
	 *
	 * A buffer being sent cannot be modified. Since Cherokee's
	 * core does that by default (it removes the chunk of info
	 * already sent), we have to trick it. This piece of code
	 * reports EAGAIN errors until the complete buffer is sent.
	 */

	/* Stage 1: Keep track of the buffer
	 */
	if (cryp->writing.buf != buf) {
		TRACE (ENTRIES, "SSL-Write. Sets new buffer: %p (len %d)\n", buf, buf_len);

		cryp->writing.buf     = buf;
		cryp->writing.buf_len = buf_len;
		cryp->writing.written = 0;
	}

	/* Proceed to write
	 */
	CLEAR_LIBSSL_ERRORS;

	len = SSL_write (cryp->session, buf, buf_len);
	if (likely (len > 0) ) {

		/* Stage 2: Lie, if required
		 */
		cryp->writing.written += len;

		if (cryp->writing.written >= buf_len) {
			TRACE (ENTRIES, "SSL-Write. Buffer sent: %p (total len %d)\n", buf, buf_len);
			*pcnt_written = buf_len;
			return ret_ok;
		}

		TRACE (ENTRIES",lie", "SSL-Write lies, (package %d, written %d, total %d): eagain\n", len, cryp->writing.written, buf_len);
		return ret_eagain;
	}

	if (len == 0) {
		TRACE (ENTRIES",write", "write got %s\n", "EOF");
		return ret_eof;
	}

	/* len < 0 */
	error = errno;

	re = SSL_get_error (cryp->session, len);
	switch (re) {
	case SSL_ERROR_WANT_READ:
	case SSL_ERROR_WANT_WRITE:
		TRACE (ENTRIES",write", "write len=%d (written=0), EAGAIN\n", buf_len);
		return ret_eagain;

	case SSL_ERROR_SYSCALL:
		switch (error) {
		case EAGAIN:
			TRACE (ENTRIES",write", "write len=%d (written=0), EAGAIN\n", buf_len);
			return ret_eagain;
#ifdef ENOTCONN
		case ENOTCONN:
#endif
		case EPIPE:
		case ECONNRESET:
			TRACE (ENTRIES",write", "write len=%d (written=0), EOF\n", buf_len);
			return ret_eof;
		default:
			LOG_ERRNO_S (error, cherokee_err_error,
			             CHEROKEE_ERROR_SSL_SW_DEFAULT);
		}

		TRACE (ENTRIES",write", "write len=%d (written=0), ERROR: %s\n", buf_len, ERR_error_string(re, NULL));
		return ret_error;

	case SSL_ERROR_SSL:
		TRACE (ENTRIES",write", "write len=%d (written=0), ERROR: %s\n", buf_len, ERR_error_string(re, NULL));
		return ret_error;
	}

	LOG_ERROR (CHEROKEE_ERROR_SSL_SW_ERROR,
	           SSL_get_fd(cryp->session), (int)len, ERR_error_string(re, NULL));

	TRACE (ENTRIES",write", "write len=%d (written=0), ERROR: %s\n", buf_len, ERR_error_string(re, NULL));
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
	ssize_t len    = 0;

	CLEAR_LIBSSL_ERRORS;

	*pcnt_read = 0;

	while (buf_size > 0) {
		len = SSL_read (cryp->session, buf, buf_size);
		if (len < 1)
			break;
		*pcnt_read += len;
		buf += len;
		buf_size -= len;
	}

	/* We have more data than buffer space. Mark the socket as
	 * having pending data. */
	cryp->is_pending = (buf_size == 0);

	if (*pcnt_read > 0) {
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
		case EAGAIN:
			return ret_eagain;
		case EPIPE:
		case ECONNRESET:
			return ret_eof;
		default:
			LOG_ERRNO_S (error, cherokee_err_error,
			             CHEROKEE_ERROR_SSL_SR_DEFAULT);
		}
		return ret_error;
	}

	LOG_ERROR (CHEROKEE_ERROR_SSL_SR_ERROR,
	           SSL_get_fd(cryp->session), (int)len, ERR_error_string(re, NULL));
	return ret_error;
}

static int
_socket_pending (cherokee_cryptor_socket_libssl_t *cryp)
{
	return cryp->is_pending;
}

static ret_t
_socket_clean (cherokee_cryptor_socket_libssl_t *cryp_socket)
{
	cherokee_cryptor_socket_clean_base (CRYPTOR_SOCKET(cryp_socket));

	if (cryp_socket->session != NULL) {
		SSL_free (cryp_socket->session);
		cryp_socket->session = NULL;
	}

	if (cryp_socket->ssl_ctx != NULL) {
		SSL_CTX_free (cryp_socket->ssl_ctx);
		cryp_socket->ssl_ctx = NULL;
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

	UNUSED(cryp);

	ret = cherokee_cryptor_socket_init_base (CRYPTOR_SOCKET(n));
	if (unlikely (ret != ret_ok))
		return ret;

	/* Socket properties */
	n->session = NULL;
	n->ssl_ctx = NULL;

	n->writing.buf     = NULL;
	n->writing.buf_len = -1;
	n->writing.written = -1;

	/* Virtual methods */
	CRYPTOR_SOCKET(n)->free     = (cryptor_socket_func_free_t) _socket_free;
	CRYPTOR_SOCKET(n)->clean    = (cryptor_socket_func_clean_t) _socket_clean;
	CRYPTOR_SOCKET(n)->init_tls = (cryptor_socket_func_init_tls_t) _socket_init_tls;
	CRYPTOR_SOCKET(n)->shutdown = (cryptor_socket_func_shutdown_t) _socket_shutdown;
	CRYPTOR_SOCKET(n)->read     = (cryptor_socket_func_read_t) _socket_read;
	CRYPTOR_SOCKET(n)->write    = (cryptor_socket_func_write_t) _socket_write;
	CRYPTOR_SOCKET(n)->pending  = (cryptor_socket_func_pending_t) _socket_pending;

	*cryp_socket = n;
	return ret_ok;
}

static ret_t
_client_init_tls (cherokee_cryptor_client_libssl_t *cryp,
                  cherokee_buffer_t                *host,
                  cherokee_socket_t                *socket)
{
	int         re;
	const char *error;

	/* New context
	 */
	cryp->ssl_ctx = SSL_CTX_new (SSLv23_client_method());
	if (cryp->ssl_ctx == NULL) {
		OPENSSL_LAST_ERROR(error);
		LOG_CRITICAL (CHEROKEE_ERROR_SSL_CREATE_CTX, error);
		return ret_error;
	}

#if 0
	/* CA verifications
	 */
	re = cherokee_buffer_is_empty (&cryp->vserver_ref->certs_ca);
	if (! re) {
		re = SSL_CTX_load_verify_locations (socket->ssl_ctx,
		                                    socket->vserver_ref->certs_ca.buf, NULL);
		if (! re) {
			OPENSSL_LAST_ERROR(error);
			LOG_ERROR (CHEROKEE_ERROR_SSL_CTX_LOAD,
			           socket->vserver_ref->certs_ca.buf, error);
			return ret_error;
		}

		re = SSL_CTX_set_default_verify_paths (socket->ssl_ctx);
		if (! re) {
			OPENSSL_LAST_ERROR(error);
			LOG_ERROR (CHEROKEE_ERROR_SSL_CTX_SET, error);
			return ret_error;
		}
	}
#endif

	SSL_CTX_set_verify (cryp->ssl_ctx, SSL_VERIFY_NONE, NULL);

	/* New session
	 */
	cryp->session = SSL_new (cryp->ssl_ctx);
	if (cryp->session == NULL) {
		OPENSSL_LAST_ERROR(error);
		LOG_ERROR (CHEROKEE_ERROR_SSL_CONNECTION, error);
		return ret_error;
	}

	/* Set the socket file descriptor
	 */
	re = SSL_set_fd (cryp->session, socket->socket);
	if (re != 1) {
		OPENSSL_LAST_ERROR(error);
		LOG_ERROR (CHEROKEE_ERROR_SSL_FD, socket->socket, error);
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
			LOG_ERROR (CHEROKEE_ERROR_SSL_SNI_SRV, error);
			return ret_error;
		}
	}
#endif

	re = SSL_connect (cryp->session);
	if (re <= 0) {
		OPENSSL_LAST_ERROR(error);
		LOG_ERROR (CHEROKEE_ERROR_SSL_CONNECT, error);
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

	UNUSED(cryp);

	ret = cherokee_cryptor_socket_init_base (CRYPTOR_SOCKET(n));
	if (unlikely (ret != ret_ok))
		return ret;

	/* Client properties */
	n->session = NULL;
	n->ssl_ctx = NULL;

	/* Socket */
	CRYPTOR_SOCKET(n)->shutdown = (cryptor_socket_func_shutdown_t) _socket_shutdown;
	CRYPTOR_SOCKET(n)->read     = (cryptor_socket_func_read_t) _socket_read;
	CRYPTOR_SOCKET(n)->write    = (cryptor_socket_func_write_t) _socket_write;
	CRYPTOR_SOCKET(n)->pending  = (cryptor_socket_func_pending_t) _socket_pending;
	CRYPTOR_SOCKET(n)->clean    = (cryptor_socket_func_clean_t) _socket_clean;

	/* Client */
	CRYPTOR_SOCKET(n)->free     = (cryptor_socket_func_free_t) _client_free;
	CRYPTOR_SOCKET(n)->init_tls = (cryptor_socket_func_init_tls_t) _client_init_tls;

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



/* Private low-level functions for OpenSSL
 */
static pthread_mutex_t *locks;
static size_t           locks_num;

static unsigned long
__get_thread_id (void)
{
	return (unsigned long) pthread_self();
}


static void
__lock_thread (int mode, int n, const char *file, int line)
{
	UNUSED (file);
	UNUSED (line);

	if (mode & CRYPTO_LOCK) {
		CHEROKEE_MUTEX_LOCK (&locks[n]);
	} else {
		CHEROKEE_MUTEX_UNLOCK (&locks[n]);
	}
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

	/* Do not initialize the library twice
	 */
	PLUGIN_INIT_ONCE_CHECK (libssl);

	/* Init OpenSSL
	 */
	OPENSSL_config (NULL);
	SSL_library_init();
	SSL_load_error_strings();
	OpenSSL_add_all_algorithms();

	/* Ensure PRNG has been seeded with enough data
	 */
	if (RAND_status() == 0) {
		LOG_WARNING_S (CHEROKEE_ERROR_SSL_NO_ENTROPY);
	}

	/* Init concurrency related stuff
	 */
	if ((CRYPTO_get_id_callback()      == NULL) &&
	    (CRYPTO_get_locking_callback() == NULL))
	{
		cuint_t n;

		CRYPTO_set_id_callback (__get_thread_id);
		CRYPTO_set_locking_callback (__lock_thread);

		locks_num = CRYPTO_num_locks();
		locks     = malloc (locks_num * sizeof(*locks));

		for (n = 0; n < locks_num; n++) {
			CHEROKEE_MUTEX_INIT (&locks[n], NULL);
		}
	}


# if HAVE_OPENSSL_ENGINE_H
#  if OPENSSL_VERSION_NUMBER >= 0x00907000L
	ENGINE_load_builtin_engines();
	OpenSSL_add_all_algorithms();
#  endif
	e = ENGINE_by_id("pkcs11");
	while (e != NULL) {
		if(! ENGINE_init(e)) {
			ENGINE_free (e);
			LOG_CRITICAL_S (CHEROKEE_ERROR_SSL_PKCS11);
			break;
		}

		if(! ENGINE_set_default(e, ENGINE_METHOD_ALL)) {
			ENGINE_free (e);
			LOG_CRITICAL_S (CHEROKEE_ERROR_SSL_DEFAULTS);
			break;
		}

		ENGINE_finish(e);
		ENGINE_free(e);
		break;
	}
#endif
}
