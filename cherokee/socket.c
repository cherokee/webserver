/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/* Cherokee
 *
 * Authors:
 *      Alvaro Lopez Ortega <alvaro@alobbs.com>
 *
 * Some patches by:
 *      Ricardo Cardenes Medina <ricardo@conysis.com>
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
#include "socket.h"

#include <sys/types.h>

#ifdef HAVE_NETINET_TCP_H
# include <netinet/tcp.h>
#endif

#ifdef HAVE_SYS_FILIO_H
# include <sys/filio.h>     /* defines FIONBIO and FIONREAD */
#endif

#ifdef HAVE_SYS_SOCKIO_H
# include <sys/sockio.h>    /* defines SIOCATMARK */
#endif

#ifdef HAVE_SYS_SOCKET_H
# include <sys/socket.h> 
#endif

#ifdef HAVE_SYS_IOCTL_H
# include <sys/ioctl.h>
#endif

#ifdef HAVE_NETDB_H
# include <netdb.h>
#endif

#ifdef HAVE_SYS_TIME_H
# include <sys/time.h>
#else 
# include <time.h>
#endif

/* sendfile() function related includes
 */
#if defined(LINUX_SENDFILE_API) 
# include <sys/sendfile.h>

#elif defined(SOLARIS_SENDFILE_API)
# include <sys/sendfile.h>

#elif defined(HPUX_SENDFILE_API)
# include <sys/socket.h>

#elif defined(FREEBSD_SENDFILE_API)
# include <sys/types.h>
# include <sys/socket.h>

#elif defined(LINUX_BROKEN_SENDFILE_API)
extern int32_t sendfile (int out_fd, int in_fd, int32_t *offset, uint32_t count);
#endif

#ifdef HAVE_SYS_UIO_H
# include <sys/uio.h>		/* sendfile and writev() */
#endif

#if !defined(TCP_CORK) && defined(TCP_NOPUSH)
#define TCP_CORK TCP_NOPUSH
#endif

#define ENTRIES "socket"


#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>

#include "util.h"
#include "buffer.h"
#include "virtual_server.h"


/* Max. sendfile block size (bytes), limiting size of data sent by
 * each sendfile call may: improve responsiveness, reduce memory
 * pressure, prevent blocking calls, etc.
 * 
 * NOTE: try to reduce block size (down to 64 KB) on small or old
 * systems and see if something improves under heavy load.
 */
#define MAX_SF_BLK_SIZE		(65536 * 16)	          /* limit size of block size */
#define MAX_SF_BLK_SIZE2	(MAX_SF_BLK_SIZE + 65536) /* upper limit */


ret_t
cherokee_socket_init (cherokee_socket_t *socket)
{
	memset (&socket->client_addr, 0, sizeof(cherokee_sockaddr_t));
	socket->client_addr_len = -1;

	socket->socket          = -1;
	socket->status          = socket_closed;
	socket->is_tls          = non_TLS;

#ifdef HAVE_TLS
	socket->initialized     = false;
	socket->vserver_ref     = NULL;
#endif

#if   defined (HAVE_GNUTLS)
	socket->session         = NULL;
#elif defined (HAVE_OPENSSL)
	socket->session         = NULL;
	socket->ssl_ctx         = NULL;
#endif

	return ret_ok;
}


ret_t
cherokee_socket_mrproper (cherokee_socket_t *socket)
{
#if   defined (HAVE_GNUTLS)
	if (socket->session != NULL) {
		gnutls_deinit (socket->session);
		socket->session = NULL;
	}
#elif defined (HAVE_OPENSSL)
	if (socket->session != NULL) {
		SSL_free (socket->session);
		socket->session = NULL;
	}

	if (socket->ssl_ctx != NULL) {
		SSL_CTX_free (socket->ssl_ctx);
		socket->ssl_ctx = NULL;
	}
#else
	UNUSED (socket);
#endif

	return ret_ok;
}


ret_t 
cherokee_socket_clean (cherokee_socket_t *socket)
{
	socket->socket   = -1;
	socket->status   = socket_closed;
	socket->is_tls   = non_TLS;

	/* Client address
	 */
	socket->client_addr_len = -1;
	memset (&socket->client_addr, 0, sizeof(cherokee_sockaddr_t));

#ifdef HAVE_TLS
	socket->initialized = false;
	socket->vserver_ref = NULL;
#endif

#if   defined (HAVE_GNUTLS)
	if (socket->session != NULL) {
		gnutls_deinit (socket->session);
		socket->session = NULL;
	}
#elif defined (HAVE_OPENSSL)
	if (socket->session != NULL) {
		SSL_free (socket->session);
		socket->session = NULL;
	}

	if (socket->ssl_ctx != NULL) {
		SSL_CTX_free (socket->ssl_ctx);
		socket->ssl_ctx = NULL;
	}
#endif

	return ret_ok;
}


#ifdef HAVE_GNUTLS

static const gnutls_datum err_datum = { NULL, 0 };

static gnutls_datum
db_retrieve (void *ptr, gnutls_datum key)
{
	ret_t                      ret;
	cherokee_avl_r_t          *cache;
	cherokee_virtual_server_t *vserver;
	gnutls_datum               session;
	cherokee_buffer_t          faked;
	cherokee_buffer_t          strkey  = CHEROKEE_BUF_INIT;
	cherokee_socket_t         *socket  = SOCKET(ptr);
	
	/* Sanity check
	 */
	vserver = socket->vserver_ref;
	if (unlikely (vserver == NULL)) {
		PRINT_ERROR_S ("No virtual server reference\n");
		return err_datum;
	}

	cache = &vserver->session_cache;

	/* Build the key string
	 */
	cherokee_buffer_fake (&faked, (char *)key.data, key.size);
	ret = cherokee_buffer_encode_hex (&faked, &strkey);
	if (ret != ret_ok) {
		cherokee_buffer_mrproper (&strkey);
		return err_datum;
	}

	/* Get (and remove) the object from the session cache
	 */
	ret = cherokee_avl_r_get (cache, &strkey, (void **)&session);
	if (ret == ret_ok) {
		TRACE (ENTRIES",ssl", "fd=%d key=%s - found=%p\n", socket->socket, strkey.buf, session);
		cherokee_buffer_mrproper (&strkey);
		return session;
	}

	/* Failure
	 */
	TRACE (ENTRIES",ssl", "fd=%d key=%s - error=%d\n", socket->socket, strkey.buf, ret);
	cherokee_buffer_mrproper (&strkey);
	return err_datum; 
}

