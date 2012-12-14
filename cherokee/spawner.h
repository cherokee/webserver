/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/* Cherokee
 *
 * Authors:
 *      Alvaro Lopez Ortega <alvaro@alobbs.com>
 *
 * Copyright (C) 2001-2013 Alvaro Lopez Ortega
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

#ifndef CHEROKEE_SPAWNER_H
#define CHEROKEE_SPAWNER_H

#ifdef HAVE_SYS_TYPES_H
# include <sys/types.h>
#endif

#ifdef HAVE_PWD_H
# include <pwd.h>
#endif

#include <cherokee/common.h>
#include <cherokee/buffer.h>
#include <cherokee/shm.h>
#include <cherokee/logger_writer.h>

CHEROKEE_BEGIN_DECLS

#define SPAWN_SHARED_LEN  4096

extern cherokee_shm_t cherokee_spawn_shared;

ret_t cherokee_spawner_set_active (cherokee_boolean_t active);

ret_t cherokee_spawner_init       (void);
ret_t cherokee_spawner_free       (void);

ret_t cherokee_spawner_spawn      (cherokee_buffer_t         *binary,
				   cherokee_buffer_t         *user_name,
				   uid_t                      uid,
				   gid_t                      gid,
				   int                        env_inherited,
				   char                     **envp,
				   cherokee_logger_writer_t  *error_writer,
				   pid_t                     *pid_ret);

CHEROKEE_END_DECLS

#endif /* CHEROKEE_SPAWNER_H */
