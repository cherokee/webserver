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

#include "common-internal.h"
#include "buffer.h"

#include <string.h>
#include <strings.h>
#include <ctype.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include "md5.h"
#include "util.h"
#include "crc32.h"
#include "sha1.h"


#define TO_HEX(c) (c>9? c+'a'-10 : c+'0')

ret_t
cherokee_buffer_new  (cherokee_buffer_t **buf)
{
	CHEROKEE_NEW_STRUCT(n, buffer);

	n->buf  = NULL;
	n->size = 0;
	n->len  = 0;
	   
	*buf = n;
	return ret_ok;
}


ret_t
cherokee_buffer_free (cherokee_buffer_t *buf)
{
	if (buf->buf) {
		free (buf->buf);
		buf->buf = NULL;
	}
	
	free (buf);	
	return ret_ok;
}


ret_t 
cherokee_buffer_init (cherokee_buffer_t *buf)
{
	buf->buf  = NULL;
	buf->len  = 0;
	buf->size = 0;

	return ret_ok;
}

ret_t 
cherokee_buffer_mrproper (cherokee_buffer_t *buf)
{
	if (buf->buf) {
		free (buf->buf);
	}
	
	return cherokee_buffer_init (buf);
}

ret_t
cherokee_buffer_clean (cherokee_buffer_t *buf)
{
	if ((buf->buf != NULL) &&
	    (buf->len > 0))
	{
		buf->buf[0] = '\0';
	}

	buf->len = 0;	
	return ret_ok;
}


void
cherokee_buffer_swap_buffers (cherokee_buffer_t *buf, cherokee_buffer_t *second)
{
	char    *tmp_buf;
	cuint_t  tmp_len;
	cuint_t  tmp_size;

	tmp_buf  = buf->buf;
	tmp_len  = buf->len;
	tmp_size = buf->size;

	buf->buf  = second->buf;
	buf->len  = second->len;
	buf->size = second->size;

	second->buf = tmp_buf;
	second->len = tmp_len;
	second->size = tmp_size;
}


ret_t
cherokee_buffer_add (cherokee_buffer_t *buf, char *txt, size_t size)
{	   
	int free = buf->size - buf->len;

	if (size <= 0)
		return ret_ok;

	/* Get memory
	 */
	if (free < (size+1)) {
		buf->buf = (char *) realloc(buf->buf, buf->size + size - free + 1);
		return_if_fail (buf->buf, ret_nomem);

		buf->size += size - free + 1;
	}
	   
	/* Copy	
	 */
	memcpy (buf->buf + buf->len, txt, size);

	buf->len += size;
	buf->buf[buf->len] = '\0';
	   
	return ret_ok;
}


ret_t 
cherokee_buffer_add_buffer (cherokee_buffer_t *buf, cherokee_buffer_t *buf2)
{
	return cherokee_buffer_add (buf, buf2->buf, buf2->len);
}


ret_t 
cherokee_buffer_add_va_list (cherokee_buffer_t *buf, char *format, va_list args)
{
	cuint_t len;
	cuint_t estimated_length;
	va_list args2;

	va_copy (args2, args);

	estimated_length = cherokee_estimate_va_length (format, args);
	cherokee_buffer_ensure_size (buf, buf->len + estimated_length + 2);

	len = vsnprintf (buf->buf + buf->len, buf->size - buf->len -1, format, args2);
	
#if 1
	if (estimated_length < len)
		PRINT_ERROR ("  -> '%s' -> '%s', esti=%d real=%d\n", 
			     format, buf->buf + buf->len, estimated_length, len);
#endif

	if (len < 0) return ret_error;

	buf->len += len;
	return ret_ok;
}


ret_t 
cherokee_buffer_add_va (cherokee_buffer_t *buf, char *format, ...)
{
	ret_t   ret;
	va_list ap;

	va_start (ap, format);
	ret = cherokee_buffer_add_va_list (buf, format, ap);
	va_end (ap);

	return ret;
}