static int
db_remove (void *ptr, gnutls_datum key)
{
	ret_t                      ret;
	cherokee_avl_r_t          *cache;
	cherokee_virtual_server_t *vserver;
	cherokee_buffer_t          faked;
	cherokee_buffer_t          strkey  = CHEROKEE_BUF_INIT;
	gnutls_datum              *n      = NULL;
	cherokee_socket_t         *socket = SOCKET(ptr);

	/* Sanity check
	 */
	vserver = socket->vserver_ref;
	if (unlikely (vserver == NULL)) {
		PRINT_ERROR_S ("No virtual server reference\n");
		return 1;
	}

	cache = &vserver->session_cache;

	/* Build the key string
	 */
	cherokee_buffer_fake (&faked, (char *)key.data, key.size);
	ret = cherokee_buffer_encode_hex (&faked, &strkey);
	if (ret != ret_ok) {
		cherokee_buffer_mrproper (&strkey);
		return -1;
	}

	/* Remove the entry
	 */
	ret = cherokee_avl_r_del (cache, &strkey, (void **)&n);
	if ((ret == ret_ok) && (n != NULL)) {
		TRACE (ENTRIES",ssl", "fd=%d key=%s - found=%p ret=%d\n", socket->socket, strkey.buf, n, ret);
		cherokee_buffer_mrproper (&strkey);
		gnutls_free (n->data);
		gnutls_free (n);
		return 0;
	}

	TRACE (ENTRIES",ssl", "fd=%d key=%s - exiting ret=%d\n", socket->socket, strkey.buf, ret);
	cherokee_buffer_mrproper (&strkey);
	return -1;
}

static int
db_store (void *ptr, gnutls_datum key, gnutls_datum data)
{
	ret_t                      ret;
	cherokee_avl_r_t          *cache;
	cherokee_virtual_server_t *vserver;
	cherokee_buffer_t          faked;
	cherokee_buffer_t          strkey = CHEROKEE_BUF_INIT;
	cherokee_socket_t         *socket = SOCKET(ptr);

	/* Remove the entry
	 */
	vserver = socket->vserver_ref;
	if (vserver == NULL) {
		PRINT_ERROR_S ("No virtual server reference\n");
		return 1;
	}

	cache = &vserver->session_cache;

	/* Build the key string
	 */
	cherokee_buffer_fake (&faked, (char *)key.data, key.size);
	ret = cherokee_buffer_encode_hex (&faked, &strkey);
	if (ret != ret_ok) {
		cherokee_buffer_mrproper (&strkey);
		return -1;
	}

	/* Add the copy to the table
	 */
	ret = cherokee_avl_r_add (cache, &strkey, &data);

	TRACE (ENTRIES",ssl", "fd=%d key=%s - added=%p ret=%d\n", socket->socket, strkey.buf, &data, ret);
	cherokee_buffer_mrproper (&strkey);
	return (ret == ret_ok) ? 0 : -1;
}

#endif /* HAVE_GNUTLS */



#ifdef HAVE_TLS

static ret_t
initialize_tls_session (cherokee_socket_t         *socket, 
			cherokee_virtual_server_t *vserver)
{
# if defined(HAVE_GNUTLS)
	int re;
	
	/* Set the virtual server object reference
	 */
	socket->vserver_ref = vserver;

	/* Init the TLS session
	 */
        re = gnutls_init (&socket->session, GNUTLS_SERVER);
	if (unlikely (re != GNUTLS_E_SUCCESS))
		return ret_error;

	gnutls_session_set_ptr (socket->session, socket);

	/* Set the socket file descriptor
	 */
  	gnutls_transport_set_ptr (socket->session, (gnutls_transport_ptr)socket->socket);

	/* Load the credentials
	 */
	gnutls_set_default_priority (socket->session);   

	{
		static const int kx_priority[] = { GNUTLS_KX_RSA, 
						   GNUTLS_KX_DHE_DSS, 
						   GNUTLS_KX_DHE_RSA, 
						   GNUTLS_KX_ANON_DH, 0 };

		gnutls_kx_set_priority (socket->session, kx_priority);
	}

	/* Set the virtual host credentials
	 */
 	gnutls_credentials_set (socket->session, GNUTLS_CRD_ANON, vserver->credentials);
 	gnutls_credentials_set (socket->session, GNUTLS_CRD_CERTIFICATE, vserver->credentials);
	
	/* Set the number of bits, for use in an Diffie Hellman key
	 * exchange: minimum size of the prime that will be used for
	 * the handshake.
	 */
	gnutls_dh_set_prime_bits (socket->session, 1024);

	/* Set the session DB management functions
	 */
	gnutls_db_set_retrieve_function (socket->session, db_retrieve);
	gnutls_db_set_remove_function   (socket->session, db_remove);
	gnutls_db_set_store_function    (socket->session, db_store);
	gnutls_db_set_ptr               (socket->session, socket);

	/* Request client certificate if any.
	 */
	gnutls_certificate_server_set_request (socket->session, GNUTLS_CERT_REQUEST);
	gnutls_handshake_set_private_extensions (socket->session, 1);

# elif defined (HAVE_OPENSSL)
	int re;

	/* Set the virtual server object reference
	 */
	socket->vserver_ref = vserver;

	/* Check whether the virtual server supports SSL
	 */
	if (vserver->context == NULL) {
		return ret_not_found;
	}

	/* New session
	 */
	socket->session = SSL_new (vserver->context);
	if (socket->session == NULL) {
		char *error;

		OPENSSL_LAST_ERROR(error);
		PRINT_ERROR ("ERROR: OpenSSL: Unable to create a new SSL "
			     "connection from the SSL context: %s\n", error);
		return ret_error;
	}
	
	/* Set the socket file descriptor
	 */
	re = SSL_set_fd (socket->session, socket->socket);
	if (re != 1) {
		char *error;

		OPENSSL_LAST_ERROR(error);
		PRINT_ERROR ("ERROR: OpenSSL: Can not set fd(%d): %s\n", 
			     socket->socket, error);
		return ret_error;
	}

#ifndef OPENSSL_NO_TLSEXT 
	SSL_set_app_data (socket->session, socket);
#endif

	/* Set the SSL context cache
	 */
	re = SSL_CTX_set_session_id_context (vserver->context, (const unsigned char *)"SSL", 3);
	if (re != 1) {
		PRINT_ERROR_S("ERROR: OpenSSL: Unable to set SSL session-id context\n");
	}
# endif
	return ret_ok;
}

#endif /* HAVE_TLS */


