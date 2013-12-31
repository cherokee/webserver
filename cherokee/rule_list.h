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

#ifndef CHEROKEE_RULE_LIST_H
#define CHEROKEE_RULE_LIST_H

#include <cherokee/rule.h>
#include <cherokee/list.h>
#include <cherokee/connection.h>

CHEROKEE_BEGIN_DECLS

typedef struct {
	cherokee_list_t  rules;
	cherokee_rule_t *def_rule;
} cherokee_rule_list_t;


ret_t cherokee_rule_list_init     (cherokee_rule_list_t *list);
ret_t cherokee_rule_list_mrproper (cherokee_rule_list_t *list);

ret_t cherokee_rule_list_add      (cherokee_rule_list_t *list, cherokee_rule_t *rule);
ret_t cherokee_rule_list_sort     (cherokee_rule_list_t *list);

ret_t cherokee_rule_list_match    (cherokee_rule_list_t    *list,
                                   cherokee_connection_t   *conn,
                                   cherokee_config_entry_t *ret_config);

CHEROKEE_END_DECLS

#endif /* CHEROKEE_RULE_LIST_H */
