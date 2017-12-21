/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/*
 * Libburner-burn
 * Copyright (C) Philippe Rouquier 2005-2009 <bonfire-app@wanadoo.fr>
 *
 * Libburner-burn is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * The Libburner-burn authors hereby grant permission for non-GPL compatible
 * GStreamer plugins to be used and distributed together with GStreamer
 * and Libburner-burn. This permission is above and beyond the permissions granted
 * by the GPL license by which Libburner-burn is covered. If you modify this code
 * you may extend this exception to your version of the code, but you are not
 * obligated to do so. If you do not wish to do so, delete this exception
 * statement from your version.
 * 
 * Libburner-burn is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Library General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to:
 * 	The Free Software Foundation, Inc.,
 * 	51 Franklin Street, Fifth Floor
 * 	Boston, MA  02110-1301, USA.
 */

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <string.h>

#include <gio/gio.h>

#include "burner-misc.h"

#include "burn-basics.h"

#include "burner-file-node.h"
#include "burner-io.h"


BurnerFileNode *
burner_file_node_root_new (void)
{
	BurnerFileNode *root;

	root = g_new0 (BurnerFileNode, 1);
	root->is_root = TRUE;
	root->is_imported = TRUE;

	root->union3.stats = g_new0 (BurnerFileTreeStats, 1);
	return root;
}

BurnerFileNode *
burner_file_node_get_root (BurnerFileNode *node,
			    guint *depth_retval)
{
	BurnerFileNode *parent;
	guint depth = 0;

	parent = node;
	while (parent) {
		if (parent->is_root) {
			if (depth_retval)
				*depth_retval = depth;

			return parent;
		}

		depth ++;
		parent = parent->parent;
	}

	return NULL;
}


guint
burner_file_node_get_depth (BurnerFileNode *node)
{
	guint depth = 0;

	while (node) {
		if (node->is_root)
			return depth;

		depth ++;
		node = node->parent;
	}

	return 0;
}

BurnerFileTreeStats *
burner_file_node_get_tree_stats (BurnerFileNode *node,
				  guint *depth)
{
	BurnerFileTreeStats *stats;
	BurnerFileNode *root;

	stats = BURNER_FILE_NODE_STATS (node);
	if (stats)
		return stats;

	root = burner_file_node_get_root (node, depth);
	stats = BURNER_FILE_NODE_STATS (root);

	return stats;
}

gint
burner_file_node_sort_default_cb (gconstpointer obj_a, gconstpointer obj_b)
{
	const BurnerFileNode *a = obj_a;
	const BurnerFileNode *b = obj_b;

	if (a->is_file == b->is_file)
		return 0;

	if (b->is_file)
		return -1;
	
	return 1;
}

gint
burner_file_node_sort_name_cb (gconstpointer obj_a, gconstpointer obj_b)
{
	gint res;
	const BurnerFileNode *a = obj_a;
	const BurnerFileNode *b = obj_b;


	res = burner_file_node_sort_default_cb (a, b);
	if (res)
		return res;

	return strcmp (BURNER_FILE_NODE_NAME (a), BURNER_FILE_NODE_NAME (b));
}

gint
burner_file_node_sort_size_cb (gconstpointer obj_a, gconstpointer obj_b)
{
	gint res;
	gint num_a, num_b;
	const BurnerFileNode *a = obj_a;
	const BurnerFileNode *b = obj_b;


	res = burner_file_node_sort_default_cb (a, b);
	if (res)
		return res;

	if (a->is_file)
		return BURNER_FILE_NODE_SECTORS (a) - BURNER_FILE_NODE_SECTORS (b);

	/* directories */
	num_a = burner_file_node_get_n_children (a);
	num_b = burner_file_node_get_n_children (b);
	return num_a - num_b;
}

gint 
burner_file_node_sort_mime_cb (gconstpointer obj_a, gconstpointer obj_b)
{
	gint res;
	const BurnerFileNode *a = obj_a;
	const BurnerFileNode *b = obj_b;


	res = burner_file_node_sort_default_cb (a, b);
	if (res)
		return res;

	return strcmp (BURNER_FILE_NODE_NAME (a), BURNER_FILE_NODE_NAME (b));
}