ret_t 
cherokee_buffer_add_char_n (cherokee_buffer_t *buf, char c, int num)
{
	int free = buf->size - buf->len;

	if (num <= 0)
		return ret_ok;

	/* Get memory
	 */
	if (free < (num+1)) {
		buf->buf = (char *) realloc(buf->buf, buf->size + num - free + 1);
		return_if_fail (buf->buf, ret_nomem);

		buf->size += num - free + 1;
	}

	memset (buf->buf+buf->len, c, num);
	buf->len += num;
	buf->buf[buf->len] = '\0';

	return ret_ok;
}



ret_t
cherokee_buffer_prepend (cherokee_buffer_t  *buf, char *txt, size_t size)
{
	int free = buf->size - buf->len;

	/* Get memory
	 */
	if (free <= size) {
		buf->buf = (char *) realloc(buf->buf, buf->size + size - free + 1);
		return_if_fail (buf->buf, ret_nomem);
		   
		buf->size += size - free + 1;
	}

	memmove (buf->buf+size, buf->buf, buf->len);

	memcpy (buf->buf, txt, size);
	buf->len += size;
	buf->buf[buf->len] = '\0';
	   
	return ret_ok;
}


int   
cherokee_buffer_is_endding (cherokee_buffer_t *buf, char c)
{
	if (cherokee_buffer_is_empty(buf)) {
		return 0;
	}

	return (buf->buf[buf->len -1] == c);
}


ret_t
cherokee_buffer_move_to_begin (cherokee_buffer_t *buf, int pos)
{
	int offset;

	if (pos <= 0) 
		return ret_ok;

	if (pos >= buf->len) 
		return cherokee_buffer_clean(buf);

	offset = MIN(pos, buf->len);
	memmove (buf->buf, buf->buf+offset, (buf->len - offset) +1);
	buf->len -= offset;

#if 0
	if (strlen(buf->buf) != buf->len) {
		PRINT_ERROR ("ERROR: cherokee_buffer_move_to_begin(): strlen=%d buf->len=%d\n", 
			     strlen(buf->buf), buf->len);
	}
#endif

	return ret_ok;
}


ret_t
cherokee_buffer_ensure_size (cherokee_buffer_t *buf, size_t size)
{
	/* Maybe it doesn't need it
	 */
	if (size < buf->len) 
		return ret_ok;

	/* If it is a new buffer, take memory and return
	 */
	if (buf->buf == NULL) {
		buf->buf = (char *) malloc (size);
		if (unlikely (buf->buf == NULL)) return ret_nomem;
		buf->size = size;
		return ret_ok;
	}

	/* It already has memory, but it needs more..
	 */
	buf->buf = (char *) realloc(buf->buf, size);
	if (unlikely (buf->buf == NULL)) return ret_nomem;
	buf->size = size;
	   
	return ret_ok;
}


/* This function is based on code from thttpd
 * Copyright: Jef Poskanzer <jef@acme.com>
 *
 * Copies and decodes a string.  It's ok for from and to to be the
 * same string.
 */

ret_t
cherokee_buffer_decode (cherokee_buffer_t *buffer)
{
	char *from;
	char *to;

	if (buffer->buf == NULL) {
		return ret_error;
	}

	from = to = buffer->buf;

	for (; *from != '\0'; ++to, ++from) {
		if (from[0] == '%' && isxdigit(from[1]) && isxdigit(from[2])) {
			if ((from[1] == '0') && (from[2] == '0')) {
				/* Replace null bytes (%00) with spaces,
				 * to prevent attacks 	
				 */
				*to = ' ';
			} else {
				*to = cherokee_hexit(from[1]) * 16 + cherokee_hexit(from[2]);
			}

			from += 2;
			buffer->len -= 2;
		} else {
			*to = *from;
		}
	}
	*to = '\0';

	return ret_ok;
}

