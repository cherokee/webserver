/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/* Cherokee
 *
 * Authors:
 *      Alvaro Lopez Ortega <alvaro@alobbs.com>
 *
 * Copyright (C) 2001-2007 Alvaro Lopez Ortega
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

struct cherokee_avl_node {	   
	   /* Tree */
	   short                     balance;
	   struct cherokee_avl_node *left;
	   struct cherokee_avl_node *right;

	   /* Info */
	   cherokee_buffer_t         id;
	   void                     *value;
};


/* Nodes
 */
static ret_t 
cherokee_avl_node_init (cherokee_avl_node_t *node)
{
	   node->balance = 0;
	   node->left    = NULL;
	   node->right   = NULL;
	   node->value   = NULL;

	   cherokee_buffer_init (&node->id);
	   return ret_ok;
}

static ret_t 
cherokee_avl_node_mrproper (cherokee_avl_node_t *node)
{
	   cherokee_buffer_mrproper (&node->id);
	   return ret_ok;
}


/* Tree
 */
ret_t 
cherokee_avl_init (cherokee_avl_t *avl)
{
	   avl->root = NULL;
	   return ret_ok;
}


ret_t 
cherokee_avl_mrproper (cherokee_avl_t *avl)
{
	   return ret_ok;
}


static int 
compare_buffers (cherokee_buffer_t *A,
			  cherokee_buffer_t *B)
{
	   if (A->len > B->len)
			 return A->len - B->len;
	   else if (B->len > A->len)
			 return - (B->len - A->len);
	   else
			 return strcmp (A->buf, B->buf);
}

ret_t
find_smallest_child (cherokee_avl_node_t *root, cherokee_avl_node_t **ret_parent_node, cherokee_avl_node_t **ret_node)
{
	   cherokee_avl_node_t *node   = root;
	   cherokee_avl_node_t *parent = NULL;

	   while (node->left != NULL) {
			 parent = node;
			 node = node->left;
	   }
	   
	   *ret_node        = node;
	   *ret_parent_node = parent;

	   return ret_ok;
}

ret_t
find_biggest_child (cherokee_avl_node_t *root, cherokee_avl_node_t **ret_parent_node, cherokee_avl_node_t **ret_node)
{
	   cherokee_avl_node_t *node   = root;
	   cherokee_avl_node_t *parent = NULL;

	   while (node->right != NULL) {
			 parent = node;
			 node = node->right;
	   }
	   
	   *ret_node        = node;
	   *ret_parent_node = parent;

	   return ret_ok;
}


ret_t 
find_parent_node (cherokee_avl_t *avl, cherokee_buffer_t *key, cherokee_avl_node_t **ret_parent_node, cherokee_avl_node_t **ret_node)
{
	   int                  re;
	   cherokee_avl_node_t *node        = avl->root;
	   cherokee_avl_node_t *parent_node = NULL;

	   /* It is empty
	    */
	   if (node == NULL)
			 return ret_not_found;
	   
	   /* If it's the top node
	    */
	   if (compare_buffers (&node->id, key) == 0) {
			 *ret_parent_node = NULL;
			 *ret_node = node;
			 return ret_ok;
	   }

	   while (node != NULL) {
			 re = compare_buffers (&node->id, key);
			 
			 if (re < 0) {
				    parent_node = node;
				    node = node->left;
			 } else if (re > 0) {
				    parent_node = node;
				    node = node->right;
			 } else {
				    *ret_node = node;
				    *ret_parent_node = parent_node;
				    return ret_ok;
			 }
	   }
	   return ret_not_found;
}


ret_t 
find_node (cherokee_avl_t *avl, cherokee_buffer_t *key, cherokee_avl_node_t **ret_node)
{
	   cherokee_avl_node_t *ignored_parent;
	   return find_parent_node (avl, key, &ignored_parent, ret_node);
}


ret_t 
cherokee_avl_get (cherokee_avl_t *avl, cherokee_buffer_t *key, void **value)
{
	   ret_t                ret;
	   cherokee_avl_node_t *ret_node = NULL;

	   ret = find_node (avl, key, &ret_node);
	   if (ret != ret_ok) 
			 return ret;
	   if (ret_node == NULL)
			 return ret_error;

	   *value = ret_node->value;
	   return ret_ok;
}


static void
rotate_right_once (cherokee_avl_node_t **root)
{
	   cherokee_avl_node_t *A = *root;
	   cherokee_avl_node_t *B = (*root)->left;

	   A->left = B->right;
	   B->right = A;
	   *root = B;
	   A->balance = 0;
	   B->balance = 0;
}

