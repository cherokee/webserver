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

#ifndef CHEROKEE_MIME_PROTECTED_H
#define CHEROKEE_MIME_PROTECTED_H

#include "list.h"
#include "avl.h"

struct cherokee_mime {
	cherokee_avl_t   ext_table;
	cherokee_list_t  entry_list;
};

#define MIME_TABLE(m)  (&((cherokee_mime_t *)(m))->mime_table)
#define MIME_LIST(m)   (&((cherokee_mime_t *)(m))->mime_list)

#endif /* CHEROKEE_MIME_PROTECTED_H */
