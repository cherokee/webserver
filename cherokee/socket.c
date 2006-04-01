/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/* Cherokee
 *
 * Authors:
 *      Alvaro Lopez Ortega <alvaro@alobbs.com>
 *
 * Some patches by:
 *      Ricardo Cardenes Medina <ricardo@conysis.com>
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
# include <sys/uio.h>

#elif defined(FREEBSD_SENDFILE_API)
# include <sys/types.h>
# include <sys/socket.h>
# include <sys/uio.h>

#elif defined(LINUX_BROKEN_SENDFILE_API)
extern int32_t sendfile (int out_fd, int in_fd, int32_t *offset, uint32_t count);
#endif

#define ENTRIES "socket"


#include <errno.h>
#include <unistd.h>
#include <fcntl.h>

#include "util.h"
#include "buffer.h"
#include "virtual_server.h"


ret_t
cherokee_socket_init (cherokee_socket_t *socket)
{
	memset (&socket->client_addr, 0, sizeof(cherokee_sockaddr_t));
	socket->client_addr_len = -1;

	socket->socket      = -1;
	socket->status      = socket_closed;
	socket->is_tls      = non_TLS;

#ifdef HAVE_TLS
	socket->initialized = false;
	socket->vserver_ref = NULL;
#endif

#ifdef HAVE_GNUTLS
	socket->session     = NULL;
#endif

#ifdef HAVE_OPENSSL
	socket->session     = NULL;
	socket->ssl_ctx     = NULL;
#endif

	return ret_ok;
}


ret_t
cherokee_socket_mrproper (cherokee_socket_t *socket)
{
#ifdef HAVE_GNUTLS
	if (socket->session != NULL) {
		gnutls_deinit (socket->session);
		socket->session = NULL;
	}
#endif

#ifdef HAVE_OPENSSL
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

ret_t
cherokee_socket_new (cherokee_socket_t **socket)
{
 	CHEROKEE_NEW_STRUCT (n, socket);

	/* Init 
	 */
	cherokee_socket_init (n);
	
	/* Return it
	 */
	*socket = n;
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
	memset (&socket->client_addr, 0, sizeof(struct sockaddr));

#ifdef HAVE_TLS
	socket->initialized = false;
	socket->vserver_ref = NULL;
#endif

#ifdef HAVE_GNUTLS
	if (socket->session != NULL) {
		gnutls_deinit (socket->session);
		socket->session = NULL;
	}
#endif

#ifdef HAVE_OPENSSL
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

static gnutls_datum
db_retrieve (void *ptr, gnutls_datum key)
{
	ret_t                     ret;
	gnutls_datum              new    = { NULL, 0 };
	cherokee_socket_t        *socket = SOCKET(ptr);
	cherokee_session_cache_t *cache;

	// printf ("db::retrieve\n");

	if (socket->vserver_ref == NULL) {
		PRINT_ERROR ("Assert failed %s, %d\n", __FILE__, __LINE__);
		return new;
	}

	cache = socket->vserver_ref->session_cache;

	/* Retrieve (get and remove) the object from the session cache
	 */
	ret = cherokee_session_cache_retrieve (cache, key.data, key.size, 
					       (void **)&new.data, &new.size);
	if (unlikely(ret != ret_ok)) return new;

	return new;
}

static int
db_remove (void *ptr, gnutls_datum key)
{
	ret_t                     ret;
	cherokee_socket_t        *socket = SOCKET(ptr);
	cherokee_session_cache_t *cache;

	// printf ("db::remove\n");

	if (socket->vserver_ref == NULL) {
		PRINT_ERROR ("ERROR: Assert failed %s, %d\n", __FILE__, __LINE__);
		return 1;
	}

	cache = socket->vserver_ref->session_cache;
	ret = cherokee_session_cache_del (cache, key.data, key.size);
	
	return (ret == ret_ok) ? 0 : 1;
}

