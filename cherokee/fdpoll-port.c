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
#include "fdpoll-protected.h"
#include "util.h"

#include <stdio.h>
#include <poll.h>
#include <port.h>
#include <unistd.h>
#include <errno.h>

#define WRITE ""
#define READ  NULL

#define POLL_READ   (POLLIN)
#define POLL_WRITE  (POLLOUT)
#define POLL_ERROR  (POLLHUP | POLLERR | POLLNVAL)



/***********************************************************************/
/* Solaris 10: Event ports                                             */
/*                                                                     */
/* #include <port.h>                                                   */
/*                                                                     */
/* int port_get (int port, port_event_t  *pe,                          */
/*               const timespec_t        *timeout);                    */
/*                                                                     */
/* Info:                                                               */
/* http://iforce.sun.com/protected/solaris10/adoptionkit/tech/tecf.html*/
/*                                                                     */
/* Event ports is an event completion framework that provides a        */
/* scalable way for multiple threads or processes to wait for multiple */
/* pending asynchronous events from multiple objects.                  */
/*                                                                     */
/***********************************************************************/


typedef struct {
	struct cherokee_fdpoll poll;

	int                    port;
	port_event_t           *port_events;
	int                    *port_activefd;
	int                    port_readyfds;
} cherokee_fdpoll_port_t;


static ret_t
fd_associate( cherokee_fdpoll_port_t *fdp, int fd, void *rw )
{
	int rc;

	rc = port_associate (fdp->port,                /* port */
	                     PORT_SOURCE_FD,           /* source */
	                     fd,                       /* object */
	                     rw?POLL_WRITE:POLL_READ,  /* events */
	                     rw);                      /* user data */

	if ( rc == -1 ) {
		LOG_ERRNO (errno, cherokee_err_error,
		           CHEROKEE_ERROR_FDPOLL_PORTS_ASSOCIATE, fd);
		return ret_error;
	}

	return ret_ok;
}


static ret_t
_free (cherokee_fdpoll_port_t *fdp)
{
	if (fdp == NULL)
		return ret_ok;

	if ( fdp->port >= 0 ) {
		close (fdp->port);
	}

	/* ANSI C required, so that free() can handle NULL pointers
	 */
	free( fdp->port_events );
	free( fdp->port_activefd );

	/* Caller has to set this pointer to NULL.
	 */
	free( fdp );

	return ret_ok;
}


static ret_t
_add (cherokee_fdpoll_port_t *fdp, int fd, int rw)
{
	int rc;

	rc = fd_associate(fdp, fd, (rw == FDPOLL_MODE_WRITE ? WRITE : READ));
	if ( rc == -1 ) {
		LOG_ERRNO (errno, cherokee_err_error,
		           CHEROKEE_ERROR_FDPOLL_PORTS_FD_ASSOCIATE, fd);
		return ret_error;
	}

	FDPOLL(fdp)->npollfds++;
	return ret_ok;
}


static ret_t
_del (cherokee_fdpoll_port_t *fdp, int fd)
{
	int rc;

	rc = port_dissociate( fdp->port,      /* port */
	                      PORT_SOURCE_FD, /* source */
	                      fd);            /* object */
	if ( rc == -1 ) {
		LOG_ERRNO (errno, cherokee_err_error,
		           CHEROKEE_ERROR_FDPOLL_PORTS_ASSOCIATE, fd);
		return ret_error;
	}

	FDPOLL(fdp)->npollfds--;

	/* remote fd from the active fd list
	 */
	fdp->port_activefd[fd] = -1;

	return ret_ok;
}


static int
_watch (cherokee_fdpoll_port_t *fdp, int timeout_msecs)
{
	int          i, rc, fd;
	struct timespec  timeout;

	timeout.tv_sec  = timeout_msecs/1000L;
	timeout.tv_nsec = ( timeout_msecs % 1000L ) * 1000000L;

	for (i=0; i<FDPOLL(fdp)->system_nfiles; i++) {
		fdp->port_activefd[i] = -1;
	}

	/* First call to get the number of file descriptors with activity
	 */
	rc = port_getn (fdp->port, fdp->port_events, 0,
	                (uint_t *)&fdp->port_readyfds,
	                &timeout);
	if ( rc < 0 ) {
		LOG_ERRNO_S (errno, cherokee_err_error, CHEROKEE_ERROR_FDPOLL_PORTS_GETN);
		return 0;
	}

	if ( fdp->port_readyfds == 0 ) {
		/* Get at least 1 fd to wait for activity
		 */
		fdp->port_readyfds = 1;
	}

	/* Second call to get the events of the file descriptors with
	 * activity
	 */
	rc = port_getn (fdp->port, fdp->port_events,FDPOLL(fdp)->nfiles,
	                &fdp->port_readyfds, &timeout);
	if ( ( (rc < 0) && (errno != ETIME) ) || (fdp->port_readyfds == -1)) {
		LOG_ERRNO_S (errno, cherokee_err_error, CHEROKEE_ERROR_FDPOLL_PORTS_GETN);
		return 0;
	}

	for ( i = 0; i < fdp->port_readyfds; ++i ) {
		int nfd;

		nfd = fdp->port_events[i].portev_object;
		fdp->port_activefd[nfd] = fdp->port_events[i].portev_events;
		rc = fd_associate( fdp,
		                   nfd,
		                   fdp->port_events[i].portev_user);
		if ( rc < 0 ) {
			LOG_ERRNO (errno, cherokee_err_error,
			           CHEROKEE_ERROR_FDPOLL_PORTS_FD_ASSOCIATE, nfd);
		}
	}

	return fdp->port_readyfds;
}


