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

#if !defined (CHEROKEE_INSIDE_CHEROKEE_H) && !defined (CHEROKEE_COMPILATION)
# error "Only <cherokee/cherokee.h> can be included directly, this file may disappear or change contents."
#endif


#ifndef CHEROKEE_VERSION_H
#define CHEROKEE_VERSION_H

#include <cherokee/common.h>
#include <cherokee/buffer.h>

CHEROKEE_BEGIN_DECLS


typedef enum {
	cherokee_version_product,
	cherokee_version_minor,  
	cherokee_version_minimal,
	cherokee_version_os,
	cherokee_version_full
} cherokee_server_token_t;

ret_t cherokee_version_add        (cherokee_buffer_t *buf, cherokee_server_token_t level);
ret_t cherokee_version_add_w_port (cherokee_buffer_t *buf, cherokee_server_token_t level, cuint_t port);
ret_t cherokee_version_add_simple (cherokee_buffer_t *buf, cherokee_server_token_t level);


CHEROKEE_END_DECLS

#endif /* CHEROKEE_VERSION_H */
