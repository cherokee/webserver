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
#include "dwriter.h"

#define CS  (w->state[w->depth])
#define OUT (w->buf)

#define ADD_SEP                                                              \
	do {                                                                 \
		if ((CS == dwriter_dict_key) ||                              \
		    (CS == dwriter_list_in))                                 \
		{                                                            \
			cherokee_buffer_add_str (OUT, ",");                  \
			if (w->pretty)                                       \
				cherokee_buffer_add_str(OUT, "\n");          \
		} else if (CS == dwriter_dict_val) {                         \
			if ((w->lang == dwriter_php) ||                      \
			    (w->lang == dwriter_ruby))                       \
				cherokee_buffer_add_str (OUT, "=>");         \
			else                                                 \
				cherokee_buffer_add_str (OUT, ":");          \
			if (w->pretty)                                       \
				cherokee_buffer_add_str (OUT, " ");          \
		}                                                            \
	} while(0)

#define ADD_WHITE                                                            \
	do {                                                                 \
		unsigned int i;                                              \
		if (w->pretty) {                                             \
			if (CS != dwriter_dict_val) {                        \
				for (i=0; i<w->depth; i++)                   \
					cherokee_buffer_add_str (OUT, "  "); \
			}                                                    \
		}                                                            \
	} while(0)

#define ADD_END                                                              \
	do {                                                                 \
		switch(CS) {                                                 \
		case dwriter_start:                                          \
			CS = dwriter_complete;                               \
			break;                                               \
		case dwriter_dict_start:                                     \
		case dwriter_dict_key:                                       \
			CS = dwriter_dict_val;                               \
			break;                                               \
		case dwriter_list_start:                                     \
			CS = dwriter_list_in;                                \
			break;                                               \
		case dwriter_dict_val:                                       \
			CS = dwriter_dict_key;                               \
			break;                                               \
		default:                                                     \
			break;                                               \
		}                                                            \
	} while(0)

#define ADD_NEW_LINE                                                         \
	if ((w->pretty) && (CS == dwriter_complete))                         \
		cherokee_buffer_add_str (OUT, "\n")


#define ENSURE_NOT_KEY                                                       \
	if (CS == dwriter_dict_key)                                          \
		return ret_error

#define ENSURE_VALID_STATE                                                   \
	if ((CS == dwriter_error) || (CS == dwriter_error))                  \
		return ret_error

#define INCREMENT_DEPTH                                                      \
	w->depth += 1;                                                       \
	if (w->depth >= DWRITER_STACK_LEN)                                   \
		return ret_error


ret_t
cherokee_dwriter_init (cherokee_dwriter_t *writer,
                       cherokee_buffer_t  *tmp)
{
	writer->buf    = NULL;
	writer->pretty = false;
	writer->depth  = 0;
	writer->lang   = dwriter_json;
	writer->tmp    = tmp;

	writer->state[writer->depth] = dwriter_start;
	return ret_ok;
}

ret_t
cherokee_dwriter_mrproper (cherokee_dwriter_t *writer)
{
	UNUSED (writer);
	return ret_ok;
}

ret_t
cherokee_dwriter_set_buffer (cherokee_dwriter_t *writer,
                             cherokee_buffer_t  *output)
{
	writer->buf = output;
	return ret_ok;
}

static ret_t
escape_string (cherokee_buffer_t      *buffer,
               const char             *s,
               cuint_t                 len,
	       cherokee_dwriter_lang_t lang)
{
	char    c;
	cuint_t i, j;

	cherokee_buffer_ensure_size (buffer, len*2);

	for (i=0,j=0; i<len; i++) {
		c = s[i];
		switch (c) {
		case '\n':
			buffer->buf[j++] = '\\';
			buffer->buf[j++] = 'n';
			break;
		case '\r':
			buffer->buf[j++] = '\\';
			buffer->buf[j++] = 'r';
			break;
		case '\t':
			buffer->buf[j++] = '\\';
			buffer->buf[j++] = 't';
			break;
		case '\b':
			buffer->buf[j++] = '\\';
			buffer->buf[j++] = 'b';
			break;
		case '\f':
			buffer->buf[j++] = '\\';
			buffer->buf[j++] = 'f';
			break;
		case '\\':
			buffer->buf[j++] = '\\';
			buffer->buf[j++] = '\\';
			break;
		case '/':
			if (lang == dwriter_json)
				buffer->buf[j++] = '\\';

			buffer->buf[j++] = '/';
			break;
		case '"':
			buffer->buf[j++] = '\\';
			buffer->buf[j++] = '"';
			break;
		default:
			buffer->buf[j++] = c;
		}
	}

	buffer->buf[j] = '\0';
	buffer->len    = j;
	return ret_ok;
}

