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
#include "dwriter.h"

#define CS  (w->state[w->depth])
#define OUT (w->buf)

#define ADD_SEP                                                         \
        do {							      	\
	        if ((CS == dwriter_dict_key) ||				\
		    (CS == dwriter_list_in))				\
		{							\
			cherokee_buffer_add_str (OUT, ",");		\
			if (w->pretty)					\
				cherokee_buffer_add_str(OUT, "\n");	\
		} else if (CS == dwriter_dict_val) {			\
			if ((w->lang == dwriter_php) ||			\
			    (w->lang == dwriter_ruby))			\
				cherokee_buffer_add_str (OUT, "=>");	\
			else						\
				cherokee_buffer_add_str (OUT, ":");	\
			if (w->pretty)					\
				cherokee_buffer_add_str (OUT, " ");	\
		}							\
	} while(0)

#define ADD_WHITE							\
	do {								\
		unsigned int i;						\
		if (w->pretty) {					\
			if (CS != dwriter_dict_val) {			\
				for (i=0; i<w->depth; i++)		\
					cherokee_buffer_add_str (OUT, "  "); \
			}						\
		}							\
	} while(0)

#define ADD_END							\
	do {							\
		switch(CS) {					\
		case dwriter_start:				\
			CS = dwriter_complete;			\
			break;					\
		case dwriter_dict_start:			\
		case dwriter_dict_key:				\
			CS = dwriter_dict_val;			\
			break;					\
		case dwriter_list_start:			\
			CS = dwriter_list_in;			\
			break;					\
		case dwriter_dict_val:				\
			CS = dwriter_dict_key;			\
			break;					\
		default:					\
			break;					\
		}						\
	} while(0)

#define ADD_NEW_LINE					\
	if ((w->pretty) && (CS == dwriter_complete))	\
		cherokee_buffer_add_str (OUT, "\n")


#define ENSURE_NOT_KEY				\
	if (CS == dwriter_dict_key)		\
		return ret_error

#define ENSURE_VALID_STATE					\
	if ((CS == dwriter_error) || (CS == dwriter_error))	\
		return ret_error

#define INCREMENT_DEPTH				\
	w->depth += 1;				\
	if (w->depth >= DWRITER_STACK_LEN)	\
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
escape_string (cherokee_buffer_t *buffer, 
	       char              *s,
	       cuint_t            len)
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
cherokee_dwriter_integer (cherokee_dwriter_t *w, unsigned long l)
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
cherokee_dwriter_number (cherokee_dwriter_t *w, char *s, int len)
{
	ENSURE_VALID_STATE; ENSURE_NOT_KEY;
	ADD_SEP; ADD_WHITE;

	cherokee_buffer_add (OUT, s, len);

	ADD_END; ADD_NEW_LINE;
	return ret_ok;
}

ret_t
cherokee_dwriter_string (cherokee_dwriter_t *w, char *s, int len)
{
	ENSURE_VALID_STATE;
	ADD_SEP; ADD_WHITE;

	cherokee_buffer_add_str (OUT, "\"");

	escape_string (w->tmp, s, len);
	cherokee_buffer_add (OUT, w->tmp->buf, w->tmp->len);

	cherokee_buffer_add_str (OUT, "\"");

	ADD_END; ADD_NEW_LINE;
	return ret_ok;
}


ret_t
cherokee_dwriter_null (cherokee_dwriter_t *w)
{
	ENSURE_VALID_STATE; ENSURE_NOT_KEY;
	ADD_SEP; ADD_WHITE;

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
		cherokee_buffer_add_str (OUT, b ? "true" : "false");
		break;
	case dwriter_python:
		cherokee_buffer_add_str (OUT, b ? "True" : "False");
		break;
	case dwriter_php:
		cherokee_buffer_add_str (OUT, b ? "TRUE" : "FALSE");
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
