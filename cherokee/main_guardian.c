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
#include <errno.h>
#include "server.h"

#ifdef HAVE_GETOPT_H
# include <getopt.h>
#endif

#define ERROR_DELAY       3000 * 1000
#define RESTARTING_DELAY   500 * 1000
#define PID_FILE          CHEROKEE_VAR_RUN "/cherokee-guardian.pid"

static cherokee_boolean_t exit_guardian = false;
static pid_t              pid;


static ret_t
process_wait (pid_t pid)
{
	pid_t re;
	int   exitcode = 0;

	re = waitpid (pid, &exitcode, 0);
	if (re == -1) 
		return ret_error;

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

	switch (sig) {
	case SIGUSR1:
		/* Restart Cherokee */
		kill (pid, SIGINT);
		process_wait (pid);
		break;

	case SIGCHLD:
		/* Child exited */
		wait (&exitcode);
		break;

	case SIGTERM:
		/* Kill child and exit */
		kill (pid, SIGTERM);
		process_wait (pid);
		exit(0);

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
	sigaction (SIGSEGV, &act, NULL);
	sigaction (SIGTERM, &act, NULL);
	sigaction (SIGUSR1, &act, NULL);
}

static pid_t
process_launch (const char *path, int argc, char *argv[])
{
	int   i;
	pid_t pid;
	int   daemonize = 0;

	/* Look for the '-b' parameter
	 */
	for (i=0; i<argc; i++) {
		if (strcmp(argv[i], "-b") == 0) {
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

	/* Execute the server
	 */
	pid = fork();
	if (pid == 0) {
		argv[0] = CHEROKEE_SRV_PATH;
		execvp (CHEROKEE_SRV_PATH, argv);
		exit (1);
	}
	
	return pid;
}


static void
save_pid_file (int pid)
{
	FILE *file;
	char  tmp[10];

	file = fopen (PID_FILE, "w+");
	if (file == NULL) {
		PRINT_MSG ("Cannot write PID file '%s'\n", PID_FILE);
	}

	snprintf (tmp, sizeof(tmp), "%d\n", getpid());
	fwrite (tmp, 1, strlen(tmp), file);
	fclose (file);
}


int
main (int argc, char *argv[])
{
	ret_t ret;

	set_guardian_signals();	   

	while (! exit_guardian) {
		pid = process_launch (CHEROKEE_SRV_PATH, argc, argv);
		if (pid < 0) {
			PRINT_MSG ("Couldn't launch '%s'\n", CHEROKEE_SRV_PATH);
			exit (1);
		}
		
		save_pid_file(pid);

		ret = process_wait (pid);
		usleep ((ret == ret_ok) ? RESTARTING_DELAY : ERROR_DELAY);
	} 

	return 0;
}
