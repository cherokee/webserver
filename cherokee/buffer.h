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

#ifndef CHEROKEE_BUFFER_H
#define CHEROKEE_BUFFER_H

#include <cherokee/common.h>
#include <stddef.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

CHEROKEE_BEGIN_DECLS


typedef struct {
	char    *buf;        /**< Memory chunk           */
	cuint_t  size;       /**< Total amount of memory */
	cuint_t  len;        /**< Length of the string   */
} cherokee_buffer_t;

#define BUF(x) ((cherokee_buffer_t *)(x))
#define CHEROKEE_BUF_INIT       {NULL, 0, 0}
#define CHEROKEE_BUF_SLIDE_NONE INT_MIN

#define cherokee_buffer_is_empty(b)        (BUF(b)->len == 0)
#define cherokee_buffer_add_str(b,s)       cherokee_buffer_add (b, s, CSZLEN(s))
#define cherokee_buffer_prepend_str(b,s)   cherokee_buffer_prepend (b, s, CSZLEN(s))
#define cherokee_buffer_prepend_buf(b,s)   cherokee_buffer_prepend (b, (s)->buf, (s)->len)
#define cherokee_buffer_cmp_str(b,s)       cherokee_buffer_cmp      (b, (char *)(s), sizeof(s)-1)
#define cherokee_buffer_case_cmp_str(b,s)  cherokee_buffer_case_cmp (b, (char *)(s), sizeof(s)-1)
#define cherokee_buffer_fake_str(b,s)      cherokee_buffer_fake (b, s, sizeof(s)-1)

ret_t cherokee_buffer_new                  (cherokee_buffer_t **buf);
ret_t cherokee_buffer_free                 (cherokee_buffer_t  *buf);
ret_t cherokee_buffer_init                 (cherokee_buffer_t  *buf);
ret_t cherokee_buffer_mrproper             (cherokee_buffer_t  *buf);
void  cherokee_buffer_fake                 (cherokee_buffer_t  *buf, const char *str, cuint_t len);

void  cherokee_buffer_clean                (cherokee_buffer_t  *buf);
ret_t cherokee_buffer_dup                  (cherokee_buffer_t  *buf, cherokee_buffer_t **dup);
void  cherokee_buffer_swap_buffers         (cherokee_buffer_t  *buf, cherokee_buffer_t *second);

ret_t cherokee_buffer_add                  (cherokee_buffer_t  *buf, const char *txt, size_t size);
ret_t cherokee_buffer_add_long10           (cherokee_buffer_t  *buf, clong_t lNum);
ret_t cherokee_buffer_add_llong10          (cherokee_buffer_t  *buf, cllong_t lNum);
ret_t cherokee_buffer_add_ulong10          (cherokee_buffer_t  *buf, culong_t ulNum);
ret_t cherokee_buffer_add_ullong10         (cherokee_buffer_t  *buf, cullong_t ulNum);
ret_t cherokee_buffer_add_ulong16          (cherokee_buffer_t  *buf, culong_t ulNum);
ret_t cherokee_buffer_add_ullong16         (cherokee_buffer_t  *buf, cullong_t ulNum);
ret_t cherokee_buffer_add_va               (cherokee_buffer_t  *buf, const char *format, ...);
ret_t cherokee_buffer_add_va_fixed         (cherokee_buffer_t  *buf, const char *format, ...);
ret_t cherokee_buffer_add_va_list          (cherokee_buffer_t  *buf, const char *format, va_list args);
ret_t cherokee_buffer_add_char             (cherokee_buffer_t  *buf, char c);
ret_t cherokee_buffer_add_char_n           (cherokee_buffer_t  *buf, char c, int n);
ret_t cherokee_buffer_add_buffer           (cherokee_buffer_t  *buf, cherokee_buffer_t *buf2);
ret_t cherokee_buffer_add_buffer_slice     (cherokee_buffer_t  *buf, cherokee_buffer_t *buf2, ssize_t begin, ssize_t end);
ret_t cherokee_buffer_add_fsize            (cherokee_buffer_t  *buf, CST_OFFSET size);
ret_t cherokee_buffer_prepend              (cherokee_buffer_t  *buf, const char *txt, size_t size);

cint_t cherokee_buffer_cmp                 (cherokee_buffer_t  *buf, char *txt, cuint_t txt_len);
cint_t cherokee_buffer_cmp_buf             (cherokee_buffer_t  *buf, cherokee_buffer_t *buf2);
cint_t cherokee_buffer_case_cmp            (cherokee_buffer_t  *buf, char *txt, cuint_t txt_len);
cint_t cherokee_buffer_case_cmp_buf        (cherokee_buffer_t  *buf, cherokee_buffer_t *buf2);

ret_t cherokee_buffer_read_file            (cherokee_buffer_t  *buf, char *filename);
ret_t cherokee_buffer_read_from_fd         (cherokee_buffer_t  *buf, int fd, size_t size, size_t *ret_size);

