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

#include <errno.h>

#ifdef HAVE_SYS_SELECT_H
# include <sys/select.h>
#endif

#ifdef HAVE_SYS_TIME_H
# include <sys/time.h>
#endif

#ifndef INFTIM
# define INFTIM -1
#endif


/***********************************************************************/
/* select()                                                            */
/*                                                                     */
/* #include <sys/select.h>                                             */
/* int select(int n, fd_set *readfds, fd_set *writefds,                */
/*            fd_set *exceptfds, struct timeval *timeout);             */
/*                                                                     */
/***********************************************************************/

typedef struct {
	struct cherokee_fdpoll poll;

	int    *fd_rw;
	fd_set  master_rfdset;
	fd_set  master_wfdset;
	fd_set  working_rfdset;
	fd_set  working_wfdset;
	int    *select_fds;
	int    *select_fdidx;
	int    *select_rfdidx;
	int     maxfd;
	int     maxfd_recompute;
} cherokee_fdpoll_select_t;


static ret_t 
_free (cherokee_fdpoll_select_t *fdp)
{
	if (fdp == NULL)
		return ret_ok;

	/* ANSI C required, so that free() can handle NULL pointers
	 */
	free (fdp->select_fds);
	free (fdp->select_fdidx);
	free (fdp->select_rfdidx);
	free (fdp->fd_rw);

	/* Caller has to set this pointer to NULL.
	 */
	free (fdp);

	return ret_ok;
}


static ret_t 
_add (cherokee_fdpoll_select_t *fdp, int fd, int rw)
{
	cherokee_fdpoll_t *nfd = FDPOLL(fdp);

	/* Check the fd limit
	 */
	if (cherokee_fdpoll_is_full(FDPOLL(fdp))) {
		PRINT_ERROR_S("select_add: fdpoll is full !\n");
		return ret_error;
	}

	fdp->select_fds[nfd->npollfds] = fd;
	switch (rw) {
	case FDPOLL_MODE_READ:
		FD_SET (fd, &fdp->master_rfdset); 
		break;
	case FDPOLL_MODE_WRITE:
		FD_SET (fd, &fdp->master_wfdset);
		break;
	default: 
		break;
	}

	if (fd > fdp->maxfd) {
		fdp->maxfd = fd;
	}

	fdp->select_fdidx[fd] = nfd->npollfds;
	fdp->fd_rw[fd]        = rw;
	nfd->npollfds++;

	return ret_ok;
}


static ret_t 
_del (cherokee_fdpoll_select_t *fdp, int fd)
{
	cherokee_fdpoll_t *nfd = FDPOLL(fdp);
	int                idx = fdp->select_fdidx[fd];

	if ((idx < 0) || (idx >= FDPOLL(fdp)->nfiles)) {
		PRINT_ERROR ("Bad idx (%d) in select_del_fd!\n", idx);
		return ret_error;
	}

	nfd->npollfds--;
	fdp->select_fds[idx]                    = fdp->select_fds[nfd->npollfds];
	fdp->select_fdidx[fdp->select_fds[idx]] = idx;
	fdp->select_fds[nfd->npollfds]          = -1;
	fdp->select_fdidx[fd]                   = -1;
	fdp->fd_rw[fd]                          = -1;

	FD_CLR (fd, &fdp->master_rfdset);
	FD_CLR (fd, &fdp->master_wfdset);

	if (fd >= fdp->maxfd) {
		fdp->maxfd_recompute = 1;
	}

	return ret_ok;
}


static ret_t
_set_mode (cherokee_fdpoll_select_t *fdp, int fd, int rw)
{
	ret_t ret;

	ret = _del (fdp, fd);
	if (unlikely(ret < ret_ok))
		return ret;

	return _add (fdp, fd, rw);
}


static int   
_check (cherokee_fdpoll_select_t *fdp, int fd, int rw)
{
	UNUSED(rw);

	switch (fdp->fd_rw[fd]) {
		case FDPOLL_MODE_READ:
			return FD_ISSET (fd, &fdp->working_rfdset);
		case FDPOLL_MODE_WRITE:
			return FD_ISSET (fd, &fdp->working_wfdset);
	}

	return 0;
}


