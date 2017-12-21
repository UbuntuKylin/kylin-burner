/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/*
 * burner
 * Copyright (C) Philippe Rouquier 2007 <bonfire-app@wanadoo.fr>
 * 
 *  Burner is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 * 
 * burner is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with burner.  If not, write to:
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

#include "burner-units.h"

#include "burner-session-cfg.h"
#include "burner-tags.h"
#include "burner-track-stream-cfg.h"

#include "burner-utils.h"
#include "burner-video-tree-model.h"

#include "eggtreemultidnd.h"

typedef struct _BurnerVideoTreeModelPrivate BurnerVideoTreeModelPrivate;
struct _BurnerVideoTreeModelPrivate
{
	BurnerSessionCfg *session;

	GSList *gaps;

	guint stamp;
	GtkIconTheme *theme;
};

#define BURNER_VIDEO_TREE_MODEL_PRIVATE(o)  (G_TYPE_INSTANCE_GET_PRIVATE ((o), BURNER_TYPE_VIDEO_TREE_MODEL, BurnerVideoTreeModelPrivate))

static void
burner_video_tree_model_multi_drag_source_iface_init (gpointer g_iface, gpointer data);
static void
burner_video_tree_model_drag_source_iface_init (gpointer g_iface, gpointer data);
static void
burner_video_tree_model_drag_dest_iface_init (gpointer g_iface, gpointer data);
static void
burner_video_tree_model_iface_init (gpointer g_iface, gpointer data);

G_DEFINE_TYPE_WITH_CODE (BurnerVideoTreeModel,
			 burner_video_tree_model,
			 G_TYPE_OBJECT,
			 G_IMPLEMENT_INTERFACE (GTK_TYPE_TREE_MODEL,
					        burner_video_tree_model_iface_init)
			 G_IMPLEMENT_INTERFACE (GTK_TYPE_TREE_DRAG_DEST,
					        burner_video_tree_model_drag_dest_iface_init)
			 G_IMPLEMENT_INTERFACE (GTK_TYPE_TREE_DRAG_SOURCE,
					        burner_video_tree_model_drag_source_iface_init)
			 G_IMPLEMENT_INTERFACE (EGG_TYPE_TREE_MULTI_DRAG_SOURCE,
					        burner_video_tree_model_multi_drag_source_iface_init));

enum {
	BURNER_STREAM_ROW_NORMAL	= 0,
	BURNER_STREAM_ROW_GAP		= 1,
};

/**
 * This is mainly a list so the following functions are not implemented.
 * But we may need them for AUDIO models when we display GAPs
 */
static gboolean
burner_video_tree_model_iter_parent (GtkTreeModel *model,
				      GtkTreeIter *iter,
				      GtkTreeIter *child)
{
	return FALSE;
}

static gboolean
burner_video_tree_model_iter_nth_child (GtkTreeModel *model,
					 GtkTreeIter *iter,
					 GtkTreeIter *parent,
					 gint n)
{
	return FALSE;
}

static gint
burner_video_tree_model_iter_n_children (GtkTreeModel *model,
					  GtkTreeIter *iter)
{
	if (!iter) {
		guint num = 0;
		GSList *iter;
		GSList * tracks;
		BurnerVideoTreeModelPrivate *priv;

		priv = BURNER_VIDEO_TREE_MODEL_PRIVATE (model);

		/* This is a special case in which we return the number
		 * of rows that are in the model. */
		tracks = burner_burn_session_get_tracks (BURNER_BURN_SESSION (priv->session));
		for (iter = tracks; iter; iter = iter->next) {
			BurnerTrackStream *track;

			track = iter->data;
			num ++;

			if (burner_track_stream_get_gap (track) > 0)
				num ++;
		}

		return num;
	}

	return 0;
}

static gboolean
burner_video_tree_model_iter_has_child (GtkTreeModel *model,
					 GtkTreeIter *iter)
{
	return FALSE;
}

static gboolean
burner_video_tree_model_iter_children (GtkTreeModel *model,
				        GtkTreeIter *iter,
				        GtkTreeIter *parent)
{
	return FALSE;
}