static int
db_store (void *ptr, gnutls_datum key, gnutls_datum data)
{
	ret_t                     ret;
	cherokee_socket_t        *socket = SOCKET(ptr);
	cherokee_session_cache_t *cache;

	// printf ("db::store\n");

	if (socket->vserver_ref == NULL) {
		PRINT_ERROR ("ERROR: Assert failed %s, %d\n", __FILE__, __LINE__);
		return 1;
	}

	cache = socket->vserver_ref->session_cache;
	ret = cherokee_session_cache_add (cache, key.data, key.size, data.data, data.size);

	return (ret == ret_ok) ? 0 : 1;
}

#endif /* HAVE_GNUTLS */



static ret_t
initialize_tls_session (cherokee_socket_t *socket, cherokee_virtual_server_t *vserver)
{
#ifdef HAVE_TLS
	int re;

	/* Set the virtual server object reference
	 */
	socket->vserver_ref = vserver;

# if defined(HAVE_GNUTLS)
	/* Init the TLS session
	 */
        re = gnutls_init (&socket->session, GNUTLS_SERVER);
	if (unlikely (re != GNUTLS_E_SUCCESS)) return ret_error;

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
	
	/* Request client certificate if any.
	 */
	gnutls_certificate_server_set_request (socket->session, GNUTLS_CERT_REQUEST);

	/* Set the number of bits, for use in an Diffie Hellman key
	 * exchange: minimum size of the prime that will be used for
	 * the handshake.
	 */
	gnutls_dh_set_prime_bits (socket->session, 1024);

	/* Set the session DB manage functions
	 */
	gnutls_db_set_retrieve_function (socket->session, db_retrieve);
	gnutls_db_set_remove_function   (socket->session, db_remove);
	gnutls_db_set_store_function    (socket->session, db_store);
	gnutls_db_set_ptr               (socket->session, socket);

# elif defined (HAVE_OPENSSL)

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

	/* Set the SSL context cache
	 */
	re = SSL_CTX_set_session_id_context (vserver->context, "SSL", 3);
	if (re != 1) {
		PRINT_ERROR_S("ERROR: OpenSSL: Unable to set SSL session-id context\n");
	}
# endif
#endif
	return ret_ok;
}


ret_t 
cherokee_socket_init_tls (cherokee_socket_t *socket, cherokee_virtual_server_t *vserver)
{
#ifdef HAVE_TLS
	int re;
	
	socket->is_tls = TLS;

	if (socket->initialized == false) {
		initialize_tls_session (socket, vserver);
		socket->initialized = true;
	}

# ifdef HAVE_GNUTLS
	re = gnutls_handshake (socket->session);

	switch (re) {
	case GNUTLS_E_AGAIN:
		return ret_eagain;
	case GNUTLS_E_INTERRUPTED:
		return ret_error;
	}
	
	if (re < 0) {
		PRINT_ERROR ("ERROR: Init GNUTLS: Handshake has failed: %s\n", gnutls_strerror(re));
		return ret_error;
	}
# endif

# ifdef HAVE_OPENSSL
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
#endif
	return ret_ok;
}


ret_t
cherokee_socket_free (cherokee_socket_t *socket)
{
	cherokee_socket_mrproper (socket);

	free (socket);
	return ret_ok;
}

ret_t       
cherokee_socket_close (cherokee_socket_t *socket)
{
	int re;

	if (socket->socket < 0) {
		return ret_error;
	}

	if (socket->is_tls == TLS) {
#ifdef HAVE_GNUTLS
		gnutls_bye (socket->session, GNUTLS_SHUT_WR);
		gnutls_deinit (socket->session);
		socket->session = NULL;
#endif

#ifdef HAVE_OPENSSL
		SSL_shutdown (socket->session);
#endif
	}

#ifdef _WIN32
	re = closesocket (socket->socket);
#else	
	re = close (socket->socket);
#endif

	TRACE (ENTRIES",close", "fd=%d is_tls=%d re=%d\n", socket->socket, socket->is_tls, re);

	socket->socket = -1;
	socket->status = socket_closed;
	socket->is_tls = non_TLS;

	return (re == 0) ? ret_ok : ret_error;
}


