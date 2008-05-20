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

#include <poll.h>
#include <errno.h>

#define POLL_ERROR  (POLLHUP | POLLERR | POLLNVAL)
#define POLL_READ   (POLLIN  | POLL_ERROR)
#define POLL_WRITE  (POLLOUT | POLL_ERROR)


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


static ret_t
_free (cherokee_fdpoll_poll_t *fdp)
{
	if (fdp == NULL)
		return ret_ok;

	free (fdp->pollfds);
	free (fdp->fdidx);      

	/* Caller has to set this pointer to NULL.
	 */
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
		PRINT_ERROR_S("poll_add: fdpoll is full !\n");
		return ret_error;
	}

	fdp->pollfds[nfd->npollfds].fd      = fd;
	fdp->pollfds[nfd->npollfds].revents = 0; 

	switch (rw) {
	case FDPOLL_MODE_READ:
		fdp->pollfds[nfd->npollfds].events = POLLIN; 
		break;
	case FDPOLL_MODE_WRITE:
		fdp->pollfds[nfd->npollfds].events = POLLOUT;
		break;
	default:
		SHOULDNT_HAPPEN;
		return ret_error;
	}

	fdp->fdidx[fd] = nfd->npollfds;
	nfd->npollfds++;

	return ret_ok;
}


static ret_t
_set_mode (cherokee_fdpoll_poll_t *fdp, int fd, int rw)
{
	fdp->pollfds[fdp->fdidx[fd]].events = (rw == FDPOLL_MODE_WRITE ? POLLOUT : POLLIN);
	return ret_ok;
}


static ret_t
_del (cherokee_fdpoll_poll_t *fdp, int fd)
{
	int                idx = fdp->fdidx[fd];
	cherokee_fdpoll_t *nfd = FDPOLL(fdp);

	if (idx < 0 || idx >= nfd->nfiles) {
		PRINT_ERROR ("Error dropping socket '%d' from fdpoll\n", idx);
		return ret_error;
	}

	/* Check the fd counter.
	 */
	if (cherokee_fdpoll_is_empty (nfd)) {
		SHOULDNT_HAPPEN;
		return ret_error;
	}

	/* Decrease the total number of descriptors
	 */
	nfd->npollfds--;

	/* Move the last one to the empty space
	 */
	if (idx != nfd->npollfds) {
		fdp->pollfds[idx] = fdp->pollfds[nfd->npollfds];
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
_check (cherokee_fdpoll_poll_t *fdp, int fd, int rw)
{
	int revents;
	int idx = fdp->fdidx[fd];

	if (idx < 0 || idx >= FDPOLL(fdp)->nfiles)
		return -1;

	revents = fdp->pollfds[idx].revents;

	switch (rw) {
		case FDPOLL_MODE_READ:
			return revents & POLL_READ;
		case FDPOLL_MODE_WRITE:
			return revents & POLL_WRITE;
		default: 
			SHOULDNT_HAPPEN;
			return -1;
	}
}


static ret_t
_reset (cherokee_fdpoll_poll_t *fdp, int fd)
{
	UNUSED(fdp);
	UNUSED(fd);

	/* fdp->fdidx[fd] = -1;
	 */
	return ret_ok;
}


static int
_watch (cherokee_fdpoll_poll_t *fdp, int timeout_msecs)
{
	if (unlikely (FDPOLL(fdp)->npollfds < 0)) {
		SHOULDNT_HAPPEN;
	}

	return poll (fdp->pollfds, FDPOLL(fdp)->npollfds, timeout_msecs);
}


ret_t
fdpoll_poll_get_fdlimits (cuint_t *system_fd_limit, cuint_t *fd_limit)
{
	*system_fd_limit = 0;
	*fd_limit = 0;

	return ret_ok;
}


ret_t
fdpoll_poll_new (cherokee_fdpoll_t **fdp, int system_fd_limit, int fd_limit)
{
	int                i;
	cherokee_fdpoll_t *nfd;
	CHEROKEE_CNEW_STRUCT (1, n, fdpoll_poll);

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
	n->pollfds = (struct pollfd *) calloc (nfd->nfiles, sizeof(struct pollfd));
	if (n->pollfds == NULL) {
		_free(n);
		return ret_nomem;
	}
	for (i = 0; i < nfd->nfiles; i++) {
		n->pollfds[i].fd      = -1;
		n->pollfds[i].events  =  0;
		n->pollfds[i].revents =  0;
	}

	/* Get memory: reverse fd index
	 */
	n->fdidx = (int*) calloc (nfd->system_nfiles, sizeof(int));
	if (n->fdidx == NULL) {
		_free(n);
		return ret_nomem;
	}
	for (i = 0; i < nfd->system_nfiles; i++) {
		n->fdidx[i] = -1;
	}

	/* Return the object
	 */
	*fdp = nfd;
	return ret_ok;
}

