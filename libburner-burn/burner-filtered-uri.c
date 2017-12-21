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
#include <glib/gi18n-lib.h>

#include <gtk/gtk.h>

#include "burner-misc.h"
#include "burner-filtered-uri.h"
#include "burner-track-data-cfg.h"


G_DEFINE_TYPE (BurnerFilteredUri, burner_filtered_uri, GTK_TYPE_LIST_STORE);

typedef struct _BurnerFilteredUriPrivate BurnerFilteredUriPrivate;
struct _BurnerFilteredUriPrivate
{
	/* This keeps a record of all URIs that have been restored by
	 * the user despite the filtering rules. */
	GHashTable *restored;
};

#define BURNER_FILTERED_URI_PRIVATE(o)  (G_TYPE_INSTANCE_GET_PRIVATE ((o), BURNER_TYPE_FILTERED_URI, BurnerFilteredUriPrivate))

static const gchar *labels [] = { N_("Hidden file"),
				  N_("Unreadable file"),
				  N_("Broken symbolic link"),
				  N_("Recursive symbolic link"),
				  NULL };

GSList *
burner_filtered_uri_get_restored_list (BurnerFilteredUri *filtered)
{
	BurnerFilteredUriPrivate *priv;
	GSList *retval = NULL;
	GHashTableIter iter;
	gpointer key;

	priv = BURNER_FILTERED_URI_PRIVATE (filtered);

	g_hash_table_iter_init (&iter, priv->restored);
	while (g_hash_table_iter_next (&iter, &key, NULL))
		retval = g_slist_prepend (retval, g_strdup (key));

	return retval;
}

BurnerFilterStatus
burner_filtered_uri_lookup_restored (BurnerFilteredUri *filtered,
				      const gchar *uri)
{
	BurnerFilteredUriPrivate *priv;

	priv = BURNER_FILTERED_URI_PRIVATE (filtered);
	/* See if this file is already in filtered */
	return GPOINTER_TO_INT (g_hash_table_lookup (priv->restored, uri));
}

void
burner_filtered_uri_dont_filter (BurnerFilteredUri *filtered,
				  const gchar *uri)
{
	BurnerFilteredUriPrivate *priv;

	priv = BURNER_FILTERED_URI_PRIVATE (filtered);
	uri = burner_utils_register_string (uri);
	g_hash_table_insert (priv->restored,
			     (gchar *) uri,
			     GINT_TO_POINTER (1));
}
				       
void
burner_filtered_uri_filter (BurnerFilteredUri *filtered,
			     const gchar *uri,
			     BurnerFilterStatus status)
{
	GtkTreeIter iter;
	gboolean fatal;

	gtk_list_store_append (GTK_LIST_STORE (filtered), &iter);
	fatal = (status != BURNER_FILTER_HIDDEN && status != BURNER_FILTER_BROKEN_SYM);

	gtk_list_store_set (GTK_LIST_STORE (filtered), &iter,
			    BURNER_FILTERED_STOCK_ID_COL, fatal ? GTK_STOCK_CANCEL:NULL,
			    BURNER_FILTERED_URI_COL, uri,
			    BURNER_FILTERED_STATUS_COL, _(labels [status - 1]),
			    BURNER_FILTERED_FATAL_ERROR_COL, fatal == FALSE,
			    -1);
}

