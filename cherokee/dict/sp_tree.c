/*
 * sp_tree.c
 *
 * Implementation for splay binary search tree.
 * Copyright (C) 2001 Farooq Mela.
 *
 * $Id: sp_tree.c,v 1.7 2001/11/14 06:22:47 farooq Exp $
 *
 * cf. [Sleator and Tarjan, 1985], [Tarjan 1985], [Tarjan 1983]
 *
 * A single operation on a splay tree has a worst-case time complexity of O(N),
 * but a series of M operations have a time complexity of O(M lg N), and thus
 * the amortized time complexity of an operation is O(lg N). More specifically,
 * a series of M operations on a tree with N nodes will runs in O((N+M)lg(N+M))
 * time. Splay trees work by "splaying" a node up the tree using a series of
 * rotations until it is the root each time it is accessed. They are much
 * simpler to code than most balanced trees, because there is no strict
 * requirement about maintaining a balance scheme among nodes. When inserting
 * and searching, we simply splay the node in question up until it becomes the
 * root of the tree.
 *
 * This implementation is a bottom-up, move-to-root splay tree.
 */

#include <stdlib.h>

#include "sp_tree.h"
#include "dict_private.h"

typedef struct sp_node sp_node;
struct sp_node {
	void		*key;
	void		*dat;
	sp_node		*parent;
	sp_node		*llink;
	sp_node		*rlink;
};

struct sp_tree {
	sp_node			*root;
	unsigned		 count;
	dict_cmp_func	 key_cmp;
	dict_del_func	 key_del;
	dict_del_func	 dat_del;
};

struct sp_itor {
	sp_tree	*tree;
	sp_node	*node;
};

static void rot_left __P((sp_tree *tree, sp_node *node));
static void rot_right __P((sp_tree *tree, sp_node *node));

static sp_node *node_new __P((void *key, void *dat));
static sp_node *node_next __P((sp_node *node));
static sp_node *node_prev __P((sp_node *node));
static sp_node *node_max __P((sp_node *node));
static sp_node *node_min __P((sp_node *node));
static unsigned node_height __P((const sp_node *node));
static unsigned node_mheight __P((const sp_node *node));
static unsigned node_pathlen __P((const sp_node *node, unsigned level));

sp_tree *
sp_tree_new(key_cmp, key_del, dat_del)
	dict_cmp_func	key_cmp;
	dict_del_func	key_del;
	dict_del_func	dat_del;
{
	sp_tree *tree;

	tree = MALLOC(sizeof(*tree));
	if (tree == NULL)
		return NULL;

	tree->root = NULL;
	tree->count = 0;
	tree->key_cmp = key_cmp ? key_cmp : _dict_key_cmp;
	tree->key_del = key_del;
	tree->dat_del = dat_del;
	return tree;
}

dict *
sp_dict_new(key_cmp, key_del, dat_del)
	dict_cmp_func	key_cmp;
	dict_del_func	key_del;
	dict_del_func	dat_del;
{
	dict *dct;
	sp_tree *tree;

	dct = MALLOC(sizeof(*dct));
	if (dct == NULL)
		return NULL;

	if ((tree = sp_tree_new(key_cmp, key_del, dat_del)) == NULL) {
		FREE(dct);
		return NULL;
	}

	dct->_object = tree;
	dct->_inew = (inew_func)sp_dict_itor_new;
	dct->_destroy = (destroy_func)sp_tree_destroy;
	dct->_insert = (insert_func)sp_tree_insert;
	dct->_probe = (probe_func)sp_tree_probe;
	dct->_search = (search_func)sp_tree_search;
	dct->_csearch = (csearch_func)sp_tree_csearch;
	dct->_remove = (remove_func)sp_tree_remove;
	dct->_empty = (empty_func)sp_tree_empty;
	dct->_walk = (walk_func)sp_tree_walk;
	dct->_count = (count_func)sp_tree_count;

	return dct;
}

void
sp_tree_destroy(tree, del)
	sp_tree	*tree;
	int		 del;
{
	ASSERT(tree != NULL);

	if (tree->root)
		sp_tree_empty(tree, del);
	FREE(tree);
}

void
sp_tree_set_free (sp_tree       *tree,
			   dict_del_func  key_del,
			   dict_del_func  dat_del)
{
	tree->key_del = key_del;
	tree->dat_del = dat_del;
}

