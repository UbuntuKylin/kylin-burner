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

#include <fcntl.h>

#include <glib.h>
#include <glib/gi18n-lib.h>
#include <glib/gstdio.h>

#include <gtk/gtk.h>

#include "burner-units.h"
#include "burner-volume.h"

#include "burner-track-data-cfg.h"

#include "libburner-marshal.h"

#include "burner-filtered-uri.h"

#include "burner-misc.h"
#include "burn-basics.h"
#include "burner-data-project.h"
#include "burner-data-tree-model.h"

typedef struct _BurnerTrackDataCfgPrivate BurnerTrackDataCfgPrivate;
struct _BurnerTrackDataCfgPrivate
{
	BurnerImageFS forced_fs;
	BurnerImageFS banned_fs;

	BurnerFileNode *autorun;
	BurnerFileNode *icon;
	GFile *image_file;

	BurnerDataTreeModel *tree;
	guint stamp;

	/* allows some sort of caching */
	GSList *grafts;
	GSList *excluded;

	guint loading;
	guint loading_remaining;
	GSList *load_errors;

	GtkIconTheme *theme;

	GSList *shown;

	gint sort_column;
	GtkSortType sort_type;

	guint joliet_rename:1;

	guint deep_directory:1;
	guint G2_files:1;
};

#define BURNER_TRACK_DATA_CFG_PRIVATE(o)  (G_TYPE_INSTANCE_GET_PRIVATE ((o), BURNER_TYPE_TRACK_DATA_CFG, BurnerTrackDataCfgPrivate))


static void
burner_track_data_cfg_drag_source_iface_init (gpointer g_iface, gpointer data);
static void
burner_track_data_cfg_drag_dest_iface_init (gpointer g_iface, gpointer data);
static void
burner_track_data_cfg_sortable_iface_init (gpointer g_iface, gpointer data);
static void
burner_track_data_cfg_iface_init (gpointer g_iface, gpointer data);

G_DEFINE_TYPE_WITH_CODE (BurnerTrackDataCfg,
			 burner_track_data_cfg,
			 BURNER_TYPE_TRACK_DATA,
			 G_IMPLEMENT_INTERFACE (GTK_TYPE_TREE_MODEL,
					        burner_track_data_cfg_iface_init)
			 G_IMPLEMENT_INTERFACE (GTK_TYPE_TREE_DRAG_DEST,
					        burner_track_data_cfg_drag_dest_iface_init)
			 G_IMPLEMENT_INTERFACE (GTK_TYPE_TREE_DRAG_SOURCE,
					        burner_track_data_cfg_drag_source_iface_init)
			 G_IMPLEMENT_INTERFACE (GTK_TYPE_TREE_SORTABLE,
						burner_track_data_cfg_sortable_iface_init));

typedef enum {
	BURNER_ROW_REGULAR		= 0,
	BURNER_ROW_BOGUS
} BurnerFileRowType;

enum {
	AVAILABLE,
	LOADED,
	IMAGE,
	UNREADABLE,
	RECURSIVE,
	UNKNOWN,
	G2_FILE,
	ICON_CHANGED,
	NAME_COLLISION,
	DEEP_DIRECTORY,
	SOURCE_LOADED, 
	SOURCE_LOADING,
	JOLIET_RENAME_SIGNAL,
	LAST_SIGNAL
};

static gulong burner_track_data_cfg_signals [LAST_SIGNAL] = { 0 };


/**
 * GtkTreeModel part
 */

static guint
burner_track_data_cfg_get_pos_as_child (BurnerFileNode *node)
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

		/* Don't increment when is_hidden */
		if (peers->is_hidden)
			continue;

		pos ++;
	}

	return pos;
}

static GtkTreePath *
burner_track_data_cfg_node_to_path (BurnerTrackDataCfg *self,
				     BurnerFileNode *node)
{
	GtkTreePath *path;

	path = gtk_tree_path_new ();
	for (; node->parent && !node->is_root; node = node->parent) {
		guint nth;

		nth = burner_track_data_cfg_get_pos_as_child (node);
		gtk_tree_path_prepend_index (path, nth);
	}

	return path;
}

static gboolean
burner_track_data_cfg_iter_parent (GtkTreeModel *model,
				    GtkTreeIter *iter,
				    GtkTreeIter *child)
{
	BurnerTrackDataCfgPrivate *priv;
	BurnerFileNode *node;

	priv = BURNER_TRACK_DATA_CFG_PRIVATE (model);

	/* make sure that iter comes from us */
	g_return_val_if_fail (priv->stamp == child->stamp, FALSE);
	g_return_val_if_fail (child->user_data != NULL, FALSE);

	node = child->user_data;
	if (child->user_data2 == GINT_TO_POINTER (BURNER_ROW_BOGUS)) {
		/* This is a bogus row intended for empty directories
		 * user_data has the parent empty directory. */
		iter->user_data2 = GINT_TO_POINTER (BURNER_ROW_REGULAR);
		iter->user_data = child->user_data;
		iter->stamp = priv->stamp;
		return TRUE;
	}

	if (!node->parent) {
		iter->user_data = NULL;
		return FALSE;
	}

	iter->stamp = priv->stamp;
	iter->user_data = node->parent;
	iter->user_data2 = GINT_TO_POINTER (BURNER_ROW_REGULAR);
	return TRUE;
}

static BurnerFileNode *
burner_track_data_cfg_nth_child (BurnerFileNode *parent,
				  guint nth)
{
	BurnerFileNode *peers;
	gint pos;

	if (!parent)
		return NULL;

	peers = BURNER_FILE_NODE_CHILDREN (parent);
	while (peers && peers->is_hidden)
		peers = peers->next;
		
	for (pos = 0; pos < nth && peers; pos ++) {
		peers = peers->next;

		/* Skip hidden */
		while (peers && peers->is_hidden)
			peers = peers->next;
	}

	return peers;
}

static gboolean
burner_track_data_cfg_iter_nth_child (GtkTreeModel *model,
				       GtkTreeIter *iter,
				       GtkTreeIter *parent,
				       gint n)
{
	BurnerTrackDataCfgPrivate *priv;
	BurnerFileNode *node;

	priv = BURNER_TRACK_DATA_CFG_PRIVATE (model);

	if (parent) {
		/* make sure that iter comes from us */
		g_return_val_if_fail (priv->stamp == parent->stamp, FALSE);
		g_return_val_if_fail (parent->user_data != NULL, FALSE);

		if (parent->user_data2 == GINT_TO_POINTER (BURNER_ROW_BOGUS)) {
			/* This is a bogus row intended for empty directories,
			 * it hasn't got children. */
			return FALSE;
		}

		node = parent->user_data;
	}
	else
		node = burner_data_project_get_root (BURNER_DATA_PROJECT (priv->tree));

	iter->user_data = burner_track_data_cfg_nth_child (node, n);
	if (!iter->user_data)
		return FALSE;

	iter->stamp = priv->stamp;
	iter->user_data2 = GINT_TO_POINTER (BURNER_ROW_REGULAR);
	return TRUE;
}

static guint
burner_track_data_cfg_get_n_children (const BurnerFileNode *node)
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

static gint
burner_track_data_cfg_iter_n_children (GtkTreeModel *model,
					 GtkTreeIter *iter)
{
	BurnerTrackDataCfgPrivate *priv;
	BurnerFileNode *node;

	priv = BURNER_TRACK_DATA_CFG_PRIVATE (model);

	if (iter == NULL) {
		/* special case */
		node = burner_data_project_get_root (BURNER_DATA_PROJECT (priv->tree));
		return burner_track_data_cfg_get_n_children (node);
	}

	/* make sure that iter comes from us */
	g_return_val_if_fail (priv->stamp == iter->stamp, 0);
	g_return_val_if_fail (iter->user_data != NULL, 0);

	if (iter->user_data2 == GINT_TO_POINTER (BURNER_ROW_BOGUS))
		return 0;

	node = iter->user_data;
	if (node->is_file)
		return 0;

	/* return at least one for the bogus row labelled "empty". */
	if (!burner_track_data_cfg_get_n_children (node))
		return 1;

	return burner_track_data_cfg_get_n_children (node);
}

static gboolean
burner_track_data_cfg_iter_has_child (GtkTreeModel *model,
					GtkTreeIter *iter)
{
	BurnerTrackDataCfgPrivate *priv;
	BurnerFileNode *node;

	priv = BURNER_TRACK_DATA_CFG_PRIVATE (model);

	/* make sure that iter comes from us */
	g_return_val_if_fail (priv->stamp == iter->stamp, FALSE);
	g_return_val_if_fail (iter->user_data != NULL, FALSE);

	if (iter->user_data2 == GINT_TO_POINTER (BURNER_ROW_BOGUS)) {
		/* This is a bogus row intended for empty directories
		 * it hasn't got children */
		return FALSE;
	}

	node = iter->user_data;

	/* This is a workaround for a warning in gailtreeview.c line 2946 where
	 * gail uses the GtkTreePath and not a copy which if the node inserted
	 * declares to have children and is not expanded leads to the path being
	 * upped and therefore wrong. */
	if (node->is_inserting)
		return FALSE;

	if (node->is_loading)
		return FALSE;

	if (node->is_file)
		return FALSE;

	/* always return TRUE here when it's a directory since even if
	 * it's empty we'll add a row written empty underneath it
	 * anyway. */
	return TRUE;
}

static gboolean
burner_track_data_cfg_iter_children (GtkTreeModel *model,
				       GtkTreeIter *iter,
				       GtkTreeIter *parent)
{
	BurnerTrackDataCfgPrivate *priv;
	BurnerFileNode *node;

	priv = BURNER_TRACK_DATA_CFG_PRIVATE (model);

	if (!parent) {
		BurnerFileNode *root;
		BurnerFileNode *node;

		/* This is for the top directory */
		root = burner_data_project_get_root (BURNER_DATA_PROJECT (priv->tree));
		if (!root)
			return FALSE;

		node = BURNER_FILE_NODE_CHILDREN (root);
		while (node && node->is_hidden)
			node = node->next;

		if (!node || node->is_hidden)
			return FALSE;

		iter->user_data = node;
		iter->stamp = priv->stamp;
		iter->user_data2 = GINT_TO_POINTER (BURNER_ROW_REGULAR);
		return TRUE;
	}

	/* make sure that iter comes from us */
	g_return_val_if_fail (priv->stamp == parent->stamp, FALSE);
	g_return_val_if_fail (parent->user_data != NULL, FALSE);

	if (parent->user_data2 == GINT_TO_POINTER (BURNER_ROW_BOGUS)) {
		iter->user_data = NULL;
		return FALSE;
	}

	node = parent->user_data;
	if (node->is_file) {
		iter->user_data = NULL;
		return FALSE;
	}

	iter->stamp = priv->stamp;
	if (!burner_track_data_cfg_get_n_children (node)) {
		/* This is a directory but it hasn't got any child; yet
		 * we show a row written empty for that. Set bogus in
		 * user_data and put parent in user_data. */
		iter->user_data = parent->user_data;
		iter->user_data2 = GINT_TO_POINTER (BURNER_ROW_BOGUS);
		return TRUE;
	}

	iter->user_data = BURNER_FILE_NODE_CHILDREN (node);
	iter->user_data2 = GINT_TO_POINTER (BURNER_ROW_REGULAR);
	return TRUE;
}

static gboolean
burner_track_data_cfg_iter_next (GtkTreeModel *model,
				   GtkTreeIter *iter)
{
	BurnerTrackDataCfgPrivate *priv;
	BurnerFileNode *node;

	priv = BURNER_TRACK_DATA_CFG_PRIVATE (model);

	/* make sure that iter comes from us */
	g_return_val_if_fail (priv->stamp == iter->stamp, FALSE);
	g_return_val_if_fail (iter->user_data != NULL, FALSE);

	if (iter->user_data2 == GINT_TO_POINTER (BURNER_ROW_BOGUS)) {
		/* This is a bogus row intended for empty directories
		 * user_data has the parent empty directory. It hasn't
		 * got any peer.*/
		iter->user_data = NULL;
		return FALSE;
	}

	node = iter->user_data;
	node = node->next;

	/* skip all hidden files */
	while (node && node->is_hidden)
		node = node->next;

	if (!node || node->is_hidden)
		return FALSE;

	iter->user_data = node;
	return TRUE;
}

static void
burner_track_data_cfg_node_shown (GtkTreeModel *model,
				   GtkTreeIter *iter)
{
	BurnerFileNode *node;
	BurnerTrackDataCfgPrivate *priv;

	priv = BURNER_TRACK_DATA_CFG_PRIVATE (model);
	node = iter->user_data;

	/* Check if that's a BOGUS row. In this case that means the parent was
	 * expanded. Therefore ask vfs to increase its priority if it's loading
	 * its contents. */
	if (iter->user_data2 == GINT_TO_POINTER (BURNER_ROW_BOGUS)) {
		/* NOTE: this has to be a directory */
		/* NOTE: there is no need to check for is_loading case here
		 * since before showing its BOGUS row the tree will have shown
		 * its parent itself and therefore that's the cases that follow
		 */
		if (node->is_exploring) {
			/* the directory is being explored increase priority */
			burner_data_vfs_require_directory_contents (BURNER_DATA_VFS (priv->tree), node);
		}

		/* Otherwise, that's simply a BOGUS row and its parent was
		 * loaded but it is empty. Nothing to do. */
		node->is_expanded = TRUE;
		return;
	}

	if (!node)
		return;

	node->is_visible ++;

	if (node->parent && !node->parent->is_root) {
		if (!node->parent->is_expanded && node->is_visible > 0) {
			GtkTreePath *treepath;

			node->parent->is_expanded = TRUE;
			treepath = gtk_tree_model_get_path (model, iter);
			gtk_tree_model_row_changed (model,
						    treepath,
						    iter);
			gtk_tree_path_free (treepath);
		}
	}

	if (node->is_imported) {
		if (node->is_fake && !node->is_file) {
			/* we don't load all nodes when importing a session do it now */
			burner_data_session_load_directory_contents (BURNER_DATA_SESSION (priv->tree), node, NULL);
		}

		return;
	}

	if (node->is_visible > 1)
		return;

	/* NOTE: no need to see if that's a directory being explored here. If it
	 * is being explored then it has a BOGUS row and that's the above case 
	 * that is reached. */
	if (node->is_loading) {
		/* in this case have vfs to increase priority for this node */
		burner_data_vfs_require_node_load (BURNER_DATA_VFS (priv->tree), node);
	}
	else if (!BURNER_FILE_NODE_MIME (node)) {
		/* that means that file wasn't completly loaded. To save
		 * some time we delayed the detection of the mime type
		 * since that takes a lot of time. */
		burner_data_vfs_load_mime (BURNER_DATA_VFS (priv->tree), node);
	}

	/* add the node to the visible list that is used to update the disc 
	 * share for the node (we don't want to update the whole tree).
	 * Moreover, we only want files since directories don't have space. */
	priv->shown = g_slist_prepend (priv->shown, node);
}

