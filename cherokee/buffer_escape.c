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
#include "buffer_escape.h"

#include <stdio.h>
#include <stdlib.h>

ret_t 
cherokee_buffer_escape_new  (cherokee_buffer_escape_t **esc)
{
	int i;
	CHEROKEE_NEW_STRUCT (n, buffer_escape);
	   
	/* Init
	 */
	for (i=0; i<ESCAPE_NUM; i++) {
		n->internal[i]  = NULL;
		n->state[i]     = unchecked;
	}

	n->reference = NULL;

	/* Return the object
	 */
	*esc = n;
	return ret_ok;
}


ret_t 
cherokee_buffer_escape_free (cherokee_buffer_escape_t *esc)
{
	int i;

	for (i=0; i<ESCAPE_NUM; i++) {
		if (esc->internal[i] != NULL) {
			cherokee_buffer_free (esc->internal[i]);
			esc->internal[i] = NULL;
		}
	}

	free (esc);
	return ret_ok;
}


ret_t 
cherokee_buffer_escape_clean (cherokee_buffer_escape_t *esc)
{
	int i;

	for (i=0; i<ESCAPE_NUM; i++) {
		if (esc->internal[i] != NULL) {
			cherokee_buffer_free (esc->internal[i]);
			esc->internal[i]  = NULL;
		}

		esc->state[i] = unchecked;
	}

	esc->reference = NULL;
	   
	return ret_ok;
}


ret_t 
cherokee_buffer_escape_set_ref (cherokee_buffer_escape_t *esc, cherokee_buffer_t *buf_ref)
{
	/* The buffer has been already set
	 */
	if (buf_ref == esc->reference) 
		return ret_ok;

	/* If it's a different buffer, clean everything
	 */
	if (esc->reference != NULL) {
		cherokee_buffer_escape_clean (esc);
	}

	esc->reference = buf_ref;
	return ret_ok;
}


ret_t
cherokee_buffer_escape_get_html (cherokee_buffer_escape_t *esc, cherokee_buffer_t **buf_ref)
{
	ret_t ret;

	/* Sanity check
	 */
	if (esc->reference == NULL) SHOULDNT_HAPPEN;

	/* Look for the right buffer
	 */
	switch (esc->state[escape_html]) {
	case unchecked:
		ret = cherokee_buffer_escape_html (esc->reference, buf_ref);
		switch (ret) {
		case ret_ok:
			esc->internal[escape_html] = *buf_ref;
			esc->state[escape_html] = use_internal;
			break;
		case ret_not_found:
			*buf_ref = esc->reference;
			esc->state[escape_html] = use_reference;
			break;
		default:
			SHOULDNT_HAPPEN;
		}
		return ret_ok;
		   
	case use_internal:
		*buf_ref = esc->internal[escape_html];
		return ret_ok;
		   
	case use_reference:
		*buf_ref = esc->reference;
		return ret_ok;
		   
	default:
		SHOULDNT_HAPPEN;
	}
	   
	return ret_error;
}
