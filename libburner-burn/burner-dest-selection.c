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

#include <glib.h>
#include <glib-object.h>
#include <glib/gi18n-lib.h>

#include <gtk/gtk.h>

#include "burn-basics.h"
#include "burn-plugin-manager.h"
#include "burner-medium-selection-priv.h"
#include "burner-session-helper.h"

#include "burner-dest-selection.h"

#include "burner-drive.h"
#include "burner-medium.h"
#include "burner-volume.h"

#include "burner-burn-lib.h"
#include "burner-tags.h"
#include "burner-track.h"
#include "burner-session.h"
#include "burner-session-cfg.h"

typedef struct _BurnerDestSelectionPrivate BurnerDestSelectionPrivate;
struct _BurnerDestSelectionPrivate
{
	BurnerBurnSession *session;

	BurnerDrive *locked_drive;

	guint user_changed:1;
};

#define BURNER_DEST_SELECTION_PRIVATE(o)  (G_TYPE_INSTANCE_GET_PRIVATE ((o), BURNER_TYPE_DEST_SELECTION, BurnerDestSelectionPrivate))

enum {
	PROP_0,
	PROP_SESSION
};

G_DEFINE_TYPE (BurnerDestSelection, burner_dest_selection, BURNER_TYPE_MEDIUM_SELECTION);

static void
burner_dest_selection_lock (BurnerDestSelection *self,
			     gboolean locked)
{
	BurnerDestSelectionPrivate *priv;

	priv = BURNER_DEST_SELECTION_PRIVATE (self);

	if (locked == (priv->locked_drive != NULL))
		return;

	gtk_widget_set_sensitive (GTK_WIDGET (self), (locked != TRUE));
	gtk_widget_queue_draw (GTK_WIDGET (self));

	if (priv->locked_drive) {
		burner_drive_unlock (priv->locked_drive);
		g_object_unref (priv->locked_drive);
		priv->locked_drive = NULL;
	}

	if (locked) {
		BurnerMedium *medium;

		medium = burner_medium_selection_get_active (BURNER_MEDIUM_SELECTION (self));
		priv->locked_drive = burner_medium_get_drive (medium);

		if (priv->locked_drive) {
			g_object_ref (priv->locked_drive);
			burner_drive_lock (priv->locked_drive,
					    _("Ongoing burning process"),
					    NULL);
		}

		if (medium)
			g_object_unref (medium);
	}
}

static void
burner_dest_selection_valid_session (BurnerSessionCfg *session,
				      BurnerDestSelection *self)
{
	burner_medium_selection_update_media_string (BURNER_MEDIUM_SELECTION (self));
}

static void
burner_dest_selection_output_changed (BurnerSessionCfg *session,
				       BurnerMedium *former,
				       BurnerDestSelection *self)
{
	BurnerDestSelectionPrivate *priv;
	BurnerMedium *medium;
	BurnerDrive *burner;

	priv = BURNER_DEST_SELECTION_PRIVATE (self);

	/* make sure the current displayed drive reflects that */
	burner = burner_burn_session_get_burner (priv->session);
	medium = burner_medium_selection_get_active (BURNER_MEDIUM_SELECTION (self));
	if (burner != burner_medium_get_drive (medium))
		burner_medium_selection_set_active (BURNER_MEDIUM_SELECTION (self),
						     burner_drive_get_medium (burner));

	if (medium)
		g_object_unref (medium);
}

static void
burner_dest_selection_flags_changed (BurnerBurnSession *session,
                                      GParamSpec *pspec,
				      BurnerDestSelection *self)
{
	BurnerDestSelectionPrivate *priv;

	priv = BURNER_DEST_SELECTION_PRIVATE (self);

	burner_dest_selection_lock (self, (burner_burn_session_get_flags (BURNER_BURN_SESSION (priv->session)) & BURNER_BURN_FLAG_MERGE) != 0);
}

