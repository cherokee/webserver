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


ret_t 
cherokee_logger_writer_init (cherokee_logger_writer_t *writer)
{
	writer->type        = cherokee_logger_writer_syslog;
	writer->fd          = -1;
	writer->max_bufsize = DEFAULT_LOGGER_MAX_BUFSIZE;

	cherokee_buffer_init (&writer->command);
	cherokee_buffer_init (&writer->filename);
	cherokee_buffer_init (&writer->buffer);

	cherokee_buffer_ensure_size (&writer->buffer, writer->max_bufsize);

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
cherokee_logger_writer_mrproper (cherokee_logger_writer_t *writer)
{
	ret_t ret;

	ret = logger_writer_close_file (writer);

	cherokee_buffer_mrproper (&writer->buffer);
	cherokee_buffer_mrproper (&writer->filename);
	cherokee_buffer_mrproper (&writer->command);

	return ret;
}


ret_t 
cherokee_logger_writer_configure (cherokee_logger_writer_t *writer, cherokee_config_node_t *config)
{
	ret_t              ret;
	cherokee_buffer_t *tmp = NULL;

	/* Check the type
	 */
	ret = cherokee_config_node_read (config, "type", &tmp);
	if (ret != ret_ok) {
		PRINT_MSG_S ("Logger writer type is needed\n");
		return ret_ok;
	}
	
	if (equal_buf_str (tmp, "syslog")) {
		writer->type = cherokee_logger_writer_syslog;
	} else if (equal_buf_str (tmp, "stderr")) {
		writer->type = cherokee_logger_writer_stderr;
	} else if (equal_buf_str (tmp, "file")) {
		writer->type = cherokee_logger_writer_file;				
	} else if (equal_buf_str (tmp, "exec")) {
		writer->type = cherokee_logger_writer_pipe;		
	} else {
		PRINT_MSG ("Unknown logger writer type '%s'\n", tmp->buf);
		return ret_error;
	}	

	/* Extra properties
	 */
	switch (writer->type) {
	case cherokee_logger_writer_file:
		ret = cherokee_config_node_read (config, "filename", &tmp);
		if (ret != ret_ok) { 
			PRINT_MSG_S ("Logger writer (file): Couldn't read the filename\n");
			return ret_error;
		}
		cherokee_buffer_add_buffer (&writer->filename, tmp);
		break;

	case cherokee_logger_writer_pipe:
		ret = cherokee_config_node_read (config, "command", &tmp);
		if (ret != ret_ok) { 
			PRINT_MSG_S ("Logger writer (exec): Couldn't read the command\n");
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
		int buf_len = atoi (tmp->buf);
		
		if (buf_len < LOGGER_MIN_BUFSIZE)
			buf_len = LOGGER_MIN_BUFSIZE;
		else if (buf_len > LOGGER_MAX_BUFSIZE)
			buf_len = LOGGER_MAX_BUFSIZE;
		
		cherokee_buffer_mrproper (&writer->buffer);
		cherokee_buffer_init (&writer->buffer);
		
		ret = cherokee_buffer_ensure_size (&writer->buffer, buf_len);
		if (ret != ret_ok) {
			PRINT_ERROR ("Allocation logger->max_bufsize " FMT_SIZE " failed !\n", 
				     (CST_SIZE) writer->max_bufsize);
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

	if (pipe (to_log_fds)) { 
		PRINT_ERRNO (errno, "Pipe error: '${errno}'");
		return ret_error;
	}

	switch (pid = fork()) { 
	case 0: 
		/* Child 
		 */
		close (STDIN_FILENO); 
		dup2 (to_log_fds[0], STDIN_FILENO);
		close (to_log_fds[0]); 
		close (to_log_fds[1]); 

		for (fd = 3; fd < 256; fd++)
			close (fd);

		execl("/bin/sh", "sh", "-c", writer->command.buf, NULL);

		SHOULDNT_HAPPEN;

	case -1:
		PRINT_ERRNO (errno, "Fork failed: '${errno}'");
		break;

	default:
		close(to_log_fds[0]);
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
		return ret_ok;

	case cherokee_logger_writer_pipe:
		return launch_logger_process (writer);

	case cherokee_logger_writer_stderr:
		writer->fd = STDERR_FILENO;
		return ret_ok;

	case cherokee_logger_writer_file:
		writer->fd = open (writer->filename.buf, O_APPEND | O_WRONLY | O_CREAT | O_LARGEFILE | O_NOFOLLOW, 0640);
		if (writer->fd == -1) {
			PRINT_MSG ("Couldn't open '%s' for appending\n", writer->filename.buf);
			return ret_error;
		}

		ret = cherokee_fd_set_closexec (writer->fd);
		if (ret != ret_ok)
			return ret;

		return ret_ok;

	default:
		SHOULDNT_HAPPEN;
	}

	return ret_error;
}


ret_t
cherokee_logger_writer_reopen (cherokee_logger_writer_t *writer)
{
	ret_t ret;

	switch (writer->type) {
	case cherokee_logger_writer_syslog:
		return ret_ok;

	case cherokee_logger_writer_file:
	case cherokee_logger_writer_pipe:
	case cherokee_logger_writer_stderr:
		ret = logger_writer_close_file (writer);
		break;

	default:
		SHOULDNT_HAPPEN;
		return ret_error;
	}

	ret = cherokee_logger_writer_open (writer);
	if (ret != ret_ok) return ret;

	return ret_ok;
}


ret_t 
cherokee_logger_writer_get_buf (cherokee_logger_writer_t *writer, cherokee_buffer_t **buf)
{
	*buf = &writer->buffer;
	return ret_ok;
}


ret_t 
cherokee_logger_writer_flush (cherokee_logger_writer_t *writer)
{
	ret_t ret = ret_ok;

	/* The internal buffer might be empty
	 */
	if (cherokee_buffer_is_empty (&writer->buffer))
		return ret_ok;

	/* If not, do the proper thing
	 */
	switch (writer->type) {
	case cherokee_logger_writer_stderr:
		/* In this case we ignore errors.
		 */
		if (fwrite (writer->buffer.buf, 1, writer->buffer.len, stderr)
			!= (size_t) writer->buffer.len)
			ret = ret_error;
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

		/* If there is at least 1 page to write
		 * then round down the length to speed up write(s).
		 */
		if (buflen > LOGGER_BUF_PAGESIZE)
			buflen &= ~LOGGER_BUF_PAGESIZE;
		do {
			nwr = write (writer->fd, writer->buffer.buf, buflen);
		} while (nwr == -1 && errno == EINTR);
		if (nwr <= 0) {
			/* If an error occured in blocking write,
			 * then cleanup the log buffer now
			 * because we don't want to let it grow too much.
			 */
			cherokee_buffer_clean (&writer->buffer);
			return ret_error;
		}
		/* OK, something has been written.
		 */
		cherokee_buffer_move_to_begin (&writer->buffer, nwr);
		if (! cherokee_buffer_is_empty (&writer->buffer))
			return ret_eagain;
		}
		break;

	case cherokee_logger_writer_syslog:
		/* Write to syslog the whole log buffer,
		 * then cleanup it in any case.
		 */
		ret = cherokee_syslog (LOG_INFO, &writer->buffer);
		cherokee_buffer_clean (&writer->buffer);
		break;

	default:
		SHOULDNT_HAPPEN;
		return ret_error;
	}

	return ret;
}

