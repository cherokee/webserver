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

#ifndef CHEROKEE_FDPOLL_PROTECTED_H
#define CHEROKEE_FDPOLL_PROTECTED_H

#include "module.h"
#include "fdpoll.h"

typedef ret_t (* fdpoll_func_free_t)     (void  *fdpoll);
typedef ret_t (* fdpoll_func_add_t)      (void  *fdpoll, int fd, int rw);
typedef ret_t (* fdpoll_func_del_t)      (void  *fdpoll, int fd);
typedef ret_t (* fdpoll_func_reset_t)    (void  *fdpoll, int fd);
typedef ret_t (* fdpoll_func_set_mode_t) (void  *fdpoll, int fd, int rw);
typedef int   (* fdpoll_func_check_t)    (void  *fdpoll, int fd, int rw);
typedef int   (* fdpoll_func_watch_t)    (void  *fdpoll, int timeout_msecs);
typedef ret_t (* fdpoll_func_is_full_t)  (void  *fdpoll);

ret_t fdpoll_epoll_get_fdlimits  (cuint_t *sys_fd_limit, cuint_t *fd_limit);
ret_t fdpoll_kqueue_get_fdlimits (cuint_t *sys_fd_limit, cuint_t *fd_limit);
ret_t fdpoll_port_get_fdlimits   (cuint_t *sys_fd_limit, cuint_t *fd_limit);
ret_t fdpoll_poll_get_fdlimits   (cuint_t *sys_fd_limit, cuint_t *fd_limit);
ret_t fdpoll_select_get_fdlimits (cuint_t *sys_fd_limit, cuint_t *fd_limit);

ret_t fdpoll_epoll_new  (cherokee_fdpoll_t **fdp, int sys_fd_limit, int fd_limit);
ret_t fdpoll_kqueue_new (cherokee_fdpoll_t **fdp, int sys_fd_limit, int fd_limit);
ret_t fdpoll_port_new   (cherokee_fdpoll_t **fdp, int sys_fd_limit, int fd_limit);
ret_t fdpoll_poll_new   (cherokee_fdpoll_t **fdp, int sys_fd_limit, int fd_limit);
ret_t fdpoll_select_new (cherokee_fdpoll_t **fdp, int sys_fd_limit, int fd_limit);


struct cherokee_fdpoll {
	cherokee_poll_type_t type;

	/* Properties
	 */
	int nfiles;            /* Max. fds in this FD poll */
	int system_nfiles;     /* Max. fds in the system   */
	int npollfds;          /* Currently, how many FDs  */

	/* Virtual methods
	 */
	fdpoll_func_free_t       free;
	fdpoll_func_add_t        add;
	fdpoll_func_del_t        del;
	fdpoll_func_reset_t      reset;
	fdpoll_func_set_mode_t   set_mode;
	fdpoll_func_check_t      check;
	fdpoll_func_watch_t      watch;
};

#endif /* CHEROKEE_FDPOLL_PROTECTED_H */