static BurnerFileNode *
burner_file_node_insert (BurnerFileNode *head,
			  BurnerFileNode *node,
			  GCompareFunc sort_func,
			  guint *newpos)
{
	BurnerFileNode *iter;
	guint n = 0;

	/* check for some special cases */
	if (!head) {
		node->next = NULL;
		return node;
	}

	/* Set hidden nodes (whether virtual or not) always last */
	if (head->is_hidden) {
		node->next = head;
		if (newpos)
			*newpos = 0;

		return node;
	}

	if (node->is_hidden) {
		iter = head;
		n = 1;
		while (iter->next)  {
			iter = iter->next;
			n ++;
		}

		iter->next = node;

		if (newpos)
			*newpos = n;

		return head;
	}

	/* regular node, regular head node */
	if (sort_func (head, node) > 0) {
		/* head is after node */
		node->next = head;
		if (newpos)
			*newpos = 0;

		return node;
	}

	n = 1;
	for (iter = head; iter->next; iter = iter->next) {
		if (sort_func (iter->next, node) > 0) {
			/* iter->next should be located after node */
			node->next = iter->next;
			iter->next = node;

			if (newpos)
				*newpos = n;

			return head;
		}
		n ++;
	}

	/* append it */
	iter->next = node;
	node->next = NULL;

	if (newpos)
		*newpos = n;

	return head;
}

gint *
burner_file_node_need_resort (BurnerFileNode *node,
			       GCompareFunc sort_func)
{
	BurnerFileNode *previous;
	BurnerFileNode *parent;
	BurnerFileNode *head;
	gint *array = NULL;
	guint newpos = 0;
	guint oldpos;
	guint size;

	if (node->is_hidden)
		return NULL;

	parent = node->parent;
	head = BURNER_FILE_NODE_CHILDREN (parent);

	/* find previous node and get old position */
	if (head != node) {
		previous = head;
		oldpos = 0;
		while (previous->next != node) {
			previous = previous->next;
			oldpos ++;
		}
		oldpos ++;
	}
	else {
		previous = NULL;
		oldpos = 0;
	}

	/* see where we should start from head or from node->next */
	if (previous && sort_func (previous, node) > 0) {
		gint i;

		/* move on the left */

		previous->next = node->next;

		head = burner_file_node_insert (head, node, sort_func, &newpos);
		parent->union2.children = head;

		/* create an array to reflect the changes */
		/* NOTE: hidden nodes are not taken into account. */
		size = burner_file_node_get_n_children (parent);
		array = g_new0 (gint, size);

		for (i = 0; i < size; i ++) {
			if (i == newpos)
				array [i] = oldpos;
			else if (i > newpos && i <= oldpos)
				array [i] = i - 1;
			else
				array [i] = i;
		}
	}
	/* Hidden nodes stay at the end, hence the !node->next->is_hidden */
	else if (node->next && !node->next->is_hidden && sort_func (node, node->next) > 0) {
		gint i;

		/* move on the right */

		if (previous)
			previous->next = node->next;
		else
			parent->union2.children = node->next;

		/* NOTE: here we're sure head hasn't changed since we checked 
		 * that node should go after node->next (given as head for the
		 * insertion here) */
		burner_file_node_insert (node->next, node, sort_func, &newpos);

		/* we started from oldpos so newpos needs updating */
		newpos += oldpos;

		/* create an array to reflect the changes. */
		/* NOTE: hidden nodes are not taken into account. */
		size = burner_file_node_get_n_children (parent);
		array = g_new0 (gint, size);

		for (i = 0; i < size; i ++) {
			if (i == newpos)
				array [i] = oldpos;
			else if (i >= oldpos && i < newpos)
				array [i] = i + 1;
			else
				array [i] = i;
		}
	}

	return array;
}