ret_t 
cherokee_buffer_drop_endding (cherokee_buffer_t *buffer, int num_chars)
{
	int num;

	if ((buffer->buf == NULL) || (num_chars <= 0)) {
		return ret_ok;
	}

	num = MIN (num_chars, buffer->len);

	buffer->buf[buffer->len - num] = '\0';
	buffer->len -= num;

	return ret_ok;
}


ret_t
cherokee_buffer_swap_chars (cherokee_buffer_t *buffer, char a, char b)
{
	int i = 0;

	if (buffer->buf == NULL) {
		return ret_ok;
	}

	for (i=0; i<buffer->len; i++) {
		if (buffer->buf[i] == a) {
			buffer->buf[i] = b;
		}
	}

	return ret_ok;
}


ret_t 
cherokee_buffer_remove_dups (cherokee_buffer_t *buffer, char c)
{
	char *a      = buffer->buf;
	int   offset = 0;
	
	if (buffer->len < 2) {
		return ret_ok;
	}

	do {
		if ((a[0] == c) && (a[offset+1] == c)) {
			offset++;
			continue;
		}
		
		a++;
		*a = a[offset];

	} while ((a && *a != '\0') && (a < buffer->buf + buffer->len) && (offset+1 < buffer->len));

	buffer->len -= offset;
	buffer->buf[buffer->len] = '\0';

	return ret_ok;
}


ret_t 
cherokee_buffer_remove_string (cherokee_buffer_t *buf, char *string, int string_len)
{
	char *tmp;
	int   offset;

	if (buf->len <= 0) {
		return ret_ok;
	}

	while ((tmp = strstr (buf->buf, string)) != NULL) {
		offset = tmp - buf->buf;
		memmove (tmp, tmp+string_len, buf->len - (offset+string_len) +1);
		buf->len -= string_len;
	}

	return ret_ok;
}


ret_t 
cherokee_buffer_remove_chunk (cherokee_buffer_t *buf, int from, int len)
{
	char *end;
	char *begin;

	begin = buf->buf + from;
	end   = begin + len;

	memmove (begin, end, ((buf->buf + buf->len) - end) + 1);
	buf->len -= len;

	return ret_ok;
}


crc_t 
cherokee_buffer_crc32 (cherokee_buffer_t  *buf)
{
	return crc32_sz (buf->buf, buf->len);
}


ret_t 
cherokee_buffer_read_file (cherokee_buffer_t *buf, char *filename)
{
	int r, f;
	ret_t ret;
	struct stat info;

	/* Stat() the file
	 */
	r = stat (filename, &info);
	if (r != 0) return ret_error;

	/* Is a regular file?
	 */
	if (S_ISREG(info.st_mode) == 0) {
		return ret_error;
	}

	/* Maybe get memory
	 */
	ret = cherokee_buffer_ensure_size (buf, buf->len + info.st_size + 1);
	if (unlikely(ret != ret_ok)) return ret;
	
	/* Open the file
	 */
	f = open (filename, CHE_O_READ);
	if (f < 0) return ret_error;

	/* Read the content
	 */
	r = read (f, buf->buf + buf->len, info.st_size);
	if (r < 0) {
		buf->buf[buf->len] = '\0';

		close(f);
		return ret_error;
	}
	
	/* Close it and exit
	 */
	close(f);

	buf->len += r;
	buf->buf[buf->len] = '\0';

	return ret_ok;
}


ret_t 
cherokee_buffer_read_from_fd (cherokee_buffer_t *buf, int fd, size_t size, size_t *ret_size)
{
	int  len;
	char tmp[size+1];

	len = read (fd, tmp, size);
	if (len < 0) {
		/* On error
		 */
		switch (errno) {
		case EINTR:      return ret_eagain;
		case EAGAIN:     return ret_eagain;
		case EPIPE:      return ret_eof;
		case ECONNRESET: return ret_eof;
		case EIO:        return ret_error;
		}

		PRINT_ERROR ("ERROR: read(%d, %u,..) -> errno=%d '%s'\n", fd, size, errno, strerror(errno));
		return ret_error;
	}
	else if (len == 0) {
		/* On EOF
		 */
		return ret_eof;
	}

	/* Add readed information
	 */
	cherokee_buffer_add (buf, tmp, len);
	*ret_size = len;
	
	return ret_ok;
}


