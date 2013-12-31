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

#ifndef CHEROKEE_REGEX_TABLE_H
#define CHEROKEE_REGEX_TABLE_H

#include <cherokee/common.h>
#include <cherokee/list.h>
#include <cherokee/buffer.h>
#include <cherokee/config_node.h>
#include <pcre/pcre.h>

CHEROKEE_BEGIN_DECLS

/* Field num x 3: man pcre_exec */
#define OVECTOR_LEN (20*3)

typedef struct cherokee_regex_table cherokee_regex_table_t;
#define REGEX(x) ((cherokee_regex_table_t *)(x))

/* RegEx table
 */
ret_t cherokee_regex_table_new   (cherokee_regex_table_t **table);
ret_t cherokee_regex_table_free  (cherokee_regex_table_t  *table);
ret_t cherokee_regex_table_clean (cherokee_regex_table_t  *table);

ret_t cherokee_regex_table_get  (cherokee_regex_table_t *table, char *pattern, void **pcre);
ret_t cherokee_regex_table_add  (cherokee_regex_table_t *table, char *pattern);

/* RegEx lists
 */
typedef struct {
	cherokee_list_t    listed;
	pcre              *re;
	char               hidden;
	cherokee_buffer_t  subs;
} cherokee_regex_entry_t;

#define REGEX_ENTRY(r) ((cherokee_regex_entry_t *)(r))

ret_t cherokee_regex_list_configure (cherokee_list_t        *list,
                                     cherokee_config_node_t *conf,
                                     cherokee_regex_table_t *regexs);

ret_t cherokee_regex_list_mrproper  (cherokee_list_t        *list);

/* Utilities
 */
ret_t cherokee_regex_substitute (cherokee_buffer_t *regex_str,
                                 cherokee_buffer_t *source,
                                 cherokee_buffer_t *target,
                                 cint_t             ovector[],
                                 cint_t             stringcount,
                                 char               dollar_char);

CHEROKEE_END_DECLS

#endif /* CHEROKEE_REGEX_TABLE_H */
