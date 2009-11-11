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

#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <signal.h>
#include <sys/stat.h>

#include "rrd_tools.h"
#include "virtual_server.h"
#include "util.h"

#define ELAPSE_UPDATE     60 
#define ENTRIES "rrd"


/* Global RRDtool connection instance
 */
cherokee_rrd_connection_t *rrd_connection = NULL;

cherokee_collector_rrd_interval_t cherokee_rrd_intervals[] = {
	{ "1h", "1 Hour",       ( 1 * 60 * 60) / 580},
	{ "6h", "6 Hours",      ( 6 * 60 * 60) / 580},
	{ "1d", "1 Day",        (24 * 60 * 60) / 580},
	{ "1w", "1 Week",   (7 * 24 * 60 * 60) / 580},
	{ "1m", "1 Month", (28 * 24 * 60 * 60) / 580},
	{ NULL, NULL,       0}
};


static ret_t
cherokee_rrd_connection_init (cherokee_rrd_connection_t *rrd_conn)
{
	rrd_conn->write_fd = -1;
	rrd_conn->read_fd  = -1;
	rrd_conn->pid      = -1;

	cherokee_buffer_init (&rrd_conn->tmp);
	cherokee_buffer_init (&rrd_conn->path_rrdtool);
	cherokee_buffer_init (&rrd_conn->path_databases);

	CHEROKEE_MUTEX_INIT (&rrd_conn->mutex, NULL);

	return ret_ok;
}


ret_t
cherokee_rrd_connection_get (cherokee_rrd_connection_t **rrd_conn)
{
	if (rrd_connection == NULL) {
		rrd_connection = malloc (sizeof(cherokee_rrd_connection_t));
		if (unlikely (rrd_connection == NULL)) {
			return ret_error;
		}

		cherokee_rrd_connection_init (rrd_connection);
	}

	if (rrd_conn != NULL) {
		*rrd_conn = rrd_connection;
	}

	return ret_ok;
}


ret_t
cherokee_rrd_connection_configure (cherokee_rrd_connection_t *rrd_conn,
				   cherokee_config_node_t    *config)
{
	ret_t                   ret;
	cherokee_config_node_t *subconf;

	/* RRDtool binary
	 */
	ret = cherokee_config_node_get (config, "rrdtool_path", &subconf);
	if (ret == ret_ok) {
		cherokee_buffer_add_buffer (&rrd_conn->path_rrdtool, &subconf->val);
	} else {
		ret = cherokee_find_exec_in_path ("rrdtool", &rrd_conn->path_rrdtool);
		if (ret != ret_ok) {
			LOG_ERROR (CHEROKEE_ERROR_RRD_NO_BINARY, getenv("PATH"));
			return ret_error;
		}
	}

	/* RRDtool databases directory
	 */
	ret = cherokee_config_node_get (config, "database_dir", &subconf);
	if (ret == ret_ok) {
		cherokee_buffer_add_buffer (&rrd_conn->path_databases, &subconf->val);
	} else {
		cherokee_buffer_add_str (&rrd_conn->path_databases, CHEROKEE_RRD_DIR);
	}

	return ret_ok;
}


ret_t
cherokee_rrd_connection_mrproper (cherokee_rrd_connection_t *rrd_conn)
{
	CHEROKEE_MUTEX_DESTROY (&rrd_conn->mutex);

	cherokee_buffer_mrproper (&rrd_conn->tmp);
	cherokee_buffer_mrproper (&rrd_conn->path_databases);
	cherokee_buffer_mrproper (&rrd_conn->path_rrdtool);

	return cherokee_rrd_connection_kill (rrd_conn, true);
}


