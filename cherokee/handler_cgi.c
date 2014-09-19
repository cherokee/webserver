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
#include "handler_cgi.h"
#include "util.h"

#ifdef HAVE_STDARG_H
# include <stdarg.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <ctype.h>
#include <fcntl.h>
#include <signal.h>

#ifdef HAVE_SYS_WAIT_H
# include <sys/wait.h>
#endif

#include "module.h"
#include "connection.h"
#include "connection-protected.h"
#include "socket.h"
#include "server.h"
#include "server-protected.h"
#include "header.h"
#include "header-protected.h"
#include "post.h"
#include "error_log.h"
#include "services.h"

#define ENTRIES "handler,cgi"

# define fork_and_execute_cgi(c) fork_and_execute_cgi_unix(c)
static ret_t fork_and_execute_cgi_unix (cherokee_handler_cgi_t *cgi);

#define set_env(cgi,k,v,vl) cherokee_handler_cgi_add_env_pair(cgi, k, sizeof(k)-1, v, vl)

static ret_t fork_and_execute_cgi_via_spawner(cherokee_handler_cgi_t *cgi);


/* Plugin initialization
 */
CGI_LIB_INIT (cgi, http_get | http_post | http_head);



static ret_t
read_from_cgi (cherokee_handler_cgi_base_t *cgi_base, cherokee_buffer_t *buffer)
{
	ret_t                   ret;
	size_t                  read_ = 0;
	cherokee_handler_cgi_t *cgi   = HDL_CGI(cgi_base);

	/* Sanity check: pipe() accessed
	 */
	if (unlikely (cgi->pipeInput < 0))
		return ret_eof;

	/* Read the data from the pipe:
	 */
	ret = cherokee_buffer_read_from_fd (buffer, cgi->pipeInput, 4096, &read_);

	TRACE (ENTRIES, "read... ret=%d %d\n", ret, read_);

	switch (ret) {
	case ret_eagain:
		cherokee_thread_deactive_to_polling (HANDLER_THREAD(cgi),
		                                     HANDLER_CONN(cgi), cgi->pipeInput,
		                                     FDPOLL_MODE_READ, false);
		return ret_eagain;

	case ret_ok:
		TRACE (ENTRIES, "%d bytes read\n", read_);
		return ret_ok;

	case ret_eof:
	case ret_error:
		cgi_base->got_eof = true;
		return ret;

	default:
		RET_UNKNOWN(ret);
	}

	SHOULDNT_HAPPEN;
	return ret_error;
}


ret_t
cherokee_handler_cgi_new (cherokee_handler_t **hdl, void *cnt, cherokee_module_props_t *props)
{
	int i;
	CHEROKEE_NEW_STRUCT (n, handler_cgi);

	/* Init the base class
	 */
	cherokee_handler_cgi_base_init (HDL_CGI_BASE(n), cnt, PLUGIN_INFO_HANDLER_PTR(cgi),
	                                HANDLER_PROPS(props), cherokee_handler_cgi_add_env_pair, read_from_cgi);

	/* Virtual methods
	 */
	MODULE(n)->init         = (module_func_init_t) cherokee_handler_cgi_init;
	MODULE(n)->free         = (module_func_free_t) cherokee_handler_cgi_free;
	HANDLER(n)->read_post   = (handler_func_read_post_t) cherokee_handler_cgi_read_post;

	/* Virtual methods: implemented by handler_cgi_base
	 */
	HANDLER(n)->add_headers = (handler_func_add_headers_t) cherokee_handler_cgi_base_add_headers;
	HANDLER(n)->step        = (handler_func_step_t) cherokee_handler_cgi_base_step;

	/* Init
	 */
	n->pipeInput  = -1;
	n->pipeOutput = -1;
	n->pid       = -1;
	n->envp_last =  0;

	for (i=0; i<ENV_VAR_NUM; i++)
		n->envp[i] = NULL;

	/* Return the object
	 */
	*hdl = HANDLER(n);
	return ret_ok;
}


static int
do_reap (void)
{
	pid_t pid;
	int   status;
	int   child_count = 0;

	/* Reap defunct children until there aren't any more.
	 */
	for (child_count = 0; ; ++child_count) {

		pid = waitpid (-1, &status, WNOHANG);

		/* none left */
		if (pid == 0)
			break;
		else
		if (pid < 0) {
			/* because of ptrace */
			if (errno == EINTR)
				continue;
			break;
		}
	}

	return child_count;
}