static void
burner_track_data_cfg_node_hidden (GtkTreeModel *model,
				    GtkTreeIter *iter)
{
	BurnerFileNode *node;
	BurnerTrackDataCfgPrivate *priv;

	/* if it's a BOGUS row stop here since they are not added to shown list.
	 * In the same way returns if it is a file. */
	node = iter->user_data;
	if (iter->user_data2 == GINT_TO_POINTER (BURNER_ROW_BOGUS)) {
		node->is_expanded = FALSE;
		return;
	}

	if (!node)
		return;

	node->is_visible --;
	if (node->parent && !node->parent->is_root) {
		if (node->parent->is_expanded && node->is_visible == 0) {
			GtkTreePath *treepath;
			GtkTreeIter parent_iter;

			node->parent->is_expanded = FALSE;
			treepath = burner_track_data_cfg_node_to_path (BURNER_TRACK_DATA_CFG (model), node->parent);
			gtk_tree_model_get_iter (model, &parent_iter, treepath);
			gtk_tree_model_row_changed (model,
						    treepath,
						    &parent_iter);
			gtk_tree_path_free (treepath);
		}
	}

	if (node->is_imported)
		return;

	priv = BURNER_TRACK_DATA_CFG_PRIVATE (model);

	/* update shown list */
	if (!node->is_visible)
		priv->shown = g_slist_remove (priv->shown, node);
}

static void
burner_track_data_cfg_get_value (GtkTreeModel *model,
				  GtkTreeIter *iter,
				  gint column,
				  GValue *value)
{
	BurnerTrackDataCfgPrivate *priv;
	BurnerFileNode *node;

	priv = BURNER_TRACK_DATA_CFG_PRIVATE (model);

	/* make sure that iter comes from us */
	g_return_if_fail (priv->stamp == iter->stamp);
	g_return_if_fail (iter->user_data != NULL);

	node = iter->user_data;

	if (iter->user_data2 == GINT_TO_POINTER (BURNER_ROW_BOGUS)) {
		switch (column) {
		case BURNER_DATA_TREE_MODEL_NAME:
			g_value_init (value, G_TYPE_STRING);
			if (node->is_exploring)
				g_value_set_string (value, _("(loading…)"));
			else
				g_value_set_string (value, _("Empty"));

			return;

		case BURNER_DATA_TREE_MODEL_MIME_DESC:
		case BURNER_DATA_TREE_MODEL_MIME_ICON:
		case BURNER_DATA_TREE_MODEL_SIZE:
			g_value_init (value, G_TYPE_STRING);
			g_value_set_string (value, NULL);
			return;

		case BURNER_DATA_TREE_MODEL_SHOW_PERCENT:
			g_value_init (value, G_TYPE_BOOLEAN);
			g_value_set_boolean (value, FALSE);
			return;

		case BURNER_DATA_TREE_MODEL_PERCENT:
			g_value_init (value, G_TYPE_INT);
			g_value_set_int (value, 0);
			return;

		case BURNER_DATA_TREE_MODEL_STYLE:
			g_value_init (value, PANGO_TYPE_STYLE);
			g_value_set_enum (value, PANGO_STYLE_ITALIC);
			return;

		case BURNER_DATA_TREE_MODEL_EDITABLE:
			g_value_init (value, G_TYPE_BOOLEAN);
			g_value_set_boolean (value, FALSE);
			return;
	
		case BURNER_DATA_TREE_MODEL_COLOR:
			g_value_init (value, G_TYPE_STRING);
			if (node->is_imported)
				g_value_set_string (value, "grey50");
			else
				g_value_set_string (value, NULL);
			return;

		case BURNER_DATA_TREE_MODEL_IS_FILE:
			g_value_init (value, G_TYPE_BOOLEAN);
			g_value_set_boolean (value, FALSE);
			return;

		case BURNER_DATA_TREE_MODEL_IS_LOADING:
			g_value_init (value, G_TYPE_BOOLEAN);
			g_value_set_boolean (value, FALSE);
			return;

		case BURNER_DATA_TREE_MODEL_IS_IMPORTED:
			g_value_init (value, G_TYPE_BOOLEAN);
			g_value_set_boolean (value, FALSE);
			return;

		case BURNER_DATA_TREE_MODEL_URI:
			g_value_init (value, G_TYPE_STRING);
			g_value_set_string (value, NULL);
			return;

		default:
			return;
		}

		return;
	}

	switch (column) {
	case BURNER_DATA_TREE_MODEL_EDITABLE:
		g_value_init (value, G_TYPE_BOOLEAN);
		g_value_set_boolean (value, (node->is_imported == FALSE)/* && node->is_selected*/);
		return;

	case BURNER_DATA_TREE_MODEL_NAME: {
		gchar *filename;

		g_value_init (value, G_TYPE_STRING);
		filename = g_filename_to_utf8 (BURNER_FILE_NODE_NAME (node),
					       -1,
					       NULL,
					       NULL,
					       NULL);
		if (!filename)
			filename = burner_utils_validate_utf8 (BURNER_FILE_NODE_NAME (node));

		if (filename)
			g_value_set_string (value, filename);
		else	/* Glib section on g_convert advise to use a string like
			 * "Invalid Filename". */
			g_value_set_string (value, BURNER_FILE_NODE_NAME (node));

		g_free (filename);
		return;
	}

	case BURNER_DATA_TREE_MODEL_MIME_DESC:
		g_value_init (value, G_TYPE_STRING);
		if (node->is_loading)
			g_value_set_string (value, _("(loading…)"));
		else if (!node->is_file) {
			gchar *description;

			description = g_content_type_get_description ("inode/directory");
			g_value_set_string (value, description);
			g_free (description);
		}
		else if (node->is_imported)
			g_value_set_string (value, _("Disc file"));
		else if (!BURNER_FILE_NODE_MIME (node))
			g_value_set_string (value, _("(loading…)"));
		else {
			gchar *description;

			description = g_content_type_get_description (BURNER_FILE_NODE_MIME (node));
			g_value_set_string (value, description);
			g_free (description);
		}

		return;

	case BURNER_DATA_TREE_MODEL_MIME_ICON:
		g_value_init (value, G_TYPE_STRING);
		if (node->is_loading)
			g_value_set_string (value, "image-loading");
		else if (!node->is_file) {
			/* Here we have two states collapsed and expanded */
			if (node->is_expanded)
				g_value_set_string (value, "folder-open");
			else if (node->is_imported)
				/* that's for all the imported folders */
				g_value_set_string (value, "folder-visiting");
			else
				g_value_set_string (value, "folder");
		}
		else if (node->is_imported) {
			g_value_set_string (value, "media-cdrom");
		}
		else if (BURNER_FILE_NODE_MIME (node)) {
			const gchar *icon_string = "text-x-preview";
			GIcon *icon;

			/* NOTE: implemented in glib 2.15.6 (not for windows though) */
			icon = g_content_type_get_icon (BURNER_FILE_NODE_MIME (node));
			if (G_IS_THEMED_ICON (icon)) {
				const gchar * const *names = NULL;

				names = g_themed_icon_get_names (G_THEMED_ICON (icon));
				if (names) {
					gint i;

					for (i = 0; names [i]; i++) {
						if (gtk_icon_theme_has_icon (priv->theme, names [i])) {
							icon_string = names [i];
							break;
						}
					}
				}
			}

			g_value_set_string (value, icon_string);
			g_object_unref (icon);
		}
		else
			g_value_set_string (value, "image-loading");

		return;

	case BURNER_DATA_TREE_MODEL_SIZE:
		g_value_init (value, G_TYPE_STRING);
		if (node->is_loading)
			g_value_set_string (value, _("(loading…)"));
		else if (!node->is_file) {
			guint nb_items;

			if (node->is_exploring) {
				g_value_set_string (value, _("(loading…)"));
				return;
			}

			nb_items = burner_track_data_cfg_get_n_children (node);
			if (!nb_items)
				g_value_set_string (value, _("Empty"));
			else {
				gchar *text;

				text = g_strdup_printf (ngettext ("%d item", "%d items", nb_items), nb_items);
				g_value_set_string (value, text);
				g_free (text);
			}
		}
		else {
			gchar *text;

			text = g_format_size (BURNER_FILE_NODE_SECTORS (node) * 2048);
			g_value_set_string (value, text);
			g_free (text);
		}

		return;

	case BURNER_DATA_TREE_MODEL_SHOW_PERCENT:
		g_value_init (value, G_TYPE_BOOLEAN);
		if (node->is_imported || node->is_loading)
			g_value_set_boolean (value, FALSE);
		else
			g_value_set_boolean (value, TRUE);

		return;

	case BURNER_DATA_TREE_MODEL_PERCENT:
		g_value_init (value, G_TYPE_INT);
		if (!node->is_imported && !burner_data_vfs_is_active (BURNER_DATA_VFS (priv->tree))) {
			gint64 sectors;
			goffset node_sectors;

			sectors = burner_data_project_get_sectors (BURNER_DATA_PROJECT (priv->tree));

			if (!node->is_file)
				node_sectors = burner_data_project_get_folder_sectors (BURNER_DATA_PROJECT (priv->tree), node);
			else
				node_sectors = BURNER_FILE_NODE_SECTORS (node);

			if (sectors)
				g_value_set_int (value, MAX (0, MIN (node_sectors * 100 / sectors, 100)));
			else
				g_value_set_int (value, 0);
		}
		else
			g_value_set_int (value, 0);

		return;

	case BURNER_DATA_TREE_MODEL_STYLE:
		g_value_init (value, PANGO_TYPE_STYLE);
		if (node->is_imported)
			g_value_set_enum (value, PANGO_STYLE_ITALIC);

		return;

	case BURNER_DATA_TREE_MODEL_COLOR:
		g_value_init (value, G_TYPE_STRING);
		if (node->is_imported)
			g_value_set_string (value, "grey50");

		return;

	case BURNER_DATA_TREE_MODEL_URI: {
		gchar *uri;

		g_value_init (value, G_TYPE_STRING);
		uri = burner_data_project_node_to_uri (BURNER_DATA_PROJECT (priv->tree), node);
		g_value_set_string (value, uri);
		g_free (uri);
		return;
	}

	case BURNER_DATA_TREE_MODEL_IS_FILE:
		g_value_init (value, G_TYPE_BOOLEAN);
		g_value_set_boolean (value, node->is_file);
		return;

	case BURNER_DATA_TREE_MODEL_IS_LOADING:
		g_value_init (value, G_TYPE_BOOLEAN);
		g_value_set_boolean (value, node->is_loading);
		return;

	case BURNER_DATA_TREE_MODEL_IS_IMPORTED:
		g_value_init (value, G_TYPE_BOOLEAN);
		g_value_set_boolean (value, node->is_imported);
		return;

	default:
		return;
	}

	return;
}

static GtkTreePath *
burner_track_data_cfg_get_path (GtkTreeModel *model,
				 GtkTreeIter *iter)
{
	BurnerTrackDataCfgPrivate *priv;
	BurnerFileNode *node;
	GtkTreePath *path;

	priv = BURNER_TRACK_DATA_CFG_PRIVATE (model);

	/* make sure that iter comes from us */
	g_return_val_if_fail (priv->stamp == iter->stamp, NULL);
	g_return_val_if_fail (iter->user_data != NULL, NULL);

	node = iter->user_data;

	/* NOTE: there is only one single node without a name: root */
	path = burner_track_data_cfg_node_to_path (BURNER_TRACK_DATA_CFG (model), node);

	/* Add index 0 for empty bogus row */
	if (iter->user_data2 == GINT_TO_POINTER (BURNER_ROW_BOGUS))
		gtk_tree_path_append_index (path, 0);

	return path;
}

static BurnerFileNode *
burner_track_data_cfg_path_to_node (BurnerTrackDataCfg *self,
				     GtkTreePath *path)
{
	BurnerTrackDataCfgPrivate *priv;
	BurnerFileNode *node;
	gint *indices;
	guint depth;
	guint i;

	priv = BURNER_TRACK_DATA_CFG_PRIVATE (self);

	indices = gtk_tree_path_get_indices (path);
	depth = gtk_tree_path_get_depth (path);

	node = burner_data_project_get_root (BURNER_DATA_PROJECT (priv->tree));
	for (i = 0; i < depth; i ++) {
		BurnerFileNode *parent;

		parent = node;
		node = burner_track_data_cfg_nth_child (parent, indices [i]);
		if (!node)
			return NULL;
	}
	return node;
}

static gboolean
burner_track_data_cfg_get_iter (GtkTreeModel *model,
				 GtkTreeIter *iter,
				 GtkTreePath *path)
{
	BurnerTrackDataCfgPrivate *priv;
	BurnerFileNode *root;
	BurnerFileNode *node;
	const gint *indices;
	guint depth;
	guint i;

	priv = BURNER_TRACK_DATA_CFG_PRIVATE (model);

	indices = gtk_tree_path_get_indices (path);
	depth = gtk_tree_path_get_depth (path);

	root = burner_data_project_get_root (BURNER_DATA_PROJECT (priv->tree));
	/* NOTE: if we're in reset, then root won't exist anymore */
	if (!root)
		return FALSE;
		
	node = burner_track_data_cfg_nth_child (root, indices [0]);
	if (!node)
		return FALSE;

	for (i = 1; i < depth; i ++) {
		BurnerFileNode *parent;

		parent = node;
		node = burner_track_data_cfg_nth_child (parent, indices [i]);
		if (!node) {
			/* There is one case where this can happen and
			 * is allowed: that's when the parent is an
			 * empty directory. Then index must be 0. */
			if (!parent->is_file
			&&  !burner_track_data_cfg_get_n_children (parent)
			&&   indices [i] == 0) {
				iter->stamp = priv->stamp;
				iter->user_data = parent;
				iter->user_data2 = GINT_TO_POINTER (BURNER_ROW_BOGUS);
				return TRUE;
			}

			iter->user_data = NULL;
			return FALSE;
		}
	}

	iter->user_data2 = GINT_TO_POINTER (BURNER_ROW_REGULAR);
	iter->stamp = priv->stamp;
	iter->user_data = node;

	return TRUE;
}