void
sp_tree_empty(tree, del)
     sp_tree		*tree;
	int			 del;
{
	sp_node *node, *parent;

	ASSERT(tree != NULL);

	node = tree->root;
	while (node) {
		parent = node->parent;
		if (node->llink || node->rlink) {
			node = node->llink ? node->llink : node->rlink;
			continue;
		}

		if (del) {
			if (tree->key_del)
				tree->key_del(node->key);
			if (tree->dat_del)
				tree->dat_del(node->dat);
		}
		FREE(node);

		if (parent) {
			if (parent->llink == node)
				parent->llink = NULL;
			else
				parent->rlink = NULL;
		}
		node = parent;
	}

	tree->root = NULL;
	tree->count = 0;
}

/*
 * XXX Each zig/zig and zig/zag operation can be optimized, but for now we just
 * use two rotations.
 */
/*
 * zig_zig_right(T, A):
 *
 *     C               A
 *    /        B        \
 *   B   ==>  / \  ==>   B
 *  /        A   C        \
 * A                       C
 *
 * zig_zig_left(T, C):
 *
 * A                        C
 *  \           B          /
 *   B    ==>  / \  ==>   B
 *    \       A   C      /
 *     C                A
 *
 * zig_zag_right(T, B)
 *
 * A        A
 *  \        \          B
 *   C  ==>   B   ==>  / \
 *  /          \      A   C
 * B            C
 *
 * zig_zag_left(T, B)
 *
 *   C          C
 *  /          /        B
 * A    ==>   B   ==>  / \
 *  \        /        A   C
 *   B      A
 */
#define SPLAY(t,n)															\
{																			\
	sp_node *p;																\
																			\
	p = (n)->parent;														\
	if (p == (t)->root) {													\
		if (p->llink == (n))				/* zig right */					\
			rot_right((t), p);												\
		else								/* zig left */					\
			rot_left((t), p);												\
	} else {																\
		if (p->llink == (n)) {												\
			if (p->parent->llink == p) {	/* zig zig right */				\
				rot_right((t), p->parent);									\
				rot_right((t), (n)->parent);								\
			} else {						/* zig zag right */				\
				rot_right((t), p);											\
				rot_left((t), (n)->parent);									\
			}																\
		} else {															\
			if (p->parent->rlink == p) {	/* zig zig left */				\
				rot_left((t), p->parent);									\
				rot_left((t), (n)->parent);									\
			} else {						/* zig zag left */				\
				rot_left((t), p);											\
				rot_right((t), (n)->parent);								\
			}																\
		}																	\
	}																		\
}

int
sp_tree_insert(tree, key, dat, overwrite)
	sp_tree	*tree;
	void	*key;
	void	*dat;
	int		 overwrite;
{
	int rv = 0; /* shut up GCC */
	sp_node *node, *parent = NULL;

	ASSERT(tree != NULL);

	node = tree->root;
	while (node) {
		rv = tree->key_cmp(key, node->key);
		if (rv == 0) {
			if (overwrite == 0)
				return 1;
			if (tree->key_del)
				tree->key_del(node->key);
			if (tree->dat_del)
				tree->dat_del(node->dat);
			node->key = key;
			node->dat = dat;
			return 0;
		}

		parent = node;
		node = rv < 0 ? node->llink : node->rlink;
	}

	node = node_new(key, dat);
	if (node == NULL)
		return -1;

	node->parent = parent;
	if (parent == NULL) {
		ASSERT(tree->count == 0);
		tree->root = node;
		tree->count = 1;
		return 0;
	}
	if (rv < 0)
		parent->llink = node;
	else
		parent->rlink = node;
	tree->count++;

	while (tree->root != node)
		SPLAY(tree, node);

	return 0;
}

int
sp_tree_probe(tree, key, dat)
	sp_tree	 *tree;
	void	 *key;
	void	**dat;
{
	int rv = 0;
	sp_node *node, *parent = NULL;

	ASSERT(tree != NULL);

	node = tree->root;
	while (node) {
		rv = tree->key_cmp(key, node->key);
		if (rv == 0) {
			while (tree->root != node)
				SPLAY(tree, node);
			*dat = node->dat;
			return 0;
		}

		parent = node;
		node = rv < 0 ? node->llink : node->rlink;
	}

	node = node_new(key, *dat);
	if (node == NULL)
		return -1;

	node->parent = parent;
	if (parent == NULL) {
		ASSERT(tree->count == 0);
		tree->root = node;
		tree->count = 1;
		return 1;
	}
	if (rv < 0)
		parent->llink = node;
	else
		parent->rlink = node;
	tree->count++;

	while (tree->root != node)
		SPLAY(tree, node);

	return 1;
}

