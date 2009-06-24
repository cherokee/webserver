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
#include "spawner.h"

#ifdef HAVE_GETOPT_LONG
# include <getopt.h>
#else 
# include "getopt/getopt.h"
#endif

#define DELAY_ERROR       3000 * 1000
#define DELAY_RESTARTING   500 * 1000

#define DEFAULT_PID_FILE  CHEROKEE_VAR_RUN "/cherokee.pid"
#define DEFAULT_CONFIG    CHEROKEE_CONFDIR "/cherokee.conf"

pid_t               pid;
char               *pid_file_path;
char               *worker_uid;
cherokee_boolean_t  graceful_restart; 
char               *cherokee_worker;
char               *spawn_shared          = NULL;
char               *spawn_shared_name     = NULL;
int                 spawn_shared_sem      = -1;
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
		len = strlen(arg0) + sizeof("-worker") + 1;
		cherokee_worker = malloc (len);

		snprintf (cherokee_worker, len, "%s-worker", arg0);
		return;
	}

	/* Partial path work around */
	d = arg0;
	while (*d && *d != '/') d++;
		
	if ((arg0[0] == '.') || (*d == '/')) {
		d = getcwd (tmp, sizeof(tmp));
		len = strlen(arg0) + strlen(d) + sizeof("-worker") + 1;
		cherokee_worker = malloc (len);

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
			len = re + sizeof("-worker") + 1;

			cherokee_worker = malloc (len);
			snprintf (cherokee_worker, len, "%s-worker", link);
			return;
		}
	}

	/* The very last option, use the default path
	 */
	cherokee_worker = malloc (sizeof(CHEROKEE_WORKER));
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
		PRINT_MSG ("Cannot execute '%s'\n", tmp);
		goto error;
	}
	
	while (! feof(f)) {
		/* Skip line until it found the version entry
		 */
		line = fgets (tmp, sizeof(tmp), f);
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
		goto error;
	}

	PRINT_MSG ("Could not find the version string: '%s -i'\n", cherokee_worker);
error:
	pclose(f);
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

	f = fopen (config, "r");
	if (f == NULL)
		return NULL;

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
		 * created. It wouldn't allow cherokee-admin to start
		 * Cherokee. The worker would complain later anyway..
		 */
		return;
	}

	snprintf (tmp, sizeof(tmp), "%d\n", pid);
	written = fwrite (tmp, 1, strlen(tmp), file);
	fclose (file);

	if (written <= 0) {
		PRINT_MSG ("Cannot write PID file '%s'\n", pid_file);
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

	/* Clean main choerkee pid file
	 */
	remove_pid_file (pid_file);

	/* Clean also "worker" pid file
	 */
	len = strlen(pid_file);
	pid_file_worker = (char *) malloc (len + 8);
	if (unlikely (pid_file_worker == NULL))
		return;
	
	memcpy (pid_file_worker, pid_file, len);
	memcpy (pid_file_worker + len, ".worker\0", 8);
	
	remove_pid_file (pid_file_worker);

	free (pid_file_worker);
}

#ifdef HAVE_POSIX_SHM

