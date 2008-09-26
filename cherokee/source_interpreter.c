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
#include "source_interpreter.h"
#include "util.h"
#include "connection-protected.h"
#include "thread.h"
#include "bogotime.h"

#include <sys/types.h>
#include <unistd.h>


#define ENTRIES "source,src,interpreter"

static void interpreter_free (void *src);


ret_t 
cherokee_source_interpreter_new  (cherokee_source_interpreter_t **src)
{
	CHEROKEE_NEW_STRUCT(n, source_interpreter);

	cherokee_source_init (SOURCE(n));
	cherokee_buffer_init (&n->interpreter);

	n->custom_env     = NULL;
	n->custom_env_len = 0;
	n->debug          = false;

	SOURCE(n)->type   = source_interpreter;
	SOURCE(n)->free   = (cherokee_func_free_t)interpreter_free;
	
	CHEROKEE_MUTEX_INIT (&n->launching_mutex, NULL);
	n->launching = false;

	*src = n;
	return ret_ok;
}


static void
free_custon_env (void *ptr)
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
	cherokee_buffer_mrproper (&src->interpreter);

	if (src->custom_env)
		free_custon_env (src);

	CHEROKEE_MUTEX_DESTROY (&src->launching_mutex);
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
	if (ret != ret_ok) return ret;

	/* Interpreter parameters
	 */
	cherokee_config_node_foreach (i, conf) {
		child = CONFIG_NODE(i);

		if (equal_buf_str (&child->key, "interpreter")) {
			/* TODO: fix win32 path */
			cherokee_buffer_add_buffer (&src->interpreter, &child->val);

		} else if (equal_buf_str (&child->key, "debug")) {
			src->debug = !! atoi (child->val.buf);

		} else if (equal_buf_str (&child->key, "env")) {			
			cherokee_config_node_foreach (j, child) {
				cherokee_config_node_t *child2 = CONFIG_NODE(j);
                                
				ret = cherokee_source_interpreter_add_env (src, child2->key.buf, child2->val.buf);
				if (ret != ret_ok) return ret;
			}
		}	
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


ret_t 
cherokee_source_interpreter_spawn (cherokee_source_interpreter_t *src)
{
	int                re;
	char             **envp;
	char              *argv[]       = {"sh", "-c", NULL, NULL};
	int                child        = -1;
	char              *empty_envp[] = {NULL};
	cherokee_buffer_t  tmp          = CHEROKEE_BUF_INIT;

#if 0
	int s;
	cherokee_sockaddr_t addr;

	/* This code is meant to, in some way, signal the FastCGI that
	 * it is certainly a FastCGI.  The fcgi client will execute
	 * getpeername (FCGI_LISTENSOCK_FILENO) and, then if it is a
	 * fcgi, error will have the ENOTCONN value.
	 */
	addr.sa_in.sin_addr.s_addr = htonl(INADDR_ANY);

	s = socket (AF_INET, SOCK_STREAM, 0);
	if (s < 0) return ret_error;
	
	re = 1;
	setsockopt (s, SOL_SOCKET, SO_REUSEADDR, &re, sizeof(re));

	re = bind (s, (struct sockaddr *) &addr, sizeof(cherokee_sockaddr_t));
	if (re == -1) return ret_error;

	re = listen (s, 1024);
	if (re == -1) return ret_error;
#endif

	/* Sanity check
	 */
	if (cherokee_buffer_is_empty (&src->interpreter)) 
		return ret_not_found;

	/* Maybe set a custom enviroment variable set 
	 */
	envp = (src->custom_env) ? src->custom_env : empty_envp;

	/* Execute the FastCGI server
	 */
	cherokee_buffer_add_va (&tmp, "exec %s", src->interpreter.buf);

	TRACE (ENTRIES, "Spawn \"/bin/sh %s\"\n", src->interpreter.buf);

#ifndef _WIN32
	child = fork();
#endif
	switch (child) {
	case 0:
#if 0 
		/* More FCGI_LISTENSOCK_FILENO stuff..
		 */
		close (STDIN_FILENO);

		if (s != FCGI_LISTENSOCK_FILENO) {
			close (FCGI_LISTENSOCK_FILENO);
			dup2 (s, FCGI_LISTENSOCK_FILENO);
			close (s);
		}
		
		close (STDOUT_FILENO);
		close (STDERR_FILENO);
#endif

		/* Doesn't care about it's output either.  It can fill
		 * out the system buffers and free the interpreter.
		 */
		if (! src->debug) {
			close (STDOUT_FILENO);
			close (STDERR_FILENO);
		}

		argv[2] = (char *)tmp.buf;
		re = execve ("/bin/sh", argv, envp);
		if (re < 0) {
			PRINT_ERROR ("ERROR: Could spawn %s\n", tmp.buf);
			exit (1);
		}

		exit ((re == 0) ? 0 : 1);
	case -1:
		goto error;
		
	default:
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

		ret = cherokee_source_interpreter_spawn (src);
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

	} else if (cherokee_bogonow_now > *spawned + 3) {	
		TRACE (ENTRIES, "Giving up; spawned 3 secs ago: %s\n",
		       src->interpreter.buf);

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
