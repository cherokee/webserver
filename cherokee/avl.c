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
#include "avl.h"

#define MAX_HEIGHT 45


struct cherokee_avl_node {	   
	/* Tree */
	short                     balance;
	struct cherokee_avl_node *left;
	struct cherokee_avl_node *right;
	cherokee_boolean_t        left_child;
	cherokee_boolean_t        right_child;

	/* Info */
	cherokee_buffer_t         id;
	void                     *value;
};


static cherokee_avl_node_t *node_first (cherokee_avl_t *avl);
static cherokee_avl_node_t *node_prev  (cherokee_avl_node_t *node);
static cherokee_avl_node_t *node_next  (cherokee_avl_node_t *node);


/* Nodes
 */
static ret_t 
node_new (cherokee_avl_node_t **node, cherokee_buffer_t *key, void *value)
{
	CHEROKEE_NEW_STRUCT (n, avl_node);

	n->balance     = 0;
	n->left        = NULL;
	n->right       = NULL;
	n->left_child  = false;
	n->right_child = false;
	n->value       = value;

	cherokee_buffer_init (&n->id);
	cherokee_buffer_add_buffer (&n->id, key);

	*node = n;
	return ret_ok;
}

static ret_t 
node_free (cherokee_avl_node_t *node)
{
	cherokee_buffer_mrproper (&node->id);

	free (node);
	return ret_ok;
}

/* Tree constructor / destructor 
 */

ret_t 
cherokee_avl_init (cherokee_avl_t *avl)
{
	avl->root             = NULL;
	avl->case_insensitive = false;

	return ret_ok;
}

CHEROKEE_ADD_FUNC_NEW (avl);


ret_t 
cherokee_avl_mrproper (cherokee_avl_t *avl, cherokee_func_free_t free_func)
{
	cherokee_avl_node_t *node;
	cherokee_avl_node_t *next;

	if (unlikely (avl == NULL))
		return ret_ok;

	node = node_first (avl);
  
	while (node) {
		next = node_next (node);

		if (free_func)
			free_func (node->value);
		node_free (node);

		node = next;
	}

	return ret_ok;
}

ret_t 
cherokee_avl_free (cherokee_avl_t *avl, cherokee_func_free_t free_func)
{
	cherokee_avl_mrproper (avl, free_func);
	free (avl);
	return ret_ok;
}


/* Tree methods
 */

static int 
compare_buffers (cherokee_buffer_t *A,
		 cherokee_buffer_t *B,
		 cherokee_boolean_t case_insensitive)
{
	if (case_insensitive)
		return cherokee_buffer_cmp_buf (A, B);
	else
		return cherokee_buffer_case_cmp_buf (A, B);
}


ret_t 
cherokee_avl_set_case (cherokee_avl_t *avl, cherokee_boolean_t case_insensitive)
{
	avl->case_insensitive = case_insensitive;
	return ret_ok;
}


static cherokee_avl_node_t *
node_first (cherokee_avl_t *avl)
{
	cherokee_avl_node_t *tmp;

	if (!avl->root)	
		return NULL;

	tmp = avl->root;

	while (tmp->left_child)
		tmp = tmp->left;

	return tmp;
}

static cherokee_avl_node_t *
node_next (cherokee_avl_node_t *node)
{
	cherokee_avl_node_t *tmp = node->right;

	if (node->right_child)
		while (tmp->left_child)
			tmp = tmp->left;
	return tmp;
}

static cherokee_avl_node_t *
node_prev (cherokee_avl_node_t *node)
{
	cherokee_avl_node_t *tmp = node->left;

	if (node->left_child)
		while (tmp->right_child)
			tmp = tmp->right;
	return tmp;
}


static cherokee_avl_node_t *
node_rotate_left (cherokee_avl_node_t *node)
{
	cherokee_avl_node_t *right;
	cint_t               a_bal;
	cint_t               b_bal;
  
	right = node->right;

	if (right->left_child)
		node->right = right->left;
	else {
		node->right_child = FALSE;
		node->right = right;
		right->left_child = TRUE;
	}
	right->left = node;

	a_bal = node->balance;
	b_bal = right->balance;

	if (b_bal <= 0) {
		if (a_bal >= 1)
			right->balance = b_bal - 1;
		else
			right->balance = a_bal + b_bal - 2;
		node->balance = a_bal - 1;
	} else {
		if (a_bal <= b_bal)
			right->balance = a_bal - 2;
		else
			right->balance = b_bal - 1;
		node->balance = a_bal - b_bal - 1;
	}

	return right;
}