ret_t
cherokee_dwriter_integer (cherokee_dwriter_t *w, long int l)
{
	ENSURE_VALID_STATE; ENSURE_NOT_KEY;
	ADD_SEP; ADD_WHITE;

	cherokee_buffer_add_va (OUT, "%ld", l);

	ADD_END; ADD_NEW_LINE;
	return ret_ok;
}


ret_t
cherokee_dwriter_unsigned (cherokee_dwriter_t *w, unsigned long int l)
{
	ENSURE_VALID_STATE; ENSURE_NOT_KEY;
	ADD_SEP; ADD_WHITE;

	cherokee_buffer_add_va (OUT, "%lu", l);

	ADD_END; ADD_NEW_LINE;
	return ret_ok;
}

ret_t
cherokee_dwriter_double (cherokee_dwriter_t *w, double d)
{
	int  re;
	char tmp[32];

	ENSURE_VALID_STATE; ENSURE_NOT_KEY;
	ADD_SEP; ADD_WHITE;

	re = snprintf(tmp, sizeof(tmp), "%g", d);
	if (re <= 0)
		return ret_error;

	cherokee_buffer_add (OUT, tmp, re);

	ADD_END; ADD_NEW_LINE;
	return ret_ok;
}

ret_t
cherokee_dwriter_number (cherokee_dwriter_t *w, const char *s, int len)
{
	ENSURE_VALID_STATE; ENSURE_NOT_KEY;
	ADD_SEP; ADD_WHITE;

	cherokee_buffer_add (OUT, s, len);

	ADD_END; ADD_NEW_LINE;
	return ret_ok;
}

static void
add_null (cherokee_dwriter_t *w)
{
	switch (w->lang) {
	case dwriter_json:
		cherokee_buffer_add_str (OUT, "null");
		break;
	case dwriter_python:
		cherokee_buffer_add_str (OUT, "None");
		break;
	case dwriter_php:
		cherokee_buffer_add_str (OUT, "NULL");
		break;
	case dwriter_ruby:
		cherokee_buffer_add_str (OUT, "nil");
		break;
	default:
		SHOULDNT_HAPPEN;
	}
}

ret_t
cherokee_dwriter_null (cherokee_dwriter_t *w)
{
	ENSURE_VALID_STATE; ENSURE_NOT_KEY;
	ADD_SEP; ADD_WHITE;

	add_null (w);

	ADD_END; ADD_NEW_LINE;
	return ret_ok;
}

ret_t
cherokee_dwriter_string (cherokee_dwriter_t *w, const char *s, int len)
{
	ENSURE_VALID_STATE;
	ADD_SEP; ADD_WHITE;

	if (unlikely (s == NULL)) {
		add_null (w);
		goto out;
	}

	cherokee_buffer_add_str (OUT, "\"");

	escape_string (w->tmp, s, len, w->lang);
	cherokee_buffer_add (OUT, w->tmp->buf, w->tmp->len);

	cherokee_buffer_add_str (OUT, "\"");

out:
	ADD_END; ADD_NEW_LINE;
	return ret_ok;
}

