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

#if !defined (CHEROKEE_INSIDE_CHEROKEE_H) && !defined (CHEROKEE_COMPILATION)
# error "Only <cherokee/cherokee.h> can be included directly, this file may disappear or change contents."
#endif

#ifndef CHEROKEE_FDPOLL_H
#define CHEROKEE_FDPOLL_H

#include <cherokee/common.h>


CHEROKEE_BEGIN_DECLS

#define FDPOLL_MODE_NONE	(-1)	/* no mode set */
#define FDPOLL_MODE_READ	0	/* poll read  side */
#define FDPOLL_MODE_WRITE	1	/* poll write side */

typedef enum {
	cherokee_poll_epoll,
	cherokee_poll_kqueue,
	cherokee_poll_port,
	cherokee_poll_poll,
	cherokee_poll_select,
	cherokee_poll_UNSET
} cherokee_poll_type_t;

#define FDPOLL(x) ((cherokee_fdpoll_t *)(x))

typedef struct cherokee_fdpoll cherokee_fdpoll_t;

ret_t cherokee_fdpoll_get_fdlimits(cherokee_poll_type_t type, cuint_t *sys_fd_limit, cuint_t *fd_limit);

ret_t cherokee_fdpoll_new        (cherokee_fdpoll_t **fdp, cherokee_poll_type_t type, int sys_fd_limit, int fd_limit);
ret_t cherokee_fdpoll_best_new   (cherokee_fdpoll_t **fdp, int sys_limit, int limit);
ret_t cherokee_fdpoll_free       (cherokee_fdpoll_t  *fdp);

ret_t cherokee_fdpoll_get_method (cherokee_fdpoll_t *fdp, cherokee_poll_type_t *poll_type);
ret_t cherokee_fdpoll_get_method_str (cherokee_fdpoll_t *fdp, const char **str_method);
ret_t cherokee_fdpoll_str_to_method  (char *str_method, cherokee_poll_type_t *poll_method);

ret_t cherokee_fdpoll_add        (cherokee_fdpoll_t *fdp, int fd, int rw);
ret_t cherokee_fdpoll_del        (cherokee_fdpoll_t *fdp, int fd);
ret_t cherokee_fdpoll_reset      (cherokee_fdpoll_t *fdp, int fd);
ret_t cherokee_fdpoll_set_mode   (cherokee_fdpoll_t *fdp, int fd, int rw);
int   cherokee_fdpoll_check      (cherokee_fdpoll_t *fdp, int fd, int rw);
int   cherokee_fdpoll_watch      (cherokee_fdpoll_t *fdp, int timeout_msecs);
ret_t cherokee_fdpoll_is_full    (cherokee_fdpoll_t *fdp);
int   cherokee_fdpoll_is_empty   (cherokee_fdpoll_t *fdp);

CHEROKEE_END_DECLS

#endif /* CHEROKEE_FDPOLL_H */
