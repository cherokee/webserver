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
#include "fdpoll-protected.h"
#include "util.h"

#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/epoll.h>


/***********************************************************************/
/* epoll:                                                              */
/*                                                                     */
/* #include <sys/epoll.h>                                              */
/*                                                                     */
/* int epoll_ctl(int epfd, int op, int fd, struct epoll_event *event); */
/*                                                                     */
/* Info:                                                               */
/* http://www.xmailserver.org/linux-patches/nio-improve.html           */
/*                                                                     */
/* As you can see in /usr/src/linux/fs/eventpoll.c, the epoll          */
/* implementation is thread-safe, so we don't need to use mutex        */
/* as using poll()                                                     */
/*                                                                     */
/***********************************************************************/

typedef struct {
	struct cherokee_fdpoll poll;
	
	int                    ep_fd;
	struct epoll_event    *ep_events;
	int                    ep_nrevents;
	int                   *epoll_fd2idx;
} cherokee_fdpoll_epoll_t;


static ret_t 
_free (cherokee_fdpoll_epoll_t *fdp)
{
	if (fdp == NULL)
		return ret_ok;

	if (fdp->ep_fd >= 0)
		close (fdp->ep_fd);

	/* ANSI C required, so that free() can handle NULL pointers
	 */
	free (fdp->ep_events);
	free (fdp->epoll_fd2idx);

	/* Caller has to set this pointer to NULL.
	 */
	free (fdp);        

	return ret_ok;
}


static ret_t
_add (cherokee_fdpoll_epoll_t *fdp, int fd, int rw)
{
	struct epoll_event ev;

	/* Check the fd limit
	 */
	if (cherokee_fdpoll_is_full (FDPOLL(fdp))) {
		PRINT_ERROR_S("epoll_add: fdpoll is full !\n");
		return ret_error;
	}

	/* Add the new descriptor
	 */
	ev.data.u64 = 0;
	ev.data.fd = fd;
	switch (rw) {
	case FDPOLL_MODE_READ:
		ev.events = EPOLLIN | EPOLLERR | EPOLLHUP; 
		break;
	case FDPOLL_MODE_WRITE:
		ev.events = EPOLLOUT | EPOLLERR | EPOLLHUP;  
		break;
	default:
		ev.events = 0;
		SHOULDNT_HAPPEN;
		return ret_error;
	}

	if (epoll_ctl (fdp->ep_fd, EPOLL_CTL_ADD, fd, &ev) < 0) {
		PRINT_ERRNO (errno, "epoll_ctl(%d, EPOLL_CTL_ADD, %d): '${errno}'", fdp->ep_fd, fd);
		return ret_error;
	}

	FDPOLL(fdp)->npollfds++;
	return ret_ok;
}


static ret_t
_del (cherokee_fdpoll_epoll_t *fdp, int fd)
{
	struct epoll_event ev;

	ev.events   = 0;
	ev.data.u64 = 0;  /* <- I just wanna be sure there aren't */
	ev.data.fd  = fd; /* <- 4 bytes uninitialized */

	/* Check the fd limit
	 */
	if (cherokee_fdpoll_is_empty (FDPOLL(fdp))) {
		SHOULDNT_HAPPEN;
		return ret_error;
	}

	if (epoll_ctl(fdp->ep_fd, EPOLL_CTL_DEL, fd, &ev) < 0) {
		PRINT_ERRNO (errno, "epoll_ctl(%d, EPOLL_CTL_DEL, %d): '${errno}'", fdp->ep_fd, fd);
		return ret_error;
	}

	FDPOLL(fdp)->npollfds--;
	return ret_ok;
}


static int
_check (cherokee_fdpoll_epoll_t *fdp, int fd, int rw)
{
	int      fdidx;
	uint32_t events;

	/* Sanity check: is it a wrong fd?
	 */
	if (fd < 0 || fd >= FDPOLL(fdp)->system_nfiles)
		return -1;
	
	/* If fdidx is -1, it is not valid, ignore it.
	 */
	fdidx = fdp->epoll_fd2idx[fd];
	if (fdidx < 0 || fdidx >= fdp->ep_nrevents)
		return 0;

	/* Sanity check
	 */
	if (fdidx >= FDPOLL(fdp)->nfiles) {
		PRINT_ERROR ("ERROR: fdpoll: out of range, %d of %d, fd=%d\n",
			     fdidx, FDPOLL(fdp)->nfiles, fd);
		return -1;
	}

	/* Sanity check
	 */
	if (fdp->ep_events[fdidx].data.fd != fd)
		return 0;
	/*	return -1; */

	/* Check for errors
	 */
	events = fdp->ep_events[fdidx].events;

	switch (rw) {
	case FDPOLL_MODE_READ:
		return events & (EPOLLIN  | EPOLLERR | EPOLLHUP);
	case FDPOLL_MODE_WRITE:
		return events & (EPOLLOUT | EPOLLERR | EPOLLHUP);
	default:
		return -1;
	}
}


