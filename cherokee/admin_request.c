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
#include "admin_request.h"
#include "list_ext.h"
#include "pickle.h"


struct cherokee_admin_request {
	cherokee_buffer_t req;
	int               num;
	struct list_head  list;
};


ret_t cherokee_admin_request_new  (cherokee_admin_request_t **req)
{
	CHEROKEE_NEW_STRUCT (n, admin_request);

	INIT_LIST_HEAD(&n->list);
	cherokee_buffer_init (&n->req);
	n->num = 0;

	*req = n;
	return ret_ok;
}


ret_t
cherokee_admin_request_free (cherokee_admin_request_t *req)
{
	cherokee_list_content_free (&req->list, free);
	cherokee_buffer_mrproper (&req->req);

	free (req);
	return ret_ok;
}


ret_t
cherokee_admin_request_add (cherokee_admin_request_t *req, char *key)
{
	cherokee_list_add_tail_content (&req->list, strdup(key));
	req->num++;
	return ret_ok;
}


ret_t
cherokee_admin_request_serialize (cherokee_admin_request_t *req)
{
	list_t *i;

	/* Add the number of elements
	 */
	cherokee_pickle_add_uint (&req->req, req->num);

	/* Add each string
	 */
	list_for_each (i, &req->list) {
		char *str = (char *)LIST_ITEM_INFO(i);
		cherokee_pickle_add_string (&req->req, str, strlen(str));
	}

	/* Clean the list
	 */
	cherokee_list_content_free (&req->list, free);
	return ret_ok;
}