static cherokee_avl_node_t *
node_rotate_right (cherokee_avl_node_t *node)
{
	cherokee_avl_node_t *left;
	cint_t               a_bal;
	cint_t               b_bal;

	left = node->left;
	
	if (left->right_child)
		node->left = left->right;
	else {
		node->left_child = FALSE;
		node->left = left;
		left->right_child = TRUE;
	}
	left->right = node;

	a_bal = node->balance;
	b_bal = left->balance;

	if (b_bal <= 0) {
		if (b_bal > a_bal)
			left->balance = b_bal + 1;
		else
			left->balance = a_bal + 2;
		node->balance = a_bal - b_bal + 1;
	} else {
		if (a_bal <= -1)
			left->balance = b_bal + 1;
		else
			left->balance = a_bal + b_bal + 2;
		node->balance = a_bal + 1;
	}

	return left;
}


static cherokee_avl_node_t *
node_balance (cherokee_avl_node_t *node)
{
	if (node->balance < -1) {
		if (node->left->balance > 0)
			node->left = node_rotate_left (node->left);
		node = node_rotate_right (node);

	} else if (node->balance > 1) {
		if (node->right->balance < 0)
			node->right = node_rotate_right (node->right);
		node = node_rotate_left (node);
	}

	return node;
}



static void
node_add (cherokee_avl_t *tree, cherokee_avl_node_t *child)
{
	short                re;                 
	cherokee_boolean_t   is_left;
	cherokee_avl_node_t *path[MAX_HEIGHT];
	cherokee_avl_node_t *node   = tree->root;
	cherokee_avl_node_t *parent = NULL;
	cint_t               idx    = 1;

	path[0] = NULL;

	/* If the tree is empty..
	 */
	if (tree->root == NULL) {
		tree->root = child;
		return;
	}

	/* Insert the node
	 */
	while (true) {
		re = compare_buffers (&child->id, &node->id, tree->case_insensitive);

		if (re < 0) {
			/* Insert it on the left */
			if (node->left_child) {
				path[idx++] = node;
				node        = node->left;
			} else {
				child->left      = node->left;
				child->right     = node;
				node->left       = child;
				node->left_child = true;
				node->balance   -= 1;
				break;
			}

		} else if (re > 0) {
			/* Insert it on the left */
			if (node->right_child) {
				path[idx++] = node;
				node        = node->right;
			} else {
				child->right      = node->right;
				child->left       = node;
				node->right       = child;
				node->right_child = true;
				node->balance    += 1;
				break;
			}

		} else {
			node_free (child);
			return;
		}
	}

	/* Rebalance the tree
	 */
	while (true) {
		parent = path[--idx];
		is_left = (parent && (parent->left == node));
		
		if ((node->balance < -1) ||
		    (node->balance >  1)) 
		{
			node = node_balance (node);
			if (parent == NULL) 
				tree->root = node;
			else if (is_left) 
				parent->left = node;
			else
				parent->right = node;
		}

		if ((node->balance == 0) || (parent == NULL))
			break;
		
		if (is_left)
			parent->balance -= 1;
		else
			parent->balance += 1;
		
		node = parent;
	}
}


ret_t 
cherokee_avl_add (cherokee_avl_t *avl, cherokee_buffer_t *key, void *value)
{
	ret_t                ret;
	cherokee_avl_node_t *n = NULL;

	if (unlikely (cherokee_buffer_is_empty(key)))
		return ret_error;

	/* Create the new AVL node
	 */
	ret = node_new (&n, key, value);
	if (unlikely (ret != ret_ok)) return ret;

	/* Add th node to the tree
	 */
	node_add (avl, n);
	return ret_ok;
}


