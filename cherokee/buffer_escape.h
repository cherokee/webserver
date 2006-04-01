/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/* Cherokee
 *
 * Authors:
 *      Alvaro Lopez Ortega <alvaro@alobbs.com>
 *
 * Copyright (C) 2001-2006 Alvaro Lopez Ortega
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

#ifndef __CHEROKEE_BUFFER_ESCAPE_H__
#define __CHEROKEE_BUFFER_ESCAPE_H__

#include "buffer.h"


typedef enum {
	escape_html,
	ESCAPE_NUM
} cherokee_escpe_types_t;

typedef struct {
	cherokee_buffer_t *reference;
	cherokee_buffer_t *internal [ESCAPE_NUM];
	
	enum {
		unchecked,
		use_internal,
		use_reference
	} state [ESCAPE_NUM];
} cherokee_buffer_escape_t;


ret_t cherokee_buffer_escape_new   (cherokee_buffer_escape_t **esc);
ret_t cherokee_buffer_escape_free  (cherokee_buffer_escape_t  *esc);
ret_t cherokee_buffer_escape_clean (cherokee_buffer_escape_t  *esc);

ret_t cherokee_buffer_escape_set_ref  (cherokee_buffer_escape_t *esc, cherokee_buffer_t  *buf_ref);
ret_t cherokee_buffer_escape_get_html (cherokee_buffer_escape_t *esc, cherokee_buffer_t **buf_ref);

#endif /* __CHEROKEE_BUFFER_ESCAPE_H__ */