ret_t
cherokee_dwriter_bool (cherokee_dwriter_t *w, cherokee_boolean_t b)
{
	ENSURE_VALID_STATE; ENSURE_NOT_KEY;
	ADD_SEP; ADD_WHITE;

	switch (w->lang) {
	case dwriter_json:
	case dwriter_ruby:
		if (b)
			cherokee_buffer_add_str (OUT, "true");
		else
			cherokee_buffer_add_str (OUT, "false");
		break;
	case dwriter_python:
		if (b)
			cherokee_buffer_add_str (OUT, "True");
		else
			cherokee_buffer_add_str (OUT, "False");
		break;
	case dwriter_php:
		if (b)
			cherokee_buffer_add_str (OUT, "TRUE");
		else
			cherokee_buffer_add_str (OUT, "FALSE");
		break;
	default:
		SHOULDNT_HAPPEN;
	}

	ADD_END; ADD_NEW_LINE;
	return ret_ok;
}


ret_t
cherokee_dwriter_dict_open (cherokee_dwriter_t *w)
{
	ENSURE_VALID_STATE; ENSURE_NOT_KEY;
	ADD_SEP; ADD_WHITE;

	INCREMENT_DEPTH;
	CS = dwriter_dict_start;

	switch (w->lang) {
	case dwriter_json:
	case dwriter_python:
	case dwriter_ruby:
		cherokee_buffer_add_str (OUT, "{");
		break;
	case dwriter_php:
		cherokee_buffer_add_str (OUT, "array(");
		break;
	default:
		SHOULDNT_HAPPEN;
	}

	if (w->pretty)
		cherokee_buffer_add_str (OUT, "\n");

	ADD_NEW_LINE;
	return ret_ok;
}


ret_t
cherokee_dwriter_dict_close (cherokee_dwriter_t *w)
{
	ENSURE_VALID_STATE;

	w->depth -= 1;
	if (w->pretty)
		cherokee_buffer_add_str (OUT, "\n");

	ADD_END; ADD_WHITE;

	switch (w->lang) {
	case dwriter_json:
	case dwriter_python:
	case dwriter_ruby:
		cherokee_buffer_add_str (OUT, "}");
		break;
	case dwriter_php:
		cherokee_buffer_add_str (OUT, ")");
		break;
	default:
		SHOULDNT_HAPPEN;
	}

	ADD_NEW_LINE;
	return ret_ok;
}


ret_t
cherokee_dwriter_list_open (cherokee_dwriter_t *w)
{
	ENSURE_VALID_STATE; ENSURE_NOT_KEY;
	ADD_SEP; ADD_WHITE;

	INCREMENT_DEPTH;
	CS = dwriter_list_start;

	switch (w->lang) {
	case dwriter_json:
	case dwriter_python:
	case dwriter_ruby:
		cherokee_buffer_add_str (OUT, "[");
		break;
	case dwriter_php:
		cherokee_buffer_add_str (OUT, "array(");
		break;
	default:
		SHOULDNT_HAPPEN;
	}

	if (w->pretty)
		cherokee_buffer_add_str (OUT, "\n");

	ADD_NEW_LINE;
	return ret_ok;
}


ret_t
cherokee_dwriter_list_close (cherokee_dwriter_t *w)
{
	ENSURE_VALID_STATE;

	if (w->pretty)
		cherokee_buffer_add_str (OUT, "\n");
	w->depth -= 1;

	ADD_END; ADD_WHITE;

	switch (w->lang) {
	case dwriter_json:
	case dwriter_python:
	case dwriter_ruby:
		cherokee_buffer_add_str (OUT, "]");
		break;
	case dwriter_php:
		cherokee_buffer_add_str (OUT, ")");
		break;
	default:
		SHOULDNT_HAPPEN;
	}

	ADD_NEW_LINE;
	return ret_ok;
}


ret_t
cherokee_dwriter_lang_to_type (cherokee_buffer_t       *buf,
                               cherokee_dwriter_lang_t *lang)
{
	if (equal_buf_str (buf, "json")) {
		*lang = dwriter_json;
		return ret_ok;
	}

	if (equal_buf_str (buf, "python")) {
		*lang = dwriter_python;
		return ret_ok;
	}

	if (equal_buf_str (buf, "php")) {
		*lang = dwriter_php;
		return ret_ok;
	}

	if (equal_buf_str (buf, "ruby")) {
		*lang = dwriter_ruby;
		return ret_ok;
	}

	return ret_not_found;
}
