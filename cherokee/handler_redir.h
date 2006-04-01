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

#ifndef CHEROKEE_HANDLER_REDIR_H
#define CHEROKEE_HANDLER_REDIR_H

#include "common.h"
#include "handler.h"
#include "module_loader.h"
#include "list.h"

typedef struct {
	cherokee_handler_t    handler;

	char                 *target_url;
	int                   target_url_len;

	list_t               *regex_list_ref;
	void                 *regex_list_cre;

	cherokee_boolean_t    use_previous_match;

} cherokee_handler_redir_t;

#define REHANDLER(x)  ((cherokee_handler_redir_t *)(x))


void MODULE_INIT(redir) (cherokee_module_loader_t *loader);
ret_t cherokee_handler_redir_new (cherokee_handler_t **hdl, void *cnt, cherokee_table_t *properties);

/* virtual methods implementation
 */
ret_t cherokee_handler_redir_init        (cherokee_handler_redir_t *rehdl);
ret_t cherokee_handler_redir_free        (cherokee_handler_redir_t *rehdl);
ret_t cherokee_handler_redir_add_headers (cherokee_handler_redir_t *rehdl, cherokee_buffer_t *buffer);

#endif /* CHEROKEE_HANDLER_REDIR_H */