static GType
burner_track_data_cfg_get_column_type (GtkTreeModel *model,
					 gint index)
{
	switch (index) {
	case BURNER_DATA_TREE_MODEL_NAME:
		return G_TYPE_STRING;

	case BURNER_DATA_TREE_MODEL_URI:
		return G_TYPE_STRING;

	case BURNER_DATA_TREE_MODEL_MIME_DESC:
		return G_TYPE_STRING;

	case BURNER_DATA_TREE_MODEL_MIME_ICON:
		return G_TYPE_STRING;

	case BURNER_DATA_TREE_MODEL_SIZE:
		return G_TYPE_STRING;

	case BURNER_DATA_TREE_MODEL_SHOW_PERCENT:
		return G_TYPE_BOOLEAN;

	case BURNER_DATA_TREE_MODEL_PERCENT:
		return G_TYPE_INT;

	case BURNER_DATA_TREE_MODEL_STYLE:
		return PANGO_TYPE_STYLE;

	case BURNER_DATA_TREE_MODEL_COLOR:
		return G_TYPE_STRING;

	case BURNER_DATA_TREE_MODEL_EDITABLE:
		return G_TYPE_BOOLEAN;

	case BURNER_DATA_TREE_MODEL_IS_FILE:
		return G_TYPE_BOOLEAN;

	case BURNER_DATA_TREE_MODEL_IS_LOADING:
		return G_TYPE_BOOLEAN;

	case BURNER_DATA_TREE_MODEL_IS_IMPORTED:
		return G_TYPE_BOOLEAN;

	default:
		break;
	}

	return G_TYPE_INVALID;
}

static gint
burner_track_data_cfg_get_n_columns (GtkTreeModel *model)
{
	return BURNER_DATA_TREE_MODEL_COL_NUM;
}

static GtkTreeModelFlags
burner_track_data_cfg_get_flags (GtkTreeModel *model)
{
	return 0;
}

static gboolean
burner_track_data_cfg_row_draggable (GtkTreeDragSource *drag_source,
				      GtkTreePath *treepath)
{
	BurnerFileNode *node;

	node = burner_track_data_cfg_path_to_node (BURNER_TRACK_DATA_CFG (drag_source), treepath);
	if (node && !node->is_imported)
		return TRUE;

	return FALSE;
}

static gboolean
burner_track_data_cfg_drag_data_get (GtkTreeDragSource *drag_source,
				      GtkTreePath *treepath,
				      GtkSelectionData *selection_data)
{
	if (gtk_selection_data_get_target (selection_data) == gdk_atom_intern (BURNER_DND_TARGET_DATA_TRACK_REFERENCE_LIST, TRUE)) {
		GtkTreeRowReference *reference;

		reference = gtk_tree_row_reference_new (GTK_TREE_MODEL (drag_source), treepath);
		gtk_selection_data_set (selection_data,
					gdk_atom_intern_static_string (BURNER_DND_TARGET_DATA_TRACK_REFERENCE_LIST),
					8,
					(void *) g_list_prepend (NULL, reference),
					sizeof (GList));
	}
	else
		return FALSE;

	return TRUE;
}

static gboolean
burner_track_data_cfg_drag_data_delete (GtkTreeDragSource *drag_source,
					 GtkTreePath *path)
{
	/* NOTE: it's not the data in the selection_data here that should be
	 * deleted but rather the rows selected when there is a move. FALSE
	 * here means that we didn't delete anything. */
	/* return TRUE to stop other handlers */
	return TRUE;
}

static gboolean
burner_track_data_cfg_drag_data_received (GtkTreeDragDest *drag_dest,
					   GtkTreePath *dest_path,
					   GtkSelectionData *selection_data)
{
	GdkAtom target;
	BurnerFileNode *node;
	BurnerFileNode *parent;
	GtkTreePath *dest_parent;
	BurnerTrackDataCfgPrivate *priv;

	priv = BURNER_TRACK_DATA_CFG_PRIVATE (drag_dest);

	/* NOTE: dest_path is the path to insert before; so we may not have a 
	 * valid path if it's in an empty directory */

	dest_parent = gtk_tree_path_copy (dest_path);
	gtk_tree_path_up (dest_parent);
	parent = burner_track_data_cfg_path_to_node (BURNER_TRACK_DATA_CFG (drag_dest), dest_parent);
	if (!parent) {
		gtk_tree_path_up (dest_parent);
		parent = burner_track_data_cfg_path_to_node (BURNER_TRACK_DATA_CFG (drag_dest), dest_parent);
	}
	else if (parent->is_file)
		parent = parent->parent;

	gtk_tree_path_free (dest_parent);

	target = gtk_selection_data_get_target (selection_data);
	/* Received data: see where it comes from:
	 * - from us, then that's a simple move
	 * - from another widget then it's going to be URIS and we add
	 *   them to the DataProject */
	if (target == gdk_atom_intern (BURNER_DND_TARGET_DATA_TRACK_REFERENCE_LIST, TRUE)) {
		GList *iter;

		iter = (GList *) gtk_selection_data_get_data (selection_data);

		/* That's us: move the row and its children. */
		for (; iter; iter = iter->next) {
			GtkTreeRowReference *reference;
			GtkTreeModel *src_model;
			GtkTreePath *treepath;

			reference = iter->data;
			src_model = gtk_tree_row_reference_get_model (reference);
			if (src_model != GTK_TREE_MODEL (drag_dest))
				continue;

			treepath = gtk_tree_row_reference_get_path (reference);

			node = burner_track_data_cfg_path_to_node (BURNER_TRACK_DATA_CFG (drag_dest), treepath);
			gtk_tree_path_free (treepath);

			burner_data_project_move_node (BURNER_DATA_PROJECT (priv->tree), node, parent);
		}
	}
	else if (target == gdk_atom_intern ("text/uri-list", TRUE)) {
		gint i;
		gchar **uris;

		/* NOTE: there can be many URIs at the same time. One
		 * success is enough to return TRUE. */
		uris = gtk_selection_data_get_uris (selection_data);
		if (!uris) {
			const guchar *selection_data_raw;

			selection_data_raw = gtk_selection_data_get_data (selection_data);
			uris = g_uri_list_extract_uris ((gchar *) selection_data_raw);
		}

		if (!uris)
			return TRUE;

		for (i = 0; uris [i]; i ++) {
			/* Add the URIs to the project */
			burner_data_project_add_loading_node (BURNER_DATA_PROJECT (priv->tree),
			                                       uris [i],
			                                       parent);
		}
		g_strfreev (uris);
	}
	else
		return FALSE;

	return TRUE;
}

static gboolean
burner_track_data_cfg_row_drop_possible (GtkTreeDragDest *drag_dest,
					  GtkTreePath *dest_path,
					  GtkSelectionData *selection_data)
{
	GdkAtom target;

	target = gtk_selection_data_get_target (selection_data);
	/* See if we are dropping to ourselves */
	if (target == gdk_atom_intern_static_string (BURNER_DND_TARGET_DATA_TRACK_REFERENCE_LIST)) {
		GtkTreePath *dest_parent;
		BurnerFileNode *parent;
		GList *iter;

		iter = (GList *) gtk_selection_data_get_data (selection_data);

		/* make sure the parent is a directory.
		 * NOTE: in this case dest_path is the exact path where it
		 * should be inserted. */
		dest_parent = gtk_tree_path_copy (dest_path);
		gtk_tree_path_up (dest_parent);

		parent = burner_track_data_cfg_path_to_node (BURNER_TRACK_DATA_CFG (drag_dest), dest_parent);

		if (!parent) {
			/* See if that isn't a BOGUS row; if so, try with parent */
			gtk_tree_path_up (dest_parent);
			parent = burner_track_data_cfg_path_to_node (BURNER_TRACK_DATA_CFG (drag_dest), dest_parent);

			if (!parent) {
				gtk_tree_path_free (dest_parent);
				return FALSE;
			}
		}
		else if (parent->is_file) {
			/* if that's a file try with parent */
			gtk_tree_path_up (dest_parent);
			parent = parent->parent;
		}

		if (parent->is_loading) {
			gtk_tree_path_free (dest_parent);
			return FALSE;
		}

		for (; iter; iter = iter->next) {
			GtkTreePath *src_path;
			GtkTreeModel *src_model;
			GtkTreeRowReference *reference;

			reference = iter->data;
			src_model = gtk_tree_row_reference_get_model (reference);
			if (src_model != GTK_TREE_MODEL (drag_dest))
				continue;

			src_path = gtk_tree_row_reference_get_path (reference);

			/* see if we are not moving a parent to one of its children */
			if (gtk_tree_path_is_ancestor (src_path, dest_path)) {
				gtk_tree_path_free (src_path);
				continue;
			}

			if (gtk_tree_path_up (src_path)) {
				/* check that node was moved to another directory */
				if (!parent->parent) {
					if (gtk_tree_path_get_depth (src_path)) {
						gtk_tree_path_free (src_path);
						gtk_tree_path_free (dest_parent);
						return TRUE;
					}
				}
				else if (!gtk_tree_path_get_depth (src_path)
				     ||   gtk_tree_path_compare (src_path, dest_parent)) {
					gtk_tree_path_free (src_path);
					gtk_tree_path_free (dest_parent);
					return TRUE;
				}
			}

			gtk_tree_path_free (src_path);
		}

		gtk_tree_path_free (dest_parent);
		return FALSE;
	}
	else if (target == gdk_atom_intern_static_string ("text/uri-list"))
		return TRUE;

	return FALSE;
}

/**
 * Sorting part
 */

static gboolean
burner_track_data_cfg_get_sort_column_id (GtkTreeSortable *sortable,
					   gint *column,
					   GtkSortType *type)
{
	BurnerTrackDataCfgPrivate *priv;

	priv = BURNER_TRACK_DATA_CFG_PRIVATE (sortable);

	if (column)
		*column = priv->sort_column;

	if (type)
		*type = priv->sort_type;

	return TRUE;
}

static void
burner_track_data_cfg_set_sort_column_id (GtkTreeSortable *sortable,
					   gint column,
					   GtkSortType type)
{
	BurnerTrackDataCfgPrivate *priv;

	priv = BURNER_TRACK_DATA_CFG_PRIVATE (sortable);
	priv->sort_column = column;
	priv->sort_type = type;

	switch (column) {
	case BURNER_DATA_TREE_MODEL_NAME:
		burner_data_project_set_sort_function (BURNER_DATA_PROJECT (priv->tree),
							type,
							burner_file_node_sort_name_cb);
		break;
	case BURNER_DATA_TREE_MODEL_SIZE:
		burner_data_project_set_sort_function (BURNER_DATA_PROJECT (priv->tree),
							type,
							burner_file_node_sort_size_cb);
		break;
	case BURNER_DATA_TREE_MODEL_MIME_DESC:
		burner_data_project_set_sort_function (BURNER_DATA_PROJECT (priv->tree),
							type,
							burner_file_node_sort_mime_cb);
		break;
	default:
		burner_data_project_set_sort_function (BURNER_DATA_PROJECT (priv->tree),
							type,
							burner_file_node_sort_default_cb);
		break;
	}

	gtk_tree_sortable_sort_column_changed (sortable);
}

static gboolean
burner_track_data_cfg_has_default_sort_func (GtkTreeSortable *sortable)
{
	/* That's always true since we sort files and directories */
	return TRUE;
}


static BurnerFileNode *
burner_track_data_cfg_autorun_inf_parse (BurnerTrackDataCfg *track,
					  const gchar *uri)
{
	BurnerTrackDataCfgPrivate *priv;
	BurnerFileNode *root;
	BurnerFileNode *node;
	GKeyFile *key_file;
	gchar *icon_path;
	gchar *path;

	priv = BURNER_TRACK_DATA_CFG_PRIVATE (track);

	if (!uri)
		return FALSE;

	path = g_filename_from_uri (uri, NULL, NULL);
	key_file = g_key_file_new ();

	if (!g_key_file_load_from_file (key_file, path, G_KEY_FILE_KEEP_COMMENTS|G_KEY_FILE_KEEP_TRANSLATIONS, NULL)) {
		g_key_file_free (key_file);
		g_free (path);
		return NULL;
	}
	g_free (path);

	/* NOTE: icon_path is the ON DISC path of the icon */
	icon_path = g_key_file_get_value (key_file, "autorun", "icon", NULL);
	g_key_file_free (key_file);

	if (icon_path && icon_path [0] == '\0') {
		g_free (icon_path);
		return NULL;
	}

	/* Get the node (hope it already exists) */
	root = burner_data_project_get_root (BURNER_DATA_PROJECT (priv->tree));
	node = burner_file_node_get_from_path (root, icon_path);
	if (node) {
		g_free (icon_path);
		return node;
	}

	/* Add a virtual node to get warned when/if the icon is added to the tree */
	node = burner_data_project_watch_path (BURNER_DATA_PROJECT (priv->tree), icon_path);
	g_free (icon_path);

	return node;
}

static void
burner_track_data_cfg_clean_cache (BurnerTrackDataCfg *track)
{
	BurnerTrackDataCfgPrivate *priv;

	priv = BURNER_TRACK_DATA_CFG_PRIVATE (track);

	if (priv->grafts) {
		g_slist_foreach (priv->grafts, (GFunc) burner_graft_point_free, NULL);
		g_slist_free (priv->grafts);
		priv->grafts = NULL;
	}

	if (priv->excluded) {
		g_slist_foreach (priv->excluded, (GFunc) g_free, NULL);
		g_slist_free (priv->excluded);
		priv->excluded = NULL;
	}
}