static void
burner_video_tree_model_get_value (GtkTreeModel *model,
				    GtkTreeIter *iter,
				    gint column,
				    GValue *value)
{
	BurnerVideoTreeModelPrivate *priv;
	BurnerBurnResult result;
	BurnerStatus *status;
	BurnerTrack *track;
	const gchar *string;
	GdkPixbuf *pixbuf;
	GValue *value_tag;
	GSList *tracks;
	gchar *text;

	priv = BURNER_VIDEO_TREE_MODEL_PRIVATE (model);

	/* make sure that iter comes from us */
	g_return_if_fail (priv->stamp == iter->stamp);
	g_return_if_fail (iter->user_data != NULL);

	track = iter->user_data;
	if (!BURNER_IS_TRACK_STREAM (track))
		return;

	if (GPOINTER_TO_INT (iter->user_data2) == BURNER_STREAM_ROW_GAP) {
		switch (column) {
		case BURNER_VIDEO_TREE_MODEL_WEIGHT:
			g_value_init (value, PANGO_TYPE_STYLE);
			g_value_set_enum (value, PANGO_WEIGHT_BOLD);
			return;
		case BURNER_VIDEO_TREE_MODEL_STYLE:
			g_value_init (value, PANGO_TYPE_STYLE);
			g_value_set_enum (value, PANGO_STYLE_ITALIC);
			return;
		case BURNER_VIDEO_TREE_MODEL_NAME:
			g_value_init (value, G_TYPE_STRING);
			g_value_set_string (value, _("Pause"));
			break;
		case BURNER_VIDEO_TREE_MODEL_ICON_NAME:
			g_value_init (value, G_TYPE_STRING);
			g_value_set_string (value, GTK_STOCK_MEDIA_PAUSE);
			break;
		case BURNER_VIDEO_TREE_MODEL_EDITABLE:
		case BURNER_VIDEO_TREE_MODEL_SELECTABLE:
			g_value_init (value, G_TYPE_BOOLEAN);
			g_value_set_boolean (value, FALSE);
			break;
		case BURNER_VIDEO_TREE_MODEL_IS_GAP:
			g_value_init (value, G_TYPE_BOOLEAN);
			g_value_set_boolean (value, TRUE);
			break;

		case BURNER_VIDEO_TREE_MODEL_SIZE:
			g_value_init (value, G_TYPE_STRING);
			text = burner_units_get_time_string (burner_track_stream_get_gap (BURNER_TRACK_STREAM (track)), TRUE, FALSE);
			g_value_set_string (value, text);
			g_free (text);
			break;

		case BURNER_VIDEO_TREE_MODEL_INDEX:
		case BURNER_VIDEO_TREE_MODEL_ARTIST:
			g_value_init (value, G_TYPE_STRING);
			g_value_set_string (value, NULL);
			break;

		case BURNER_VIDEO_TREE_MODEL_THUMBNAIL:
		case BURNER_VIDEO_TREE_MODEL_INDEX_NUM:
		default:
			g_value_init (value, G_TYPE_INVALID);
			break;
		}

		return;
	}

	switch (column) {
	case BURNER_VIDEO_TREE_MODEL_WEIGHT:
		g_value_init (value, PANGO_TYPE_STYLE);
		g_value_set_enum (value, PANGO_WEIGHT_NORMAL);
		return;
	case BURNER_VIDEO_TREE_MODEL_STYLE:
		g_value_init (value, PANGO_TYPE_STYLE);
		g_value_set_enum (value, PANGO_STYLE_NORMAL);
		return;
	case BURNER_VIDEO_TREE_MODEL_NAME:
		g_value_init (value, G_TYPE_STRING);

		string = burner_track_tag_lookup_string (track, BURNER_TRACK_STREAM_TITLE_TAG);
		if (string) {
			g_value_set_string (value, string);
		}
		else {
			GFile *file;
			gchar *uri;
			gchar *name;
			gchar *unescaped;

			uri = burner_track_stream_get_source (BURNER_TRACK_STREAM (track), TRUE);
			unescaped = g_uri_unescape_string (uri, NULL);
			g_free (uri);

			file = g_file_new_for_uri (unescaped);
			g_free (unescaped);

			name = g_file_get_basename (file);
			g_object_unref (file);

			g_value_set_string (value, name);
			g_free (name);
		}

		return;

	case BURNER_VIDEO_TREE_MODEL_ARTIST:
		g_value_init (value, G_TYPE_STRING);

		string = burner_track_tag_lookup_string (track, BURNER_TRACK_STREAM_ARTIST_TAG);
		if (string) 
			g_value_set_string (value, string);

		return;

	case BURNER_VIDEO_TREE_MODEL_ICON_NAME:
		status = burner_status_new ();
		burner_track_get_status (track, status);
		g_value_init (value, G_TYPE_STRING);

		value_tag = NULL;
		result = burner_status_get_result (status);
		if (result == BURNER_BURN_NOT_READY || result == BURNER_BURN_RUNNING)
			g_value_set_string (value, "image-loading");
		else if (burner_track_tag_lookup (track, BURNER_TRACK_STREAM_MIME_TAG, &value_tag) == BURNER_BURN_OK)
			g_value_set_string (value, g_value_get_string (value_tag));
		else
			g_value_set_string (value, "image-missing");

		g_object_unref (status);
		return;

	case BURNER_VIDEO_TREE_MODEL_THUMBNAIL:
		g_value_init (value, GDK_TYPE_PIXBUF);

		status = burner_status_new ();
		burner_track_get_status (track, status);
		result = burner_status_get_result (status);

		if (result == BURNER_BURN_NOT_READY || result == BURNER_BURN_RUNNING)
			pixbuf = gtk_icon_theme_load_icon (priv->theme,
							   "image-loading",
							   48,
							   0,
							   NULL);
		else {
			value_tag = NULL;
			burner_track_tag_lookup (track,
						  BURNER_TRACK_STREAM_THUMBNAIL_TAG,
						  &value_tag);

			if (value_tag)
				pixbuf = g_value_dup_object (value_tag);
			else
				pixbuf = gtk_icon_theme_load_icon (priv->theme,
								   "image-missing",
								   48,
								   0,
								   NULL);
		}

		g_value_set_object (value, pixbuf);
		g_object_unref (pixbuf);

		g_object_unref (status);
		return;

	case BURNER_VIDEO_TREE_MODEL_SIZE:
		status = burner_status_new ();
		burner_track_get_status (track, status);

		g_value_init (value, G_TYPE_STRING);

		result = burner_status_get_result (status);
		if (result == BURNER_BURN_OK) {
			guint64 len = 0;

			burner_track_stream_get_length (BURNER_TRACK_STREAM (track), &len);
			len -= burner_track_stream_get_gap (BURNER_TRACK_STREAM (track));
			text = burner_units_get_time_string (len, TRUE, FALSE);
			g_value_set_string (value, text);
			g_free (text);
		}
		else
			g_value_set_string (value, _("(loading…)"));

		g_object_unref (status);
		return;

	case BURNER_VIDEO_TREE_MODEL_EDITABLE:
		g_value_init (value, G_TYPE_BOOLEAN);
		/* This can be used for gap lines */
		g_value_set_boolean (value, TRUE);
		//g_value_set_boolean (value, file->editable);
		return;

	case BURNER_VIDEO_TREE_MODEL_SELECTABLE:
		g_value_init (value, G_TYPE_BOOLEAN);
		/* This can be used for gap lines */
		g_value_set_boolean (value, TRUE);
		//g_value_set_boolean (value, file->editable);
		return;

	case BURNER_VIDEO_TREE_MODEL_IS_GAP:
		g_value_init (value, G_TYPE_BOOLEAN);
		g_value_set_boolean (value, FALSE);
		return;

	case BURNER_VIDEO_TREE_MODEL_INDEX:
		tracks = burner_burn_session_get_tracks (BURNER_BURN_SESSION (priv->session));
		g_value_init (value, G_TYPE_STRING);
		text = g_strdup_printf ("%02i", g_slist_index (tracks, track) + 1);
		g_value_set_string (value, text);
		g_free (text);
		return;

	case BURNER_VIDEO_TREE_MODEL_INDEX_NUM:
		tracks = burner_burn_session_get_tracks (BURNER_BURN_SESSION (priv->session));
		g_value_init (value, G_TYPE_UINT);
		g_value_set_uint (value, g_slist_index (tracks, track) + 1);
		return;

	default:
		break;
	}
}