static int
_check (cherokee_fdpoll_port_t *fdp, int fd, int rw)
{
	uint32_t events;

	/* Sanity check: is it a wrong fd?
	 */
	if ( fd < 0 ) return -1;

	events = fdp->port_activefd[fd];
	if ( events == -1 ) return 0;

	switch (rw) {
	case FDPOLL_MODE_READ:
		events &= (POLL_READ | POLL_ERROR);
		break;
	case FDPOLL_MODE_WRITE:
		events &= (POLL_WRITE | POLL_ERROR);
		break;
	}

	return events;
}


static ret_t
_reset (cherokee_fdpoll_port_t *fdp, int fd)
{
	UNUSED (fdp);
	UNUSED (fd);

	return ret_ok;
}


static ret_t
_set_mode (cherokee_fdpoll_port_t *fdp, int fd, int rw)
{
	int rc;

	rc = port_associate( fdp->port,
	                     PORT_SOURCE_FD,
	                     fd,
	                    (rw == FDPOLL_MODE_WRITE ? POLLOUT : POLLIN),
	                    (rw == FDPOLL_MODE_WRITE ? WRITE   : READ));
	if ( rc == -1 ) {
		LOG_ERRNO (errno, cherokee_err_error,
		           CHEROKEE_ERROR_FDPOLL_PORTS_ASSOCIATE, fd);
		return ret_error;
	}

	return ret_ok;
}


ret_t
fdpoll_port_get_fdlimits (cuint_t *system_fd_limit, cuint_t *fd_limit)
{
	*system_fd_limit = 0;
	*fd_limit        = 0;

	return ret_ok;
}


ret_t
fdpoll_port_new (cherokee_fdpoll_t **fdp, int sys_limit, int limit)
{
	int                re;
	cuint_t            i;
	cherokee_fdpoll_t *nfd;
	CHEROKEE_CNEW_STRUCT (1, n, fdpoll_port);

	nfd = FDPOLL(n);

	/* Init base class properties
	 */
	nfd->type          = cherokee_poll_port;
	nfd->nfiles        = limit;
	nfd->system_nfiles = sys_limit;
	nfd->npollfds      = 0;

	/* Init base class virtual methods
	 */
	nfd->free          = (fdpoll_func_free_t) _free;
	nfd->add           = (fdpoll_func_add_t) _add;
	nfd->del           = (fdpoll_func_del_t) _del;
	nfd->reset         = (fdpoll_func_reset_t) _reset;
	nfd->set_mode      = (fdpoll_func_set_mode_t) _set_mode;
	nfd->check         = (fdpoll_func_check_t) _check;
	nfd->watch         = (fdpoll_func_watch_t) _watch;

	/* Allocate data
	 */
	n->port = -1;
	n->port_readyfds = 0;
	n->port_events   = (port_event_t *) calloc(nfd->nfiles, sizeof(port_event_t));
	n->port_activefd = (int *) calloc(nfd->system_nfiles, sizeof(int));

	if ( n->port_events == NULL || n->port_activefd == NULL ) {
		_free( n );
		return ret_nomem;
	}

	for (i=0; i < nfd->system_nfiles; i++) {
		n->port_activefd[i] = -1;
	}

	n->port = port_create();
	if (n->port == -1 ) {
		_free( n );
		return ret_error;
	}

	re = fcntl (n->port, F_SETFD, FD_CLOEXEC);
	if (re < 0) {
		LOG_ERRNO (errno, cherokee_err_error,
		           CHEROKEE_ERROR_FDPOLL_EPOLL_CLOEXEC);
		_free (n);
		return ret_error;
	}

	/* Return the object
	 */
	*fdp = nfd;
	return ret_ok;
}