gint *
burner_file_node_sort_children (BurnerFileNode *parent,
				 GCompareFunc sort_func)
{
	BurnerFileNode *new_order = NULL;
	BurnerFileNode *iter;
	BurnerFileNode *next;
	gint *array = NULL;
	gint num_children;
	guint oldpos = 1;
	guint newpos;

	/* check for some special cases */
	if (parent->is_hidden)
		return NULL;

	new_order = BURNER_FILE_NODE_CHILDREN (parent);
	if (!new_order)
		return NULL;

	if (!new_order->next)
		return NULL;

	/* make the array */
	num_children = burner_file_node_get_n_children (parent);
	array = g_new (gint, num_children);

	next = new_order->next;
	new_order->next = NULL;
	array [0] = 0;

	for (iter = next; iter; iter = next, oldpos ++) {
		/* unlink iter */
		next = iter->next;
		iter->next = NULL;

		newpos = 0;
		new_order = burner_file_node_insert (new_order,
						      iter,
						      sort_func,
						      &newpos);

		if (newpos < oldpos)
			memmove (array + newpos + 1, array + newpos, (oldpos - newpos) * sizeof (guint));

		array [newpos] = oldpos;
	}

	/* set the new order */
	parent->union2.children = new_order;

	return array;
}

gint *
burner_file_node_reverse_children (BurnerFileNode *parent)
{
	BurnerFileNode *previous;
	BurnerFileNode *last;
	BurnerFileNode *iter;
	gint firstfile = 0;
	gint *array;
	gint size;
	gint i;
	
	/* when reversing the list of children the only thing we must pay 
	 * attention to is to keep directories first; so we do it in two passes
	 * first order the directories and then the files */
	last = BURNER_FILE_NODE_CHILDREN (parent);

	/* special case */
	if (!last || !last->next)
		return NULL;

	previous = last;
	iter = last->next;
	size = 1;

	if (!last->is_file) {
		while (!iter->is_file) {
			BurnerFileNode *next;

			next = iter->next;
			iter->next = previous;

			size ++;
			if (!next) {
				/* No file afterwards */
				parent->union2.children = iter;
				last->next = NULL;
				firstfile = size;
				goto end;
			}

			previous = iter;
			iter = next;
		}

		/* the new head is the last processed node */
		parent->union2.children = previous;
		firstfile = size;

		previous = iter;
		iter = iter->next;
		previous->next = NULL;
	}

	while (iter) {
		BurnerFileNode *next;

		next = iter->next;
		iter->next = previous;

		size ++;

		previous = iter;
		iter = next;
	}

	/* NOTE: iter is NULL here */
	if (last->is_file) {
		last->next = NULL;
		parent->union2.children = previous;
	}
	else
		last->next = previous;

end:

	array = g_new (gint, size);

	for (i = 0; i < firstfile; i ++)
		array [i] = firstfile - i - 1;

	for (i = firstfile; i < size; i ++)
		array [i] = size - i + firstfile - 1;

	return array;
}

BurnerFileNode *
burner_file_node_nth_child (BurnerFileNode *parent,
			     guint nth)
{
	BurnerFileNode *peers;
	guint pos;

	if (!parent)
		return NULL;

	peers = BURNER_FILE_NODE_CHILDREN (parent);
	for (pos = 0; pos < nth && peers; pos ++)
		peers = peers->next;

	return peers;
}

guint
burner_file_node_get_n_children (const BurnerFileNode *node)
{
	BurnerFileNode *children;
	guint num = 0;

	if (!node)
		return 0;

	for (children = BURNER_FILE_NODE_CHILDREN (node); children; children = children->next) {
		if (children->is_hidden)
			continue;
		num ++;
	}

	return num;
}

guint
burner_file_node_get_pos_as_child (BurnerFileNode *node)
{
	BurnerFileNode *parent;
	BurnerFileNode *peers;
	guint pos = 0;

	if (!node)
		return 0;

	parent = node->parent;
	for (peers = BURNER_FILE_NODE_CHILDREN (parent); peers; peers = peers->next) {
		if (peers == node)
			break;
		pos ++;
	}

	return pos;
}

gboolean
burner_file_node_is_ancestor (BurnerFileNode *parent,
			       BurnerFileNode *node)
{
	while (node && node != parent)
		node = node->parent;

	if (!node)
		return FALSE;

	return TRUE;
}

