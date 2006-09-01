/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/* Cherokee
 *
 * Authors:
 *      Alvaro Lopez Ortega <alvaro@alobbs.com>
 *
 * Copyright (C) 2001-2006 Alvaro Lopez Ortega
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
#include "ext_source.h"
#include "util.h"
#include "socket.h"

#include <sys/types.h>
#include <unistd.h>

#define ENTRIES "extsrc"


static void
init_server (cherokee_ext_source_t *n)
{
	INIT_LIST_HEAD (LIST(n));
	
	cherokee_buffer_init (&n->interpreter);
	cherokee_buffer_init (&n->host);
	cherokee_buffer_init (&n->unix_socket);
	cherokee_buffer_init (&n->original_server);

	n->port           = -1;
	n->custom_env     = NULL;
	n->custom_env_len = 0;
}

static void
mrproper_server (cherokee_ext_source_t *n)
{
	if (n->custom_env != NULL) {
		cuint_t i = 0;

		while (n->custom_env[i] != NULL) {
			free (n->custom_env[i]);
			i++;
		}

		free (n->custom_env);

		n->custom_env     = NULL;
		n->custom_env_len = 0;
	}

	cherokee_buffer_mrproper (&n->interpreter);
	cherokee_buffer_mrproper (&n->host);
	cherokee_buffer_mrproper (&n->unix_socket);
	cherokee_buffer_mrproper (&n->original_server);
}

static void
server_free (cherokee_ext_source_t *n)
{
	mrproper_server (n);
	free (n);
}


static void
server_head_free (cherokee_ext_source_head_t *n)
{
	mrproper_server (EXT_SOURCE(n));
	CHEROKEE_MUTEX_DESTROY (&n->current_server_lock);

	free (n);
}


ret_t 
cherokee_ext_source_new (cherokee_ext_source_t **server)
{
	CHEROKEE_NEW_STRUCT (n, ext_source);
	
	init_server (n);
//	n->free_func = (cherokee_typed_free_func_t) server_free;
	
	*server = n;
	return ret_ok;
}


ret_t 
cherokee_ext_source_head_new (cherokee_ext_source_head_t **server)
{
	CHEROKEE_NEW_STRUCT (n, ext_source_head);
	
	init_server (EXT_SOURCE(n));
//	EXT_SOURCE(n)->free_func = (cherokee_typed_free_func_t) server_head_free;
	
	n->current_server = EXT_SOURCE(n);
	CHEROKEE_MUTEX_INIT (&n->current_server_lock, NULL);
	
	*server = n;
	return ret_ok;
}


ret_t 
cherokee_ext_source_free (cherokee_ext_source_t *server)
{
//	if (server->free_func == NULL)
		return ret_error;

//	server->free_func(server);
	return ret_ok;
}


ret_t 
cherokee_ext_source_add_env (cherokee_ext_source_t *server, char *env, char *val)
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
	if (server->custom_env_len == 0) {
		server->custom_env = malloc (sizeof (char *) * 2);
	} else {
		server->custom_env = realloc (server->custom_env, (server->custom_env_len + 2) * sizeof (char *));
	}
	server->custom_env_len +=  1;

	server->custom_env[server->custom_env_len - 1] = entry;
	server->custom_env[server->custom_env_len] = NULL;

	return ret_ok;
}


ret_t 
cherokee_ext_source_connect (cherokee_ext_source_t *src, cherokee_socket_t *socket)
{
	ret_t ret;

	/* UNIX socket
	 */
	if (! cherokee_buffer_is_empty (&src->unix_socket)) {
		ret = cherokee_socket_set_client (socket, AF_UNIX);
		if (ret != ret_ok) return ret;
		
		ret = cherokee_socket_gethostbyname (socket, &src->unix_socket);
		if (ret != ret_ok) return ret;

		return cherokee_socket_connect (socket);
	}

	/* INET socket
	 */
	ret = cherokee_socket_set_client (socket, AF_INET);
	if (ret != ret_ok) return ret;
	
	ret = cherokee_socket_gethostbyname (socket, &src->host);
	if (ret != ret_ok) return ret;
	
	SOCKET_SIN_PORT(socket) = htons(src->port);
 	
	return cherokee_socket_connect (socket);
}


