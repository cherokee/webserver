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
#include <signal.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <errno.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <grp.h>
#include <pthread.h>

#ifdef HAVE_SYSV_SEMAPHORES
# include <sys/ipc.h>
# include <sys/sem.h>
#endif

#include "server.h"
#include "services.h"

#ifdef HAVE_GETOPT_LONG
# include <getopt.h>
#else
# include "getopt/getopt.h"
#endif


/* union semun is defined by including <sys/sem.h>
 * according to X/OPEN we have to define it ourselves:
 */
#ifndef HAVE_SEMUN
union semun {
	int                 val;
	struct semid_ds    *buf;
	unsigned short int *array;
	struct seminfo     *__buf;
};
# undef  HAVE_SEMUN
# define HAVE_SEMUN 1
#endif

#ifndef NSIG
# define NSIG 32
#endif

#define DELAY_ERROR       3000 * 1000
#define DELAY_RESTARTING   500 * 1000

#define DEFAULT_PID_FILE  CHEROKEE_VAR_RUN "/cherokee.pid"
#define DEFAULT_CONFIG    CHEROKEE_CONFDIR "/cherokee.conf"

#define VALGRIND_PREFIX   {"valgrind", "--leak-check=full", "--num-callers=40", "-v", "--leak-resolution=high", NULL}

pid_t               pid;
char               *worker_uid;
int                 worker_retcode;
int                 worker_services_fd = -1;
char                worker_services_fd_str[16];
char               *worker_services_argv[] = { "-s", worker_services_fd_str };
cherokee_boolean_t  graceful_restart;
char               *cherokee_worker;
char               *pid_file_path         = NULL;
cherokee_boolean_t  use_valgrind          = false;
pthread_t           spawn_thread;

static void
figure_worker_path (const char *arg0)
{
	pid_t       me;
	char        tmp[512];
	int         len, re, i;
	const char *d;
	const char *unix_paths[] = {"/proc/%d/exe",        /* Linux   */
	                            "/proc/%d/path/a.out", /* Solaris */
	                            "/proc/%d/file",       /* BSD     */
	                            NULL};

	/* Invoked with the fullpath */
	if (arg0[0] == '/') {
		len = strlen(arg0) + sizeof("-worker");

		cherokee_worker = malloc (len);
		if (cherokee_worker == NULL)
			goto out;

		snprintf (cherokee_worker, len, "%s-worker", arg0);
		return;
	}

	/* Partial path work around */
	d = arg0;
	while (*d && *d != '/') d++;

	if ((arg0[0] == '.') || (*d == '/')) {
		d = getcwd (tmp, sizeof(tmp));
		len = strlen(arg0) + strlen(d) + sizeof("-worker") +1;

		cherokee_worker = malloc (len);
		if (cherokee_worker == NULL)
			goto out;

		snprintf (cherokee_worker, len, "%s/%s-worker", d, arg0);
		return;
	}

	/* Deal with unixes
	 */
	me = getpid();

	for (i=0; unix_paths[i]; i++) {
		char link[512];

		snprintf (tmp, sizeof(tmp), unix_paths[i], me);
		re = readlink (tmp, link, sizeof(link));
		if (re > 0) {
			link[re] = '\0';
			len = re + sizeof("-worker");

			cherokee_worker = malloc (len);
			if (cherokee_worker == NULL)
				goto out;

			snprintf (cherokee_worker, len, "%s-worker", link);
			return;
		}
	}

out:
	/* The very last option, use the default path
	 */
	cherokee_worker = malloc (sizeof(CHEROKEE_WORKER));
	if (cherokee_worker == NULL) {
		PRINT_MSG_S ("ERROR: Could not find cherokee-worker\n");
		PRINT_MSG_S ("\n");
		exit(1);
	}

	snprintf (cherokee_worker, sizeof(CHEROKEE_WORKER), CHEROKEE_WORKER);
}

