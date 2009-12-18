/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/* Cherokee
 *
 * Authors:
 *      Alvaro Lopez Ortega <alvaro@alobbs.com>
 *
 * Copyright (C) 2001-2009 Alvaro Lopez Ortega
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
#include "spawner.h"
#include "logger_writer.h"
#include "util.h"

#include <unistd.h>
#include <signal.h>
#include <sys/sem.h>

#define ENTRIES "spawn,spawner"

cherokee_shm_t             cherokee_spawn_shared;
int                        cherokee_spawn_sem;
static cherokee_boolean_t _active                 = true;
CHEROKEE_MUTEX_T          (spawning_mutex);

ret_t
cherokee_spawner_set_active (cherokee_boolean_t active)
{
	_active = active;
	return ret_ok;
}

ret_t
cherokee_spawner_init (void)
{
#ifdef HAVE_POSIX_SHM
	ret_t             ret;
	cherokee_buffer_t name = CHEROKEE_BUF_INIT;

	if (! _active) {
		return ret_ok;
	}

	/* Monitor mutex */
	CHEROKEE_MUTEX_INIT (&spawning_mutex, CHEROKEE_MUTEX_FAST);

	/* Shared memory */
	cherokee_buffer_add_va (&name, "/cherokee-spawner-%d", getppid());

	ret = cherokee_shm_init (&cherokee_spawn_shared);
	if (ret != ret_ok) {
		goto error;
	}

	ret = cherokee_shm_map (&cherokee_spawn_shared, &name);
	if (ret != ret_ok) {
		goto error;
	}

	/* Semaphore */
	cherokee_spawn_sem = semget (getppid(), 0, 0);
	if (cherokee_spawn_sem < 0) {
		goto error;
	}

	TRACE (ENTRIES",sem", "Got semaphore %d\n", cherokee_spawn_sem);

	cherokee_buffer_mrproper (&name);
	return ret_ok;

error:
	LOG_ERRNO (errno, cherokee_err_error, CHEROKEE_ERROR_SPAWNER_SHM_INIT, name.buf);
	cherokee_buffer_mrproper (&name);
	return ret_error;
#else
	return ret_not_found;
#endif
}


ret_t
cherokee_spawner_free (void)
{
	CHEROKEE_MUTEX_DESTROY (&spawning_mutex);

#ifdef HAVE_POSIX_SHM
	cherokee_shm_mrproper (&cherokee_spawn_shared);
#endif
	return ret_ok;
}

#ifdef HAVE_POSIX_SHM
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
#endif

static ret_t
sem_unlock (int sem)
{
	int           re;
	struct sembuf so;

	do {
		so.sem_num = 0;
		so.sem_op  = 1;
		so.sem_flg = SEM_UNDO;

		errno = 0;
		re = semop (sem, &so, 1);
		if (re >= 0) {
			return ret_ok;
		}
	} while ((re < 0) && (errno == EINTR));

	LOG_ERRNO (errno, cherokee_err_error, CHEROKEE_ERROR_SPAWNER_UNLOCK_SEMAPHORE, sem);
	return ret_error;
}

ret_t
cherokee_spawner_spawn (cherokee_buffer_t         *binary,
			cherokee_buffer_t         *user,
			uid_t                      uid,
			gid_t                      gid,
			int                        env_inherited,
			char                     **envp,
			cherokee_logger_writer_t  *error_writer,
			pid_t                     *pid_ret)
{
#ifdef HAVE_POSIX_SHM
	char             **n;
	int               *pid_shm;
	int                pid_prev;
	int                k;
	int                envs     = 0;
	cherokee_buffer_t  tmp      = CHEROKEE_BUF_INIT;

	/* Check it's initialized
	 */
	if ((! _active) ||
	    (cherokee_spawn_shared.mem == NULL))
	{
		TRACE (ENTRIES, "Spawner is not active. Returning: %s\n", binary->buf);
		return ret_deny;
	}

	/* Lock the monitor mutex
	 */
	k = CHEROKEE_MUTEX_TRY_LOCK (&spawning_mutex);
	if (k) {
		return ret_eagain;
	}

	/* Build the string
	 */
	cherokee_buffer_ensure_size (&tmp, SPAWN_SHARED_LEN);

	/* 1.- Executable */
	cherokee_buffer_add        (&tmp, (char *)&binary->len, sizeof(int));
	cherokee_buffer_add_buffer (&tmp, binary);
	cherokee_buffer_add_char   (&tmp, '\0');

	/* 2.- UID & GID */
	cherokee_buffer_add        (&tmp, (char *)&user->len, sizeof(int));
	cherokee_buffer_add_buffer (&tmp, user);
	cherokee_buffer_add_char   (&tmp, '\0');

	cherokee_buffer_add (&tmp, (char *)&uid, sizeof(uid_t));
	cherokee_buffer_add (&tmp, (char *)&gid, sizeof(gid_t));

	/* 3.- Environment */
	for (n=envp; *n; n++) {
		envs ++;
	}

	cherokee_buffer_add (&tmp, (char *)&env_inherited, sizeof(int));
	cherokee_buffer_add (&tmp, (char *)&envs, sizeof(int));

	for (n=envp; *n; n++) {
		int len = strlen(*n);
		cherokee_buffer_add      (&tmp, (char *)&len, sizeof(int));
		cherokee_buffer_add      (&tmp, *n, len);
		cherokee_buffer_add_char (&tmp, '\0');
	}

	/* 4.- Error log */
	write_logger (&tmp, error_writer);

	/* 5.- PID (will be rewritten by the other side) */
	pid_shm = (int *) (((char *)cherokee_spawn_shared.mem) + tmp.len);
	k        = *pid_ret;
	pid_prev = *pid_ret;
	cherokee_buffer_add (&tmp, (char *)&k, sizeof(int));

	/* Copy it to the shared memory
	 */
	if (unlikely (tmp.len > SPAWN_SHARED_LEN)) {
		goto error;
	}

	memcpy (cherokee_spawn_shared.mem, tmp.buf, tmp.len);
	cherokee_buffer_mrproper (&tmp);

	/* Wake up the spawning thread
	 */
	sem_unlock (cherokee_spawn_sem);

	/* Wait for the PID
	 */
	k = 0;
	while (((*pid_shm == pid_prev) || (*pid_shm <= 0)) && (k < 3)) {
		k++;
		sleep(1);
	}

	if (*pid_shm == -1) {
		TRACE(ENTRIES, "Could not get the PID of: '%s'\n", binary->buf);
		goto error;

	} else if (*pid_shm == pid_prev) {
		TRACE(ENTRIES, "Could not the new PID, previously it was %d\n", pid_prev);
		goto error;
	}

	TRACE(ENTRIES, "Successfully launched PID=%d\n", *pid_shm);
	*pid_ret = *pid_shm;

	CHEROKEE_MUTEX_UNLOCK (&spawning_mutex);
	return ret_ok;

error:
	CHEROKEE_MUTEX_UNLOCK (&spawning_mutex);
	return ret_error;
#else
	return ret_not_found;
#endif
}