ret_t 
cherokee_ext_source_get_next (cherokee_ext_source_head_t *head_config, cherokee_list_t *server_list, cherokee_ext_source_t **next)
{
	cherokee_ext_source_t *current_config;

	CHEROKEE_MUTEX_LOCK (&head_config->current_server_lock);

	/* Set the next server
	 */
	current_config              = head_config->current_server;
	head_config->current_server = EXT_SOURCE(LIST(current_config)->next);

	/* This is a special case: if the next is the base of the list, we have to
	 * skip the entry and point to the next one
	 */
	if (LIST(head_config->current_server) == server_list) {
		current_config = head_config->current_server;
//		head_config->current_server = EXT_SOURCE((LIST(current_config)->next);
	}		

	*next = head_config->current_server;

	CHEROKEE_MUTEX_UNLOCK (&head_config->current_server_lock);
	return ret_ok;
}


ret_t 
cherokee_ext_source_spawn_srv (cherokee_ext_source_t *server)
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
	 * it is centainly a FastCGI.  The fcgi client will execute
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

	re = listen(s, 1024);
	if (re == -1) return ret_error;
#endif

	/* Maybe set a custom enviroment variable set 
	 */
	envp = (server->custom_env) ? server->custom_env : empty_envp;

	/* Execute the FastCGI server
	 */
	cherokee_buffer_add_va (&tmp, "exec %s", server->interpreter.buf);

	TRACE (ENTRIES, "Spawn \"/bin/sh %s\"\n", server->interpreter.buf);

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
		argv[2] = (char *)tmp.buf;

		re = execve ("/bin/sh", argv, envp);
		if (re < 0) {
			PRINT_ERROR ("ERROR: Could spawn %s\n", tmp.buf);
			exit (1);
		}

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


static ret_t
split_address_or_path (char *str, cherokee_buffer_t *hostname, cint_t *port_num, 
				   cherokee_buffer_t *unix_socket, cherokee_buffer_t *original)
{
	char    *p;
	cuint_t  len;
	
	len = strlen(str);
	if (len <= 0) return ret_error;
	
	/* Original
	 */
	cherokee_buffer_add (original, str, len);
	
	/* Unix socket
	 */
	if (str[0] == '/') {
		cherokee_buffer_add (unix_socket, str, len);
		return ret_ok;
	} 
	
	/* Host name
	 */
	p = strchr(str, ':');
	if (p == NULL) {
		cherokee_buffer_add (hostname, str, len);
		return ret_ok;
	} 
	
	/* Host name + port
	 */
	*p = '\0';
	*port_num = atoi (p+1);
	cherokee_buffer_add (hostname, str, p - str);
	*p = ':';
	
	return ret_ok;
}


ret_t 
cherokee_ext_source_configure (cherokee_config_node_t *conf, cherokee_list_t *ext_list)
{
	ret_t                       ret;
	cherokee_config_node_t     *child;
	cherokee_config_node_t     *child2;
	cherokee_config_node_t     *child3;
	cherokee_list_t            *i, *j, *k;
	cherokee_list_t             nlist        = LIST_HEAD_INIT(nlist);
	cherokee_ext_source_t      *server_entry = NULL;
	cherokee_ext_source_head_t *head         = NULL;

	TRACE (ENTRIES, "Configuring '%s'\n", conf->key.buf);

	cherokee_config_node_foreach (i, conf) {
		child = CONFIG_NODE(i);

		/* Instance a new entry
		 */
		if (cherokee_list_empty (ext_list)) {
			ret = cherokee_ext_source_head_new (&head);
			if (ret != ret_ok) return ret;

			server_entry = EXT_SOURCE(head);
		} else {
			ret = cherokee_ext_source_new (&server_entry);
			if (ret != ret_ok) return ret;
		}
		
		/* Add the entry to the list
		 */
		cherokee_list_add_tail (LIST(server_entry), ext_list);

		/* Parse properties
		 */
		cherokee_config_node_foreach (j, child) {
			child2 = CONFIG_NODE(j);

			if (equal_buf_str (&child2->key, "host")) {
				split_address_or_path (child2->val.buf, &server_entry->host, &server_entry->port,
						       &server_entry->unix_socket, &server_entry->original_server);
				
			} else if (equal_buf_str (&child2->key, "env")) {
				cherokee_config_node_foreach (k, child2) {
					child3 = CONFIG_NODE(k);
					
					ret = cherokee_ext_source_add_env (server_entry, child3->key.buf, child3->val.buf);
					if (ret != ret_ok) return ret;
				}

			} else if (equal_buf_str (&child2->key, "interpreter")) {
				// fix win32 path
				cherokee_buffer_add_buffer (&server_entry->interpreter, &child2->val);

			} else {
				PRINT_MSG ("ERROR: Handler ext_source: Unknown key: '%s'\n", child2->key.buf);
				return ret_error;
			}
		}
	}

	return ret_ok;
}