GtkTreePath *
burner_video_tree_model_track_to_path (BurnerVideoTreeModel *self,
				        BurnerTrack *track_arg)
{
	BurnerVideoTreeModelPrivate *priv;
	GSList *tracks;
	gint nth = 0;

	if (!BURNER_IS_TRACK_STREAM (track_arg))
		return NULL;

	priv = BURNER_VIDEO_TREE_MODEL_PRIVATE (self);

	tracks = burner_burn_session_get_tracks (BURNER_BURN_SESSION (priv->session));
	for (; tracks; tracks = tracks->next) {
		BurnerTrackStream *track;

		track = tracks->data;
		if (track == BURNER_TRACK_STREAM (track_arg))
			break;

		nth ++;

		if (burner_track_stream_get_gap (track) > 0)
			nth ++;

	}

	return gtk_tree_path_new_from_indices (nth, -1);
}

static GtkTreePath *
burner_video_tree_model_get_path (GtkTreeModel *model,
				   GtkTreeIter *iter)
{
	BurnerVideoTreeModelPrivate *priv;
	GSList *tracks;
	gint nth = 0;

	priv = BURNER_VIDEO_TREE_MODEL_PRIVATE (model);

	/* make sure that iter comes from us */
	g_return_val_if_fail (priv->stamp == iter->stamp, NULL);
	g_return_val_if_fail (iter->user_data != NULL, NULL);

	tracks = burner_burn_session_get_tracks (BURNER_BURN_SESSION (priv->session));
	for (; tracks; tracks = tracks->next) {
		BurnerTrackStream *track;

		track = tracks->data;
		if (GPOINTER_TO_INT (iter->user_data2) == BURNER_STREAM_ROW_NORMAL
		&&  track == iter->user_data)
			break;

		nth ++;

		if (burner_track_stream_get_gap (track) > 0) {
			if (GPOINTER_TO_INT (iter->user_data2) == BURNER_STREAM_ROW_GAP
			&&  track == iter->user_data)
				break;

			nth ++;
		}
	}

	/* NOTE: there is only one single file without a name: root */
	return gtk_tree_path_new_from_indices (nth, -1);
}

BurnerTrack *
burner_video_tree_model_path_to_track (BurnerVideoTreeModel *self,
				        GtkTreePath *path)
{
	BurnerVideoTreeModelPrivate *priv;
	const gint *indices;
	GSList *tracks;
	guint depth;
	gint index;

	priv = BURNER_VIDEO_TREE_MODEL_PRIVATE (self);

	indices = gtk_tree_path_get_indices (path);
	depth = gtk_tree_path_get_depth (path);

	/* NOTE: it can happen that paths are depth 2 when there is DND but then
	 * only the first index is relevant. */
	if (depth > 2)
		return NULL;

	/* Whether it is a GAP or a NORMAL row is of no importance */
	tracks = burner_burn_session_get_tracks (BURNER_BURN_SESSION (priv->session));
	index = indices [0];
	for (; tracks; tracks = tracks->next) {
		BurnerTrackStream *track;

		track = tracks->data;
		if (index <= 0)
			return BURNER_TRACK (track);

		index --;

		if (burner_track_stream_get_gap (track) > 0) {
			if (index <= 0)
				return BURNER_TRACK (track);

				index --;
		}
	}

	return NULL;
}