ret_t
cherokee_handler_cgi_free (cherokee_handler_cgi_t *cgi)
{
	int i;

	/* Free the rest of the handler CGI memory
	 */
	cherokee_handler_cgi_base_free (HDL_CGI_BASE(cgi));

	/* Close the connection with the CGI
	 */
	if (cgi->pipeInput > 0) {
		cherokee_fd_close (cgi->pipeInput);
		cgi->pipeInput = -1;
	}

	if (cgi->pipeOutput > 0) {
		cherokee_fd_close (cgi->pipeOutput);
		cgi->pipeOutput = -1;
	}

	/* Kill the CGI
	 */
	if (cgi->pid > 0) {
		pid_t  pid;
		cint_t tries = 2;

		while (true) {
			do {
				pid = waitpid (cgi->pid, NULL, WNOHANG);
			} while ((pid == 1) && (errno == EINTR));

			if (pid > 0) {
				/* Ok */
				break;
			} else if (errno == ECHILD) {
				/* Already death */
				break;
			}

			/* Failed */
			kill (cgi->pid, SIGTERM);

			tries--;
			if (tries < 0)
				break;
		}
	}

	/* Free the environment variables
	 */
	for (i=0; i<cgi->envp_last; i++) {
		free (cgi->envp[i]);
		cgi->envp[i] = NULL;
	}

	/* For some reason, we have seen that the SIGCHLD signal does not call to
	 * our handler in a server with a lot of requests, so the wait() call,
	 * necessary to free the resources used by the CGI, is not called. So I
	 * think that a possible solution couble be to put the waitpid call in the
	 * _free method of this handler, so when the handler ends, this will free
	 * the resources used by our cool CGI.
	 */
	do_reap();

	return ret_ok;
}


ret_t
cherokee_handler_cgi_props_free (cherokee_handler_cgi_props_t *props)
{
	return cherokee_handler_cgi_base_props_free (PROP_CGI_BASE(props));
}


ret_t
cherokee_handler_cgi_configure (cherokee_config_node_t *conf, cherokee_server_t *srv, cherokee_module_props_t **_props)
{
	/* Instance a new property object
	 */
	if (*_props == NULL) {
		CHEROKEE_NEW_STRUCT (n, handler_cgi_props);

		cherokee_handler_cgi_base_props_init_base (PROP_CGI_BASE(n),
			MODULE_PROPS_FREE(cherokee_handler_cgi_props_free));
		*_props = MODULE_PROPS(n);
	}

	/* Parse local options
	 */
	return cherokee_handler_cgi_base_configure (conf, srv, _props);
}


void
cherokee_handler_cgi_add_env_pair (cherokee_handler_cgi_base_t *cgi_base,
                                   const char *name,    int name_len,
                                   const char *content, int content_len)
{
	cherokee_handler_cgi_t *cgi = HDL_CGI(cgi_base);

	char *entry;

	/* Build the new envp entry
	 */
	if (name == NULL)
		return;

	entry = (char *) malloc (name_len + content_len + 2);
	if (entry == NULL)
		return;

	memcpy (entry, name, name_len);
	entry[name_len] = '=';

	memcpy (entry + name_len + 1, content, content_len);

	entry[name_len+content_len+1] = '\0';

	/* Set it in the table
	 */
	cgi->envp[cgi->envp_last] = entry;
	cgi->envp_last++;

	/* Sanity check
	 */
	if (cgi->envp_last >= ENV_VAR_NUM) {
		SHOULDNT_HAPPEN;
	}
}

