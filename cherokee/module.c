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
#include "module.h"

ret_t 
cherokee_module_init_base  (cherokee_module_t *module)
{
	   module->instance = NULL;
	   module->init     = NULL;
	   module->get_name = NULL;
	   module->free     = NULL;

	   return ret_ok;
}


void
cherokee_module_get_name (cherokee_module_t *module, const char **name)
{
	/* Sanity check
	 */
	if (module == NULL) return;

	if (module->get_name != NULL) {
		module->get_name (module, name);
		return;
	}

	*name = "unknown";
}

