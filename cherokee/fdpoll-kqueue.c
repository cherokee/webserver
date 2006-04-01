/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/* Cherokee
 *
 * Authors:
 *      Alvaro Lopez Ortega <alvaro@alobbs.com>
 *      Rodrigo Fernandez-Vizarra <rfdzvizarra@yahoo.ie>
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

#include <stdio.h>
#include <sys/event.h>
#include <sys/time.h>
#include <unistd.h>
#include <errno.h>

#define KQUEUE_READ_EVENT  0x1
#define KQUEUE_WRITE_EVENT 0x2


/***********************************************************************/
/* kqueue, kevent: kernel event notification mechanism                 */
/*                                                                     */
/* #include <sys/event.h>                                              */
/* #include <sys/time.h>                                               */
/*                                                                     */
/* int                                                                 */
/* kqueue(void);                                                       */
/*                                                                     */
/* int                                                                 */
/*  kevent(int kq, const struct kevent *changelist, size_t nchanges,   */
/*         struct kevent *eventlist, size_t nevents,                   */
/*         const struct timespec *timeout);                            */
/*                                                                     */
/* EV_SET(&kev, ident, filter, flags, fflags, data, udata);            */
/*                                                                     */
/***********************************************************************/


typedef struct {
	struct cherokee_fdpoll poll;

	int                    kqueue;
	struct kevent          *changelist;
	int                    *fdevents;
	int                    *fdinterest;
	size_t                 nchanges;
} cherokee_fdpoll_kqueue_t;



static ret_t 
_free (cherokee_fdpoll_kqueue_t *fdp)
{
	close( fdp->kqueue );

	free( fdp->changelist );
	free( fdp->fdevents );
	free( fdp->fdinterest );
	
	free( fdp );
        return ret_ok;
}


static ret_t
_add_change(cherokee_fdpoll_kqueue_t *fdp, int fd, int rw, int change )
{
	int index;
	struct kevent *event;

	index = fdp->nchanges;
	if (index >= FDPOLL(fdp)->nfiles) {
		return ret_error;
	}

	event = &fdp->changelist[index];
	event->ident = fd;
	switch (rw) {
	case 0:
		event->filter = EVFILT_READ;
		break;
	case 1:
		event->filter = EVFILT_WRITE;
		break;
	default:
		SHOULDNT_HAPPEN;
        }
	event->flags = change;
	event->fflags = 0;

	fdp->fdinterest[fd]=rw;
	fdp->nchanges++;
	return ret_ok;
}

static ret_t
_add (cherokee_fdpoll_kqueue_t *fdp, int fd, int rw)
{
	int re;
	
	re = _add_change( fdp, fd, rw, EV_ADD);
	if ( re == ret_ok) {
		FDPOLL(fdp)->npollfds++;
	}
	
	return re;
}

static ret_t
_del (cherokee_fdpoll_kqueue_t *fdp, int fd)
{
       int re;
	       
       re = _add_change( fdp, fd, fdp->fdinterest[fd], EV_DELETE);
       if ( re == ret_ok) {
	       FDPOLL(fdp)->npollfds--;
       }
       
       return re;
}

static int
_watch (cherokee_fdpoll_kqueue_t *fdp, int timeout_msecs)
{
	struct timespec  timeout;
	int              i, re, fd;
	int              n_events;


	timeout.tv_sec  = timeout_msecs/1000L;
	timeout.tv_nsec = ( timeout_msecs % 1000L ) * 1000000L;

	/* Get the events of the file descriptors with
	 * activity
	 */
	n_events = kevent(fdp->kqueue, 
			  fdp->changelist, 
			  fdp->nchanges,
			  fdp->changelist,
			  FDPOLL(fdp)->nfiles,
			  &timeout);
	fdp->nchanges=0;
	if ( n_events < 0 ) {
		PRINT_ERROR ("ERROR: kevent: %s\n", strerror(errno));
		return 0;
	} else if ( n_events > 0 ) {
		memset(fdp->fdevents, 0, FDPOLL(fdp)->system_nfiles*sizeof(int));
		for ( i = 0; i < n_events; ++i ) {
			if ( fdp->changelist[i].filter == EVFILT_READ ) {
				fdp->fdevents[fdp->changelist[i].ident] = KQUEUE_READ_EVENT;
			} else if (fdp->changelist[i].filter == EVFILT_WRITE) {
				fdp->fdevents[fdp->changelist[i].ident] = KQUEUE_WRITE_EVENT;
			} else {
				SHOULDNT_HAPPEN;
			}
		}
	}
	
	return n_events;
}


static int
_check (cherokee_fdpoll_kqueue_t *fdp, int fd, int rw)
{
	uint32_t events;
	
	/* Sanity check: is it a wrong fd?
	 */
	if ( fd < 0 ) return -1;
	
	events = fdp->fdevents[fd];
	
	switch (rw) {
	case 0:
		events &= KQUEUE_READ_EVENT;
		break;
	case 1:
		events &= KQUEUE_WRITE_EVENT;
		break;
	default:
		SHOULDNT_HAPPEN;
	}
	
	return events;
}


static ret_t
_reset (cherokee_fdpoll_kqueue_t *fdp, int fd)
{
	return ret_ok;
}


static void
_set_mode (cherokee_fdpoll_kqueue_t *fdp, int fd, int rw)
{
	/* If transitioning from r -> w or from w -> r
	 * disable any active event on the fd as we are
	 * no longer interested on it.
	 */
	if ( rw && (fdp->fdinterest[fd] == 0) ) {
		_add_change(fdp, fd, 0, EV_DELETE);
	} else if ( (rw==0 ) && (fdp->fdinterest[fd] == 1) ) {
		_add_change(fdp, fd, 1, EV_DELETE);
	}

	_add_change( fdp, fd, rw, EV_ADD );
}

ret_t 
fdpoll_kqueue_new (cherokee_fdpoll_t **fdp, int sys_limit, int limit)
{
	cherokee_fdpoll_t *nfd;
	CHEROKEE_NEW_STRUCT (n, fdpoll_kqueue);

	nfd = FDPOLL(n);

	/* Init base class properties
	 */
	nfd->type          = cherokee_poll_kqueue;
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

	/* Init kqueue specific variables
	 */
	n->nchanges        = 0;
	n->changelist      = ( struct kevent *)malloc(sizeof(struct kevent)*2*
						      nfd->nfiles);
	n->fdevents        = (int *)malloc(sizeof(int) * nfd->system_nfiles);
	n->fdinterest      = (int *)malloc(sizeof(int) * nfd->system_nfiles);

	if ( (!n->fdevents) || (!n->changelist) || (!n->fdinterest) ) {
		_free( n );
		return ret_nomem;
	}

	memset(n->fdevents, 0, sizeof(int)*nfd->system_nfiles);

	if ( (n->kqueue = kqueue()) == -1 ) {
		_free( n );
		return ret_error;
	}

	/* Return the object
	 */
	*fdp = nfd;
	return ret_ok;
}