ret_t 
cherokee_socket_shutdown (cherokee_socket_t *socket, int how)
{
	int re;

	if (socket->socket < 0) {
		return ret_error;
	}

	re = shutdown (socket->socket, how);	

	return (re == 0)? ret_ok : ret_error;
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
	int r;

#ifdef HAVE_IPV6
	if (SOCKET_AF(socket) == AF_INET6) {
		r = inet_pton (AF_INET6, host->buf, &SOCKET_SIN_ADDR(socket));
	} else 
#endif

#if defined(HAVE_INET_PTON)
		r = inet_pton (AF_INET, host->buf, &SOCKET_SIN_ADDR(socket));
#else		
	r = inet_aton (host->buf, &SOCKET_SIN_ADDR(socket));
#endif

	return (r > 0) ? ret_ok : ret_error;
}


ret_t 
cherokee_socket_accept (cherokee_socket_t *socket, int server_socket)
{
	ret_t               ret;
	int                 fd;
	cherokee_sockaddr_t sa;

	ret = cherokee_socket_accept_fd (server_socket, &fd, &sa);
	if (unlikely(ret < ret_ok)) return ret;

	ret = cherokee_socket_set_sockaddr (socket, fd, &sa);
	if (unlikely(ret < ret_ok)) return ret;

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
	
	SOCKET_FD(socket) = fd;
	return ret_ok;
}

ret_t
cherokee_socket_accept_fd (int server_socket, int *new_fd, cherokee_sockaddr_t *sa)
{
	socklen_t len;
	int       new_socket;
	int       tmp = 1;

	/* Get the new connection
	 */
	len = sizeof (cherokee_sockaddr_t);
	new_socket = accept (server_socket, &sa->sa, &len);
	if (new_socket < 0) {
		return ret_error;
	}		

	/* Close-on-exec
	 */
	CLOSE_ON_EXEC (new_socket);
	
	/* Disable Nagle's algorithm for this connection.
	 * Written data to the network is not buffered pending
	 * acknowledgement of previously written data.
	 */
 	setsockopt (new_socket, IPPROTO_TCP, TCP_NODELAY, (const void *)&tmp, sizeof(tmp));
	
	/* Enables nonblocking I/O.
	 */
	cherokee_fd_set_nonblocking (new_socket);
	
	*new_fd = new_socket;
	return ret_ok;
}


ret_t 
cherokee_socket_set_client (cherokee_socket_t *sock, int type)
{
	sock->socket = socket (type, SOCK_STREAM, 0);
	if (sock->socket < 0) {
		return ret_error;
	}
	
	SOCKET_AF(sock) = type;

	return ret_ok;
}


