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

#ifndef CHEROKEE_SOCKET_H
#define CHEROKEE_SOCKET_H

#include "common-internal.h"

#include "socket_lowlevel.h"
#include "buffer.h"
#include "virtual_server.h"
#include "fdpoll.h"
#include "cryptor.h"


/* Socket status
 */
typedef enum {
	socket_reading = FDPOLL_MODE_READ,
	socket_writing = FDPOLL_MODE_WRITE,
	socket_closed  = FDPOLL_MODE_NONE
} cherokee_socket_status_t;


/* Socket crypt
 */
typedef enum {
	non_TLS,
	TLS
} cherokee_socket_type_t;


/* Socket address
 */
typedef union {
	struct sockaddr         sa;
	struct sockaddr_in      sa_in;

#ifdef HAVE_SOCKADDR_UN
	struct sockaddr_un      sa_un;
#endif
#ifdef HAVE_SOCKADDR_IN6
	struct sockaddr_in6     sa_in6;
#endif
} cherokee_sockaddr_t;


/* Socket
 */
typedef struct {
	int                        socket;
	cherokee_sockaddr_t        client_addr;
	socklen_t                  client_addr_len;
	cherokee_socket_status_t   status;
	cherokee_socket_type_t     is_tls;
	cherokee_cryptor_socket_t *cryptor;
} cherokee_socket_t;


#define S_SOCKET(s)            ((cherokee_socket_t)(s))
#define S_SOCKET_FD(s)         ((s).socket)

#define SOCKET(s)              ((cherokee_socket_t *)(s))
#define SOCKET_FD(s)           (SOCKET(s)->socket)
#define SOCKET_AF(s)           (SOCKET(s)->client_addr.sa.sa_family)
#define SOCKET_STATUS(s)       (SOCKET(s)->status)

#define SOCKET_ADDR(s)         (SOCKET(s)->client_addr)
#define SOCKET_ADDR_UNIX(s)    ((struct sockaddr_un  *) &SOCKET_ADDR(s))
#define SOCKET_ADDR_IPv4(s)    ((struct sockaddr_in  *) &SOCKET_ADDR(s))
#define SOCKET_ADDR_IPv6(s)    ((struct sockaddr_in6 *) &SOCKET_ADDR(s))

#define SOCKET_SIN_PORT(s)     (SOCKET(s)->client_addr.sa_in.sin_port)
#define SOCKET_SIN_ADDR(s)     (SOCKET(s)->client_addr.sa_in.sin_addr)
#define SOCKET_SIN6_ADDR(s)    (SOCKET(s)->client_addr.sa_in6.sin6_addr)

#define SOCKET_SUN_PATH(s)     (SOCKET(s)->client_addr.sa_un.sun_path)

#define SOCKET_ADDRESS_IPv4(s) (SOCKET_ADDR_IPv4(s)->sin_addr.s_addr)
#define SOCKET_ADDRESS_IPv6(s) (SOCKET_ADDR_IPv6(s)->sin6_addr.s6_addr)


#define cherokee_socket_configured(c)    (SOCKET_FD(c) >= 0)
#define cherokee_socket_is_connected(c)  (cherokee_socket_configured(c) && \
                                          (SOCKET_STATUS(c) != socket_closed))


ret_t cherokee_socket_init              (cherokee_socket_t *socket);
ret_t cherokee_socket_mrproper          (cherokee_socket_t *socket);
ret_t cherokee_socket_clean             (cherokee_socket_t *socket);

ret_t cherokee_socket_init_tls          (cherokee_socket_t *socket, cherokee_virtual_server_t *vserver, cherokee_connection_t *conn, cherokee_socket_status_t *blocking);
ret_t cherokee_socket_init_client_tls   (cherokee_socket_t *socket, cherokee_buffer_t *host);

ret_t cherokee_socket_close             (cherokee_socket_t *socket);
ret_t cherokee_socket_shutdown          (cherokee_socket_t *socket, int how);
ret_t cherokee_socket_reset             (cherokee_socket_t *socket);
ret_t cherokee_socket_accept            (cherokee_socket_t *socket, cherokee_socket_t *server_socket);
ret_t cherokee_socket_accept_fd         (cherokee_socket_t *socket, int *new_fd, cherokee_sockaddr_t *sa);
int   cherokee_socket_pending_read      (cherokee_socket_t *socket);
ret_t cherokee_socket_flush             (cherokee_socket_t *socket);
ret_t cherokee_socket_test_read         (cherokee_socket_t *socket);

ret_t cherokee_socket_create_fd         (cherokee_socket_t *socket, unsigned short int family);
ret_t cherokee_socket_bind              (cherokee_socket_t *socket, cuint_t port, cherokee_buffer_t *listen_to);
ret_t cherokee_socket_listen            (cherokee_socket_t *socket, int backlog);

ret_t cherokee_socket_bufwrite          (cherokee_socket_t *socket, cherokee_buffer_t *buf, size_t *written);
ret_t cherokee_socket_bufread           (cherokee_socket_t *socket, cherokee_buffer_t *buf, size_t count, size_t *read);
ret_t cherokee_socket_sendfile          (cherokee_socket_t *socket, int fd, size_t size, off_t *offset, ssize_t *sent);
ret_t cherokee_socket_connect           (cherokee_socket_t *socket);

ret_t cherokee_socket_ntop              (cherokee_socket_t *socket, char *buf, size_t buf_size);
ret_t cherokee_socket_pton              (cherokee_socket_t *socket, cherokee_buffer_t *buf);
ret_t cherokee_socket_gethostbyname     (cherokee_socket_t *socket, cherokee_buffer_t *hostname);
ret_t cherokee_socket_set_status        (cherokee_socket_t *socket, cherokee_socket_status_t status);
ret_t cherokee_socket_set_cork          (cherokee_socket_t *socket, cherokee_boolean_t enable);


/* Low level functions
 */
ret_t cherokee_socket_read   (cherokee_socket_t *socket, char *buf, int buf_size, size_t *pcnt_read);
ret_t cherokee_socket_write  (cherokee_socket_t *socket, const char *buf, int len, size_t *pcnt_written);
ret_t cherokee_socket_writev (cherokee_socket_t *socket, const struct iovec *vector, uint16_t vector_len, size_t *pcnt_written);

/* Extra
 */
ret_t cherokee_socket_set_sockaddr         (cherokee_socket_t *socket, int fd, cherokee_sockaddr_t *sa);
ret_t cherokee_socket_update_from_addrinfo (cherokee_socket_t *socket, const struct addrinfo *addr_info, cuint_t num);

#endif /* CHEROKEE_SOCKET_H */
