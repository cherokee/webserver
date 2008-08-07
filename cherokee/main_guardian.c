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
#include <signal.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <errno.h>
#include "server.h"

#ifdef HAVE_GETOPT_LONG
# include <getopt.h>
#else 
# include "getopt/getopt.h"
#endif

#define DELAY_ERROR       3000 * 1000
#define DELAY_RESTARTING   500 * 1000
#define PID_FILE          CHEROKEE_VAR_RUN "/cherokee-guardian.pid"

pid_t              pid;
cherokee_boolean_t graceful_restart; 

static void
pid_file_save (const char *pid_file, int pid)
{
	FILE *file;
	char  tmp[10];

	file = fopen (pid_file, "w+");
	if (file == NULL) {
		PRINT_MSG ("Cannot write PID file '%s'\n", pid_file);
		return;
	}

	snprintf (tmp, sizeof(tmp), "%d\n", pid);
	fwrite (tmp, 1, strlen(tmp), file);
	fclose (file);
}

static void
pid_file_clean (const char *pid_file)
{
	struct stat info;

	if (lstat (pid_file, &info) != 0) 
		return;
	if (! S_ISREG(info.st_mode))
		return;
	if (info.st_uid != getuid())
		return;
	if (info.st_size > (int) sizeof("65535\r\n"))
		return;

	unlink (pid_file);
}

static ret_t
process_wait (pid_t pid)
{
	pid_t re;
	int   exitcode = 0;

	while (true) {
		re = waitpid (pid, &exitcode, 0);
		if (re > 0)
			break;
		else if (errno == EINTR) 
			if (graceful_restart)
				break;
			else
				continue;
		else 
			return ret_error;
	}

	if (WIFEXITED(exitcode)) {
		int re = WEXITSTATUS(exitcode);

		/* Child terminated normally */ 
		PRINT_MSG ("Server PID=%d exited re=%d\n", pid, re);
		if (re != 0) 
			return ret_error;
	} 
	else if (WIFSIGNALED(exitcode)) {
		/* Child process terminated by a signal */
		PRINT_MSG ("Server PID=%d received a signal=%d\n", pid, WTERMSIG(exitcode));
	}

	return ret_ok;
}

static void 
guardian_signals_handler (int sig, siginfo_t *si, void *context) 
{
	int exitcode;

	UNUSED(si);
	UNUSED(context);

	switch (sig) {
	case SIGUSR1:
		/* Restart: the tough way */
		kill (pid, SIGINT);
		process_wait (pid);
		break;

	case SIGHUP:
		/* Graceful restart */
		graceful_restart = true;
		kill (pid, SIGHUP);
		break;

	case SIGTERM:
		/* Kill child and exit */
		kill (pid, SIGTERM);
		process_wait (pid);
		pid_file_clean (PID_FILE);
		exit(0);

	case SIGCHLD:
		/* Child exited */
		wait (&exitcode);
		break;
		
	default:
		/* Forward the signal */
		kill (pid, sig);
	}
}


static void
set_guardian_signals (void)
{
	struct sigaction act;

	/* Signals it handles
	 */
	memset(&act, 0, sizeof(act));

	act.sa_handler = SIG_IGN;
	sigaction (SIGPIPE, &act, NULL);
	
	/* Signals it handles
	 */
	act.sa_sigaction = guardian_signals_handler;
	sigemptyset (&act.sa_mask);
	act.sa_flags = SA_SIGINFO;

	sigaction (SIGHUP,  &act, NULL);
	sigaction (SIGTERM, &act, NULL);
	sigaction (SIGUSR1, &act, NULL);
	sigaction (SIGCHLD, &act, NULL);
}

static ret_t
may_daemonize (int argc, char *argv[])
{
	int   i;
	pid_t pid;
	int   daemonize = 0;

	/* Look for the '-d' parameter
	 */
	for (i=0; i<argc; i++) {
		if ((strcmp(argv[i], "-d") == 0) ||
		    (strcmp(argv[i], "--detach") == 0))
		{
			daemonize = 1;
			argv[i]   = "";
		}
	}

	if (daemonize) {
		pid = fork();
		if (pid != 0) 
			exit(0);
		close(0);
		setsid();
	}

	return ret_ok;
}

static pid_t
process_launch (const char *path, char *argv[])
{
	pid_t pid;

	/* Execute the server
	 */
	pid = fork();
	if (pid == 0) {
		argv[0] = (char *) path;
		execvp (path, argv);
		exit (1);
	}
	
	return pid;
}

static cherokee_boolean_t
is_single_execution (int argc, char *argv[])
{
	int i;

	for (i=0; i<argc; i++) {
		if (!strcmp (argv[i], "-t") || !strcmp (argv[i], "--test")    ||
		    !strcmp (argv[i], "-h") || !strcmp (argv[i], "--help")    ||
		    !strcmp (argv[i], "-V") || !strcmp (argv[i], "--version") ||
		    !strcmp (argv[i], "-i") || !strcmp (argv[i], "--print-server-info"))
			return true;
	}

	return false;
}

int
main (int argc, char *argv[])
{
	ret_t              ret;
	cherokee_boolean_t single_time;

	set_guardian_signals();
	single_time = is_single_execution (argc, argv);

	/* Turn into a daemon
	 */
	if (! single_time) {
		may_daemonize (argc, argv);
		pid_file_save (PID_FILE, getpid());
	}

	while (true) {
		graceful_restart = false;

		pid = process_launch (CHEROKEE_SRV_PATH, argv);
		if (pid < 0) {
			PRINT_MSG ("Couldn't launch '%s'\n", CHEROKEE_SRV_PATH);
			exit (1);
		}

		ret = process_wait (pid);
		if (single_time)
			break;

		usleep ((ret == ret_ok) ? 
			DELAY_RESTARTING : 
			DELAY_ERROR);
	}

	pid_file_clean (PID_FILE);
	return 0;
}
