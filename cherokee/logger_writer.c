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
#include "logger_writer.h"
#include "util.h"

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>

#ifdef HAVE_SYSLOG_H
# include <syslog.h>
#endif

typedef struct {
	CHEROKEE_MUTEX_T  (mutex);
	int                foo;
} priv_t;

#define PRIV(l) ((priv_t *)(l->priv))


ret_t
cherokee_logger_writer_new (cherokee_logger_writer_t **writer)
{
	CHEROKEE_NEW_STRUCT(n,logger_writer);

	INIT_LIST_HEAD (&n->listed);

	n->type        = cherokee_logger_writer_syslog;
	n->fd          = -1;
	n->max_bufsize = DEFAULT_LOGGER_MAX_BUFSIZE;

	cherokee_buffer_init (&n->command);
	cherokee_buffer_init (&n->filename);
	cherokee_buffer_init (&n->buffer);

	cherokee_buffer_ensure_size (&n->buffer, n->max_bufsize);

	n->priv = malloc (sizeof(priv_t));
	if (n->priv == NULL) {
		cherokee_buffer_mrproper (&n->buffer);
		free(n);
		return ret_nomem;
	}

	CHEROKEE_MUTEX_INIT (&PRIV(n)->mutex, NULL);
	n->initialized = false;

	*writer = n;
	return ret_ok;
}


ret_t
cherokee_logger_writer_new_stderr (cherokee_logger_writer_t **writer)
{
	ret_t                     ret;
	cherokee_logger_writer_t *n;

	ret = cherokee_logger_writer_new (&n);
	if (ret != ret_ok) {
		return ret_error;
	}

	n->type        = cherokee_logger_writer_stderr;
	n->max_bufsize = 0;

	*writer = n;
	return ret_ok;
}


static ret_t
logger_writer_close_file (cherokee_logger_writer_t *writer)
{
	ret_t ret = ret_ok;

	if (writer->fd != -1) {
		if (writer->type != cherokee_logger_writer_stderr) {
			if (cherokee_fd_close (writer->fd) != 0)
				ret = ret_error;
		}
		writer->fd = -1;
	}

	return ret;
}


ret_t
cherokee_logger_writer_free (cherokee_logger_writer_t *writer)
{
	logger_writer_close_file (writer);

	cherokee_buffer_mrproper (&writer->buffer);
	cherokee_buffer_mrproper (&writer->filename);
	cherokee_buffer_mrproper (&writer->command);

	CHEROKEE_MUTEX_DESTROY (&PRIV(writer)->mutex);

	free (writer->priv);
	free (writer);

	return ret_ok;
}

static ret_t
config_read_type (cherokee_config_node_t         *config,
                  cherokee_logger_writer_types_t *type)
{
	ret_t              ret;
	cherokee_buffer_t *tmp = NULL;

	ret = cherokee_config_node_read (config, "type", &tmp);
	if (ret != ret_ok) {
		LOG_ERROR_S (CHEROKEE_ERROR_LOGGER_NO_WRITER);
		return ret_error;
	}

	if (equal_buf_str (tmp, "syslog")) {
		*type = cherokee_logger_writer_syslog;
	} else if (equal_buf_str (tmp, "stderr")) {
		*type = cherokee_logger_writer_stderr;
	} else if (equal_buf_str (tmp, "file")) {
		*type = cherokee_logger_writer_file;
	} else if (equal_buf_str (tmp, "exec")) {
		*type = cherokee_logger_writer_pipe;
	} else {
		LOG_CRITICAL (CHEROKEE_ERROR_LOGGER_WRITER_UNKNOWN, tmp->buf);
		return ret_error;
	}

	return ret_ok;
}


