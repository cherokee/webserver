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
#include "session_cache.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "buffer.h"


typedef struct {
	cherokee_buffer_t *key;
	cherokee_buffer_t *value;
} item_t;




static item_t *
new_item (void)
{
	item_t *item = (item_t *) malloc(sizeof(item_t));

	cherokee_buffer_new (&item->key);
	cherokee_buffer_new (&item->value);

	return item;
}

static void
del_item (void *avl_item, void *avl_param)
{
	item_t *item = (item_t *)avl_item;

	cherokee_buffer_free (item->value);
	cherokee_buffer_free (item->key);

	free (item);
}

static int
equal (const void *a, const void *b, void *param)
{
	if (BUF(a)->len > BUF(b)->len) 
		return BUF(a)->len - BUF(b)->len;

	if (BUF(b)->len > BUF(a)->len) 
		return -(BUF(b)->len - BUF(a)->len);

	return memcmp(BUF(a)->buf, BUF(b)->buf, BUF(a)->len);
} 




ret_t 
cherokee_session_cache_new (cherokee_session_cache_t **tab)
{
	CHEROKEE_NEW_STRUCT (n, session_cache);

	n->tree = avl_create (equal, NULL, NULL);

	/* Return the object
	 */
	*tab = n;
	return ret_ok;
}


ret_t 
cherokee_session_cache_free (cherokee_session_cache_t *tab)
{
	avl_destroy (tab->tree, del_item);

	free (tab);
	return ret_ok;
}


ret_t 
cherokee_session_cache_add (cherokee_session_cache_t *tab,
			    unsigned char *key,   unsigned int key_len,
			    unsigned char *value, unsigned int value_len)
{
	item_t *node;

	/* Create the AVL tree node
	 */
	node = new_item();

	/* Copy the info to the buffers
	 */
	cherokee_buffer_add (node->key, (char *)key, key_len);
	cherokee_buffer_add (node->value, (char *)value, value_len);

	/* Add the node to the tree
	 */
	avl_insert (tab->tree, node);
	
	return ret_ok;
}


ret_t 
cherokee_session_cache_retrieve  (cherokee_session_cache_t *tab,
				  unsigned char *key, int  key_len,
				  void **buf, unsigned int *buf_len)
{
	item_t n, *found;
	CHEROKEE_NEW(k,buffer);

	/* Add the key
	 */
	cherokee_buffer_add (k, (char *)key, key_len);

	/* Search the key in the tree
	 */
	n.key = k;
	found = avl_delete (tab->tree, &n);

	/* Free the temporal buffer
	 */
	cherokee_buffer_free (k);

	if (found != NULL) {
		char              *new_buf;
		cherokee_buffer_t *b = found->value;
		
		/* Get memory and copy the content
		 */
		new_buf = (char *) malloc(b->len);
		memcpy (new_buf, b->buf, b->len);

		/* Return the parameters
		 */
		*buf     = new_buf;
		*buf_len = b->len;

		return ret_ok;
	}

	return ret_not_found;
}


ret_t 
cherokee_session_cache_del (cherokee_session_cache_t *tab, 
			    unsigned char *key, int key_len)
{
	item_t n, *found;
	CHEROKEE_NEW(k,buffer);
	
	cherokee_buffer_add (k, (char *)key, key_len);
	n.key = k;
	
	found = avl_delete (tab->tree, &n);
	cherokee_buffer_free (k);

	return (found != NULL) ? ret_ok : ret_error;
}
