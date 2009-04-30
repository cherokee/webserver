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
#include "source_interpreter.h"
#include "util.h"
#include "connection-protected.h"
#include "thread.h"
#include "bogotime.h"
#include "spawner.h"
#include "logger_writer.h"

#include <sys/types.h>
#include <unistd.h>
#include <signal.h>

#define ENTRIES "source,src,interpreter"
#define DEFAULT_TIMEOUT 10


static void interpreter_free (void *src);


ret_t 
cherokee_source_interpreter_new  (cherokee_source_interpreter_t **src)
{
	CHEROKEE_NEW_STRUCT(n, source_interpreter);

	cherokee_source_init (SOURCE(n));
	cherokee_buffer_init (&n->interpreter);
	cherokee_buffer_init (&n->change_user_name);

	n->custom_env     = NULL;
	n->custom_env_len = 0;
	n->debug          = false;
	n->pid            = -1;
	n->timeout        = DEFAULT_TIMEOUT;
	n->change_user    = -1;
	n->change_group   = -1;

	SOURCE(n)->type   = source_interpreter;
	SOURCE(n)->free   = (cherokee_func_free_t)interpreter_free;
	
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
interpreter_free (void *ptr)
{
	cherokee_source_interpreter_t *src = ptr;

	/* Only frees its stuff, the rest will be freed by
	 * cherokee_source_t.
	 */
	if (src->pid > 0) {
		TRACE(ENTRIES, "Killing %s, pid=%d\n", src->interpreter.buf, src->pid);
		kill (src->pid, SIGTERM);
	}

	cherokee_buffer_mrproper (&src->interpreter);
	cherokee_buffer_mrproper (&src->change_user_name);

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
check_interpreter_full (cherokee_buffer_t *fullpath)
{
	int          re;
	struct stat  inter;
	char        *p;
	char         tmp;
	const char  *end    = fullpath->buf + fullpath->len;

	p = find_next_stop (fullpath->buf + 1);
	if (p == NULL)
		return ret_error;

	while (p <= end) {
		/* Set a temporal end */
		tmp = *p;
		*p  = '\0';

		/* Does the file exist? */
		re = cherokee_stat (fullpath->buf, &inter);
		if ((re == 0) &&
		    (! S_ISDIR(inter.st_mode))) 
		{
			*p = tmp;
			return ret_ok;
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

	return ret_error;
}


static ret_t
check_interpreter_path (cherokee_buffer_t *partial_path)
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

		ret = check_interpreter_full (&fullpath);
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
check_interpreter (cherokee_source_interpreter_t *src)
{
	ret_t ret;

	if (src->interpreter.buf[0] == '/') {
		ret = check_interpreter_full (&src->interpreter);
		if (ret == ret_ok) 
			return ret_ok;

		PRINT_ERROR ("ERROR: Could find interpreter '%s'\n", src->interpreter.buf);
		return ret_error;
	}
	
	return check_interpreter_path (&src->interpreter);
}

ret_t 
cherokee_source_interpreter_configure (cherokee_source_interpreter_t *src, cherokee_config_node_t *conf)
{
	ret_t                   ret;
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
			src->debug = !! atoi (child->val.buf);

		} else if (equal_buf_str (&child->key, "timeout")) {
			src->timeout = atoi (child->val.buf);

		} else if (equal_buf_str (&child->key, "user")) {
			struct passwd pwd;
			char          tmp[1024];

			cherokee_buffer_add_buffer (&src->change_user_name, &child->val);

			ret = cherokee_getpwnam (child->val.buf, &pwd, tmp, sizeof(tmp));
			if ((ret != ret_ok) || (pwd.pw_dir == NULL)) {
				PRINT_MSG ("ERROR: User '%s' not found in the system\n",
					   child->val.buf);
				return ret_error;
			}

			src->change_user = pwd.pw_uid;

			if (src->change_group == -1) {
				src->change_group = pwd.pw_gid;
			}

		} else if (equal_buf_str (&child->key, "group")) {
			struct group grp;
			char         tmp[1024];
		
			ret = cherokee_getgrnam (child->val.buf, &grp, tmp, sizeof(tmp));
			if (ret != ret_ok) {
				PRINT_MSG ("ERROR: Group '%s' not found in the system\n", conf->val.buf);
				return ret_error;
			}		
			
			src->change_group = grp.gr_gid;

		} else if (equal_buf_str (&child->key, "env")) {			
			cherokee_config_node_foreach (j, child) {
				cherokee_config_node_t *child2 = CONFIG_NODE(j);
                                
				ret = cherokee_source_interpreter_add_env (src, child2->key.buf, child2->val.buf);
				if (ret != ret_ok) return ret;
			}
		}	
	}

	/* Sanity check
	 */
	if (cherokee_buffer_is_empty (&src->interpreter)) {
		PRINT_ERROR_S ("ERROR: 'Source interpreter' with no interpreter\n");
		return ret_error;
	}

	ret = check_interpreter (src);
	if (ret != ret_ok) {
		PRINT_ERROR ("ERROR: Couldn't find interpreter '%s'\n", src->interpreter.buf);
		return ret_error;
	}

	return ret_ok;
}


ret_t 
cherokee_source_interpreter_add_env (cherokee_source_interpreter_t *src, char *env, char *val)
{
	char    *entry;
	cuint_t  env_len;
	cuint_t  val_len;

	/* Build the env entry
	 */
	env_len = strlen (env);
	val_len = strlen (val);

	entry = (char *) malloc (env_len + val_len + 2);
	if (entry == NULL) return ret_nomem;

	memcpy (entry, env, env_len);
	entry[env_len] = '=';
	memcpy (entry + env_len + 1, val, val_len);
	entry[env_len + val_len+1] = '\0';
	
	/* Add it into the env array
	 */
	if (src->custom_env_len == 0) {
		src->custom_env = malloc (sizeof (char *) * 2);
	} else {
		src->custom_env = realloc (src->custom_env, (src->custom_env_len + 2) * sizeof (char *));
	}
	src->custom_env_len +=  1;

	src->custom_env[src->custom_env_len - 1] = entry;
	src->custom_env[src->custom_env_len] = NULL;

	return ret_ok;
}

#ifdef HAVE_POSIX_SHM
static ret_t 
_spawn_shm (cherokee_source_interpreter_t *src,
	    cherokee_logger_t             *logger)
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
		src->change_user = getuid();
	}
	
	/* Invoke the spawn mechanism
	 */
	ret = cherokee_spawner_spawn (&src->interpreter,
				      &src->change_user_name,
				      src->change_user,
				      src->change_group,
				      envp,
				      logger,
				      &src->pid);
	if (ret != ret_ok) {
		return ret_error;
	}

	return ret_ok;
}
#endif


