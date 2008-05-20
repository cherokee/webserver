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


/* Returns max. fd limits (system and fdpoll set).
 * NOTE: 0 means unlimited,
 *     > 0 max. acceptable limit for cherokee_fdpoll_new().
 */
ret_t
cherokee_fdpoll_get_fdlimits (cherokee_poll_type_t type, cuint_t *sys_fd_limit, cuint_t *fd_limit)
{
	/* Initialize values.
	 */
	*sys_fd_limit = 0;
	*fd_limit = 0;

	/* Call the proper method to get system and per fdpoll set limits.
	 */
	switch (type) {
	case cherokee_poll_epoll:
#if HAVE_EPOLL	
		return fdpoll_epoll_get_fdlimits (sys_fd_limit, fd_limit);
#else			
		return ret_no_sys;
#endif

	case cherokee_poll_kqueue:
#if HAVE_KQUEUE	
		return fdpoll_kqueue_get_fdlimits(sys_fd_limit, fd_limit);
#else			
		return ret_no_sys;
#endif	

	case cherokee_poll_port:
#if HAVE_PORT
		return fdpoll_port_get_fdlimits(sys_fd_limit, fd_limit);
#else			
		return ret_no_sys;
#endif	

	case cherokee_poll_poll:
#if HAVE_POLL		
		return fdpoll_poll_get_fdlimits (sys_fd_limit, fd_limit);
#else			
		return ret_no_sys;
#endif	

	case cherokee_poll_win32:
#if HAVE_WIN32_SELECT
		return fdpoll_win32_get_fdlimits (sys_fd_limit, fd_limit);
#else			
		return ret_no_sys;
#endif

	case cherokee_poll_select:
#if HAVE_SELECT	
		return fdpoll_select_get_fdlimits (sys_fd_limit, fd_limit);
#else			
		return ret_no_sys;
#endif	
	default:
		SHOULDNT_HAPPEN;
		return ret_error;
	}

	/* NOTREACHED */
}


ret_t
cherokee_fdpoll_new (cherokee_fdpoll_t **fdp, cherokee_poll_type_t type, int sys_fd_limit, int fd_limit)
{
	if ((sys_fd_limit < fd_limit) ||
	    (sys_fd_limit < FD_NUM_MIN_AVAILABLE))
		return ret_error;

	switch (type) {
	case cherokee_poll_epoll:
#if HAVE_EPOLL	
		return fdpoll_epoll_new (fdp, sys_fd_limit, fd_limit);
#else			
		return ret_no_sys;
#endif

	case cherokee_poll_kqueue:
#if HAVE_KQUEUE	
		return fdpoll_kqueue_new (fdp, sys_fd_limit, fd_limit);
#else			
		return ret_no_sys;
#endif	

	case cherokee_poll_port:
#if HAVE_PORT
		return fdpoll_port_new (fdp, sys_fd_limit, fd_limit);
#else			
		return ret_no_sys;
#endif	

	case cherokee_poll_poll:
#if HAVE_POLL		
		return fdpoll_poll_new (fdp, sys_fd_limit, fd_limit);
#else			
		return ret_no_sys;
#endif	

	case cherokee_poll_win32:
#if HAVE_WIN32_SELECT
		return fdpoll_win32_new (fdp, sys_fd_limit, fd_limit);
#else			
		return ret_no_sys;
#endif

	case cherokee_poll_select:
#if HAVE_SELECT	
		return fdpoll_select_new (fdp, sys_fd_limit, fd_limit);
#else			
		return ret_no_sys;
#endif	
	default:
		SHOULDNT_HAPPEN;
		return ret_error;
	}

	/* NOTREACHED */
}


