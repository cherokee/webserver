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
static cherokee_buffer_t recvbuf = CHEROKEE_BUF_INIT;

ret_t
cherokee_services_server_init (int *client_fd)
{
	int fds[2];

	if (_fd != -1) {
		LOG_ERRNO (EBUSY, cherokee_err_warning, CHEROKEE_ERROR_SERVER_ALREADY_INIT);
		return ret_error;
	}

	if (cherokee_socketpair (fds, false) < 0) {
		LOG_ERRNO (errno, cherokee_err_warning, CHEROKEE_ERROR_SERVER_CANNOT_SOCKETPAIR);
	}

	_fd = fds[0];
	cherokee_fd_set_closexec(_fd);

	*client_fd = fds[1];

	return ret_ok;
}


ret_t
cherokee_services_server_free (void)
{
	if (_fd == -1)
		return ret_ok;

	cherokee_fd_close (_fd);
	_fd = -1;

	return ret_ok;
}

static void
do_spawn (cherokee_buffer_t *buf, cherokee_services_fdmap_t *fd_map)
{
	int                  n;
	int                  size;
	uid_t                uid;
	gid_t                gid;
	int                  env_inherit;
	pid_t                child        = -1;
	int                  envs         = 0;
	int                  log_stderr   = 0;
	char                *interpreter  = NULL;
	char                *log_file     = NULL;
	char                *uid_str      = NULL;
	char                *chroot_dir   = NULL;
	char               **envp         = NULL;
	char                *chdir_dir    = NULL;
	char                *p            = buf->buf;
	const char          *argv[]       = {"sh", "-c", NULL, NULL};
	cherokee_boolean_t   use_fdmap    = false;

#define CHECK_MARK(val)           \
	if ((*(int *)p) != val) { \
		goto cleanup;     \
	} else {                  \
		p += sizeof(int); \
	}

#define ALIGN4(buf) while ((long)p & 0x3) p++;

	/* Read the supplied buffer
	 */

	CHECK_MARK (service_id_spawn_request);

	/* 1.- Interpreter */
	CHECK_MARK (service_magic_executable);

	size = *((int *)p);
	p += sizeof(int);
	if (size <= 0) {
		goto cleanup;
	}

	interpreter = malloc (sizeof("exec ") + size);
	if (interpreter == NULL) {
		goto cleanup;
	}
	strncpy (interpreter, "exec ", 5);
	strncpy (interpreter + 5, p, size + 1);
	p += size + 1;
	ALIGN4 (p);

	/* 2.- UID & GID */
	CHECK_MARK (service_magic_uid_gid);

	size = *((int *)p);
	if (size > 0) {
		uid_str = strdup (p + sizeof(int));
	}
	p += sizeof(int) + size + 1;
	ALIGN4 (p);

	memcpy (&uid, p, sizeof(uid_t));
	p += sizeof(uid_t);

	memcpy (&gid, p, sizeof(gid_t));
	p += sizeof(gid_t);

	/* 3.- Chroot directory */
	CHECK_MARK (service_magic_chroot);
	size = *((int *) p);
	p += sizeof(int);
	if (size > 0) {
		chroot_dir = malloc(size + 1);
		memcpy(chroot_dir, p, size + 1);
	}
	p += size + 1;
	ALIGN4 (p);


	/* 4.- Chdir directory */
	CHECK_MARK (service_magic_chdir);
	size = *((int *) p);
	p += sizeof(int);
	if (size > 0) {
		chdir_dir = malloc(size + 1);
		memcpy(chdir_dir, p, size + 1);
	}
	p += size + 1;
	ALIGN4 (p);

	/* 5.- Environment */
	CHECK_MARK (service_magic_environment);

	env_inherit = *((int *)p);
	p += sizeof(int);

	envs = *((int *)p);
	p += sizeof(int);

	envp = malloc (sizeof(char *) * (envs + 1));
	if (envp == NULL) {
		goto cleanup;
	}
	envp[envs] = NULL;

	for (n = 0; n < envs; n++) {
		char *e;

		size = *((int *)p);
		p += sizeof(int);

		e = malloc (size + 1);
		if (e == NULL) {
			goto cleanup;
		}

		memcpy (e, p, size);
		e[size] = '\0';

		envp[n] = e;
		p += size + 1;
		ALIGN4 (p);
	}

	/* 6.- Error log */
	CHECK_MARK (service_magic_error_log);

	size = *((int *)p);
	p += sizeof(int);

	if (size > 0) {
		if (! strncmp (p, "stderr", 6)) {
			log_stderr = 1;
		} else if (! strncmp (p, "file,", 5)) {
			log_file = p+5;
		}

		p += (size + 1);
		ALIGN4 (p);
	}

	/* 7.- PID: if not -1, attempt to kill it */
	CHECK_MARK (service_magic_pid);

	n = *((int *)p);
	if (n > 0) {
		kill (n, SIGTERM);
		*p = -1;
	}
	p += sizeof(int);

	/* 8.- FDMap: 0 entries or 3 entries supported */
	CHECK_MARK (service_magic_fdmap);

	n  = *((int *)p);
	p += sizeof(int);
	if (n > 0) {
		/* Right now we only support stdin, stdout and stderr all
		 * being passed in and in order.  As such, simply assure
		 * ourselves of this for now
		 */
		if (n != 3)
			goto cleanup;
		CHECK_MARK (0);
		CHECK_MARK (1);
		CHECK_MARK (2);
		/* At this point we know we want to use the fdmap */
		use_fdmap = true;
	}

	/* Spawn
	 */
	child = fork();
	switch (child) {
	case 0: {
		/* Reset signal handlers */
		cherokee_reset_signals();

		/* Logging */
		if (!use_fdmap && log_file) {
			int fd;
			fd = open (log_file, O_WRONLY | O_APPEND | O_CREAT, 0600);
			if (fd < 0) {
				PRINT_ERROR ("(warning) Couldn't open '%s' for writing..\n", log_file);
			}
			close (STDOUT_FILENO);
			close (STDERR_FILENO);
			dup2 (fd, STDOUT_FILENO);
			dup2 (fd, STDERR_FILENO);
		} else if (!use_fdmap && log_stderr) {
			/* do nothing */
		} else if (!use_fdmap) {
			int tmp_fd;
			tmp_fd = open ("/dev/null", O_WRONLY);

			close (STDOUT_FILENO);
			close (STDERR_FILENO);
			dup2 (tmp_fd, STDOUT_FILENO);
			dup2 (tmp_fd, STDERR_FILENO);
		} else {
			/* Using fdmap */
			dup2 (fd_map->fd_in, STDIN_FILENO);
			dup2 (fd_map->fd_out, STDOUT_FILENO);
			dup2 (fd_map->fd_err, STDERR_FILENO);
			close (fd_map->fd_in);
			close (fd_map->fd_out);
			close (fd_map->fd_err);
		}

		/* Change directory */
		if (chdir_dir != NULL || chroot_dir != NULL) {
			int re = chdir((chdir_dir != NULL) ?
				       chdir_dir : chroot_dir);
			if (re < 0) {
				PRINT_ERROR ("(critical) Couldn't chdir to %s\n",
					     ((chdir_dir != NULL) ?
					      chdir_dir : chroot_dir));
			}
		}

		/* Change root */
		if (chroot_dir) {
			int re = chroot(chroot_dir);
			if (re < 0) {
				PRINT_ERROR ("(critial) Couldn't chroot to %s\n", chroot_dir);
				exit (1);
			}
		}

		/* Change user & group */
		if (uid_str != NULL) {
			n = initgroups (uid_str, gid);
			if (n == -1) {
				PRINT_ERROR ("(warning) initgroups failed User=%s, GID=%d\n", uid_str, gid);
			}
		}

		if ((int)gid != -1) {
			n = setgid (gid);
			if (n != 0) {
				PRINT_ERROR ("(warning) Couldn't set GID=%d\n", gid);
			}
		}

		if ((int)uid != -1) {
			n = setuid (uid);
			if (n != 0) {
				PRINT_ERROR ("(warning) Couldn't set UID=%d\n", uid);
			}
		}

		/* Clean the incoming buffer */
		cherokee_buffer_mrproper(buf);

		/* Execute the interpreter */
		argv[2] = interpreter;

		if (env_inherit) {
			do {
				execv ("/bin/sh", (char **)argv);
			} while (errno == EINTR);
		} else {
			do {
				execve ("/bin/sh", (char **)argv, envp);
			} while (errno == EINTR);
		}

		PRINT_MSG ("(critical) Couldn't spawn: sh -c %s\n", interpreter);
		exit (1);
	}

	case -1:
		/* Error */
		PRINT_MSG ("(critical) Couldn't fork(): %s\n", strerror(errno));
		goto cleanup;

	default:
		break;
	}

	/* Return the PID
	 */
	printf ("PID %d: launched '/bin/sh -c %s' with uid=%d, gid=%d, chroot=%s, env=%s\n", child, interpreter, uid, gid, chroot_dir, env_inherit ? "inherited":"custom");

cleanup:
	/* Unlock worker by sending new PID back
	 */
	cherokee_buffer_mrproper(buf);
	cherokee_buffer_ensure_size(buf, sizeof(int) * 2);
	n = service_id_spawn_reply;
	cherokee_buffer_add        (buf, (char *)&n, sizeof(int));
	n = child;
	cherokee_buffer_add        (buf, (char *)&n, sizeof(int));
	cherokee_services_send(_fd, buf, NULL);


	/* Clean up
	 */
	free (uid_str);
	free (interpreter);
	free (chroot_dir);
	free (chdir_dir);

	if (envp != NULL) {
		for (n = 0; n < envs; n++) {
			free (envp[n]);
		}
		free (envp);
	}
	if (fd_map->fd_in != -1)
		close (fd_map->fd_in);
	if (fd_map->fd_out != -1)
		close (fd_map->fd_out);
	if (fd_map->fd_err != -1)
		close (fd_map->fd_err);
}


ret_t
cherokee_services_server_serve_request (void)
{
	ret_t ret;
	cherokee_services_fdmap_t fd_map = { -1, -1, -1 };

	ret = cherokee_buffer_ensure_size(&recvbuf, SERVICES_MESSAGE_MAX_SIZE);
	if (ret != ret_ok)
		return ret;

	ret = cherokee_services_receive (_fd, &recvbuf, &fd_map);
	if (ret != ret_ok)
		return ret;

	switch (*((int *)recvbuf.buf)) {
	case service_id_spawn_request:
		do_spawn (&recvbuf, &fd_map);
		return ret_ok;
	}

	return ret_error;
}
