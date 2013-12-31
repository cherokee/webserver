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

#ifndef CHEROKEE_RULE_H
#define CHEROKEE_RULE_H

#include <cherokee/common.h>
#include <cherokee/module.h>
#include <cherokee/buffer.h>
#include <cherokee/list.h>
#include <cherokee/plugin.h>
#include <cherokee/config_entry.h>

CHEROKEE_BEGIN_DECLS

#define CHEROKEE_RULE_PRIO_NONE    0
#define CHEROKEE_RULE_PRIO_DEFAULT 1

/* Callback function definitions
 */
typedef ret_t (* rule_func_new_t)       (void **rule);
typedef ret_t (* rule_func_configure_t) (void  *rule, cherokee_config_node_t *conf, void *vsrv);
typedef ret_t (* rule_func_match_t)     (void  *rule, void *cnt, void *ret_conf);

/* Data types
 */
struct cherokee_rule {
	cherokee_module_t       module;

	/* Properties */
	cherokee_list_t         list_node;
	cherokee_config_entry_t config;

	cherokee_boolean_t      final;
	cuint_t                 priority;
	struct cherokee_rule   *parent_rule;

	/* Virtual methods */
	rule_func_match_t       match;
	rule_func_configure_t   configure;
};

typedef struct cherokee_rule cherokee_rule_t;
#define RULE(x) ((cherokee_rule_t *)(x))

/* Easy initialization
 */
#define PLUGIN_INFO_RULE_EASY_INIT(name)                         \
	PLUGIN_INFO_INIT(name, cherokee_rule,                    \
		(void *)cherokee_rule_ ## name ## _new,          \
		(void *)NULL)

#define PLUGIN_INFO_RULE_EASIEST_INIT(name)                      \
	PLUGIN_EMPTY_INIT_FUNCTION(name)                         \
	PLUGIN_INFO_RULE_EASY_INIT(name)

/* Methods
 */
ret_t cherokee_rule_free        (cherokee_rule_t *rule);

/* Rule methods
 */
ret_t cherokee_rule_init_base   (cherokee_rule_t *rule, cherokee_plugin_info_t *info);

/* Rule virtual methods
 */
ret_t cherokee_rule_match       (cherokee_rule_t *rule, void *cnt, void *ret_conf);
ret_t cherokee_rule_configure   (cherokee_rule_t *rule, cherokee_config_node_t *conf, void *vsrv);

/* Utilities
 */
void cherokee_rule_get_config  (cherokee_rule_t *rule, cherokee_config_entry_t **config);

CHEROKEE_END_DECLS

#endif /* CHEROKEE_RULE_H */