ret_t 
cherokee_write (cherokee_socket_t *socket, const char *buf, int buf_len, size_t *written)
{
	ssize_t len;

	return_if_fail (buf != NULL, ret_error);


	if (socket->is_tls == TLS) {
#ifdef HAVE_GNUTLS
		len = gnutls_record_send (socket->session, buf, buf_len);
		
		if (len < 0) {
			switch (len) {
			case GNUTLS_E_PUSH_ERROR:
			case GNUTLS_E_INTERRUPTED:
			case GNUTLS_E_INVALID_SESSION: 
				socket->status = socket_closed;
				return ret_eof;
			case GNUTLS_E_AGAIN:           
				return ret_eagain;
			}

			PRINT_ERROR ("ERROR: GNUTLS: gnutls_record_send(%d, ..) -> err=%d '%s'\n", 
				     SOCKET_FD(socket), (int)len, gnutls_strerror(len));
			return ret_error;

		} else if (len == 0) {
			socket->status = socket_closed;
			return ret_eof;
		}
#endif

#ifdef HAVE_OPENSSL
		len = SSL_write (socket->session, buf, buf_len);

		if (len < 0) {
			int re;

			re = SSL_get_error (socket->session, len);
			switch (re) {
			case SSL_ERROR_WANT_WRITE: return ret_eagain;
			case SSL_ERROR_SSL:        return ret_error;
			}

			PRINT_ERROR ("ERROR: SSL_write (%d, ..) -> err=%d '%s'\n", 
				     SOCKET_FD(socket), len, ERR_error_string(re, NULL));
			return ret_error;

		} else if (len == 0) {
			socket->status = socket_closed;
			return ret_eof;
		}
#endif		

		goto out;
	}

	len = send (SOCKET_FD(socket), buf, buf_len, 0);
	if (len < 0) {
		int err = SOCK_ERRNO();

		switch (err) {
#if defined(EWOULDBLOCK) && (EWOULDBLOCK != EAGAIN)
		case EWOULDBLOCK:
#endif	
		case EAGAIN:      
		case EINTR:       
			return ret_eagain;
		case EPIPE:       
		case ETIMEDOUT:
		case ECONNRESET:  
		case EHOSTUNREACH:
			socket->status = socket_closed;
			return ret_eof;
		}
	
		PRINT_ERROR ("ERROR: write(%d, ..) -> errno=%d '%s'\n", 
			     SOCKET_FD(socket), err, strerror(err));
		return ret_error;

	}  else if (len == 0) {
		socket->status = socket_closed;
		return ret_eof;
	}

out:
	/* Return info
	 */
	*written = len;
	return ret_ok;
}


ret_t
cherokee_read (cherokee_socket_t *socket, char *buf, int buf_size, size_t *done)
{
	ssize_t len;

	if ((socket->is_tls == TLS) && (buf != NULL)) {
#ifdef HAVE_GNUTLS
		len = gnutls_record_recv (socket->session, buf, buf_size);
		if (len < 0) {
			switch (len) {
			case GNUTLS_E_PUSH_ERROR:
			case GNUTLS_E_INTERRUPTED:              
			case GNUTLS_E_INVALID_SESSION:
			case GNUTLS_E_UNEXPECTED_PACKET_LENGTH:
				socket->status = socket_closed;
				return ret_eof;
			case GNUTLS_E_AGAIN:
				return ret_eagain;
			}
			
			PRINT_ERROR ("ERROR: GNUTLS: gnutls_record_recv(%d, ..) -> err=%d '%s'\n", 
				     SOCKET_FD(socket), (int)len, gnutls_strerror(len));
			return ret_error;
			
		} else if (len == 0) {
			socket->status = socket_closed;
			return ret_eof;
		}
#endif

#ifdef HAVE_OPENSSL
		len = SSL_read (socket->session, buf, buf_size);
		if (len < 0) {
			int re;

			re = SSL_get_error (socket->session, len);
			switch (re) {
			case SSL_ERROR_WANT_READ:   
				return ret_eagain;
			case SSL_ERROR_ZERO_RETURN: 
				socket->status = socket_closed;
				return ret_eof;
			case SSL_ERROR_SSL:         
				return ret_error;
			}

			PRINT_ERROR ("ERROR: OpenSSL: SSL_read (%d, ..) -> err=%d '%s'\n", 
				     SOCKET_FD(socket), len, ERR_error_string(re, NULL));
			return ret_error;

		} else if (len == 0) {
			socket->status = socket_closed;
			return ret_eof;
		}
#endif
		goto out;
	}

	/* Plain read
	 */
	if (unlikely (buf == NULL)) {
		static char trash[256];
		len = recv (SOCKET_FD(socket), trash, 256, 0);
	} else {
		len = recv (SOCKET_FD(socket), buf, buf_size, 0);
	}

	if (len < 0) {
		int err = SOCK_ERRNO();

		switch (err) {
#if defined(EWOULDBLOCK) && (EWOULDBLOCK != EAGAIN)
		case EWOULDBLOCK:
#endif
		case EINTR:      
		case EAGAIN:    
			return ret_eagain;
		case EBADF:
		case EPIPE: 
		case ENOTSOCK:
		case ETIMEDOUT:
		case ECONNRESET:
		case EHOSTUNREACH:
			socket->status = socket_closed;
			return ret_eof;
		}

		PRINT_ERROR ("ERROR: read(%d, ..) -> errno=%d '%s'\n", 
			     SOCKET_FD(socket), err, strerror(err));
		return ret_error;

	} else if (len == 0) {
		socket->status = socket_closed;
		return ret_eof;
	}
	
out:
	/* Return info
	 */
	if (done != NULL)
		*done = len;

	return ret_ok;
}