static int
select_get_maxfd (cherokee_fdpoll_select_t *fdp) 
{
	if (fdp->maxfd_recompute) {
		int i;

		fdp->maxfd = -1;
		for (i = 0; i < FDPOLL(fdp)->npollfds; ++i) {
			if (fdp->select_fds[i] > fdp->maxfd ) {
				fdp->maxfd = fdp->select_fds[i];
			}
		}

		fdp->maxfd_recompute = 0;
	}

	return fdp->maxfd;
}


static int   
_watch (cherokee_fdpoll_select_t *fdp, int timeout_msecs)
{
	int mfd;
	int r, idx, ridx;

	fdp->working_rfdset = fdp->master_rfdset;
	fdp->working_wfdset = fdp->master_wfdset;
	mfd = select_get_maxfd(fdp);
	if (timeout_msecs == INFTIM) {
		r = select (mfd + 1, &fdp->working_rfdset, &fdp->working_wfdset, NULL, NULL);
	} else {
		struct timeval timeout;
		timeout.tv_sec = timeout_msecs / 1000L;
		timeout.tv_usec = ( timeout_msecs % 1000L ) * 1000L;
		r = select (mfd + 1, &fdp->working_rfdset, &fdp->working_wfdset, NULL, &timeout);
	}

	if (r <= 0) {
		return r;
	}

	ridx = 0;
	for (idx = 0; idx < FDPOLL(fdp)->npollfds; ++idx) {
		if (_check (fdp, fdp->select_fds[idx], 0)) {
			fdp->select_rfdidx[ridx++] = fdp->select_fds[idx];
			if (ridx == r)
				break;
		}
	}

	return ridx;        /* should be equal to r */
}


static ret_t 
_reset (cherokee_fdpoll_select_t *fdp, int fd)
{
	UNUSED(fdp);
	UNUSED(fd);

	return ret_ok;
}


ret_t
fdpoll_select_get_fdlimits (cuint_t *system_fd_limit, cuint_t *fd_limit)
{
	*system_fd_limit = FD_SETSIZE;
	*fd_limit = 0;

	return ret_ok;
}


ret_t 
fdpoll_select_new (cherokee_fdpoll_t **fdp, int system_fd_limit, int fd_limit)
{
	int                i;
	cherokee_fdpoll_t *nfd;
	CHEROKEE_CNEW_STRUCT (1, n, fdpoll_select);

	/* Verify that the max. number of system fds
	 * is below the max. acceptable limit.
	 */
	if (system_fd_limit > FD_SETSIZE) {
		_free(n);
		return ret_error;
	}

	nfd = FDPOLL(n);
        
	/* Init base class properties
	 */
	nfd->type          = cherokee_poll_select;
	nfd->nfiles        = fd_limit;
	nfd->system_nfiles = system_fd_limit;
	nfd->npollfds      =  0;

	/* Init base class virtual methods
	 */
	nfd->free          = (fdpoll_func_free_t) _free;
	nfd->add           = (fdpoll_func_add_t) _add;
	nfd->del           = (fdpoll_func_del_t) _del;
	nfd->reset         = (fdpoll_func_reset_t) _reset;
	nfd->set_mode      = (fdpoll_func_set_mode_t) _set_mode;
	nfd->check         = (fdpoll_func_check_t) _check;
	nfd->watch         = (fdpoll_func_watch_t) _watch;	

	/* Init properties
	 */
	FD_ZERO (&n->master_rfdset);
	FD_ZERO (&n->master_wfdset);

	n->select_fds      = (int*) calloc(nfd->nfiles, sizeof(int));
	n->select_rfdidx   = (int*) calloc(nfd->nfiles, sizeof(int));
	n->select_fdidx    = (int*) calloc(nfd->system_nfiles, sizeof(int));
	n->fd_rw           = (int*) calloc(nfd->system_nfiles, sizeof(int));
	n->maxfd           = -1;
	n->maxfd_recompute =  0;

	if (n->select_fds == NULL ||
	    n->select_rfdidx == NULL ||
	    n->select_fdidx  == NULL ||
	    n->fd_rw         == NULL) {
		_free (n);
		return ret_nomem;
	}

	for (i = 0; i < nfd->nfiles; ++i) {
		n->select_fds[i] = -1;
	}

	for (i = 0; i < nfd->system_nfiles; ++i) {
		n->select_fdidx[i] = -1;
		n->fd_rw[i]        = -1;
	}

	*fdp = nfd;
	return ret_ok;
}