ret_t
cherokee_rrd_connection_spawn (cherokee_rrd_connection_t *rrd_conn)
{
#ifdef HAVE_FORK
	int    re;
        pid_t  pid;
	char  *argv[3];
	int    fds_to[2];
        int    fds_from[2];
	
	/* There might be a live process */
	if ((rrd_conn->write_fd != -1) &&
	    (rrd_conn->read_fd != -1) &&
	    (rrd_conn->pid != -1))
	{
		return ret_ok;
	}

	TRACE (ENTRIES, "Spawning a new RRDtool instance: %s -\n", rrd_conn->path_rrdtool.buf);

	/* Create communication pipes */
	re = pipe(fds_to);
	if (re != 0) {
		return ret_error;
	}

	re = pipe(fds_from);
	if (re != 0) {
		return ret_error;
	}

	/* Spawn the new child process */
	pid = fork();
	switch (pid) {
        case 0:
		argv[0] = rrd_conn->path_rrdtool.buf;
		argv[1] = (char *) "-";
		argv[2] = NULL;

		/* Move stdout to fd_from[1] */
		dup2 (fds_from[1], STDOUT_FILENO);
                close (fds_from[1]);
                close (fds_from[0]);

                /* Move the stdin to fd_to[0] */
                dup2 (fds_to[0], STDIN_FILENO);
                close (fds_to[0]);
                close (fds_to[1]);

		/* Execute it */
		re = execv(argv[0], argv);

		LOG_ERRNO (errno, cherokee_err_error, CHEROKEE_ERROR_RRD_EXECV, argv[0]);
		exit (EXIT_ERROR);

        case -1:
		LOG_ERRNO (errno, cherokee_err_error, CHEROKEE_ERROR_RRD_FORK, pid);
                break;

        default:
                close (fds_from[1]);
                close (fds_to[0]);
			 
                rrd_conn->write_fd = fds_to[1];
                rrd_conn->read_fd  = fds_from[0];
                rrd_conn->pid      = pid;

                fcntl (rrd_conn->write_fd, F_SETFD, FD_CLOEXEC);
                fcntl (rrd_conn->read_fd,  F_SETFD, FD_CLOEXEC);
		break;
	}

	return ret_ok;
#else
	return ret_error;
#endif
}


ret_t
cherokee_rrd_connection_kill (cherokee_rrd_connection_t *rrd_conn,
			      cherokee_boolean_t         do_kill)
{
	int status;

	if (rrd_conn->pid != -1) {
		if (do_kill) {
			kill (rrd_conn->pid, SIGINT);
		}
		waitpid (rrd_conn->pid, &status, 0);
		rrd_conn->pid = -1;
	}

	if (rrd_conn->write_fd) {
		close (rrd_conn->write_fd);
		rrd_conn->write_fd = -1;
	}

	if (rrd_conn->read_fd) {
		close (rrd_conn->read_fd);
		rrd_conn->read_fd = -1;
	}

	return ret_ok;
}


static ret_t
read_rrdtool (cherokee_rrd_connection_t *rrd_conn,
	      cherokee_buffer_t         *buffer)
{
	ret_t  ret;
	size_t got;

	do {
		ret = cherokee_buffer_read_from_fd (buffer, rrd_conn->read_fd,
						    DEFAULT_RECV_SIZE, &got);
	} while (ret == ret_eagain);

	return ret;
}


static ret_t
write_rrdtool (cherokee_rrd_connection_t *rrd_conn,
	       cherokee_buffer_t         *buffer)
{
	ssize_t written;

	while (true) {
		written = write (rrd_conn->write_fd, buffer->buf, buffer->len);
		if (written >= (ssize_t) buffer->len) {
			cherokee_buffer_clean (buffer);
			return ret_ok;
				    
		} else if (written > 0) {
			cherokee_buffer_move_to_begin (buffer, written);
			continue;

		} else {
			switch (errno) {
			case EINTR:
				continue;
			default:
				return ret_error;
			}
		}
	}

	SHOULDNT_HAPPEN;
	return ret_error;
}


ret_t
cherokee_rrd_connection_execute (cherokee_rrd_connection_t *rrd_conn,
				 cherokee_buffer_t         *buf)
{
	ret_t ret;

	TRACE (ENTRIES, "Sending to RRDtool: %s\n", buf->buf);

	/* Spawn rrdtool
	 */
	ret = cherokee_rrd_connection_spawn (rrd_conn);
	if (unlikely (ret != ret_ok)) {
		return ret_error;
	}

	/* Write command
	 */
	ret = write_rrdtool (rrd_conn, buf);
	if (unlikely (ret != ret_ok)) {
		return ret_error;
	}

	/* Read response
	 */
	cherokee_buffer_clean (buf);

	ret = read_rrdtool (rrd_conn, buf);
	if (unlikely (ret != ret_ok)) {
		cherokee_rrd_connection_kill (rrd_conn, false);
		return ret_error;
	}

	return ret_ok;
}


