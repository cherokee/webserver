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
#include "config_node.h"


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
	cherokee_buffer_mrproper (&conf->key);
	cherokee_buffer_mrproper (&conf->val);

	return ret_ok;
}


static cherokee_config_node_t *
search_child (cherokee_config_node_t *current, cherokee_buffer_t *child)
{
	list_t                        *i;
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
	cherokee_config_node_t *n;

	n = (cherokee_config_node_t *) malloc(sizeof(cherokee_config_node_t));
	if (unlikely(n==NULL)) return NULL;
	   
	cherokee_config_node_init (n);
	cherokee_buffer_add_buffer (&n->key, key);	   

	list_add ((list_t *)n, &entry->child);
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

	do {
		/* Extract current child
		 */
		sep = strchr (begin, '!');
		if (sep != NULL) {
			cherokee_buffer_add (&tmp, (char *)begin, sep - begin);
		} else {
			cherokee_buffer_add (&tmp, (char *)begin, strlen(begin));
			final = true;
		}

		/* Look for the child entry
		 */
		child = search_child (current, &tmp);
		if (child == NULL) {
			child = add_new_child (current, &tmp);

			if (final) {
				cherokee_buffer_add_buffer (&child->val, val);
			}
		}

		/* Prepare for next step
		 */
		begin = sep + 1;
		current = child;

		cherokee_buffer_clean (&tmp);

	} while (! final);

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
			cherokee_buffer_add (&tmp, (char *)begin, sep - begin);
		} else {
			cherokee_buffer_add (&tmp, (char *)begin, strlen(begin));
			final = true;
		}

		/* Look for the child entry
		 */
		child = search_child (current, &tmp);
		if (child == NULL) return ret_error;

		if (final) {
			*entry = child;
		}

		/* Prepare for next step
		 */
		begin = sep + 1;
		current = child;

		cherokee_buffer_clean (&tmp);

	} while (! final);

	return ret_ok;
}

ret_t 
cherokee_config_node_get_buf (cherokee_config_node_t *conf, cherokee_buffer_t *key, cherokee_config_node_t **entry)
{
	return cherokee_config_node_get (conf, key->buf, entry);
}



ret_t 
cherokee_config_node_foreach (cherokee_config_node_t *conf, cherokee_config_node_foreach_func_t func, void *data)
{
	list_t *i;

	list_for_each (i, &conf->child) {
		func (CONFIG_NODE(i), data);
	}

	return ret_ok;
}