static void
rotate_left_once (cherokee_avl_node_t **root)
{
	   cherokee_avl_node_t *A = *root;
	   cherokee_avl_node_t *B = (*root)->right;

	   A->right = B->left;
	   B->left = A;
	   *root = B;
	   A->balance = 0;
	   B->balance = 0;
}

static void
rotate_right_twice (cherokee_avl_node_t **root)
{
	   cherokee_avl_node_t *A = *root;
	   cherokee_avl_node_t *B = (*root)->left;

	   *root = B->right;
	   B->right = (*root)->left;
	   A->left = (*root)->right;
	   (*root)->left = B;
	   (*root)->right = A;

	   switch ((*root)->balance) {
	   case -1:
			 A->balance = 1;
			 B->balance = 0;
			 break;
	   case  0:
			 A->balance = 0;
			 B->balance = 0;
			 break;
	   case  1:
			 A->balance = 0;
			 B->balance = -1;
			 break;
	   }
	   (*root)->balance = 0;
}

static void
rotate_left_twice (cherokee_avl_node_t **root)
{
	   cherokee_avl_node_t *A = *root;
	   cherokee_avl_node_t *B = (*root)->right;

	   *root = B->left;
	   B->left = (*root)->right;
	   A->right = (*root)->left;
	   (*root)->left = A;
	   (*root)->right = B;

	   switch ((*root)->balance) {
	   case -1:
			 A->balance = 0;
			 B->balance = 1;
			 break;
	   case  0:
			 A->balance = 0;
			 B->balance = 0;
			 break;
	   case  1:
			 A->balance = -1;
			 B->balance = 0;
			 break;
	   }
	   (*root)->balance = 0;
}


static cherokee_boolean_t
balance (cherokee_avl_node_t **root)
{
	   switch ((*root)->balance) {
	   case 0:  /* no height increase */
			 return false;
	   case  1:
	   case -1: /* height increase */
			 return true;

	   case -2:
			 if ((*root)->left->balance <= 0) 
				    rotate_right_once (root);
			 else
				    rotate_right_twice (root);
			 break;

	   case 2:
			 if ((*root)->right->balance >= 0)
				    rotate_left_once (root);
			 else 
				    rotate_left_twice (root);
			 break;

	   default:
			 SHOULDNT_HAPPEN;
	   }

	   return false;
}


/* Return: 
 *  TRUE  - if the tree height has increased
 *  FALSE - otherwise
 *     -1 - error
 */
static int
add_node (cherokee_avl_node_t **root, cherokee_avl_node_t *node)
{
	   int                re;
	   cherokee_boolean_t grew;

	   /* If the tree is empty..
	    */
	   if (*root == NULL) {
			 *root = node;
			 return true;
	   }

	   /* Comparet the key
	    */
	   re = compare_buffers (&(*root)->id, &node->id);
	   if (re < 0) {
			 /* Insert it on the left */
			 grew = add_node (&(*root)->left, node);
			 if (grew) {
				    (*root)->balance--;
				    return balance (root);
			 }
	   } else if (re > 0) {
			 /* Insert it on the right */
			 grew = add_node (&(*root)->right, node);
			 if (grew) {
				    (*root)->balance++;
				    return balance (root);
			 }
	   } else {
			 return -1;
	   }

	   return false;
}


ret_t 
cherokee_avl_add (cherokee_avl_t *avl, cherokee_buffer_t *key, void *value)
{
	   int   re;
	   ret_t ret;
	   CHEROKEE_NEW_STRUCT (n, avl_node);

	   /* Create the new AVL node
	    */
	   ret = cherokee_avl_node_init (n);
	   if (unlikely (ret != ret_ok)) return ret;

	   n->value = value;
	   cherokee_buffer_add_buffer (&n->id, key);

	   /* Add th node to the tree
	    */
	   re = add_node (&avl->root, n);
	   if (re == -1) {
			 cherokee_avl_node_mrproper (n);
			 free (n);
			 return ret_deny;
	   }

	   return ret_ok;
}


