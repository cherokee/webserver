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

#define ENTRIES "socket"


#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>

#include "util.h"
#include "buffer.h"
#include "server-protected.h"
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

	socket->socket  = -1;
	socket->status  = socket_closed;
	socket->is_tls  = non_TLS;
	socket->cryptor = NULL;

	return ret_ok;
}


ret_t
cherokee_socket_mrproper (cherokee_socket_t *socket)
{
	if (socket->cryptor != NULL) {
		cherokee_cryptor_socket_free (socket->cryptor);
		socket->cryptor = NULL;
	}

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
	
	if (socket->cryptor != NULL) {
		cherokee_cryptor_socket_clean (socket->cryptor);
		socket->cryptor = NULL;
	}

	return ret_ok;
}


ret_t 
cherokee_socket_init_tls (cherokee_socket_t         *socket,
			  cherokee_virtual_server_t *vserver)
{
	ret_t              ret;
	cherokee_server_t *srv = VSERVER_SRV(vserver);

	if (socket->cryptor == NULL) {
		ret = cherokee_cryptor_socket_new (srv->cryptor, &socket->cryptor);
		if (ret != ret_ok)
			return ret;
	}

	ret = cherokee_cryptor_socket_init_tls (socket->cryptor, socket, vserver);
	if (ret != ret_ok) {
		return ret;
	}

	socket->is_tls = TLS;
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
	if ((socket->is_tls == TLS) &&
	    (socket->cryptor != NULL))
	{
		cherokee_cryptor_socket_close (socket->cryptor);
	}
	
	/* Close the socket
	 */
#ifdef _WIN32
	ret = closesocket (socket->socket);
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
	int           re;
	ret_t         ret;
	socklen_t     len;
	int           new_socket;
	struct linger linger;

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

	/* Deal with the FIN_WAIT2 state
	 */
	re = 1;
	re = setsockopt (new_socket, SOL_SOCKET, SO_KEEPALIVE, &re, sizeof(re));
	if (re == -1) {
		PRINT_ERRNO (errno, "WARNING: Couldn't set SO_KEEPALIVE on fd=%d: ${errno}",
			     new_socket);
	}	

	linger.l_onoff  = 1;
	linger.l_linger = 0;

	re = setsockopt (new_socket, SOL_SOCKET, SO_LINGER, &linger, sizeof(linger));
	if (re == -1) {
		PRINT_ERRNO (errno, "WARNING: Couldn't set SO_LINGER on fd=%d: ${errno}",
			     new_socket);
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
		PRINT_ERROR_S ("Could not disable Nagle's algorithm.\n");
		return ret_error;
	}

	/* Enables nonblocking I/O.
	 */
	ret = cherokee_fd_set_nonblocking (new_socket, true);
	if (ret != ret_ok) {
		PRINT_ERROR_S ("Could not set non-blocking.\n");
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
cherokee_socket_write (cherokee_socket_t *socket,
		       const char        *buf,
		       int                buf_len,
		       size_t            *pcnt_written)
{
	ret_t   ret;
	ssize_t len;

	*pcnt_written = 0;

	/* There must be something to send, otherwise behaviour is undefined
	 * and as we don't want this case, we have to enforce assertions.
	 */
	return_if_fail (buf != NULL && buf_len > 0, ret_error);

	if (likely (socket->is_tls != TLS)) {
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
		}
		return ret_error;

	} else if (socket->cryptor != NULL) {
		ret = cherokee_cryptor_socket_write (socket->cryptor,
						     (char *)buf, buf_len, pcnt_written);
		switch (ret) {
		case ret_ok:
		case ret_error:
		case ret_eagain:
			return ret;
		case ret_eof:
			socket->status = socket_closed;
			return ret_eof;
		default:
			RET_UNKNOWN(ret);
			return ret;
		}
	}

	return ret_error;
}


/* WARNING: all parameters MUST be valid,
 *          NULL pointers lead to a crash.
 */
ret_t
cherokee_socket_read (cherokee_socket_t *socket,
		      char              *buf,
		      int                buf_size,
		      size_t            *pcnt_read)
{
	ret_t   ret;
	ssize_t len;

	*pcnt_read = 0;

	/* There must be something to read, otherwise behaviour is undefined
	 * and as we don't want this case, we have to enforce assertions.
	 */
	return_if_fail (buf != NULL && buf_size > 0, ret_error);

	if (unlikely (socket->status == socket_closed))
		return ret_eof;

	if (likely (socket->is_tls != TLS)) {
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
		}
		return ret_error;

	} else if (socket->cryptor != NULL) {
		ret = cherokee_cryptor_socket_read (socket->cryptor,
						    buf, buf_size, pcnt_read);
		switch (ret) {
		case ret_ok:
		case ret_error:
		case ret_eagain:
			return ret;
		case ret_eof:
			socket->status = socket_closed;
			return ret_eof;
		default:
			RET_UNKNOWN(ret);
			return ret;
		}
	}

	return ret_error;
}


