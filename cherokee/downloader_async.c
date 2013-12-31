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
#include "downloader_async.h"
#include "downloader-protected.h"
#include "socket.h"
#include "fdpoll.h"
#include "util.h"

#define ENTRIES     "adownloader,async"
#define WATCH_SLEEP 1000

struct cherokee_downloader_async {
	cherokee_downloader_t  downloader;
	cherokee_fdpoll_t     *fdpoll_ref;
	int                    fd_added;
};



ret_t
cherokee_downloader_async_init (cherokee_downloader_async_t *adownloader)
{
	ret_t ret;

	ret = cherokee_downloader_init (DOWNLOADER(adownloader));
	if (ret != ret_ok) return ret;

	adownloader->fd_added   = -1;
	adownloader->fdpoll_ref = NULL;
	return ret_ok;
}


ret_t
cherokee_downloader_async_mrproper (cherokee_downloader_async_t *adownloader)
{
	if (adownloader->fd_added != -1)
		cherokee_fdpoll_del (adownloader->fdpoll_ref, adownloader->fd_added);

	cherokee_downloader_mrproper (DOWNLOADER(adownloader));
	return ret_ok;
}


CHEROKEE_ADD_FUNC_NEW (downloader_async);
CHEROKEE_ADD_FUNC_FREE (downloader_async);


ret_t
cherokee_downloader_async_set_fdpoll (cherokee_downloader_async_t *adownloader, cherokee_fdpoll_t *fdpoll)
{
	adownloader->fdpoll_ref = fdpoll;
	return ret_ok;
}


ret_t
cherokee_downloader_async_connect (cherokee_downloader_async_t *adownloader)
{
	ret_t                  ret;
	cherokee_downloader_t *down   = DOWNLOADER(adownloader);
	cherokee_fdpoll_t     *fdpoll = adownloader->fdpoll_ref;

	if (fdpoll == NULL)
		return ret_error;

	ret = cherokee_downloader_connect (down);
	if (ret != ret_ok) return ret;

	ret = cherokee_fd_set_nonblocking (down->socket.socket, true);
	if (ret != ret_ok) return ret;

	ret = cherokee_fdpoll_add (fdpoll, down->socket.socket, FDPOLL_MODE_WRITE);
	if (ret != ret_ok) return ret;

	adownloader->fd_added = down->socket.socket;
	return ret_ok;
}


ret_t
cherokee_downloader_async_step (cherokee_downloader_async_t *adownloader)
{
	int                    re;
	int                    fd;
	int                    rw;
	cherokee_downloader_t *down   = DOWNLOADER(adownloader);
	cherokee_fdpoll_t     *fdpoll = adownloader->fdpoll_ref;

	TRACE(ENTRIES, "Enters obj=%p fdpoll=%p\n", adownloader, fdpoll);
	return_if_fail ((fdpoll != NULL), ret_error);

	/* Watch the fd poll
	 */
	re = cherokee_fdpoll_watch (fdpoll, WATCH_SLEEP);
	TRACE(ENTRIES, "watch = %d\n", re);
	if (re <= 0)
		return ret_eagain;

	/* Check whether we are reading or writting
	 */
	if (down->phase <= downloader_phase_send_post)
		rw = FDPOLL_MODE_WRITE;
	else
		rw = FDPOLL_MODE_READ;

	TRACE(ENTRIES, "rw = %d\n", rw);

	/* Check the downloader fd
	 */
	fd = down->socket.socket;
	re = cherokee_fdpoll_check (fdpoll, fd, rw);
	switch (re) {
	case -1:
		PRINT_ERROR("Error polling fd=%d rw=%d\n", fd, rw);
		return ret_error;
	case 0:
		TRACE(ENTRIES, "fd=%d rw=%d, not ready\n", fd, rw);
/*		return ret_eagain; */
	}

	/* If there's something to do, go for it..
	 */
	TRACE(ENTRIES, "fd=%d rw=%d is active\n", fd, rw);
	return cherokee_downloader_step (down, NULL, NULL);
}