static cherokee_boolean_t
ensure_db_exists (cherokee_buffer_t *path_database)
{
	int          re;
	struct stat  info;
	char        *slash;

	/* It exists
	 */
	re = cherokee_stat (path_database->buf, &info);
	if ((re == 0) && (info.st_size > 0)) {
		return true;
	}

	/* Write access
	 */
	slash = strrchr (path_database->buf, '/');
	if (slash == NULL) {
		return false;
	}

	*slash = '\0';
	re = access (path_database->buf, W_OK);
	if (re != 0) {
		LOG_ERRNO (errno, cherokee_err_error, CHEROKEE_ERROR_RRD_WRITE, path_database->buf);
		return false;
	}
	*slash = '/';

	return false;
}


static ret_t
create_dirs (cherokee_rrd_connection_t *rrd_conn)
{
	int re;

	cherokee_buffer_add_str (&rrd_conn->path_databases, "/images");

	re = access (rrd_conn->path_databases.buf, W_OK);
	if (re != 0) {
		cherokee_mkdir (rrd_conn->path_databases.buf, 0775);

		re = access (rrd_conn->path_databases.buf, W_OK);
		if (re != 0) {
			LOG_CRITICAL (CHEROKEE_ERROR_RRD_MKDIR_WRITE, rrd_conn->path_databases.buf);
			goto error;
		}
	}

	cherokee_buffer_drop_ending (&rrd_conn->path_databases, 7);
	return ret_ok;

error:
	cherokee_buffer_drop_ending (&rrd_conn->path_databases, 7);
	return ret_error;
}


ret_t
cherokee_rrd_connection_create_srv_db (cherokee_rrd_connection_t *rrd_conn)
{
	ret_t              ret;
	cherokee_boolean_t exist;
	cherokee_buffer_t  tmp    = CHEROKEE_BUF_INIT;
	cherokee_buffer_t  dbname = CHEROKEE_BUF_INIT;

	/* Ensure directories are accessible
	 */
	create_dirs (rrd_conn);

	/* Check the Server database
	 */
	cherokee_buffer_add_buffer (&dbname, &rrd_conn->path_databases);
	cherokee_buffer_add_str    (&dbname, "/server.rrd");

	exist = ensure_db_exists (&dbname);
	if (exist) {
		return ret_ok;
	}

	cherokee_buffer_add_str    (&tmp, "create ");
	cherokee_buffer_add_buffer (&tmp, &dbname);
	cherokee_buffer_add_str    (&tmp, " --step ");
	cherokee_buffer_add_long10 (&tmp, ELAPSE_UPDATE);
	cherokee_buffer_add_str    (&tmp, " ");

	/* Data Sources */
	cherokee_buffer_add_va     (&tmp, "DS:Accepts:ABSOLUTE:%d:U:U ",  ELAPSE_UPDATE*10);
	cherokee_buffer_add_va     (&tmp, "DS:Timeouts:ABSOLUTE:%d:U:U ", ELAPSE_UPDATE*10);
	cherokee_buffer_add_va     (&tmp, "DS:RX:ABSOLUTE:%d:U:U ", ELAPSE_UPDATE*10);
	cherokee_buffer_add_va     (&tmp, "DS:TX:ABSOLUTE:%d:U:U ", ELAPSE_UPDATE*10);

	/* Archives */
	cherokee_buffer_add_str    (&tmp, "RRA:AVERAGE:0.5:1:600 ");
	cherokee_buffer_add_str    (&tmp, "RRA:AVERAGE:0.5:6:700 ");
	cherokee_buffer_add_str    (&tmp, "RRA:AVERAGE:0.5:24:775 ");
	cherokee_buffer_add_str    (&tmp, "RRA:AVERAGE:0.5:288:797 ");

	cherokee_buffer_add_str    (&tmp, "RRA:MAX:0.5:1:600 ");
	cherokee_buffer_add_str    (&tmp, "RRA:MAX:0.5:6:700 ");
	cherokee_buffer_add_str    (&tmp, "RRA:MAX:0.5:24:775 ");
	cherokee_buffer_add_str    (&tmp, "RRA:MAX:0.5:288:797 ");

	cherokee_buffer_add_str    (&tmp, "RRA:MIN:0.5:1:600 ");
	cherokee_buffer_add_str    (&tmp, "RRA:MIN:0.5:6:700 ");
	cherokee_buffer_add_str    (&tmp, "RRA:MIN:0.5:24:775 ");
	cherokee_buffer_add_str    (&tmp, "RRA:MIN:0.5:288:797 ");	   
	cherokee_buffer_add_str    (&tmp, "\n");

	/* Exec */
	TRACE (ENTRIES, "Creating RRDtool server database: %s\n", tmp.buf);

	ret = cherokee_rrd_connection_spawn (rrd_conn);
	if (unlikely (ret != ret_ok)) {
		return ret_error;
	}

	ret = cherokee_rrd_connection_execute (rrd_conn, &tmp);
	if (unlikely (ret != ret_ok)) {
		return ret_error;
	}

	cherokee_buffer_mrproper (&dbname);
	cherokee_buffer_mrproper (&tmp);
	return ret_ok;
}