static ret_t
_reset (cherokee_fdpoll_epoll_t *fdp, int fd)
{
	/* Sanity check: is it a wrong fd?
	 */
	if (fd < 0 || fd >= FDPOLL(fdp)->system_nfiles)
		return ret_error;

	fdp->epoll_fd2idx[fd] = -1;

	return ret_ok;
}


static ret_t
_set_mode (cherokee_fdpoll_epoll_t *fdp, int fd, int rw)
{
	struct epoll_event ev;

	ev.data.u64 = 0;
	ev.data.fd  = fd;

	switch (rw) {
	case FDPOLL_MODE_READ:
		ev.events = EPOLLIN; 
		break;
	case FDPOLL_MODE_WRITE:
		ev.events = EPOLLOUT; 
		break;
	default:
		ev.events = 0;
		return ret_error;
	}

	if (epoll_ctl(fdp->ep_fd, EPOLL_CTL_MOD, fd, &ev) < 0) {
		PRINT_ERRNO (errno, "epoll_ctl(%d, EPOLL_CTL_MOD, %d): '${errno}'", fdp->ep_fd, fd);
		return ret_error;
	}
	return ret_ok;
}


static int
_watch (cherokee_fdpoll_epoll_t *fdp, int timeout_msecs)
{
	int i;

	fdp->ep_nrevents = epoll_wait (fdp->ep_fd, fdp->ep_events, FDPOLL(fdp)->nfiles, timeout_msecs);
	if (fdp->ep_nrevents < 1)
		return fdp->ep_nrevents;

	for (i = 0; i < fdp->ep_nrevents; ++i) {
		fdp->epoll_fd2idx[fdp->ep_events[i].data.fd] = i;
	}

	return fdp->ep_nrevents;
}


ret_t
fdpoll_epoll_get_fdlimits (cuint_t *system_fd_limit, cuint_t *fd_limit)
{
	*system_fd_limit = 0;
	*fd_limit = 0;

	return ret_ok;
}


ret_t 
fdpoll_epoll_new (cherokee_fdpoll_t **fdp, int sys_fd_limit, int fd_limit)
{
	int                re;
	cherokee_fdpoll_t *nfd;
	CHEROKEE_CNEW_STRUCT (1, n, fdpoll_epoll);

	nfd = FDPOLL(n);

	/* Init base class properties
	 */
	nfd->type          = cherokee_poll_epoll;
	nfd->nfiles        = fd_limit;
	nfd->system_nfiles = sys_fd_limit;
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

	/* Look for max fd limit	
	 */
	n->ep_fd = -1;
	n->ep_nrevents  = 0;
	n->ep_events    = (struct epoll_event *) calloc (nfd->nfiles, sizeof(struct epoll_event));
	n->epoll_fd2idx = (int *) calloc (nfd->system_nfiles, sizeof(int));

	/* If anyone fails free all and return ret_nomem 
	 */
	if (n->ep_events == NULL || n->epoll_fd2idx == NULL) {
		_free(n);
		return ret_nomem;
	}

	for (re = 0; re < nfd->system_nfiles; re++) {
		n->epoll_fd2idx[re] = -1;
	}

	n->ep_fd = epoll_create (nfd->nfiles);
	if (n->ep_fd < 0) {
		/* It may fail here if the glibc library supports epoll,
		 * but the kernel doesn't.
		 */
#if 0
		PRINT_ERRNO (errno, "epoll_create(%d): '${errno}'", nfd->nfiles+1);
#endif
		_free (n);
		return ret_error;
	}

	re = fcntl (n->ep_fd, F_SETFD, FD_CLOEXEC);
	if (re < 0) {
		PRINT_ERRNO (errno, "Could not set CloseExec to the epoll descriptor: fcntl: '${errno}'");
		_free (n);
		return ret_error;		
	}

	/* Return the object
	 */
	*fdp = nfd;
	return ret_ok;
}

