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

#ifndef CHEROKEE_MACHING_LIST_H
#define CHEROKEE_MACHING_LIST_H

#include "common.h"
#include "list.h"
#include "config_node.h"

typedef enum {
	default_allow,
	default_deny,
	deny_allow,
	allow_deny
} cherokee_matching_t;


typedef struct {
	cherokee_list_t     list;
	char               *string;
} cherokee_matching_list_entry_t;


typedef struct {
	cherokee_list_t     list_allow;
	cherokee_list_t     list_deny;
	cherokee_matching_t type;
} cherokee_matching_list_t;

#define MLIST(x)       ((cherokee_matching_list_t *)(x))
#define MLIST_ENTRY(x) ((cherokee_matching_list_entry_t *)(x))


ret_t cherokee_matching_list_new       (cherokee_matching_list_t **mlist);
ret_t cherokee_matching_list_free      (cherokee_matching_list_t  *mlist);

ret_t cherokee_matching_list_add_allow (cherokee_matching_list_t *mlist, const char *item);
ret_t cherokee_matching_list_add_deny  (cherokee_matching_list_t *mlist, const char *item);

ret_t cherokee_matching_list_configure (cherokee_matching_list_t *mlist, cherokee_config_node_t *config);
ret_t cherokee_matching_list_set_type  (cherokee_matching_list_t *mlist, cherokee_matching_t type);
int   cherokee_matching_list_match     (cherokee_matching_list_t *mlist, char *match);

#endif /* CHEROKEE_MACHING_LIST_H */