BurnerFileNode *
burner_file_node_check_name_existence_case (BurnerFileNode *parent,
					     const gchar *name)
{
	BurnerFileNode *iter;

	if (name && name [0] == '\0')
		return NULL;

	iter = BURNER_FILE_NODE_CHILDREN (parent);
	for (; iter; iter = iter->next) {
		if (!strcasecmp (name, BURNER_FILE_NODE_NAME (iter)))
			return iter;
	}

	return NULL;
}

BurnerFileNode *
burner_file_node_check_name_existence (BurnerFileNode *parent,
				        const gchar *name)
{
	BurnerFileNode *iter;

	if (name && name [0] == '\0')
		return NULL;

	iter = BURNER_FILE_NODE_CHILDREN (parent);
	for (; iter; iter = iter->next) {
		if (!strcmp (name, BURNER_FILE_NODE_NAME (iter)))
			return iter;
	}

	return NULL;
}

BurnerFileNode *
burner_file_node_get_from_path (BurnerFileNode *root,
				 const gchar *path)
{
	gchar **array;
	gchar **iter;

	if (!path)
		return NULL;

	/* If we don't do that array[0] == '\0' */
	if (g_str_has_prefix (path, G_DIR_SEPARATOR_S))
		array = g_strsplit (path + 1, G_DIR_SEPARATOR_S, 0);
	else
		array = g_strsplit (path, G_DIR_SEPARATOR_S, 0);

	if (!array)
		return NULL;

	for (iter = array; iter && *iter; iter++) {
		root = burner_file_node_check_name_existence (root, *iter);
		if (!root)
			break;
	}
	g_strfreev (array);

	return root;
}

BurnerFileNode *
burner_file_node_check_imported_sibling (BurnerFileNode *node)
{
	BurnerFileNode *parent;
	BurnerFileNode *iter;
	BurnerImport *import;

	parent = node->parent;

	/* That could happen if a node is moved to a location where another node
	 * (to be removed) has the same name and is a parent of this node */
	if (!parent)
		return NULL;

	/* See if among the imported children of the parent one of them
	 * has the same name as the node being removed. If so, restore
	 * it with all its imported children (provided that's a
	 * directory). */
	import = BURNER_FILE_NODE_IMPORT (parent);
	if (!import)
		return NULL;

	iter = import->replaced;
	if (!strcmp (BURNER_FILE_NODE_NAME (iter), BURNER_FILE_NODE_NAME (node))) {
		/* A match, remove it from the list and return it */
		import->replaced = iter->next;
		if (!import->replaced) {
			/* no more imported saved import structure */
			parent->union1.name = import->name;
			parent->has_import = FALSE;
			g_free (import);
		}

		iter->next = NULL;
		return iter;			
	}

	for (; iter->next; iter = iter->next) {
		if (!strcmp (BURNER_FILE_NODE_NAME (iter->next), BURNER_FILE_NODE_NAME (node))) {
			BurnerFileNode *removed;
			/* There is one match, remove it from the list */
			removed = iter->next;
			iter->next = removed->next;
			removed->next = NULL;
			return removed;
		}
	}

	return NULL;
}

void
burner_file_node_graft (BurnerFileNode *file_node,
			 BurnerURINode *uri_node)
{
	BurnerGraft *graft;

	if (!file_node->is_grafted) {
		BurnerFileNode *parent;

		graft = g_new (BurnerGraft, 1);
		graft->name = file_node->union1.name;
		file_node->union1.graft = graft;
		file_node->is_grafted = TRUE;

		/* since it wasn't grafted propagate the size change; that is
		 * substract the current node size from the parent nodes until
		 * the parent graft point. */
		for (parent = file_node->parent; parent && !parent->is_root; parent = parent->parent) {
			parent->union3.sectors -= BURNER_FILE_NODE_SECTORS (file_node);
			if (parent->is_grafted)
				break;
		}
	}
	else {
		BurnerURINode *old_uri_node;

		graft = BURNER_FILE_NODE_GRAFT (file_node);
		old_uri_node = graft->node;
		if (old_uri_node == uri_node)
			return;

		old_uri_node->nodes = g_slist_remove (old_uri_node->nodes, file_node);
	}

	graft->node = uri_node;
	uri_node->nodes = g_slist_prepend (uri_node->nodes, file_node);
}