static gboolean
burner_video_tree_model_get_iter (GtkTreeModel *model,
				   GtkTreeIter *iter,
				   GtkTreePath *path)
{
	BurnerVideoTreeModelPrivate *priv;
	const gint *indices;
	GSList *tracks;
	guint depth;
	gint index;

	priv = BURNER_VIDEO_TREE_MODEL_PRIVATE (model);

	depth = gtk_tree_path_get_depth (path);

	/* NOTE: it can happen that paths are depth 2 when there is DND but then
	 * only the first index is relevant. */
	if (depth > 2)
		return FALSE;

	/* Whether it is a GAP or a NORMAL row is of no importance */
	indices = gtk_tree_path_get_indices (path);
	index = indices [0];
	tracks = burner_burn_session_get_tracks (BURNER_BURN_SESSION (priv->session));
	for (; tracks; tracks = tracks->next) {
		BurnerTrackStream *track;

		track = tracks->data;
		if (index <= 0) {
			iter->stamp = priv->stamp;
			iter->user_data2 = GINT_TO_POINTER (BURNER_STREAM_ROW_NORMAL);
			iter->user_data = track;
			return TRUE;
		}
		index --;

		if (burner_track_stream_get_gap (track) > 0) {
			if (index <= 0) {
				iter->stamp = priv->stamp;
				iter->user_data2 = GINT_TO_POINTER (BURNER_STREAM_ROW_GAP);
				iter->user_data = track;
				return TRUE;
			}
			index --;
		}
	}

	return FALSE;
}

static BurnerTrack *
burner_video_tree_model_track_next (BurnerVideoTreeModel *model,
				     BurnerTrack *track)
{
	BurnerVideoTreeModelPrivate *priv;
	GSList *tracks;
	GSList *node;

	priv = BURNER_VIDEO_TREE_MODEL_PRIVATE (model);

	tracks = burner_burn_session_get_tracks (BURNER_BURN_SESSION (priv->session));
	node = g_slist_find (tracks, track);
	if (!node || !node->next)
		return NULL;

	return node->next->data;
}

static BurnerTrack *
burner_video_tree_model_track_previous (BurnerVideoTreeModel *model,
					 BurnerTrack *track)
{
	BurnerVideoTreeModelPrivate *priv;
	GSList *tracks;

	priv = BURNER_VIDEO_TREE_MODEL_PRIVATE (model);

	tracks = burner_burn_session_get_tracks (BURNER_BURN_SESSION (priv->session));
	while (tracks && tracks->next) {
		if (tracks->next->data == track)
			return tracks->data;

		tracks = tracks->next;
	}

	return NULL;
}

static gboolean
burner_video_tree_model_iter_next (GtkTreeModel *model,
				    GtkTreeIter *iter)
{
	BurnerVideoTreeModelPrivate *priv;
	BurnerTrackStream *track;
	GSList *tracks;
	GSList *node;

	priv = BURNER_VIDEO_TREE_MODEL_PRIVATE (model);

	/* make sure that iter comes from us */
	g_return_val_if_fail (priv->stamp == iter->stamp, FALSE);
	g_return_val_if_fail (iter->user_data != NULL, FALSE);

	track = BURNER_TRACK_STREAM (iter->user_data);
	if (!track)
		return FALSE;

	if (GPOINTER_TO_INT (iter->user_data2) == BURNER_STREAM_ROW_NORMAL
	&&  burner_track_stream_get_gap (track) > 0) {
		iter->user_data2 = GINT_TO_POINTER (BURNER_STREAM_ROW_GAP);
		return TRUE;
	}

	tracks = burner_burn_session_get_tracks (BURNER_BURN_SESSION (priv->session));
	node = g_slist_find (tracks, track);
	if (!node || !node->next)
		return FALSE;

	iter->user_data2 = GINT_TO_POINTER (BURNER_STREAM_ROW_NORMAL);
	iter->user_data = node->next->data;
	return TRUE;
}

static GType
burner_video_tree_model_get_column_type (GtkTreeModel *model,
					 gint index)
{
	switch (index) {
	case BURNER_VIDEO_TREE_MODEL_NAME:
		return G_TYPE_STRING;

	case BURNER_VIDEO_TREE_MODEL_ARTIST:
		return G_TYPE_STRING;

	case BURNER_VIDEO_TREE_MODEL_THUMBNAIL:
		return GDK_TYPE_PIXBUF;

	case BURNER_VIDEO_TREE_MODEL_ICON_NAME:
		return G_TYPE_STRING;

	case BURNER_VIDEO_TREE_MODEL_SIZE:
		return G_TYPE_STRING;

	case BURNER_VIDEO_TREE_MODEL_EDITABLE:
		return G_TYPE_BOOLEAN;

	case BURNER_VIDEO_TREE_MODEL_SELECTABLE:
		return G_TYPE_BOOLEAN;

	case BURNER_VIDEO_TREE_MODEL_INDEX:
		return G_TYPE_STRING;

	case BURNER_VIDEO_TREE_MODEL_INDEX_NUM:
		return G_TYPE_UINT;

	case BURNER_VIDEO_TREE_MODEL_IS_GAP:
		return G_TYPE_STRING;

	case BURNER_VIDEO_TREE_MODEL_WEIGHT:
		return PANGO_TYPE_WEIGHT;

	case BURNER_VIDEO_TREE_MODEL_STYLE:
		return PANGO_TYPE_STYLE;

	default:
		break;
	}

	return G_TYPE_INVALID;
}