void *
sp_tree_search(tree, key)
	sp_tree		*tree;
	const void	*key;
{
	int rv;
	sp_node *node, *parent = NULL;

	ASSERT(tree != NULL);

	node = tree->root;
	while (node) {
		rv = tree->key_cmp(key, node->key);
		if (rv == 0) {
			while (tree->root != node)
				SPLAY(tree, node);
			return node->dat;
		}
		parent = node;
		node = rv < 0 ? node->llink : node->rlink;
	}
	/*
	 * XXX This is questionable. Just because a node is the nearest match
	 * doesn't mean it should become the new root.
	 */
	if (parent)
		while (tree->root != parent)
			SPLAY(tree, parent);
	return NULL;
}

const void *
sp_tree_csearch(tree, key)
	const sp_tree	*tree;
	const void		*key;
{
	/*
	 * This cast is OK, because contents of tree remain same, it is only the
	 * relative "ordering" that changes with splaying.
	 */
	return sp_tree_search((sp_tree *)tree, key);
}

int
sp_tree_remove(tree, key, del)
	sp_tree		*tree;
	const void	*key;
	int			 del;
{
	int rv;
	sp_node *node, *temp, *out;
	void *tmp;

	ASSERT(tree != NULL);

	node = tree->root;
	while (node) {
		rv = tree->key_cmp(key, node->key);
		if (rv == 0)
			break;
		node = rv < 0 ? node->llink : node->rlink;
	}

	if (node == NULL)
		return -1;

	if (node->llink == NULL || node->rlink == NULL) {
		out = node;
	} else {
		/*
		 * This is sure to screw up iterators that were positioned at the node
		 * "out".
		 */
		for (out = node->rlink; out->llink; out = out->llink)
			/* void */;
		SWAP(node->key, out->key, tmp);
		SWAP(node->dat, out->dat, tmp);
	}

	temp = out->llink ? out->llink : out->rlink;
	if (temp)
		temp->parent = out->parent;
	if (out->parent) {
		if (out->parent->llink == out)
			out->parent->llink = temp;
		else
			out->parent->rlink = temp;
	} else {
		tree->root = temp;
	}
	if (del) {
		if (tree->key_del)
			tree->key_del(out->key);
		if (tree->dat_del)
			tree->dat_del(out->dat);
	}

	/*
	 * Splay an adjacent node to the root, if possible.
	 */
	temp =
		node->parent ? node->parent :
		node->rlink ? node->rlink :
		node->llink;
	if (temp)
		while (tree->root != temp)
			SPLAY(tree, temp);

	FREE(out);
	tree->count--;

	return 0;
}

void
sp_tree_walk(tree, visit)
	sp_tree			*tree;
	dict_vis_func	 visit;
{
	sp_node *node;

	ASSERT(tree != NULL);
	ASSERT(visit != NULL);

	if (tree->root == NULL)
		return;

	for (node = node_min(tree->root); node; node = node_next(node))
		if (visit(node->key, node->dat) == 0)
			break;
}


/* ALO: Like sp_tree_walk() but:
 *  - it is safer
 *  - returns last key/value
 *  - visit function admits a parameter
 */

int
sp_tree_walk2 (tree, visit, param, ret_key, ret_dat)
	sp_tree		 *tree;
	int            (*visit) (char *key, void *value, void *param);
	void            *param;
	char           **ret_key;
	void           **ret_dat;
{
	sp_node *node;
	sp_node *n;

	ASSERT(tree != NULL);
	ASSERT(visit != NULL);

	if (tree->root == NULL)
		return -1;

	for (node = node_min(tree->root), n = node_next(node); node; node = n, n = node_next(node)) {
		   if (visit(node->key, node->dat, param) == 0) {
				 if (ret_key) *ret_key = node->key;
				 if (ret_dat) *ret_dat = node->dat;
				 return 1;
		   }
	}

	return 0;
}



unsigned
sp_tree_count(tree)
	const sp_tree	*tree;
{
	ASSERT(tree != NULL);

	return tree->count;
}

unsigned
sp_tree_height(tree)
	const sp_tree	*tree;
{
	ASSERT(tree != NULL);

	return tree->root ? node_height(tree->root) : 0;
}

unsigned
sp_tree_mheight(tree)
	const sp_tree	*tree;
{
	ASSERT(tree != NULL);

	return tree->root ? node_mheight(tree->root) : 0;
}

unsigned
sp_tree_pathlen(tree)
	const sp_tree *tree;
{
	ASSERT(tree != NULL);

	return tree->root ? node_pathlen(tree->root, 1) : 0;
}

const void *
sp_tree_min(tree)
	const sp_tree *tree;
{
	const sp_node *node;

	ASSERT(tree != NULL);

	if ((node = tree->root) == NULL)
		return NULL;

	for (; node->llink; node = node->llink)
		/* void */;
	return node->key;
}