static ret_t
check_worker_version (const char *this_exec)
{
	FILE *f;
	char  tmp[256];
	char *line, *p;
	int   re;

	snprintf (tmp, sizeof(tmp), "%s -i", cherokee_worker);
	f = popen (tmp, "r");
	if (f == NULL) {
		PRINT_MSG ("(critical) Cannot execute '%s'\n", tmp);
		goto error;
	}

	while (! feof(f)) {
		/* Skip line until it found the version entry
		 */
		line = fgets (tmp, sizeof(tmp), f);
		if (line == NULL)
			continue;

		line = strstr (line, "Version: ");
		if (line == NULL)
			continue;

		/* Compare both version strings
		 */
		line += 9;
		re = strncmp (line, PACKAGE_VERSION, strlen(PACKAGE_VERSION));
		if (re == 0) {
			pclose(f);
			return ret_ok;
		}

		/* Remove the new line character and report the error
		 */
		p = line;
		while (*p++ != '\n');
		*p = '\0';

		PRINT_MSG_S ("ERROR: Broken installation detected\n");
		PRINT_MSG   ("  Cherokee        (%s) %s\n", this_exec, PACKAGE_VERSION);
		PRINT_MSG   ("  Cherokee-worker (%s) %s\n", cherokee_worker, line);
		PRINT_MSG_S ("\n");
		PRINT_MSG_S ("This issue is usually caused by a secondary Cherokee installation in your $PATH.\n");
		PRINT_MSG_S ("The following command might help to find where the installations were performed:\n\n");
		PRINT_MSG_S ("find `echo $PATH | sed 's/:/ /g'` -name 'cherokee*' -exec dirname \"{}\" \\; | uniq\n");
		PRINT_MSG_S ("\n");

		goto error;
	}

	PRINT_MSG ("(critical) Couldn't find the version string: '%s -i'\n", cherokee_worker);
error:
	if (f != NULL) {
		pclose(f);
	}
	return ret_error;
}

static const char *
figure_config_file (int argc, char **argv)
{
	int i;

	for (i=0; i<argc; i++) {
		if ((i+1 < argc) &&
		    (strcmp(argv[i], "-C") == 0))
			return argv[i+1];
	}

	return DEFAULT_CONFIG;
}


static cherokee_boolean_t
figure_use_valgrind (int argc, char **argv)
{
	int i;

	for (i=0; i<argc; i++) {
		if ((strcmp(argv[i], "-v") == 0) ||
		    (strcmp(argv[i], "--valgrind") == 0))
		{
			argv[i] = (char *)"";
			return true;
		}
	}

	return false;
}


static char *
figure_worker_uid (const char *config)
{
	FILE *f;
	char *p;
	char *line;
	char  tmp[512];

	f = fopen (config, "r");
	if (f == NULL)
		return NULL;

	while (! feof(f)) {
		line = fgets (tmp, sizeof(tmp), f);
		if ((line != NULL) &&
		    (! strncmp (line, "server!user = ", 14)))
		{
			fclose(f);
			p = line + 14;
			while (*p) {
				if ((*p == '\r') || (*p == '\n')) {
					*p = '\0';
					break;
				}
				p += 1;
			}
			return strdup(line + 14);
		}
	}

	fclose(f);
	return NULL;
}

static char *
figure_pid_file_path (const char *config)
{
	FILE *f;
	char *p;
	char *line;
	char  tmp[512];

	/* Open the config file
	 */
	f = fopen (config, "r");
	if (f == NULL) {
		return strdup(DEFAULT_PID_FILE);
	}

	/* Look up for the right key
	 */
	while (! feof(f)) {
		line = fgets (tmp, sizeof(tmp), f);
		if ((line != NULL) &&
		    (! strncmp (line, "server!pid_file = ", 18)))
		{
			fclose(f);
			p = line + 18;
			while (*p) {
				if ((*p == '\r') || (*p == '\n')) {
					*p = '\0';
					break;
				}
				p += 1;
			}
			return strdup(line + 18);
		}
	}

	fclose(f);

	/* Not found, use default
	 */
	return strdup(DEFAULT_PID_FILE);
}

static void
pid_file_save (const char *pid_file, int pid)
{
	size_t  written;
	FILE   *file;
	char    tmp[10];

	file = fopen (pid_file, "w+");
	if (file == NULL) {
		/* Do not report any error if the PID file cannot be
		 * created. It wouldn't allow cherokee-admin to start.
		 */
		return;
	}

	snprintf (tmp, sizeof(tmp), "%d\n", pid);
	written = fwrite (tmp, 1, strlen(tmp), file);
	fclose (file);

	if (written <= 0) {
		PRINT_MSG ("(warning) Couldn't write PID file '%s'\n", pid_file);
	}
}