void
burner_file_node_ungraft (BurnerFileNode *node)
{
	BurnerGraft *graft;
	BurnerFileNode *parent;

	if (!node->is_grafted)
		return;

	graft = node->union1.graft;

	/* Remove it from the URINode list of grafts */
	graft->node->nodes = g_slist_remove (graft->node->nodes, node);

	/* The name must be exactly the one of the URI */
	node->is_grafted = FALSE;
	node->union1.name = graft->name;

	/* Removes the graft */
	g_free (graft);

	/* Propagate the size change up the parents to the next
	 * grafted parent in the tree (if any). */
	for (parent = node->parent; parent && !parent->is_root; parent = parent->parent) {
		parent->union3.sectors += BURNER_FILE_NODE_SECTORS (node);
		if (parent->is_grafted)
			break;
	}
}

void
burner_file_node_rename (BurnerFileNode *node,
			  const gchar *name)
{
	g_free (BURNER_FILE_NODE_NAME (node));
	if (node->is_grafted)
		node->union1.graft->name = g_strdup (name);
	else
		node->union1.name = g_strdup (name);
}

void
burner_file_node_add (BurnerFileNode *parent,
		       BurnerFileNode *node,
		       GCompareFunc sort_func)
{
	BurnerFileTreeStats *stats;
	guint depth = 0;

	parent->union2.children = burner_file_node_insert (BURNER_FILE_NODE_CHILDREN (parent),
							    node,
							    sort_func,
							    NULL);
	node->parent = parent;

	if (BURNER_FILE_NODE_VIRTUAL (node))
		return;

	stats = burner_file_node_get_tree_stats (node->parent, &depth);
	if (!node->is_imported) {
		/* book keeping */
		if (!node->is_file)
			stats->num_dir ++;
		else
			stats->children ++;

		/* NOTE: parent will be changed afterwards !!! */
		if (!node->is_grafted) {
			/* propagate the size change*/
			for (; parent && !parent->is_root; parent = parent->parent) {
				parent->union3.sectors += BURNER_FILE_NODE_SECTORS (node);
				if (parent->is_grafted)
					break;
			}
		}
	}

	/* Even imported should be included. The only type of nodes that are not
	 * heeded are the virtual nodes. */
	if (node->is_file) {
		if (depth < 6)
			return;
	}
	else if (depth < 5)
		return;

	stats->num_deep ++;
	node->is_deep = TRUE;
}