static void
do_spawn (void)
{
	int          n;
	int          size;
	uid_t        uid;
	gid_t        gid;
	int          envs;
	pid_t        child;
	char        *interpreter;
	int          log_stderr   = 0;
	char        *log_file     = NULL;
	char        *uid_str      = NULL;
	char       **envp         = NULL;
	char        *p            = spawn_shared;
	const char  *argv[]       = {"sh", "-c", NULL, NULL};

	/* Read the shared memory
	 */

	/* 1.- Interpreter */
	size = *((int *)p);
	p += sizeof(int);

	interpreter = malloc (sizeof("exec ") + size);
	memcpy (interpreter, "exec ", 5);
	memcpy (interpreter + 5, p, size + 1);
	p += size + 1;

	/* 2.- UID & GID */
	size = *((int *)p);
	if (size > 0) {
		uid_str = strdup (p + sizeof(int));
	}
	p += sizeof(int) + size + 1;

	memcpy (&uid, p, sizeof(uid_t));
	p += sizeof(uid_t);

	memcpy (&gid, p, sizeof(gid_t));
	p += sizeof(gid_t);

	/* 3.- Environment */
	envs = *((int *)p);
	p += sizeof(int);

	envp = malloc (sizeof(char *) * (envs + 1));
	envp[envs] = NULL;

	for (n=0; n<envs; n++) {
		char *e;

		size = *((int *)p);
		p += sizeof(int);
		
		e = malloc (size + 1);
		memcpy (e, p, size);
		e[size] = '\0';

		envp[n] = e;
		p += size + 1;
	}

	/* 4.- Error log */
	size = *((int *)p);
	p += sizeof(int);
		
	if (size > 0) {
		if (! strncmp (p, "stderr", 6)) {
			log_stderr = 1;
		} else if (! strncmp (p, "file,", 5)) {
			log_file = p+5;
		}

		p += size + 1;
	}

	/* 5.- PID: it's -1 now */
	n = *((int *)p);
	if (n > 0) {
		kill (n, SIGTERM);
		*p = -1;
	}

	/* Spawn
	 */
	child = fork();
	switch (child) {
	case 0:
		/* Logging */
		if (log_file) {
			int fd;
			fd = open (log_file, O_WRONLY | O_APPEND | O_CREAT, 0600);
			if (fd < 0) {
				PRINT_ERROR ("WARNING: Couldn't open '%s' for writing..\n", log_file);
			}
			dup2 (fd, STDOUT_FILENO);
			dup2 (fd, STDERR_FILENO);
		} else if (log_stderr) {
			/* do nothing */
		} else {
			close (STDOUT_FILENO);
			close (STDERR_FILENO);
		}

		/* Change user & group */
		if (uid_str != NULL) {
			n = initgroups (uid_str, gid);
			if (n == -1) {
				PRINT_ERROR ("WARNING: initgroups failed User=%s, GID=%d\n", uid_str, gid);
			}
		}

		if (gid != -1) {
			n = setgid (gid);
			if (n != 0) {
				PRINT_ERROR ("WARNING: Could not set GID=%d\n", gid);
			}
		}

		if (uid != -1) {
			n = setuid (uid);
			if (n != 0) {
				PRINT_ERROR ("WARNING: Could not set UID=%d\n", uid);
			}
		}

		/* Clean the shared memory */
		size = (p - spawn_shared) - sizeof(int);
		memset (spawn_shared, 0, size);

		/* Execute the interpreter */
		argv[2] = interpreter;
		execve ("/bin/sh", (char **)argv, envp);

		PRINT_ERROR ("ERROR: Could spawn %s\n", interpreter);
		exit (1);
	case -1:
		/* Error */
		break;
	default:
		/* Return the PID */
		memcpy (p, (char *)&child, sizeof(int));
		printf ("PID %d: launched '/bin/sh -c %s' with uid=%d, gid=%d\n", child, interpreter, uid, gid);
		break;
	}
	
	/* Clean up
	 */
	free (interpreter);

	for (n=0; n<envs; n++) {
		free (envp[n]);
	}

	free (envp);
}

static void *
spawn_thread_func (void *param)
{
	int           re;
	struct sembuf so;

	UNUSED (param);

	while (true) {
		do {
			so.sem_num = 0;
			so.sem_op  = -1;
			so.sem_flg = SEM_UNDO;

			re = semop (spawn_shared_sem, &so, 1);
		} while (re < 0 && errno == EINTR);
		do_spawn ();
	}

	return NULL;
}


static int
sem_chmod (int sem, char *worker_uid)
{
	int              re;
	struct semid_ds  buf;
	struct passwd   *passwd;
	int              uid     = 0;
		
	uid = (int)strtol (worker_uid, (char **)NULL, 10);
	if (uid == 0) {
		passwd = getpwnam (worker_uid);
		if (passwd != NULL) {
			uid = passwd->pw_uid;
		}
	}
	
	if (uid == 0) {
		PRINT_ERROR ("Couldn't get UID for user '%s'\n", worker_uid);
		return -1;
	}

	re = semctl (sem, 0, IPC_STAT, &buf);
	if (re != -0) {
		PRINT_ERROR ("Couldn't IPC_STAT: errno=%d\n", errno);
		return -1;
	}

	buf.sem_perm.uid = uid;
	re = semctl (spawn_shared_sem, 0, IPC_SET, &buf);
	if (re != 0) {
		PRINT_ERROR ("Couldn't IPC_SET: uid=%d errno=%d\n", uid, errno);
		return -1;
	}

	return 0;
}