const void *
sp_tree_max(tree)
	const sp_tree *tree;
{
	const sp_node *node;

	ASSERT(tree != NULL);

	if ((node = tree->root) == NULL)
		return NULL;

	for (; node->rlink; node = node->rlink)
		/* void */;
	return node->key;
}

static void
rot_left(tree, node)
	sp_tree	*tree;
	sp_node	*node;
{
	sp_node *rlink, *parent;

	ASSERT(tree != NULL);
	ASSERT(node != NULL);
	ASSERT(node->rlink != NULL);

	rlink = node->rlink;
	node->rlink = rlink->llink;
	if (rlink->llink)
		rlink->llink->parent = node;
	parent = node->parent;
	rlink->parent = parent;
	if (parent) {
		if (parent->llink == node)
			parent->llink = rlink;
		else
			parent->rlink = rlink;
	} else {
		tree->root = rlink;
	}
	rlink->llink = node;
	node->parent = rlink;
}

static void
rot_right(tree, node)
	sp_tree	*tree;
	sp_node	*node;
{
	sp_node *llink, *parent;

	ASSERT(tree != NULL);
	ASSERT(node != NULL);
	ASSERT(node->llink != NULL);

	llink = node->llink;
	node->llink = llink->rlink;
	if (llink->rlink)
		llink->rlink->parent = node;
	parent = node->parent;
	llink->parent = parent;
	if (parent) {
		if (parent->llink == node)
			parent->llink = llink;
		else
			parent->rlink = llink;
	} else {
		tree->root = llink;
	}
	llink->rlink = node;
	node->parent = llink;
}

static sp_node *
node_new(key, dat)
	void	*key;
	void	*dat;
{
	sp_node *node;

	node = MALLOC(sizeof(*node));
	if (node == NULL)
		return NULL;

	node->key = key;
	node->dat = dat;
	node->parent = NULL;
	node->llink = NULL;
	node->rlink = NULL;

	return node;
}

static sp_node *
node_next(node)
	sp_node *node;
{
	sp_node *temp;

	ASSERT(node != NULL);

	if (node->rlink) {
		for (node = node->rlink; node->llink; node = node->llink)
			/* void */;
		return node;
	}
	temp = node->parent;
	while (temp && temp->rlink == node) {
		node = temp;
		temp = temp->parent;
	}
	return temp;
}

static sp_node *
node_prev(node)
	sp_node *node;
{
	sp_node *temp;

	ASSERT(node != NULL);

	if (node->llink) {
		for (node = node->llink; node->rlink; node = node->rlink)
			/* void */;
		return node;
	}
	temp = node->parent;
	while (temp && temp->llink == node) {
		node = temp;
		temp = temp->parent;
	}
	return temp;
}

static sp_node *
node_max(node)
	sp_node *node;
{
	ASSERT(node != NULL);

	while (node->rlink)
		node = node->rlink;
	return node;
}

static sp_node *
node_min(node)
	sp_node *node;
{
	ASSERT(node != NULL);

	while (node->llink)
		node = node->llink;
	return node;
}

static unsigned
node_height(node)
	const sp_node	*node;
{
	unsigned l, r;

	l = node->llink ? node_height(node->llink) + 1 : 0;
	r = node->rlink ? node_height(node->rlink) + 1 : 0;
	return MAX(l, r);
}

static unsigned
node_mheight(node)
	const sp_node	*node;
{
	unsigned l, r;

	l = node->llink ? node_mheight(node->llink) + 1 : 0;
	r = node->rlink ? node_mheight(node->rlink) + 1 : 0;
	return MIN(l, r);
}

static unsigned
node_pathlen(node, level)
	const sp_node *node;
	unsigned level;
{
	unsigned n = 0;

	ASSERT(node != NULL);

	if (node->llink)
		n += level + node_pathlen(node->llink, level + 1);
	if (node->rlink)
		n += level + node_pathlen(node->rlink, level + 1);
	return n;
}

sp_itor *
sp_itor_new(tree)
	sp_tree *tree;
{
	sp_itor *itor;

	ASSERT(tree != NULL);

	itor = MALLOC(sizeof(*itor));
	if (itor) {
		itor->tree = tree;
		sp_itor_first(itor);
	}
	return itor;
}