static void
remove_pid_file (const char *file)
{
	struct stat info;

	if (lstat (file, &info) != 0)
		return;
	if (! S_ISREG(info.st_mode))
		return;
	if (info.st_uid != getuid())
		return;
	if (info.st_size > (int) sizeof("65535\r\n"))
		return;

	unlink (file);
}

static void
pid_file_clean (const char *pid_file)
{
	char    *pid_file_worker;
	cuint_t  len;

	if (pid_file == NULL)
		return;

	/* Clean main choerkee pid file
	 */
	remove_pid_file (pid_file);

	/* Clean also "worker" pid file
	 */
	len = strlen (pid_file);
	pid_file_worker = (char *) malloc (len + 8);
	if (unlikely (pid_file_worker == NULL)) {
		return;
	}

	memcpy (pid_file_worker, pid_file, len);
	memcpy (pid_file_worker + len, ".worker\0", 8);

	remove_pid_file (pid_file_worker);

	free (pid_file_worker);
}

static NORETURN void *
spawn_thread_func (void *param)
{
	UNUSED (param);

	while (true) {
		if (cherokee_services_server_serve_request () != ret_ok)
			break;
	}

	while (true) /* Do nothing, we failed in our service work */;
}

static ret_t
spawn_init (void)
{
	ret_t ret;
	int re;

	ret = cherokee_services_server_init (&worker_services_fd);
	if (ret != ret_ok) {
		return ret;
	}

	snprintf(worker_services_fd_str, 16, "%d", worker_services_fd);

	re = pthread_create (&spawn_thread, NULL, spawn_thread_func, NULL);

	if (re != 0) {
               PRINT_MSG_S ("(critical) Couldn't spawning thread..\n");
               return ret_error;
	}

	return ret_ok;
}

static void
clean_up (void)
{
	cherokee_services_server_free ();
	pid_file_clean (pid_file_path);
}

static ret_t
process_wait (pid_t wpid)
{
	pid_t re;
	int   exitcode = 0;

	while (true) {
		re = waitpid (wpid, &exitcode, 0);
		if (re > 0)
			break;
		else if (errno == EINTR)
			if (graceful_restart)
				break;
			else
				continue;
		else if (errno == ECHILD) {
			return ret_ok;
		} else {
			return ret_error;
		}
	}

	if (WIFEXITED(exitcode)) {
		int re = WEXITSTATUS(exitcode);

		if (wpid == pid && re == EXIT_OK_ONCE) {
			clean_up();
			exit (EXIT_OK);

		} else if (wpid == pid && re == EXIT_ERROR_FATAL) {
			clean_up();
			exit (EXIT_ERROR);
		}

		/* Child terminated normally */
		PRINT_MSG ("PID %d: exited re=%d\n", wpid, re);
		if (re != 0 && wpid == pid) {
			worker_retcode = re;
			return ret_error;
		}
	}
	else if (WIFSIGNALED(exitcode)) {
		/* Child process terminated by a signal */
		PRINT_MSG ("PID %d: received a signal=%d\n", wpid, WTERMSIG(exitcode));
	}

	worker_retcode = 0;
	return ret_ok;
}

static void
signals_handler (int sig, siginfo_t *si, void *context)
{
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

	case SIGINT:
	case SIGTERM:
		/* Kill all child */
		kill (0, SIGTERM);

		errno = 0;
		while ((wait (0) == -1) && (errno == EINTR));

		/* Clean up and exit */
		clean_up();
		exit(0);

	case SIGCHLD:
		/* Child exited */
		process_wait (si->si_pid);
		break;

	default:
		/* Forward the signal */
		kill (pid, sig);
	}
}