ret_t 
cherokee_socket_init_tls (cherokee_socket_t *socket, cherokee_virtual_server_t *vserver)
{
#ifdef HAVE_TLS
	int   re;
	ret_t ret;
	
	socket->is_tls = TLS;

	if (socket->initialized == false) {
		ret = initialize_tls_session (socket, vserver);
		if (ret != ret_ok) 
			return ret_error;

		socket->initialized = true;
	}

# if   defined (HAVE_GNUTLS)

	re = gnutls_handshake (socket->session);

	switch (re) {
#if (GNUTLS_E_INTERRUPTED != GNUTLS_E_AGAIN)
	case GNUTLS_E_INTERRUPTED:
#endif
	case GNUTLS_E_AGAIN:
		return ret_eagain;
	}

	if (re < 0) {
		PRINT_ERROR ("ERROR: Init GNUTLS: Handshake has failed: %s\n", gnutls_strerror(re));
		return ret_error;
	}

# elif defined (HAVE_OPENSSL)

	re = SSL_accept (socket->session);

	if (re <= 0) {
		char *error;

		switch (SSL_get_error(socket->session, re)) {
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
# endif
#else
	UNUSED (socket);
	UNUSED (vserver);
#endif	/* HAVE_TLS */
	return ret_ok;
}


ret_t       
cherokee_socket_close (cherokee_socket_t *socket)
{
	ret_t ret;

	/* Sanity check
	 */
	if (socket->socket < 0) {
		return ret_error;
	}

	/* SSL/TLS shutdown
	 */
#ifdef HAVE_TLS
	if (socket->is_tls == TLS && socket->session != NULL) {
# ifdef HAVE_GNUTLS
		gnutls_bye (socket->session, GNUTLS_SHUT_WR);
		gnutls_deinit (socket->session);
		socket->session = NULL;

# elif defined(HAVE_OPENSSL)
		SSL_shutdown (socket->session);

# endif
	}
#endif
	
	/* Close the socket
	 */
#ifdef _WIN32
	re = closesocket (socket->socket);
#else
	ret = cherokee_fd_close (socket->socket);
#endif

	/* Clean up
	 */
	TRACE (ENTRIES",close", "fd=%d is_tls=%d re=%d\n", 
	       socket->socket, socket->is_tls, (int) ret);

	socket->socket = -1;
	socket->status = socket_closed;
	socket->is_tls = non_TLS;

	return ret;
}


ret_t 
cherokee_socket_shutdown (cherokee_socket_t *socket, int how)
{
	/* If the read side of the socket has been closed but the
	 * write side is not, then don't bother to call shutdown
	 * because the socket is going to be closed anyway.
	 */
	if (unlikely (socket->status == socket_closed))
		return ret_eof;
	
	if (unlikely (socket->socket < 0))
		return ret_error;

	if (unlikely (shutdown (socket->socket, how) != 0))
		return ret_error;

	return ret_ok;
}


ret_t
cherokee_socket_ntop (cherokee_socket_t *socket, char *dst, size_t cnt)
{
	const char *str = NULL;

	errno = EAFNOSUPPORT;

	if (SOCKET_FD(socket) < 0) {
		return ret_error;
	}

	/* Only old systems without inet_ntop() function
	 */
#ifndef HAVE_INET_NTOP
	{
		struct sockaddr_in *addr = (struct sockaddr_in *) &SOCKET_ADDR(socket);
		
		str = inet_ntoa (addr->sin_addr);
		memcpy(dst, str, strlen(str));

		return ret_ok;
	}
#endif


#ifdef HAVE_IPV6
	if (SOCKET_AF(socket) == AF_INET6) {
		struct sockaddr_in6 *addr = (struct sockaddr_in6 *) &SOCKET_ADDR(socket);

		str = (char *) inet_ntop (AF_INET6, &addr->sin6_addr, dst, cnt);
		if (str == NULL) {
			return ret_error;
		}
	} else
#endif
	{
		struct sockaddr_in *addr = (struct sockaddr_in *) &SOCKET_ADDR(socket);
		
		str = inet_ntop (AF_INET, &addr->sin_addr, dst, cnt);
		if (str == NULL) {
			return ret_error;
		}
	}

	return ret_ok;
}


ret_t 
cherokee_socket_pton (cherokee_socket_t *socket, cherokee_buffer_t *host)
{
	int re;

	switch (SOCKET_AF(socket)) {
	case AF_INET:
#ifdef HAVE_INET_PTON
		re = inet_pton (AF_INET, host->buf, &SOCKET_SIN_ADDR(socket));
		if (re <= 0) return ret_error;
#else		
		re = inet_aton (host->buf, &SOCKET_SIN_ADDR(socket));
		if (re == 0) return ret_error;
#endif
		break;
#ifdef HAVE_IPV6
	case AF_INET6:
		re = inet_pton (AF_INET6, host->buf, &SOCKET_SIN_ADDR(socket));
		if (re <= 0) return ret_error;
		break;
#endif
	default:
		PRINT_ERROR ("ERROR: Unknown socket family: %d\n", SOCKET_AF(socket));
		return ret_error;
	}

	return ret_ok;
}


ret_t 
cherokee_socket_accept (cherokee_socket_t *socket, int server_socket)
{
	ret_t               ret;
	int                 fd;
	cherokee_sockaddr_t sa;

	ret = cherokee_socket_accept_fd (server_socket, &fd, &sa);
	if (unlikely(ret < ret_ok))
		return ret;

	ret = cherokee_socket_set_sockaddr (socket, fd, &sa);
	if (unlikely(ret < ret_ok)) {
		cherokee_fd_close (fd);
		SOCKET_FD(socket) = -1;
		return ret;
	}

	return ret_ok;
}


ret_t 
cherokee_socket_set_sockaddr (cherokee_socket_t *socket, int fd, cherokee_sockaddr_t *sa)
{
	/* Copy the client address
	 */
	switch (sa->sa.sa_family) {
	case AF_INET: 
		socket->client_addr_len = sizeof(struct sockaddr_in);
		break;
#ifdef HAVE_IPV6
	case AF_INET6:
		socket->client_addr_len = sizeof(struct sockaddr_in6);
		break;
#endif
	default:
		SHOULDNT_HAPPEN;
		return ret_error;
	}

	memcpy (&socket->client_addr, sa, socket->client_addr_len);

	/* Status is no more closed.
	 */
	socket->status = socket_reading;
	
	SOCKET_FD(socket) = fd;
	return ret_ok;
}


ret_t
cherokee_socket_accept_fd (int server_socket, int *new_fd, cherokee_sockaddr_t *sa)
{
	ret_t     ret;
	socklen_t len;
	int       new_socket;

	/* Get the new connection
	 */
	len = sizeof (cherokee_sockaddr_t);

	new_socket = accept (server_socket, &sa->sa, &len);
	if (new_socket < 0) {
		int err = SOCK_ERRNO();
		/* Caller has to retry the call on ret_deny.
		 */
		switch (err) {
#if defined(EWOULDBLOCK) && (EWOULDBLOCK != EAGAIN)
		case EWOULDBLOCK:
#endif	
		case EAGAIN:      
		case EINTR:       
			/* no data or error.
			 */
			return ret_eagain;
#ifdef ECONNABORTED
		case ECONNABORTED:
			/* aborted connection, retry immediately
			 */
			return ret_deny;
#endif
		default:
			/* error
			 */
			return ret_error;
		}
		/* NOTREACHED */
	}

	/* Close-on-exec: Child processes won't inherit this fd
	 */
	cherokee_fd_set_closexec (new_socket);

	/* Disable Nagle's algorithm for this connection
	 * so that there is no delay involved when sending data
	 * which don't fill up a full IP datagram.
	 */
	ret = cherokee_fd_set_nodelay (new_socket, true);
	if (ret != ret_ok) {
		PRINT_ERROR ("Could not disable Nagle's algorithm.\n");
		return ret_error;
	}

	/* Enables nonblocking I/O.
	 */
	ret = cherokee_fd_set_nonblocking (new_socket, true);
	if (ret != ret_ok) {
		PRINT_ERROR ("Could not set non-blocking.\n");
		return ret_error;
	}

	*new_fd = new_socket;
	return ret_ok;
}


ret_t 
cherokee_socket_set_client (cherokee_socket_t *sock, unsigned short int type)
{
	/* Create the socket
	 */
	sock->socket = socket (type, SOCK_STREAM, 0);
	if (sock->socket < 0) {
		return ret_error;
	}

	/* Set the family length
	 */
	switch (type) {
	case AF_INET:
		sock->client_addr_len = sizeof (struct sockaddr_in);
		memset (&sock->client_addr, 0, sock->client_addr_len);
		break;
#ifdef HAVE_IPV6
	case AF_INET6:
		sock->client_addr_len = sizeof (struct sockaddr_in6);
		memset (&sock->client_addr, 0, sock->client_addr_len);
		break;
#endif
#ifdef HAVE_SOCKADDR_UN
	case AF_UNIX:
		memset (&sock->client_addr, 0, sizeof (struct sockaddr_un));
		break;
#endif		
	default:
		SHOULDNT_HAPPEN;
		return ret_error;
	}

	/* Set the family 
	 */
	SOCKET_AF(sock) = type;	
	return ret_ok;
}


static ret_t
cherokee_bind_v4 (cherokee_socket_t *sock, int port, cherokee_buffer_t *listen_to)
{
	int   re;
	ret_t ret;

	SOCKET_ADDR_IPv4(sock)->sin_port = htons (port);

	if (cherokee_buffer_is_empty (listen_to)) {
		SOCKET_ADDR_IPv4(sock)->sin_addr.s_addr = INADDR_ANY;
	} else{
		ret = cherokee_socket_pton (sock, listen_to);
		if (ret != ret_ok) return ret;
	}

	re = bind (SOCKET_FD(sock), (struct sockaddr *)&SOCKET_ADDR(sock), sock->client_addr_len);
	if (re != 0) return ret_error;

	return ret_ok;
}


static ret_t
cherokee_bind_v6 (cherokee_socket_t *sock, int port, cherokee_buffer_t *listen_to)
{
	int   re;
	ret_t ret;

	SOCKET_ADDR_IPv6(sock)->sin6_port = htons(port);

	if (cherokee_buffer_is_empty (listen_to)) {
		SOCKET_ADDR_IPv6(sock)->sin6_addr = in6addr_any; 
	} else{
		ret = cherokee_socket_pton (sock, listen_to);
		if (ret != ret_ok) return ret;
	}

	re = bind (SOCKET_FD(sock), (struct sockaddr *)&SOCKET_ADDR(sock), sock->client_addr_len);
	if (re != 0) return ret_error;

	return ret_ok;
}


static ret_t
cherokee_bind_local (cherokee_socket_t *sock, cherokee_buffer_t *listen_to)
{
#ifdef HAVE_SOCKADDR_UN
	int         re;
	struct stat buf;

	/* Sanity check
	 */
	if ((listen_to->len <= 0) ||
	    (listen_to->len >= sizeof(SOCKET_SUN_PATH(sock))))
		return ret_error;
	
	/* Remove the socket if it already exists
	 */
	re = stat (listen_to->buf, &buf);
	if (re == 0) {
		if (! S_ISSOCK(buf.st_mode)) {
			PRINT_ERROR ("ERROR: %s isn't a socket!\n", listen_to->buf);
			return ret_error;			
		}

		re = unlink (listen_to->buf);
		if (re != 0) {
			PRINT_ERROR ("ERROR: Couldn't remove %s\n", listen_to->buf);
			return ret_error;
		}
	}

	/* Create the socket
	 */
	memcpy (SOCKET_SUN_PATH(sock), listen_to->buf, listen_to->len + 1);
	sock->client_addr_len = sizeof(SOCKET_ADDR_UNIX(sock)->sun_family) + listen_to->len;

	re = bind (SOCKET_FD(sock), 
		   (const struct sockaddr *)SOCKET_ADDR_UNIX(sock), 
		   sock->client_addr_len);
	if (re != 0) return ret_error;

	return ret_ok;
#else
	return ret_no_sys;
#endif
}


ret_t 
cherokee_socket_bind (cherokee_socket_t *sock, int port, cherokee_buffer_t *listen_to)
{
	/* Bind
	 */
	switch (SOCKET_AF(sock)) {
	case AF_INET:
		return cherokee_bind_v4 (sock, port, listen_to);
#ifdef HAVE_IPV6
	case AF_INET6:
		return cherokee_bind_v6 (sock, port, listen_to);
#endif
#ifdef HAVE_SOCKADDR_UN
	case AF_UNIX:
		return cherokee_bind_local (sock, listen_to);
#endif
	default:
		break;
	}
	
	SHOULDNT_HAPPEN;
	return ret_error;
}


ret_t 
cherokee_socket_listen (cherokee_socket_t *socket, int backlog)
{
	int re;

	re = listen (SOCKET_FD(socket), backlog);
	if (re < 0) return ret_error;

	return ret_ok;
}


/* WARNING: all parameters MUST be valid,
 *          NULL pointers lead to a crash.
 */
ret_t 
cherokee_socket_write (cherokee_socket_t *socket, const char *buf, int buf_len, size_t *pcnt_written)
{
	ssize_t len;

	*pcnt_written = 0;

	/* There must be something to send, otherwise behaviour is undefined
	 * and as we don't want this case, we have to enforce assertions.
	 */
	return_if_fail (buf != NULL && buf_len > 0, ret_error);

#ifdef HAVE_TLS
	if (likely (socket->is_tls != TLS)) {
#endif
		len = send (SOCKET_FD(socket), buf, buf_len, 0);
		if (likely (len > 0) ) {
			/* Return n. of bytes sent.
			 */
			*pcnt_written = len;
			return ret_ok;
		}
		if (len == 0) {
			/* Very strange, socket is ready but nothing has been written,
			 * retry later.
			 */
			return ret_eagain;
		}
		/* else len < 0 */
		{
			int err = SOCK_ERRNO();

			switch (err) {
#if defined(EWOULDBLOCK) && (EWOULDBLOCK != EAGAIN)
			case EWOULDBLOCK:
#endif	
			case EAGAIN:      
			case EINTR:       
				return ret_eagain;

			case EPIPE:       
#ifdef ENOTCONN
			case ENOTCONN:
#endif
			case ECONNRESET:  
				socket->status = socket_closed;
			case ETIMEDOUT:
			case EHOSTUNREACH:
				return ret_error;
			}

			PRINT_ERRNO (err, "write(%d, ..): '${errno}'", SOCKET_FD(socket));
			return ret_error;
		}

#ifdef HAVE_TLS
	}

	/* TLS connection.
	 */
#if  defined (HAVE_GNUTLS)
	len = gnutls_record_send (socket->session, buf, buf_len);
	if (likely (len > 0) ) {
		/* Return info
		 */
		*pcnt_written = len;
		return ret_ok;
	}

	if (len == 0) {
		/* Very strange, socket is ready but nothing has been written,
		 * retry later.
		 */
		return ret_eagain;
	}

	{	/* len < 0 */
		switch (len) {
			case GNUTLS_E_PUSH_ERROR:
			case GNUTLS_E_INVALID_SESSION: 
				socket->status = socket_closed;
				return ret_eof;
#if (GNUTLS_E_INTERRUPTED != GNUTLS_E_AGAIN)
			case GNUTLS_E_INTERRUPTED:
#endif
			case GNUTLS_E_AGAIN:           
				return ret_eagain;
		}

		PRINT_ERROR ("ERROR: GNUTLS: gnutls_record_send(%d, ..) -> err=%d '%s'\n", 
			SOCKET_FD(socket), (int)len, gnutls_strerror(len));
		return ret_error;
	}

#elif defined (HAVE_OPENSSL)

	len = SSL_write (socket->session, buf, buf_len);

	if (likely (len > 0) ) {
		/* Return info
		 */
		*pcnt_written = len;
		return ret_ok;
	}

	if (len == 0) {
		/* maybe socket was closed by client, no write was performed
		 */
		int re = SSL_get_error (socket->session, len);
		if (re == SSL_ERROR_ZERO_RETURN)
			socket->status = socket_closed;
		return ret_eof;
	}

	{	/* len < 0 */
		int re = SSL_get_error (socket->session, len);
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
#else
	return ret_error;
#endif

#endif	/* HAVE_TLS */

}


/* WARNING: all parameters MUST be valid,
 *          NULL pointers lead to a crash.
 */
ret_t
cherokee_socket_read (cherokee_socket_t *socket, char *buf, int buf_size, size_t *pcnt_read)
{
	ssize_t len;

	*pcnt_read = 0;

	/* There must be something to read, otherwise behaviour is undefined
	 * and as we don't want this case, we have to enforce assertions.
	 */
	return_if_fail (buf != NULL && buf_size > 0, ret_error);

	if (unlikely (socket->status == socket_closed))
		return ret_eof;

#ifdef HAVE_TLS
	if (likely (socket->is_tls != TLS)) {
#endif
		/* Plain read
		 */
		len = recv (SOCKET_FD(socket), buf, buf_size, 0);

		if (likely (len > 0)) {
			*pcnt_read = len;
			return ret_ok;
		}

		if (len == 0) {
			socket->status = socket_closed;
			return ret_eof;
		}

		{	/* len < 0 */
			int err = SOCK_ERRNO();

			TRACE(ENTRIES",read", "Socket read error fd=%d: '%s'\n",
			      SOCKET_FD(socket), strerror(errno));

			switch (err) {
#if defined(EWOULDBLOCK) && (EWOULDBLOCK != EAGAIN)
			case EWOULDBLOCK:
#endif
			case EINTR:      
			case EAGAIN:    
				return ret_eagain;

			case EPIPE: 
#ifdef ENOTCONN
			case ENOTCONN:
#endif
			case ECONNRESET:
				socket->status = socket_closed;
			case ETIMEDOUT:
			case EHOSTUNREACH:
				return ret_error;
			}

			PRINT_ERRNO (err, "read(%d, ..): '${errno}'", SOCKET_FD(socket));
			return ret_error;
		}

#ifdef HAVE_TLS
	}

	/* socket->is_tls == TLS
	 */
#if   defined (HAVE_GNUTLS)

	len = gnutls_record_recv (socket->session, buf, buf_size);

	if (likely (len > 0)) {
 		*pcnt_read = len;
		if (gnutls_record_check_pending (socket->session) > 0)
			return ret_eagain;
		return ret_ok;
	}

	if (len == 0) {
		socket->status = socket_closed;
		return ret_eof;
	}

	{	/* len < 0 */
		switch (len) {
		case GNUTLS_E_PUSH_ERROR:
		case GNUTLS_E_INVALID_SESSION:
		case GNUTLS_E_UNEXPECTED_PACKET_LENGTH:
			socket->status = socket_closed;
			return ret_eof;
#if (GNUTLS_E_INTERRUPTED != GNUTLS_E_AGAIN)
		case GNUTLS_E_INTERRUPTED:              
#endif
		case GNUTLS_E_AGAIN:
			return ret_eagain;
		}

		PRINT_ERROR ("ERROR: GNUTLS: gnutls_record_recv(%d, ..) -> err=%d '%s'\n", 
			SOCKET_FD(socket), (int)len, gnutls_strerror(len));
		return ret_error;
	}

#elif defined (HAVE_OPENSSL)
	len = SSL_read (socket->session, buf, buf_size);
	if (likely (len > 0)) {
		*pcnt_read = len;
		if (SSL_pending (socket->session))
			return ret_eagain;
		return ret_ok;
	}

	if (len == 0) {
		socket->status = socket_closed;
		return ret_eof;
	}

	{	/* len < 0 */
		int re;
		int error = errno;

		re = SSL_get_error (socket->session, len);
		switch (re) {
		case SSL_ERROR_WANT_READ:   
		case SSL_ERROR_WANT_WRITE:   
			return ret_eagain;
		case SSL_ERROR_ZERO_RETURN: 
			socket->status = socket_closed;
			return ret_eof;
		case SSL_ERROR_SSL:         
			return ret_error;
		case SSL_ERROR_SYSCALL:
			switch (error) {
			case EPIPE:
			case ECONNRESET:
				socket->status = socket_closed;
				return ret_eof;
			default:
				PRINT_ERRNO (error, "SSL_read: unknown errno: ${errno}\n");
			}
			return ret_error;
		}

		PRINT_ERROR ("ERROR: OpenSSL: SSL_read (%d, ..) -> err=%d '%s'\n", 
			     SOCKET_FD(socket), (int)len, ERR_error_string(re, NULL));
		return ret_error;
	}
#else
	return ret_error;
#endif

#endif	/* HAVE_TLS */

}


int
cherokee_socket_pending_read (cherokee_socket_t *socket)
{
	if (socket->is_tls != TLS)
		return 0;

	if (unlikely ((socket->status != socket_reading) &&
		      (socket->status != socket_writing)))
		return 0;

#ifdef HAVE_TLS
# ifdef HAVE_GNUTLS
	return (gnutls_record_check_pending (socket->session) > 0);
# elif defined (HAVE_OPENSSL)
	return (SSL_pending (socket->session) > 0);
# endif
#endif
	return 0;
}


ret_t
cherokee_socket_flush (cherokee_socket_t *socket)
{
	int re;
	int op = 1;

	re = setsockopt (SOCKET_FD(socket), IPPROTO_TCP, TCP_NODELAY,
			 (const void *) &op, sizeof(int));
	if (unlikely(re != 0))
		return ret_error;

	return ret_ok;	
}


/* WARNING: all parameters MUST be valid,
 *          NULL pointers lead to a crash.
 */
ret_t 
cherokee_socket_writev (cherokee_socket_t  *socket, 
			const struct iovec *vector,
			uint16_t            vector_len,
			size_t             *pcnt_written)
{
	*pcnt_written = 0;

	/* There must be something to send, otherwise behaviour is undefined
	 * and as we don't want this case, we have to enforce assertions.
	 */
	return_if_fail (vector != NULL && vector_len > 0, ret_error);

#ifdef HAVE_TLS
	if (likely (socket->is_tls != TLS))
#endif
	{
		int re = 0;
#ifdef _WIN32
		int i;
		size_t total;

		for (i = 0, re = 0, total = 0; i < vector_len; i++) {
			if (vector[i].iov_len == 0)
				continue;
			re = send (SOCKET_FD(socket), vector[i].iov_base, vector[i].iov_len, 0);
			if (re < 0)
				break;

			total += re;

			/* if it is a partial send, then stop sending data
			 */
			if (re != vector[i].iov_len)
				break;
		}
		*pcnt_written = total;

		/* if we have sent at least one byte,
		 * then return OK.
		 */
		if (likely (total > 0))
			return ret_ok;

		if (re == 0) {
			int err = SOCK_ERRNO();
			if (i == vector_len)
				return ret_ok;
			/* Retry later.
			 */
			return ret_eagain;
		}

#else	/* ! WIN32 */

		re = writev (SOCKET_FD(socket), vector, vector_len);

		if (likely (re > 0)) {
			*pcnt_written = (size_t) re;
			return ret_ok;
		}
		if (re == 0) {
			int i;
			/* Find out whether there was something to send or not.
			 */
			for (i = 0; i < vector_len; i++) {
				if (vector[i].iov_base != NULL && vector[i].iov_len > 0)
					break;
			}
			if (i < vector_len)
				return ret_eagain;
			/* No, nothing to send, so return ok.
			 */
			return ret_ok;
		}
#endif
		if (re < 0) {
			int err = SOCK_ERRNO();

			switch (err) {
#if defined(EWOULDBLOCK) && (EWOULDBLOCK != EAGAIN)
			case EWOULDBLOCK:
#endif
			case EAGAIN:
			case EINTR: 
				return ret_eagain;

			case EPIPE:
#ifdef ENOTCONN
			case ENOTCONN:
#endif
			case ECONNRESET:
				socket->status = socket_closed;
			case ETIMEDOUT:
			case EHOSTUNREACH:
				return ret_error;
			}

			PRINT_ERRNO (err, "writev(%d, ..): '${errno}'", SOCKET_FD(socket));
			return ret_error;
		}
	}

#ifdef HAVE_TLS
#if defined (HAVE_GNUTLS) || defined (HAVE_OPENSSL)

	/* TLS connection: Here we don't worry about sparing a few CPU
	 * cycles, so we reuse the single send case for TLS.
	 */
	{
		int      i;
		ret_t  ret;
		size_t cnt;
		
		for (i = 0; i < vector_len; i++) {

			if (vector[i].iov_base == NULL || vector[i].iov_len == 0)
				continue;
			
			cnt = 0;
			ret = cherokee_socket_write (socket, vector[i].iov_base, vector[i].iov_len, &cnt);
			*pcnt_written += cnt;

			if (ret == ret_ok && cnt == vector[i].iov_len)
				continue;

			/* else != ret_ok || cnt != vector[i].iov_len
			 */
			if (*pcnt_written != 0)
				return ret_ok;

			/* Nothing has been written, return error code.
			 */
			return ret;
		}

		return ret_ok;
	}
#endif
#endif	/* HAVE_TLS */

	return ret_error;
}


/* WARNING: all parameters MUST be valid,
 *          NULL pointers lead to a crash.
 */
ret_t       
cherokee_socket_bufwrite (cherokee_socket_t *socket, cherokee_buffer_t *buf, size_t *pcnt_written)
{
	ret_t  ret;

	ret = cherokee_socket_write (socket, buf->buf, buf->len, pcnt_written);

	TRACE (ENTRIES",bufwrite", "write fd=%d len=%d ret=%d written=%u\n", socket->socket, buf->len, ret, *pcnt_written);

	return ret;
}


/* WARNING: all parameters MUST be valid,
 *          NULL pointers lead to a crash.
 */
ret_t      
cherokee_socket_bufread (cherokee_socket_t *socket, 
			 cherokee_buffer_t *buf, 
			 size_t             count, 
			 size_t            *pcnt_read)
{
	ret_t    ret;
	char    *starting;
	
	/* Read
	 */
	ret = cherokee_buffer_ensure_size (buf, buf->len + count + 2);
	if (unlikely(ret < ret_ok))
		return ret;

	starting = buf->buf + buf->len;
	
	ret = cherokee_socket_read (socket, starting, count, pcnt_read);
	if (*pcnt_read > 0) {
		buf->len += *pcnt_read;
		buf->buf[buf->len] = '\0';
	}

	TRACE (ENTRIES",bufread", "read fd=%d count=%d ret=%d read=%d\n",
	       socket->socket, count, ret, *pcnt_read);

	return ret;
}


ret_t 
cherokee_socket_sendfile (cherokee_socket_t *socket,
			  int      fd, 
			  size_t   size, 
			  off_t   *offset, 
			  ssize_t *sent)
{
	static cherokee_boolean_t no_sys = false;

	/* Exit if there is no sendfile() function in the system
	 */
	if (unlikely (no_sys)) 
		return ret_no_sys;

 	/* If there is nothing to send then return now, this may be
 	 * needed in some systems (i.e. *BSD) because value 0 may have
 	 * special meanings or trigger occasional hidden bugs.
 	 */
	if (size == 0)
		return ret_ok;

	/* Limit size of data that has to be sent.
	 */
	if (size > MAX_SF_BLK_SIZE2)
		size = MAX_SF_BLK_SIZE;

#if defined(LINUX_BROKEN_SENDFILE_API)
	UNUSED(socket);
	UNUSED(fd);
	UNUSED(offset);
	UNUSED(sent);

	/* Large file support is set but native Linux 2.2 or 2.4 sendfile()
	 * does not support _FILE_OFFSET_BITS 64
	 */
	return ret_no_sys;

#elif defined(LINUX_SENDFILE_API)

	/* Linux sendfile
	 *
	 * ssize_t 
	 * sendfile (int out_fd, int in_fd, off_t *offset, size_t *count);
	 *
	 * ssize_t 
	 * sendfile64 (int out_fd, int in_fd, off64_t *offset, size_t *count);
	 */
	do {
		*sent = sendfile (SOCKET_FD(socket),     /* int     out_fd */
		                  fd,                    /* int     in_fd  */
		                  offset,                /* off_t  *offset */
		                  size);                 /* size_t  count  */
	} while ((*sent == -1) && (errno == EINTR));
		
	if (*sent < 0) {
		switch (errno) {
		case EAGAIN:
#if defined(EWOULDBLOCK) && (EWOULDBLOCK != EAGAIN)
		case EWOULDBLOCK:
#endif
			return ret_eagain;
		case EINVAL:
			/* maybe sendfile is not supported by this FS (no mmap available),
			 * since more than one FS can be used (ext2, ext3, ntfs, etc.)
			 * we should retry with emulated sendfile (read+write).
			 */
			return ret_no_sys;
		case ENOSYS: 
			/* This kernel does not support sendfile at all.
			 */
			no_sys = true;
			return ret_no_sys;
		}

		return ret_error;
	}

#elif DARWIN_SENDFILE_API
	int   re;
	off_t _sent = size;

	/* MacOS X: BSD-like System Call
	 *
	 * int
	 * sendfile (int fd, int s, off_t offset, off_t *len,
	 *           struct sf_hdtr *hdtr, int flags);
	 */	
	do {
		re = sendfile (fd,                        /* int             fd     */
			       SOCKET_FD(socket),         /* int             s      */
			       *offset,                   /* off_t           offset */
			       &_sent,                    /* off_t          *len    */
			       NULL,                      /* struct sf_hdtr *hdtr   */
			       0);                        /* int             flags  */
	}  while (re == -1 && errno == EINTR);

	if (re == -1) {
		switch (errno) {
		case EAGAIN:
			/* It might have sent some information
			 */
			if (_sent <= 0)
				return ret_eagain;
			break;
		case ENOSYS:
			no_sys = true;
			return ret_no_sys;
		default:
			return ret_error;
		}
	}

	*sent = _sent;
	*offset = *offset + _sent;

#elif SOLARIS_SENDFILE_API

	do {
		*sent = sendfile (SOCKET_FD(socket),     /* int   out_fd */
				  fd,                    /* int    in_fd */
				  offset,                /* off_t   *off */
				  size);                 /* size_t   len */
	} while ((*sent == -1) && (errno == EINTR));

	if (*sent < 0) {
		switch (errno) {
		case EAGAIN:
#if defined(EWOULDBLOCK) && (EWOULDBLOCK != EAGAIN)
		case EWOULDBLOCK:
#endif
			return ret_eagain;
		case ENOSYS: 
			/* This kernel does not support sendfile at all.
			 */
			no_sys = true;
			return ret_no_sys;
		case EAFNOSUPPORT:
			return ret_no_sys;
		}
		return ret_error;
	}

#elif FREEBSD_SENDFILE_API
	int            re;
	struct sf_hdtr hdr;
	struct iovec   hdtrl;

	hdr.headers    = &hdtrl;
	hdr.hdr_cnt    = 1;
	hdr.trailers   = NULL;
	hdr.trl_cnt    = 0;
	
	hdtrl.iov_base = NULL;
	hdtrl.iov_len  = 0;

	*sent = 0;

	/* FreeBSD sendfile: in_fd and out_fd are reversed
	 *
	 * int
	 * sendfile (int fd, int s, off_t offset, size_t nbytes,
	 *           struct sf_hdtr *hdtr, off_t *sbytes, int flags);
	 */	
	do {
		re = sendfile (fd,                        /* int             fd     */
			       SOCKET_FD(socket),         /* int             s      */
			       *offset,                   /* off_t           offset */
			       size,                      /* size_t          nbytes */
			       &hdr,                      /* struct sf_hdtr *hdtr   */
			       sent,                      /* off_t          *sbytes */ 
			       0);                        /* int             flags  */

	}  while (re == -1 && errno == EINTR);

	if (re == -1) {
		switch (errno) {
		case EAGAIN:
#if defined(EWOULDBLOCK) && (EWOULDBLOCK != EAGAIN)
		case EWOULDBLOCK:
#endif
			if (*sent < 1)
				return ret_eagain;

			/* else it's ok, something has been sent.
			 */
			break;
		case ENOSYS:
			no_sys = true;
			return ret_no_sys;
		default:
			return ret_error;
		}
	}
	*offset = *offset + *sent;

#elif HPUX_SENDFILE_API
	
	/* HP-UX:
	 *
	 * sbsize_t sendfile(int s, int fd, off_t offset, bsize_t nbytes, 
	 *                   const struct iovec *hdtrl, int flags); 
	 *
	 * HPUX guarantees that if any data was written before
	 * a signal interrupt then sendfile returns the number of
	 * bytes written (which may be less than requested) not -1.
	 * nwritten includes the header data sent.
	 */
	do {
 		*sent = sendfile (SOCKET_FD(socket),     /* socket          */
 		                  fd,                    /* fd to send      */
 		                  *offset,               /* where to start  */
 		                  size,                  /* bytes to send   */
 		                  NULL,                  /* Headers/footers */
 		                  0);                    /* flags           */
	} while ((*sent == -1) && (errno == EINTR));

	if (*sent < 0) {
		switch (errno) {
		case EAGAIN:
#if defined(EWOULDBLOCK) && (EWOULDBLOCK != EAGAIN)
		case EWOULDBLOCK:
#endif
			return ret_eagain;
		case ENOSYS:
			no_sys = true;
			return ret_no_sys;
		}
		return ret_error;
	}
	*offset = *offset + *sent;

#else
	UNUSED(socket);
	UNUSED(fd);
	UNUSED(offset);
	UNUSED(sent);

	SHOULDNT_HAPPEN;
	return ret_error;
#endif

	return ret_ok;
}


ret_t
cherokee_socket_gethostbyname (cherokee_socket_t *socket, cherokee_buffer_t *hostname)
{
	if (SOCKET_AF(socket) == AF_UNIX) {
#ifdef _WIN32
		SHOULDNT_HAPPEN;
		return ret_no_sys;
#else
		memset ((char*) SOCKET_SUN_PATH(socket), 0, 
			sizeof (SOCKET_ADDR_UNIX(socket)));

		SOCKET_ADDR_UNIX(socket)->sun_family = AF_UNIX;

		strncpy (SOCKET_SUN_PATH (socket), hostname->buf, 
			 sizeof (SOCKET_ADDR_UNIX(socket)->sun_path) - sizeof(short));

		return ret_ok;
#endif
	}

	return cherokee_gethostbyname (hostname->buf, &SOCKET_SIN_ADDR(socket));
}


ret_t 
cherokee_socket_connect (cherokee_socket_t *sock)
{
	int r;

	TRACE (ENTRIES, "connect type=%d\n", SOCKET_AF(sock));

	switch (SOCKET_AF(sock)) {
	case AF_INET:
		r = connect (SOCKET_FD(sock),
			     (struct sockaddr *) &SOCKET_ADDR(sock), 
			     sizeof(struct sockaddr_in));
		break;
#ifdef HAVE_IPV6
	case AF_INET6:
		r = connect (SOCKET_FD(sock), 
			     (struct sockaddr *) &SOCKET_ADDR(sock),
			     sizeof(struct sockaddr_in6));
		break;
#endif
#ifdef HAVE_SOCKADDR_UN
	case AF_UNIX:
		r = connect (SOCKET_FD(sock), 
			     (struct sockaddr *) &SOCKET_ADDR(sock), 
			     SUN_LEN (SOCKET_ADDR_UNIX(sock)));
		break;
#endif	
	default:
		SHOULDNT_HAPPEN;
		return ret_no_sys;			
	}

	if (r < 0) {
		int err = SOCK_ERRNO();
		
		TRACE (ENTRIES",connect", "connect error type=%d errno='%s'\n", 
		       SOCKET_AF(sock), strerror(err));
		
		switch (err) {
		case EISCONN:
			break;
		case EINVAL:
		case ENOENT:
		case ECONNREFUSED:
		case EADDRNOTAVAIL:
			return ret_deny;
		case ETIMEDOUT:
			return ret_error;
		case EAGAIN:
		case EALREADY:
		case EINPROGRESS:
#if defined(EWOULDBLOCK) && (EWOULDBLOCK != EAGAIN)
		case EWOULDBLOCK:
#endif
			return ret_eagain;
		default:
			PRINT_ERRNO_S (err, "Cannot connect: '${errno}'");
			return ret_error;
		}
	}

	TRACE (ENTRIES",connect", "successful connect (type=%d)\n", SOCKET_AF(sock));

	sock->status = socket_reading;
	return ret_ok;
}


ret_t 
cherokee_socket_init_client_tls (cherokee_socket_t *socket, cherokee_buffer_t *host)
{
#ifdef HAVE_TLS
	int re;

# if defined (HAVE_GNUTLS)
	const int                       kx_priority[] = {GNUTLS_KX_ANON_DH, 0};
	gnutls_anon_client_credentials  anoncred;

	socket->is_tls = TLS;	

	/* Acredentials
	 */
	gnutls_anon_allocate_client_credentials(&anoncred);

	/* Init session
	 */
	re = gnutls_init (&socket->session, GNUTLS_CLIENT);
	if (unlikely (re != GNUTLS_E_SUCCESS)) return ret_error;

	
	/* Settings
	 */
	gnutls_set_default_priority(socket->session);
	gnutls_kx_set_priority (socket->session, kx_priority);

	gnutls_credentials_set (socket->session, GNUTLS_CRD_ANON, anoncred);
	gnutls_transport_set_ptr (socket->session, (gnutls_transport_ptr)socket->socket);

	do {
		re = gnutls_handshake (socket->session);
		if (re < 0) {
			switch (re) {
#if (GNUTLS_E_INTERRUPTED != GNUTLS_E_AGAIN)
			case GNUTLS_E_INTERRUPTED:
#endif
			case GNUTLS_E_AGAIN:
				break;
			default:
				return ret_error;
			}
		}
	} while ((re == GNUTLS_E_AGAIN) ||
		 (re == GNUTLS_E_INTERRUPTED));

	UNUSED (host);

# elif defined (HAVE_OPENSSL)
	char *error;

	socket->is_tls = TLS;

	/* New context
	 */
	socket->ssl_ctx = SSL_CTX_new (SSLv23_client_method());
	if (socket->ssl_ctx == NULL) {
		OPENSSL_LAST_ERROR(error);
		PRINT_ERROR ("ERROR: OpenSSL: Unable to create a new SSL context: %s\n", error);
		return ret_error;
	}
	
	/* CA verifications
	 */
	re = cherokee_buffer_is_empty (&socket->vserver_ref->ca_cert);
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


	SSL_CTX_set_verify (socket->ssl_ctx, SSL_VERIFY_NONE, NULL);
	SSL_CTX_set_mode (socket->ssl_ctx, SSL_MODE_ENABLE_PARTIAL_WRITE);

	/* New session
	 */
	socket->session = SSL_new (socket->ssl_ctx);
	if (socket->session == NULL) {
		OPENSSL_LAST_ERROR(error);
		PRINT_ERROR ("ERROR: OpenSSL: Unable to create a new SSL connection "
			     "from the SSL context: %s\n", error);
		return ret_error;
	}

	/* Set the socket file descriptor
	 */
	re = SSL_set_fd (socket->session, socket->socket);
	if (re != 1) {
		OPENSSL_LAST_ERROR(error);
		PRINT_ERROR ("ERROR: OpenSSL: Can not set fd(%d): %s\n", socket->socket, error);
		return ret_error;
	}

	SSL_set_connect_state (socket->session); 

#ifndef OPENSSL_NO_TLSEXT 
	re = SSL_set_tlsext_host_name (socket->session, host->buf);
	if (re <= 0) {
		OPENSSL_LAST_ERROR(error);
		PRINT_ERROR ("ERROR: OpenSSL: Could set SNI server name: %s\n", error);
		return ret_error;
	}
#endif

	re = SSL_connect (socket->session);
	if (re <= 0) {
		OPENSSL_LAST_ERROR(error);
		PRINT_ERROR ("ERROR: OpenSSL: Can not connect: %s\n", error);
		return ret_error;
	}

# endif
#else 
	UNUSED (host);
	UNUSED (socket);
#endif
	return ret_ok;
}


ret_t
cherokee_socket_has_block_timeout (cherokee_socket_t *socket)
{
	UNUSED (socket);

#if defined(SO_RCVTIMEO) && !defined(HAVE_BROKEN_SO_RCVTIMEO)
	return ret_ok;
#else
	return ret_not_found;
#endif
}

 
ret_t 
cherokee_socket_set_block_timeout (cherokee_socket_t *socket, cuint_t timeout)
{
	int            re;
	cuint_t        block;
	struct timeval tv;

	if (socket->socket < 0) {
		return ret_error;
	}

#if defined(SO_RCVTIMEO) && !defined(HAVE_BROKEN_SO_RCVTIMEO)
	/* Set the socket to blocking
	 */
	block = 0;
		
#ifdef _WIN32
	re = ioctlsocket (socket->socket, FIONBIO, &block);	
	if (re == SOCKET_ERROR) {
#else	
	re = ioctl (socket->socket, FIONBIO, &block);	
	if (re < 0) {
#endif
		PRINT_ERROR ("ioctl (%d, FIONBIO, &%d) = %d\n", socket->socket, block, re);
		return ret_error;
	}

	/* Set the send / receive timeouts
	 */
	tv.tv_sec  = timeout / 1000;
	tv.tv_usec = timeout % 1000;
	
	re = setsockopt (socket->socket, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
	if (re < 0) {
		int err = errno;

		PRINT_ERRNO (err, "Couldn't set SO_RCVTIMEO, fd=%d, timeout=%d: '${errno}'", 
			     SOCKET_FD(socket), timeout);
		return ret_error;
	}
#else
	return ret_no_sys;
#endif
	
	return ret_ok;		      
}


ret_t       
cherokee_socket_set_status (cherokee_socket_t *socket, cherokee_socket_status_t status)
{
	socket->status = status;
	return ret_ok;
}


ret_t
cherokee_socket_set_cork (cherokee_socket_t *socket, cherokee_boolean_t enable)
{
#if defined(HAVE_TCP_CORK) || defined(HAVE_TCP_NOPUSH)
	int re;
	int tmp = 0;
	int fd  = socket->socket;

	TRACE(ENTRIES",cork", "%s TCP_CORK on fd %d\n", 
	      enable ? "Setting" : "Removing", fd);

	if (enable) {
		tmp = 0;
		re = setsockopt (fd, IPPROTO_TCP, TCP_NODELAY, &tmp, sizeof(tmp));
		if (unlikely (re < 0)) { 
			PRINT_ERRNO (errno, "ERROR: Removing TCP_NODELAY to fd %d: ${errno}", fd);
			return ret_error;
		}

		tmp = 1;
		re = setsockopt (fd, IPPROTO_TCP, TCP_CORK, &tmp, sizeof(tmp));
		if (unlikely (re < 0)) {
			PRINT_ERRNO (errno, "ERROR: Setting TCP_CORK to fd %d: ${errno}", fd);
			return ret_error;
		}

		return ret_ok;
	}

	tmp = 0;
	re = setsockopt (fd, IPPROTO_TCP, TCP_CORK, &tmp, sizeof(tmp));
	if (unlikely (re < 0)) {
		PRINT_ERRNO (errno, "ERROR: Removing TCP_CORK to fd %d: ${errno}", fd);
		return ret_error;
	}

	tmp = 1;
	re = setsockopt (fd, IPPROTO_TCP, TCP_NODELAY, &tmp, sizeof(tmp));
	if (unlikely (re < 0)) { 
		PRINT_ERRNO (errno, "ERROR: Setting TCP_NODELAY to fd %d: ${errno}", fd);
		return ret_error;
	}

	return ret_ok;
	
#else
	UNUSED(socket);
	UNUSED(enable);

	return ret_ok;
#endif
}