static void
burner_track_data_cfg_node_added (BurnerDataProject *project,
				   BurnerFileNode *node,
				   BurnerTrackDataCfg *self)
{
	BurnerTrackDataCfgPrivate *priv;
	BurnerFileNode *parent;
	GtkTreePath *path;
	GtkTreeIter iter;

	priv = BURNER_TRACK_DATA_CFG_PRIVATE (self);

	if (priv->icon == node) {
		/* Our icon node has showed up, signal that */
		g_signal_emit (self,
			       burner_track_data_cfg_signals [ICON_CHANGED],
			       0);
	}

	/* Check if the parent is root */
	if (node->parent->is_root) {
		if (!strcasecmp (BURNER_FILE_NODE_NAME (node), "autorun.inf")) {
			gchar *uri;

			
			/* This has been added by the user or by a project so
			 * we do display it; also we signal the change in icon.
			 * NOTE: if we had our own autorun.inf it was wiped out
			 * in the callback for "name-collision". */
			uri = burner_data_project_node_to_uri (BURNER_DATA_PROJECT (priv->tree), node);
			if (!uri) {
				/* URI can be NULL sometimes if the autorun.inf is from
				 * the session of the imported medium */
				priv->icon = burner_track_data_cfg_autorun_inf_parse (self, uri);
				g_free (uri);
			}

			g_signal_emit (self,
				       burner_track_data_cfg_signals [ICON_CHANGED],
				       0);
		}
	}

	iter.stamp = priv->stamp;
	iter.user_data = node;
	iter.user_data2 = GINT_TO_POINTER (BURNER_ROW_REGULAR);

	path = burner_track_data_cfg_node_to_path (self, node);

	/* if the node is reloading (because of a file system change or because
	 * it was a node that was a tmp folder) then no need to signal an added
	 * signal but a changed one */
	if (node->is_reloading) {
		gtk_tree_model_row_changed (GTK_TREE_MODEL (self), path, &iter);
		gtk_tree_path_free (path);
		return;
	}

	/* Add the row itself */
	/* This is a workaround for a warning in gailtreeview.c line 2946 where
	 * gail uses the GtkTreePath and not a copy which if the node inserted
	 * declares to have children and is not expanded leads to the path being
	 * upped and therefore wrong. */
	node->is_inserting = 1;
	gtk_tree_model_row_inserted (GTK_TREE_MODEL (self),
				     path,
				     &iter);
	node->is_inserting = 0;
	gtk_tree_path_free (path);

	parent = node->parent;
	if (!parent->is_root) {
		/* Tell the tree that the parent changed (since the number of children
		 * changed as well). */
		iter.user_data = parent;
		path = burner_track_data_cfg_node_to_path (self, parent);

		gtk_tree_model_row_changed (GTK_TREE_MODEL (self), path, &iter);

		/* Check if the parent of this node is empty if so remove the BOGUS row.
		 * Do it afterwards to prevent the parent row to be collapsed if it was
		 * previously expanded. */
		if (parent && burner_track_data_cfg_get_n_children (parent) == 1) {
			gtk_tree_path_append_index (path, 1);
			gtk_tree_model_row_deleted (GTK_TREE_MODEL (self), path);
		}

		gtk_tree_path_free (path);
	}

	/* Now see if this is a directory which is empty and needs a BOGUS */
	if (!node->is_file && !node->is_loading) {
		/* emit child-toggled. Thanks to bogus rows we only need to emit
		 * this signal once since a directory will always have a child
		 * in the tree */
		path = burner_track_data_cfg_node_to_path (self, node);
		gtk_tree_model_row_has_child_toggled (GTK_TREE_MODEL (self), path, &iter);
		gtk_tree_path_free (path);
	}

	/* we also have to set the is_visible property as all nodes added to 
	 * root are always visible but ref_node is not necessarily called on
	 * these nodes. */
//	if (parent->is_root)
//		node->is_visible = TRUE;
}

static guint
burner_track_data_cfg_convert_former_position (BurnerFileNode *former_parent,
                                                guint former_position)
{
	BurnerFileNode *child;
	guint hidden_num = 0;
	guint current_pos;

	/* We need to convert former_position to the right position in the
	 * treeview as there could be some hidden node before this position. */
	child = BURNER_FILE_NODE_CHILDREN (former_parent);

	for (current_pos = 0; child && current_pos != former_position; current_pos ++) {
		if (child->is_hidden)
			hidden_num ++;

		child = child->next;
	}

	return former_position - hidden_num;
}

static void
burner_track_data_cfg_node_removed (BurnerDataProject *project,
				     BurnerFileNode *former_parent,
				     guint former_position,
				     BurnerFileNode *node,
				     BurnerTrackDataCfg *self)
{
	BurnerTrackDataCfgPrivate *priv;
	GSList *iter, *next;
	GtkTreePath *path;

	priv = BURNER_TRACK_DATA_CFG_PRIVATE (self);
	/* NOTE: there is no special case of autorun.inf here when we created
	 * it as a temprary file since it's hidden and BurnerDataTreeModel
	 * won't emit a signal for removed file in this case.
	 * On the other hand we check for a node at root of the CD called
	 * "autorun.inf" just in case an autorun added by the user would be
	 * removed. Do it also for the icon node. */
	if (former_parent->is_root) {
		if (!strcasecmp (BURNER_FILE_NODE_NAME (node), "autorun.inf")) {
			priv->icon = NULL;
			g_signal_emit (self,
				       burner_track_data_cfg_signals [ICON_CHANGED],
				       0);
		}
		else if (priv->icon == node
		     || (priv->icon && !priv->autorun && burner_file_node_is_ancestor (node, priv->icon))) {
			/* This icon had been added by the user. Do nothing but
			 * register that the icon is no more on the disc */
			priv->icon = NULL;
			g_signal_emit (self,
				       burner_track_data_cfg_signals [ICON_CHANGED],
				       0);
		}
	}
	
	/* remove it from the shown list and all its children as well */
	priv->shown = g_slist_remove (priv->shown, node);
	for (iter = priv->shown; iter; iter = next) {
		BurnerFileNode *tmp;

		tmp = iter->data;
		next = iter->next;
		if (burner_file_node_is_ancestor (node, tmp))
			priv->shown = g_slist_remove (priv->shown, tmp);
	}

	/* See if the parent of this node still has children. If not we need to
	 * add a bogus row. If it hasn't got children then it only remains our
	 * node in the list.
	 * NOTE: parent has to be a directory. */
	if (!former_parent->is_root && !burner_track_data_cfg_get_n_children (former_parent)) {
		GtkTreeIter iter;

		iter.stamp = priv->stamp;
		iter.user_data = former_parent;
		iter.user_data2 = GINT_TO_POINTER (BURNER_ROW_BOGUS);

		path = burner_track_data_cfg_node_to_path (self, former_parent);
		gtk_tree_path_append_index (path, 1);

		gtk_tree_model_row_inserted (GTK_TREE_MODEL (self), path, &iter);
		gtk_tree_path_free (path);
	}

	/* remove the node. Do it after adding a possible BOGUS row.
	 * NOTE since BOGUS row has been added move row. */
	path = burner_track_data_cfg_node_to_path (self, former_parent);
	gtk_tree_path_append_index (path, burner_track_data_cfg_convert_former_position (former_parent, former_position));

	gtk_tree_model_row_deleted (GTK_TREE_MODEL (self), path);
	gtk_tree_path_free (path);
}

static void
burner_track_data_cfg_node_changed (BurnerDataProject *project,
				     BurnerFileNode *node,
				     BurnerTrackDataCfg *self)
{
	BurnerTrackDataCfgPrivate *priv;
	GtkTreePath *path;
	GtkTreeIter iter;

	priv = BURNER_TRACK_DATA_CFG_PRIVATE (self);

	/* Get the iter for the node */
	iter.stamp = priv->stamp;
	iter.user_data = node;
	iter.user_data2 = GINT_TO_POINTER (BURNER_ROW_REGULAR);

	path = burner_track_data_cfg_node_to_path (self, node);
	gtk_tree_model_row_changed (GTK_TREE_MODEL (self),
				    path,
				    &iter);

	/* Now see if this is a directory which is empty and needs a BOGUS */
	if (!node->is_file) {
		/* NOTE: No need to check for the number of children ... */

		/* emit child-toggled. Thanks to bogus rows we only need to emit
		 * this signal once since a directory will always have a child
		 * in the tree */
		gtk_tree_model_row_has_child_toggled (GTK_TREE_MODEL (self),
						      path,
						      &iter);

		/* The problem is that without that, the folder contents on disc
		 * won't be added to the tree if the node it replaced was
		 * already visible. */
		if (node->is_imported
		&&  node->is_visible
		&&  node->is_fake)
			burner_data_session_load_directory_contents (BURNER_DATA_SESSION (project),
								      node,
								      NULL);

		/* add the row */
		if (!burner_track_data_cfg_get_n_children (node))  {
			iter.user_data2 = GINT_TO_POINTER (BURNER_ROW_BOGUS);
			gtk_tree_path_append_index (path, 0);

			gtk_tree_model_row_inserted (GTK_TREE_MODEL (self),
						     path,
						     &iter);
		}
	}
	gtk_tree_path_free (path);
}

static void
burner_track_data_cfg_node_reordered (BurnerDataProject *project,
				       BurnerFileNode *parent,
				       gint *new_order,
				       BurnerTrackDataCfg *self)
{
	GtkTreePath *treepath;
	BurnerTrackDataCfgPrivate *priv;

	priv = BURNER_TRACK_DATA_CFG_PRIVATE (self);

	treepath = burner_track_data_cfg_node_to_path (self, parent);
	if (parent != burner_data_project_get_root (project)) {
		GtkTreeIter iter;

		iter.stamp = priv->stamp;
		iter.user_data = parent;
		iter.user_data2 = GINT_TO_POINTER (BURNER_ROW_REGULAR);

		gtk_tree_model_rows_reordered (GTK_TREE_MODEL (self),
					       treepath,
					       &iter,
					       new_order);
	}
	else
		gtk_tree_model_rows_reordered (GTK_TREE_MODEL (self),
					       treepath,
					       NULL,
					       new_order);

	gtk_tree_path_free (treepath);
}

static void
burner_track_data_clean_autorun (BurnerTrackDataCfg *track)
{
	gchar *uri;
	gchar *path;
	BurnerTrackDataCfgPrivate *priv;

	priv = BURNER_TRACK_DATA_CFG_PRIVATE (track);

	if (priv->image_file) {
		g_object_unref (priv->image_file);
		priv->image_file = NULL;
	}

	if (priv->autorun) {
		/* ONLY remove icon if it's our own */
		if (priv->icon) {
			uri = burner_data_project_node_to_uri (BURNER_DATA_PROJECT (priv->tree), priv->icon);
			path = g_filename_from_uri (uri, NULL, NULL);
			g_free (uri);
			g_remove (path);
			g_free (path);
			priv->icon = NULL;
		}

		uri = burner_data_project_node_to_uri (BURNER_DATA_PROJECT (priv->tree), priv->autorun);
		path = g_filename_from_uri (uri, NULL, NULL);
		g_free (uri);
		g_remove (path);
		g_free (path);
		priv->autorun = NULL;
	}
	else
		priv->icon = NULL;
}

static void
burner_track_data_cfg_iface_init (gpointer g_iface, gpointer data)
{
	GtkTreeModelIface *iface = g_iface;
	static gboolean initialized = FALSE;

	if (initialized)
		return;

	initialized = TRUE;

	iface->ref_node = burner_track_data_cfg_node_shown;
	iface->unref_node = burner_track_data_cfg_node_hidden;

	iface->get_flags = burner_track_data_cfg_get_flags;
	iface->get_n_columns = burner_track_data_cfg_get_n_columns;
	iface->get_column_type = burner_track_data_cfg_get_column_type;
	iface->get_iter = burner_track_data_cfg_get_iter;
	iface->get_path = burner_track_data_cfg_get_path;
	iface->get_value = burner_track_data_cfg_get_value;
	iface->iter_next = burner_track_data_cfg_iter_next;
	iface->iter_children = burner_track_data_cfg_iter_children;
	iface->iter_has_child = burner_track_data_cfg_iter_has_child;
	iface->iter_n_children = burner_track_data_cfg_iter_n_children;
	iface->iter_nth_child = burner_track_data_cfg_iter_nth_child;
	iface->iter_parent = burner_track_data_cfg_iter_parent;
}

static void
burner_track_data_cfg_drag_source_iface_init (gpointer g_iface, gpointer data)
{
	GtkTreeDragSourceIface *iface = g_iface;
	static gboolean initialized = FALSE;

	if (initialized)
		return;

	initialized = TRUE;

	iface->row_draggable = burner_track_data_cfg_row_draggable;
	iface->drag_data_get = burner_track_data_cfg_drag_data_get;
	iface->drag_data_delete = burner_track_data_cfg_drag_data_delete;
}

static void
burner_track_data_cfg_drag_dest_iface_init (gpointer g_iface, gpointer data)
{
	GtkTreeDragDestIface *iface = g_iface;
	static gboolean initialized = FALSE;

	if (initialized)
		return;

	initialized = TRUE;

	iface->drag_data_received = burner_track_data_cfg_drag_data_received;
	iface->row_drop_possible = burner_track_data_cfg_row_drop_possible;
}

static void
burner_track_data_cfg_sortable_iface_init (gpointer g_iface, gpointer data)
{
	GtkTreeSortableIface *iface = g_iface;
	static gboolean initialized = FALSE;

	if (initialized)
		return;

	initialized = TRUE;

	iface->get_sort_column_id = burner_track_data_cfg_get_sort_column_id;
	iface->set_sort_column_id = burner_track_data_cfg_set_sort_column_id;
	iface->has_default_sort_func = burner_track_data_cfg_has_default_sort_func;
}

/**
 * Track part
 */

/**
 * burner_track_data_cfg_add:
 * @track: a #BurnerTrackDataCfg
 * @uri: a #gchar
 * @parent: a #GtkTreePath or NULL
 *
 * Add a new file (with @uri as URI) under a directory (@parent).
 * If @parent is NULL, the file is added to the root.
 * Also if @uri is the path of a directory, this directory will be explored
 * and all its children added to the tree.
 *
 * Return value: a #gboolean. TRUE if the operation was successful, FALSE otherwise
 **/

