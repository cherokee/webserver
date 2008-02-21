/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/* Cherokee Benchmarker
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

#if !defined (CHEROKEE_INSIDE_CHEROKEE_H) && !defined (CHEROKEE_COMPILATION)
# error "Only <cherokee/cherokee.h> can be included directly, this file may disappear or change contents."
#endif

#ifndef CHEROKEE_URL_H
#define CHEROKEE_URL_H

#include <stdlib.h>
#include <cherokee/common.h>
#include <cherokee/list.h>
#include <cherokee/buffer.h>

CHEROKEE_BEGIN_DECLS


typedef struct {	
	cherokee_buffer_t  host;
	cuint_t            port;
	cherokee_buffer_t  request;
	
	enum {
		http,
		https
	}                  protocol;

} cherokee_url_t;

#define URL(u)           ((cherokee_url_t *)(u))
#define URL_PORT(u)      (URL(u)->port)
#define URL_HOST(u)      (&URL(u)->host)
#define URL_REQUEST(u)   (&URL(u)->request)


ret_t cherokee_url_init     (cherokee_url_t *url);
ret_t cherokee_url_clean    (cherokee_url_t *url);
ret_t cherokee_url_mrproper (cherokee_url_t *url);

ret_t cherokee_url_parse        (cherokee_url_t *url, cherokee_buffer_t *string, cherokee_buffer_t *user_ret, cherokee_buffer_t *password_ret);
ret_t cherokee_url_build_string (cherokee_url_t *url, cherokee_buffer_t *buf);

ret_t cherokee_url_print (cherokee_url_t *url);


CHEROKEE_END_DECLS

#endif /* CHEROKEE_URL_H */

