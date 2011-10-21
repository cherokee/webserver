/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/* Cherokee
 *
 * Authors:
 *      Alvaro Lopez Ortega <alvaro@alobbs.com>
 *
 * Copyright (C) 2001-2011 Alvaro Lopez Ortega
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

/* TODO:
 * XML-RPC also supports Base64 and a Datetime field in ISO8601. Since this is not yet
 * handled in DBSlayer, I cannot yet test Dbslayer code I'll leave the addiction of
 * DATE_TYTE2S to the next brave young soul. In essence it would be duplicating the
 * string function, and replace 'string' with 'dateTime.iso8601'.
 */

#include "common-internal.h"
#include "dwriter.h"

#define CS  (w->state[w->depth])
#define OUT (w->buf)

#define ADD_SEP									\
        do {									\
            if (w->lang == dwriter_xmlrpc) {					\
                if (CS == dwriter_dict_key ||					\
                    CS == dwriter_dict_start ) {				\
                     if (w->pretty)						\
                        cherokee_buffer_add_str (OUT, "\t");			\
                     cherokee_buffer_add_str (OUT, "<member>");			\
                     if (w->pretty)						\
                        cherokee_buffer_add_str (OUT, "\n\t\t");		\
                     cherokee_buffer_add_str (OUT, "<name>");			\
                } else if (CS == dwriter_list_in ||				\
                           CS == dwriter_dict_val ||				\
                           CS == dwriter_list_start) {				\
                    if (w->pretty)						\
                        cherokee_buffer_add_str (OUT, "\t\t");			\
                     cherokee_buffer_add_str (OUT, "<value>");			\
                }								\
            } else {								\
	            if ((CS == dwriter_dict_key) ||				\
        		    (CS == dwriter_list_in)) {				\
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
                }								\
            }									\
	} while(0);

#define ADD_WHITE								\
	do {									\
		unsigned int i;							\
		if (w->pretty && w->lang != dwriter_csv &&			\
				 w->lang != dwriter_xmlrpc) {			\
			if (CS != dwriter_dict_val) {				\
				for (i=0; i<w->depth; i++)			\
					cherokee_buffer_add_str (OUT, "  ");	\
			}							\
		}								\
	} while(0)

#define ADD_END										\
	do {										\
		if (w->lang == dwriter_xmlrpc) {					\
			switch (CS) {							\
			    case dwriter_dict_start:					\
			    case dwriter_dict_key:					\
				cherokee_buffer_add_str (OUT, "</name>");		\
				break;							\
			    case dwriter_dict_val:					\
				cherokee_buffer_add_str (OUT, "</value>");		\
				if (w->pretty)						\
					cherokee_buffer_add_str (OUT, "\n\t");		\
					cherokee_buffer_add_str (OUT, "</member>");	\
				    break;						\
			    case dwriter_list_in:					\
			    case dwriter_list_start:					\
				cherokee_buffer_add_str (OUT, "</value>");		\
			    default:							\
				break;							\
			}								\
		} else {								\
			switch(CS) {							\
			case dwriter_start:						\
				CS = dwriter_complete;					\
				break;							\
			case dwriter_dict_start:					\
			case dwriter_dict_key:						\
				CS = dwriter_dict_val;					\
				break;							\
			case dwriter_list_start:					\
				CS = dwriter_list_in;					\
				break;							\
			case dwriter_dict_val:						\
				CS = dwriter_dict_key;					\
				break;							\
			default:							\
				break;							\
			}								\
		}									\
	} while(0)

#define ADD_NEW_LINE						\
	if (w->pretty && (w->lang == dwriter_xmlrpc ||		\
	                  CS == dwriter_dict_start  ||		\
			  CS == dwriter_list_start  ||		\
			  CS == dwriter_complete))		\
   		cherokee_buffer_add_str (OUT, "\n");		\

#define ENSURE_NOT_KEY						\
	if (CS == dwriter_dict_key)				\
		return ret_error;

#define ENSURE_VALID_STATE					\
	if ((CS == dwriter_error) || (CS == dwriter_error))	\
		return ret_error;

#define INCREMENT_DEPTH						\
	w->depth += 1;						\
	if (w->depth >= DWRITER_STACK_LEN)			\
		return ret_error;


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
escape_string (cherokee_buffer_t *buffer,
	       const char        *s,
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

	if (w->lang == dwriter_xmlrpc) {
		cherokee_buffer_add_va (OUT, "<int>%lu</int>", l);
	} else {
		cherokee_buffer_add_va (OUT, "%lu", l);
	}

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

	if (w->lang == dwriter_xmlrpc) {
		cherokee_buffer_add_str (OUT, "<double>");
		cherokee_buffer_add (OUT, tmp, re);
		cherokee_buffer_add_str (OUT, "</double>");
	} else {
		cherokee_buffer_add (OUT, tmp, re);
	}

	ADD_END; ADD_NEW_LINE;
	return ret_ok;
}