static void
burner_dest_selection_medium_changed (BurnerMediumSelection *selection,
				       BurnerMedium *medium)
{
	BurnerDestSelectionPrivate *priv;

	priv = BURNER_DEST_SELECTION_PRIVATE (selection);

	if (!priv->session)
		goto chain;

	if (!medium) {
	    	gtk_widget_set_sensitive (GTK_WIDGET (selection), FALSE);
		goto chain;
	}

	if (burner_medium_get_drive (medium) == burner_burn_session_get_burner (priv->session))
		goto chain;

	if (priv->locked_drive && priv->locked_drive != burner_medium_get_drive (medium)) {
		burner_medium_selection_set_active (selection, medium);
		goto chain;
	}

	burner_burn_session_set_burner (priv->session, burner_medium_get_drive (medium));
	gtk_widget_set_sensitive (GTK_WIDGET (selection), (priv->locked_drive == NULL));

chain:

	if (BURNER_MEDIUM_SELECTION_CLASS (burner_dest_selection_parent_class)->medium_changed)
		BURNER_MEDIUM_SELECTION_CLASS (burner_dest_selection_parent_class)->medium_changed (selection, medium);
}

static void
burner_dest_selection_user_change (BurnerDestSelection *selection,
                                    GParamSpec *pspec,
                                    gpointer NULL_data)
{
	gboolean shown = FALSE;
	BurnerDestSelectionPrivate *priv;

	/* we are only interested when the menu is shown */
	g_object_get (selection,
	              "popup-shown", &shown,
	              NULL);

	if (!shown)
		return;

	priv = BURNER_DEST_SELECTION_PRIVATE (selection);
	priv->user_changed = TRUE;
}

static void
burner_dest_selection_medium_removed (GtkTreeModel *model,
                                       GtkTreePath *path,
                                       gpointer user_data)
{
	BurnerDestSelectionPrivate *priv;

	priv = BURNER_DEST_SELECTION_PRIVATE (user_data);
	if (priv->user_changed)
		return;

	if (gtk_combo_box_get_active (GTK_COMBO_BOX (user_data)) == -1)
		burner_dest_selection_choose_best (BURNER_DEST_SELECTION (user_data));
}

static void
burner_dest_selection_medium_added (GtkTreeModel *model,
                                     GtkTreePath *path,
                                     GtkTreeIter *iter,
                                     gpointer user_data)
{
	BurnerDestSelectionPrivate *priv;

	priv = BURNER_DEST_SELECTION_PRIVATE (user_data);
	if (priv->user_changed)
		return;

	burner_dest_selection_choose_best (BURNER_DEST_SELECTION (user_data));
}

static void
burner_dest_selection_constructed (GObject *object)
{
	G_OBJECT_CLASS (burner_dest_selection_parent_class)->constructed (object);

	/* Only show media on which we can write and which are in a burner.
	 * There is one exception though, when we're copying media and when the
	 * burning device is the same as the dest device. */
	burner_medium_selection_show_media_type (BURNER_MEDIUM_SELECTION (object),
						  BURNER_MEDIA_TYPE_WRITABLE|
						  BURNER_MEDIA_TYPE_FILE);
}

static void
burner_dest_selection_init (BurnerDestSelection *object)
{
	GtkTreeModel *model;

	model = gtk_combo_box_get_model (GTK_COMBO_BOX (object));
	g_signal_connect (model,
	                  "row-inserted",
	                  G_CALLBACK (burner_dest_selection_medium_added),
	                  object);
	g_signal_connect (model,
	                  "row-deleted",
	                  G_CALLBACK (burner_dest_selection_medium_removed),
	                  object);

	/* This is to know when the user changed it on purpose */
	g_signal_connect (object,
	                  "notify::popup-shown",
	                  G_CALLBACK (burner_dest_selection_user_change),
	                  NULL);
}

