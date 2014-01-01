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

#include "config_node.h"
#include "config_reader.h"
#include "util.h"

#define ENTRIES "config"


/* Implements _new() and _free()
 */
CHEROKEE_ADD_FUNC_NEW  (config_node);
CHEROKEE_ADD_FUNC_FREE (config_node);


ret_t
cherokee_config_node_init (cherokee_config_node_t *conf)
{
	INIT_LIST_HEAD (&conf->entry);
	INIT_LIST_HEAD (&conf->child);

	cherokee_buffer_init (&conf->key);
	cherokee_buffer_init (&conf->val);

	return ret_ok;
}


ret_t
cherokee_config_node_mrproper (cherokee_config_node_t *conf)
{
	cherokee_list_t *i, *j;

	cherokee_buffer_mrproper (&conf->key);
	cherokee_buffer_mrproper (&conf->val);

	list_for_each_safe (i, j, &conf->child) {
		cherokee_config_node_free (CONFIG_NODE(i));
	}

	return ret_ok;
}


static cherokee_config_node_t *
search_child (cherokee_config_node_t *current, cherokee_buffer_t *child)
{
	cherokee_list_t        *i;
	cherokee_config_node_t *entry;

	list_for_each (i, &current->child) {
		entry = CONFIG_NODE(i);

		if (strcmp (entry->key.buf, child->buf) == 0)
			return entry;
	}

	return NULL;
}


static cherokee_config_node_t *
add_new_child (cherokee_config_node_t *entry, cherokee_buffer_t *key)
{
	ret_t                   ret;
	cherokee_config_node_t *n;

	ret = cherokee_config_node_new (&n);
	if (ret != ret_ok) return NULL;

	cherokee_buffer_add_buffer (&n->key, key);

	cherokee_list_add_tail (LIST(n), &entry->child);
	return n;
}


ret_t
cherokee_config_node_add (cherokee_config_node_t *conf, const char *key, cherokee_buffer_t *val)
{
	char                   *sep;
	cherokee_config_node_t *child;
	cherokee_config_node_t *current = conf;
	const char             *begin   = key;
	cherokee_buffer_t       tmp     = CHEROKEE_BUF_INIT;
	cherokee_boolean_t      final   = false;

	/* 'include' is a special case
	 */
	if (equal_str (key, "include")) {
		return cherokee_config_reader_parse (conf, val);
	} else if (equal_str (key, "try_include")) {
		cherokee_config_reader_parse (conf, val);
	}

	do {
		/* Extract current child
		 */
		sep = strchr (begin, '!');
		if (sep != NULL) {
			cherokee_buffer_add (&tmp, begin, sep - begin);
		} else {
			cherokee_buffer_add (&tmp, begin, strlen(begin));
			final = true;
		}

		/* Look for the child entry
		 */
		child = search_child (current, &tmp);
		if (child == NULL) {
			child = add_new_child (current, &tmp);
			if (child == NULL) return ret_error;
		}

		if (final) {
			cherokee_buffer_clean (&child->val);
			cherokee_buffer_add_buffer (&child->val, val);
		}

		/* Prepare for next step
		 */
		begin = sep + 1;
		current = child;

		cherokee_buffer_clean (&tmp);

	} while (! final);

	cherokee_buffer_mrproper (&tmp);
	return ret_ok;
}


ret_t
cherokee_config_node_add_buf (cherokee_config_node_t *conf, cherokee_buffer_t *key, cherokee_buffer_t *val)
{
	return cherokee_config_node_add (conf, key->buf, val);
}


ret_t
cherokee_config_node_get (cherokee_config_node_t *conf, const char *key, cherokee_config_node_t **entry)
{
	char                   *sep;
	cherokee_config_node_t *child;
	cherokee_config_node_t *current = conf;
	const char             *begin   = key;
	cherokee_buffer_t       tmp     = CHEROKEE_BUF_INIT;
	cherokee_boolean_t      final   = false;

	do {
		/* Extract current child
		 */
		sep = strchr (begin, '!');
		if (sep != NULL) {
			cherokee_buffer_add (&tmp, begin, sep - begin);
		} else {
			cherokee_buffer_add (&tmp, begin, strlen(begin));
			final = true;
		}

		/* Look for the child entry
		 */
		child = search_child (current, &tmp);
		if (child == NULL) {
			cherokee_buffer_mrproper (&tmp);
			return ret_not_found;
		}

		if (final) {
			*entry = child;
		}

		/* Prepare for next step
		 */
		begin = sep + 1;
		current = child;

		cherokee_buffer_clean (&tmp);

	} while (! final);

	cherokee_buffer_mrproper (&tmp);
	return ret_ok;
}


ret_t
cherokee_config_node_get_buf (cherokee_config_node_t *conf, cherokee_buffer_t *key, cherokee_config_node_t **entry)
{
	return cherokee_config_node_get (conf, key->buf, entry);
}