static void
set_signals (void)
{
	struct sigaction act;

	/* Signals it handles
	 */
	memset(&act, 0, sizeof(act));

	act.sa_handler = SIG_IGN;
	sigaction (SIGPIPE, &act, NULL);

	/* Signals it handles
	 */
	act.sa_sigaction = signals_handler;
	sigemptyset (&act.sa_mask);
	act.sa_flags = SA_SIGINFO;

	sigaction (SIGHUP,  &act, NULL);
	sigaction (SIGINT,  &act, NULL);
	sigaction (SIGTERM, &act, NULL);
	sigaction (SIGUSR1, &act, NULL);
	sigaction (SIGUSR2, &act, NULL);
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
			argv[i]   = (char *)"";
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
	pid_t       pid;
	char      **new_args = NULL;
	int         total = 0;
	int         len_argv;
	int         len_valg = 0;
	const char *valgrind_args[] = VALGRIND_PREFIX;

	for (len_argv=0; argv[len_argv]; len_argv++);
	for (len_valg=0; valgrind_args[len_valg]; len_valg++);

	if (use_valgrind) {
		new_args = malloc ((len_argv + len_valg + 2 +
		                   ((worker_services_fd == -1) ? 0 : 2)) *
		                   sizeof(char *));
	} else {
		new_args = malloc ((len_argv + 2 +
		                   ((worker_services_fd == -1) ? 0 : 2)) *
		                   sizeof(char *));
	}
	if (new_args == NULL) {
		PRINT_MSG_S ("(critical) Could not allocate memory for worker args.\n");
		exit(1);
	}

	/* Copy
	 */
	total = 0;

	if (use_valgrind) {
		for (len_valg=0; valgrind_args[len_valg]; len_valg++, total++) {
			new_args[total] = (char *)valgrind_args[len_valg];
		}
	}

	for (len_argv=0; argv[len_argv]; len_argv++, total++) {
		new_args[total] = argv[len_argv];
	}

	if (worker_services_fd != -1) {
		new_args[total] = worker_services_argv[0];
		new_args[total+1] = worker_services_argv[1];
		total += 2;
	}

	new_args[total] = NULL;
	argv = new_args;

	#ifdef DEBUG
	for (total = 0; argv[total]; ++total) {
		PRINT_MSG ("ARGV[%d] = %s\n", total, argv[total]);
	}
	#endif

	/* Execute the server
	 */
	pid = fork();
	if (pid == 0) {
		if (use_valgrind) {
			path = "valgrind";
		} else {
			argv[0] = (char *) path;
		}

		do {
			execvp (path, argv);
		} while (errno == EINTR);

		PRINT_MSG ("ERROR: Could not execute %s\n", path);
		exit (1);
	}

	/* Clean up
	 */
	free (new_args);

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
	ret_t               ret;
	cherokee_boolean_t  single_time;
	const char         *config_file_path;

	/* Find the worker exec
	 */
	figure_worker_path (argv[0]);

	/* Sanity check
	 */
	ret = check_worker_version (argv[0]);
	if (ret != ret_ok)
		exit (EXIT_ERROR);

	set_signals();
	single_time = is_single_execution (argc, argv);

	/* Figure out some stuff
	 */
	config_file_path = figure_config_file (argc, argv);
	use_valgrind     = figure_use_valgrind (argc, argv);
	pid_file_path    = figure_pid_file_path (config_file_path);
	worker_uid       = figure_worker_uid (config_file_path);
	worker_retcode   = -1;

	/* Turn into a daemon
	 */
	if (! single_time) {
		may_daemonize (argc, argv);
		pid_file_save (pid_file_path, getpid());
	}

	/* Launch the spawning thread
	 */
	if (! single_time && ! use_valgrind) {
		ret = spawn_init();
		if (ret != ret_ok) {
			PRINT_MSG_S ("(warning) Couldn't initialize spawn mechanism.\n");
		}
	}

	while (true) {
		graceful_restart = false;

		pid = process_launch (cherokee_worker, argv);
		if (pid < 0) {
			PRINT_MSG ("(critical) Couldn't launch '%s'\n", cherokee_worker);
			exit (EXIT_ERROR);
		}

		ret = process_wait (pid);
		if (single_time)
			break;

		usleep ((ret == ret_ok) ?
			DELAY_RESTARTING :
			DELAY_ERROR);
	}

	if (! single_time) {
		clean_up();
	}

	return (worker_retcode == 0) ? EXIT_OK : EXIT_ERROR;
}
