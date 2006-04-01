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

#if !defined (CHEROKEE_INSIDE_CHEROKEE_H) && !defined (CHEROKEE_COMPILATION)
# error "Only <cherokee/cherokee.h> can be included directly, this file may disappear or change contents."
#endif

#ifndef CHEROKEE_DICT_H
#define CHEROKEE_DICT_H

#include <cherokee/common.h>


CHEROKEE_BEGIN_DECLS

typedef int   (*cherokee_dict_cmp_func_t)   (const void *, const void *);
typedef void  (*cherokee_dict_del_func_t)   (void *);
typedef ret_t (*cherokee_dict_while_func_t) (void *key, void *value, void *param);

typedef struct dict cherokee_dict_t;
#define DICT(x) ((cherokee_dict_t *)(x))


ret_t cherokee_dict_new     (cherokee_dict_t        **dict, 
					    cherokee_dict_cmp_func_t key_cmp,
					    cherokee_dict_del_func_t key_del,
					    cherokee_dict_del_func_t val_del);

ret_t cherokee_dict_free    (cherokee_dict_t    *dict,
					    cherokee_boolean_t  free);

ret_t cherokee_dict_clean   (cherokee_dict_t    *dict, 
					    cherokee_boolean_t  free);

ret_t cherokee_dict_get     (cherokee_dict_t *dict, char *key, void **value);
ret_t cherokee_dict_add     (cherokee_dict_t *dict, void *key, void  *value, cherokee_boolean_t overwrite);
ret_t cherokee_dict_len     (cherokee_dict_t *dict, unsigned int *len);

ret_t cherokee_dict_foreach (cherokee_dict_t            *dict, 
					    cherokee_dict_while_func_t  func,
					    void                       *param,
					    void                      **key,
					    void                      **value);

CHEROKEE_END_DECLS

#endif /* CHEROKEE_DICT_H */