ret_t 
cherokee_avl_del (cherokee_avl_t *avl, cherokee_buffer_t *key, void **value)
{
	short                re;
	cherokee_boolean_t   is_left;
	cherokee_avl_node_t *path[MAX_HEIGHT];
	cherokee_avl_node_t *parent;
	cherokee_avl_node_t *pbalance;
	cherokee_avl_node_t *node     = avl->root;
	cint_t               idx      = 1;

	if (unlikely (cherokee_buffer_is_empty(key)))
		return ret_error;
	
	if (avl->root == NULL)
		return ret_not_found;

	path[0] = NULL;

	while (true) {
		re = compare_buffers (key, &node->id, avl->case_insensitive);
		if (re == 0) {
			if (value) 
				*value = node->value;
			break;
		} else if (re < 0) {
			if (!node->left_child)
				return ret_not_found;
			path[idx++] = node;
			node = node->left;
		} else {
			if (!node->right_child)
				return ret_not_found;
			path[idx++] = node;
			node = node->right;
		}
	}

	pbalance = path[idx-1];
	parent   = pbalance;
	idx     -= 1;
	is_left  = (parent && (node == parent->left));

	if (!node->left_child) {
		if (!node->right_child) {
			if (!parent)
				avl->root = NULL;
			else if (is_left) {
				parent->left_child  = false;
				parent->left        = node->left;
				parent->balance   += 1;
			} else {
				parent->right_child = false;
				parent->right       = node->right;
				parent->balance     -= 1;
			}

		} else { /* right child */
			cherokee_avl_node_t *tmp = node_next (node);
			tmp->left = node->left;

			if (!parent)
				avl->root = node->right;
			else if (is_left) {
				parent->left     = node->right;
				parent->balance += 1;
			} else {
				parent->right    = node->right;
				parent->balance -= 1;
			}
		}

	} else { /* left child */
		if (!node->right_child) {
			cherokee_avl_node_t *tmp = node_prev(node);
			tmp->right = node->right;

			if (parent == NULL)
				avl->root = node->left;
			else if (is_left) {
				parent->left     = node->left;
				parent->balance += 1;
			} else {
				parent->right    = node->left;
				parent->balance -= 1;
			}
		} else { /* both children */
			cherokee_avl_node_t *prev    = node->left;
			cherokee_avl_node_t *next    = node->right;
			cherokee_avl_node_t *nextp   = node;
			cint_t               old_idx = idx + 1;
			idx++;

			/* find the immediately next node (and its parent) */
			while (next->left_child) {
				path[++idx] = nextp = next;
				next = next->left;
			}
 	  
			path[old_idx] = next;
			pbalance      = path[idx];
	  
			/* remove 'next' from the tree */
			if (nextp != node) {
				if (next->right_child)
					nextp->left = next->right;
				else
					nextp->left_child = FALSE;

				nextp->balance    += 1;
				next->right_child  = TRUE;
				next->right        = node->right;

			} else {
				node->balance -= 1;
			}
	    
			/* set the prev to point to the right place */
			while (prev->right_child)
				prev = prev->right;
			prev->right = next;
	    
			/* prepare 'next' to replace 'node' */
			next->left_child = TRUE;
			next->left = node->left;
			next->balance = node->balance;
	  
			if (!parent)
				avl->root = next;
			else if (is_left)
				parent->left = next;
			else
				parent->right = next;
		}
	} 
	
	/* restore balance */
	if (pbalance) {
		while (true) {
			cherokee_avl_node_t *bparent = path[--idx];
			is_left = (bparent && (pbalance == bparent->left));
			      
			if(pbalance->balance < -1 || pbalance->balance > 1) {
				pbalance = node_balance (pbalance); 
				if (!bparent)
					avl->root = pbalance;
				else if (is_left)
					bparent->left = pbalance;
				else
					bparent->right = pbalance;
			}
	
			if (pbalance->balance != 0 || !bparent)
				break;
	
			if (is_left)
				bparent->balance += 1;
			else
				bparent->balance -= 1;
			
			pbalance = bparent;
		}
	}
	
	node_free (node);
	return ret_ok;
}


ret_t 
cherokee_avl_get (cherokee_avl_t *avl, cherokee_buffer_t *key, void **value)
{
	short                re;
	cherokee_avl_node_t *node;

	if (unlikely (cherokee_buffer_is_empty(key)))
		return ret_error;
	
	node = avl->root;
	if (!node)
		return ret_not_found;

	while (true) {
		re = compare_buffers (key, &node->id, avl->case_insensitive);
		if (re == 0) {
			if (value)
				*value = node->value;
			return ret_ok;

		} else if (re < 0) {
			if (!node->left_child)
				return ret_not_found;
			node = node->left;

		} else {
			if (!node->right_child)
				return ret_not_found;
			node = node->right;
		}
	}

	SHOULDNT_HAPPEN;
	return ret_error;
}