ret_t
cherokee_rrd_connection_create_vsrv_db (cherokee_rrd_connection_t *rrd_conn,
					cherokee_buffer_t         *dbpath)
{
	ret_t              ret;
	cherokee_boolean_t exist;
	cherokee_buffer_t  tmp    = CHEROKEE_BUF_INIT;

	/* Ensure directories are accessible
	 */
	create_dirs (rrd_conn);

	/* Check the Server database
	 */
	exist = ensure_db_exists (dbpath);
	if (exist) {
		return ret_ok;
	}

	cherokee_buffer_add_str    (&tmp, "create ");
	cherokee_buffer_add_buffer (&tmp, dbpath);
	cherokee_buffer_add_str    (&tmp, " --step ");
	cherokee_buffer_add_long10 (&tmp, ELAPSE_UPDATE);
	cherokee_buffer_add_str    (&tmp, " ");

	/* Data Sources */
	cherokee_buffer_add_va     (&tmp, "DS:RX:ABSOLUTE:%d:U:U ", ELAPSE_UPDATE*10);
	cherokee_buffer_add_va     (&tmp, "DS:TX:ABSOLUTE:%d:U:U ", ELAPSE_UPDATE*10);

	/* Archives */
	cherokee_buffer_add_str    (&tmp, "RRA:AVERAGE:0.5:1:600 ");
	cherokee_buffer_add_str    (&tmp, "RRA:AVERAGE:0.5:6:700 ");
	cherokee_buffer_add_str    (&tmp, "RRA:AVERAGE:0.5:24:775 ");
	cherokee_buffer_add_str    (&tmp, "RRA:AVERAGE:0.5:288:797 ");

	cherokee_buffer_add_str    (&tmp, "RRA:MAX:0.5:1:600 ");
	cherokee_buffer_add_str    (&tmp, "RRA:MAX:0.5:6:700 ");
	cherokee_buffer_add_str    (&tmp, "RRA:MAX:0.5:24:775 ");
	cherokee_buffer_add_str    (&tmp, "RRA:MAX:0.5:288:797 ");

	cherokee_buffer_add_str    (&tmp, "RRA:MIN:0.5:1:600 ");
	cherokee_buffer_add_str    (&tmp, "RRA:MIN:0.5:6:700 ");
	cherokee_buffer_add_str    (&tmp, "RRA:MIN:0.5:24:775 ");
	cherokee_buffer_add_str    (&tmp, "RRA:MIN:0.5:288:797 ");	   
	cherokee_buffer_add_str    (&tmp, "\n");

	/* Exec */
	TRACE (ENTRIES, "Creating RRDtool vserver database: %s\n", tmp.buf);

	ret = cherokee_rrd_connection_spawn (rrd_conn);
	if (unlikely (ret != ret_ok)) {
		return ret_error;
	}

	ret = cherokee_rrd_connection_execute (rrd_conn, &tmp);
	if (unlikely (ret != ret_ok)) {
		return ret_error;
	}

	cherokee_buffer_mrproper (&tmp);
	return ret_ok;
}