ret_t 
cherokee_writev (cherokee_socket_t *socket, const struct iovec *vector, uint16_t vector_len, size_t *written)
{
	int re;

#ifdef _WIN32
	int i;
	size_t total;
	
	for (i = 0, re = 0, total = 0; i < vector_len; i++, vector++) {
		re = send (SOCKET_FD(socket), vector->iov_base, vector->iov_len, 0);
		if (re < 0)
			break;
		total += re;
	}
	if (re >= 0)
		re = total;
#else
	re = writev (SOCKET_FD(socket), vector, vector_len);
#endif

	if (re <= 0) {
		int err = SOCK_ERRNO();
		
		switch (err) {
#if defined(EWOULDBLOCK) && (EWOULDBLOCK != EAGAIN)
		case EWOULDBLOCK:
#endif
		case EAGAIN:
		case EINTR: 
			return ret_eagain;
		case EPIPE:
		case ETIMEDOUT:
		case ECONNRESET:
			socket->status = socket_closed;
			return ret_eof;
		}
	       
		PRINT_ERROR ("ERROR: writev(%d, ..) -> errno=%d '%s'\n", 
			     SOCKET_FD(socket), err, strerror(err));
		return ret_error;
	}
	
	*written = re;
	return ret_ok;
}


ret_t       
cherokee_socket_write (cherokee_socket_t *socket, cherokee_buffer_t *buf, size_t *written)
{
	ret_t   ret;
	size_t _written;

	ret = cherokee_write (socket, buf->buf, buf->len, &_written);

	TRACE (ENTRIES",write", "write fd=%d len=%d ret=%d done=%d\n", socket->socket, buf->len, ret, _written);

	*written = _written;
	return ret;
}


ret_t      
cherokee_socket_read (cherokee_socket_t *socket, cherokee_buffer_t *buf, size_t count, size_t *done)
{
	ret_t    ret;
	char    *starting;
	
	/* Special case: read to empty the buffer
	 */
	if (buf == NULL) {
		return cherokee_read (socket, NULL, count, NULL);
	}

	/* Read
	 */
	ret = cherokee_buffer_ensure_size (buf, buf->len + count +2);
	if (unlikely(ret < ret_ok)) return ret;

	starting = buf->buf + buf->len;

	ret = cherokee_read (socket, starting, count, done);
	if (ret == ret_ok) {
		buf->len += *done;
		buf->buf[buf->len] = '\0';
	}

	TRACE (ENTRIES",read", "read fd=%d count=%d ret=%d done=%d\n", socket->socket, count, ret, *done);

	return ret;
}


