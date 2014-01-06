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

#if !defined (CHEROKEE_INSIDE_CHEROKEE_H) && !defined (CHEROKEE_COMPILATION)
# error "Only <cherokee/cherokee.h> can be included directly, this file may disappear or change contents."
#endif

#ifndef CHEROKEE_DATA_WRITER_H
#define CHEROKEE_DATA_WRITER_H

#include <cherokee/common.h>
#include <cherokee/buffer.h>

#define DWRITER_STACK_LEN 256

CHEROKEE_BEGIN_DECLS

typedef enum {
	dwriter_start,
	dwriter_dict_start,
	dwriter_dict_key,
	dwriter_dict_val,
	dwriter_list_start,
	dwriter_list_in,
	dwriter_complete,
	dwriter_error
} cherokee_dwriter_state_t;

typedef enum {
	dwriter_json,
	dwriter_python,
	dwriter_php,
	dwriter_ruby
} cherokee_dwriter_lang_t;

typedef struct {
	cherokee_buffer_t        *buf;
	cuint_t                   depth;
	cherokee_boolean_t        pretty;
	cherokee_dwriter_state_t  state[DWRITER_STACK_LEN];
	cherokee_dwriter_lang_t   lang;
	cherokee_buffer_t        *tmp;
} cherokee_dwriter_t;

#define DWRITER(x) ((cherokee_dwriter_t *)(x))
#define cherokee_dwriter_cstring(w,s) cherokee_dwriter_string(w, s, sizeof(s)-1)
#define cherokee_dwriter_bstring(w,b) cherokee_dwriter_string(w, (b)->buf, (b)->len)

ret_t cherokee_dwriter_init       (cherokee_dwriter_t *writer, cherokee_buffer_t *tmp);
ret_t cherokee_dwriter_mrproper   (cherokee_dwriter_t *writer);
ret_t cherokee_dwriter_set_buffer (cherokee_dwriter_t *writer, cherokee_buffer_t *output);

ret_t cherokee_dwriter_unsigned   (cherokee_dwriter_t *w, unsigned long int l);
ret_t cherokee_dwriter_integer    (cherokee_dwriter_t *w, long int l);
ret_t cherokee_dwriter_double     (cherokee_dwriter_t *w, double d);
ret_t cherokee_dwriter_number     (cherokee_dwriter_t *w, const char *s, int len);
ret_t cherokee_dwriter_string     (cherokee_dwriter_t *w, const char *s, int len);
ret_t cherokee_dwriter_null       (cherokee_dwriter_t *w);
ret_t cherokee_dwriter_bool       (cherokee_dwriter_t *w, cherokee_boolean_t b);
ret_t cherokee_dwriter_dict_open  (cherokee_dwriter_t *w);
ret_t cherokee_dwriter_dict_close (cherokee_dwriter_t *w);
ret_t cherokee_dwriter_list_open  (cherokee_dwriter_t *w);
ret_t cherokee_dwriter_list_close (cherokee_dwriter_t *w);

/* Helpers */
ret_t cherokee_dwriter_lang_to_type (cherokee_buffer_t *buf, cherokee_dwriter_lang_t *lang);

CHEROKEE_END_DECLS

#endif /* CHEROKEE_DATA_WRITER_H */
