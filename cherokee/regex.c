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
#include "regex.h"
#include "avl.h"
#include "util.h"

#define ENTRIES "regex"

struct cherokee_regex_table {
	cherokee_avl_t      cache;
	CHEROKEE_RWLOCK_T  (rwlock);
};


ret_t
cherokee_regex_table_new  (cherokee_regex_table_t **table)
{
	CHEROKEE_NEW_STRUCT (n, regex_table);

	/* Init
	 */
	CHEROKEE_RWLOCK_INIT (&n->rwlock, NULL);
	cherokee_avl_init (&n->cache);

	/* Return the new object
	 */
	*table = n;
	return ret_ok;
}


ret_t
cherokee_regex_table_free (cherokee_regex_table_t *table)
{
	CHEROKEE_RWLOCK_DESTROY (&table->rwlock);

	cherokee_avl_mrproper (AVL_GENERIC(&table->cache), free);

	free(table);
	return ret_ok;
}


static ret_t
_add (cherokee_regex_table_t *table, char *pattern, void **regex)
{
	ret_t      ret;
	const char *error_msg;
	int         error_offset;
	void       *tmp           = NULL;

	/* It wasn't in the cache. Lets go to compile the pattern..
	 * First of all, we have to check again the table because another
	 * thread could create a new entry after the previous unlock.
	 */
	CHEROKEE_RWLOCK_WRITER (&table->rwlock);

	ret = cherokee_avl_get_ptr (&table->cache, pattern, &tmp);
	if ((tmp != NULL) && (ret == ret_ok)) {
		if (regex != NULL)
			*regex = tmp;

		CHEROKEE_RWLOCK_UNLOCK (&table->rwlock);
		return ret_ok;
	}

	tmp = pcre_compile (pattern, 0, &error_msg, &error_offset, NULL);
	if (tmp == NULL) {
		LOG_ERROR (CHEROKEE_ERROR_REGEX_COMPILATION, pattern, error_msg, error_offset);
		CHEROKEE_RWLOCK_UNLOCK (&table->rwlock);
		return ret_error;
	}

	cherokee_avl_add_ptr (&table->cache, pattern, tmp);
	CHEROKEE_RWLOCK_UNLOCK (&table->rwlock);

	if (regex != NULL)
		*regex = tmp;

	return ret_ok;
}


ret_t
cherokee_regex_table_get (cherokee_regex_table_t *table, char *pattern, void **regex)
{
	ret_t ret;

	/* Check if it is already in the cache
	 */
	CHEROKEE_RWLOCK_READER(&table->rwlock);
	ret = cherokee_avl_get_ptr (&table->cache, pattern, regex);
	CHEROKEE_RWLOCK_UNLOCK(&table->rwlock);
	if (ret == ret_ok)
		return ret_ok;

	/* It wasn't there; add a new entry
	 */
	return _add (table, pattern, regex);
}


ret_t
cherokee_regex_table_add (cherokee_regex_table_t *table, char *pattern)
{
	return _add (table, pattern, NULL);
}


/* RegEx lists
 */

static ret_t
configure_rewrite_entry (cherokee_list_t        *list,
			 cherokee_config_node_t *conf,
			 cherokee_regex_table_t *regexs)
{
	ret_t                   ret;
	cherokee_regex_entry_t *n;
	cherokee_buffer_t      *substring;
	cherokee_boolean_t      hidden     = true;
	pcre                   *re         = NULL;
	cherokee_buffer_t      *regex      = NULL;

	TRACE(ENTRIES, "Converting rewrite rule '%s'\n", conf->key.buf);

	/* Query conf
	 */
	cherokee_config_node_read_bool (conf, "show", &hidden);
	hidden = !hidden;

	ret = cherokee_config_node_read (conf, "regex", &regex);
	if (ret == ret_ok) {
		ret = cherokee_regex_table_get (regexs, regex->buf, (void **)&re);
		if (ret != ret_ok)
			return ret;
	}

	ret = cherokee_config_node_read (conf, "substring", &substring);
	if (ret != ret_ok)
		return ret;

	/* New RegEx
	 */
	n = (cherokee_regex_entry_t *) malloc(sizeof(cherokee_regex_entry_t));
	if (unlikely (n == NULL))
		return ret_nomem;

	INIT_LIST_HEAD (&n->listed);
	n->re         = re;
	n->hidden     = hidden;

	cherokee_buffer_init (&n->subs);
	cherokee_buffer_add_buffer (&n->subs, substring);

	/* Add the list
	 */
	cherokee_list_add_tail (&n->listed, list);
	return ret_ok;
}


ret_t
cherokee_regex_list_configure (cherokee_list_t        *list,
                               cherokee_config_node_t *conf,
                               cherokee_regex_table_t *regexs)
{
	ret_t            ret;
	cherokee_list_t *i;

	cherokee_config_node_foreach (i, conf) {
		ret = configure_rewrite_entry (list, CONFIG_NODE(i), regexs);
		if (ret != ret_ok)
			return ret;
	}

	return ret_ok;
}


ret_t
cherokee_regex_list_mrproper (cherokee_list_t *list)
{
	cherokee_list_t *i, *tmp;

	list_for_each_safe (i, tmp, list) {
		cherokee_regex_entry_t *n = REGEX_ENTRY(i);

		cherokee_buffer_mrproper (&n->subs);
		free (i);
	}

	return ret_ok;
}


ret_t
cherokee_regex_substitute (cherokee_buffer_t *regex_str,
                           cherokee_buffer_t *source,
                           cherokee_buffer_t *target,
                           cint_t             ovector[],
                           cint_t             stringcount,
                           char               dollar_char)
{
	cint_t              re;
	char               *s;
	char                num;
	cherokee_boolean_t  dollar    = false;
	const char         *substring = NULL;

	for (s = regex_str->buf; *s != '\0'; s++) {
		if (! dollar) {
			if (*s == dollar_char)
				dollar = true;
			else
				cherokee_buffer_add_char (target, *s);
			continue;
		}

		/* Convert the $<num>. Limit 99.
		 */
		if ((*s >= '0') && (*s <= '9')) {
			num = *s - '0';
			if ((s[1] >= '0') && (s[1] <= '9')) {
				s++;
				num = (num * 10) + (*s - '0');
			}
		} else {
			/* Add the characters if it wasn't a number
			 */
			cherokee_buffer_add_char (target, dollar_char);
			cherokee_buffer_add_char (target, *s);

			dollar = false;
			continue;
		}

		/* Perform the actually substitution
		 */
		substring = NULL;

		re = pcre_get_substring (source->buf, ovector, stringcount, num, &substring);
		if ((re < 0) || (substring == NULL)) {
			dollar = false;
			continue;
		}

		cherokee_buffer_add (target, substring, strlen(substring));
		pcre_free_substring (substring);
		dollar = false;
	}

	return ret_ok;
}