ret_t
cherokee_dwriter_number (cherokee_dwriter_t *w, const char *s, int len)
{
	ENSURE_VALID_STATE; ENSURE_NOT_KEY;
	ADD_SEP; ADD_WHITE;
    
	if (w->lang == dwriter_xmlrpc) {
		cherokee_buffer_add_str (OUT, "<int>");
		cherokee_buffer_add (OUT, s, len);
		cherokee_buffer_add_str (OUT, "</int>");
	} else {
	    	cherokee_buffer_add (OUT, s, len);
	}

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
	case dwriter_xmlrpc:
		cherokee_buffer_add_str (OUT, "<nil/>");
		break;
	case dwriter_csv:
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

	if (w->lang == dwriter_xmlrpc) {
		cherokee_buffer_t buf = CHEROKEE_BUF_INIT;
		cherokee_buffer_add (&buf, s, len);

		/* XML Encoding, as you see with &, sequence matters! */
		cherokee_buffer_replace_string(&buf, "&", 1, "&amp;", 5);
		cherokee_buffer_replace_string(&buf, "<", 1, "&lt;", 4);
		cherokee_buffer_replace_string(&buf, ">", 1, "&gt;", 4);
		cherokee_buffer_replace_string(&buf, "'", 1, "&apos;", 6);
		cherokee_buffer_replace_string(&buf, "\"", 1,"&quot;", 6);

		if (CS == dwriter_dict_val || CS == dwriter_list_in || CS == dwriter_list_start) {
			cherokee_buffer_add_str (OUT, "<string>");
			cherokee_buffer_add_buffer (OUT, &buf);
			cherokee_buffer_add_str (OUT, "</string>");
		} else {
			cherokee_buffer_add_buffer (OUT, &buf);
		}

		cherokee_buffer_mrproper(&buf);
	} else {
		cherokee_buffer_add_str (OUT, "\"");

		escape_string (w->tmp, s, len);
		cherokee_buffer_add (OUT, w->tmp->buf, w->tmp->len);

		cherokee_buffer_add_str (OUT, "\"");
	}
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
	case dwriter_csv:
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
	case dwriter_xmlrpc:
		cherokee_buffer_add_str (OUT, "<boolean>");
		cherokee_buffer_add_str (OUT, b ? "1" : "0");
		cherokee_buffer_add_str (OUT, "</boolean>");
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
	case dwriter_xmlrpc:
		cherokee_buffer_add_str (OUT, "<struct>");
		break;
	case dwriter_csv:
		break;
	default:
		SHOULDNT_HAPPEN;
	}

	ADD_NEW_LINE;
	return ret_ok;
}


ret_t
cherokee_dwriter_dict_close (cherokee_dwriter_t *w)
{
	ENSURE_VALID_STATE;

	w->depth -= 1;

	ADD_END; ADD_WHITE;

	switch (w->lang) {
	case dwriter_json:
	case dwriter_python:
	case dwriter_ruby:
        if (w->pretty)
            cherokee_buffer_add_str (OUT, "\n");
		cherokee_buffer_add_str (OUT, "}");
		break;
	case dwriter_php:
        if (w->pretty)
            cherokee_buffer_add_str (OUT, "\n");
		cherokee_buffer_add_str (OUT, ")");
		break;
	case dwriter_xmlrpc:
		cherokee_buffer_add_str (OUT, "</struct>");
		break;
	case dwriter_csv:
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
	case dwriter_xmlrpc:
		cherokee_buffer_add_str (OUT, "<array>");
		if (w->pretty)
			cherokee_buffer_add_str (OUT, "\n\t");
		cherokee_buffer_add_str (OUT, "<data>");
		break;
	case dwriter_csv:
		break;
	default:
		SHOULDNT_HAPPEN;
	}

	ADD_NEW_LINE;
	return ret_ok;
}


ret_t
cherokee_dwriter_list_close (cherokee_dwriter_t *w)
{
	ENSURE_VALID_STATE;

	w->depth -= 1;

	ADD_END; ADD_WHITE;

	switch (w->lang) {
	case dwriter_json:
	case dwriter_python:
	case dwriter_ruby:
        	if (w->pretty)
        		cherokee_buffer_add_str (OUT, "\n");
		cherokee_buffer_add_str (OUT, "]");
		break;
	case dwriter_php:
        	if (w->pretty)
			cherokee_buffer_add_str (OUT, "\n");
		cherokee_buffer_add_str (OUT, ")");
		break;
	case dwriter_xmlrpc:
		if (w->pretty)
			cherokee_buffer_add_str (OUT, "\t");
		cherokee_buffer_add_str (OUT, "</data>");
		if (w->pretty)
			cherokee_buffer_add_str (OUT, "\n");
		cherokee_buffer_add_str (OUT, "</array>");
		break;
	case dwriter_csv:
		cherokee_buffer_add_str (OUT, "\n");
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

	if (equal_buf_str (buf, "xmlrpc")) {
		*lang = dwriter_xmlrpc;
		return ret_ok;
	}

	if (equal_buf_str (buf, "csv")) {
		*lang = dwriter_csv;
		return ret_ok;
	}

	return ret_not_found;
}