dict_itor *
sp_dict_itor_new(tree)
	sp_tree *tree;
{
	dict_itor *itor;

	ASSERT(tree != NULL);

	itor = MALLOC(sizeof(*itor));
	if (itor == NULL)
		return NULL;

	if ((itor->_itor = sp_itor_new(tree)) == NULL) {
		FREE(itor);
		return NULL;
	}

	itor->_destroy = (idestroy_func)sp_itor_destroy;
	itor->_valid = (valid_func)sp_itor_valid;
	itor->_invalid = (invalidate_func)sp_itor_invalidate;
	itor->_next = (next_func)sp_itor_next;
	itor->_prev = (prev_func)sp_itor_prev;
	itor->_nextn = (nextn_func)sp_itor_nextn;
	itor->_prevn = (prevn_func)sp_itor_prevn;
	itor->_first = (first_func)sp_itor_first;
	itor->_last = (last_func)sp_itor_last;
	itor->_search = (isearch_func)sp_itor_search;
	itor->_key = (key_func)sp_itor_key;
	itor->_data = (data_func)sp_itor_data;
	itor->_cdata = (cdata_func)sp_itor_cdata;
	itor->_setdata = (dataset_func)sp_itor_set_data;

	return itor;
}

void
sp_itor_destroy(itor)
	sp_itor *itor;
{
	ASSERT(itor != NULL);

	FREE(itor);
}

#define RETVALID(itor)		return itor->node != NULL

int
sp_itor_valid(itor)
	const sp_itor *itor;
{
	ASSERT(itor != NULL);

	RETVALID(itor);
}

void
sp_itor_invalidate(itor)
	sp_itor *itor;
{
	ASSERT(itor != NULL);

	itor->node = NULL;
}

int
sp_itor_next(itor)
	sp_itor *itor;
{
	ASSERT(itor != NULL);

	if (itor->node == NULL)
		sp_itor_first(itor);
	else
		itor->node = node_next(itor->node);
	RETVALID(itor);
}

int
sp_itor_prev(itor)
	sp_itor *itor;
{
	ASSERT(itor != NULL);

	if (itor->node == NULL)
		sp_itor_last(itor);
	else
		itor->node = node_prev(itor->node);
	RETVALID(itor);
}

int
sp_itor_nextn(itor, count)
	sp_itor *itor;
	unsigned count;
{
	ASSERT(itor != NULL);

	if (count) {
		if (itor->node == NULL) {
			sp_itor_first(itor);
			count--;
		}

		while (count-- && itor->node)
			itor->node = node_next(itor->node);
	}

	RETVALID(itor);
}

int
sp_itor_prevn(itor, count)
	sp_itor *itor;
	unsigned count;
{
	ASSERT(itor != NULL);

	if (count) {
		if (itor->node == NULL) {
			sp_itor_last(itor);
			count--;
		}

		while (count-- && itor->node)
			itor->node = node_prev(itor->node);
	}

	RETVALID(itor);
}

int
sp_itor_first(itor)
	sp_itor *itor;
{
	ASSERT(itor != NULL);

	if (itor->tree->root == NULL)
		itor->node = NULL;
	else
		itor->node = node_min(itor->tree->root);
	RETVALID(itor);
}

int
sp_itor_last(itor)
	sp_itor *itor;
{
	ASSERT(itor != NULL);

	if (itor->tree->root == NULL)
		itor->node = NULL;
	else
		itor->node = node_max(itor->tree->root);
	RETVALID(itor);
}

int
sp_itor_search(itor, key)
	sp_itor		*itor;
	const void	*key;
{
	int rv;
	sp_node *node;
	dict_cmp_func cmp;

	ASSERT(itor != NULL);

	cmp = itor->tree->key_cmp;
	for (node = itor->tree->root; node;) {
		rv = cmp(key, node->key);
		if (rv == 0)
			break;
		node = rv < 0 ? node->llink : node->rlink;
	}
	itor->node = node;
	RETVALID(itor);
}

const void *
sp_itor_key(itor)
	const sp_itor *itor;
{
	ASSERT(itor != NULL);

	return itor->node ? itor->node->key : NULL;
}

void *
sp_itor_data(itor)
	sp_itor *itor;
{
	ASSERT(itor != NULL);

	return itor->node ? itor->node->dat : NULL;
}

const void *
sp_itor_cdata(itor)
	const sp_itor *itor;
{
	ASSERT(itor != NULL);

	return itor->node ? itor->node->dat : NULL;
}

int
sp_itor_set_data(itor, dat, del)
	sp_itor	*itor;
	void	*dat;
	int		 del;
{
	ASSERT(itor != NULL);

	if (itor->node == NULL)
		return -1;

	if (del && itor->tree->dat_del)
		itor->tree->dat_del(itor->node->dat);
	itor->node->dat = dat;
	return 0;
}
