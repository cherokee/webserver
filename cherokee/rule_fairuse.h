/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/* Cherokee
 *
 * Authors:
 *      Alvaro Lopez Ortega <alvaro@alobbs.com>
 *      Stefan de Konink <stefan@konink.de>
 *
 * Copyright (C) 2001-2013 Alvaro Lopez Ortega
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

#ifndef CHEROKEE_RULE_FAIRUSE_H
#define CHEROKEE_RULE_FAIRUSE_H

#include <cherokee/common.h>
#include <cherokee/rule.h>
#include <cherokee/plugin_loader.h>
#include <gdbm.h>

CHEROKEE_BEGIN_DECLS

typedef struct {
	cherokee_rule_t    rule;
    GDBM_FILE          dbf;
    cuint_t            limit;
    cherokee_boolean_t count;
} cherokee_rule_fairuse_t;

typedef struct {
    cuint_t tm_yday;
    cuint_t count;
} track_ip_t;

#define RULE_FAIRUSE(x) ((cherokee_rule_fairuse_t *)(x))

void  PLUGIN_INIT_NAME(fairuse) (cherokee_plugin_loader_t *loader);
ret_t cherokee_rule_fairuse_new (cherokee_rule_fairuse_t **rule);

CHEROKEE_END_DECLS

#endif /* CHEROKEE_RULE_FAIRUSE_H */
