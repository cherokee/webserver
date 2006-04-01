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

#include <poll.h>
#include <errno.h>

#define POLL_READ   (POLLIN  | POLL_ERROR)
#define POLL_WRITE  (POLLOUT | POLL_ERROR)
#define POLL_ERROR  (POLLHUP | POLLERR | POLLNVAL)


/***********************************************************************/
/* poll()                                                              */
/*                                                                     */
/* #include <sys/poll.h>                                               */
/* int poll(struct pollfd *ufds, unsigned int nfds, int timeout);      */
/*                                                                     */
/***********************************************************************/


typedef struct {
	struct cherokee_fdpoll poll;

        struct pollfd *pollfds;
        int           *fdidx;
} cherokee_fdpoll_poll_t;


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
_free (cherokee_fdpoll_poll_t *fdp)
{
        free (fdp->pollfds);
        free (fdp->fdidx);      
        free (fdp);
        
        return ret_ok;
}


static ret_t
_add (cherokee_fdpoll_poll_t *fdp, int fd, int rw)
{
	cherokee_fdpoll_t *nfd = FDPOLL(fdp);

        /* Check the fd limit
         */
        if (cherokee_fdpoll_is_full(nfd)) {
                return ret_error;
        }

        fdp->pollfds[nfd->npollfds].fd      = fd;
	fdp->pollfds[nfd->npollfds].revents = 0; 

        switch (rw) {
        case 0:  
                fdp->pollfds[nfd->npollfds].events = POLLIN; 
                break;
        case 1: 
                fdp->pollfds[nfd->npollfds].events = POLLOUT;
                break;
	default:
		SHOULDNT_HAPPEN;
        }
        fdp->fdidx[fd] = nfd->npollfds;
        nfd->npollfds++;

        return ret_ok;
}


static void 
_set_mode (cherokee_fdpoll_poll_t *fdp, int fd, int rw)
{
        fdp->pollfds[fdp->fdidx[fd]].events = rw ? POLLOUT : POLLIN;
}

static ret_t
_del (cherokee_fdpoll_poll_t *fdp, int fd)
{
        int                idx = fdp->fdidx[fd];
	cherokee_fdpoll_t *nfd = FDPOLL(fdp);

        if (idx < 0 || idx >= nfd->nfiles) {
                PRINT_ERROR ("Error droping socket '%d' from fdpoll\n", idx);
                return ret_error;
        }

	/* Decrease the total number of descriptors
	 */
        nfd->npollfds--;

	/* Move the last one to the empty space
	 */
	if (nfd->npollfds > 0) {
		memcpy (&fdp->pollfds[idx], &fdp->pollfds[nfd->npollfds], sizeof(struct pollfd));
		fdp->fdidx[fdp->pollfds[idx].fd] = idx;
	}

	/* Clean the new empty entry
	 */
        fdp->fdidx[fd]                      = -1;
        fdp->pollfds[nfd->npollfds].fd      = -1;
	fdp->pollfds[nfd->npollfds].events  = 0; 
	fdp->pollfds[nfd->npollfds].revents = 0; 

        return ret_ok;
}


static int
_watch (cherokee_fdpoll_poll_t *fdp, int timeout_msecs)
{
	if (FDPOLL(fdp)->npollfds < 0) {
		SHOULDNT_HAPPEN;
	}
		
        return poll (fdp->pollfds, FDPOLL(fdp)->npollfds, timeout_msecs);
}


static int
_check (cherokee_fdpoll_poll_t *fdp, int fd, int rw)
{
	short revents = fdp->pollfds[fdp->fdidx[fd]].revents;

	if (revents & POLLNVAL) {
		_del (fdp, fd);
	}

        switch (rw) {
        case 0: 
                return revents & POLL_READ;
        case 1: 
                return revents & POLL_WRITE;
        default: 
                SHOULDNT_HAPPEN;
                return -1;
        }
}


static ret_t
_reset (cherokee_fdpoll_poll_t *fdp, int fd)
{
	return ret_ok;
}


ret_t
fdpoll_poll_new (cherokee_fdpoll_t **fdp, int system_fd_limit, int fd_limit)
{
        int                i;
	cherokee_fdpoll_t *nfd;
        CHEROKEE_NEW_STRUCT (n, fdpoll_poll);
        
	nfd = FDPOLL(n);

	/* Init base class properties
	 */
	nfd->type          = cherokee_poll_poll;
        nfd->nfiles        = fd_limit;
        nfd->system_nfiles = system_fd_limit;
        nfd->npollfds      = 0;
        
	/* Init base class virtual methods
	 */
	nfd->free      = (fdpoll_func_free_t) _free;
	nfd->add       = (fdpoll_func_add_t) _add;
	nfd->del       = (fdpoll_func_del_t) _del;
	nfd->reset     = (fdpoll_func_reset_t) _reset;
	nfd->set_mode  = (fdpoll_func_set_mode_t) _set_mode;
	nfd->check     = (fdpoll_func_check_t) _check;
	nfd->watch     = (fdpoll_func_watch_t) _watch;	

        /* Get memory: pollfds structs
         */
        n->pollfds = (struct pollfd *) malloc (sizeof(struct pollfd) * nfd->nfiles);
        return_if_fail (n->pollfds, ret_nomem);
        
        for (i=0; i < nfd->nfiles; i++) {
                n->pollfds[i].fd      = -1;
		n->pollfds[i].events  =  0;
		n->pollfds[i].revents =  0;
        }

        /* Get memory: reverse fd index
         */
        n->fdidx = (int*) malloc (sizeof(int) * nfd->system_nfiles);
        return_if_fail (n->fdidx, ret_nomem);
        
        for (i=0; i < nfd->system_nfiles; i++) {
                n->fdidx[i] = -1;
        }

        /* Return the object
         */
        *fdp = nfd;
        return ret_ok;
}
