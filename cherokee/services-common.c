/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/* Cherokee
 *
 * Authors:
 *      Daniel Silverstone <dsilvers@digital-scurf.org>
 *
 * Copyright (C) 2014 Alvaro Lopez Ortega
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
#include "services.h"

#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <errno.h>

ret_t
cherokee_services_send (int fd,
                        cherokee_buffer_t *buf,
                        cherokee_services_fdmap_t *fd_map)
{
	struct msghdr   sendbuf;
	struct iovec    iov[1];
	int             ret, *fdptr;
	struct cmsghdr *control_msg;
	char            control_data[CMSG_SPACE (sizeof(int) * 3)];

	sendbuf.msg_name = NULL;
	sendbuf.msg_namelen = 0;
	sendbuf.msg_iov = &iov[0];
	sendbuf.msg_iovlen = 1;
	sendbuf.msg_control = NULL;
	sendbuf.msg_controllen = 0;
	sendbuf.msg_flags = 0;

	iov[0].iov_base = buf->buf;
	iov[0].iov_len = buf->len;

	if (fd_map != NULL) {
		sendbuf.msg_control = control_data;
		sendbuf.msg_controllen = sizeof(control_data);
		control_msg = CMSG_FIRSTHDR (&sendbuf);
		control_msg->cmsg_level = SOL_SOCKET;
		control_msg->cmsg_type  = SCM_RIGHTS;
		control_msg->cmsg_len   = CMSG_LEN (sizeof(int) * 3);
		fdptr = (int*) CMSG_DATA (control_msg);
		*fdptr++ = fd_map->fd_in;
		*fdptr++ = fd_map->fd_out;
		*fdptr++ = fd_map->fd_err;
		sendbuf.msg_controllen = control_msg->cmsg_len;
	}

send_again:

	if ((ret = sendmsg (fd, &sendbuf, 0)) != buf->len) {
		if (errno == EINTR ||
		    errno == EAGAIN) {
			goto send_again;
		}
		return ret_error;
	}

	return ret_ok;
}

ret_t
cherokee_services_receive (int fd,
                           cherokee_buffer_t *buf,
                           cherokee_services_fdmap_t *fd_map)
{
	struct msghdr   recvbuf;
	struct iovec    iov[1];
	int             ret, *fdptr, nfds;
	struct cmsghdr *control_msg;
	char            control_data[CMSG_SPACE (sizeof(int) * 3)];

	recvbuf.msg_name = NULL;
	recvbuf.msg_namelen = 0;
	recvbuf.msg_iov = &iov[0];
	recvbuf.msg_iovlen = 1;
	recvbuf.msg_control = control_data;
	recvbuf.msg_controllen = sizeof (control_data);
	recvbuf.msg_flags = 0;

	iov[0].iov_base = buf->buf;
	iov[0].iov_len = buf->size;

receive_again:

	if ((ret = recvmsg (fd, &recvbuf, 0)) < 0) {
		if (errno == EINTR ||
		    errno == EAGAIN) {
			goto receive_again;
		}
		return ret_error;
	}

	if (recvbuf.msg_controllen > 0) {
		/* Received some rights, process them */
		control_msg = CMSG_FIRSTHDR (&recvbuf);
		nfds = (control_msg->cmsg_len - sizeof (*control_msg)) / sizeof(int);
		fdptr = (int *) CMSG_DATA (control_msg);
		if (nfds == 3 &&
		    fd_map != NULL &&
		    control_msg->cmsg_type == SCM_RIGHTS &&
		    control_msg->cmsg_level == SOL_SOCKET) {
			/* We got 3 and have a map to put them in */
			fd_map->fd_in = *fdptr++;
			fd_map->fd_out = *fdptr++;
			fd_map->fd_err = *fdptr++;
		} else if (control_msg->cmsg_type == SCM_RIGHTS &&
			   control_msg->cmsg_level == SOL_SOCKET) {
			/* Close the passed FDs, we don't want them */
			while (nfds--)
				close (*fdptr++);
		}
	}

	if (recvbuf.msg_flags & MSG_TRUNC) {
		return ret_nomem;
	}

	buf->len = ret;

	return ret_ok;
}