ret_t
cherokee_fdpoll_best_new (cherokee_fdpoll_t **fdp, int sys_limit, int limit)
{
	ret_t ret;

	ret = cherokee_fdpoll_new (fdp, cherokee_poll_epoll, sys_limit, limit);
	if (ret == ret_ok) return ret_ok;

	ret = cherokee_fdpoll_new (fdp, cherokee_poll_poll, sys_limit, limit);
	if (ret == ret_ok) return ret_ok;

	ret = cherokee_fdpoll_new (fdp, cherokee_poll_kqueue, sys_limit, limit);
	if (ret == ret_ok) return ret_ok;

	ret = cherokee_fdpoll_new (fdp, cherokee_poll_port, sys_limit, limit);
	if (ret == ret_ok) return ret_ok;

	ret = cherokee_fdpoll_new (fdp, cherokee_poll_win32, sys_limit, limit);
	if (ret == ret_ok) return ret_ok;

	ret = cherokee_fdpoll_new (fdp, cherokee_poll_select, sys_limit, limit);
	if (ret == ret_ok) return ret_ok;

	SHOULDNT_HAPPEN;
	return ret_error;
}


ret_t 
cherokee_fdpoll_get_method (cherokee_fdpoll_t *fdp, cherokee_poll_type_t *type)
{
	*type = fdp->type;
	return ret_ok;
}


ret_t 
cherokee_fdpoll_get_method_str (cherokee_fdpoll_t *fdp, char **str)
{
	switch (fdp->type) {
	case cherokee_poll_epoll:
		*str = "epoll";
		break;
	case cherokee_poll_kqueue:
		*str = "kqueue";
		break;
	case cherokee_poll_port:
		*str = "ports";
		break;
	case cherokee_poll_poll:
		*str = "poll";
		break;
	case cherokee_poll_win32:
		*str = "win32";
		break;
	case cherokee_poll_select:
		*str = "select";
		break;
	default:
		SHOULDNT_HAPPEN;
		*str = "unknown";
		return ret_error;
	}

	return ret_ok;
}


/* Given a string poll type, returns the poll type.
 */
ret_t 
cherokee_fdpoll_str_to_method (char *str, cherokee_poll_type_t *poll_type)
{
	if (equal_str(str, "epoll")) {
		*poll_type = cherokee_poll_epoll;
		return ret_ok;
	}

	if (equal_str(str, "kqueue")) {
		*poll_type = cherokee_poll_kqueue;
		return ret_ok;
	}

	if (equal_str(str, "port")) {
		*poll_type = cherokee_poll_port;
		return ret_ok;
	}

	if (equal_str(str, "poll")) {
		*poll_type = cherokee_poll_poll;
		return ret_ok;
	}

	if (equal_str(str, "win32")) {
		*poll_type = cherokee_poll_win32;
		return ret_ok;
	}

	if (equal_str(str, "select")) {
		*poll_type = cherokee_poll_select;
		return ret_ok;
	}

	/* Unknown type.
	 */
	return ret_error;
}


ret_t
cherokee_fdpoll_is_empty (cherokee_fdpoll_t *fdp)
{
	return (fdp->npollfds <= 0);
}


ret_t
cherokee_fdpoll_is_full (cherokee_fdpoll_t *fdp)
{
	return (fdp->npollfds >= fdp->nfiles);
}


/* Virtual methods
 */

ret_t 
cherokee_fdpoll_free (cherokee_fdpoll_t *fdp)
{
	return fdp->free (fdp);
}


ret_t 
cherokee_fdpoll_add (cherokee_fdpoll_t *fdp, int fd, int rw)
{
	return fdp->add (fdp, fd, rw);
}


ret_t 
cherokee_fdpoll_del (cherokee_fdpoll_t *fdp, int fd)
{
	return fdp->del (fdp, fd);
}


ret_t 
cherokee_fdpoll_reset (cherokee_fdpoll_t *fdp, int fd)
{
	return fdp->reset (fdp, fd);
}


ret_t  
cherokee_fdpoll_set_mode (cherokee_fdpoll_t *fdp, int fd, int rw)
{
	return fdp->set_mode (fdp, fd, rw);
}


int   
cherokee_fdpoll_check (cherokee_fdpoll_t *fdp, int fd, int rw)
{
	return fdp->check (fdp, fd, rw);
}


int   
cherokee_fdpoll_watch (cherokee_fdpoll_t *fdp, int timeout_msecs)
{
	return fdp->watch (fdp, timeout_msecs);
}

