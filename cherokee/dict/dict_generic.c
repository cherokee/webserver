/*
 * dict.c
 *
 * Implementation of generic dictionary routines.
 * Copyright (C) 2001 Farooq Mela.
 *
 * $Id: dict.c,v 1.5 2001/11/14 05:18:41 farooq Exp $
 */

#include <stdlib.h>

#include "dict.h"
#include "dict_private.h"

dict_malloc_func _dict_malloc = malloc;
dict_free_func _dict_free = free;

dict_malloc_func
dict_set_malloc(func)
	dict_malloc_func func;
{
	dict_malloc_func old = _dict_malloc;
	_dict_malloc = func ? func : malloc;
	return old;
}

dict_free_func
dict_set_free(func)
	dict_free_func func;
{
	dict_free_func old = _dict_free;
	_dict_free = func ? func : free;
	return old;
}

int
_dict_key_cmp(p1, p2)
	const void	*p1;
	const void	*p2;
{
	/*
	 * We cannot simply subtract pointers because that might result in signed
	 * overflow.
	 */
	return p1 < p2 ? -1 : p1 > p2 ? +1 : 0;
}

void
dict_destroy(dct, del)
	dict	*dct;
	int		 del;
{
	ASSERT(dct != NULL);

	dct->_destroy(dct->_object, del);
	FREE(dct);
}

void
dict_itor_destroy(itor)
	dict_itor *itor;
{
	ASSERT(itor != NULL);

	itor->_destroy(itor->_itor);
	FREE(itor);
}