static gint
burner_video_tree_model_get_n_columns (GtkTreeModel *model)
{
	return BURNER_VIDEO_TREE_MODEL_COL_NUM;
}

static GtkTreeModelFlags
burner_video_tree_model_get_flags (GtkTreeModel *model)
{
	return GTK_TREE_MODEL_LIST_ONLY;
}

static gboolean
burner_video_tree_model_multi_row_draggable (EggTreeMultiDragSource *drag_source,
					      GList *path_list)
{
	/* All rows are draggable so return TRUE */
	return TRUE;
}

static gboolean
burner_video_tree_model_multi_drag_data_get (EggTreeMultiDragSource *drag_source,
					      GList *path_list,
					      GtkSelectionData *selection_data)
{
	if (gtk_selection_data_get_target (selection_data) == gdk_atom_intern (BURNER_DND_TARGET_SELF_FILE_NODES, TRUE)) {
		BurnerDNDVideoContext context;

		context.model = GTK_TREE_MODEL (drag_source);
		context.references = path_list;

		gtk_selection_data_set (selection_data,
					gdk_atom_intern_static_string (BURNER_DND_TARGET_SELF_FILE_NODES),
					8,
					(void *) &context,
					sizeof (context));
	}
	else
		return FALSE;

	return TRUE;
}

static gboolean
burner_video_tree_model_multi_drag_data_delete (EggTreeMultiDragSource *drag_source,
						 GList *path_list)
{
	/* NOTE: it's not the data in the selection_data here that should be
	 * deleted but rather the rows selected when there is a move. FALSE
	 * here means that we didn't delete anything. */
	/* return TRUE to stop other handlers */
	return TRUE;
}

void
burner_video_tree_model_move_before (BurnerVideoTreeModel *self,
				      GtkTreeIter *iter,
				      GtkTreePath *dest_before)
{
	BurnerTrack *track;
	GtkTreeIter sibling;
	BurnerTrack *track_sibling;
	BurnerVideoTreeModelPrivate *priv;

	priv = BURNER_VIDEO_TREE_MODEL_PRIVATE (self);

	track = BURNER_TRACK (iter->user_data);
	if (!dest_before || !burner_video_tree_model_get_iter (GTK_TREE_MODEL (self), &sibling, dest_before)) {
		if (GPOINTER_TO_INT (iter->user_data2) == BURNER_STREAM_ROW_GAP) {
			guint64 gap;
			GSList *tracks;

			gap = burner_track_stream_get_gap (BURNER_TRACK_STREAM (track));
			burner_track_stream_set_boundaries (BURNER_TRACK_STREAM (track),
							     -1,
							     -1,
							     0);
			burner_track_changed (track);

			/* Get last track */
			tracks = burner_burn_session_get_tracks (BURNER_BURN_SESSION (priv->session));
			tracks = g_slist_last (tracks);
			track_sibling = tracks->data;

			gap += burner_track_stream_get_gap (BURNER_TRACK_STREAM (track_sibling));
			burner_track_stream_set_boundaries (BURNER_TRACK_STREAM (track_sibling),
							     -1,
							     -1,
							     gap);
			burner_track_changed (track_sibling);
			return;
		}

		burner_burn_session_move_track (BURNER_BURN_SESSION (priv->session),
						 track,
						 NULL);
		return;
	}

	track_sibling = BURNER_TRACK (sibling.user_data);

	if (GPOINTER_TO_INT (iter->user_data2) == BURNER_STREAM_ROW_GAP) {
		guint64 gap;
		BurnerTrack *previous_sibling;

		/* Merge the gaps or add it */
		gap = burner_track_stream_get_gap (BURNER_TRACK_STREAM (track));
		burner_track_stream_set_boundaries (BURNER_TRACK_STREAM (track),
						     -1,
						     -1,
						     0);
		burner_track_changed (track);

		if (GPOINTER_TO_INT (sibling.user_data2) == BURNER_STREAM_ROW_GAP) {
			gap += burner_track_stream_get_gap (BURNER_TRACK_STREAM (track_sibling));
			burner_track_stream_set_boundaries (BURNER_TRACK_STREAM (track_sibling),
							     -1,
							     -1,
							     gap);
			burner_track_changed (track_sibling);
			return;
		}

		/* get the track before track_sibling */
		previous_sibling = burner_video_tree_model_track_previous (self, track_sibling);
		if (previous_sibling)
			track_sibling = previous_sibling;

		gap += burner_track_stream_get_gap (BURNER_TRACK_STREAM (track_sibling));
		burner_track_stream_set_boundaries (BURNER_TRACK_STREAM (track_sibling),
						     -1,
						     -1,
						     gap);
		burner_track_changed (track_sibling);
		return;
	}

	if (GPOINTER_TO_INT (sibling.user_data2) == BURNER_STREAM_ROW_GAP) {
		guint64 gap;

		/* merge */
		gap = burner_track_stream_get_gap (BURNER_TRACK_STREAM (track_sibling));
		burner_track_stream_set_boundaries (BURNER_TRACK_STREAM (track_sibling),
						     -1,
						     -1,
						     0);
		burner_track_changed (track_sibling);

		gap += burner_track_stream_get_gap (BURNER_TRACK_STREAM (track));
		burner_track_stream_set_boundaries (BURNER_TRACK_STREAM (track),
						     -1,
						     -1,
						     gap);
		burner_track_changed (track);

		/* Track sibling is now the next track of current track_sibling */
		track_sibling = burner_video_tree_model_track_next (self, track_sibling);
	}

	burner_burn_session_move_track (BURNER_BURN_SESSION (priv->session),
					 track,
					 track_sibling);
}