ret_t
cherokee_logger_writer_configure (cherokee_logger_writer_t *writer, cherokee_config_node_t *config)
{
	ret_t              ret;
	cherokee_buffer_t *tmp = NULL;

	/* Check the type
	 */
	ret = config_read_type (config, &writer->type);
	if (ret != ret_ok) {
		return ret;
	}

	/* Extra properties
	 */
	switch (writer->type) {
	case cherokee_logger_writer_file:
		ret = cherokee_config_node_read (config, "filename", &tmp);
		if (ret != ret_ok) {
			LOG_ERROR (CHEROKEE_ERROR_LOGGER_WRITER_READ, "file");
			return ret_error;
		}
		cherokee_buffer_add_buffer (&writer->filename, tmp);
		break;

	case cherokee_logger_writer_pipe:
		ret = cherokee_config_node_read (config, "command", &tmp);
		if (ret != ret_ok) {
			LOG_ERROR (CHEROKEE_ERROR_LOGGER_WRITER_READ, "exec");
			return ret_error;
		}
		cherokee_buffer_add_buffer (&writer->command, tmp);
		break;
	default:
		break;
	}

	/* Reside the internal buffer if needed
	 */
	ret = cherokee_config_node_read (config, "bufsize", &tmp);
	if (ret == ret_ok) {
		int buf_len;

		ret = cherokee_atoi (tmp->buf, &buf_len);
		if (ret != ret_ok) {
			return ret_error;
		}

		if (buf_len < LOGGER_MIN_BUFSIZE)
			buf_len = LOGGER_MIN_BUFSIZE;
		else if (buf_len > LOGGER_MAX_BUFSIZE)
			buf_len = LOGGER_MAX_BUFSIZE;

		cherokee_buffer_mrproper (&writer->buffer);
		cherokee_buffer_init (&writer->buffer);

		ret = cherokee_buffer_ensure_size (&writer->buffer, buf_len);
		if (ret != ret_ok) {
			LOG_ERROR (CHEROKEE_ERROR_LOGGER_WRITER_ALLOC, writer->max_bufsize);
			return ret_nomem;
		}

		writer->max_bufsize = (size_t)buf_len;
	}

	return ret_ok;
}


static ret_t
launch_logger_process (cherokee_logger_writer_t *writer)
{
#ifdef HAVE_FORK
	int   fd;
	int   to_log_fds[2];
	pid_t pid;

	if (cherokee_pipe (to_log_fds)) {
		LOG_ERRNO (errno, cherokee_err_error, CHEROKEE_ERROR_LOGGER_WRITER_PIPE, errno);
		return ret_error;
	}

	switch (pid = fork()) {
	case 0:
		/* Child
		 */
		cherokee_fd_close (STDIN_FILENO);
		dup2 (to_log_fds[0], STDIN_FILENO);
		cherokee_fd_close (to_log_fds[0]);
		cherokee_fd_close (to_log_fds[1]);

		for (fd = 3; fd < 256; fd++) {
			cherokee_fd_close (fd);
		}

		do {
			execl("/bin/sh", "sh", "-c", writer->command.buf, NULL);
		} while (errno == EINTR);

		SHOULDNT_HAPPEN;

	case -1:
		LOG_ERRNO (errno, cherokee_err_error, CHEROKEE_ERROR_LOGGER_WRITER_FORK, errno);
		break;

	default:
		cherokee_fd_close (to_log_fds[0]);
		writer->fd = to_log_fds[1];
	}
#else
	return ret_no_sys;
#endif
	return ret_ok;
}


ret_t
cherokee_logger_writer_open (cherokee_logger_writer_t *writer)
{
	ret_t ret;

	switch (writer->type) {
	case cherokee_logger_writer_syslog:
		/* Nothing to do, syslog already opened at startup.
		 */
		goto out;

	case cherokee_logger_writer_pipe:
		ret = launch_logger_process (writer);
		if (ret != ret_ok)
			goto error;

		goto out;

	case cherokee_logger_writer_stderr:
		writer->fd = STDERR_FILENO;
		goto out;

	case cherokee_logger_writer_file:
		writer->fd = cherokee_open (writer->filename.buf, O_APPEND | O_WRONLY | O_CREAT | O_LARGEFILE | O_NOFOLLOW, 0640);
		if (writer->fd == -1) {
			LOG_ERROR (CHEROKEE_ERROR_LOGGER_WRITER_APPEND, writer->filename.buf);
			ret = ret_error;
			goto error;
		}

		ret = cherokee_fd_set_closexec (writer->fd);
		if (ret != ret_ok)
			goto error;

		goto out;

	default:
		SHOULDNT_HAPPEN;
		ret = ret_error;
		goto error;
	}

out:
	writer->initialized = true;
	return ret_ok;
error:
	return ret;
}


ret_t
cherokee_logger_writer_reopen (cherokee_logger_writer_t *writer)
{
	ret_t ret;

	CHEROKEE_MUTEX_LOCK (&PRIV(writer)->mutex);

	switch (writer->type) {
	case cherokee_logger_writer_syslog:
		goto out;

	case cherokee_logger_writer_file:
	case cherokee_logger_writer_pipe:
	case cherokee_logger_writer_stderr:
		ret = logger_writer_close_file (writer);
		break;

	default:
		SHOULDNT_HAPPEN;
		goto error;
	}

	ret = cherokee_logger_writer_open (writer);
	if (ret != ret_ok)
		goto error;

out:
	CHEROKEE_MUTEX_UNLOCK (&PRIV(writer)->mutex);
	return ret_ok;
error:
	CHEROKEE_MUTEX_UNLOCK (&PRIV(writer)->mutex);
	return ret_error;
}


