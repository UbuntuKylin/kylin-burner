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

#include <glib.h>

#include "libburner-marshal.h"
#include "burner-data-tree-model.h"
#include "burner-data-project.h"
#include "burner-data-vfs.h"
#include "burner-file-node.h"

typedef struct _BurnerDataTreeModelPrivate BurnerDataTreeModelPrivate;
struct _BurnerDataTreeModelPrivate
{
	guint stamp;
};

#define BURNER_DATA_TREE_MODEL_PRIVATE(o)  (G_TYPE_INSTANCE_GET_PRIVATE ((o), BURNER_TYPE_DATA_TREE_MODEL, BurnerDataTreeModelPrivate))

G_DEFINE_TYPE (BurnerDataTreeModel, burner_data_tree_model, BURNER_TYPE_DATA_VFS);

enum {
	ROW_ADDED,
	ROW_REMOVED,
	ROW_CHANGED,
	ROWS_REORDERED,
	LAST_SIGNAL
};

static guint burner_data_tree_model_signals [LAST_SIGNAL] = {0};

static gboolean
burner_data_tree_model_node_added (BurnerDataProject *project,
				    BurnerFileNode *node,
				    const gchar *uri)
{
	/* see if we really need to tell the treeview we changed */
	if (node->is_hidden)
		goto end;

	if (node->parent
	&& !node->parent->is_root
	&& !node->parent->is_visible)
		goto end;

	g_signal_emit (project,
		       burner_data_tree_model_signals [ROW_ADDED],
		       0,
		       node);

end:
	/* chain up this function */
	if (BURNER_DATA_PROJECT_CLASS (burner_data_tree_model_parent_class)->node_added)
		return BURNER_DATA_PROJECT_CLASS (burner_data_tree_model_parent_class)->node_added (project, node, uri);

	return TRUE;
}

static void
burner_data_tree_model_node_removed (BurnerDataProject *project,
				      BurnerFileNode *former_parent,
				      guint former_position,
				      BurnerFileNode *node)
{
	/* see if we really need to tell the treeview we changed */
	if (node->is_hidden)
		goto end;

	if (!node->is_visible
	&&   former_parent
	&&  !former_parent->is_root
	&&  !former_parent->is_visible)
		goto end;

	g_signal_emit (project,
		       burner_data_tree_model_signals [ROW_REMOVED],
		       0,
		       former_parent,
		       former_position,
		       node);

end:
	/* chain up this function */
	if (BURNER_DATA_PROJECT_CLASS (burner_data_tree_model_parent_class)->node_removed)
		BURNER_DATA_PROJECT_CLASS (burner_data_tree_model_parent_class)->node_removed (project,
												 former_parent,
												 former_position,
												 node);
}

static void
burner_data_tree_model_node_changed (BurnerDataProject *project,
				      BurnerFileNode *node)
{
	/* see if we really need to tell the treeview we changed */
	if (node->is_hidden)
		goto end;

	if (node->parent
	&& !node->parent->is_root
	&& !node->parent->is_visible)
		goto end;

	g_signal_emit (project,
		       burner_data_tree_model_signals [ROW_CHANGED],
		       0,
		       node);

end:
	/* chain up this function */
	if (BURNER_DATA_PROJECT_CLASS (burner_data_tree_model_parent_class)->node_changed)
		BURNER_DATA_PROJECT_CLASS (burner_data_tree_model_parent_class)->node_changed (project, node);
}

static void
burner_data_tree_model_node_reordered (BurnerDataProject *project,
					BurnerFileNode *parent,
					gint *new_order)
{
	/* see if we really need to tell the treeview we changed */
	if (!parent->is_root
	&&  !parent->is_visible)
		goto end;

	g_signal_emit (project,
		       burner_data_tree_model_signals [ROWS_REORDERED],
		       0,
		       parent,
		       new_order);

end:
	/* chain up this function */
	if (BURNER_DATA_PROJECT_CLASS (burner_data_tree_model_parent_class)->node_reordered)
		BURNER_DATA_PROJECT_CLASS (burner_data_tree_model_parent_class)->node_reordered (project, parent, new_order);
}

static void
burner_data_tree_model_init (BurnerDataTreeModel *object)
{ }

static void
burner_data_tree_model_finalize (GObject *object)
{
	G_OBJECT_CLASS (burner_data_tree_model_parent_class)->finalize (object);
}

static void
burner_data_tree_model_class_init (BurnerDataTreeModelClass *klass)
{
	GObjectClass* object_class = G_OBJECT_CLASS (klass);
	BurnerDataProjectClass *data_project_class = BURNER_DATA_PROJECT_CLASS (klass);

	g_type_class_add_private (klass, sizeof (BurnerDataTreeModelPrivate));

	object_class->finalize = burner_data_tree_model_finalize;

	data_project_class->node_added = burner_data_tree_model_node_added;
	data_project_class->node_removed = burner_data_tree_model_node_removed;
	data_project_class->node_changed = burner_data_tree_model_node_changed;
	data_project_class->node_reordered = burner_data_tree_model_node_reordered;

	burner_data_tree_model_signals [ROW_ADDED] = 
	    g_signal_new ("row_added",
			  G_TYPE_FROM_CLASS (klass),
			  G_SIGNAL_RUN_LAST|G_SIGNAL_NO_RECURSE,
			  0,
			  NULL, NULL,
			  g_cclosure_marshal_VOID__POINTER,
			  G_TYPE_NONE,
			  1,
			  G_TYPE_POINTER);
	burner_data_tree_model_signals [ROW_REMOVED] = 
	    g_signal_new ("row_removed",
			  G_TYPE_FROM_CLASS (klass),
			  G_SIGNAL_RUN_LAST|G_SIGNAL_NO_RECURSE,
			  0,
			  NULL, NULL,
			  burner_marshal_VOID__POINTER_UINT_POINTER,
			  G_TYPE_NONE,
			  3,
			  G_TYPE_POINTER,
			  G_TYPE_UINT,
			  G_TYPE_POINTER);
	burner_data_tree_model_signals [ROW_CHANGED] = 
	    g_signal_new ("row_changed",
			  G_TYPE_FROM_CLASS (klass),
			  G_SIGNAL_RUN_LAST|G_SIGNAL_NO_RECURSE,
			  0,
			  NULL, NULL,
			  g_cclosure_marshal_VOID__POINTER,
			  G_TYPE_NONE,
			  1,
			  G_TYPE_POINTER);
	burner_data_tree_model_signals [ROWS_REORDERED] = 
	    g_signal_new ("rows_reordered",
			  G_TYPE_FROM_CLASS (klass),
			  G_SIGNAL_RUN_LAST|G_SIGNAL_NO_RECURSE,
			  0,
			  NULL, NULL,
			  burner_marshal_VOID__POINTER_POINTER,
			  G_TYPE_NONE,
			  2,
			  G_TYPE_POINTER,
			  G_TYPE_POINTER);
}

BurnerDataTreeModel *
burner_data_tree_model_new (void)
{
	return g_object_new (BURNER_TYPE_DATA_TREE_MODEL, NULL);
}
