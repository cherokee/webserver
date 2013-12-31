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

#ifndef CHEROKEE_TEMPLATE_H
#define CHEROKEE_TEMPLATE_H

#include <cherokee/common.h>
#include <cherokee/buffer.h>
#include <cherokee/list.h>

CHEROKEE_BEGIN_DECLS

typedef ret_t (* cherokee_tem_repl_func_t) (void *template, void *token, cherokee_buffer_t *output, void *param);

typedef struct {
	cherokee_buffer_t         text;
	cherokee_list_t           tokens;
	cherokee_list_t           replacements;
} cherokee_template_t;

typedef struct {
	cherokee_list_t           listed;
	cherokee_buffer_t         key;
	cherokee_tem_repl_func_t  func;
	void                     *param;
} cherokee_template_token_t;

#define TEMPLATE(x)       ((cherokee_template_t *)(x))
#define TEMPLATE_TOKEN(x) ((cherokee_template_token_t *)(x))
#define TEMPLATE_FUNC(x)  ((cherokee_tem_repl_func_t)(x))

/* Template
 */
ret_t cherokee_template_init       (cherokee_template_t *tem);
ret_t cherokee_template_mrproper   (cherokee_template_t *tem);

ret_t cherokee_template_new_token  (cherokee_template_t        *tem,
                                    cherokee_template_token_t **token);
ret_t cherokee_template_set_token  (cherokee_template_t        *tem,
                                    const char                 *name,
                                    cherokee_tem_repl_func_t    func,
                                    void                       *param,
                                    cherokee_template_token_t **token);

ret_t cherokee_template_parse      (cherokee_template_t *tem,
                                    cherokee_buffer_t   *incoming);

ret_t cherokee_template_parse_file (cherokee_template_t *tem,
                                    const char          *file);

ret_t cherokee_template_render     (cherokee_template_t *tem,
                                    cherokee_buffer_t   *output,
                                    void                *param);

CHEROKEE_END_DECLS

#endif /* CHEROKEE_TEMPLATE_H */