static gboolean
burner_video_tree_model_drag_data_received (GtkTreeDragDest *drag_dest,
					     GtkTreePath *dest_path,
					     GtkSelectionData *selection_data)
{
	BurnerTrack *sibling;
	BurnerVideoTreeModelPrivate *priv;
	GdkAtom target;

	priv = BURNER_VIDEO_TREE_MODEL_PRIVATE (drag_dest);

	/* The new row(s) must be before dest_path but after our sibling */
	sibling = burner_video_tree_model_path_to_track (BURNER_VIDEO_TREE_MODEL (drag_dest), dest_path);

	/* Received data: see where it comes from:
	 * - from us, then that's a simple move
	 * - from another widget then it's going to be URIS and we add
	 *   them to VideoProject */
	target = gtk_selection_data_get_target (selection_data);
	if (target == gdk_atom_intern (BURNER_DND_TARGET_SELF_FILE_NODES, TRUE)) {
		BurnerDNDVideoContext *context;
		GtkTreeRowReference *dest;
		GList *iter;

		context = (BurnerDNDVideoContext *) gtk_selection_data_get_data (selection_data);
		if (context->model != GTK_TREE_MODEL (drag_dest))
			return TRUE;

		dest = gtk_tree_row_reference_new (GTK_TREE_MODEL (drag_dest), dest_path);
		/* That's us: move the row and its children. */
		for (iter = context->references; iter; iter = iter->next) {
			GtkTreeRowReference *reference;
			GtkTreePath *destination;
			GtkTreePath *treepath;
			GtkTreeIter tree_iter;

			reference = iter->data;
			treepath = gtk_tree_row_reference_get_path (reference);
			gtk_tree_model_get_iter (GTK_TREE_MODEL (drag_dest),
						 &tree_iter,
						 treepath);
			gtk_tree_path_free (treepath);

			destination = gtk_tree_row_reference_get_path (dest);
			burner_video_tree_model_move_before (BURNER_VIDEO_TREE_MODEL (drag_dest),
							      &tree_iter,
							      destination);
			gtk_tree_path_free (destination);
		}
		gtk_tree_row_reference_free (dest);
	}
	else if (target == gdk_atom_intern ("text/uri-list", TRUE)) {
		gint i;
		gchar **uris = NULL;

		/* NOTE: for some reason gdk_text_property_to_utf8_list_for_display ()
		 * fails with banshee DND URIs list when calling gtk_selection_data_get_uris ().
		 * so do like nautilus */

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
			BurnerTrackStreamCfg *track;

			/* Add the URIs to the project */
			track = burner_track_stream_cfg_new ();
			burner_track_stream_set_source (BURNER_TRACK_STREAM (track), uris [i]);
			burner_burn_session_add_track (BURNER_BURN_SESSION (priv->session),
							BURNER_TRACK (track),
							sibling);
		}
		g_strfreev (uris);
	}
	else
		return FALSE;

	return TRUE;
}

static gboolean
burner_video_tree_model_row_drop_possible (GtkTreeDragDest *drag_dest,
					    GtkTreePath *dest_path,
					    GtkSelectionData *selection_data)
{
	/* It's always possible */
	return TRUE;
}

static gboolean
burner_video_tree_model_drag_data_delete (GtkTreeDragSource *source,
					   GtkTreePath *treepath)
{
	return TRUE;
}

static void
burner_video_tree_model_reindex (BurnerVideoTreeModel *model,
				  BurnerBurnSession *session,
				  BurnerTrack *track_arg,
				  GtkTreeIter *iter,
				  GtkTreePath *path)
{
	GSList *tracks;
	BurnerVideoTreeModelPrivate *priv;

	priv = BURNER_VIDEO_TREE_MODEL_PRIVATE (model);

	/* tracks (including) after sibling need to be reindexed */
	tracks = burner_burn_session_get_tracks (session);
	tracks = g_slist_find (tracks, track_arg);
	if (!tracks)
		return;

	tracks = tracks->next;
	for (; tracks; tracks = tracks->next) {
		BurnerTrack *track;

		track = tracks->data;

		iter->stamp = priv->stamp;
		iter->user_data = track;
		iter->user_data2 = GINT_TO_POINTER (BURNER_STREAM_ROW_NORMAL);

		gtk_tree_path_next (path);
		gtk_tree_model_row_changed (GTK_TREE_MODEL (model),
					    path,
					    iter);

		/* skip gap rows */
		if (burner_track_stream_get_gap (BURNER_TRACK_STREAM (track)) > 0)
			gtk_tree_path_next (path);
	}
}

