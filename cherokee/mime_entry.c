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

#include "common-internal.h"
#include "mime_entry.h"

struct cherokee_mime_entry {
	cherokee_list_t    base;

	cuint_t            maxage;
	cherokee_boolean_t maxage_set;

	cherokee_buffer_t  mime_name;
};

ret_t
cherokee_mime_entry_new (cherokee_mime_entry_t **mentry)
{
	CHEROKEE_NEW_STRUCT(n, mime_entry);

	INIT_LIST_HEAD(&n->base);

	n->maxage     = -1;
	n->maxage_set = false;

	cherokee_buffer_init (&n->mime_name);

	*mentry = n;
	return ret_ok;
}


ret_t
cherokee_mime_entry_free (cherokee_mime_entry_t *mentry)
{
	cherokee_buffer_mrproper (&mentry->mime_name);

	free (mentry);
	return ret_ok;
}


ret_t
cherokee_mime_entry_set_type (cherokee_mime_entry_t *mentry, cherokee_buffer_t *type)
{
	cherokee_buffer_clean (&mentry->mime_name);
	return cherokee_buffer_add_buffer (&mentry->mime_name, type);
}


ret_t
cherokee_mime_entry_get_type (cherokee_mime_entry_t *mentry, cherokee_buffer_t **name)
{
	*name = &mentry->mime_name;
	return ret_ok;
}


ret_t
cherokee_mime_entry_set_maxage (cherokee_mime_entry_t *mentry, cuint_t maxage)
{
	mentry->maxage_set = true;

	mentry->maxage = maxage;
	return ret_ok;
}


ret_t
cherokee_mime_entry_get_maxage (cherokee_mime_entry_t *mentry, cuint_t *maxage)
{
	if (mentry->maxage_set == false)
		return ret_not_found;

	*maxage = mentry->maxage;
	return ret_ok;
}