ret_t 
cherokee_socket_sendfile (cherokee_socket_t *socket, int fd, size_t size, off_t *offset, ssize_t *sent)
{
#ifndef HAVE_SENDFILE
	SHOULDNT_HAPPEN;
	return ret_error;
#else

# ifdef FREEBSD_SENDFILE_API
	int            re;
	struct sf_hdtr hdr;
	struct iovec   hdtrl;

	hdr.headers    = &hdtrl;
	hdr.hdr_cnt    = 1;
	hdr.trailers   = NULL;
	hdr.trl_cnt    = 0;
	
	hdtrl.iov_base = NULL;
	hdtrl.iov_len  = 0;


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

	if (*sent < 0) {
		if (errno == EAGAIN) {
			return ret_eagain;
		}
		return ret_error;
	}
	*offset = *offset + *sent;

# elif HPUX_SENDFILE_API
	
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
		*sent = sendfile (SOCKET_FD(socket),      /* socket          */
				  fd,                     /* fd to send      */
				  *offset,                /* where to start  */
				  size,                   /* bytes to send   */
				  NULL,                   /* Headers/footers */
				  0);                     /* flags           */
	} while ((*sent == -1) && (errno == EINTR));

	if (*sent < 0) {
		if (errno == EAGAIN) {
			return ret_eagain;
		}
		return ret_error;
	}
	*offset = *offset + *sent;

# elif SOLARIS_SENDFILE_API

	/* TODO!!
	 */
	return ret_no_sys;

# elif HAVE_SENDFILE_BROKEN

	/* Some Linux 2.4 kernels doesn't support sendfile in a LFS environment
	 */
	return ret_no_sys;

# elif LINUX_SENDFILE_API

	/* Linux sendfile
	 *
	 * ssize_t 
	 * sendfile (int out_fd, int in_fd, off_t *offset, size_t *count);
	 */
	do {
		*sent = sendfile (SOCKET_FD(socket),            /* int     out_fd */
				  fd,                           /* int     in_fd  */
				  offset,                       /* off_t  *offset */
				  size);                        /* size_t  count  */
	} while ((*sent == -1) && (errno == EINTR));
		
	if (*sent < 0) {
		switch (errno) {
		case ENOSYS: return ret_no_sys;
		case EAGAIN: return ret_eagain;
		}

		return ret_error;
	}

# else
	SHOULDNT_HAPPEN;
	return ret_no_sys;
# endif

	return ret_ok;

#endif /* HAVE_SENDFILE */
}


ret_t
cherokee_socket_gethostbyname (cherokee_socket_t *socket, cherokee_buffer_t *hostname)
{
	if (SOCKET_AF(socket) == AF_UNIX) {
#ifdef _WIN32
		SHOULDNT_HAPPEN;
		return ret_no_sys;
#else
		SOCKET_ADDR_UNIX(socket).sun_family = AF_UNIX;
		memset ((char*) SOCKET_SUN_PATH (socket), 0, sizeof (SOCKET_ADDR_UNIX(socket)));
		strncpy (SOCKET_SUN_PATH (socket), hostname->buf, hostname->len);
		return ret_ok;
#endif
	}

	return cherokee_gethostbyname (hostname->buf, &SOCKET_SIN_ADDR(socket));
}


ret_t 
cherokee_socket_connect (cherokee_socket_t *socket)
{
	int r;

	if (SOCKET_AF(socket) == AF_UNIX) {
#ifndef _WIN32
		r = connect (SOCKET_FD(socket), (struct sockaddr *) &SOCKET_ADDR_UNIX(socket), sizeof(SOCKET_ADDR_UNIX(socket)));
#else
		SHOULDNT_HAPPEN;
		return ret_no_sys;
#endif
	} else {
#ifdef HAVE_IPV6
		r = connect (SOCKET_FD(socket), (struct sockaddr *) &SOCKET_ADDR(socket), sizeof(struct sockaddr_in6));
#else
		r = connect (SOCKET_FD(socket), (struct sockaddr *) &SOCKET_ADDR(socket), sizeof(struct sockaddr_in));
#endif
	}

	if (r < 0) {
		int err = SOCK_ERRNO();
		
		switch (err) {
		case ECONNREFUSED:
			return ret_deny;
		case EAGAIN:
#if defined(EWOULDBLOCK) && (EWOULDBLOCK != EAGAIN)
		case EWOULDBLOCK:
#endif
			return ret_eagain;
		default:           
#if 1
			PRINT_ERROR ("ERROR: Can not connect: %s\n", strerror(err));
#endif
			return ret_error;
		}
	}

	socket->status = socket_reading;
	return ret_ok;
}