gboolean
burner_track_data_cfg_add (BurnerTrackDataCfg *track,
			    const gchar *uri,
			    GtkTreePath *parent)
{
	BurnerTrackDataCfgPrivate *priv;
	BurnerFileNode *parent_node;

	g_return_val_if_fail (BURNER_TRACK_DATA_CFG (track), FALSE);
	priv = BURNER_TRACK_DATA_CFG_PRIVATE (track);

	if (priv->loading)
		return FALSE;

	if (parent) {
		parent_node = burner_track_data_cfg_path_to_node (track, parent);
		if (parent_node && (parent_node->is_file || parent_node->is_loading))
			parent_node = parent_node->parent;
	}
	else
		parent_node = burner_data_project_get_root (BURNER_DATA_PROJECT (priv->tree));

	return (burner_data_project_add_loading_node (BURNER_DATA_PROJECT (BURNER_DATA_PROJECT (priv->tree)), uri, parent_node) != NULL);
}

/**
 * burner_track_data_cfg_add_empty_directory:
 * @track: a #BurnerTrackDataCfg
 * @name: a #gchar
 * @parent: a #GtkTreePath or NULL
 *
 * Add a new empty directory (with @name as name) under another directory (@parent).
 * If @parent is NULL, the file is added to the root.
 *
 * Return value: a #GtkTreePath which should be destroyed when not needed; NULL if the operation was not successful.
 **/

GtkTreePath *
burner_track_data_cfg_add_empty_directory (BurnerTrackDataCfg *track,
					    const gchar *name,
					    GtkTreePath *parent)
{
	BurnerFileNode *parent_node = NULL;
	BurnerTrackDataCfgPrivate *priv;
	gchar *default_name = NULL;
	BurnerFileNode *node;
	GtkTreePath *path;

	g_return_val_if_fail (BURNER_TRACK_DATA_CFG (track), FALSE);

	priv = BURNER_TRACK_DATA_CFG_PRIVATE (track);
	if (priv->loading)
		return NULL;

	if (parent) {
		parent_node = burner_track_data_cfg_path_to_node (track, parent);
		if (parent_node && (parent_node->is_file || parent_node->is_loading))
			parent_node = parent_node->parent;
	}

	if (!parent_node)
		parent_node = burner_data_project_get_root (BURNER_DATA_PROJECT (priv->tree));

	if (!name) {
		guint nb = 1;

		default_name = g_strdup_printf (_("New folder"));
		while (burner_file_node_check_name_existence (parent_node, default_name)) {
			g_free (default_name);
			default_name = g_strdup_printf (_("New folder %i"), nb);
			nb++;
		}

	}

	node = burner_data_project_add_empty_directory (BURNER_DATA_PROJECT (priv->tree),
							 name? name:default_name,
							 parent_node);
	if (default_name)
		g_free (default_name);

	if (!node)
		return NULL;

	path = burner_track_data_cfg_node_to_path (track, node);
	if (path)
		burner_track_changed (BURNER_TRACK (track));

	return path;
}

/**
 * burner_track_data_cfg_remove:
 * @track: a #BurnerTrackDataCfg
 * @treepath: a #GtkTreePath
 *
 * Removes a file or a directory (as well as its children) from the tree.
 * NOTE: some files cannot be removed like files from an imported session.
 *
 * Return value: a #gboolean. TRUE if the operation was successful, FALSE otherwise
 **/

gboolean
burner_track_data_cfg_remove (BurnerTrackDataCfg *track,
			       GtkTreePath *treepath)
{
	BurnerTrackDataCfgPrivate *priv;
	BurnerFileNode *node;

	g_return_val_if_fail (BURNER_TRACK_DATA_CFG (track), FALSE);
	priv = BURNER_TRACK_DATA_CFG_PRIVATE (track);
	if (priv->loading)
		return FALSE;

	node = burner_track_data_cfg_path_to_node (track, treepath);
	if (!node)
		return FALSE;

	burner_data_project_remove_node (BURNER_DATA_PROJECT (priv->tree), node);
	return TRUE;
}

/**
 * burner_track_data_cfg_rename:
 * @track: a #BurnerTrackDataCfg
 * @newname: a #gchar
 * @treepath: a #GtkTreePath
 *
 * Renames the file in the tree pointed by @treepath.
 *
 * Return value: a #gboolean. TRUE if the operation was successful, FALSE otherwise
 **/

gboolean
burner_track_data_cfg_rename (BurnerTrackDataCfg *track,
			       const gchar *newname,
			       GtkTreePath *treepath)
{
	BurnerTrackDataCfgPrivate *priv;
	BurnerFileNode *node;

	g_return_val_if_fail (BURNER_TRACK_DATA_CFG (track), FALSE);
	priv = BURNER_TRACK_DATA_CFG_PRIVATE (track);
	node = burner_track_data_cfg_path_to_node (track, treepath);
	return burner_data_project_rename_node (BURNER_DATA_PROJECT (priv->tree),
						 node,
						 newname);
}

/**
 * burner_track_data_cfg_reset:
 * @track: a #BurnerTrackDataCfg
 *
 * Completely empties @track and unloads any currently loaded session
 *
 * Return value: a #gboolean. TRUE if the operation was successful, FALSE otherwise
 **/

gboolean
burner_track_data_cfg_reset (BurnerTrackDataCfg *track)
{
	BurnerTrackDataCfgPrivate *priv;
	BurnerFileNode *root;
	GtkTreePath *treepath;
	guint num;
	guint i;

	g_return_val_if_fail (BURNER_TRACK_DATA_CFG (track), FALSE);
	priv = BURNER_TRACK_DATA_CFG_PRIVATE (track);
	if (priv->loading)
		return FALSE;

	/* Do it now */
	burner_track_data_clean_autorun (track);

	root = burner_data_project_get_root (BURNER_DATA_PROJECT (priv->tree));
	num = burner_track_data_cfg_get_n_children (root);

	burner_data_project_reset (BURNER_DATA_PROJECT (priv->tree));

	treepath = gtk_tree_path_new_first ();
	for (i = 0; i < num; i++)
		gtk_tree_model_row_deleted (GTK_TREE_MODEL (track), treepath);
	gtk_tree_path_free (treepath);

	g_slist_free (priv->shown);
	priv->shown = NULL;

	priv->G2_files = FALSE;
	priv->deep_directory = FALSE;
	priv->joliet_rename = FALSE;

	burner_track_data_cfg_clean_cache (track);

	burner_track_changed (BURNER_TRACK (track));
	return TRUE;
}

/**
 * burner_track_data_cfg_get_filtered_model:
 * @track: a #BurnerTrackDataCfg
 *
 * Gets a GtkTreeModel which contains all the files that were
 * automatically filtered while added directories were explored.
 *
 * Return value: a #GtkTreeModel. Unref when not needed.
 **/

GtkTreeModel *
burner_track_data_cfg_get_filtered_model (BurnerTrackDataCfg *track)
{
	BurnerTrackDataCfgPrivate *priv;
	GtkTreeModel *model;

	g_return_val_if_fail (BURNER_TRACK_DATA_CFG (track), NULL);
	priv = BURNER_TRACK_DATA_CFG_PRIVATE (track);
	model = GTK_TREE_MODEL (burner_data_vfs_get_filtered_model (BURNER_DATA_VFS (priv->tree)));
	g_object_ref (model);
	return model;
}

/**
 * burner_track_data_cfg_restore:
 * @track: a #BurnerTrackDataCfg
 * @treepath: a #GtkTreePath
 *
 * Removes a file from the filtered file list (see burner_track_data_cfg_get_filtered_model ())
 * and re-adds it wherever it should be in the tree.
 * @treepath is a #GtkTreePath associated with the #GtkTreeModel which holds the
 * filtered files not the main tree.
 *
 **/

void
burner_track_data_cfg_restore (BurnerTrackDataCfg *track,
				GtkTreePath *treepath)
{
	BurnerTrackDataCfgPrivate *priv;
	BurnerFilteredUri *filtered;
	gchar *uri;

	g_return_if_fail (BURNER_TRACK_DATA_CFG (track));
	priv = BURNER_TRACK_DATA_CFG_PRIVATE (track);

	filtered = burner_data_vfs_get_filtered_model (BURNER_DATA_VFS (priv->tree));
	uri = burner_filtered_uri_restore (filtered, treepath);

	burner_data_project_restore_uri (BURNER_DATA_PROJECT (priv->tree), uri);
	g_free (uri);
}

/**
 * burner_track_data_cfg_dont_filter_uri:
 * @track: a #BurnerTrackDataCfg
 * @uri: a #gchar
 *
 * Prevents @uri to be filtered while automatic exploration
 * of added directories is performed.
 *
 **/

void
burner_track_data_cfg_dont_filter_uri (BurnerTrackDataCfg *track,
					const gchar *uri)
{
	BurnerTrackDataCfgPrivate *priv;
	BurnerFilteredUri *filtered;

	g_return_if_fail (BURNER_TRACK_DATA_CFG (track));
	priv = BURNER_TRACK_DATA_CFG_PRIVATE (track);

	filtered = burner_data_vfs_get_filtered_model (BURNER_DATA_VFS (priv->tree));
	burner_filtered_uri_dont_filter (filtered, uri);
	burner_data_project_restore_uri (BURNER_DATA_PROJECT (priv->tree), uri);
}

/**
 * burner_track_data_cfg_get_restored_list:
 * @track: a #BurnerTrackDataCfg
 *
 * Gets a list of URIs (as #gchar *) that were restored with burner_track_data_cfg_restore ().
 *
 * Return value: a #GSList; free the list and its contents when not needed anymore.
 **/

GSList *
burner_track_data_cfg_get_restored_list (BurnerTrackDataCfg *track)
{
	BurnerTrackDataCfgPrivate *priv;
	BurnerFilteredUri *filtered;

	g_return_val_if_fail (BURNER_TRACK_DATA_CFG (track), NULL);
	priv = BURNER_TRACK_DATA_CFG_PRIVATE (track);

	filtered = burner_data_vfs_get_filtered_model (BURNER_DATA_VFS (priv->tree));
	return burner_filtered_uri_get_restored_list (filtered);
}

/**
 * burner_track_data_cfg_load_medium:
 * @track: a #BurnerTrackDataCfg
 * @medium: a #BurnerMedium
 * @error: a #GError
 *
 * Tries to load the contents of the last session of @medium so all its files will be included in the tree
 * to perform a merge between files from the session and new added files.
 * Errors are stored in @error.
 *
 * Return value: a #gboolean. TRUE if the operation was successful, FALSE otherwise
 **/

gboolean
burner_track_data_cfg_load_medium (BurnerTrackDataCfg *track,
				    BurnerMedium *medium,
				    GError **error)
{
	BurnerTrackDataCfgPrivate *priv;

	g_return_val_if_fail (BURNER_TRACK_DATA_CFG (track), FALSE);
	priv = BURNER_TRACK_DATA_CFG_PRIVATE (track);
	return burner_data_session_add_last (BURNER_DATA_SESSION (priv->tree),
					      medium,
					      error);
}

/**
 * burner_track_data_cfg_unload_current_medium:
 * @track: a #BurnerTrackDataCfg
 *
 * Unload the contents of the last session of the currently loaded medium.
 * See burner_track_data_cfg_load_medium ().
 *
 **/

void
burner_track_data_cfg_unload_current_medium (BurnerTrackDataCfg *track)
{
	BurnerTrackDataCfgPrivate *priv;

	g_return_if_fail (BURNER_TRACK_DATA_CFG (track));
	priv = BURNER_TRACK_DATA_CFG_PRIVATE (track);
	burner_data_session_remove_last (BURNER_DATA_SESSION (priv->tree));
}

/**
 * burner_track_data_cfg_get_current_medium:
 * @track: a #BurnerTrackDataCfg
 *
 * Gets the currently loaded medium.
 *
 * Return value: a #BurnerMedium. NULL if no medium are currently loaded.
 * Do not unref when the #BurnerMedium is not needed anymore.
 **/

BurnerMedium *
burner_track_data_cfg_get_current_medium (BurnerTrackDataCfg *track)
{
	BurnerTrackDataCfgPrivate *priv;

	g_return_val_if_fail (BURNER_TRACK_DATA_CFG (track), NULL);
	priv = BURNER_TRACK_DATA_CFG_PRIVATE (track);
	return burner_data_session_get_loaded_medium (BURNER_DATA_SESSION (priv->tree));
}

/**
 * burner_track_data_cfg_get_available_media:
 * @track: a #BurnerTrackDataCfg
 *
 * Gets a list of all the media that can be appended with new data and which have a session that can be loaded.
 *
 * Return value: a #GSList of #BurnerMedium. Free the list and unref its contents when the list is not needed anymore.
 **/

GSList *
burner_track_data_cfg_get_available_media (BurnerTrackDataCfg *track)
{
	BurnerTrackDataCfgPrivate *priv;

	g_return_val_if_fail (BURNER_TRACK_DATA_CFG (track), NULL);
	priv = BURNER_TRACK_DATA_CFG_PRIVATE (track);
	return burner_data_session_get_available_media (BURNER_DATA_SESSION (priv->tree));
}

static BurnerBurnResult
burner_track_data_cfg_set_source (BurnerTrackData *track,
				   GSList *grafts,
				   GSList *excluded)
{
	BurnerTrackDataCfgPrivate *priv;

	priv = BURNER_TRACK_DATA_CFG_PRIVATE (track);
	if (!grafts)
		return BURNER_BURN_ERR;

	priv->loading = burner_data_project_load_contents (BURNER_DATA_PROJECT (priv->tree),
							    grafts,
							    excluded);

	/* Remember that we own the list grafts and excluded
	 * so we have to free them ourselves. */
	g_slist_foreach (grafts, (GFunc) burner_graft_point_free, NULL);
	g_slist_free (grafts);

	g_slist_foreach (excluded, (GFunc) g_free, NULL);
	g_slist_free (excluded);

	if (!priv->loading)
		return BURNER_BURN_OK;

	return BURNER_BURN_NOT_READY;
}

static BurnerBurnResult
burner_track_data_cfg_add_fs (BurnerTrackData *track,
			       BurnerImageFS fstype)
{
	BurnerTrackDataCfgPrivate *priv;

	priv = BURNER_TRACK_DATA_CFG_PRIVATE (track);
	priv->forced_fs |= fstype;
	priv->banned_fs &= ~(fstype);
	return BURNER_BURN_OK;
}