ret_t
cherokee_config_node_while (cherokee_config_node_t *conf, cherokee_config_node_while_func_t func, void *data)
{
	ret_t            ret;
	cherokee_list_t *i;

	cherokee_config_node_foreach (i, conf) {
		ret = func (CONFIG_NODE(i), data);
		if (ret != ret_ok) return ret;
	}

	return ret_ok;
}


ret_t
cherokee_config_node_read (cherokee_config_node_t *conf, const char *key, cherokee_buffer_t **buf)
{
	ret_t                   ret;
	cherokee_config_node_t *tmp;

	ret = cherokee_config_node_get (conf, key, &tmp);
	if (ret != ret_ok) return ret;

	*buf = &tmp->val;
	return ret_ok;
}


ret_t
cherokee_config_node_copy (cherokee_config_node_t *conf, const char *key, cherokee_buffer_t *buf)
{
	ret_t              ret;
	cherokee_buffer_t *tmp = NULL;

	ret = cherokee_config_node_read (conf, key, &tmp);
	if (ret != ret_ok) return ret;

	return cherokee_buffer_add_buffer (buf, tmp);
}


ret_t
cherokee_config_node_read_int (cherokee_config_node_t *conf, const char *key, int *num)
{
	ret_t                   ret;
	cherokee_config_node_t *tmp;

	ret = cherokee_config_node_get (conf, key, &tmp);
	if (ret != ret_ok) return ret;

	if (cherokee_buffer_is_empty (&tmp->val)) {
		return ret_not_found;
	}

	ret = cherokee_atoi (tmp->val.buf, num);
	if (unlikely (ret != ret_ok))
		return ret_error;

	return ret_ok;
}


ret_t
cherokee_config_node_read_uint (cherokee_config_node_t *conf, const char *key, cuint_t *num)
{
	ret_t                   ret;
	cherokee_config_node_t *tmp;

	ret = cherokee_config_node_get (conf, key, &tmp);
	if (ret != ret_ok) return ret;

	if (cherokee_buffer_is_empty (&tmp->val)) {
		return ret_not_found;
	}

	ret = cherokee_atou (tmp->val.buf, num);
	if (unlikely (ret != ret_ok))
		return ret_error;

	return ret_ok;
}


ret_t
cherokee_config_node_read_long (cherokee_config_node_t *conf, const char *key, long *num)
{
	ret_t                   ret;
	cherokee_config_node_t *tmp;

	ret = cherokee_config_node_get (conf, key, &tmp);
	if (ret != ret_ok) return ret;

	if (cherokee_buffer_is_empty (&tmp->val)) {
		return ret_not_found;
	}

	*num = atol (tmp->val.buf);
	return ret_ok;
}


ret_t
cherokee_config_node_read_bool (cherokee_config_node_t *conf, const char *key, cherokee_boolean_t *val)
{
	ret_t ret;
	int   tmp;

	ret = cherokee_config_node_read_int (conf, key, &tmp);
	if (ret != ret_ok) return ret;

	*val = !! tmp;
	return ret_ok;
}


ret_t
cherokee_config_node_read_path (cherokee_config_node_t *conf, const char *key, cherokee_buffer_t **buf)
{
	ret_t                   ret;
	cherokee_config_node_t *tmp;

	if (key != NULL) {
		ret = cherokee_config_node_get (conf, key, &tmp);
		if (ret != ret_ok) return ret;
	} else {
		tmp = conf;
	}

	if (cherokee_buffer_is_empty (&tmp->val)) {
		return ret_not_found;
	}

	if (cherokee_buffer_end_char (&tmp->val) != '/')
		cherokee_buffer_add_str (&tmp->val, "/");

	*buf = &tmp->val;
	return ret_ok;
}


ret_t
cherokee_config_node_read_list (cherokee_config_node_t           *conf,
                                const char                       *key,
                                cherokee_config_node_list_func_t  func,
                                void                             *param)
{
	ret_t                   ret;
	char                   *ptr;
	char                   *stop;
	cherokee_config_node_t *tmp;

	if (key != NULL) {
		ret = cherokee_config_node_get (conf, key, &tmp);
		if (ret != ret_ok) return ret;
	} else {
		tmp = conf;
	}

	if (cherokee_buffer_is_empty (&tmp->val)) {
		return ret_not_found;
	}

	ptr = tmp->val.buf;

	if (ptr == NULL)
		return ret_not_found;

	for (;;) {
		while ((*ptr == ' ') && (*ptr != '\0'))
			ptr++;

		stop = strchr (ptr, ',');
		if (stop != NULL) *stop = '\0';

		ret = func (ptr, param);
		if (ret != ret_ok) return ret;

		if (stop == NULL)
			break;

		*stop = ',';
		ptr = stop + 1;
	}

	return ret_ok;
}


static ret_t
convert_to_list_step (char *entry, void *data)
{
	CHEROKEE_NEW(buf, buffer);

	cherokee_buffer_add (buf, entry, strlen(entry));
	return cherokee_list_add_tail_content (LIST(data), buf);
}


ret_t
cherokee_config_node_convert_list (cherokee_config_node_t *conf, const char *key, cherokee_list_t *list)
{
	return cherokee_config_node_read_list (conf, key, convert_to_list_step, list);
}
