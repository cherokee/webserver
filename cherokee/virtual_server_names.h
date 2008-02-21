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

#if !defined (CHEROKEE_INSIDE_CHEROKEE_H) && !defined (CHEROKEE_COMPILATION)
# error "Only <cherokee/cherokee.h> can be included directly, this file may disappear or change contents."
#endif
     
#ifndef CHEROKEE_VSERVER_LIST_H
#define CHEROKEE_VSERVER_LIST_H

#include <cherokee/common.h>
#include <cherokee/list.h>
#include <cherokee/buffer.h>


/* Virtual server names
 */
typedef cherokee_list_t cherokee_vserver_names_t;

ret_t cherokee_vserver_names_mrproper (cherokee_vserver_names_t *list);
ret_t cherokee_vserver_names_add_name (cherokee_vserver_names_t *list, cherokee_buffer_t *name);
ret_t cherokee_vserver_names_find     (cherokee_vserver_names_t *list, cherokee_buffer_t *name);


/* Virtual server entry
 */
typedef struct {
	cherokee_list_t     node;
	cherokee_buffer_t   name;
	cherokee_boolean_t  is_wildcard;
} cherokee_vserver_name_entry_t;

#define VSERVER_NAME(v) ((cherokee_vserver_name_entry_t *)(v))

ret_t cherokee_vserver_name_entry_new   (cherokee_vserver_name_entry_t **entry, cherokee_buffer_t *name);
ret_t cherokee_vserver_name_entry_match (cherokee_vserver_name_entry_t  *entry, cherokee_buffer_t *name);
ret_t cherokee_vserver_name_entry_free  (cherokee_vserver_name_entry_t  *entry);

#endif /* CHEROKEE_VSERVER_LIST_H */
