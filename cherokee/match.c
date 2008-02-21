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

#include "common-internal.h"
#include "match.h"


ret_t
cherokee_wildcard_match (const char *pattern, const char *text) 
{
	cint_t      ch;
	const char *retry_text     = NULL;
	const char *retry_pattern  = NULL;

	while (*text || *pattern) {
		ch = *pattern++;
		
		switch (ch) {
		case '*':
			retry_pattern = pattern;
			retry_text = text;
			break;

		case '?':
			if (*text++ == '\0')
				return ret_not_found;
			break;

		default:
			if (*text == ch) {
				if (*text) text++;
				break;
			}

			if (*text) {
				pattern = retry_pattern;
				text    = ++retry_text;
				break;
			}

			return ret_not_found;
		}

		if (pattern == NULL)
			return ret_not_found;
	}

	return ret_ok;
}