void
burner_file_node_set_from_info (BurnerFileNode *node,
				 BurnerFileTreeStats *stats,
				 GFileInfo *info)
{
	/* NOTE: the name will never be replaced here since that means
	 * we could replace a previously set name (that triggered the
	 * creation of a graft). If someone wants to set a new name,
	 * then rename_node is the function. */

	if (node->parent) {
		/* update the stats since a file could have been added to the tree but
		 * at this point we didn't know what it was (a file or a directory).
		 * Only do this if it wasn't a file before.
		 * Do this only if it's in the tree. */
		if (!node->is_file && (g_file_info_get_file_type (info) != G_FILE_TYPE_DIRECTORY)) {
			stats->children ++;
			stats->num_dir --;
		}
		else if (node->is_file && (g_file_info_get_file_type (info) == G_FILE_TYPE_DIRECTORY)) {
			stats->children --;
			stats->num_dir ++;
		}
	}

	if (!node->is_symlink
	&& (g_file_info_get_file_type (info) == G_FILE_TYPE_SYMBOLIC_LINK)) {
		/* only count files */
		stats->num_sym ++;
	}

	/* update :
	 * - the mime type
	 * - the size (and possibly the one of his parent)
	 * - the type */
	node->is_file = (g_file_info_get_file_type (info) != G_FILE_TYPE_DIRECTORY);
	node->is_fake = FALSE;
	node->is_loading = FALSE;
	node->is_imported = FALSE;
	node->is_reloading = FALSE;
	node->is_symlink = (g_file_info_get_file_type (info) == G_FILE_TYPE_SYMBOLIC_LINK);

	if (node->is_file) {
		guint sectors;
		gint sectors_diff;

		/* register mime type string */
		if (g_file_info_has_attribute (info, G_FILE_ATTRIBUTE_STANDARD_CONTENT_TYPE)) {
			const gchar *mime;

			if (BURNER_FILE_NODE_MIME (node))
				burner_utils_unregister_string (BURNER_FILE_NODE_MIME (node));

			mime = g_file_info_get_content_type (info);
			node->union2.mime = burner_utils_register_string (mime);
		}

		sectors = BURNER_BYTES_TO_SECTORS (g_file_info_get_size (info), 2048);

		if (sectors > BURNER_FILE_2G_LIMIT && BURNER_FILE_NODE_SECTORS (node) <= BURNER_FILE_2G_LIMIT) {
			node->is_2GiB = 1;
			stats->num_2GiB ++;
		}
		else if (sectors <= BURNER_FILE_2G_LIMIT && BURNER_FILE_NODE_SECTORS (node) > BURNER_FILE_2G_LIMIT) {
			node->is_2GiB = 0;
			stats->num_2GiB --;
		}

		/* The node isn't grafted and it's a file. So we must propagate
		 * its size up to the parent graft node. */
		/* NOTE: we used to accumulate all the directory contents till
		 * the end and process all of entries at once, when it was
		 * finished. We had to do that to calculate the whole size. */
		sectors_diff = sectors - BURNER_FILE_NODE_SECTORS (node);
		for (; node; node = node->parent) {
			node->union3.sectors += sectors_diff;
			if (node->is_grafted)
				break;
		}
	}
	else	/* since that's directory then it must be explored now */
		node->is_exploring = TRUE;
}

BurnerFileNode *
burner_file_node_new_loading (const gchar *name)
{
	BurnerFileNode *node;

	node = g_new0 (BurnerFileNode, 1);
	node->union1.name = g_strdup (name);
	node->is_loading = TRUE;

	return node;
}

BurnerFileNode *
burner_file_node_new_virtual (const gchar *name)
{
	BurnerFileNode *node;

	/* virtual nodes are nodes that "don't exist". They appear as temporary
	 * parents (and therefore replacable) and hidden (not displayed in the
	 * GtkTreeModel). They are used as 'placeholders' to trigger
	 * name-collision signal. */
	node = g_new0 (BurnerFileNode, 1);
	node->union1.name = g_strdup (name);
	node->is_fake = TRUE;
	node->is_hidden = TRUE;

	return node;
}

BurnerFileNode *
burner_file_node_new (const gchar *name)
{
	BurnerFileNode *node;

	node = g_new0 (BurnerFileNode, 1);
	node->union1.name = g_strdup (name);

	return node;
}

BurnerFileNode *
burner_file_node_new_imported_session_file (GFileInfo *info)
{
	BurnerFileNode *node;

	/* Create the node information */
	node = g_new0 (BurnerFileNode, 1);
	node->union1.name = g_strdup (g_file_info_get_name (info));
	node->is_file = (g_file_info_get_file_type (info) != G_FILE_TYPE_DIRECTORY);
	node->is_imported = TRUE;

	if (!node->is_file) {
		node->is_fake = TRUE;
		node->union3.imported_address = g_file_info_get_attribute_int64 (info, BURNER_IO_DIR_CONTENTS_ADDR);
	}
	else
		node->union3.sectors = BURNER_BYTES_TO_SECTORS (g_file_info_get_size (info), 2048);

	return node;
}

BurnerFileNode *
burner_file_node_new_empty_folder (const gchar *name)
{
	BurnerFileNode *node;

	/* Create the node information */
	node = g_new0 (BurnerFileNode, 1);
	node->union1.name = g_strdup (name);
	node->is_fake = TRUE;

	return node;
}