static void
burner_video_tree_model_track_added (BurnerBurnSession *session,
				      BurnerTrack *track,
				      BurnerVideoTreeModel *model)
{
	BurnerVideoTreeModelPrivate *priv;
	GtkTreePath *path;
	GtkTreeIter iter;

	if (!BURNER_IS_TRACK_STREAM (track))
		return;

	priv = BURNER_VIDEO_TREE_MODEL_PRIVATE (model);

	iter.stamp = priv->stamp;
	iter.user_data = track;
	iter.user_data2 = GINT_TO_POINTER (BURNER_STREAM_ROW_NORMAL);

	path = burner_video_tree_model_track_to_path (model, track);

	/* if the file is reloading (because of a file system change or because
	 * it was a file that was a tmp folder) then no need to signal an added
	 * signal but a changed one */
	gtk_tree_model_row_inserted (GTK_TREE_MODEL (model),
				     path,
				     &iter);

	if (burner_track_stream_get_gap (BURNER_TRACK_STREAM (track)) > 0) {
		priv->gaps = g_slist_prepend (priv->gaps, track);

		iter.user_data2 = GINT_TO_POINTER (BURNER_STREAM_ROW_GAP);
		gtk_tree_path_next (path);
		gtk_tree_model_row_inserted (GTK_TREE_MODEL (model),
					     path,
					     &iter);
	}

	/* tracks (including) after sibling need to be reindexed */
	burner_video_tree_model_reindex (model, session, track, &iter, path);
	gtk_tree_path_free (path);
}

static void
burner_video_tree_model_track_removed (BurnerBurnSession *session,
					BurnerTrack *track,
					guint former_location,
					BurnerVideoTreeModel *model)
{
	BurnerVideoTreeModelPrivate *priv;
	GtkTreePath *path;
	GtkTreeIter iter;

	if (!BURNER_IS_TRACK_STREAM (track))
		return;

	priv = BURNER_VIDEO_TREE_MODEL_PRIVATE (model);

	/* remove the file. */
	path = gtk_tree_path_new_from_indices (former_location, -1);
	gtk_tree_model_row_deleted (GTK_TREE_MODEL (model), path);

	if (burner_track_stream_get_gap (BURNER_TRACK_STREAM (track)) > 0) {
		priv->gaps = g_slist_remove (priv->gaps, track);
		gtk_tree_model_row_deleted (GTK_TREE_MODEL (model), path);
	}

	/* tracks (including) after former_location need to be reindexed */
	burner_video_tree_model_reindex (model, session, track, &iter, path);
	gtk_tree_path_free (path);
}

static void
burner_video_tree_model_track_changed (BurnerBurnSession *session,
					BurnerTrack *track,
					BurnerVideoTreeModel *model)
{
	BurnerVideoTreeModelPrivate *priv;
	GValue *value = NULL;
	GtkTreePath *path;
	GtkTreeIter iter;

	if (!BURNER_IS_TRACK_STREAM (track))
		return;

	priv = BURNER_VIDEO_TREE_MODEL_PRIVATE (model);

	/* scale the thumbnail */
	burner_track_tag_lookup (BURNER_TRACK (track),
				  BURNER_TRACK_STREAM_THUMBNAIL_TAG,
				  &value);
	if (value) {
		GdkPixbuf *scaled;
		GdkPixbuf *snapshot;

		snapshot = g_value_get_object (value);
		scaled = gdk_pixbuf_scale_simple (snapshot,
						  48 * gdk_pixbuf_get_width (snapshot) / gdk_pixbuf_get_height (snapshot),
						  48,
						  GDK_INTERP_BILINEAR);

		value = g_new0 (GValue, 1);
		g_value_init (value, GDK_TYPE_PIXBUF);
		g_value_set_object (value, scaled);
		burner_track_tag_add (track,
				       BURNER_TRACK_STREAM_THUMBNAIL_TAG,
				       value);
	}

	/* Get the iter for the file */
	iter.stamp = priv->stamp;
	iter.user_data2 = GINT_TO_POINTER (BURNER_STREAM_ROW_NORMAL);
	iter.user_data = track;

	path = burner_video_tree_model_track_to_path (model, track);
	gtk_tree_model_row_changed (GTK_TREE_MODEL (model),
				    path,
				    &iter);

	/* Get the iter for a possible gap row.
	 * The problem is to know whether one was added, removed or simply
	 * changed. */
	gtk_tree_path_next (path);
	if (burner_track_stream_get_gap (BURNER_TRACK_STREAM (track)) > 0) {
		iter.user_data2 = GINT_TO_POINTER (BURNER_STREAM_ROW_GAP);
		if (!g_slist_find (priv->gaps, track)) {
			priv->gaps = g_slist_prepend (priv->gaps,  track);
			gtk_tree_model_row_inserted (GTK_TREE_MODEL (model),
						     path,
						     &iter);
		}
		else
			gtk_tree_model_row_changed (GTK_TREE_MODEL (model),
						    path,
						    &iter);
	}
	else if (g_slist_find (priv->gaps, track)) {
		priv->gaps = g_slist_remove (priv->gaps, track);
		gtk_tree_model_row_deleted (GTK_TREE_MODEL (model), path);		
	}

	gtk_tree_path_free (path);
}