static void
burner_dest_selection_clean (BurnerDestSelection *self)
{
	BurnerDestSelectionPrivate *priv;

	priv = BURNER_DEST_SELECTION_PRIVATE (self);

	if (priv->session) {
		g_signal_handlers_disconnect_by_func (priv->session,
						      burner_dest_selection_valid_session,
						      self);
		g_signal_handlers_disconnect_by_func (priv->session,
						      burner_dest_selection_output_changed,
						      self);
		g_signal_handlers_disconnect_by_func (priv->session,
						      burner_dest_selection_flags_changed,
						      self);

		g_object_unref (priv->session);
		priv->session = NULL;
	}

	if (priv->locked_drive) {
		burner_drive_unlock (priv->locked_drive);
		g_object_unref (priv->locked_drive);
		priv->locked_drive = NULL;
	}
}

static void
burner_dest_selection_finalize (GObject *object)
{
	burner_dest_selection_clean (BURNER_DEST_SELECTION (object));
	G_OBJECT_CLASS (burner_dest_selection_parent_class)->finalize (object);
}

static goffset
_get_medium_free_space (BurnerMedium *medium,
                        goffset session_blocks)
{
	BurnerMedia media;
	goffset blocks = 0;

	media = burner_medium_get_status (medium);
	media = burner_burn_library_get_media_capabilities (media);

	/* NOTE: we always try to blank a medium when we can */
	burner_medium_get_free_space (medium,
				       NULL,
				       &blocks);

	if ((media & BURNER_MEDIUM_REWRITABLE)
	&& blocks < session_blocks)
		burner_medium_get_capacity (medium,
		                             NULL,
		                             &blocks);

	return blocks;
}

static gboolean
burner_dest_selection_foreach_medium (BurnerMedium *medium,
				       gpointer callback_data)
{
	BurnerBurnSession *session;
	goffset session_blocks = 0;
	goffset burner_blocks = 0;
	goffset medium_blocks;
	BurnerDrive *burner;

	session = callback_data;
	burner = burner_burn_session_get_burner (session);
	if (!burner) {
		burner_burn_session_set_burner (session, burner_medium_get_drive (medium));
		return TRUE;
	}

	/* no need to deal with this case */
	if (burner_drive_get_medium (burner) == medium)
		return TRUE;

	/* The rule is:
	 * - blank media are our favourite since it avoids hiding/blanking data
	 * - take the medium that is closest to the size we need to burn
	 * - try to avoid a medium that is already our source for copying */
	/* NOTE: we could check if medium is bigger */
	if ((burner_burn_session_get_dest_media (session) & BURNER_MEDIUM_BLANK)
	&&  (burner_medium_get_status (medium) & BURNER_MEDIUM_BLANK))
		goto choose_closest_size;

	if (burner_burn_session_get_dest_media (session) & BURNER_MEDIUM_BLANK)
		return TRUE;

	if (burner_medium_get_status (medium) & BURNER_MEDIUM_BLANK) {
		burner_burn_session_set_burner (session, burner_medium_get_drive (medium));
		return TRUE;
	}

	/* In case it is the same source/same destination, choose it this new
	 * medium except if the medium is a file. */
	if (burner_burn_session_same_src_dest_drive (session)
	&& (burner_medium_get_status (medium) & BURNER_MEDIUM_FILE) == 0) {
		burner_burn_session_set_burner (session, burner_medium_get_drive (medium));
		return TRUE;
	}

	/* Any possible medium is better than file even if it means copying to
	 * the same drive with a new medium later. */
	if (burner_drive_is_fake (burner)
	&& (burner_medium_get_status (medium) & BURNER_MEDIUM_FILE) == 0) {
		burner_burn_session_set_burner (session, burner_medium_get_drive (medium));
		return TRUE;
	}


choose_closest_size:

	burner_burn_session_get_size (session, &session_blocks, NULL);
	medium_blocks = _get_medium_free_space (medium, session_blocks);

	if (medium_blocks - session_blocks <= 0)
		return TRUE;

	burner_blocks = _get_medium_free_space (burner_drive_get_medium (burner), session_blocks);
	if (burner_blocks - session_blocks <= 0)
		burner_burn_session_set_burner (session, burner_medium_get_drive (medium));
	else if (burner_blocks - session_blocks > medium_blocks - session_blocks)
		burner_burn_session_set_burner (session, burner_medium_get_drive (medium));

	return TRUE;
}