static ret_t
add_environment (cherokee_handler_cgi_t *cgi,
                 cherokee_connection_t  *conn)
{
	ret_t                        ret;
	cherokee_handler_cgi_base_t *cgi_base = HDL_CGI_BASE(cgi);
	cherokee_buffer_t           *tmp      = THREAD_TMP_BUF2(CONN_THREAD(conn));

	ret = cherokee_handler_cgi_base_build_envp (HDL_CGI_BASE(cgi), conn);
	if (unlikely (ret != ret_ok))
		return ret;

	/* CONTENT_LENGTH
	 */
	if (conn->post.has_info) {
		cherokee_buffer_clean (tmp);
		cherokee_buffer_add_ullong10 (tmp, conn->post.len);
		set_env (cgi_base, "CONTENT_LENGTH", tmp->buf, tmp->len);
	}

	/* SCRIPT_FILENAME
	 */
	if (cgi_base->executable.len <= 0)
		return ret_error;

	set_env (cgi_base, "SCRIPT_FILENAME",
		 cgi_base->executable.buf,
		 cgi_base->executable.len);

	return ret_ok;
}


ret_t
cherokee_handler_cgi_init (cherokee_handler_cgi_t *cgi)
{
	ret_t                        ret;
	cherokee_handler_cgi_base_t *cgi_base = HDL_CGI_BASE(cgi);
	cherokee_connection_t       *conn     = HANDLER_CONN(cgi);

	switch (cgi_base->init_phase) {
	case hcgi_phase_build_headers:

		/* Extracts PATH_INFO and filename from request uri
		 */
		if (cherokee_buffer_is_empty (&cgi_base->executable)) {
			ret = cherokee_handler_cgi_base_extract_path (cgi_base, true);
			if (unlikely (ret < ret_ok)) {
				return ret;
			}
		}

		/* It has to update the timeout of the connection,
		 * otherwhise the server will drop it for the CGI
		 * isn't fast enough
		 */
		conn->timeout = cherokee_bogonow_now + CGI_TIMEOUT;
		cgi_base->init_phase = hcgi_phase_connect;

	case hcgi_phase_connect:
		/* Launch the CGI
		 */
		ret = fork_and_execute_cgi_via_spawner(cgi);
		if (unlikely (ret != ret_ok)) {
			ret = fork_and_execute_cgi(cgi);
			if (unlikely (ret != ret_ok)) {
				return ret;
			}
		}

	default:
		break;
	}

	TRACE (ENTRIES, "finishing %s\n", "ret_ok");
	return ret_ok;
}


ret_t
cherokee_handler_cgi_read_post (cherokee_handler_cgi_t *cgi)
{
	ret_t                     ret;
	cherokee_connection_t    *conn     = HANDLER_CONN(cgi);
	cherokee_socket_status_t  blocking = socket_closed;
	cherokee_boolean_t        did_IO   = false;

	if (! conn->post.has_info) {
		return ret_ok;
	}

	ret = cherokee_post_send_to_fd (&conn->post, &conn->socket,
	                                cgi->pipeOutput, NULL, &blocking, &did_IO);

	if (did_IO) {
		cherokee_connection_update_timeout (conn);
	}

	switch (ret) {
	case ret_ok:
		break;
	case ret_eagain:
		if (blocking == socket_writing) {
			cherokee_thread_deactive_to_polling (HANDLER_THREAD(cgi),
			                                     conn, cgi->pipeOutput,
			                                     FDPOLL_MODE_WRITE, false);
			return ret_deny;
		}

		/* ret_eagain - Block on read
		 * ret_deny   - Block on back-end write
		 */
		if (cherokee_post_has_buffered_info (&conn->post)) {
			return ret_deny;
		}
		return ret_eagain;

	default:
		return ret;
	}

	TRACE (ENTRIES",post", "%s\n", "finished");
	cherokee_fd_close (cgi->pipeOutput);
	cgi->pipeOutput = -1;

	return ret_ok;
}


static ret_t
_fd_set_properties (int fd, int add_flags, int remove_flags)
{
	int flags;

	flags = fcntl (fd, F_GETFL, 0);

	flags |= add_flags;
	flags &= ~remove_flags;

	if (fcntl (fd, F_SETFL, flags) == -1) {
		LOG_ERRNO (errno, cherokee_err_error,
		           CHEROKEE_ERROR_HANDLER_CGI_SET_PROP, fd);
		return ret_error;
	}

	return ret_ok;
}