ret_t 
cherokee_buffer_multiply (cherokee_buffer_t *buf, int num)
{
	int i, initial_size;

	initial_size = buf->len;
	cherokee_buffer_ensure_size (buf, buf->len * num);

	for (i=0; i<num; i++) {
		cherokee_buffer_add (buf, buf->buf, initial_size);
	}

	return ret_ok;
}


ret_t
cherokee_buffer_print_debug (cherokee_buffer_t *buf, int len)
{
	int           i, length;
	char          text[67];
	char*         hex_text;
	char*         ascii_text;
	unsigned char tmp;
	 
	if ((len == -1) || (buf->len <= len)) {
		length = buf->len;
	} else {
		length = len;
	}
	 
	if (length <= 0)
		return ret_ok;
	
	memset(text, 0, 67);
	for (i=0; i < length; i++) {
		if (i%16 == 0) {
			if (text[0] != 0){
				printf ("%s%s", text, CRLF);
			}
			sprintf (text, "%08x%57c", i, ' ');
			hex_text = text + 9;
			ascii_text = text + 49;
		}
		 
		tmp = buf->buf[i];
		sprintf (hex_text, "%02x",  tmp & 0xFF);
		hex_text += 2;
		*hex_text = ' ';
		if ((i+1)%2 == 0) {
			hex_text++;
		}
		 
		if ((tmp > ' ') &&  (tmp < 128))
			*ascii_text = tmp;
		else
			*ascii_text = '.';
		ascii_text += 1;
	}
	printf ("%s%s", text, CRLF);
	fflush(stdout);

	return ret_ok;
}


ret_t 
cherokee_buffer_add_version (cherokee_buffer_t *buf, int port, cherokee_version_t ver)
{
	ret_t       ret;
	static char port_str[6];
	static int  port_len = 0;

	if (port_len == 0) {
		port_len = snprintf (port_str, 6, "%d", port);
	}

	switch (ver) {
	case ver_full_html:
		cherokee_buffer_ensure_size (buf, buf->len + 29 + sizeof(PACKAGE_VERSION) + 6 + port_len + 10);

		cherokee_buffer_add (buf, "<address>Cherokee web server ", 29);
		cherokee_buffer_add_str (buf, PACKAGE_VERSION);
		cherokee_buffer_add (buf, " Port ", 6);
		cherokee_buffer_add (buf, port_str, port_len);
		cherokee_buffer_add (buf, "</address>", 10);
		break;

	case ver_port_html:
		cherokee_buffer_ensure_size (buf, buf->len + 34 + port_len + 10);

		cherokee_buffer_add (buf, "<address>Cherokee web server Port ", 34);
		cherokee_buffer_add (buf, port_str, port_len);
		cherokee_buffer_add (buf, "</address>", 10);
		break;

	default:	
		SHOULDNT_HAPPEN;
	}

	return ret;
}


/* b64_decode_table has been copy&pasted from the code of thttpd:  
 * Copyright <A9> 1995,1998,1999,2000,2001 by Jef Poskanzer <jef@acme.com>
 */

/* Base-64 decoding.  This represents binary data as printable ASCII
 * characters.  Three 8-bit binary bytes are turned into four 6-bit
 * values, like so:
 *
 *   [11111111]  [22222222]  [33333333]
 *   [111111] [112222] [222233] [333333]
 *
 * Then the 6-bit values are represented using the characters "A-Za-z0-9+/".
 */