void
burner_dest_selection_choose_best (BurnerDestSelection *self)
{
	BurnerDestSelectionPrivate *priv;

	priv = BURNER_DEST_SELECTION_PRIVATE (self);

	priv->user_changed = FALSE;
	if (!priv->session)
		return;

	if (!(burner_burn_session_get_flags (priv->session) & BURNER_BURN_FLAG_MERGE)) {
		BurnerDrive *drive;

		/* Select the best fitting media */
		burner_medium_selection_foreach (BURNER_MEDIUM_SELECTION (self),
						  burner_dest_selection_foreach_medium,
						  priv->session);

		drive = burner_burn_session_get_burner (BURNER_BURN_SESSION (priv->session));
		if (drive)
			burner_medium_selection_set_active (BURNER_MEDIUM_SELECTION (self),
							     burner_drive_get_medium (drive));
	}
}

void
burner_dest_selection_set_session (BurnerDestSelection *selection,
				    BurnerBurnSession *session)
{
	BurnerDestSelectionPrivate *priv;

	priv = BURNER_DEST_SELECTION_PRIVATE (selection);

	if (priv->session)
		burner_dest_selection_clean (selection);

	if (!session)
		return;

	priv->session = g_object_ref (session);
	if (burner_burn_session_get_flags (session) & BURNER_BURN_FLAG_MERGE) {
		BurnerDrive *drive;

		/* Prevent automatic resetting since a drive was set */
		priv->user_changed = TRUE;

		drive = burner_burn_session_get_burner (session);
		burner_medium_selection_set_active (BURNER_MEDIUM_SELECTION (selection),
						     burner_drive_get_medium (drive));
	}
	else {
		BurnerDrive *burner;

		/* Only try to set a better drive if there isn't one already set */
		burner = burner_burn_session_get_burner (BURNER_BURN_SESSION (priv->session));
		if (burner) {
			BurnerMedium *medium;

			/* Prevent automatic resetting since a drive was set */
			priv->user_changed = TRUE;

			medium = burner_drive_get_medium (burner);
			burner_medium_selection_set_active (BURNER_MEDIUM_SELECTION (selection), medium);
		}
		else
			burner_dest_selection_choose_best (BURNER_DEST_SELECTION (selection));
	}

	g_signal_connect (session,
			  "is-valid",
			  G_CALLBACK (burner_dest_selection_valid_session),
			  selection);
	g_signal_connect (session,
			  "output-changed",
			  G_CALLBACK (burner_dest_selection_output_changed),
			  selection);
	g_signal_connect (session,
			  "notify::flags",
			  G_CALLBACK (burner_dest_selection_flags_changed),
			  selection);

	burner_medium_selection_update_media_string (BURNER_MEDIUM_SELECTION (selection));
}

static void
burner_dest_selection_set_property (GObject *object,
				     guint property_id,
				     const GValue *value,
				     GParamSpec *pspec)
{
	BurnerBurnSession *session;

	switch (property_id) {
	case PROP_SESSION: /* Readable and only writable at creation time */
		/* NOTE: no need to unref a potential previous session since
		 * it's only set at construct time */
		session = g_value_get_object (value);
		burner_dest_selection_set_session (BURNER_DEST_SELECTION (object), session);
		break;

	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
	}
}

static void
burner_dest_selection_get_property (GObject *object,
				     guint property_id,
				     GValue *value,
				     GParamSpec *pspec)
{
	BurnerDestSelectionPrivate *priv;

	priv = BURNER_DEST_SELECTION_PRIVATE (object);

	switch (property_id) {
	case PROP_SESSION:
		g_object_ref (priv->session);
		g_value_set_object (value, priv->session);
		break;

	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
	}
}

