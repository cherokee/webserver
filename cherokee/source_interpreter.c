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

#include "common-internal.h"
#include "source_interpreter.h"
#include "util.h"
#include "connection-protected.h"
#include "thread.h"
#include "bogotime.h"
#include "services.h"
#include "logger_writer.h"

#include <sys/types.h>
#include <unistd.h>
#include <signal.h>

#define ENTRIES "source,src,interpreter"

#define DEFAULT_TIMEOUT          10
#define GRNAM_BUF_LEN            8192
#define MAX_SPAWN_FAILS_IN_A_ROW 5

static void interpreter_free (void *src);

#define source_is_unresponsive(src) \
	(cherokee_bogonow_now > (src)->spawning_since + (src)->timeout)

ret_t
cherokee_source_interpreter_new  (cherokee_source_interpreter_t **src)
{
	CHEROKEE_NEW_STRUCT(n, source_interpreter);

	cherokee_source_init (SOURCE(n));
	cherokee_buffer_init (&n->interpreter);
	cherokee_buffer_init (&n->change_user_name);
	cherokee_buffer_init (&n->chroot);

	n->custom_env           = NULL;
	n->custom_env_len       = 0;
	n->env_inherited        = true;
	n->debug                = false;
	n->pid                  = -1;
	n->timeout              = DEFAULT_TIMEOUT;
	n->change_user          = -1;
	n->change_group         = -1;
	n->spawn_type           = spawn_unknown;
	n->spawning_since       = 0;
	n->spawning_since_fails = 0;
	n->last_connect         = 0;

	SOURCE(n)->type = source_interpreter;
	SOURCE(n)->free = (cherokee_func_free_t)interpreter_free;

	CHEROKEE_MUTEX_INIT (&n->launching_mutex, NULL);
	n->launching = false;

	*src = n;
	return ret_ok;
}


static void
free_custom_env (void *ptr)
{
	cuint_t                        i;
	cherokee_source_interpreter_t *src = ptr;

	for (i=0; src->custom_env[i] != NULL; i++) {
		free (src->custom_env[i]);
	}

	free (src->custom_env);
}


static void
kill_pid (cherokee_source_interpreter_t *src)
{
	if (src->pid <= 0)
		return;

	TRACE(ENTRIES, "Killing %s, pid=%d\n", src->interpreter.buf, src->pid);
	kill (src->pid, SIGTERM);
}

static void
interpreter_free (void *ptr)
{
	cherokee_source_interpreter_t *src = ptr;

	/* Only frees its stuff, the rest will be freed by
	 * cherokee_source_t.
	 */
	kill_pid (src);

	cherokee_buffer_mrproper (&src->interpreter);
	cherokee_buffer_mrproper (&src->change_user_name);
	cherokee_buffer_mrproper (&src->chroot);

	if (src->custom_env)
		free_custom_env (src);

	CHEROKEE_MUTEX_DESTROY (&src->launching_mutex);
}

static char *
find_next_stop (char *p)
{
	char *s;
	char *w;

	s = strchr (p, '/');
	w = strchr (p, ' ');

	if ((s == NULL) && (w == NULL))
		return NULL;

	if (w == NULL)
		return s;
	if (s == NULL)
		return w;

	return (w > s) ? s : w;
}

