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

#ifndef CHEROKEE_DOWNLOADER_PROTECTED_H
#define CHEROKEE_DOWNLOADER_PROTECTED_H


#include "common-internal.h"
#include "buffer.h"
#include "request.h"
#include "socket.h"
#include "header.h"
#include "post.h"


CHEROKEE_BEGIN_DECLS

typedef enum {
	downloader_phase_init,
	downloader_phase_send_headers,
	downloader_phase_send_post,
	downloader_phase_read_headers,
	downloader_phase_step,
	downloader_phase_finished
} cherokee_downloader_phase_t;


struct cherokee_downloader {
	cherokee_header_t           *header;
	cherokee_request_header_t    request;

	cherokee_buffer_t            request_header;
	cherokee_buffer_t            reply_header;
	cherokee_buffer_t            body;

	cherokee_buffer_t            post;
	cherokee_buffer_t            proxy;
	cuint_t                      proxy_port;

	cherokee_socket_t            socket;
	cherokee_sockaddr_t          sockaddr;

	cherokee_downloader_phase_t  phase;
	cherokee_downloader_status_t status;

	cuint_t                      content_length;

	/* Information
	 */
	struct {
		uint32_t request_sent;
		uint32_t headers_recv;
		uint32_t post_sent;
		uint32_t body_recv;
	} info;

	cherokee_buffer_t            tmp1;
	cherokee_buffer_t            tmp2;
};


ret_t cherokee_downloader_init     (cherokee_downloader_t  *downloader);
ret_t cherokee_downloader_mrproper (cherokee_downloader_t  *downloader);

CHEROKEE_END_DECLS

#endif /* CHEROKEE_DOWNLOADER_PROTECTED_H */