static ret_t
spawn_init (void)
{
	int re;
	int fd;
	int mem_len;

	/* Names
	*/
	mem_len = sizeof("/cherokee-spawner-XXXXXXX");
	spawn_shared_name = malloc (mem_len);
	snprintf (spawn_shared_name, mem_len, "/cherokee-spawner-%d", getpid());

	/* Create the shared memory
	 */
	fd = shm_open (spawn_shared_name, O_RDWR | O_EXCL | O_CREAT, 0600);
	if (fd < 0) {
		return ret_error;
	}

	re = ftruncate (fd, SPAWN_SHARED_LEN);
	if (re < 0) {
		close (fd);
		return ret_error;
	}
	
	spawn_shared = mmap (0, SPAWN_SHARED_LEN, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	if (spawn_shared == MAP_FAILED) {
		close (fd);
		spawn_shared = NULL;
		return ret_error;
	}

	memset (spawn_shared, 0, SPAWN_SHARED_LEN);

	close (fd);

	/* Semaphore
	*/
	spawn_shared_sem = semget (getpid(), 1, IPC_CREAT | SEM_R | SEM_A);
	if (spawn_shared_sem < 0) {
		return ret_error;
	}

	if (worker_uid != NULL) {
		sem_chmod (spawn_shared_sem, worker_uid);
	}

	/* Thread
	*/
	re = pthread_create (&spawn_thread, NULL, spawn_thread_func, NULL);
	if (re != 0) {
		PRINT_ERROR_S ("Couldn't spawning thread..\n");
		return ret_error;
	}
	
	return ret_ok;
}

static void
spawn_clean (void)
{
	long dummy = 0;

	shm_unlink (spawn_shared_name);
	semctl (spawn_shared_sem, 0, IPC_RMID, dummy);
}
#endif /* HAVE_POSIX_SHM */

static void
clean_up (void)
{
#ifdef HAVE_POSIX_SHM
	spawn_clean();
#endif
	pid_file_clean (pid_file_path);
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
		else if (errno == ECHILD) 
			return ret_ok;
		else
			return ret_error;
	}

	if (WIFEXITED(exitcode)) {
		int re = WEXITSTATUS(exitcode);

		if (re == EXIT_OK_ONCE)
			exit (EXIT_OK_ONCE);

		/* Child terminated normally */ 
		PRINT_MSG ("PID %d: exited re=%d\n", pid, re);
		if (re != 0) 
			return ret_error;
	} 
	else if (WIFSIGNALED(exitcode)) {
		/* Child process terminated by a signal */
		PRINT_MSG ("PID %d: received a signal=%d\n", pid, WTERMSIG(exitcode));
	}

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
	pid_t pid;

	/* Execute the server
	 */
	pid = fork();
	if (pid == 0) {
		argv[0] = (char *) path;
		execvp (path, argv);

		printf ("ERROR: Could not execute %s\n", path);
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
	pid_file_path    = figure_pid_file_path (config_file_path);
	worker_uid       = figure_worker_uid (config_file_path);

	/* Turn into a daemon
	 */
	if (! single_time) {
		may_daemonize (argc, argv);
		pid_file_save (pid_file_path, getpid());
	}

	/* Launch the spawning thread
	 */
#ifdef HAVE_POSIX_SHM
	if (! single_time) {
		ret = spawn_init();
		if (ret != ret_ok) {
			PRINT_MSG_S ("Couldn't initialize spawn mechanism.\n");
			return ret_error;
		}
	}
#endif

	while (true) {
		graceful_restart = false;

		pid = process_launch (cherokee_worker, argv);
		if (pid < 0) {
			PRINT_MSG ("Couldn't launch '%s'\n", cherokee_worker);
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

	return EXIT_OK;
}
