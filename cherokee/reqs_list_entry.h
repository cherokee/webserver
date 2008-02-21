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

#ifndef CHEROKEE_REQS_LIST_ENTRY_H
#define CHEROKEE_REQS_LIST_ENTRY_H

#include "common-internal.h"
#include "config_entry.h"

#define OVECTOR_LEN 10


typedef struct {
	cherokee_config_entry_t  base_entry;
	cherokee_buffer_t        request;
	cherokee_list_t          list_node;
	
	int                      ovector[OVECTOR_LEN];
	int                      ovecsize;
} cherokee_reqs_list_entry_t; 

#define RQ_ENTRY(x) ((cherokee_reqs_list_entry_t *)(x))


ret_t cherokee_reqs_list_entry_new  (cherokee_reqs_list_entry_t **entry);
ret_t cherokee_reqs_list_entry_free (cherokee_reqs_list_entry_t  *entry);


#endif /* CHEROKEE_REQS_LIST_ENTRY_H */