static NORETURN void
manage_child_cgi_process (cherokee_handler_cgi_t *cgi, int pipe_cgi[2], int pipe_server[2])
{
	/* Child process
	 */
	int                          re;
	cherokee_connection_t       *conn          = HANDLER_CONN(cgi);
	cherokee_handler_cgi_base_t *cgi_base      = HDL_CGI_BASE(cgi);
	char                        *absolute_path = cgi_base->executable.buf;
	char                        *argv[2]       = { NULL, NULL };

#ifdef TRACE_ENABLED
	TRACE(ENTRIES, "About to execute: '%s'\n", absolute_path);

	if (! cherokee_buffer_is_empty (&conn->effective_directory))
		TRACE(ENTRIES, "Effective directory: '%s'\n", conn->effective_directory.buf);
	else
		TRACE(ENTRIES, "No Effective directory %s", "\n");
#endif

	/* Close useless sides
	 */
	cherokee_fd_close (pipe_cgi[0]);
	cherokee_fd_close (pipe_server[1]);

	/* Change stdin and out
	 */
	cherokee_fd_close (STDIN_FILENO);
	re = dup2 (pipe_server[0], STDIN_FILENO);
	cherokee_fd_close (pipe_server[0]);

	if (unlikely (re != 0)) {
		printf ("Status: 500" CRLF_CRLF);
		printf ("X-Debug: file=%s line=%d" CRLF_CRLF, __FILE__, __LINE__);
		exit(1);
	}

	cherokee_fd_close (STDOUT_FILENO);
	re |= dup2 (pipe_cgi[1], STDOUT_FILENO);
	cherokee_fd_close (pipe_cgi[1]);

	/* Redirect the stderr
	 */
	if ((CONN_VSRV(conn)->error_writer != NULL) &&
	    (CONN_VSRV(conn)->error_writer->fd != -1))
	{
		cherokee_fd_close (STDERR_FILENO);
		dup2 (CONN_VSRV(conn)->error_writer->fd, STDERR_FILENO);
	}

# if 0
	/* Set unbuffered
	 */
	setvbuf (stdin,  NULL, _IONBF, 0);
	setvbuf (stdout, NULL, _IONBF, 0);
# endif

	/* Enable blocking mode
	 */
	_fd_set_properties (STDIN_FILENO,  0, O_NONBLOCK);
	_fd_set_properties (STDOUT_FILENO, 0, O_NONBLOCK);
	_fd_set_properties (STDERR_FILENO, 0, O_NONBLOCK);

	/* Sets the new environ.
	 */
	add_environment (cgi, conn);

	/* Change the directory
	 */
	if (! cherokee_buffer_is_empty (&conn->effective_directory)) {
		re = chdir (conn->effective_directory.buf);
	} else {
		char *file = strrchr (absolute_path, '/');

		if (file != NULL) {
			*file = '\0';
			re = chdir (absolute_path);
			*file = '/';
		} else {
			re = -1;
		}
	}

	if (re < 0) {
		printf ("Status: 500" CRLF_CRLF);
		printf ("X-Debug: file=%s line=%d" CRLF_CRLF, __FILE__, __LINE__);
		exit(1);
	}

	/* Build de argv array
	 */
	argv[0] = absolute_path;

	/* Change the execution user?
	 */
	if (HANDLER_CGI_PROPS(cgi_base)->change_user) {
		struct stat                        nocache_info;
		struct stat                       *info;
		cherokee_iocache_entry_t          *io_entry = NULL;
		cherokee_server_t                 *srv      = CONN_SRV(conn);
		cherokee_handler_cgi_base_props_t *props    = HANDLER_CGI_BASE_PROPS(cgi);

		ret_t ret = cherokee_io_stat (srv->iocache, &cgi_base->executable, props->use_cache, &nocache_info, &io_entry, &info);
		if (ret != ret_ok) {
			info = &nocache_info;
		}

		re = setuid (info->st_uid);
		if (re != 0) {
			LOG_ERROR (CHEROKEE_ERROR_HANDLER_CGI_SETID, absolute_path, info->st_uid);
		}

		cherokee_iocache_entry_unref(&io_entry);
	}

	/* Reset the server-wide signal handlers
	 */
	cherokee_reset_signals();

	/* Lets go.. execute it!
	 */
	do {
		re = execve (absolute_path, argv, cgi->envp);
	} while ((re == -1) && (errno == EINTR));

	if (re < 0) {
		int err = errno;
		char buferr[ERROR_MAX_BUFSIZE];

		switch (err) {
		case ENODEV:
		case ENOTDIR:
		case ENOENT:
			printf ("Status: 404" CRLF_CRLF);
			exit(0);
		case EPERM:
		case EACCES:
		case ENOEXEC:
			printf ("Status: 403" CRLF_CRLF);
			exit(0);
		default:
			printf ("Status: 500" CRLF_CRLF);
			printf ("X-Debug: file=%s line=%d cmd=%s errno=%d: %s" CRLF_CRLF,
			        __FILE__, __LINE__, absolute_path, err, strerror(err));
		}

		/* Don't use the logging system (concurrency issues)
		 */
		LOG_ERROR (CHEROKEE_ERROR_HANDLER_CGI_EXECUTE,
		           absolute_path, cherokee_strerror_r(err, buferr, sizeof(buferr)));
		exit(1);
	}

	/* There is no way, it could reach this point.
	 */
	SHOULDNT_HAPPEN;
	exit(2);
}