static BurnerBurnResult
burner_track_data_cfg_rm_fs (BurnerTrackData *track,
			      BurnerImageFS fstype)
{
	BurnerTrackDataCfgPrivate *priv;

	priv = BURNER_TRACK_DATA_CFG_PRIVATE (track);
	priv->banned_fs |= fstype;
	priv->forced_fs &= ~(fstype);
	return BURNER_BURN_OK;
}

static BurnerImageFS
burner_track_data_cfg_get_fs (BurnerTrackData *track)
{
	BurnerTrackDataCfgPrivate *priv;
	BurnerFileTreeStats *stats;
	BurnerImageFS fs_type;
	BurnerFileNode *root;

	priv = BURNER_TRACK_DATA_CFG_PRIVATE (track);

	root = burner_data_project_get_root (BURNER_DATA_PROJECT (priv->tree));
	stats = BURNER_FILE_NODE_STATS (root);

	fs_type = BURNER_IMAGE_FS_ISO;
	fs_type |= priv->forced_fs;

	if (burner_data_project_has_symlinks (BURNER_DATA_PROJECT (priv->tree)))
		fs_type |= BURNER_IMAGE_FS_SYMLINK;
	else {
		/* These two are incompatible with symlinks */
		if (burner_data_project_is_joliet_compliant (BURNER_DATA_PROJECT (priv->tree)))
			fs_type |= BURNER_IMAGE_FS_JOLIET;

		if (burner_data_project_is_video_project (BURNER_DATA_PROJECT (priv->tree)))
			fs_type |= BURNER_IMAGE_FS_VIDEO;
	}

	if (stats->num_2GiB != 0) {
		fs_type |= BURNER_IMAGE_ISO_FS_LEVEL_3;
		if (!(fs_type & BURNER_IMAGE_FS_SYMLINK))
			fs_type |= BURNER_IMAGE_FS_UDF;
	}

	if (stats->num_deep != 0)
		fs_type |= BURNER_IMAGE_ISO_FS_DEEP_DIRECTORY;

	fs_type &= ~(priv->banned_fs);
	return fs_type;
}

static GSList *
burner_track_data_cfg_get_grafts (BurnerTrackData *track)
{
	BurnerTrackDataCfgPrivate *priv;
	BurnerImageFS fs_type;

	priv = BURNER_TRACK_DATA_CFG_PRIVATE (track);

	if (priv->grafts)
		return priv->grafts;

	/* append a slash for mkisofs */
	fs_type = burner_track_data_cfg_get_fs (track);
	burner_data_project_get_contents (BURNER_DATA_PROJECT (priv->tree),
					   &priv->grafts,
					   &priv->excluded,
					   TRUE, /* include hidden nodes */
					   (fs_type & BURNER_IMAGE_FS_JOLIET) != 0,
					   TRUE);
	return priv->grafts;
}

static GSList *
burner_track_data_cfg_get_excluded (BurnerTrackData *track)
{
	BurnerTrackDataCfgPrivate *priv;
	BurnerImageFS fs_type;

	priv = BURNER_TRACK_DATA_CFG_PRIVATE (track);

	if (priv->excluded)
		return priv->excluded;

	/* append a slash for mkisofs */
	fs_type = burner_track_data_cfg_get_fs (track);
	burner_data_project_get_contents (BURNER_DATA_PROJECT (priv->tree),
					   &priv->grafts,
					   &priv->excluded,
					   TRUE, /* include hidden nodes */
					   (fs_type & BURNER_IMAGE_FS_JOLIET) != 0,
					   TRUE);
	return priv->excluded;
}

static guint64
burner_track_data_cfg_get_file_num (BurnerTrackData *track)
{
	BurnerTrackDataCfgPrivate *priv;
	BurnerFileTreeStats *stats;
	BurnerFileNode *root;

	priv = BURNER_TRACK_DATA_CFG_PRIVATE (track);

	root = burner_data_project_get_root (BURNER_DATA_PROJECT (priv->tree));
	stats = BURNER_FILE_NODE_STATS (root);

	return stats->children;
}

static BurnerBurnResult
burner_track_data_cfg_get_track_type (BurnerTrack *track,
				       BurnerTrackType *type)
{
	burner_track_type_set_has_data (type);
	burner_track_type_set_data_fs (type, burner_track_data_cfg_get_fs (BURNER_TRACK_DATA (track)));

	return BURNER_BURN_OK;
}

static BurnerBurnResult
burner_track_data_cfg_get_status (BurnerTrack *track,
				   BurnerStatus *status)
{
	BurnerTrackDataCfgPrivate *priv;

	priv = BURNER_TRACK_DATA_CFG_PRIVATE (track);

	if (priv->loading) {
		burner_status_set_not_ready (status,
					      (gdouble) (priv->loading - priv->loading_remaining) / (gdouble) priv->loading,
					      _("Analysing files"));
		return BURNER_BURN_NOT_READY;
	}

	/* This one goes before the next since a node may be loading but not
	 * yet in the project and therefore project will look empty */
	if (burner_data_vfs_is_active (BURNER_DATA_VFS (priv->tree))) {
		if (status)
			burner_status_set_running (status,
						    -1.0,
						    _("Analysing files"));
		return BURNER_BURN_NOT_READY;
	}

	if (priv->load_errors) {
		g_slist_foreach (priv->load_errors, (GFunc) g_free, NULL);
		g_slist_free (priv->load_errors);
		priv->load_errors = NULL;
		return BURNER_BURN_ERR;
	}

	if (burner_data_project_is_empty (BURNER_DATA_PROJECT (priv->tree))) {
		if (status)
			burner_status_set_error (status,
						  g_error_new (BURNER_BURN_ERROR,
						               BURNER_BURN_ERROR_EMPTY,
						               _("There are no files to write to disc")));
		return BURNER_BURN_ERR;
	}

	return BURNER_BURN_OK;
}

static BurnerBurnResult
burner_track_data_cfg_get_size (BurnerTrack *track,
				 goffset *blocks,
				 goffset *block_size)
{
	BurnerTrackDataCfgPrivate *priv;
	goffset sectors = 0;

	priv = BURNER_TRACK_DATA_CFG_PRIVATE (track);

	sectors = burner_data_project_get_sectors (BURNER_DATA_PROJECT (priv->tree));
	if (blocks) {
		BurnerFileNode *root;
		BurnerImageFS fs_type;
		BurnerFileTreeStats *stats;

		if (!sectors)
			return sectors;

		fs_type = burner_track_data_cfg_get_fs (BURNER_TRACK_DATA (track));
		root = burner_data_project_get_root (BURNER_DATA_PROJECT (priv->tree));
		stats = BURNER_FILE_NODE_STATS (root);
		sectors = burner_data_project_improve_image_size_accuracy (sectors,
									    stats->num_dir,
									    fs_type);
		*blocks = sectors;
	}

	if (block_size)
		*block_size = 2048;

	return BURNER_BURN_OK;
}

static BurnerBurnResult
burner_track_data_cfg_image_uri_cb (BurnerDataVFS *vfs,
				     const gchar *uri,
				     BurnerTrackDataCfg *self)
{
	BurnerTrackDataCfgPrivate *priv;
	GValue instance_and_params [2];
	GValue return_value;
	GValue *params;

	priv = BURNER_TRACK_DATA_CFG_PRIVATE (self);
	if (priv->loading)
		return BURNER_BURN_OK;

	/* object which signalled */
	instance_and_params->g_type = 0;
	g_value_init (instance_and_params, G_TYPE_FROM_INSTANCE (self));
	g_value_set_instance (instance_and_params, self);

	/* arguments of signal (name) */
	params = instance_and_params + 1;
	params->g_type = 0;
	g_value_init (params, G_TYPE_STRING);
	g_value_set_string (params, uri);

	/* default to OK (for addition) */
	return_value.g_type = 0;
	g_value_init (&return_value, G_TYPE_INT);
	g_value_set_int (&return_value, BURNER_BURN_OK);

	g_signal_emitv (instance_and_params,
			burner_track_data_cfg_signals [IMAGE],
			0,
			&return_value);

	g_value_unset (instance_and_params);
	g_value_unset (params);

	return g_value_get_int (&return_value);
}

static void
burner_track_data_cfg_unreadable_uri_cb (BurnerDataVFS *vfs,
					  const GError *error,
					  const gchar *uri,
					  BurnerTrackDataCfg *self)
{
	BurnerTrackDataCfgPrivate *priv;

	priv = BURNER_TRACK_DATA_CFG_PRIVATE (self);

	if (priv->loading) {
		priv->load_errors = g_slist_prepend (priv->load_errors,
						     g_strdup (error->message));

		return;
	}

	g_signal_emit (self,
		       burner_track_data_cfg_signals [UNREADABLE],
		       0,
		       error,
		       uri);
}

static void
burner_track_data_cfg_recursive_uri_cb (BurnerDataVFS *vfs,
					 const gchar *uri,
					 BurnerTrackDataCfg *self)
{
	BurnerTrackDataCfgPrivate *priv;

	priv = BURNER_TRACK_DATA_CFG_PRIVATE (self);

	if (priv->loading) {
		gchar *message;
		gchar *name;

		name = burner_utils_get_uri_name (uri);
		message = g_strdup_printf (_("\"%s\" is a recursive symbolic link."), name);
		priv->load_errors = g_slist_prepend (priv->load_errors, message);
		g_free (name);

		return;
	}
	g_signal_emit (self,
		       burner_track_data_cfg_signals [RECURSIVE],
		       0,
		       uri);
}

static void
burner_track_data_cfg_unknown_uri_cb (BurnerDataVFS *vfs,
				       const gchar *uri,
				       BurnerTrackDataCfg *self)
{
	BurnerTrackDataCfgPrivate *priv;

	priv = BURNER_TRACK_DATA_CFG_PRIVATE (self);

	if (priv->loading) {
		gchar *message;
		gchar *name;

		name = burner_utils_get_uri_name (uri);
		message = g_strdup_printf (_("\"%s\" cannot be found."), name);
		priv->load_errors = g_slist_prepend (priv->load_errors, message);
		g_free (name);

		return;
	}
	g_signal_emit (self,
		       burner_track_data_cfg_signals [UNKNOWN],
		       0,
		       uri);
}

static gboolean
burner_track_data_cfg_autorun_inf_update (BurnerTrackDataCfg *self)
{
	BurnerTrackDataCfgPrivate *priv;
	gchar *icon_path = NULL;
	gsize data_size = 0;
	GKeyFile *key_file;
	gchar *data = NULL;
	gchar *path = NULL;
	gchar *uri;
	int fd;

	priv = BURNER_TRACK_DATA_CFG_PRIVATE (self);

	uri = burner_data_project_node_to_uri (BURNER_DATA_PROJECT (priv->tree), priv->autorun);
	path = g_filename_from_uri (uri, NULL, NULL);
	g_free (uri);

	fd = open (path, O_WRONLY|O_TRUNC);
	g_free (path);

	if (fd == -1)
		return FALSE;

	icon_path = burner_data_project_node_to_path (BURNER_DATA_PROJECT (priv->tree), priv->icon);

	/* Write the autorun.inf if we don't have one yet */
	key_file = g_key_file_new ();
	g_key_file_set_value (key_file, "autorun", "icon", icon_path);
	g_free (icon_path);

	data = g_key_file_to_data (key_file, &data_size, NULL);
	g_key_file_free (key_file);

	if (write (fd, data, data_size) == -1) {
		g_free (data);
		close (fd);
		return FALSE;
	}

	g_free (data);
	close (fd);
	return TRUE;
}

static gchar *
burner_track_data_cfg_find_icon_name (BurnerTrackDataCfg *track)
{
	BurnerTrackDataCfgPrivate *priv;
	BurnerFileNode *root;
	gchar *name = NULL;
	int i = 0;

	priv = BURNER_TRACK_DATA_CFG_PRIVATE (track);

	root = burner_data_project_get_root (BURNER_DATA_PROJECT (priv->tree));
	do {
		g_free (name);
		name = g_strdup_printf ("Autorun%i.ico", i++);
	} while (burner_file_node_check_name_existence (root, name));

	return name;
}

static void
burner_track_data_cfg_joliet_rename_cb (BurnerDataProject *project,
					 BurnerTrackDataCfg *self)
{
	BurnerTrackDataCfgPrivate *priv;

	priv = BURNER_TRACK_DATA_CFG_PRIVATE (self);

	/* Signal this once */
	if (priv->joliet_rename)
		return;

	g_signal_emit (self,
		       burner_track_data_cfg_signals [JOLIET_RENAME_SIGNAL],
		       0);

	priv->joliet_rename = 1;
}

static void
burner_track_data_cfg_virtual_sibling_cb (BurnerDataProject *project,
					   BurnerFileNode *node,
					   BurnerFileNode *sibling,
					   BurnerTrackDataCfg *self)
{
	BurnerTrackDataCfgPrivate *priv;

	priv = BURNER_TRACK_DATA_CFG_PRIVATE (self);
	if (sibling == priv->icon) {
		/* This is a warning that the icon has been added. Update our 
		 * icon node and wait for it to appear in the callback for the
		 * 'node-added' signal. Then we'll be able to fire the "icon-
		 * changed" signal. */
		priv->icon = node;
	}
}

static gboolean
burner_track_data_cfg_name_collision_cb (BurnerDataProject *project,
					  BurnerFileNode *node,
					  BurnerTrackDataCfg *self)
{
	BurnerTrackDataCfgPrivate *priv;
	gboolean result;

	priv = BURNER_TRACK_DATA_CFG_PRIVATE (self);

	/* some names are interesting for us */
	if (node == priv->autorun) {
		BurnerFileNode *icon;

		/* An autorun.inf has been added by the user. Whether or not we
		 * are loading a project, if there is an autorun.inf file, then
		 * wipe it, parse the new and signal */

		/* Save icon node as we'll need it afterwards to remove it */
		icon = priv->icon;

		/* Do it now as this is the hidden temporarily created
		 * graft point whose deletion won't be signalled by 
		 * BurnerDataTreeModel */
		burner_track_data_clean_autorun (self);
		burner_data_project_remove_node (BURNER_DATA_PROJECT (priv->tree), icon);

		g_signal_emit (self,
			       burner_track_data_cfg_signals [ICON_CHANGED],
			       0);

		return FALSE;
	}

	if (node == priv->icon) {
		gchar *uri;
		gchar *name = NULL;
		BurnerFileNode *root;

		/* we need to recreate another one with a different name */
		uri = burner_data_project_node_to_uri (BURNER_DATA_PROJECT (priv->tree), node);
		root = burner_data_project_get_root (BURNER_DATA_PROJECT (priv->tree));
		name = burner_track_data_cfg_find_icon_name (self);

		priv->icon = burner_data_project_add_hidden_node (BURNER_DATA_PROJECT (priv->tree),
								   uri,
								   name,
								   root);
		g_free (name);
		g_free (uri);

		/* Update our autorun.inf */
		burner_track_data_cfg_autorun_inf_update (self);
		return FALSE;
	}

	if (priv->loading) {
		/* don't do anything accept replacement */
		return FALSE;
	}

	g_signal_emit (self,
		       burner_track_data_cfg_signals [NAME_COLLISION],
		       0,
		       BURNER_FILE_NODE_NAME (node),
		       &result);
	return result;
}

