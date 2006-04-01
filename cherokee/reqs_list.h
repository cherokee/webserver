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

#ifndef CHEROKEE_REQS_LIST_H
#define CHEROKEE_REQS_LIST_H

#include "common.h"
#include "buffer.h"
#include "list.h"
#include "regex.h"
#include "reqs_list_entry.h"
#include "config_entry.h"


typedef struct list_head cherokee_reqs_list_t;
#define REQLIST(x) ((cherokee_req_list_t *)(x))

ret_t cherokee_reqs_list_init     (cherokee_reqs_list_t *rl);
ret_t cherokee_reqs_list_mrproper (cherokee_reqs_list_t *rl);

ret_t cherokee_reqs_list_get      (cherokee_reqs_list_t *rl, cherokee_buffer_t *requested_url, cherokee_config_entry_t *plugin_entry, cherokee_connection_t *conn);
ret_t cherokee_reqs_list_add      (cherokee_reqs_list_t *rl, cherokee_reqs_list_entry_t *plugin_entry, cherokee_regex_table_t *regexs);

#endif /* CHEROKEE_REQS_LIST_H */