static ret_t
check_interpreter_full (cherokee_buffer_t *fullpath, cherokee_buffer_t *chroot_dir)
{
	ret_t        ret = ret_error;
	int          re;
	struct stat  inter;
	char        *p;
	char         tmp;
	const char  *end;
	cherokee_buffer_t completepath = CHEROKEE_BUF_INIT;

	cherokee_buffer_clean(&completepath);
	if (chroot_dir->len > 0) {
		/* Chroot and relative path, it doesn't make sense */
		if (fullpath->len == 0 || fullpath->buf[0] != '/')
			goto done;

		cherokee_buffer_add_buffer (&completepath, chroot_dir);
	}
	cherokee_buffer_add_buffer(&completepath, fullpath);
	end = completepath.buf + completepath.len;

	p = find_next_stop (completepath.buf + 1);
	if (p == NULL)
		goto done;

	while (p <= end) {
		/* Set a temporal end */
		tmp = *p;
		*p  = '\0';

		/* Does the file exist? */
		re = cherokee_stat (completepath.buf, &inter);
		if ((re == 0) &&
		    (! S_ISDIR(inter.st_mode)))
		{
			*p = tmp;
			ret = ret_ok;
			goto done;
		}

		*p = tmp;

		/* Exit if already reached the end */
		if (p >= end)
			break;

		/* Find the next position */
		p = find_next_stop (p+1);
		if (p == NULL)
			p = (char *)end;
	}

done:
	cherokee_buffer_mrproper(&completepath);
	return ret;
}


static ret_t
check_interpreter_path (cherokee_buffer_t *partial_path, cherokee_buffer_t *chroot_dir)
{
	ret_t              ret;
	char              *p;
	char              *colon;
	char              *path;
	cherokee_buffer_t  fullpath = CHEROKEE_BUF_INIT;

	p = getenv("PATH");
	if (p == NULL)
		return ret_error;

	path = strdup (p);
	if (path == NULL)
		return ret_error;

	p = path;
	do {
		colon = strchr(p, ':');
		if (colon != NULL)
			*colon = '\0';

		cherokee_buffer_clean      (&fullpath);
		cherokee_buffer_add        (&fullpath, p, strlen(p));
		cherokee_buffer_add_char   (&fullpath, '/');
		cherokee_buffer_add_buffer (&fullpath, partial_path);

		ret = check_interpreter_full (&fullpath, chroot_dir);
		if (ret == ret_ok)
			goto done;

		if (colon == NULL)
			break;

		p = colon + 1;
	} while (true);

	ret = ret_not_found;

done:
	cherokee_buffer_mrproper (&fullpath);
	free (path);

	return ret;
}

static ret_t
command_has_env_variables (cherokee_buffer_t *command)
{
	int                n     = 0;
	cherokee_boolean_t equal = false;

	while (n < command->len) {
		if (command->buf[n] == '=') {
			equal = true;
		} else if (command->buf[n] == ' ') {
			return (equal)? ret_error : ret_ok;
		}
		n++;
	}

	return ret_ok;
}

static ret_t
check_interpreter (cherokee_source_interpreter_t *src)
{
	ret_t ret;

	if (src->interpreter.buf[0] == '/') {
		ret = check_interpreter_full (&src->interpreter, &src->chroot);
	} else {
		ret = check_interpreter_path (&src->interpreter, &src->chroot);
	}

	return ret;
}


static ret_t
replace_environment_variables (cherokee_buffer_t *buf)
{
	char              *dollar;
	char              *p;
	char              *val;
	cherokee_buffer_t  tmp     = CHEROKEE_BUF_INIT;

	do {
		/* Find $
		 */
		dollar = strchr (buf->buf, '$');
		if (dollar == NULL) {
			goto out;
		}

		/* End of the variable name
		 */
		p = dollar + 1;
		while ((*p == '_') ||
		       ((*p >= '0') && (*p <= '9')) ||
		       ((*p >= 'A') && (*p <= 'Z')) ||
		       ((*p >= 'a') && (*p <= 'z')))
		{
			p++;
		}

		/* Call getenv
		 */
		cherokee_buffer_clean (&tmp);
		cherokee_buffer_add   (&tmp, dollar, p-dollar);

		val = getenv (tmp.buf + 1);

		/* Replacement
		 */
		if (val != NULL) {
			cherokee_buffer_replace_string (buf, tmp.buf, tmp.len, val, strlen(val));
		} else {
			cherokee_buffer_remove_string (buf, tmp.buf, tmp.len);
		}

	} while (true);

out:
	cherokee_buffer_mrproper (&tmp);
	return ret_ok;
}