void
burner_filtered_uri_remove_with_children (BurnerFilteredUri *filtered,
					   const gchar *uri)
{
	BurnerFilteredUriPrivate *priv;
	GtkTreeIter row_iter;
	GHashTableIter iter;
	gpointer value;
	gpointer key;

	priv = BURNER_FILTERED_URI_PRIVATE (filtered);

	g_hash_table_iter_init (&iter, priv->restored);
	while (g_hash_table_iter_next (&iter, &key, &value)) {
		gchar *key_uri;
		gint len;

		key_uri = key;
		len = strlen (key_uri);

		if (!strncmp (uri, key_uri, len)
		&&   key_uri [len] == G_DIR_SEPARATOR) {
			burner_utils_unregister_string (key);
			g_hash_table_iter_remove (&iter);
		}
	}

	if (!gtk_tree_model_get_iter_first (GTK_TREE_MODEL (filtered), &row_iter))
		return;

	do {
		gchar *row_uri;
		gint len;

		gtk_tree_model_get (GTK_TREE_MODEL (filtered), &row_iter,
				    BURNER_FILTERED_URI_COL, &row_uri,
				    -1);

		len = strlen (row_uri);

		if (!strncmp (uri, row_uri, len)
		&&   row_uri [len] == G_DIR_SEPARATOR) {
			gboolean res;

			res = gtk_list_store_remove (GTK_LIST_STORE (filtered), &row_iter);
			if (!res) {
				g_free (row_uri);
				break;
			}
		}
		g_free (row_uri);

	} while (gtk_tree_model_iter_next (GTK_TREE_MODEL (filtered), &row_iter));
}

gchar *
burner_filtered_uri_restore (BurnerFilteredUri *filtered,
			      GtkTreePath *treepath)
{
	BurnerFilteredUriPrivate *priv;
	GtkTreeIter iter;
	guint value;
	gchar *uri;

	priv = BURNER_FILTERED_URI_PRIVATE (filtered);

	if (!gtk_tree_model_get_iter (GTK_TREE_MODEL (filtered), &iter, treepath))
		return NULL;

	gtk_tree_model_get (GTK_TREE_MODEL (filtered), &iter,
			    BURNER_FILTERED_URI_COL, &uri,
			    -1);

	gtk_list_store_remove (GTK_LIST_STORE (filtered), &iter);

	value = GPOINTER_TO_INT (g_hash_table_lookup (priv->restored, uri));
	if (!value)
		g_hash_table_insert (priv->restored,
				     (gchar *) burner_utils_register_string (uri),
				     GINT_TO_POINTER (1));
	return uri;
}

void
burner_filtered_uri_clear (BurnerFilteredUri *filtered)
{
	BurnerFilteredUriPrivate *priv;
	GHashTableIter iter;
	gpointer key;

	priv = BURNER_FILTERED_URI_PRIVATE (filtered);

	g_hash_table_iter_init (&iter, priv->restored);
	while (g_hash_table_iter_next (&iter, &key, NULL)) {
		burner_utils_unregister_string (key);
		g_hash_table_iter_remove (&iter);
	}

	gtk_list_store_clear (GTK_LIST_STORE (filtered));
}

static void
burner_filtered_uri_init (BurnerFilteredUri *object)
{
	BurnerFilteredUriPrivate *priv;
	GType types [BURNER_FILTERED_NB_COL] = { 0, };

	priv = BURNER_FILTERED_URI_PRIVATE (object);

	priv->restored = g_hash_table_new (g_str_hash, g_str_equal);

	types [0] = G_TYPE_STRING;
	types [1] = G_TYPE_STRING;
	types [2] = G_TYPE_STRING;
	types [3] = G_TYPE_BOOLEAN;
	gtk_list_store_set_column_types (GTK_LIST_STORE (object),
					 BURNER_FILTERED_NB_COL,
					 types);
}

static void
burner_filtered_uri_finalize (GObject *object)
{
	BurnerFilteredUriPrivate *priv;

	priv = BURNER_FILTERED_URI_PRIVATE (object);

	burner_filtered_uri_clear (BURNER_FILTERED_URI (object));

	if (priv->restored) {
		g_hash_table_destroy (priv->restored);
		priv->restored = NULL;
	}

	G_OBJECT_CLASS (burner_filtered_uri_parent_class)->finalize (object);
}

static void
burner_filtered_uri_class_init (BurnerFilteredUriClass *klass)
{
	GObjectClass* object_class = G_OBJECT_CLASS (klass);

	g_type_class_add_private (klass, sizeof (BurnerFilteredUriPrivate));

	object_class->finalize = burner_filtered_uri_finalize;
}

BurnerFilteredUri *
burner_filtered_uri_new ()
{
	return g_object_new (BURNER_TYPE_FILTERED_URI, NULL);
}