ret_t 
cherokee_buffer_decode_base64 (cherokee_buffer_t *buf)
{
	char space[128];
	int  space_idx = 0;
	int  i, phase  = 0;
	int  d, prev_d = 0;
	int  buf_pos   = 0;

	static const char
		b64_decode_table[256] = {
			-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,  /* 00-0F */
			-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,  /* 10-1F */
			-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,62,-1,-1,-1,63,  /* 20-2F */
			52,53,54,55,56,57,58,59,60,61,-1,-1,-1,-1,-1,-1,  /* 30-3F */
			-1, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,  /* 40-4F */
			15,16,17,18,19,20,21,22,23,24,25,-1,-1,-1,-1,-1,  /* 50-5F */
			-1,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,  /* 60-6F */
			41,42,43,44,45,46,47,48,49,50,51,-1,-1,-1,-1,-1,  /* 70-7F */
			-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,  /* 80-8F */
			-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,  /* 90-9F */
			-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,  /* A0-AF */
			-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,  /* B0-BF */
			-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,  /* C0-CF */
			-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,  /* D0-DF */
			-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,  /* E0-EF */
			-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1   /* F0-FF */
		};


	space_idx = 0;
	phase = 0;
	for (i=0; i < buf->len; i++) {
		d = b64_decode_table[(int) buf->buf[i]];
		if (d != -1) {
			switch (phase) {
			case 0:
				++phase;
				break;
			case 1:
				space[space_idx++] = (( prev_d << 2 ) | ( ( d & 0x30 ) >> 4 ));
				++phase;
				break;
			case 2:
				space[space_idx++] = (( ( prev_d & 0xf ) << 4 ) | ( ( d & 0x3c ) >> 2 ));
				++phase;
				break;
			case 3:
				space[space_idx++] = (( ( prev_d & 0x03 ) << 6 ) | d );
				phase = 0;
				break;
			}
			prev_d = d;
		} 

		if (space_idx == 127) {
			memcpy (buf->buf + buf_pos, space, 127);
			buf_pos += 127;
			space_idx = 0;
		}
        }

	space[space_idx]='\0';

	memcpy (buf->buf + buf_pos, space, space_idx+1);
	buf->len = buf_pos + space_idx;
	
	return ret_ok;
}


ret_t 
cherokee_buffer_encode_base64 (cherokee_buffer_t *buf)
{
	cuchar_t         *in;
	cuchar_t         *out;
	ret_t             ret;
	int               i, j;
	cuint_t           inlen   = buf->len;
	cherokee_buffer_t new_buf = CHEROKEE_BUF_INIT;
		
	static const char base64tab[]=
		"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

	/* Get memory
	 */
	ret = cherokee_buffer_ensure_size (&new_buf, (buf->len+4)*4/3);
	if (unlikely (ret != ret_ok)) return ret;

	/* Encode
	 */
	in  = (cuchar_t *) buf->buf;
	out = (cuchar_t *) new_buf.buf;

        for (i=0, j=0; i < inlen; i += 3) {
		int     a=0,b=0,c=0;
		int     d, e, f, g;

                a=in[i];
                b= i+1 < inlen ? in[i+1]:0;
                c= i+2 < inlen ? in[i+2]:0;

                d = base64tab [a >> 2 ];
                e = base64tab [((a & 3 ) << 4) | (b >> 4)];
                f = base64tab [((b & 15) << 2) | (c >> 6)];
                g = base64tab [c & 63 ];

                if (i + 1 >= inlen) f='=';
                if (i + 2 >= inlen) g='=';

                out[j++] = d;
                out[j++] = e;
                out[j++] = f;
                out[j++] = g;
        }
	

	out[j]  = '\0';
	new_buf.len = j;

	/* Set the encoded string
	 */
	free (buf->buf);

	buf->buf  = new_buf.buf;
	buf->len  = new_buf.len;	
	buf->size = new_buf.size;

	return ret_ok;
}