static gchar *
burner_dest_selection_get_output_path (BurnerDestSelection *self)
{
	gchar *path = NULL;
	BurnerImageFormat format;
	BurnerDestSelectionPrivate *priv;

	priv = BURNER_DEST_SELECTION_PRIVATE (self);

	format = burner_burn_session_get_output_format (priv->session);
	switch (format) {
	case BURNER_IMAGE_FORMAT_BIN:
		burner_burn_session_get_output (priv->session,
						 &path,
						 NULL);
		break;

	case BURNER_IMAGE_FORMAT_CLONE:
	case BURNER_IMAGE_FORMAT_CDRDAO:
	case BURNER_IMAGE_FORMAT_CUE:
		burner_burn_session_get_output (priv->session,
						 NULL,
						 &path);
		break;

	default:
		break;
	}

	return path;
}

static gchar *
burner_dest_selection_format_medium_string (BurnerMediumSelection *selection,
					     BurnerMedium *medium)
{
	guint used;
	gchar *label;
	goffset blocks = 0;
	gchar *medium_name;
	gchar *size_string;
	BurnerMedia media;
	BurnerBurnFlag flags;
	goffset size_bytes = 0;
	goffset data_blocks = 0;
	goffset session_bytes = 0;
	BurnerTrackType *input = NULL;
	BurnerDestSelectionPrivate *priv;

	priv = BURNER_DEST_SELECTION_PRIVATE (selection);

	if (!priv->session)
		return NULL;

	medium_name = burner_volume_get_name (BURNER_VOLUME (medium));
	if (burner_medium_get_status (medium) & BURNER_MEDIUM_FILE) {
		gchar *path;

		input = burner_track_type_new ();
		burner_burn_session_get_input_type (priv->session, input);

		/* There should be a special name for image in video context */
		if (burner_track_type_get_has_stream (input)
		&&  BURNER_STREAM_FORMAT_HAS_VIDEO (burner_track_type_get_stream_format (input))) {
			BurnerImageFormat format;

			format = burner_burn_session_get_output_format (priv->session);
			if (format == BURNER_IMAGE_FORMAT_CUE) {
				g_free (medium_name);
				if (burner_burn_session_tag_lookup_int (priv->session, BURNER_VCD_TYPE) == BURNER_SVCD)
					medium_name = g_strdup (_("SVCD image"));
				else
					medium_name = g_strdup (_("VCD image"));
			}
			else if (format == BURNER_IMAGE_FORMAT_BIN) {
				g_free (medium_name);
				medium_name = g_strdup (_("Video DVD image"));
			}
		}
		burner_track_type_free (input);

		/* get the set path for the image file */
		path = burner_dest_selection_get_output_path (BURNER_DEST_SELECTION (selection));
		if (!path)
			return medium_name;

		/* NOTE for translators: the first %s is medium_name ("File
		 * Image") and the second the path for the image file */
		label = g_strdup_printf (_("%s: \"%s\""),
					 medium_name,
					 path);
		g_free (medium_name);
		g_free (path);

		burner_medium_selection_update_used_space (BURNER_MEDIUM_SELECTION (selection),
							    medium,
							    0);
		return label;
	}

	if (!priv->session) {
		g_free (medium_name);
		return NULL;
	}

	input = burner_track_type_new ();
	burner_burn_session_get_input_type (priv->session, input);
	if (burner_track_type_get_has_medium (input)) {
		BurnerMedium *src_medium;

		src_medium = burner_burn_session_get_src_medium (priv->session);
		if (src_medium == medium) {
			burner_track_type_free (input);

			/* Translators: this string is only used when the user
			 * wants to copy a disc using the same destination and
			 * source drive. It tells him that burner will use as
			 * destination disc a new one (once the source has been
			 * copied) which is to be inserted in the drive currently
			 * holding the source disc */
			label = g_strdup_printf (_("New disc in the burner holding the source disc"));
			g_free (medium_name);

			burner_medium_selection_update_used_space (BURNER_MEDIUM_SELECTION (selection),
								    medium,
								    0);
			return label;
		}
	}

	media = burner_medium_get_status (medium);
	flags = burner_burn_session_get_flags (priv->session);
	burner_burn_session_get_size (priv->session,
				       &data_blocks,
				       &session_bytes);

	if (flags & (BURNER_BURN_FLAG_MERGE|BURNER_BURN_FLAG_APPEND))
		burner_medium_get_free_space (medium, &size_bytes, &blocks);
	else if (burner_burn_library_get_media_capabilities (media) & BURNER_MEDIUM_REWRITABLE)
		burner_medium_get_capacity (medium, &size_bytes, &blocks);
	else
		burner_medium_get_free_space (medium, &size_bytes, &blocks);

	if (blocks) {
		used = data_blocks * 100 / blocks;
		if (data_blocks && !used)
			used = 1;

		used = MIN (100, used);
	}
	else
		used = 0;

	burner_medium_selection_update_used_space (BURNER_MEDIUM_SELECTION (selection),
						    medium,
						    used);
	blocks -= data_blocks;
	if (blocks <= 0) {
		burner_track_type_free (input);

		/* NOTE for translators, the first %s is the medium name */
		label = g_strdup_printf (_("%s: not enough free space"), medium_name);
		g_free (medium_name);
		return label;
	}

	/* format the size */
	if (burner_track_type_get_has_stream (input)
	&& BURNER_STREAM_FORMAT_HAS_VIDEO (burner_track_type_get_stream_format (input))) {
		guint64 free_time;

		/* This is an embarassing problem: this is an approximation
		 * based on the fact that 2 hours = 4.3GiB */
		free_time = size_bytes - session_bytes;
		free_time = free_time * 72000LL / 47LL;
		size_string = burner_units_get_time_string (free_time,
							     TRUE,
							     TRUE);
	}
	else if (burner_track_type_get_has_stream (input)
	|| (burner_track_type_get_has_medium (input)
	&& (burner_track_type_get_medium_type (input) & BURNER_MEDIUM_HAS_AUDIO)))
		size_string = burner_units_get_time_string (BURNER_SECTORS_TO_DURATION (blocks),
							     TRUE,
							     TRUE);
	else
		size_string = g_format_size (size_bytes - session_bytes);

	burner_track_type_free (input);

	/* NOTE for translators: the first %s is the medium name, the second %s
	 * is its available free space. "Free" here is the free space available. */
	label = g_strdup_printf (_("%s: %s of free space"), medium_name, size_string);
	g_free (medium_name);
	g_free (size_string);

	return label;
}

static void
burner_dest_selection_class_init (BurnerDestSelectionClass *klass)
{
	GObjectClass* object_class = G_OBJECT_CLASS (klass);
	BurnerMediumSelectionClass *medium_selection_class = BURNER_MEDIUM_SELECTION_CLASS (klass);

	g_type_class_add_private (klass, sizeof (BurnerDestSelectionPrivate));

	object_class->finalize = burner_dest_selection_finalize;
	object_class->set_property = burner_dest_selection_set_property;
	object_class->get_property = burner_dest_selection_get_property;
	object_class->constructed = burner_dest_selection_constructed;

	medium_selection_class->format_medium_string = burner_dest_selection_format_medium_string;
	medium_selection_class->medium_changed = burner_dest_selection_medium_changed;
	g_object_class_install_property (object_class,
					 PROP_SESSION,
					 g_param_spec_object ("session",
							      "The session",
							      "The session to work with",
							      BURNER_TYPE_BURN_SESSION,
							      G_PARAM_READWRITE));
}

GtkWidget *
burner_dest_selection_new (BurnerBurnSession *session)
{
	return g_object_new (BURNER_TYPE_DEST_SELECTION,
			     "session", session,
			     NULL);
}