void
burner_file_node_unlink (BurnerFileNode *node)
{
	BurnerFileNode *iter;
	BurnerImport *import;

	if (!node->parent)
		return;

	iter = BURNER_FILE_NODE_CHILDREN (node->parent);

	/* handle the size change for previous parent */
	if (!node->is_grafted
	&&  !node->is_imported
	&&  !BURNER_FILE_NODE_VIRTUAL (node)) {
		BurnerFileNode *parent;

		/* handle the size change if it wasn't grafted */
		for (parent = node->parent; parent && !parent->is_root; parent = parent->parent) {
			parent->union3.sectors -= BURNER_FILE_NODE_SECTORS (node);
			if (parent->is_grafted)
				break;
		}
	}

	node->is_deep = FALSE;

	if (iter == node) {
		node->parent->union2.children = node->next;
		node->parent = NULL;
		node->next = NULL;
		return;
	}

	for (; iter->next; iter = iter->next) {
		if (iter->next == node) {
			iter->next = node->next;
			node->parent = NULL;
			node->next = NULL;
			return;
		}
	}

	if (!node->is_imported || !node->parent->has_import)
		return;

	/* It wasn't found among the parent children. If parent is imported and
	 * the node is imported as well then check if it isn't among the import
	 * children */
	import = BURNER_FILE_NODE_IMPORT (node->parent);
	iter = import->replaced;

	if (iter == node) {
		import->replaced = iter->next;
		node->parent = NULL;
		node->next = NULL;
		return;
	}

	for (; iter->next; iter = iter->next) {
		if (iter->next == node) {
			iter->next = node->next;
			node->parent = NULL;
			node->next = NULL;
			return;
		}
	}
}

void
burner_file_node_move_from (BurnerFileNode *node,
			     BurnerFileTreeStats *stats)
{
	/* NOTE: for the time being no backend supports moving imported files */
	if (node->is_imported)
		return;

	if (node->is_deep)
		stats->num_deep --;

	burner_file_node_unlink (node);
}

void
burner_file_node_move_to (BurnerFileNode *node,
			   BurnerFileNode *parent,
			   GCompareFunc sort_func)
{
	BurnerFileTreeStats *stats;
	guint depth = 0;

	/* NOTE: for the time being no backend supports moving imported files */
	if (node->is_imported)
		return;

	/* reinsert it now at the new location */
	parent->union2.children = burner_file_node_insert (BURNER_FILE_NODE_CHILDREN (parent),
							    node,
							    sort_func,
							    NULL);
	node->parent = parent;

	if (!node->is_grafted) {
		BurnerFileNode *parent;

		/* propagate the size change for new parent */
		for (parent = node->parent; parent && !parent->is_root; parent = parent->parent) {
			parent->union3.sectors += BURNER_FILE_NODE_SECTORS (node);
			if (parent->is_grafted)
				break;
		}
	}

	/* NOTE: here stats about the tree can change if the parent has a depth
	 * > 6 and if previous didn't. Other stats remains unmodified. */
	stats = burner_file_node_get_tree_stats (node->parent, &depth);
	if (node->is_file) {
		if (depth < 6)
			return;
	}
	else if (depth < 5)
		return;

	stats->num_deep ++;
	node->is_deep = TRUE;
}