ret_t 
cherokee_buffer_escape_html (cherokee_buffer_t *buf, cherokee_buffer_t **maybe_new)
{
	ret_t   ret;
	cuint_t i;
	cuint_t j;
	cuint_t extra = 0;

	/* Count extra characters
	 */
	for (i=0; i<buf->len; i++) {
		if ((buf->buf[i] == '<') || (buf->buf[i] == '>')) {
			extra += 3;
		} else 	if (buf->buf[i] == '&') {
			extra += 4;
		}
	}

	if (extra == 0) return ret_not_found;

	/* Create a new buffer
	 */
	ret = cherokee_buffer_new (maybe_new);
	if (unlikely(ret != ret_ok)) return ret;

	ret = cherokee_buffer_ensure_size (*maybe_new, buf->len + extra + 1);
	if (unlikely(ret != ret_ok)) return ret;

	ret = cherokee_buffer_add_buffer (*maybe_new, buf);
	if (unlikely(ret != ret_ok)) return ret;

	buf = *maybe_new;

	/* Make the changes
	 */
	for (i=0, j=0; i<buf->len; i++) {
		char c = buf->buf[i+j]; 

		switch (c) {
		case '<':
			memmove (&buf->buf[i+j+4], &buf->buf[i+j+1], buf->len-i);
			memcpy (&buf->buf[i+j], "&lt;", 4);
			j += 3;
			break;

		case '>':
			memmove (&buf->buf[i+j+4], &buf->buf[i+j+1], buf->len-i);
			memcpy (&buf->buf[i+j], "&gt;", 4);
			j += 3;
			break;

		case '&':
			memmove (&buf->buf[i+j+5], &buf->buf[i+j+1], buf->len-i);
			memcpy (&buf->buf[i+j], "&amp;", 5);
			j += 4;
			break;
		}
	}

	/* Set the new length
	 */
	buf->len += extra;

	return ret_ok;
}


/* Documentation: 
 * RFC 1321, `The MD5 Message-Digest Algorithm'
 * http://www.alobbs.com/modules.php?op=modload&name=rfc&file=index&content_file=rfc1321.php
 *
 * The MD5 message-digest algorithm takes as input a message of
 * arbitrary length and produces as output a 128-bit "fingerprint" or
 * "message digest" of the input. 
 */

ret_t 
cherokee_buffer_encode_md5_digest (cherokee_buffer_t *buf)
{
	int i;
	struct MD5Context context;
	unsigned char digest[16];
	
	MD5Init (&context);
	MD5Update (&context, (md5byte *)buf->buf, buf->len);
	MD5Final (digest, &context);

	cherokee_buffer_ensure_size (buf, 34);

	for (i=0; i<16; i++) {
		int tmp;

		tmp = (digest[i] >> 4) & 0xf;
		buf->buf[i*2] = TO_HEX(tmp);

		tmp = digest[i] & 0xf;
		buf->buf[(i*2)+1] = TO_HEX(tmp);
	}
	buf->buf[32] = '\0';
	buf->len = 32;

	return ret_ok;
}


ret_t 
cherokee_buffer_encode_md5 (cherokee_buffer_t *buf, cherokee_buffer_t *encoded)
{
	struct MD5Context context;
	
	cherokee_buffer_ensure_size (encoded, 17);

	MD5Init (&context);
	MD5Update (&context, (md5byte *)buf->buf, buf->len);
	MD5Final ((unsigned char *)encoded->buf, &context);

	encoded->buf[16] = '\0';
	encoded->len = 16;

	return ret_ok;
}


#ifndef CHEROKEE_EMBEDDED
ret_t 
cherokee_buffer_encode_sha1 (cherokee_buffer_t *buf, cherokee_buffer_t *encoded)
{
	SHA_INFO sha1;

	sha_init (&sha1);
	sha_update (&sha1, (unsigned char*) buf->buf, buf->len);

	cherokee_buffer_ensure_size (encoded, SHA1_DIGEST_SIZE + 1);
	sha_final (&sha1, (unsigned char *) encoded->buf);

	encoded->len = SHA1_DIGEST_SIZE;
	encoded->buf[encoded->len] = '\0';

	return ret_ok;
}


