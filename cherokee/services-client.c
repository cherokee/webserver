/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/* Cherokee
 *
 * Authors:
 *      Alvaro Lopez Ortega <alvaro@alobbs.com>
 *      Daniel Silverstone <dsilvers@digital-scurf.org>
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

#include "common-internal.h"
#include "services.h"
#include "logger_writer.h"
#include "util.h"

#include <unistd.h>
#include <signal.h>

#define ENTRIES "services,spawn,spawner"

static int _fd = -1;
CHEROKEE_MUTEX_T          (client_mutex);

ret_t
cherokee_services_client_init (int client_fd)
{
	if (_fd != -1) {
		LOG_ERRNO (EBUSY, cherokee_err_warning, CHEROKEE_ERROR_CLIENT_ALREADY_INIT);
		return ret_error;
	}

	/* Monitor mutex */
	CHEROKEE_MUTEX_INIT (&client_mutex, CHEROKEE_MUTEX_FAST);

	_fd = client_fd;

	cherokee_fd_set_closexec(_fd);

	return ret_ok;
}


ret_t
cherokee_services_client_free (void)
{
	if (_fd == -1)
		return ret_ok;

	CHEROKEE_MUTEX_DESTROY (&client_mutex);

	cherokee_fd_close (_fd);
	_fd = -1;

	return ret_ok;
}


static ret_t
write_logger (cherokee_buffer_t        *buf,
	      cherokee_logger_writer_t *error_writer)
{
	int val;

	/* No writer
	 */
	if (error_writer == NULL) {
		goto nothing;
	}

	switch (error_writer->type) {
	case cherokee_logger_writer_stderr:
		val = 6;
		cherokee_buffer_add      (buf, (char *)&val, sizeof(int));
		cherokee_buffer_add_str  (buf, "stderr");
		cherokee_buffer_add_char (buf, '\0');
		break;
	case cherokee_logger_writer_file:
		val = 5 + error_writer->filename.len;
		cherokee_buffer_add        (buf, (char *)&val, sizeof(int));
		cherokee_buffer_add_str    (buf, "file,");
		cherokee_buffer_add_buffer (buf, &error_writer->filename);
		cherokee_buffer_add_char   (buf, '\0');
		break;
	default:
		goto nothing;
	}

	return ret_ok;

nothing:
	val = 0;
	cherokee_buffer_add (buf, (char *)&val, sizeof(int));
	return ret_ok;
}


