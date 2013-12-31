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
#include "human_strcmp.h"

#include <ctype.h>


static int
parsenum (const char *a, const char **a_ret)
{
	int result = *a - '0';

	/* Parse the number
	 */
	++a;
	while (isdigit(*a)) {
		result *= 10;
		result += *a - '0';
		++a;
	}

	/* Return the new pointer
	 */
	if (a_ret)
		*a_ret = a - 1;

	/* .. and the number
	 */
	return result;
}


int
cherokee_human_strcmp (const char *a, const char *b)
{
	int a0;
	int b0;

	/* Check corner cases
	 */
	if (unlikely (a == b))
		return  0;
	if (unlikely (a == NULL))
		return -1;
	if (unlikely (b == NULL))
		return  1;

	/* Iterate
	 */
	while (*a && *b) {
		if (isdigit(*a))
			a0 = parsenum(a, &a) + 256;
		else
			a0 = tolower(*a);

		if (isdigit(*b))
			b0 = parsenum(b, &b) + 256;
		else
			b0 = tolower(*b);

		if (a0 < b0)
			return -1;
		if (a0 > b0)
			return 1;

		a++;
		b++;
	}

	if (*a) return 1;
	if (*b) return -1;

	return 0;

}
