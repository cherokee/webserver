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
#include "regex.h"
#include "table.h"
#include "pcre/pcre.h"


struct cherokee_regex_table {
	cherokee_table_t *cache;
#ifdef HAVE_PTHREAD
	pthread_rwlock_t rwlock;
#endif
};


ret_t 
cherokee_regex_table_new  (cherokee_regex_table_t **table)
{
	CHEROKEE_NEW_STRUCT (n, regex_table);
	   
	/* Init
	 */
	CHEROKEE_RWLOCK_INIT (&n->rwlock, NULL);
	cherokee_table_new (&n->cache);
	
	/* Return the new object
	 */
	*table = n;
	return ret_ok;
}


ret_t 
cherokee_regex_table_free (cherokee_regex_table_t *table)
{
	CHEROKEE_RWLOCK_DESTROY (&table->rwlock);
	cherokee_table_free2 (table->cache, free);
	
	free (table);
	return ret_ok;
}


static ret_t
_add (cherokee_regex_table_t *table, char *pattern, void **regex)
{
	void       *tmp;
	const char *error_msg;
	int         error_offset;

	/* It wasn't in the cache. Lets go to compile the pattern..
	 * First of all, we have to check again the table because another 
	 * thread could create a new entry after the previous unlock.
	 */
	CHEROKEE_RWLOCK_WRITER (&table->rwlock);

	tmp = (pcre*)cherokee_table_get_val (table->cache, pattern);
	if (tmp != NULL) {
		if (regex != NULL)
			*regex = tmp;

		CHEROKEE_RWLOCK_UNLOCK (&table->rwlock);
		return ret_ok;
	}
	
	tmp = pcre_compile (pattern, 0, &error_msg, &error_offset, NULL);
	if (tmp == NULL) {
		PRINT_ERROR ("ERROR: regex <<%s>>: \"%s\", offset %d\n", pattern, error_msg, error_offset);

		CHEROKEE_RWLOCK_UNLOCK (&table->rwlock);
		return ret_error;
	}
	
	cherokee_table_add (table->cache, pattern, tmp);
	CHEROKEE_RWLOCK_UNLOCK (&table->rwlock);
	
	if (regex != NULL)
		*regex = tmp;

	return ret_ok;
}


ret_t 
cherokee_regex_table_get (cherokee_regex_table_t *table, char *pattern, void **regex)
{
	/* Check if it is already in the cache
	 */
	CHEROKEE_RWLOCK_READER(&table->rwlock);
	*regex = (pcre*)cherokee_table_get_val (table->cache, pattern);
	CHEROKEE_RWLOCK_UNLOCK(&table->rwlock);
	if (*regex != NULL) return ret_ok;
	
	/* It wasn't there; add a new entry
	 */
	return _add (table, pattern, regex);
}


ret_t 
cherokee_regex_table_add (cherokee_regex_table_t *table, char *pattern)
{
	return _add (table, pattern, NULL);
}