static gboolean
burner_track_data_cfg_deep_directory (BurnerDataProject *project,
				       const gchar *string,
				       BurnerTrackDataCfg *self)
{
	BurnerTrackDataCfgPrivate *priv;
	gboolean result;

	priv = BURNER_TRACK_DATA_CFG_PRIVATE (self);

	if (priv->deep_directory)
		return FALSE;

	if (priv->loading) {
		/* don't do anything just accept these directories from now on */
		priv->deep_directory = TRUE;
		return FALSE;
	}

	g_signal_emit (self,
		       burner_track_data_cfg_signals [DEEP_DIRECTORY],
		       0,
		       string,
		       &result);

	priv->deep_directory = result;
	return result;
}

static gboolean
burner_track_data_cfg_2G_file (BurnerDataProject *project,
				const gchar *string,
				BurnerTrackDataCfg *self)
{
	BurnerTrackDataCfgPrivate *priv;
	gboolean result;

	priv = BURNER_TRACK_DATA_CFG_PRIVATE (self);

	/* This is to make sure we asked once */
	if (priv->G2_files)
		return FALSE;

	if (priv->loading) {
		/* don't do anything just accept these files from now on */
		priv->G2_files = TRUE;
		return FALSE;
	}

	g_signal_emit (self,
		       burner_track_data_cfg_signals [G2_FILE],
		       0,
		       string,
		       &result);
	priv->G2_files = result;
	return result;
}

static void
burner_track_data_cfg_project_loaded (BurnerDataProject *project,
				       gint loading,
				       BurnerTrackDataCfg *self)
{
	BurnerTrackDataCfgPrivate *priv;

	priv = BURNER_TRACK_DATA_CFG_PRIVATE (self);

	priv->loading_remaining = loading;
	if (loading > 0) {
		gdouble progress;

		progress = (gdouble) (priv->loading - priv->loading_remaining) / (gdouble) priv->loading;
		g_signal_emit (self,
			       burner_track_data_cfg_signals [SOURCE_LOADING],
			       0,
			       progress);
		return;
	}

	priv->loading = 0;
	g_signal_emit (self,
		       burner_track_data_cfg_signals [SOURCE_LOADED],
		       0,
		       priv->load_errors);
}

static void
burner_track_data_cfg_activity_changed (BurnerDataVFS *vfs,
					 gboolean active,
					 BurnerTrackDataCfg *self)
{
	GSList *nodes;
	GtkTreeIter iter;
	BurnerTrackDataCfgPrivate *priv;

	if (burner_data_vfs_is_active (vfs))
		goto emit_signal;

	priv = BURNER_TRACK_DATA_CFG_PRIVATE (self);

	/* This is used when we finished exploring so we can update BURNER_DATA_TREE_MODEL_PERCENT */
	iter.stamp = priv->stamp;
	iter.user_data2 = GINT_TO_POINTER (BURNER_ROW_REGULAR);

	/* NOTE: we shouldn't need to use reference here as unref_node is used */
	for (nodes = priv->shown; nodes; nodes = nodes->next) {
		GtkTreePath *treepath;

		iter.user_data = nodes->data;
		treepath = burner_track_data_cfg_node_to_path (self, nodes->data);
		gtk_tree_model_row_changed (GTK_TREE_MODEL (self), treepath, &iter);
		gtk_tree_path_free (treepath);
	}

emit_signal:

	burner_track_data_cfg_clean_cache (self);

	burner_track_changed (BURNER_TRACK (self));
}

static void
burner_track_data_cfg_size_changed_cb (BurnerDataProject *project,
					BurnerTrackDataCfg *self)
{
	burner_track_data_cfg_clean_cache (self);
	burner_track_changed (BURNER_TRACK (self));
}

static void
burner_track_data_cfg_session_available_cb (BurnerDataSession *session,
					     BurnerMedium *medium,
					     gboolean available,
					     BurnerTrackDataCfg *self)
{
	g_signal_emit (self,
		       burner_track_data_cfg_signals [AVAILABLE],
		       0,
		       medium,
		       available);
}

static void
burner_track_data_cfg_session_loaded_cb (BurnerDataSession *session,
					  BurnerMedium *medium,
					  gboolean loaded,
					  BurnerTrackDataCfg *self)
{
	g_signal_emit (self,
		       burner_track_data_cfg_signals [LOADED],
		       0,
		       medium,
		       loaded);

	burner_track_changed (BURNER_TRACK (self));
}

/**
 * burner_track_data_cfg_span:
 * @track: a #BurnerTrackDataCfg
 * @sectors: a #goffset
 * @new_track: a #BurnerTrackData
 *
 * Creates a new #BurnerTrackData (stored in @new_track) from the files contained in @track. The sum of their sizes
 * does not exceed @sectors. This allows to burn a tree on multiple discs. This function can be
 * called repeatedly; in this case if some files were left out after the previous calls, the newly created BurnerTrackData
 * is created with all or part of the remaining files.
 *
 * Return value: a #BurnerBurnResult. BURNER_BURN_OK if there is not anymore data.
 * BURNER_BURN_RETRY if the operation was successful and a new #BurnerTrackDataCfg was created.
 * BURNER_BURN_ERR otherwise.
 **/

BurnerBurnResult
burner_track_data_cfg_span (BurnerTrackDataCfg *track,
			     goffset sectors,
			     BurnerTrackData *new_track)
{
	BurnerTrackDataCfgPrivate *priv;
	BurnerBurnResult result;

	priv = BURNER_TRACK_DATA_CFG_PRIVATE (track);
	if (priv->loading
	||  burner_data_vfs_is_active (BURNER_DATA_VFS (priv->tree))
	||  burner_data_session_get_loaded_medium (BURNER_DATA_SESSION (priv->tree)) != NULL)
		return BURNER_BURN_NOT_READY;

	result = burner_data_project_span (BURNER_DATA_PROJECT (priv->tree),
					    sectors,
					    TRUE,
					    TRUE, /* FIXME */
					    new_track);
	if (result != BURNER_BURN_RETRY)
		return result;

	burner_track_tag_copy_missing (BURNER_TRACK (new_track), BURNER_TRACK (track));
	return BURNER_BURN_RETRY;
}

/**
 * burner_track_data_cfg_span_again:
 * @track: a #BurnerTrackDataCfg
 *
 * Checks whether some files were not included during calls to burner_track_data_cfg_span ().
 *
 * Return value: a #BurnerBurnResult. BURNER_BURN_OK if there is not anymore data.
 * BURNER_BURN_RETRY if the operation was successful and a new #BurnerTrackDataCfg was created.
 * BURNER_BURN_ERR otherwise.
 **/

BurnerBurnResult
burner_track_data_cfg_span_again (BurnerTrackDataCfg *track)
{
	BurnerTrackDataCfgPrivate *priv;

	priv = BURNER_TRACK_DATA_CFG_PRIVATE (track);
	return burner_data_project_span_again (BURNER_DATA_PROJECT (priv->tree));
}

/**
 * burner_track_data_cfg_span_possible:
 * @track: a #BurnerTrackDataCfg
 * @sectors: a #goffset
 *
 * Checks if a new #BurnerTrackData can be created from the files remaining in the tree 
 * after calls to burner_track_data_cfg_span ().
 *
 * Return value: a #BurnerBurnResult. BURNER_BURN_OK if there is not anymore data.
 * BURNER_BURN_RETRY if the operation was successful and a new #BurnerTrackDataCfg was created.
 * BURNER_BURN_ERR otherwise.
 **/

BurnerBurnResult
burner_track_data_cfg_span_possible (BurnerTrackDataCfg *track,
				      goffset sectors)
{
	BurnerTrackDataCfgPrivate *priv;

	priv = BURNER_TRACK_DATA_CFG_PRIVATE (track);
	if (priv->loading
	||  burner_data_vfs_is_active (BURNER_DATA_VFS (priv->tree))
	||  burner_data_session_get_loaded_medium (BURNER_DATA_SESSION (priv->tree)) != NULL)
		return BURNER_BURN_NOT_READY;

	return burner_data_project_span_possible (BURNER_DATA_PROJECT (priv->tree),
						   sectors);
}

/**
 * burner_track_data_cfg_span_stop:
 * @track: a #BurnerTrackDataCfg
 *
 * Resets the list of files that were included after calls to burner_track_data_cfg_span ().
 **/

void
burner_track_data_cfg_span_stop (BurnerTrackDataCfg *track)
{
	BurnerTrackDataCfgPrivate *priv;

	priv = BURNER_TRACK_DATA_CFG_PRIVATE (track);
	burner_data_project_span_stop (BURNER_DATA_PROJECT (priv->tree));
}

/**
 * burner_track_data_cfg_span_max_space:
 * @track: a #BurnerTrackDataCfg
 *
 * Returns the maximum required space (in sectors) 
 * among all the possible spanned batches.
 * This means that when burningto a media
 * it will also be the minimum required
 * space to burn all the contents in several
 * batches.
 *
 * Return value: a #goffset.
 **/

goffset
burner_track_data_cfg_span_max_space (BurnerTrackDataCfg *track)
{
	BurnerTrackDataCfgPrivate *priv;

	priv = BURNER_TRACK_DATA_CFG_PRIVATE (track);
	return burner_data_project_get_max_space (BURNER_DATA_PROJECT (priv->tree));
}

/**
 * This is to handle the icon for the image
 */

/**
 * burner_track_data_cfg_get_icon_path:
 * @track: a #BurnerTrackDataCfg
 *
 * Returns a path pointing to the currently selected icon file.
 *
 * Return value: a #gchar or NULL.
 **/

gchar *
burner_track_data_cfg_get_icon_path (BurnerTrackDataCfg *track)
{
	BurnerTrackDataCfgPrivate *priv;

	g_return_val_if_fail (BURNER_IS_TRACK_DATA_CFG (track), NULL);

	priv = BURNER_TRACK_DATA_CFG_PRIVATE (track);
	if (!priv->image_file)
		return NULL;

	return g_file_get_path (priv->image_file);
}

/**
 * burner_track_data_cfg_get_icon:
 * @track: a #BurnerTrackDataCfg
 *
 * Returns the currently selected icon.
 *
 * Return value: a #GIcon or NULL.
 **/

GIcon *
burner_track_data_cfg_get_icon (BurnerTrackDataCfg *track)
{
	gchar *array [] = {"media-optical", NULL};
	BurnerTrackDataCfgPrivate *priv;
	BurnerMedium *medium;
	GIcon *icon;

	g_return_val_if_fail (BURNER_IS_TRACK_DATA_CFG (track), NULL);

	priv = BURNER_TRACK_DATA_CFG_PRIVATE (track);
	if (priv->image_file)
		icon = g_file_icon_new (priv->image_file);
	else if ((medium = burner_data_session_get_loaded_medium (BURNER_DATA_SESSION (priv->tree))))
		icon = burner_volume_get_icon (BURNER_VOLUME (medium));
	else
		icon = g_themed_icon_new_from_names (array, -1);

	return icon;
}

static gchar *
burner_track_data_cfg_get_scaled_icon_path (BurnerTrackDataCfg *track)
{
	BurnerTrackDataCfgPrivate *priv;
	gchar *path;
	gchar *uri;

	g_return_val_if_fail (BURNER_IS_TRACK_DATA_CFG (track), NULL);
	priv = BURNER_TRACK_DATA_CFG_PRIVATE (track);
	if (!priv->icon || BURNER_FILE_NODE_VIRTUAL (priv->icon))
		return NULL;

	uri = burner_data_project_node_to_uri (BURNER_DATA_PROJECT (priv->tree), priv->icon);
	path = g_filename_from_uri (uri, NULL, NULL);
	g_free (uri);

	return path;
}

/**
 * burner_track_data_cfg_set_icon:
 * @track: a #BurnerTrackDataCfg
 * @icon_path: a #gchar
 * @error: a #GError
 *
 * Sets the current icon.
 *
 * Return value: a #gboolean. TRUE if the operation was successful, FALSE otherwise
 **/

