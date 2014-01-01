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

#ifndef CHEROKEE_CONFIG_NODE_H
#define CHEROKEE_CONFIG_NODE_H

#include <cherokee/common.h>
#include <cherokee/list.h>
#include <cherokee/buffer.h>

CHEROKEE_BEGIN_DECLS

typedef struct {
	cherokee_list_t    entry;
	cherokee_list_t    child;

	cherokee_buffer_t  key;
	cherokee_buffer_t  val;
} cherokee_config_node_t;

#define CONFIG_NODE(c) ((cherokee_config_node_t *)(c))

#define cherokee_config_node_foreach(i,config) \
	list_for_each (i, &config->child)

typedef ret_t (* cherokee_config_node_while_func_t) (cherokee_config_node_t *, void *);
typedef ret_t (* cherokee_config_node_list_func_t)  (char *, void *);

ret_t cherokee_config_node_new       (cherokee_config_node_t **conf);
ret_t cherokee_config_node_free      (cherokee_config_node_t  *conf);
ret_t cherokee_config_node_init      (cherokee_config_node_t  *conf);
ret_t cherokee_config_node_mrproper  (cherokee_config_node_t  *conf);

ret_t cherokee_config_node_add       (cherokee_config_node_t *conf, const char *key, cherokee_buffer_t *val);
ret_t cherokee_config_node_add_buf   (cherokee_config_node_t *conf, cherokee_buffer_t *key, cherokee_buffer_t *val);

ret_t cherokee_config_node_get       (cherokee_config_node_t *conf, const char *key, cherokee_config_node_t **entry);
ret_t cherokee_config_node_get_buf   (cherokee_config_node_t *conf, cherokee_buffer_t *key, cherokee_config_node_t **entry);

ret_t cherokee_config_node_while     (cherokee_config_node_t *conf, cherokee_config_node_while_func_t func, void *data);

/* Convenience functions: value retrieving
 */
ret_t cherokee_config_node_read       (cherokee_config_node_t *conf, const char *key, cherokee_buffer_t **buf);
ret_t cherokee_config_node_copy       (cherokee_config_node_t *conf, const char *key, cherokee_buffer_t  *buf);

ret_t cherokee_config_node_read_path  (cherokee_config_node_t *conf, const char *key, cherokee_buffer_t **buf);
ret_t cherokee_config_node_read_int   (cherokee_config_node_t *conf, const char *key, int *num);
ret_t cherokee_config_node_read_uint  (cherokee_config_node_t *conf, const char *key, cuint_t *num);
ret_t cherokee_config_node_read_long  (cherokee_config_node_t *conf, const char *key, long *num);
ret_t cherokee_config_node_read_bool  (cherokee_config_node_t *conf, const char *key, cherokee_boolean_t *val);
ret_t cherokee_config_node_read_list  (cherokee_config_node_t *conf, const char *key,
                                       cherokee_config_node_list_func_t func, void *param);

ret_t cherokee_config_node_convert_list (cherokee_config_node_t *conf, const char *key, cherokee_list_t *list);

CHEROKEE_END_DECLS

#endif /* CHEROKEE_CONFIG_NODE_H */