ret_t 
cherokee_socket_init_client_tls (cherokee_socket_t *socket)
{
#ifdef HAVE_TLS
	int re;

	socket->is_tls = TLS;

# ifdef HAVE_GNUTLS
	const int kx_priority[] = {GNUTLS_KX_ANON_DH, 0};

	gnutls_anon_client_credentials   anoncred;
	
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
			case GNUTLS_E_AGAIN:
			case GNUTLS_E_INTERRUPTED:
				break;
			default:
				return ret_error;
			}
		}
	} while ((re == GNUTLS_E_AGAIN) ||
		 (re == GNUTLS_E_INTERRUPTED));

# elif defined(HAVE_OPENSSL)

	/* New context
	 */
	socket->ssl_ctx = SSL_CTX_new (SSLv23_client_method());
	if (socket->ssl_ctx == NULL) {
		char *error;

		OPENSSL_LAST_ERROR(error);
		PRINT_ERROR ("ERROR: OpenSSL: Unable to create a new SSL context: %s\n", error);
		return ret_error;
	}

	SSL_CTX_set_default_verify_paths (socket->ssl_ctx);
	SSL_CTX_load_verify_locations (socket->ssl_ctx, NULL, NULL);

	SSL_CTX_set_verify (socket->ssl_ctx, SSL_VERIFY_NONE, NULL);
	SSL_CTX_set_mode (socket->ssl_ctx, SSL_MODE_ENABLE_PARTIAL_WRITE);

	/* New session
	 */
	socket->session = SSL_new (socket->ssl_ctx);
	if (socket->session == NULL) {
		char *error;

		OPENSSL_LAST_ERROR(error);
		PRINT_ERROR ("ERROR: OpenSSL: Unable to create a new SSL connection "
			     "from the SSL context: %s\n", error);
		return ret_error;
	}

	/* Set the socket file descriptor
	 */
	re = SSL_set_fd (socket->session, socket->socket);
	if (re != 1) {
		char *error;

		OPENSSL_LAST_ERROR(error);
		PRINT_ERROR ("ERROR: OpenSSL: Can not set fd(%d): %s\n", socket->socket, error);
		return ret_error;
	}

	SSL_set_connect_state (socket->session); 

	re = SSL_connect (socket->session);
	if (re <= 0) {
		char *error;

		OPENSSL_LAST_ERROR(error);
		PRINT_ERROR ("ERROR: OpenSSL: Can not connect: %s\n", error);
		return ret_error;
	}

# endif
#endif
	return ret_ok;
}


ret_t 
cherokee_socket_set_timeout (cherokee_socket_t *socket, cuint_t timeout)
{
	int            re;
	cuint_t        block;
	struct timeval tv;

	if (socket->socket < 0) {
		return ret_error;
	}

	if (timeout < 0)
		timeout = 0;
	
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
#if defined(SO_RCVTIMEO) && !defined(HAVE_BROKEN_SO_RCVTIMEO)
	tv.tv_sec  = timeout / 1000;
	tv.tv_usec = timeout % 1000;
	
	re = setsockopt (socket->socket, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
	if (re < 0) {
		int err = errno;
		PRINT_ERROR ("Couldn't set SO_RCVTIMEO, fd=%d, timeout=%d: %s\n", 
			     socket->socket, timeout, strerror(err));
	}
#endif
	
	return ret_ok;		      
}


ret_t       
cherokee_socket_set_status (cherokee_socket_t *socket, cherokee_socket_status_t status)
{
	socket->status = status;
	return ret_ok;
}