gboolean
burner_track_data_cfg_set_icon (BurnerTrackDataCfg *track,
				 const gchar *icon_path,
				 GError **error)
{
	gboolean result;
	GdkPixbuf *pixbuf;
	BurnerFileNode *root;
	BurnerTrackDataCfgPrivate *priv;

	g_return_val_if_fail (BURNER_IS_TRACK_DATA_CFG (track), FALSE);

	priv = BURNER_TRACK_DATA_CFG_PRIVATE (track);

	/* Check whether we don't have an added (by the user) autorun.inf as it
	 * won't be possible to edit it. */
	root = burner_data_project_get_root (BURNER_DATA_PROJECT (priv->tree));

	if (!priv->autorun) {
		BurnerFileNode *node;

		node = burner_file_node_check_name_existence_case (root, "autorun.inf");
		if (node && !node->is_imported) {
			/* There is a native autorun.inf file. That's why we can't edit
			 * it; even if we were to create a temporary file with just the
			 * icon changed then we could not save it as a project later.
			 * If I change my mind, I should remember that it the path is
			 * the value ON DISC.
			 * The only exception is if the autorun.inf is an autorun.inf
			 * that was imported from another session when we're 
			 * merging. */
			return FALSE;
		}
	}

	/* Load and convert (48x48) the image into a pixbuf */
	pixbuf = gdk_pixbuf_new_from_file_at_scale (icon_path,
						    48,
						    48,
						    FALSE,
						    error);
	if (!pixbuf)
		return FALSE;

	/* See if we already have an icon set. If we do, reuse the tmp file */
	if (!priv->icon) {
		gchar *buffer = NULL;
		gchar *path = NULL;
		gchar *name = NULL;
		gsize buffer_size;
		int icon_fd;
		gchar *uri;

		icon_fd = g_file_open_tmp (BURNER_BURN_TMP_FILE_NAME,
					   &path,
					   error);
		if (icon_fd == -1) {
			g_object_unref (pixbuf);
			return FALSE;
		}

		/* Add it as a graft to the project */
		uri = g_filename_to_uri (path, NULL, NULL);
		g_free (path);

 		name = burner_track_data_cfg_find_icon_name (track);
		priv->icon = burner_data_project_add_hidden_node (BURNER_DATA_PROJECT (priv->tree),
								   uri,
								   name,
								   root);
		g_free (name);
		g_free (uri);

		/* Write it as an "ico" file (or a png?) */
		result = gdk_pixbuf_save_to_buffer (pixbuf,
						    &buffer,
						    &buffer_size,
						    "ico",
						    error,
						    NULL);
		if (!result) {
			close (icon_fd);
			g_object_unref (pixbuf);
			return FALSE;
		}

		if (write (icon_fd, buffer, buffer_size) == -1) {
			g_object_unref (pixbuf);
			g_free (buffer);
			close (icon_fd);
			return FALSE;
		}

		g_free (buffer);
		close (icon_fd);
	}
	else {
		gchar *path;

		path = burner_track_data_cfg_get_scaled_icon_path (track);

		/* Write it as an "ico" file (or a png?) */
		result = gdk_pixbuf_save (pixbuf,
					  path,
					  "ico",
					  error,
					  NULL);
		g_free (path);

		if (!result) {
			g_object_unref (pixbuf);
			return FALSE;
		}
	}

	g_object_unref (pixbuf);

	if (!priv->autorun) {
		gchar *path = NULL;
		gchar *uri;
		int fd;

		/* Get a temporary file if we don't have one yet */
		fd = g_file_open_tmp (BURNER_BURN_TMP_FILE_NAME,
				      &path,
				      error);
		close (fd);

		/* Add it as a graft to the project */
		uri = g_filename_to_uri (path, NULL, NULL);
		g_free (path);

		priv->autorun = burner_data_project_add_hidden_node (BURNER_DATA_PROJECT (priv->tree),
								      uri,
								      "autorun.inf",
								      root);
		g_free (uri);

		/* write the autorun.inf */
		burner_track_data_cfg_autorun_inf_update (track);
	}

	if (priv->image_file) {
		g_object_unref (priv->image_file);
		priv->image_file = NULL;
	}

	priv->image_file = g_file_new_for_path (icon_path);
	g_signal_emit (track,
		       burner_track_data_cfg_signals [ICON_CHANGED],
		       0);
	return TRUE;
}

static void
burner_track_data_cfg_init (BurnerTrackDataCfg *object)
{
	BurnerTrackDataCfgPrivate *priv;

	priv = BURNER_TRACK_DATA_CFG_PRIVATE (object);

	priv->sort_column = GTK_TREE_SORTABLE_UNSORTED_SORT_COLUMN_ID;
	do {
		priv->stamp = g_random_int ();
	} while (!priv->stamp);

	priv->theme = gtk_icon_theme_get_default ();
	priv->tree = burner_data_tree_model_new ();

	g_signal_connect (priv->tree,
			  "row-added",
			  G_CALLBACK (burner_track_data_cfg_node_added),
			  object);
	g_signal_connect (priv->tree,
			  "row-changed",
			  G_CALLBACK (burner_track_data_cfg_node_changed),
			  object);
	g_signal_connect (priv->tree,
			  "row-removed",
			  G_CALLBACK (burner_track_data_cfg_node_removed),
			  object);
	g_signal_connect (priv->tree,
			  "rows-reordered",
			  G_CALLBACK (burner_track_data_cfg_node_reordered),
			  object);

	g_signal_connect (priv->tree,
			  "size-changed",
			  G_CALLBACK (burner_track_data_cfg_size_changed_cb),
			  object);

	g_signal_connect (priv->tree,
			  "session-available",
			  G_CALLBACK (burner_track_data_cfg_session_available_cb),
			  object);
	g_signal_connect (priv->tree,
			  "session-loaded",
			  G_CALLBACK (burner_track_data_cfg_session_loaded_cb),
			  object);
	
	g_signal_connect (priv->tree,
			  "project-loaded",
			  G_CALLBACK (burner_track_data_cfg_project_loaded),
			  object);
	g_signal_connect (priv->tree,
			  "vfs-activity",
			  G_CALLBACK (burner_track_data_cfg_activity_changed),
			  object);
	g_signal_connect (priv->tree,
			  "deep-directory",
			  G_CALLBACK (burner_track_data_cfg_deep_directory),
			  object);
	g_signal_connect (priv->tree,
			  "2G-file",
			  G_CALLBACK (burner_track_data_cfg_2G_file),
			  object);
	g_signal_connect (priv->tree,
			  "unreadable-uri",
			  G_CALLBACK (burner_track_data_cfg_unreadable_uri_cb),
			  object);
	g_signal_connect (priv->tree,
			  "unknown-uri",
			  G_CALLBACK (burner_track_data_cfg_unknown_uri_cb),
			  object);
	g_signal_connect (priv->tree,
			  "recursive-sym",
			  G_CALLBACK (burner_track_data_cfg_recursive_uri_cb),
			  object);
	g_signal_connect (priv->tree,
			  "image-uri",
			  G_CALLBACK (burner_track_data_cfg_image_uri_cb),
			  object);
	g_signal_connect (priv->tree,
			  "virtual-sibling",
			  G_CALLBACK (burner_track_data_cfg_virtual_sibling_cb),
			  object);
	g_signal_connect (priv->tree,
			  "name-collision",
			  G_CALLBACK (burner_track_data_cfg_name_collision_cb),
			  object);
	g_signal_connect (priv->tree,
			  "joliet-rename",
			  G_CALLBACK (burner_track_data_cfg_joliet_rename_cb),
			  object);
}

static void
burner_track_data_cfg_finalize (GObject *object)
{
	BurnerTrackDataCfgPrivate *priv;

	priv = BURNER_TRACK_DATA_CFG_PRIVATE (object);

	burner_track_data_clean_autorun (BURNER_TRACK_DATA_CFG (object));
	burner_track_data_cfg_clean_cache (BURNER_TRACK_DATA_CFG (object));

	if (priv->shown) {
		g_slist_free (priv->shown);
		priv->shown = NULL;
	}

	if (priv->tree) {
		/* This object could outlive us just for some time
		 * so we better remove all signals.
		 * When an image URI is detected it can happen
		 * that we'll be destroyed. */
		g_signal_handlers_disconnect_matched (priv->tree,
		                                      G_SIGNAL_MATCH_DATA,
		                                      0,
		                                      0,
		                                      NULL,
		                                      NULL,
		                                      object);

		g_object_unref (priv->tree);
		priv->tree = NULL;
	}

	G_OBJECT_CLASS (burner_track_data_cfg_parent_class)->finalize (object);
}

static void
burner_track_data_cfg_class_init (BurnerTrackDataCfgClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);
	BurnerTrackClass *track_class = BURNER_TRACK_CLASS (klass);
	BurnerTrackDataClass *parent_class = BURNER_TRACK_DATA_CLASS (klass);

	g_type_class_add_private (klass, sizeof (BurnerTrackDataCfgPrivate));

	object_class->finalize = burner_track_data_cfg_finalize;

	track_class->get_size = burner_track_data_cfg_get_size;
	track_class->get_type = burner_track_data_cfg_get_track_type;
	track_class->get_status = burner_track_data_cfg_get_status;

	parent_class->set_source = burner_track_data_cfg_set_source;
	parent_class->add_fs = burner_track_data_cfg_add_fs;
	parent_class->rm_fs = burner_track_data_cfg_rm_fs;

	parent_class->get_fs = burner_track_data_cfg_get_fs;
	parent_class->get_grafts = burner_track_data_cfg_get_grafts;
	parent_class->get_excluded = burner_track_data_cfg_get_excluded;
	parent_class->get_file_num = burner_track_data_cfg_get_file_num;

	burner_track_data_cfg_signals [AVAILABLE] = 
	    g_signal_new ("session_available",
			  G_TYPE_FROM_CLASS (klass),
			  G_SIGNAL_RUN_LAST,
			  0,
			  NULL, NULL,
			  burner_marshal_VOID__OBJECT_BOOLEAN,
			  G_TYPE_NONE,
			  2,
			  G_TYPE_OBJECT,
			  G_TYPE_BOOLEAN);
	burner_track_data_cfg_signals [LOADED] = 
	    g_signal_new ("session_loaded",
			  G_TYPE_FROM_CLASS (klass),
			  G_SIGNAL_RUN_LAST,
			  0,
			  NULL, NULL,
			  burner_marshal_VOID__OBJECT_BOOLEAN,
			  G_TYPE_NONE,
			  2,
			  G_TYPE_OBJECT,
			  G_TYPE_BOOLEAN);

	burner_track_data_cfg_signals [IMAGE] = 
	    g_signal_new ("image_uri",
			  G_TYPE_FROM_CLASS (klass),
			  G_SIGNAL_RUN_LAST|G_SIGNAL_NO_RECURSE,
			  0,
			  NULL, NULL,
			  burner_marshal_INT__STRING,
			  G_TYPE_INT,
			  1,
			  G_TYPE_STRING);

	burner_track_data_cfg_signals [UNREADABLE] = 
	    g_signal_new ("unreadable_uri",
			  G_TYPE_FROM_CLASS (klass),
			  G_SIGNAL_RUN_FIRST,
			  0,
			  NULL, NULL,
			  burner_marshal_VOID__POINTER_STRING,
			  G_TYPE_NONE,
			  2,
			  G_TYPE_POINTER,
			  G_TYPE_STRING);

	burner_track_data_cfg_signals [RECURSIVE] = 
	    g_signal_new ("recursive_sym",
			  G_TYPE_FROM_CLASS (klass),
			  G_SIGNAL_RUN_FIRST,
			  0,
			  NULL, NULL,
			  g_cclosure_marshal_VOID__STRING,
			  G_TYPE_NONE,
			  1,
			  G_TYPE_STRING);

	burner_track_data_cfg_signals [UNKNOWN] = 
	    g_signal_new ("unknown_uri",
			  G_TYPE_FROM_CLASS (klass),
			  G_SIGNAL_RUN_FIRST,
			  0,
			  NULL, NULL,
			  g_cclosure_marshal_VOID__STRING,
			  G_TYPE_NONE,
			  1,
			  G_TYPE_STRING);
	burner_track_data_cfg_signals [G2_FILE] = 
	    g_signal_new ("2G_file",
			  G_TYPE_FROM_CLASS (klass),
			  G_SIGNAL_RUN_LAST|G_SIGNAL_NO_RECURSE,
			  0,
			  NULL, NULL,
			  burner_marshal_BOOLEAN__STRING,
			  G_TYPE_BOOLEAN,
			  1,
			  G_TYPE_STRING);
	burner_track_data_cfg_signals [NAME_COLLISION] = 
	    g_signal_new ("name_collision",
			  G_TYPE_FROM_CLASS (klass),
			  G_SIGNAL_RUN_LAST|G_SIGNAL_NO_RECURSE,
			  0,
			  NULL, NULL,
			  burner_marshal_BOOLEAN__STRING,
			  G_TYPE_BOOLEAN,
			  1,
			  G_TYPE_STRING);
	burner_track_data_cfg_signals [JOLIET_RENAME_SIGNAL] = 
	    g_signal_new ("joliet_rename",
			  G_TYPE_FROM_CLASS (klass),
			  G_SIGNAL_RUN_LAST|G_SIGNAL_NO_RECURSE,
			  0,
			  NULL, NULL,
			  g_cclosure_marshal_VOID__VOID,
			  G_TYPE_NONE,
			  0,
			  G_TYPE_NONE);
	burner_track_data_cfg_signals [DEEP_DIRECTORY] = 
	    g_signal_new ("deep_directory",
			  G_TYPE_FROM_CLASS (klass),
			  G_SIGNAL_RUN_LAST|G_SIGNAL_NO_RECURSE,
			  0,
			  NULL, NULL,
			  burner_marshal_BOOLEAN__STRING,
			  G_TYPE_BOOLEAN,
			  1,
			  G_TYPE_STRING);

	burner_track_data_cfg_signals [SOURCE_LOADING] = 
	    g_signal_new ("source_loading",
			  G_TYPE_FROM_CLASS (klass),
			  G_SIGNAL_RUN_LAST|G_SIGNAL_NO_RECURSE,
			  0,
			  NULL, NULL,
			  g_cclosure_marshal_VOID__DOUBLE,
			  G_TYPE_NONE,
			  1,
			  G_TYPE_DOUBLE);
	burner_track_data_cfg_signals [SOURCE_LOADED] = 
	    g_signal_new ("source_loaded",
			  G_TYPE_FROM_CLASS (klass),
			  G_SIGNAL_RUN_LAST|G_SIGNAL_NO_RECURSE,
			  0,
			  NULL, NULL,
			  g_cclosure_marshal_VOID__POINTER,
			  G_TYPE_NONE,
			  1,
			  G_TYPE_POINTER);

	burner_track_data_cfg_signals [ICON_CHANGED] = 
	    g_signal_new ("icon_changed",
			  G_TYPE_FROM_CLASS (klass),
			  G_SIGNAL_RUN_LAST|G_SIGNAL_NO_RECURSE,
			  0,
			  NULL, NULL,
			  g_cclosure_marshal_VOID__VOID,
			  G_TYPE_NONE,
			  0,
			  G_TYPE_NONE);
}

/**
 * burner_track_data_cfg_new:
 *
 * Creates a new #BurnerTrackDataCfg.
 *
 * Return value: a new #BurnerTrackDataCfg.
 **/

BurnerTrackDataCfg *
burner_track_data_cfg_new (void)
{
	return g_object_new (BURNER_TYPE_TRACK_DATA_CFG, NULL);
}