static void
burner_file_node_destroy_with_children (BurnerFileNode *node,
					 BurnerFileTreeStats *stats)
{
	BurnerFileNode *child;
	BurnerFileNode *next;
	BurnerImport *import;
	BurnerGraft *graft;

	/* destroy all children recursively */
	for (child = BURNER_FILE_NODE_CHILDREN (node); child; child = next) {
		next = child->next;
		burner_file_node_destroy_with_children (child, stats);
	}

	/* update all statistics on tree if any */
	if (!BURNER_FILE_NODE_VIRTUAL (node) && stats) {
		/* check if that's a 2 GiB file */
		if (node->is_2GiB)
			stats->num_2GiB --;

		/* check if that's a deep directory file */
		if (node->is_deep)
			stats->num_deep --;

		/* check if that's a symlink */
		if (node->is_symlink)
			stats->num_sym --;

		/* update file/directory number statistics */
		if (!node->is_imported) {
			if (node->is_file)
				stats->children --;
			else
				stats->num_dir --;
		}
	}

	/* destruction */
	import = BURNER_FILE_NODE_IMPORT (node);
	graft = BURNER_FILE_NODE_GRAFT (node);
	if (graft) {
		BurnerURINode *uri_node;

		uri_node = graft->node;

		/* Handle removal from BurnerURINode struct */
		if (uri_node)
			uri_node->nodes = g_slist_remove (uri_node->nodes, node);

		g_free (graft->name);
		g_free (graft);
	}
	else if (import) {
		/* if imported then destroy the saved children */
		for (child = import->replaced; child; child = next) {
			next = child->next;
			burner_file_node_destroy_with_children (child, stats);
		}

		g_free (import->name);
		g_free (import);
	}
	else
		g_free (BURNER_FILE_NODE_NAME (node));

	/* destroy the node */
	if (node->is_file && !node->is_imported && BURNER_FILE_NODE_MIME (node))
		burner_utils_unregister_string (BURNER_FILE_NODE_MIME (node));

	if (node->is_root)
		g_free (BURNER_FILE_NODE_STATS (node));

	g_free (node);
}

/**
 * Destroy a node and its children updating the tree stats.
 * If it isn't unlinked yet, it does it.
 */
void
burner_file_node_destroy (BurnerFileNode *node,
			   BurnerFileTreeStats *stats)
{
	/* remove from the parent children list or more probably from the 
	 * import list. */
	if (node->parent)
		burner_file_node_unlink (node);

	/* traverse the whole tree and free children updating tree stats */
	burner_file_node_destroy_with_children (node, stats);
}

/**
 * Pre-remove function that unparent a node (before a possible destruction).
 * If node is imported, it saves it in its parent, destroys all child nodes
 * that are not imported and restore children that were imported.
 * NOTE: tree stats are only updated if the node is imported.
 */

static void
burner_file_node_save_imported_children (BurnerFileNode *node,
					  BurnerFileTreeStats *stats,
					  GCompareFunc sort_func)
{
	BurnerFileNode *iter;
	BurnerImport *import;

	/* clean children */
	for (iter = BURNER_FILE_NODE_CHILDREN (node); iter; iter = iter->next) {
		if (!iter->is_imported)
			burner_file_node_destroy_with_children (iter, stats);

		if (!iter->is_file)
			burner_file_node_save_imported_children (iter, stats, sort_func);
	}

	/* restore all replaced children */
	import = BURNER_FILE_NODE_IMPORT (node);
	if (!import)
		return;

	for (iter = import->replaced; iter; iter = iter->next)
		burner_file_node_insert (iter, node, sort_func, NULL);

	/* remove import */
	node->union1.name = import->name;
	node->has_import = FALSE;
	g_free (import);
}

void
burner_file_node_save_imported (BurnerFileNode *node,
				 BurnerFileTreeStats *stats,
				 BurnerFileNode *parent,
				 GCompareFunc sort_func)
{
	BurnerImport *import;

	/* if it isn't imported return */
	if (!node->is_imported)
		return;

	/* Remove all the children that are not imported. Also restore
	 * all children that were replaced so as to restore the original
	 * order of files. */

	/* that shouldn't happen since root itself is considered imported */
	if (!parent || !parent->is_imported)
		return;

	/* save the node in its parent import structure */
	import = BURNER_FILE_NODE_IMPORT (parent);
	if (!import) {
		import = g_new0 (BurnerImport, 1);
		import->name = BURNER_FILE_NODE_NAME (parent);
		parent->union1.import = import;
		parent->has_import = TRUE;
	}

	/* unlink it and add it to the list */
	burner_file_node_unlink (node);
	node->next = import->replaced;
	import->replaced = node;
	node->parent = parent;

	/* Explore children and remove not imported ones and restore.
	 * Update the tree stats at the same time.
	 * NOTE: here the tree stats are only used for the grafted children that
	 * are not imported in the tree. */
	burner_file_node_save_imported_children (node, stats, sort_func);
}
