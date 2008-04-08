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

#ifndef CHEROKEE_MIME_ENTRY_H
#define CHEROKEE_MIME_ENTRY_H

#include <cherokee/common.h>
#include <cherokee/buffer.h>
#include <cherokee/list.h>


CHEROKEE_BEGIN_DECLS

typedef struct cherokee_mime_entry cherokee_mime_entry_t;
#define MIME_ENTRY(m) ((cherokee_mime_entry_t *)(m))

ret_t cherokee_mime_entry_new  (cherokee_mime_entry_t **mentry);
ret_t cherokee_mime_entry_free (cherokee_mime_entry_t  *mentry);

ret_t cherokee_mime_entry_set_type (cherokee_mime_entry_t *mentry, cherokee_buffer_t *type);
ret_t cherokee_mime_entry_get_type (cherokee_mime_entry_t *mentry, cherokee_buffer_t **name);

ret_t cherokee_mime_entry_set_maxage (cherokee_mime_entry_t *mentry, cuint_t  maxage);
ret_t cherokee_mime_entry_get_maxage (cherokee_mime_entry_t *mentry, cuint_t *maxage);

CHEROKEE_END_DECLS

#endif /* CHEROKEE_MIME_ENTRY_H */
