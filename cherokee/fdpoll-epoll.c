/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/* Cherokee
 *
 * Authors:
 *      Alvaro Lopez Ortega <alvaro@alobbs.com>
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
#include "fdpoll-protected.h"

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
        int                    ep_readyfds;
        int                   *epoll_rs2idx;
        int                   *epoll_idx2rs;
} cherokee_fdpoll_epoll_t;


/*
** Copyright (c) 1999,2000 by Jef Poskanzer <jef@acme.com>
** All rights reserved.
**
** Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions
** are met:
** 1. Redistributions of source code must retain the above copyright
**    notice, this list of conditions and the following disclaimer.
** 2. Redistributions in binary form must reproduce the above copyright
**    notice, this list of conditions and the following disclaimer in the
**    documentation and/or other materials provided with the distribution.
*/

static ret_t 
_free (cherokee_fdpoll_epoll_t *fdp)
{
	if (fdp->ep_fd > 0)
		close (fdp->ep_fd);

        free (fdp->ep_events);
        free (fdp->epoll_rs2idx);
        free (fdp->epoll_idx2rs);
        
        free (fdp);        
        return ret_ok;
}


static ret_t
_add (cherokee_fdpoll_epoll_t *fdp, int fd, int rw)
{
	struct epoll_event ev;

	ev.data.u64 = 0;
        
        /* Check the fd limit
         */
        if (cherokee_fdpoll_is_full (FDPOLL(fdp))) {
                return ret_error;
        }

        /* Add the new descriptor
         */
	ev.data.fd = fd;
        switch (rw) {
        case 0: 
                ev.events = EPOLLIN | EPOLLERR | EPOLLHUP; 
                break;
        case 1: 
                ev.events = EPOLLOUT | EPOLLERR | EPOLLHUP;  
		break;
        default:
                ev.events = 0;
		SHOULDNT_HAPPEN;
                break;
        }

        if (epoll_ctl (fdp->ep_fd, EPOLL_CTL_ADD, fd, &ev) < 0) 
        {
                PRINT_ERROR ("ERROR: epoll_ctl(%d, EPOLL_CTL_ADD, %d): %s\n", 
                             fdp->ep_fd, fd, strerror(errno));
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
	ev.data.fd  = fd; /* <- 4 bytes unitialized */

	if (epoll_ctl(fdp->ep_fd, EPOLL_CTL_DEL, fd, &ev) < 0)
	{
		PRINT_ERROR ("ERROR: epoll_ctl(%d, EPOLL_CTL_DEL, %d): %s\n", 
			     fdp->ep_fd, fd, strerror(errno));
		return ret_error;
	}

	FDPOLL(fdp)->npollfds--;
	return ret_ok;
}


static int
_watch (cherokee_fdpoll_epoll_t *fdp, int timeout_msecs)
{
	int i, ridx;

	fdp->ep_readyfds = epoll_wait (fdp->ep_fd, fdp->ep_events, FDPOLL(fdp)->nfiles, timeout_msecs);

	for (i = 0, ridx = 0; i < fdp->ep_readyfds; ++i) {
		fdp->epoll_idx2rs[fdp->ep_events[i].data.fd] = i;
		fdp->epoll_rs2idx[ridx] = i;
		ridx++;
	}

	return ridx;
}


static int
_check (cherokee_fdpoll_epoll_t *fdp, int fd, int rw)
{
	int      fdidx;
	uint32_t events;

	/* Sanity check: is it a wrong fd?
	 */
	if (fd < 0) return -1;

	/* If fdidx is -1 is because the _reset() function
	 */
	fdidx = fdp->epoll_idx2rs[fd];
	if (fdidx == -1) return 0;

	/* Sanity check
	 */
	if (fdp->ep_events[fdidx].data.fd != fd) {
		return 0;
	}

	/* Sanity check
	 */
	if (fdidx >= (FDPOLL(fdp)->nfiles - 1)) {
		PRINT_ERROR ("ERROR: fdpoll: out of range, %d of %d, fd=%d\n",
			     fdidx, FDPOLL(fdp)->nfiles, fd);
		return -1;
	}

	/* Check for errors
	 */
	events = fdp->ep_events[fdidx].events;

	switch (rw) {
	case 0: 
		return events & (EPOLLIN | EPOLLERR | EPOLLHUP);
	case 1: 
		return events & (EPOLLOUT | EPOLLERR | EPOLLHUP);
	}

	return 0;
}


static ret_t
_reset (cherokee_fdpoll_epoll_t *fdp, int fd)
{
	fdp->epoll_idx2rs[fd] = -1;

	return ret_ok;
}

static void
_set_mode (cherokee_fdpoll_epoll_t *fdp, int fd, int rw)
{
	struct epoll_event ev;

	ev.data.u64 = 0;
	ev.data.fd  = fd;

	switch (rw) {
	case 0: 
		ev.events = EPOLLIN; 
		break;
	case 1: 
		ev.events = EPOLLOUT; 
		break;
	default:
		ev.events = 0;
		break;
	}

	if (epoll_ctl(fdp->ep_fd, EPOLL_CTL_MOD, fd, &ev) < 0) 
	{
		PRINT_ERROR ("ERROR: epoll_ctl (%d, EPOLL_CTL_MOD, %d): %s\n",
			     fdp->ep_fd, fd, strerror(errno));
	}
}


ret_t 
fdpoll_epoll_new (cherokee_fdpoll_t **fdp, int sys_limit, int limit)
{
	int                re;
	cherokee_fdpoll_t *nfd;
	CHEROKEE_NEW_STRUCT (n, fdpoll_epoll);

	nfd = FDPOLL(n);

	/* Init base class properties
	 */
	nfd->type          = cherokee_poll_epoll;
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

	/* Look for max fd limit	
	 */
	n->ep_readyfds  = 0;
	n->ep_events    = (struct epoll_event *) malloc (sizeof(struct epoll_event) * nfd->system_nfiles);
	n->epoll_rs2idx = (int *) malloc (sizeof(int) * nfd->system_nfiles);
	n->epoll_idx2rs = (int *) malloc (sizeof(int) * nfd->system_nfiles);

	/* If anyone fails free all and return ret_nomem 
	 */
	if ((!n->ep_events ) || (!n->epoll_rs2idx) || (!n->epoll_idx2rs)) {
		_free (n);
		return ret_nomem;
	}

	memset (n->epoll_rs2idx, -1, nfd->system_nfiles);
	memset (n->epoll_idx2rs, -1, nfd->system_nfiles);

	n->ep_fd = epoll_create (nfd->nfiles + 1);
	if (n->ep_fd < 0) {
		/* It might fail here if the glibc library supports epoll, but the kernel doesn't.
		 */
#if 0
		PRINT_ERROR ("ERROR: Couldn't get epoll descriptor: epoll_create(%d): %s\n", 
			     nfd->nfiles+1, strerror(errno));
#endif
		_free (n);
		return ret_error;
	}
	
	re = fcntl (n->ep_fd, F_SETFD, FD_CLOEXEC);
	if (re < 0) {
		PRINT_ERROR ("ERROR: Couldn't set CloseExec to the epoll descriptor: fcntl: %s\n", 
			     strerror(errno));
		_free (n);
		return ret_error;		
	}

	/* Return the object
	 */
	*fdp = nfd;
	return ret_ok;
}



