/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/* Cherokee
 *
 * Authors:
 *      Daniel Silverstone <dsilvers@digital-scurf.org>
 *
 * Copyright (C) 2014 Alvaro Lopez Ortega
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

#ifndef CHEROKEE_SERVICES_H
#define CHEROKEE_SERVICES_H

#include <cherokee/common.h>
#include <cherokee/buffer.h>
#include <cherokee/logger_writer.h>

CHEROKEE_BEGIN_DECLS

#define SERVICES_MESSAGE_MAX_SIZE 4096

typedef enum {
	service_id_spawn_request,
	service_id_spawn_reply,
} cherokee_service_id_t;

typedef enum {
	service_magic_executable = 0xF0,
	service_magic_uid_gid = 0xF1,
	service_magic_chroot = 0xF2,
	service_magic_environment = 0xF3,
	service_magic_error_log = 0xF4,
	service_magic_pid = 0xF5,
	service_magic_fdmap = 0xF6,
	service_magic_chdir = 0xF7,
} cherokee_service_magic_t;

typedef struct {
	int fd_in;
	int fd_out;
	int fd_err;
} cherokee_services_fdmap_t;

ret_t cherokee_services_client_init (int fd);
ret_t cherokee_services_client_free (void);
ret_t cherokee_services_client_spawn (cherokee_buffer_t         *binary,
				      cherokee_buffer_t         *user_name,
				      uid_t                      uid,
				      gid_t                      gid,
				      cherokee_buffer_t         *chroot,
				      cherokee_buffer_t         *chdir,
				      int                        env_inherited,
				      char                     **envp,
				      cherokee_logger_writer_t  *error_writer,
				      pid_t                     *pid_ret,
				      cherokee_services_fdmap_t *fd_map);

ret_t cherokee_services_server_init (int *child_fd);
ret_t cherokee_services_server_free (void);
ret_t cherokee_services_server_serve_request (void);

ret_t cherokee_services_send (int fd,
			      cherokee_buffer_t *buf,
			      cherokee_services_fdmap_t *fd_map);
ret_t cherokee_services_receive (int fd,
				 cherokee_buffer_t *buf,
				 cherokee_services_fdmap_t *fd_map);

CHEROKEE_END_DECLS

#endif /* CHEROKEE_SERVICES_H */