static ret_t
fork_and_execute_cgi_unix (cherokee_handler_cgi_t *cgi)
{
	int                    re;
	int                    pid;
	cherokee_connection_t *conn = HANDLER_CONN(cgi);

	struct {
		int cgi[2];
		int server[2];
	} pipes;

	/* Creates the pipes ...
	 */
	re  = cherokee_pipe (pipes.cgi);
	re |= cherokee_pipe (pipes.server);

	if (re != 0) {
		conn->error_code = http_internal_error;
		return ret_error;
	}

	/* .. and fork the process
	 */
	pid = fork();
	if (pid == 0) {
		/* CGI process
		 */
		manage_child_cgi_process (cgi, pipes.cgi, pipes.server);

	} else if (pid < 0) {
		/* Error
		 */
		cherokee_fd_close (pipes.cgi[0]);
		cherokee_fd_close (pipes.cgi[1]);

		cherokee_fd_close (pipes.server[0]);
		cherokee_fd_close (pipes.server[1]);

		conn->error_code = http_internal_error;
		return ret_error;
	}

	TRACE (ENTRIES, "pid %d\n", pid);

	cherokee_fd_close (pipes.server[0]);
	cherokee_fd_close (pipes.cgi[1]);

	cgi->pid        = pid;
	cgi->pipeInput  = pipes.cgi[0];
	cgi->pipeOutput = pipes.server[1];

	/* Set to Input to NON-BLOCKING
	 */
	_fd_set_properties (cgi->pipeInput, O_NDELAY|O_NONBLOCK, 0);

	return ret_ok;
}