static ret_t
add_env (cherokee_source_interpreter_t *src, cherokee_buffer_t *env, cherokee_buffer_t *val_orig)
{
	ret_t              ret;
	int                entry_len;
	char              *entry;
	cherokee_buffer_t *val        = NULL;

	/* Replace $ENVs
	 */
	cherokee_buffer_dup (val_orig, &val);

	ret = replace_environment_variables (val);
	if (ret != ret_ok) {
		ret = ret_error;
		goto error;
	}

	/* Build the env entry
	 */
	entry_len = env->len + val->len + 2;

	entry = (char *) malloc (entry_len);
	if (unlikely (entry == NULL)) {
		ret = ret_nomem;
		goto error;
	}

	snprintf (entry, entry_len, "%s=%s", env->buf, val->buf);
	TRACE(ENTRIES, "Adding env: %s\n", entry);

	/* Add it into the env array
	 */
	if (src->custom_env_len == 0) {
		src->custom_env = malloc (sizeof (char *) * 2);
	} else {
		src->custom_env = realloc (src->custom_env, (src->custom_env_len + 2) * sizeof (char *));
	}

	if (unlikely (src->custom_env == NULL)) {
		ret = ret_nomem;
		free (entry);
		goto error;
	}

	src->custom_env_len += 1;
	src->custom_env[src->custom_env_len - 1] = entry;
	src->custom_env[src->custom_env_len]     = NULL;

	cherokee_buffer_free (val);
	return ret_ok;

error:
	cherokee_buffer_free (val);
	return ret;
}


ret_t
cherokee_source_interpreter_configure (cherokee_source_interpreter_t *src,
                                       cherokee_config_node_t        *conf,
                                       int                            prio)
{
	ret_t                   ret;
	int                     val;
	cherokee_list_t        *i, *j;
	cherokee_config_node_t *child;

	/* Configure the base class
	 */
	ret = cherokee_source_configure (SOURCE(src), conf);
	if (ret != ret_ok)
		return ret;

	/* Interpreter parameters
	 */
	cherokee_config_node_foreach (i, conf) {
		child = CONFIG_NODE(i);

		if (equal_buf_str (&child->key, "interpreter")) {
			cherokee_buffer_add_buffer (&src->interpreter, &child->val);

		} else if (equal_buf_str (&child->key, "debug")) {
			ret = cherokee_atob (child->val.buf, &src->debug);
			if (ret != ret_ok) return ret_error;

		} else if (equal_buf_str (&child->key, "timeout")) {
			ret = cherokee_atoi (child->val.buf, &val);
			if (ret != ret_ok) return ret_error;
			src->timeout = val;

		} else if (equal_buf_str (&child->key, "user")) {
			struct passwd pwd;
			char          tmp[1024];

			cherokee_buffer_add_buffer (&src->change_user_name, &child->val);

			ret = cherokee_getpwnam_uid (child->val.buf, &pwd, tmp, sizeof(tmp));
			if ((ret != ret_ok) || (pwd.pw_dir == NULL)) {
				LOG_CRITICAL (CHEROKEE_ERROR_SRC_INTER_NO_USER, child->val.buf, prio);
				return ret_error;
			}

			src->change_user = pwd.pw_uid;

			if (src->change_group == -1) {
				src->change_group = pwd.pw_gid;
			}

		} else if (equal_buf_str (&child->key, "group")) {
			struct group grp;
			char         tmp[GRNAM_BUF_LEN];

			ret = cherokee_getgrnam_gid (child->val.buf, &grp, tmp, sizeof(tmp));
			if (ret != ret_ok) {
				LOG_CRITICAL (CHEROKEE_ERROR_SRC_INTER_NO_GROUP, conf->val.buf, prio);
				return ret_error;
			}

			src->change_group = grp.gr_gid;

		} else if (equal_buf_str (&child->key, "chroot")) {
			cherokee_buffer_add_buffer (&src->chroot, &child->val);

		} else if (equal_buf_str (&child->key, "env")) {
			cherokee_config_node_foreach (j, child) {
				cherokee_config_node_t *child2 = CONFIG_NODE(j);

				ret = add_env (src, &child2->key, &child2->val);
				if (ret != ret_ok) return ret;
			}

		} else if (equal_buf_str (&child->key, "env_inherited")) {
			/* Handled later on */
		}
	}

	/* Inherited Environment
	 */
	ret = cherokee_config_node_get (conf, "env_inherited", &child);
	if (ret == ret_ok) {
		ret = cherokee_atob (child->val.buf, &src->env_inherited);
		if (ret != ret_ok) return ret_error;
	} else {
		src->env_inherited = (src->custom_env_len == 0);
	}

	/* Sanity check
	 */
	if (cherokee_buffer_is_empty (&src->interpreter)) {
		LOG_CRITICAL (CHEROKEE_ERROR_SRC_INTER_EMPTY_INTERPRETER, prio);
		return ret_error;
	}

	ret = command_has_env_variables (&src->interpreter);
	if (ret != ret_ok) {
		LOG_CRITICAL (CHEROKEE_ERROR_SRC_INTER_ENV_IN_COMMAND, prio, src->interpreter.buf);
		return ret_error;
	}

	ret = check_interpreter (src);
	if (ret != ret_ok) {
		LOG_ERROR (CHEROKEE_ERROR_SRC_INTER_NO_INTERPRETER, src->interpreter.buf, prio);
		return ret_error;
	}

	return ret_ok;
}