/* Commodity methods
 */

ret_t 
cherokee_avl_get_ptr (cherokee_avl_t *avl, const char *key, void **value)
{
	cherokee_buffer_t tmp_key;

	cherokee_buffer_fake (&tmp_key, (const char *)key, strlen(key));
	return cherokee_avl_get (avl, &tmp_key, value);
}


ret_t 
cherokee_avl_add_ptr (cherokee_avl_t *avl, const char *key, void *value)
{
	cherokee_buffer_t tmp_key;

	cherokee_buffer_fake (&tmp_key, (const char *)key, strlen(key));
	return cherokee_avl_add (avl, &tmp_key, value);
}


ret_t 
cherokee_avl_del_ptr (cherokee_avl_t *avl, const char *key, void **value)
{
	cherokee_buffer_t tmp_key;

	cherokee_buffer_fake (&tmp_key, (const char *)key, strlen(key));
	return cherokee_avl_del (avl, &tmp_key, value);
}


ret_t 
cherokee_avl_while (cherokee_avl_t *avl, cherokee_avl_while_func_t func, void *param, cherokee_buffer_t **key, void **value)
{
	ret_t                ret;
	cherokee_avl_node_t *node = avl->root;

	if (avl->root == NULL) 
		return ret_ok;

	node = node_first (avl);
	while (node) {
		if (key) 
			*key = &node->id;
		if (value)
			*value = &node->value;

		ret = func (&node->id, node->value, param);
		if (ret != ret_ok) return ret;
      
		node = node_next (node);
	}

	return ret_ok;
}

static ret_t 
func_len_each (cherokee_buffer_t *key, void *value, void *param)
{
	UNUSED(key);
	UNUSED(value);

	*((size_t *)param) += 1;
	return ret_ok;
}

ret_t 
cherokee_avl_len (cherokee_avl_t *avl, size_t *len)
{
	*len = 0;
	return cherokee_avl_while (avl, func_len_each, len, NULL, NULL);
}


/* Debugging 
 */

static void
print_node (cherokee_avl_node_t *node) 
{
	if (! node) {
		printf ("<null />\n");
		return;
	}

	printf ("<node key=\"%s\" value=\"%p\">\n", node->id.buf, node->value);
	if (node->left_child)
		print_node (node->left);  
	if (node->left_child)
		print_node (node->right); 
	printf ("</node>\n");
}

ret_t 
cherokee_avl_print (cherokee_avl_t *avl)
{
	print_node (avl->root);
	return ret_ok;
}


static cint_t
node_height (cherokee_avl_node_t *node)
{
	cint_t left_height;
	cint_t right_height;

	if (node) {
		left_height = 0;
		right_height = 0;

		if (node->left_child)
			left_height = node_height (node->left);

		if (node->right_child)
			right_height = node_height (node->right);

		return MAX (left_height, right_height) + 1;
	}
	return 0;
}


static ret_t
node_check (cherokee_avl_node_t *node) 
{
	cint_t               left_height;
	cint_t               right_height;
	cint_t               balance;
	cherokee_avl_node_t *tmp;

	if (node) {
		if (node->left_child) {
			tmp = node_prev (node);
			if (tmp->right != node) {
				PRINT_ERROR_S ("previous");
				return ret_error;
			}
		}

		if (node->right_child) {
			tmp = node_next (node);
			if (tmp->left != node) {
				PRINT_ERROR_S ("next");
				return ret_error;
			}
		}

		left_height  = 0;
		right_height = 0;
      
		if (node->left_child)
			left_height  = node_height (node->left);
		if (node->right_child)
			right_height = node_height (node->right);
      
		balance = right_height - left_height;
		if (balance != node->balance) {
			PRINT_ERROR_S ("Balance");
			return ret_error;
		}

		if (node->left_child)
			node_check (node->left);
		if (node->right_child)
			node_check (node->right);
	}
	
	return ret_ok;
}

ret_t
cherokee_avl_check (cherokee_avl_t *avl)
{
	return node_check (avl->root);
}

