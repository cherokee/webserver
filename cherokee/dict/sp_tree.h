/*
 * sp_tree.h
 *
 * Definitions for splay binary search tree.
 * Copyright (C) 2001 Farooq Mela.
 *
 * $Id: sp_tree.h,v 1.2 2001/09/10 06:50:34 farooq Exp $
 */

#ifndef _SP_TREE_H_
#define _SP_TREE_H_

#include "dict_generic.h"

BEGIN_DECL

struct sp_tree;
typedef struct sp_tree sp_tree;

sp_tree	*sp_tree_new __P((dict_cmp_func key_cmp, dict_del_func key_del,
						  dict_del_func dat_del));
dict	*sp_dict_new __P((dict_cmp_func key_cmp, dict_del_func key_del,
						  dict_del_func dat_del));
void sp_tree_destroy __P((sp_tree *tree, int del));

int sp_tree_insert __P((sp_tree *tree, void *key, void *dat, int overwrite));
int sp_tree_probe __P((sp_tree *tree, void *key, void **dat));
void *sp_tree_search __P((sp_tree *tree, const void *key));
const void *sp_tree_csearch __P((const sp_tree *tree, const void *key));
int sp_tree_remove __P((sp_tree *tree, const void *key, int del));
void sp_tree_empty __P((sp_tree *tree, int del));
void sp_tree_walk __P((sp_tree *tree, dict_vis_func visit));
unsigned sp_tree_count __P((const sp_tree *tree));
unsigned sp_tree_height __P((const sp_tree *tree));
unsigned sp_tree_mheight __P((const sp_tree *tree));
unsigned sp_tree_pathlen __P((const sp_tree *tree));
const void *sp_tree_min __P((const sp_tree *tree));
const void *sp_tree_max __P((const sp_tree *tree));

struct sp_itor;
typedef struct sp_itor sp_itor;

sp_itor *sp_itor_new __P((sp_tree *tree));
dict_itor *sp_dict_itor_new __P((sp_tree *tree));
void sp_itor_destroy __P((sp_itor *tree));

int sp_itor_valid __P((const sp_itor *itor));
void sp_itor_invalidate __P((sp_itor *itor));
int sp_itor_next __P((sp_itor *itor));
int sp_itor_prev __P((sp_itor *itor));
int sp_itor_nextn __P((sp_itor *itor, unsigned count));
int sp_itor_prevn __P((sp_itor *itor, unsigned count));
int sp_itor_first __P((sp_itor *itor));
int sp_itor_last __P((sp_itor *itor));
int sp_itor_search __P((sp_itor *itor, const void *key));
const void *sp_itor_key __P((const sp_itor *itor));
void *sp_itor_data __P((sp_itor *itor));
const void *sp_itor_cdata __P((const sp_itor *itor));
int sp_itor_set_data __P((sp_itor *itor, void *dat, int del));
int sp_itor_remove __P((sp_itor *itor, int del));


END_DECL

#endif /* !_SP_TREE_H_ */
