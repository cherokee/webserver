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

#include "common-internal.h"
#include "virtual_entries.h"


ret_t 
cherokee_virtual_entries_init (cherokee_virtual_entries_t *ventry)
{
	ret_t ret;
	
	ventry->exts = NULL;
	INIT_LIST_HEAD (&ventry->reqs);
	
	ret = cherokee_dirs_table_init (&ventry->dirs);
	if (unlikely(ret < ret_ok)) return ret;

	return ret_ok;
}


ret_t 
cherokee_virtual_entries_mrproper (cherokee_virtual_entries_t *ventry)
{
	cherokee_dirs_table_mrproper (&ventry->dirs);
	cherokee_reqs_list_mrproper (&ventry->reqs);

	if (ventry->exts != NULL) {
		cherokee_exts_table_free (ventry->exts);
		ventry->exts = NULL;
	}

	return ret_ok;
}

