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
#include "matching_list.h"



ret_t 
cherokee_matching_list_new (cherokee_matching_list_t **mlist)
{
	CHEROKEE_NEW_STRUCT (n, matching_list);

	INIT_LIST_HEAD(&n->list_allow);
	INIT_LIST_HEAD(&n->list_deny);
	   
	n->type = default_allow;

	*mlist = n;
	return ret_ok;
}


static void
free_list (list_t *list)
{
	list_t *i, *tmp;

	list_for_each_safe (i, tmp, list) {
		   if (MLIST_ENTRY(i)->string) {
				 free (MLIST_ENTRY(i)->string);
		   }

		   list_del(i);
	}
}


ret_t 
cherokee_matching_list_free (cherokee_matching_list_t *mlist)
{
	   free_list (&mlist->list_allow);
	   free_list (&mlist->list_deny);

	   free (mlist);

	   return ret_ok;
}


static ret_t
add_to_list (list_t *list, const char *item)
{
	CHEROKEE_NEW_STRUCT (n, matching_list_entry);

	n->string = strdup(item);
	list_add ((list_t *)n, list);
	
	return ret_ok;
}


ret_t 
cherokee_matching_list_add_allow (cherokee_matching_list_t *mlist, const char *item)
{
	   return add_to_list (&mlist->list_allow, item);
}


ret_t 
cherokee_matching_list_add_deny  (cherokee_matching_list_t *mlist, const char *item)
{
	   return add_to_list (&mlist->list_deny, item);
}


ret_t 
cherokee_matching_list_set_type (cherokee_matching_list_t *mlist, cherokee_matching_t type)
{
	   mlist->type = type;
	   return ret_ok;
}


static int
in_list  (list_t *list, char *match)
{
	list_t *i;

	list_for_each (i, list) {
		if (!strcmp (MLIST_ENTRY(i)->string, match)) {
			return 1;
		}
	}
	
	return 0;
}

static int
match_default_allow (cherokee_matching_list_t *mlist, char *match)
{
	   return ! in_list (&mlist->list_deny, match);
}

static int
match_default_deny (cherokee_matching_list_t *mlist, char *match)
{
	   return in_list (&mlist->list_allow, match);
}

static int
match_deny_allow (cherokee_matching_list_t *mlist, char *match)
{
	   int tmp;

	   tmp = ! in_list (&mlist->list_deny, match);
	   if (!tmp) {
			 tmp = in_list (&mlist->list_allow, match);
	   }

	   return tmp;
}

static int
match_allow_deny (cherokee_matching_list_t *mlist, char *match)
{
	   return (in_list (&mlist->list_allow, match) &&
			 (!in_list (&mlist->list_deny, match)));
}

int 
cherokee_matching_list_match (cherokee_matching_list_t *mlist, char *match)
{
	switch (mlist->type) {
	case default_allow:
		return match_default_allow (mlist, match);
		break;
		
	case default_deny:
		return match_default_deny (mlist, match);
		break;
		
	case deny_allow:
		return match_deny_allow (mlist, match);
		break;
		
	case allow_deny:
		return match_allow_deny (mlist, match);
		break;
	}
	
	SHOULDNT_HAPPEN;
	return 0;
}