static ret_t 
_spawn_local (cherokee_source_interpreter_t *src,
	      cherokee_logger_t             *logger)
{
	int                re;
	char             **envp;
	const char        *argv[]       = {"sh", "-c", NULL, NULL};
	int                child        = -1;
	char              *empty_envp[] = {NULL};
	cherokee_buffer_t  tmp          = CHEROKEE_BUF_INIT;

	/* If there is a previous instance running, kill it
	 */
	if (src->pid > 0) {
		kill (src->pid, SIGTERM);
		src->pid = 0;
	}

	/* Maybe set a custom enviroment variable set 
	 */
	envp = (src->custom_env) ? src->custom_env : empty_envp;

	/* Execute the FastCGI server
	 */
	cherokee_buffer_add_va (&tmp, "exec %s", src->interpreter.buf);

	TRACE (ENTRIES, "Spawn \"/bin/sh -c %s\"\n", src->interpreter.buf);

#ifndef _WIN32
	child = fork();
#endif
	switch (child) {
	case 0:
		/* Change user if requested
		 */
		if (! cherokee_buffer_is_empty (&src->change_user_name)) {
			initgroups (src->change_user_name.buf, src->change_user);
		}

		if (src->change_group != -1) {
			setgid (src->change_group);
		}

		if (src->change_user != -1) {
			setuid (src->change_user);
		}

		/* Redirect/Close stderr and stdout
		 */
		if (! src->debug) {
			cherokee_boolean_t        done   = false;
			cherokee_logger_writer_t *writer = NULL;

			if (logger != NULL) {
				cherokee_logger_get_error_writer (logger, &writer);
				if ((writer) && (writer->fd != -1)) {
					dup2 (writer->fd, STDOUT_FILENO);
					dup2 (writer->fd, STDERR_FILENO);		
					done = true;
				}
			} 

			if (! done) {
				close (STDOUT_FILENO);
				close (STDERR_FILENO);
			}			
		}

		argv[2] = (char *)tmp.buf;
		re = execve ("/bin/sh", (char **)argv, envp);
		if (re < 0) {
			PRINT_ERROR ("ERROR: Could spawn %s\n", tmp.buf);
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
				   cherokee_logger_t             *logger)
{
	ret_t ret;

	/* Sanity check
	 */
	if (cherokee_buffer_is_empty (&src->interpreter)) 
		return ret_not_found;

	/* Try to use the spawn mechanism
	 */
#ifdef HAVE_POSIX_SHM
	ret = _spawn_shm (src, logger);
	if (ret == ret_ok) {
		return ret_ok;
	}
#endif

	/* It has failed: do it yourself
	 */
	ret = _spawn_local (src, logger);
	if (ret != ret_ok) {
		return ret;
	}

	return ret_ok;
}


ret_t
cherokee_source_interpreter_connect_polling (cherokee_source_interpreter_t *src, 
					     cherokee_socket_t             *socket,
					     cherokee_connection_t         *conn,
					     time_t                        *spawned)
{
	ret_t ret;

	/* Try to connect
	 */
 	ret = cherokee_source_connect (SOURCE(src), socket); 
	switch (ret) {
	case ret_ok:
		TRACE (ENTRIES, "Connected successfully fd=%d\n", socket->socket);
		goto out;
	case ret_deny:
		/* The connection has been refused:
		 * Close the socket and try again.
		 */
		TRACE (ENTRIES, "Connection refused (closing fd=%d)\n", socket->socket);
		cherokee_socket_close (socket);
		break;
	case ret_eagain:
		ret = cherokee_thread_deactive_to_polling (CONN_THREAD(conn),
							   conn,
							   SOCKET_FD(socket),
							   FDPOLL_MODE_WRITE, 
							   false);
		if (ret != ret_ok) {
			ret = ret_deny;
			goto out;
		}

		/* Leave the mutex locked */
		return ret_eagain;
	case ret_error:
		ret = ret_error;
		goto out;
	default:
		break;
	}

	/* In case it did not success, launch a interpreter
	 */
	if (*spawned == 0) {
		int unlocked;

		/* Launch a new interpreter 
		 */
		unlocked = CHEROKEE_MUTEX_TRY_LOCK(&src->launching_mutex);
		if (unlocked)
			return ret_eagain;

		src->launching = true;

		ret = cherokee_source_interpreter_spawn (src, CONN_VSRV(conn)->logger);
		if (ret != ret_ok) {
			if (src->interpreter.buf)
				TRACE (ENTRIES, "Couldn't spawn: %s\n",
				       src->interpreter.buf);
			else
				TRACE (ENTRIES, "No interpreter to be spawned %s", "\n");

			ret = ret_error;
			goto out;
		}

		*spawned = cherokee_bogonow_now;

		/* Reset the internal socket */
		cherokee_socket_close (socket);

	} else if (cherokee_bogonow_now > *spawned + src->timeout) {	
		TRACE (ENTRIES, "Giving up; spawned %d secs ago: %s\n",
		       src->timeout, src->interpreter.buf);

		ret = ret_error;
		goto out;
	}

	/* Leave the mutex locked */
	return ret_eagain;

out:
	if (src->launching) {
		CHEROKEE_MUTEX_UNLOCK (&src->launching_mutex);
		src->launching = false;
	}

	return ret;
}