static ret_t
_spawn_services (cherokee_source_interpreter_t *src,
		 cherokee_logger_writer_t *error_writer)
{
	ret_t   ret;
	char  **envp;
	char   *empty_envp[] = {NULL};

	/* Sanity check
	 */
	if (cherokee_buffer_is_empty (&src->interpreter))
		return ret_not_found;

	/* Maybe set a custom enviroment variable set
	 */
	envp = (src->custom_env) ? src->custom_env : empty_envp;

	/* If a user isn't specified, use the same one..
	 */
	if (src->change_user == -1) {
		src->change_user  = getuid();
		src->change_group = getgid();
	}

	/* Invoke the spawn mechanism
	 */
	ret = cherokee_services_client_spawn (&src->interpreter,
					      &src->change_user_name,
					      src->change_user,
					      src->change_group,
					      &src->chroot,
					      &src->chroot,
					      src->env_inherited,
					      envp,
					      error_writer,
					      &src->pid,
					      NULL);
	switch (ret) {
	case ret_ok:
		break;
	case ret_eagain:
		return ret_eagain;
	default:
		return ret_error;
	}

	return ret_ok;
}


static ret_t
_spawn_local (cherokee_source_interpreter_t *src,
              cherokee_logger_writer_t *error_writer)
{
	int                re;
	int                tmp_fd;
	char             **envp;
	const char        *argv[]       = {"sh", "-c", NULL, NULL};
	int                child        = -1;
	char              *empty_envp[] = {NULL};
	cherokee_buffer_t  tmp          = CHEROKEE_BUF_INIT;

	/* If there is a previous instance running, kill it
	 */
	kill_pid (src);

	/* Maybe set a custom enviroment variable set
	 */
	envp = (src->custom_env) ? src->custom_env : empty_envp;

	/* Execute the FastCGI server
	 */
	cherokee_buffer_add_va (&tmp, "exec %s", src->interpreter.buf);
	TRACE (ENTRIES, "Spawn: /bin/sh -c \"exec %s\"\n", src->interpreter.buf);

	child = fork();

	switch (child) {
	case 0:
		/* Change user if requested
		 */
		if (! cherokee_buffer_is_empty (&src->change_user_name)) {
			initgroups (src->change_user_name.buf, src->change_user);
		}

		if (src->change_group != -1) {
			re = setgid (src->change_group);
			if (re < 0) {
				LOG_ERROR (CHEROKEE_ERROR_SRC_INTER_SETGID, src->change_group);
				exit(1);
			}
		}

		if (src->change_user != -1) {
			re = setuid (src->change_user);
			if (re < 0) {
				LOG_ERROR (CHEROKEE_ERROR_SRC_INTER_SETUID, src->change_user);
				exit(1);
			}
		}

		/* Reset signals
		 */
		cherokee_reset_signals();

		/* Redirect/Close stderr and stdout
		 */
		if (! src->debug) {
			cherokee_fd_close (STDOUT_FILENO);
			cherokee_fd_close (STDERR_FILENO);

			if ((error_writer != NULL) &&
			    (error_writer->fd != -1))
			{
				dup2 (error_writer->fd, STDOUT_FILENO);
				dup2 (error_writer->fd, STDERR_FILENO);
			}
			else {
				tmp_fd = cherokee_open ("/dev/null", O_WRONLY, 0700);
				if (tmp_fd != -1) {
					dup2 (tmp_fd, STDOUT_FILENO);
					dup2 (tmp_fd, STDERR_FILENO);
				}
			}
		}

		if (src->chroot.len > 0) {
			re = chroot(src->chroot.buf);
			if (re < 0) {
				LOG_ERROR (CHEROKEE_ERROR_SRC_INTER_CHROOT, src->chroot.buf);
				exit(1);
			}
		}

		argv[2] = (char *)tmp.buf;
		if (src->env_inherited) {
			do {
				re = execv ("/bin/sh", (char **)argv);
			} while (errno == EINTR);
		} else {
			do {
				re = execve ("/bin/sh", (char **)argv, envp);
			} while (errno == EINTR);
		}

		if (re < 0) {
			LOG_ERROR (CHEROKEE_ERROR_SRC_INTER_SPAWN, tmp.buf);
			exit (1);
		}

		exit ((re == 0) ? 0 : 1);
	case -1:
		goto error;

	default:
		src->pid = child;

		sleep (1);
		break;

	}

	cherokee_buffer_mrproper (&tmp);
	return ret_ok;

error:
	cherokee_buffer_mrproper (&tmp);
	return ret_error;
}


