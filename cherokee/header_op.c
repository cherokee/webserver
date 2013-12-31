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
#include "header_op.h"
#include "util.h"

ret_t
cherokee_header_op_new (cherokee_header_op_t **op)
{
	CHEROKEE_NEW_STRUCT (n, header_op);

	INIT_LIST_HEAD (&n->entry);
	cherokee_buffer_init (&n->header);
	cherokee_buffer_init (&n->value);

	*op = n;
	return ret_ok;
}


ret_t
cherokee_header_op_free (cherokee_header_op_t *op)
{
	if (unlikely (op == NULL)) {
		return ret_ok;
	}

	cherokee_buffer_mrproper (&op->header);
	cherokee_buffer_mrproper (&op->value);

	free (op);
	return ret_ok;
}


ret_t
cherokee_header_op_configure (cherokee_header_op_t   *op,
                              cherokee_config_node_t *conf)
{
	ret_t              ret;
	cherokee_buffer_t *tmp;

	/* Operation type
	 */
	ret = cherokee_config_node_read (conf, "type", &tmp);
	if (ret != ret_ok) {
		free (op);
		return ret_error;
	}

	if (cherokee_buffer_case_cmp_str (tmp, "add") == 0) {
		op->op = cherokee_header_op_add;
	} else if (cherokee_buffer_case_cmp_str (tmp, "del") == 0) {
		op->op = cherokee_header_op_del;
	} else {
		free (op);
		return ret_error;
	}

	/* Details
	 */
	ret = cherokee_config_node_copy (conf, "header", &op->header);
	if (ret != ret_ok) {
		free (op);
		return ret_error;
	}

	if (op->op == cherokee_header_op_add) {
		ret = cherokee_config_node_copy (conf, "value", &op->value);
		if (ret != ret_ok) {
			cherokee_buffer_mrproper (&op->header);
			free (op);
			return ret_error;
		}
	}

	return ret_ok;
}


static ret_t
remove_header (cherokee_buffer_t *buffer,
               cherokee_buffer_t *header)
{
	char *p, *s;

	p = strncasestrn (buffer->buf, buffer->len,
	                  header->buf, header->len);
	if (p == NULL)
		return ret_not_found;

	if (p[header->len] != ':')
		return ret_not_found;

	s = strchr (p, '\r');
	if (s == NULL)
		return ret_eof;

	cherokee_buffer_remove_chunk (buffer, p - buffer->buf, s-p +2);
	return ret_ok;
}


ret_t
cherokee_header_op_render (cherokee_list_t   *ops_list,
                           cherokee_buffer_t *buffer)
{
	cherokee_list_t      *i;
	cherokee_header_op_t *op;

	list_for_each (i, ops_list){
		op = HEADER_OP(i);
		if (op->op == cherokee_header_op_add) {
			/* Check whether there is a previous
			 * header. If so, get rid of it.
			 */
			remove_header (buffer, &op->header);

			/* Add the new entry
			 */
			cherokee_buffer_add_buffer (buffer, &op->header);
			cherokee_buffer_add_str    (buffer, ": ");
			cherokee_buffer_add_buffer (buffer, &op->value);
			cherokee_buffer_add_str    (buffer, CRLF);

		} else if (op->op == cherokee_header_op_del) {
			remove_header (buffer, &op->header);
		}
	}

	return ret_ok;
}
