/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/* Cherokee
 *
 * Authors:
 *      Alvaro Lopez Ortega <alvaro@alobbs.com>
 *      Ayose Cazorla León <setepo@gulic.org>
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
#include "list_ext.h"
#include "post.h"


cherokee_module_info_handler_t MODULE_INFO(cgi) = {
	.module.type     = cherokee_handler,                /* type         */
	.module.new_func = cherokee_handler_cgi_new,        /* new func     */
	.valid_methods   = http_get | http_post | http_head /* http methods */
};


#define ENTRIES "handler,cgi"

#ifdef _WIN32
# define fork_and_execute_cgi(c) fork_and_execute_cgi_win32(c)
static ret_t fork_and_execute_cgi_win32 (cherokee_handler_cgi_t *cgi);
#else
# define fork_and_execute_cgi(c) fork_and_execute_cgi_unix(c)
static ret_t fork_and_execute_cgi_unix (cherokee_handler_cgi_t *cgi);
#endif

#define set_env(cgi,k,v,vl) cherokee_handler_cgi_add_env_pair(cgi, k, sizeof(k)-1, v, vl)


static ret_t
read_from_cgi (cherokee_handler_cgi_base_t *cgi_base, cherokee_buffer_t *buffer)
{
	ret_t                   ret;
 	size_t                  readed = 0;
	cherokee_handler_cgi_t *cgi    = HANDLER_CGI(cgi_base);

	/* Read the data from the pipe:
	 */
	ret = cherokee_buffer_read_from_fd (buffer, cgi->pipeInput, 4096, &readed);
	
	TRACE (ENTRIES, "read... ret=%d %d\n", ret, readed);

	switch (ret) {
	case ret_eagain:
		cherokee_thread_deactive_to_polling (HANDLER_THREAD(cgi), HANDLER_CONN(cgi), cgi->pipeInput, 0, false);
		return ret_eagain;

	case ret_ok:
		TRACE (ENTRIES, "%d bytes read\n", readed);
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
cherokee_handler_cgi_new  (cherokee_handler_t **hdl, void *cnt, cherokee_table_t *properties)
{
	int i;
	CHEROKEE_NEW_STRUCT (n, handler_cgi);
	
	/* Init the base class
	 */
	cherokee_handler_cgi_base_init (CGI_BASE(n), cnt, properties, 
					cherokee_handler_cgi_add_env_pair, read_from_cgi);

	/* Virtual methods
	 */
	MODULE(n)->init         = (handler_func_init_t) cherokee_handler_cgi_init;
	MODULE(n)->free         = (handler_func_free_t) cherokee_handler_cgi_free;
	
	/* Virtual methods: implemented by handler_cgi_base
	 */
	HANDLER(n)->step        = (handler_func_step_t) cherokee_handler_cgi_base_step;
	HANDLER(n)->add_headers = (handler_func_add_headers_t) cherokee_handler_cgi_base_add_headers;

	/* Init
	 */
	n->post_data_sent =  0;
	n->pipeInput      = -1;
	n->pipeOutput     = -1;

#ifdef _WIN32
	n->process   = NULL;
	n->thread    = NULL;

	cherokee_buffer_init (&n->envp);
#else
	n->pid       = -1;
	n->envp_last =  0;	

	for (i=0; i<ENV_VAR_NUM; i++)
		n->envp[i] = NULL;
#endif

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

#ifndef _WIN32

	/* Reap defunct children until there aren't any more. 
	 */
	for (child_count = 0; ; ++child_count)
        {
		pid = waitpid (-1, &status, WNOHANG);

                /* none left */
		if (pid == 0) break;

		else if (pid < 0) {
			/* because of ptrace */
			if (errno == EINTR) continue;
			break;
		}
        }
#endif

	return child_count;
}


ret_t
cherokee_handler_cgi_free (cherokee_handler_cgi_t *cgi)
{
 	int i; 

	/* Free the rest of the handler CGI memory
	 */
	cherokee_handler_cgi_base_free (CGI_BASE(cgi));

	/* Close the connection with the CGI
	 */
	if (cgi->pipeInput > 0) {
		close (cgi->pipeInput);
		cgi->pipeInput = -1;
	}

	if (cgi->pipeOutput > 0) {
		close (cgi->pipeOutput);
		cgi->pipeOutput = -1;
	}

#ifndef _WIN32
        /* Maybe kill the CGI
	 */
	if (cgi->pid > 0) {
		pid_t pid;

		do {
			pid = waitpid (cgi->pid, NULL, WNOHANG);
		} while ((pid == 1) && (errno == EINTR));

		if (pid <= 0) {
			kill (cgi->pid, SIGTERM);
		}
	}
#else
	if (cgi->process) {
		WaitForSingleObject (cgi->process, INFINITE);
		CloseHandle (cgi->process);
	}

	if (cgi->thread) {
		CloseHandle (cgi->thread);		
	}
#endif

#ifdef _WIN32
	cherokee_buffer_mrproper (&cgi->envp);
#else
	for (i=0; i<cgi->envp_last; i++) {
		free (cgi->envp[i]);
		cgi->envp[i] = NULL;
	}
#endif

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


void
cherokee_handler_cgi_add_env_pair (cherokee_handler_cgi_base_t *cgi_base,
				   char *name,    int name_len,
				   char *content, int content_len)
{
	cherokee_handler_cgi_t *cgi = HANDLER_CGI(cgi_base);

#ifdef _WIN32
	cherokee_buffer_add_va (&cgi->envp, "%s=%s", name, content);
	cherokee_buffer_add (&cgi->envp, "\0", 1);
#else	
	char *entry;

	/* Build the new envp entry
	 */
	entry = (char *) malloc (name_len + content_len + 2); 
	if (entry == NULL) return;

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
#endif
}

static ret_t
add_environment (cherokee_handler_cgi_t *cgi, cherokee_connection_t *conn)
{
	ret_t                        ret;
	char                        *lenght;
	cuint_t                      lenght_len;
	cherokee_handler_cgi_base_t *cgi_base = CGI_BASE(cgi);

	ret = cherokee_handler_cgi_base_build_envp (CGI_BASE(cgi), conn);
	if (unlikely (ret != ret_ok)) return ret;

	/* CONTENT_LENGTH
	 */
	ret = cherokee_header_get_known (&conn->header, header_content_length, &lenght, &lenght_len);
	if (ret == ret_ok)
		set_env (cgi_base, "CONTENT_LENGTH", lenght, lenght_len);

	/* SCRIPT_FILENAME
	 */
	if (cgi_base->executable.len <= 0)
		return ret_error;

	set_env (cgi_base, "SCRIPT_FILENAME",
		 cgi_base->executable.buf, cgi_base->executable.len);
	return ret_ok;
}


static ret_t
send_post (cherokee_handler_cgi_t *cgi)
{
	ret_t                  ret;
	int                    eagain_fd = -1;
	int                    mode      =  0;
	cherokee_connection_t *conn      = HANDLER_CONN(cgi);
		
	ret = cherokee_post_walk_to_fd (&conn->post, cgi->pipeOutput, &eagain_fd, &mode);

	TRACE (ENTRIES",post", "Sending POST fd=%d, ret=%d\n", cgi->pipeOutput, ret);
	
	switch (ret) {
	case ret_ok:
		TRACE (ENTRIES",post", "%s\n", "finished");

		close (cgi->pipeOutput);
		cgi->pipeOutput = -1;
		return ret_ok;
		
	case ret_eagain:
		if (eagain_fd != -1) {
			cherokee_thread_deactive_to_polling (HANDLER_THREAD(cgi), conn, eagain_fd, mode, false);
		}
		
		return ret_eagain;
		
	default:
		return ret;
	}
}


ret_t 
cherokee_handler_cgi_init (cherokee_handler_cgi_t *cgi)
{
	ret_t                        ret;
	cherokee_handler_cgi_base_t *cgi_base = CGI_BASE(cgi);
	cherokee_connection_t       *conn     = HANDLER_CONN(cgi);

	switch (cgi_base->init_phase) {
	case hcgi_phase_build_headers:

		/* Extracts PATH_INFO and filename from request uri 
		 */
		if (cherokee_buffer_is_empty (&cgi_base->executable)) {
			ret = cherokee_handler_cgi_base_extract_path (cgi_base, true);
			if (unlikely (ret < ret_ok)) return ret;
		}

		/* It has to update the timeout of the connection,
		 * otherwhise the server will drop it for the CGI
		 * isn't fast enough
		 */
		conn->timeout = CONN_THREAD(conn)->bogo_now + CGI_TIMEOUT;
		
		/* Launch the CGI
		 */
		ret = fork_and_execute_cgi(cgi);
		if (unlikely (ret != ret_ok)) return ret;

		/* Send the Post if needed
		 */
		if (! cherokee_post_is_empty (&conn->post)) {
			cherokee_post_walk_reset (&conn->post);
		}

		cgi_base->init_phase = hcgi_phase_send_headers;

	case hcgi_phase_send_headers:
		/* There is nothing to do here, so move on!
		 */
		cgi_base->init_phase = hcgi_phase_send_post;

	case hcgi_phase_send_post:
		if (! cherokee_post_is_empty (&conn->post)) {
			return send_post (cgi);
		}
	}
	
	TRACE (ENTRIES, "finishing %s\n", "ret_ok");
	return ret_ok;
}


/*******************************
 * UNIX: Linux, Solaris, etc.. *
 *******************************/

#ifndef _WIN32

static ret_t
_fd_set_properties (int fd, int add_flags, int remove_flags)
{
	int flags;

	flags = fcntl (fd, F_GETFL, 0);

	flags |= add_flags;
	flags &= ~remove_flags;

	if (fcntl (fd, F_SETFL, flags) == -1) {
		PRINT_ERROR ("ERROR: Setting pipe properties fd=%d: %s\n", fd, strerror(errno));
		return ret_error;
	}	

	return ret_ok;
}

static void 
manage_child_cgi_process (cherokee_handler_cgi_t *cgi, int pipe_cgi[2], int pipe_server[2])
{
	/* Child process
	 */
	int                          re;
	cherokee_connection_t       *conn          = HANDLER_CONN(cgi);
	cherokee_handler_cgi_base_t *cgi_base      = CGI_BASE(cgi);
	char                        *absolute_path = cgi_base->executable.buf;
	char                        *argv[4]       = { NULL, NULL, NULL, NULL };

	/* Close useless sides
	 */
	close (pipe_cgi[0]);
	close (pipe_server[1]);
		
	/* Change stdin and out
	 */
	dup2 (pipe_server[0], STDIN_FILENO);
	close (pipe_server[0]);

	dup2 (pipe_cgi[1], STDOUT_FILENO);
	close (pipe_cgi[1]);

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

	/* Set SIGPIPE
	 */
	signal (SIGPIPE, SIG_DFL);

	/* Sets the new environ. 
	 */			
	add_environment (cgi, conn);

	/* Change the directory 
	 */
	if (! cherokee_buffer_is_empty (&conn->effective_directory)) {
		chdir (conn->effective_directory.buf);
	} else {
		char *file = strrchr (absolute_path, '/');

		*file = '\0';
		chdir (absolute_path);
		*file = '/';
	}

	/* Build de argv array
	 */
	argv[0] = absolute_path;
	
	if (cgi_base->param.len > 0) {
		argv[1] = cgi_base->param.buf;
		argv[2] = cgi_base->param_extra.buf;
	} else {
		argv[1] = cgi_base->param_extra.buf;
	}

	/* Change the execution user?
	 */
	if (cgi_base->change_user) {
		struct stat info;
			
		re = stat (argv[1], &info);
		if (re >= 0) {
			setuid (info.st_uid);
		}
	}

	/* Lets go.. execute it!
	 */
	re = execve (absolute_path, argv, cgi->envp);
	if (re < 0) {
		switch (errno) {
		case ENOENT:
			printf ("Status: 404" CRLF CRLF);
			break;
		default:
			printf ("Status: 500" CRLF CRLF);
		}

		exit(1);
	}

	/* OH MY GOD!!! an error is here 
	 */
	SHOULDNT_HAPPEN;
	exit(1);
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
	re  = pipe (pipes.cgi);
	re |= pipe (pipes.server);

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
		close (pipes.cgi[0]);
		close (pipes.cgi[1]);

		close (pipes.server[0]);
		close (pipes.server[1]);
		
		conn->error_code = http_internal_error;
		return ret_error;
	}

	TRACE (ENTRIES, "pid %d\n", pid);

	close (pipes.server[0]);
	close (pipes.cgi[1]);

	cgi->pid        = pid;
	cgi->pipeInput  = pipes.cgi[0];
	cgi->pipeOutput = pipes.server[1];

	/* Set to Input to NON-BLOCKING
	 */
	_fd_set_properties (cgi->pipeInput, O_NDELAY|O_NONBLOCK, 0);

	return ret_ok;
}



#else



/*******************************
 * WINDOWS                     *
 *******************************/

static ret_t
fork_and_execute_cgi_win32 (cherokee_handler_cgi_t *cgi)
{
	int                    re;
	PROCESS_INFORMATION    pi;
	STARTUPINFO            si;
	char                  *cmd;
	cherokee_buffer_t      cmd_line = CHEROKEE_BUF_INIT;
	cherokee_buffer_t      exec_dir = CHEROKEE_BUF_INIT;
	cherokee_connection_t *conn     = HANDLER_CONN(cgi);

	SECURITY_ATTRIBUTES saSecAtr;
	HANDLE hProc; 
	HANDLE hChildStdinRd  = INVALID_HANDLE_VALUE;
	HANDLE hChildStdinWr  = INVALID_HANDLE_VALUE;
	HANDLE hChildStdoutRd = INVALID_HANDLE_VALUE;
	HANDLE hChildStdoutWr = INVALID_HANDLE_VALUE;

	/* Create the environment for the process
	 */
	add_environment (cgi, conn);
	cherokee_buffer_add (&cgi->envp, "\0", 1);

	/* Command line
	 */
	cmd = CGI_BASE(cgi)->executable.buf;
	cherokee_buffer_add (&cmd_line, cmd, strlen(cmd));
	cherokee_buffer_add_va (&cmd_line, " \"%s\"", CGI_BASE(cgi)->param.buf);

	/* Execution directory
	 */
	if (! cherokee_buffer_is_empty (&conn->effective_directory)) {
		cherokee_buffer_add_buffer (&exec_dir, &conn->effective_directory);
	} else {
		char *file = strrchr (cmd, '/');
		char *end  = CGI_BASE(cgi)->executable.buf + CGI_BASE(cgi)->executable.len;

		cherokee_buffer_add (&exec_dir, cmd, 
				     CGI_BASE(cgi)->executable.len - (end - file));
	}

	/* Set the bInheritHandle flag so pipe handles are inherited. 
	 */
	memset(&saSecAtr, 0, sizeof(SECURITY_ATTRIBUTES));
	saSecAtr.nLength = sizeof(SECURITY_ATTRIBUTES);
	saSecAtr.lpSecurityDescriptor = NULL;
	saSecAtr.bInheritHandle       = TRUE;

	/* Create the pipes
	 */
	hProc = GetCurrentProcess();

	re = CreatePipe (&hChildStdoutRd, &hChildStdoutWr, &saSecAtr, 0);
	if (!re) return ret_error;

	re = CreatePipe (&hChildStdinRd, &hChildStdinWr, &saSecAtr, 0);
	if (!re) return ret_error;

	/* Make them inheritable
	 */
	re = DuplicateHandle (hProc,  hChildStdoutRd, 
			      hProc, &hChildStdoutRd, 
			      0, TRUE,
			      DUPLICATE_CLOSE_SOURCE | DUPLICATE_SAME_ACCESS); 
	if (!re) return ret_error;

	re = DuplicateHandle (hProc,  hChildStdinWr,
			      hProc, &hChildStdinWr,
			      0, TRUE,
			      DUPLICATE_CLOSE_SOURCE | DUPLICATE_SAME_ACCESS);
	if (!re) return ret_error;


	/* Starting information
	 */
	ZeroMemory (&si, sizeof(STARTUPINFO));
	si.cb         = sizeof(STARTUPINFO); 
	si.hStdOutput = hChildStdoutWr;
	si.hStdError  = hChildStdoutWr;
	si.hStdInput  = hChildStdinRd;
	si.dwFlags   |= STARTF_USESTDHANDLES;

	TRACE (ENTRIES, "exec %s dir %s\n", cmd_line.buf, exec_dir.buf);

	/* Launch the child process
	 */
	re = CreateProcess (cmd,              /* ApplicationName */
			    cmd_line.buf,     /* Command line */ 
			    NULL,             /* Process handle not inheritable */
			    NULL,             /* Thread handle not inheritable */ 
			    TRUE,             /* Handle inheritance */ 
			    0,                /* Creation flags */
			    cgi->envp.buf,    /* Use parent's environment block */
			    exec_dir.buf,     /* Use parent's starting directory */ 
			    &si,              /* Pointer to STARTUPINFO structure */
			    &pi);             /* Pointer to PROCESS_INFORMATION structure */

	CloseHandle (hChildStdinRd);
	CloseHandle (hChildStdoutWr);

	if (!re) {
		PRINT_ERROR ("CreateProcess error: error=%d\n", GetLastError());

		CloseHandle (pi.hProcess);
		CloseHandle (pi.hThread);

		conn->error_code = http_internal_error;
		return ret_error;
	}

	cherokee_buffer_mrproper (&cmd_line);
	cherokee_buffer_mrproper (&exec_dir);

	cgi->thread  = pi.hThread;
	cgi->process = pi.hProcess;

	/* Wait for the CGI process to be ready
	 */
	WaitForInputIdle (pi.hProcess, INFINITE);

	/* Extract the file descriptors
	 */
	cgi->pipeInput  = _open_osfhandle((LONG)hChildStdoutRd, O_BINARY|_O_RDONLY); 
	
	if (cherokee_post_is_empty (&conn->post)) {
		CloseHandle (hChildStdinWr);
	} else {
		cgi->pipeOutput = _open_osfhandle((LONG)hChildStdinWr,  O_BINARY|_O_WRONLY);
	}

	TRACE (ENTRIES, "In fd %d, Out fd %d\n", cgi->pipeInput, cgi->pipeOutput); 

	return ret_ok;
}

#endif


/* Library init function
 */

static cherokee_boolean_t _cgi_is_init = false;

#if 0
#include <signal.h>

static void 
child_finished(int sng)
{
	int status;
	while(waitpid (0, &status, WNOHANG) > 0);
}
#endif

void
MODULE_INIT(cgi) (cherokee_module_loader_t *loader)
{
	if (_cgi_is_init) return;
	_cgi_is_init = true;

#if 0
	signal(SIGCHLD, child_finished);
#endif
}