ret_t 
cherokee_buffer_encode_sha1_base64 (cherokee_buffer_t *buf) 
{
	cuint_t           ntmp;
	char             *ctmp;
	cherokee_buffer_t encoded = CHEROKEE_BUF_INIT;

	cherokee_buffer_ensure_size (&encoded, (SHA1_DIGEST_SIZE *2) + 1);	

	cherokee_buffer_encode_sha1 (buf, &encoded);
	cherokee_buffer_encode_base64 (&encoded);

	/* Swap buffers
	 */
	ctmp = buf->buf;
	buf->buf = encoded.buf;
	encoded.buf = ctmp;

	ntmp = buf->len;
	buf->len = encoded.len;
	encoded.len = ntmp;

	ntmp = buf->size;
	buf->size = encoded.size;
	encoded.size = ntmp;

	/* Clean and return
	 */
	cherokee_buffer_mrproper (&encoded);

	return ret_ok;
}
#endif


ret_t 
cherokee_buffer_encode_hex (cherokee_buffer_t *buf)
{
	unsigned int  i;
	unsigned char j;
	char *new_buf;

	new_buf = (char *) malloc((buf->len * 2)+1);
	if (new_buf == NULL) {
		return ret_error;
	}

	for (i = 0; i < buf->len; i++) {
		j = (buf->buf[i] >> 4) & 0xf;
		if (j <= 9)
			new_buf[i*2] = (j + '0');
		else
			new_buf[i*2] = (j + 'a' - 10);

		j = buf->buf[i] & 0xf;
		if (j <= 9)
			new_buf[i*2+1] = (j + '0');
		else
			new_buf[i*2+1] = (j + 'a' - 10);
	}

	new_buf[buf->len*2] = '\0';

	free (buf->buf);

	buf->len *= 2;
	buf->size = buf->len + 1;
	buf->buf  = new_buf;

	return ret_ok;
}


ret_t 
cherokee_buffer_decode_hex (cherokee_buffer_t *buf)
{
	int i;

	static char hex_to_bin [128] = {
		-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,    /*            */
		-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,    /*            */
		-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,    /*            */
		 0, 1, 2, 3, 4, 5, 6, 7, 8, 9,-1,-1,-1,-1,-1,-1,    /*   0..9     */
		-1,10,11,12,13,14,15,-1,-1,-1,-1,-1,-1,-1,-1,-1,    /*   A..F     */
		-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,    /*            */
		-1,10,11,12,13,14,15,-1,-1,-1,-1,-1,-1,-1,-1,-1,    /*   a..f     */
		-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1 };  /*            */


	for (i=0; i<buf->len/2; i++) {
		/* It uses << 1 rather than * 2
		 */
		cuint_t  b1 = buf->buf[(i << 1)] & 127;
		cuint_t  b2 = buf->buf[(i << 1) + 1] & 127;

 		b1 = hex_to_bin[b1];
 		b2 = hex_to_bin[b2];

		if ((b1 == -1) || (b2 == -1))
			break;

		buf->buf[i] = (((b1 << 4) & 0xF0) | (b2 & 0x0F));
	}

	buf->len /= 2;
	buf->buf[buf->len] = '\0';

	return ret_ok;
}


ret_t 
cherokee_buffer_replace (cherokee_buffer_t *buf, char *string1, int len1, char *string2, int len2)
{
	char *p = buf->buf;

	do {
		p = strstr(p, string1);
		if (p != NULL) {
			if (len1 < len2) {
				
			} else {

			}
		}
	} while (p);

	return ret_ok;
}