ret_t 
cherokee_avl_del (cherokee_avl_t *avl, cherokee_buffer_t *key, void **value)
{
	   ret_t                 ret;
	   cherokee_avl_node_t  *node   = NULL;
	   cherokee_avl_node_t  *parent = NULL;
	   cherokee_avl_node_t  *tmp    = NULL;
	   cherokee_avl_node_t  *tmp_pa = NULL;

	   /* Find the node and its parent
	    */
	   ret = find_parent_node (avl, key, &parent, &node);
	   if (ret != ret_ok) return ret;

	   /* We've got the node, read the returning value
	    */
	   if (value)
			 *value = node->value;

	   /* Leaf: simply elimitate it 
	    */
	   if (!node->left && !node->right) {
			 if (parent == NULL) {
				    avl->root = NULL;
				    return ret_ok;
			 }

			 if (parent->left == node) {
				    parent->left = NULL;
				    parent->balance++;

			 } else if (parent->right == node) {
				    parent->right = NULL;
				    parent->balance--;
			 }

			 return ret_ok;
	   }

	   /* If it has only one child
	    */
	   if (node->left && !node->right) {
			 tmp = node->left;

	   } else if (!node->left && node->right) {
			 tmp = node->right;
	   }

	   if (tmp) {

			 if (parent->left == node) {
				    parent->left = tmp;
			 } else if (parent->right == node) {
				    parent->right = tmp;
			 }

			 return ret_ok;
	   }

	   /* In the case it has more two child
	    */
	   if (node->balance < 0) {
			 ret = find_smallest_child (node, &tmp, &tmp_pa);
	   } else {
			 ret = find_biggest_child (node, &tmp, &tmp_pa);
	   }
	   if (ret != ret_ok) {
			 SHOULDNT_HAPPEN;
			 return ret;
	   }

	   /* Remove the node */
	   if (tmp_pa->left == tmp) {
			 tmp_pa->left = NULL;
			 parent->balance++;
	   } else {
			 tmp_pa->right = NULL;
			 parent->balance--;
	   }

	   /* Put it in place */
	   if (parent->left == node) {
			 parent->left = tmp;
	   } else {
			 parent->right = tmp;
	   }

	   return ret_ok;
}


/* Commodity methods
 */

ret_t 
while_node (cherokee_avl_node_t *node, cherokee_avl_while_func_t func, void *param, cherokee_buffer_t **key, void **value)
{
	   ret_t ret;

	   if (node == NULL) 
			 return ret_ok;

	   /* Returning values
	    */
	   if (key) 
			 *key = &node->id;
	   if (value)
			 *value = &node->value;

	   /* Walk the children
	    */
	   if (node->right)  {
			 ret = while_node (node->right, func, param, key, value);
			 if (ret != ret_ok) return ret;
	   }

	   ret = func (&node->id, node->value, param);
	   if (ret != ret_ok) return ret;

	   if (node->left)  {
			 ret = while_node (node->left, func, param, key, value);
			 if (ret != ret_ok) return ret;
	   }
	   
	   return ret_ok;
}

ret_t 
cherokee_avl_while (cherokee_avl_t *avl, cherokee_avl_while_func_t func, void *param, cherokee_buffer_t **key, void **value)
{
	   return while_node (avl->root, func, param, key, value);
}

ret_t 
cherokee_avl_len_each (cherokee_buffer_t *key, void *value, void *param)
{
	   *((size_t *)param) += 1;
	   return ret_ok;
}

ret_t 
cherokee_avl_len (cherokee_avl_t *avl, size_t *len)
{
	   *len = 0;
	   return cherokee_avl_while (avl, cherokee_avl_len_each, len, NULL, NULL);
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
	   print_node (node->left);  
	   print_node (node->right); 
	   printf ("</node>\n");
}


ret_t 
cherokee_avl_print (cherokee_avl_t *avl)
{
	   /* TIP: Pipe the output of this method through Tidy to
	    * reindent the tree. Eg: ./whatever | tidy -i -xml -q
	    */
	   print_node (avl->root);
	   return ret_ok;
}


static ret_t
check_node (cherokee_avl_node_t *node) 
{
	   int re;

	   /* Empty */
	   if (node == NULL)
			 return ret_ok;

	   /* No child */
	   if ((node->left == NULL) &&
		  (node->right == NULL))
			 return ret_ok;

	   /* Check order */
	   if (node->left) {
			 re = compare_buffers (&node->id, &node->left->id);
			 if (re >= 0) {
				    PRINT_ERROR ("Invalid link: node('%s') -> node('%s')\n",
							  node->id.buf, node->left->id.buf);
			 }
	   }

	   if (node->right) {
			 re = compare_buffers (&node->id, &node->left->id);
			 if (re <= 0) {
				    PRINT_ERROR ("Invalid link: node('%s') -> node('%s')\n",
							  node->id.buf, node->right->id.buf);
			 }
	   }

	   /* Check balance */
	   if (( node->left &&  node->right && (node->balance !=  0)) ||
		  (!node->left &&  node->right && (node->balance !=  1)) ||
		  ( node->left && !node->right && (node->balance != -1)))
	   {
			 PRINT_ERROR ("Wrong balance=%d at '%s'\n", node->balance, node->id.buf);
	   }

	   return ret_ok;
}

ret_t
cherokee_avl_check (cherokee_avl_t *avl)
{
	   return check_node (avl->root);
}