ret_t cherokee_buffer_move_to_begin        (cherokee_buffer_t  *buf, cuint_t pos);
ret_t cherokee_buffer_drop_ending          (cherokee_buffer_t  *buf, cuint_t num_chars);
ret_t cherokee_buffer_multiply             (cherokee_buffer_t  *buf, int num);
ret_t cherokee_buffer_swap_chars           (cherokee_buffer_t  *buf, char a, char b);
ret_t cherokee_buffer_remove_dups          (cherokee_buffer_t  *buf, char c);
ret_t cherokee_buffer_remove_string        (cherokee_buffer_t  *buf, char *string, int string_len);
ret_t cherokee_buffer_remove_chunk         (cherokee_buffer_t  *buf, cuint_t from, cuint_t len);
ret_t cherokee_buffer_replace_string       (cherokee_buffer_t  *buf, const char *subs, int subs_len, const char *repl, int repl_len);
ret_t cherokee_buffer_substitute_string    (cherokee_buffer_t  *bufsrc, cherokee_buffer_t *bufdst, char *subs, int subs_len, char *repl, int repl_len);
ret_t cherokee_buffer_trim                 (cherokee_buffer_t  *buf);
ret_t cherokee_buffer_insert               (cherokee_buffer_t  *buf, char *txt, size_t txt_len, size_t pos);
ret_t cherokee_buffer_insert_buffer        (cherokee_buffer_t  *buf, cherokee_buffer_t *src, size_t pos);

ret_t cherokee_buffer_get_utf8_len         (cherokee_buffer_t  *buf, cuint_t *len);
ret_t cherokee_buffer_ensure_addlen        (cherokee_buffer_t  *buf, size_t alen);
ret_t cherokee_buffer_ensure_size          (cherokee_buffer_t  *buf, size_t size);

int    cherokee_buffer_is_ending           (cherokee_buffer_t  *buf, char c);
char   cherokee_buffer_end_char            (cherokee_buffer_t  *buf);
size_t cherokee_buffer_cnt_spn             (cherokee_buffer_t  *buf, cuint_t offset, const char *str);
size_t cherokee_buffer_cnt_cspn            (cherokee_buffer_t  *buf, cuint_t offset, const char *str);

crc_t cherokee_buffer_crc32                (cherokee_buffer_t  *buf);
ret_t cherokee_buffer_encode_base64        (cherokee_buffer_t  *buf, cherokee_buffer_t *encoded);
ret_t cherokee_buffer_decode_base64        (cherokee_buffer_t  *buf);
ret_t cherokee_buffer_encode_md5           (cherokee_buffer_t  *buf, cherokee_buffer_t *encoded);
ret_t cherokee_buffer_encode_md5_digest    (cherokee_buffer_t  *buf);
ret_t cherokee_buffer_encode_sha1          (cherokee_buffer_t  *buf, cherokee_buffer_t *encoded);
ret_t cherokee_buffer_encode_sha1_digest   (cherokee_buffer_t  *buf);
ret_t cherokee_buffer_encode_sha1_base64   (cherokee_buffer_t  *buf, cherokee_buffer_t *encoded);
ret_t cherokee_buffer_encode_sha512        (cherokee_buffer_t  *buf, cherokee_buffer_t *encoded);
ret_t cherokee_buffer_encode_sha512_digest (cherokee_buffer_t  *buf);
ret_t cherokee_buffer_encode_sha512_base64 (cherokee_buffer_t  *buf, cherokee_buffer_t *encoded);
ret_t cherokee_buffer_encode_hex           (cherokee_buffer_t  *buf, cherokee_buffer_t *encoded);
ret_t cherokee_buffer_decode_hex           (cherokee_buffer_t  *buf);
ret_t cherokee_buffer_unescape_uri         (cherokee_buffer_t  *buf);
ret_t cherokee_buffer_escape_uri           (cherokee_buffer_t  *buf, cherokee_buffer_t *src);
ret_t cherokee_buffer_escape_uri_delims    (cherokee_buffer_t  *buf, cherokee_buffer_t *src);
ret_t cherokee_buffer_escape_arg           (cherokee_buffer_t  *buf, cherokee_buffer_t *src);
ret_t cherokee_buffer_add_escape_html      (cherokee_buffer_t  *buf, cherokee_buffer_t *src);
ret_t cherokee_buffer_escape_html          (cherokee_buffer_t  *buf, cherokee_buffer_t *src);
ret_t cherokee_buffer_add_comma_marks      (cherokee_buffer_t  *buf);

ret_t cherokee_buffer_to_lowcase           (cherokee_buffer_t  *buf);
ret_t cherokee_buffer_split_lines          (cherokee_buffer_t  *buf, int columns, const char *indent);

ret_t cherokee_buffer_print_debug          (cherokee_buffer_t  *buf, int length);


CHEROKEE_END_DECLS

#endif /* CHEROKEE_BUFFER_H */
