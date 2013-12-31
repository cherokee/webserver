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

#ifndef CHEROKEE_HEADER_OP_H
#define CHEROKEE_HEADER_OP_H

#include "common.h"
#include "list.h"
#include "buffer.h"

typedef enum {
	cherokee_header_op_add,
	cherokee_header_op_del
} cherokee_header_op_type_t;

typedef struct {
	cherokee_list_t           entry;
	cherokee_header_op_type_t op;
	cherokee_buffer_t         header;
	cherokee_buffer_t         value;
} cherokee_header_op_t;

#define HEADER_OP(x) ((cherokee_header_op_t *)(x))

ret_t cherokee_header_op_new       (cherokee_header_op_t **op);
ret_t cherokee_header_op_free      (cherokee_header_op_t  *op);

ret_t cherokee_header_op_configure (cherokee_header_op_t   *op,
                                    cherokee_config_node_t *config);

ret_t cherokee_header_op_render    (cherokee_list_t        *ops_list,
                                    cherokee_buffer_t      *header);

#endif /* CHEROKEE_HEADER_OP_H */
