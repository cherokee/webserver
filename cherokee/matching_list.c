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
#include "matching_list.h"



ret_t 
cherokee_matching_list_new (cherokee_matching_list_t **mlist)
{
	CHEROKEE_NEW_STRUCT (n, matching_list);

	INIT_LIST_HEAD(&n->list_allow);
	INIT_LIST_HEAD(&n->list_deny);
	   
	n->type = default_deny;

	*mlist = n;
	return ret_ok;
}


static ret_t 
matching_list_add_allow_cb  (char *val, void *data)
{
	return cherokee_matching_list_add_allow (MLIST(data), val);
}


static ret_t 
matching_list_add_deny_cb  (char *val, void *data)
{
	return cherokee_matching_list_add_deny (MLIST(data), val);
}


ret_t 
cherokee_matching_list_configure (cherokee_matching_list_t *mlist, cherokee_config_node_t *config)
{
	ret_t              ret;
	ret_t              ret2;
	cherokee_buffer_t *buf;

	/* Allow and Deny lists
	 */
	ret = cherokee_config_node_read_list (config, "allow", matching_list_add_allow_cb, mlist);
	if ((ret != ret_ok) && (ret != ret_not_found)) return ret;

	ret2 = cherokee_config_node_read_list (config, "deny", matching_list_add_deny_cb, mlist);
	if ((ret != ret_ok) && (ret != ret_not_found)) return ret;

	if ((ret == ret_ok) && (ret2 == ret_not_found))
		mlist->type = default_deny;
	if ((ret == ret_not_found) && (ret2 == ret_ok))
		mlist->type = default_allow;

	/* Type
	 */
	ret = cherokee_config_node_read (config, "type", &buf);
	if (ret == ret_ok) {
		if (! cherokee_buffer_cmp_str (buf, "default_allow")) {
			mlist->type = default_allow;
		} else if (! cherokee_buffer_cmp_str (buf, "default_deny")) {
			mlist->type = default_deny;
		} else if (! cherokee_buffer_cmp_str (buf, "deny_allow")) {
			mlist->type = deny_allow;
		} else if (! cherokee_buffer_cmp_str (buf, "allow_deny")) {
			mlist->type = allow_deny;
		} else {
			PRINT_MSG ("ERROR: Unknown matching list type '%s'\n", buf->buf);
			return ret_error;
		}
	}
	
	return ret_ok;
}


static void
free_list (cherokee_list_t *list)
{
	cherokee_list_t *i, *tmp;

	list_for_each_safe (i, tmp, list) {
		if (MLIST_ENTRY(i)->string) {
			free (MLIST_ENTRY(i)->string);
		}

		free(i);
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
add_to_list (cherokee_list_t *list, const char *item)
{
	CHEROKEE_NEW_STRUCT (n, matching_list_entry);

	INIT_LIST_HEAD(&n->list);
	n->string = strdup(item);

	cherokee_list_add (LIST(n), list);	
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
in_list  (cherokee_list_t *list, char *match)
{
	cherokee_list_t *i;

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