ret_t
cherokee_services_client_spawn (cherokee_buffer_t         *binary,
                                cherokee_buffer_t         *user,
                                uid_t                      uid,
                                gid_t                      gid,
                                cherokee_buffer_t         *chroot,
                                cherokee_buffer_t         *chdir,
                                int                        env_inherited,
                                char                     **envp,
                                cherokee_logger_writer_t  *error_writer,
                                pid_t                     *pid_ret,
                                cherokee_services_fdmap_t *fd_map)
{
	char             **n;
	int                pid_new;
	int                k;
	int                phase;
	int                envs     = 0;
	ret_t              ret;
	cherokee_buffer_t  tmp      = CHEROKEE_BUF_INIT;

#define ALIGN4(buf)                                    \
	while (buf.len & 0x3) {                        \
		cherokee_buffer_add_char (&buf, '\0'); \
	}


	/* Check it's initialized
	 */
	if (_fd == -1)
	{
		TRACE (ENTRIES, "Spawner is not active. Returning: %s\n", binary->buf);
		return ret_deny;
	}

	/* Lock the monitor mutex
	 */
	k = CHEROKEE_MUTEX_TRY_LOCK (&client_mutex);
	if (k) {
		return ret_eagain;
	}

	/* Build the string
	 * The first character of each block is a mark.
	 */
	cherokee_buffer_ensure_size (&tmp, SERVICES_MESSAGE_MAX_SIZE);

	phase = (int)service_id_spawn_request;
	cherokee_buffer_add        (&tmp, (char *)&phase, sizeof(int));

	/* 1.- Executable */
	phase = service_magic_executable;
	cherokee_buffer_add        (&tmp, (char *)&phase, sizeof(int));
	cherokee_buffer_add        (&tmp, (char *)&binary->len,   sizeof(int));
	cherokee_buffer_add_buffer (&tmp, binary);
	cherokee_buffer_add_char   (&tmp, '\0');
	ALIGN4 (tmp);

	/* 2.- UID & GID */
	phase = service_magic_uid_gid;
	cherokee_buffer_add        (&tmp, (char *)&phase, sizeof(int));
	cherokee_buffer_add        (&tmp, (char *)&user->len, sizeof(int));
	cherokee_buffer_add_buffer (&tmp, user);
	cherokee_buffer_add_char   (&tmp, '\0');
	ALIGN4(tmp);

	cherokee_buffer_add (&tmp, (char *)&uid, sizeof(uid_t));
	cherokee_buffer_add (&tmp, (char *)&gid, sizeof(gid_t));

	/* 3.- Chroot directory */
	phase = service_magic_chroot;
	cherokee_buffer_add (&tmp, (char *)&phase, sizeof(int));
	cherokee_buffer_add        (&tmp, (char *)&chroot->len, sizeof(int));
	cherokee_buffer_add_buffer (&tmp, chroot);
	cherokee_buffer_add_char   (&tmp, '\0');
	ALIGN4(tmp);

	/* 4.- Chdir directory */
	phase = service_magic_chdir;
	cherokee_buffer_add (&tmp, (char *)&phase, sizeof(int));
	cherokee_buffer_add        (&tmp, (char *)&chdir->len, sizeof(int));
	cherokee_buffer_add_buffer (&tmp, chdir);
	cherokee_buffer_add_char   (&tmp, '\0');
	ALIGN4(tmp);

	/* 5.- Environment */
	phase = service_magic_environment;
	cherokee_buffer_add (&tmp, (char *)&phase, sizeof(int));

	for (n = envp; *n; n++) {
		envs ++;
	}

	cherokee_buffer_add (&tmp, (char *)&env_inherited, sizeof(int));
	cherokee_buffer_add (&tmp, (char *)&envs, sizeof(int));

	for (n = envp; *n; n++) {
		int len = strlen(*n);
		cherokee_buffer_add      (&tmp, (char *)&len, sizeof(int));
		cherokee_buffer_add      (&tmp, *n, len);
		cherokee_buffer_add_char (&tmp, '\0');
		ALIGN4(tmp);
	}

	/* 6.- Error log */
	phase = service_magic_error_log;
	cherokee_buffer_add (&tmp, (char *)&phase, sizeof(int));

	write_logger (&tmp, error_writer);
	ALIGN4 (tmp);

	/* 7.- PID (will be closed by the other side if not -1) */
	phase = service_magic_pid;
	cherokee_buffer_add (&tmp, (char *)&phase, sizeof(int));
	cherokee_buffer_add (&tmp, (char *)pid_ret, sizeof(int));

	/* 8.- FD Map */
	phase = service_magic_fdmap;
	cherokee_buffer_add (&tmp, (char *)&phase, sizeof(int));
	if (fd_map != NULL) {
		int fd = 3;
		cherokee_buffer_add (&tmp, (char *)&fd,sizeof(int));
		fd = 0;
		cherokee_buffer_add (&tmp, (char *)&fd, sizeof(int));
		fd = 1;
		cherokee_buffer_add (&tmp, (char *)&fd, sizeof(int));
		fd = 2;
		cherokee_buffer_add (&tmp, (char *)&fd, sizeof(int));
	} else {
		phase = 0;
		cherokee_buffer_add (&tmp, (char *)&phase, sizeof(int));
	}

	/* Send it to the services server in the main cherokee process
	 */
	if (unlikely (tmp.len > SERVICES_MESSAGE_MAX_SIZE)) {
		goto error;
	}

	ret = cherokee_services_send(_fd, &tmp, fd_map);
	cherokee_buffer_mrproper (&tmp);

	if (ret != ret_ok) {
		goto error;
	}

	cherokee_buffer_ensure_size (&tmp, SERVICES_MESSAGE_MAX_SIZE);

	/* Wait for the PID
	 */
	ret = cherokee_services_receive(_fd, &tmp, NULL);

	if (ret != ret_ok) {
		goto error;
	}

	if (tmp.len != (sizeof(int) * 2)) {
		goto error;
	}

	if (*((int *)tmp.buf) != service_id_spawn_reply)
		goto error;

	pid_new = *(((int *)tmp.buf)+1);

	if (pid_new == -1) {
		TRACE(ENTRIES, "Could not get the PID of: '%s'\n", binary->buf);
		goto error;
	}

	if (*pid_ret == pid_new) {
		TRACE(ENTRIES, "Could not the new PID, previously it was %d\n", *pid_ret);
		goto error;
	}

	TRACE(ENTRIES, "Successfully launched PID=%d\n", pid_new);
	*pid_ret = pid_new;

	CHEROKEE_MUTEX_UNLOCK (&client_mutex);
	return ret_ok;

error:
	CHEROKEE_MUTEX_UNLOCK (&client_mutex);
	return ret_error;
}