ret_t 
cherokee_buffer_add_chunked (cherokee_buffer_t *buf, char *txt, size_t size)
{
	ret_t ret;
	
	ret = cherokee_buffer_add_va (buf, "0x%x"CRLF, size);
	if (unlikely(ret < ret_ok)) return ret_ok;

	return cherokee_buffer_add (buf, txt, size);
}


ret_t 
cherokee_buffer_add_buffer_chunked (cherokee_buffer_t *buf, cherokee_buffer_t *buf2)
{
	return cherokee_buffer_add_chunked (buf, buf2->buf, buf2->len);
}

char  
cherokee_buffer_end_char (cherokee_buffer_t *buf)
{
	if ((buf->buf == NULL) || (buf->len <= 0)) {
		return '\0';
	}

	return buf->buf[buf->len-1];
}


ret_t 
cherokee_buffer_replace_string (cherokee_buffer_t *buf, 
				char *substring,   int substring_length, 
				char *replacement, int replacement_length)
{
        int         remaining_length;
	int         result_length;
        char       *result;
	char       *result_position;
        const char *p;
	const char *substring_position;

	/* Calculate the new size
	 */
	result_length = buf->len;
        for (p = buf->buf; ; p = substring_position + substring_length) {
                substring_position = strstr (p, substring);
                if (substring_position == NULL) {
                        break;
                }

                result_length += (replacement_length - substring_length);
        } 

	/* Take the new memory chunk
	 */
	result = (char *) malloc (result_length + 1);
	if (result == NULL) return ret_nomem;
	
	/* Build the new string
	 */
	result_position = result;

        for (p = buf->buf; ; p = substring_position + substring_length) {
                substring_position = strstr (p, substring);
                if (substring_position == NULL) {
                        remaining_length = strlen (p);
                        memcpy (result_position, p, remaining_length);
                        result_position += remaining_length;
                        break;
                }
                memcpy (result_position, p, substring_position - p);
                result_position += (substring_position - p);

                memcpy (result_position, replacement, replacement_length);
                result_position += replacement_length;
        }	
	*result_position = '\0';

	/* Change the internal buffer content
	 */
	free (buf->buf);

	buf->buf  = result;
	buf->len  = result_length;
	buf->size = result_length + 1;

	return ret_ok;	
}


ret_t 
cherokee_buffer_add_comma_marks (cherokee_buffer_t  *buf)
{
	cuint_t  off, num, i;
	char    *p;

	if ((buf->buf == NULL) || (buf->len <= 3))
		return ret_ok;

	num = buf->len / 3;
	off = buf->len % 3;

	cherokee_buffer_ensure_size (buf, buf->len + num + 2);

	if (off == 0) {
		p = buf->buf + 3;
		num--;
	} else {
		p = buf->buf + off;
	}
	
	for (i=0; i<num; i++) {
		int len = (buf->buf + buf->len) - p;
		memmove(p+1, p, len);
		*p = ',';
		p+=4;
		buf->len++;
	}

	buf->buf[buf->len] = '\0';
	return ret_ok;
}


ret_t 
cherokee_buffer_cmp (cherokee_buffer_t *buf, char *txt, cuint_t txt_len)
{
	if (buf->len != txt_len)
		return ret_deny;

	return (strcmp (buf->buf, txt) == 0) ? ret_ok : ret_deny;
}


ret_t
cherokee_buffer_cmp_buf (cherokee_buffer_t *buf, cherokee_buffer_t *buf2)
{
	return cherokee_buffer_cmp (buf, buf2->buf, buf2->len);
}


ret_t 
cherokee_buffer_case_cmp (cherokee_buffer_t *buf, char *txt, cuint_t txt_len)
{
	if (buf->len != txt_len)
		return ret_deny;

	return (strcasecmp (buf->buf, txt) == 0) ? ret_ok : ret_deny;
}


ret_t
cherokee_buffer_case_cmp_buf (cherokee_buffer_t *buf, cherokee_buffer_t *buf2)
{
	return cherokee_buffer_case_cmp (buf, buf2->buf, buf2->len);
}