void
burner_video_tree_model_set_session (BurnerVideoTreeModel *model,
				      BurnerSessionCfg *session)
{
	BurnerVideoTreeModelPrivate *priv;

	priv = BURNER_VIDEO_TREE_MODEL_PRIVATE (model);
	if (priv->session) {
		g_signal_handlers_disconnect_by_func (priv->session,
						      burner_video_tree_model_track_added,
						      model);
		g_signal_handlers_disconnect_by_func (priv->session,
						      burner_video_tree_model_track_removed,
						      model);
		g_signal_handlers_disconnect_by_func (priv->session,
						      burner_video_tree_model_track_changed,
						      model);
		g_object_unref (priv->session);
		priv->session = NULL;
	}

	if (!session)
		return;

	priv->session = g_object_ref (session);
	g_signal_connect (session,
			  "track-added",
			  G_CALLBACK (burner_video_tree_model_track_added),
			  model);
	g_signal_connect (session,
			  "track-removed",
			  G_CALLBACK (burner_video_tree_model_track_removed),
			  model);
	g_signal_connect (session,
			  "track-changed",
			  G_CALLBACK (burner_video_tree_model_track_changed),
			  model);
}

BurnerSessionCfg *
burner_video_tree_model_get_session (BurnerVideoTreeModel *model)
{
	BurnerVideoTreeModelPrivate *priv;

	priv = BURNER_VIDEO_TREE_MODEL_PRIVATE (model);
	return priv->session;
}

static void
burner_video_tree_model_init (BurnerVideoTreeModel *object)
{
	BurnerVideoTreeModelPrivate *priv;

	priv = BURNER_VIDEO_TREE_MODEL_PRIVATE (object);

	priv->theme = gtk_icon_theme_get_default ();

	do {
		priv->stamp = g_random_int ();
	} while (!priv->stamp);
}

static void
burner_video_tree_model_finalize (GObject *object)
{
	BurnerVideoTreeModelPrivate *priv;

	priv = BURNER_VIDEO_TREE_MODEL_PRIVATE (object);

	if (priv->session) {
		g_signal_handlers_disconnect_by_func (priv->session,
						      burner_video_tree_model_track_added,
						      object);
		g_signal_handlers_disconnect_by_func (priv->session,
						      burner_video_tree_model_track_removed,
						      object);
		g_signal_handlers_disconnect_by_func (priv->session,
						      burner_video_tree_model_track_changed,
						      object);
		g_object_unref (priv->session);
		priv->session = NULL;
	}

	if (priv->gaps) {
		g_slist_free (priv->gaps);
		priv->gaps = NULL;
	}

	G_OBJECT_CLASS (burner_video_tree_model_parent_class)->finalize (object);
}

static void
burner_video_tree_model_iface_init (gpointer g_iface, gpointer data)
{
	GtkTreeModelIface *iface = g_iface;
	static gboolean initialized = FALSE;

	if (initialized)
		return;

	initialized = TRUE;

	iface->get_flags = burner_video_tree_model_get_flags;
	iface->get_n_columns = burner_video_tree_model_get_n_columns;
	iface->get_column_type = burner_video_tree_model_get_column_type;
	iface->get_iter = burner_video_tree_model_get_iter;
	iface->get_path = burner_video_tree_model_get_path;
	iface->get_value = burner_video_tree_model_get_value;
	iface->iter_next = burner_video_tree_model_iter_next;
	iface->iter_children = burner_video_tree_model_iter_children;
	iface->iter_has_child = burner_video_tree_model_iter_has_child;
	iface->iter_n_children = burner_video_tree_model_iter_n_children;
	iface->iter_nth_child = burner_video_tree_model_iter_nth_child;
	iface->iter_parent = burner_video_tree_model_iter_parent;
}

static void
burner_video_tree_model_multi_drag_source_iface_init (gpointer g_iface, gpointer data)
{
	EggTreeMultiDragSourceIface *iface = g_iface;
	static gboolean initialized = FALSE;

	if (initialized)
		return;

	initialized = TRUE;

	iface->row_draggable = burner_video_tree_model_multi_row_draggable;
	iface->drag_data_get = burner_video_tree_model_multi_drag_data_get;
	iface->drag_data_delete = burner_video_tree_model_multi_drag_data_delete;
}

static void
burner_video_tree_model_drag_source_iface_init (gpointer g_iface, gpointer data)
{
	GtkTreeDragSourceIface *iface = g_iface;
	static gboolean initialized = FALSE;

	if (initialized)
		return;

	initialized = TRUE;

	iface->drag_data_delete = burner_video_tree_model_drag_data_delete;
}

static void
burner_video_tree_model_drag_dest_iface_init (gpointer g_iface, gpointer data)
{
	GtkTreeDragDestIface *iface = g_iface;
	static gboolean initialized = FALSE;

	if (initialized)
		return;

	initialized = TRUE;

	iface->drag_data_received = burner_video_tree_model_drag_data_received;
	iface->row_drop_possible = burner_video_tree_model_row_drop_possible;
}

static void
burner_video_tree_model_class_init (BurnerVideoTreeModelClass *klass)
{
	GObjectClass* object_class = G_OBJECT_CLASS (klass);

	g_type_class_add_private (klass, sizeof (BurnerVideoTreeModelPrivate));

	object_class->finalize = burner_video_tree_model_finalize;
}

BurnerVideoTreeModel *
burner_video_tree_model_new (void)
{
	return g_object_new (BURNER_TYPE_VIDEO_TREE_MODEL, NULL);
}