static ret_t
fork_and_execute_cgi_via_spawner(cherokee_handler_cgi_t *cgi)
{
	int                          re;
	int                          pid           = -1;
	cherokee_connection_t       *conn          = HANDLER_CONN(cgi);
	cherokee_handler_cgi_base_t *cgi_base      = HDL_CGI_BASE(cgi);
	ret_t                        ret;
	uid_t                        uid;
	gid_t                        gid;
	cherokee_buffer_t            empty         = CHEROKEE_BUF_INIT;
	cherokee_buffer_t            username      = CHEROKEE_BUF_INIT;
	cherokee_services_fdmap_t    fd_map;
	cherokee_buffer_t           *chdir         = NULL;
	cherokee_buffer_t            chdir_backing = CHEROKEE_BUF_INIT;
	struct passwd                ent;
	char                         ent_tmp[1024];

	struct {
		int cgi[2];
		int server[2];
	} pipes;

	TRACE (ENTRIES, "Trying to create CGI via spawner\n");

	if (! cherokee_buffer_is_empty (&conn->effective_directory)) {
		chdir = &conn->effective_directory;
	} else {
		int ofs = cgi_base->executable.len - 1;
		while (ofs >= 0 && cgi_base->executable.buf[ofs] != '/') {
			ofs--;
		}
		TRACE (ENTRIES, "While building chdir, ofs=%d\n", ofs);
		if (ofs < 0 || cherokee_buffer_add (&chdir_backing,
						    cgi_base->executable.buf,
						    ofs + 1) != ret_ok) {
			conn->error_code = http_internal_error;
			TRACE (ENTRIES, "Failed, cannot build chdir entry\n");
			cherokee_buffer_mrproper(&chdir_backing);
			return ret_error;
		}
		chdir = &chdir_backing;
	}

	/* Creates the pipes ...
	 */
	re  = cherokee_pipe (pipes.cgi);
	re |= cherokee_pipe (pipes.server);

	if (re != 0) {
		conn->error_code = http_internal_error;
		cherokee_buffer_mrproper(&chdir_backing);
		TRACE (ENTRIES, "Failed, cannot build pipes\n");
		return ret_error;
	}

	if (HANDLER_CGI_PROPS(cgi_base)->change_user) {
		struct stat                        nocache_info;
		struct stat                       *info;
		cherokee_iocache_entry_t          *io_entry = NULL;
		cherokee_server_t                 *srv      = CONN_SRV(conn);
		cherokee_handler_cgi_base_props_t *props    = HANDLER_CGI_BASE_PROPS(cgi);

		ret = cherokee_io_stat (srv->iocache, &cgi_base->executable, props->use_cache, &nocache_info, &io_entry, &info);
		if (ret != ret_ok) {
			info = &nocache_info;
			nocache_info.st_uid = getuid();
			nocache_info.st_gid = getgid();
		}

		uid = info->st_uid;
		gid = info->st_gid;

		cherokee_iocache_entry_unref(&io_entry);
	} else {
		/* Not changing, so launch as the same uid/gid as the worker */
		uid = getuid();
		gid = getgid();
	}

	/* Determine the username of the owner of the file */
	ret = cherokee_getpwuid(uid, &ent, ent_tmp, sizeof (ent_tmp));
	if (ret != ret_ok ||
	    cherokee_buffer_add(&username,
				ent.pw_name,
				strlen(ent.pw_name) != ret_ok)) {
		cherokee_fd_close(pipes.cgi[0]);
		cherokee_fd_close(pipes.cgi[1]);
		cherokee_fd_close(pipes.server[0]);
		cherokee_fd_close(pipes.server[1]);
		conn->error_code = http_internal_error;
		cherokee_buffer_mrproper(&chdir_backing);
		cherokee_buffer_mrproper(&username);
		TRACE (ENTRIES, "Failed, Unable to retrieve username for uid %d\n", uid);
		return ret_error;
	}


	/* Update the environment ready to run */
	add_environment (cgi, conn);

	/* Set up the FD map */
	fd_map.fd_in = pipes.server[0];
	fd_map.fd_out = pipes.cgi[1];

	if ((CONN_VSRV(conn)->error_writer != NULL) &&
	    (CONN_VSRV(conn)->error_writer->fd != -1)) {
		fd_map.fd_err = CONN_VSRV(conn)->error_writer->fd;
	} else {
		fd_map.fd_err = fd_map.fd_out;
	}


	TRACE (ENTRIES, "Doing Spawn\n");
	ret = cherokee_services_client_spawn (&cgi_base->executable,
					      &username,
					      uid,
					      gid,
					      &empty,
					      chdir,
					      false,
					      cgi->envp,
					      CONN_VSRV(conn)->error_writer,
					      &pid,
					      &fd_map);

	cherokee_buffer_mrproper(&chdir_backing);
	cherokee_buffer_mrproper(&username);

	/* Close the client FDs */
	cherokee_fd_close (pipes.server[0]);
	cherokee_fd_close (pipes.cgi[1]);

	/* Did we fail to try to spawn? */
	if (ret != ret_ok) {
		/* Close the server FDs too */
		cherokee_fd_close (pipes.server[1]);
		cherokee_fd_close (pipes.cgi[0]);
		TRACE (ENTRIES, "Failed to spawn\n");
		return ret;
	}

	/* Did we try, but fail, to spawn? */
	if (pid == -1) {
		/* Close the server FDs too */
		cherokee_fd_close (pipes.server[1]);
		cherokee_fd_close (pipes.cgi[0]);
		TRACE (ENTRIES, "Spawned, but failed server side\n");
		return ret_error;
	}

	/* Successfully launched */
	cgi->pid        = pid;
	cgi->pipeInput  = pipes.cgi[0];
	cgi->pipeOutput = pipes.server[1];

	/* Set to Input to NON-BLOCKING
	 */
	_fd_set_properties (cgi->pipeInput, O_NDELAY|O_NONBLOCK, 0);

	TRACE (ENTRIES, "CGI running, PID=%d\n", pid);
	return ret_ok;
}