ret_t
cherokee_logger_writer_get_buf (cherokee_logger_writer_t *writer, cherokee_buffer_t **buf)
{
	CHEROKEE_MUTEX_LOCK (&PRIV(writer)->mutex);
	*buf = &writer->buffer;

	return ret_ok;
}

ret_t
cherokee_logger_writer_release_buf (cherokee_logger_writer_t *writer)
{
	CHEROKEE_MUTEX_UNLOCK (&PRIV(writer)->mutex);
	return ret_ok;
}


ret_t
cherokee_logger_writer_flush (cherokee_logger_writer_t *writer,
                              cherokee_boolean_t        locked)
{
	int   re;
	ret_t ret = ret_ok;

	/* The internal buffer might be empty
	 */
	if (cherokee_buffer_is_empty (&writer->buffer)) {
		return ret_ok;
	}

	if (!locked) {
		CHEROKEE_MUTEX_LOCK (&PRIV(writer)->mutex);
	}

	/* If not, do the proper thing
	 */
	switch (writer->type) {
	case cherokee_logger_writer_stderr:
		/* In this case we ignore errors.
		 */
		re = fwrite (writer->buffer.buf, 1, writer->buffer.len, stderr);
		if (re != (size_t) writer->buffer.len) {
			ret = ret_error;
		}

		/* Cleanup the log buffer even if there is an error,
		 * because it's safer to go on anyway.
		 */
		cherokee_buffer_clean (&writer->buffer);
		break;

	case cherokee_logger_writer_pipe:
	case cherokee_logger_writer_file:
	{
		ssize_t nwr = 0;
		size_t  buflen = writer->buffer.len;

		/* If there is at least 1 page to write then round
		 * down the length to speed up write(s).
		 */
		if (buflen > LOGGER_BUF_PAGESIZE) {
			buflen &= ~LOGGER_BUF_PAGESIZE;
		}

		do {
			nwr = write (writer->fd, writer->buffer.buf, buflen);
		} while (nwr == -1 && errno == EINTR);

		if (nwr <= 0) {
			/* If an error occured in blocking write, then
			 * cleanup the log buffer now because we don't
			 * want to let it grow too much.
			 */
			cherokee_buffer_clean (&writer->buffer);
			ret = ret_error;
			goto out;
		}

		/* OK, something has been written.
		 */
		cherokee_buffer_move_to_begin (&writer->buffer, nwr);
		if (! cherokee_buffer_is_empty (&writer->buffer)) {
			ret = ret_eagain;
		}
		break;
	}
	case cherokee_logger_writer_syslog:
		/* Write to syslog the whole log buffer, then cleanup
		 * it in any case.
		 */
		ret = cherokee_syslog (LOG_INFO, &writer->buffer);
		cherokee_buffer_clean (&writer->buffer);
		break;

	default:
		SHOULDNT_HAPPEN;
		ret = ret_error;
	}

out:
	if (! locked) {
		CHEROKEE_MUTEX_UNLOCK (&PRIV(writer)->mutex);
	}
	return ret;
}


ret_t
cherokee_logger_writer_get_id (cherokee_config_node_t   *config,
                               cherokee_buffer_t        *id)
{
	ret_t                           ret;
	cherokee_buffer_t              *tmp;
	cherokee_logger_writer_types_t  type;

	ret = config_read_type (config, &type);
	if (ret != ret_ok) {
		return ret;
	}

	switch (type) {
	case cherokee_logger_writer_syslog:
		cherokee_buffer_add_str (id, "syslog");
		break;
	case cherokee_logger_writer_stderr:
		cherokee_buffer_add_str (id, "stderr");
		break;
	case cherokee_logger_writer_file:
		ret = cherokee_config_node_read (config, "filename", &tmp);
		if (ret == ret_ok) {
			cherokee_buffer_add_buffer (id,  tmp);
		} else {
			cherokee_buffer_add_str (id,  "unknown");
		}
		break;
	case cherokee_logger_writer_pipe:
		ret = cherokee_config_node_read (config, "command", &tmp);
		if (ret == ret_ok) {
			cherokee_buffer_add_buffer (id,  tmp);
		} else {
			cherokee_buffer_add_str (id,  "unknown");
		}
		break;
	default:
		SHOULDNT_HAPPEN;
		return ret_error;
	}

	return ret_ok;
}