ret_t
cherokee_source_interpreter_spawn (cherokee_source_interpreter_t *src,
                                   cherokee_logger_writer_t      *error_writer)
{
	ret_t ret;

	/* Sanity check
	 */
	if (cherokee_buffer_is_empty (&src->interpreter)) {
		return ret_not_found;
	}

	/* Try with SHM first
	 */
	if ((src->spawn_type == spawn_services) ||
	    (src->spawn_type == spawn_unknown))
	{
		ret = _spawn_services (src, error_writer);
		if (ret == ret_ok) {
			if (src->spawn_type == spawn_unknown) {
				src->spawn_type = spawn_services;
			}

			return ret_ok;

		} else if (ret == ret_eagain) {
			return ret_eagain;
		}
		if (src->spawn_type == spawn_services) {
			return ret_error;
		}
	}

	/* No luck, go 'local' then..
	 */
	if (src->spawn_type == spawn_unknown) {
		src->spawn_type = spawn_local;
	}

	ret = _spawn_local (src, error_writer);
	if (ret != ret_ok) {
		return ret;
	}

	/* Since we managed to spawn something, clear any current address
	 * in the source so that it can retry connecting from the top.
	 */
	src->source.addr_current = NULL;

	return ret_ok;
}


ret_t
cherokee_source_interpreter_connect_polling (cherokee_source_interpreter_t *src,
                                             cherokee_socket_t             *socket,
                                             cherokee_connection_t         *conn)
{
	int   re;
	ret_t ret;
	int   unlocked;
	int   kill_prev;

	/* Connect
	 */
	ret = cherokee_source_connect (SOURCE(src), socket);
	switch (ret) {
	case ret_ok:
		/* connected */
		if (src->spawning_since != 0) {
			src->spawning_since       = 0;
			src->spawning_since_fails = 0;
		}
		src->last_connect = cherokee_bogonow_now;
		TRACE (ENTRIES, "Connected successfully fd=%d\n", socket->socket);
		return ret_ok;

	case ret_eagain:
		/* wait for the fd */
		ret = cherokee_thread_deactive_to_polling (CONN_THREAD(conn),
		                                           conn, SOCKET_FD(socket),
		                                           FDPOLL_MODE_WRITE, false);
		if (ret != ret_ok) {
			return ret_error;
		}
		return ret_eagain;

	case ret_deny:
	case ret_error:
		/* reset by peer: spawn process? */
		TRACE (ENTRIES, "Connection refused (closing fd=%d)\n", socket->socket);
		cherokee_socket_close (socket);
		break;

	default:
		cherokee_socket_close (socket);
		RET_UNKNOWN(ret);
		return ret_error;
	}

	/* Spawn a new process
	 */
	unlocked = CHEROKEE_MUTEX_TRY_LOCK (&src->launching_mutex);
	if (unlocked) {
		cherokee_connection_sleep (conn, 1000);
		return ret_eagain;
	}

	if (src->spawning_since == 0) {
		cherokee_logger_writer_t *error_writer = NULL;

		/* Check re-try limit */
		if (src->spawning_since_fails >= MAX_SPAWN_FAILS_IN_A_ROW) {
			TRACE (ENTRIES, "Failed to launch the interpreter %d consecutive times. Giving up now.\n",
			       src->spawning_since_fails);

			src->spawning_since_fails = 0;
			ret = ret_error;
			goto out;
		}

		/* Kill prev (unresponsive) interpreter? */
		kill_prev = ((src->pid > 0) && (source_is_unresponsive(src)));
		if (! kill_prev) {
			src->pid = -1;
		}

		/* Spawn */
		ret = cherokee_virtual_server_get_error_log (CONN_VSRV(conn), &error_writer);
		if (ret != ret_ok) {
			ret = ret_error;
			goto out;
		}

		ret = cherokee_source_interpreter_spawn (src, error_writer);
		switch (ret) {
		case ret_ok:
			src->spawning_since = cherokee_bogonow_now;
			ret = ret_eagain;
			goto out;

		case ret_eagain:
			cherokee_connection_sleep (conn, 1000);
			ret = ret_eagain;
			goto out;

		default:
			if (src->interpreter.buf) {
				TRACE (ENTRIES, "Couldn't spawn: %s\n", src->interpreter.buf);
			} else {
				TRACE (ENTRIES, "No interpreter to be spawned %s", "\n");
			}
			ret = ret_error;
			goto out;
		}

		SHOULDNT_HAPPEN;
		ret = ret_error;
		goto out;
	}

	/* Is the launching process death?
	 */
	if (src->pid > 0) {
		re = kill (src->pid, 0);
		if ((re != 0) && (errno == ESRCH)) {
			/* It's death */
			TRACE (ENTRIES, "PID %d is already death\n", src->pid);

			src->pid = -1;

			src->spawning_since        = 0;
			src->spawning_since_fails += 1;

			ret = ret_eagain;
			goto out;
		}
	}

	/* Is it unresponsive?
	 */
	if (source_is_unresponsive(src)) {
		src->spawning_since        = 0;
		src->spawning_since_fails += 1;

		ret = ret_eagain;
		goto out;
	}

	/* Spawning on-going
	 */
	cherokee_connection_sleep (conn, 1000);
	ret = ret_eagain;

out:
	/* Raise conn's timeout? */
	if ((src->spawning_since != 0) &&
	    (src->spawning_since + src->timeout > conn->timeout))
	{
		conn->timeout = src->spawning_since + src->timeout + 1;
	}

	CHEROKEE_MUTEX_UNLOCK (&src->launching_mutex);
	return ret;
}