int
cherokee_socket_pending_read (cherokee_socket_t *socket)
{
	if (socket->is_tls != TLS)
		return 0;

	if (unlikely ((socket->status != socket_reading) &&
		      (socket->status != socket_writing)))
		return 0;

	if (socket->cryptor != NULL) {
		return cherokee_cryptor_socket_pending (socket->cryptor);
	}

	SHOULDNT_HAPPEN;
	return 0;
}


ret_t
cherokee_socket_flush (cherokee_socket_t *socket)
{
	int re;
	int op = 1;

	TRACE (ENTRIES, "flushing fd=%d\n", socket->socket);

	do {
		re = setsockopt (SOCKET_FD(socket), IPPROTO_TCP, TCP_NODELAY,
				 (const void *) &op, sizeof(int));
	} while ((re == -1) && (errno == EINTR));
	
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
	int    re;
	int    i;
	ret_t  ret;
	size_t cnt;

	*pcnt_written = 0;

	/* There must be something to send, otherwise behaviour is undefined
	 * and as we don't want this case, we have to enforce assertions.
	 */
	return_if_fail (vector != NULL && vector_len > 0, ret_error);

	if (likely (socket->is_tls != TLS))
	{
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
		}
		return ret_error;

	} 

	/* TLS connection: Here we don't worry about sparing a few CPU
	 * cycles, so we reuse the single send case for TLS.
	 */
	for (i = 0; i < vector_len; i++) {
		if ((vector[i].iov_len == 0) ||
		    (vector[i].iov_base == NULL))
			continue;
		
		cnt = 0;
		ret = cherokee_socket_write (socket, vector[i].iov_base, vector[i].iov_len, &cnt);
		*pcnt_written += cnt;
		
		if ((ret == ret_ok) &&
		    (cnt == vector[i].iov_len))
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
		case ECONNRESET:
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
cherokee_socket_init_client_tls (cherokee_socket_t *socket,
				 cherokee_buffer_t *host)
{
	ret_t ret;

	if (unlikely (socket->cryptor == NULL)) {
		SHOULDNT_HAPPEN;
		return ret_error;
	}

	ret = cherokee_cryptor_client_init (CRYPTOR_CLIENT(socket->cryptor), host, socket);
	if (ret != ret_ok)
		return ret;

	socket->is_tls = TLS;
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
	int re;
	int tmp = 0;
	int fd  = socket->socket;

	if (enable) {
		tmp = 0;
		do {
			re = setsockopt (fd, IPPROTO_TCP, TCP_NODELAY, &tmp, sizeof(tmp));
		} while ((re == -1) && (errno == EINTR));
			
		if (unlikely (re < 0)) {
			PRINT_ERRNO (errno, "ERROR: Removing TCP_NODELAY to fd %d: ${errno}", fd);
			return ret_error;
		}

		TRACE(ENTRIES",cork,nodelay", "Set TCP_NODELAY on fd %d\n", fd);

#ifdef TCP_CORK
		tmp = 1;
		do {
			re = setsockopt (fd, IPPROTO_TCP, TCP_CORK, &tmp, sizeof(tmp));
		} while ((re == -1) && (errno == EINTR));

		if (unlikely (re < 0)) {
			PRINT_ERRNO (errno, "ERROR: Setting TCP_CORK to fd %d: ${errno}", fd);
			return ret_error;
		}

		TRACE(ENTRIES",cork", "Set TCP_CORK on fd %d\n", fd);
#endif

		return ret_ok;
	}

#ifdef TCP_CORK
	tmp = 0;
	do {
		re = setsockopt (fd, IPPROTO_TCP, TCP_CORK, &tmp, sizeof(tmp));
	} while ((re == -1) && (errno == EINTR));
	if (unlikely (re < 0)) {
		PRINT_ERRNO (errno, "ERROR: Removing TCP_CORK to fd %d: ${errno}", fd);
		return ret_error;
	}

	TRACE(ENTRIES",cork", "Removed TCP_CORK on fd %d\n", fd);
#endif 

	tmp = 1;
	do {
		re = setsockopt (fd, IPPROTO_TCP, TCP_NODELAY, &tmp, sizeof(tmp));
	} while ((re == -1) && (errno == EINTR));
	if (unlikely (re < 0)) {
		PRINT_ERRNO (errno, "ERROR: Setting TCP_NODELAY to fd %d: ${errno}", fd);
		return ret_error;
	}

	TRACE(ENTRIES",cork,nodelay", "Removed TCP_NODELAY on fd %d\n", fd);
	return ret_ok;
}
