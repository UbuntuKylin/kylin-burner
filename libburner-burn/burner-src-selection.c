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

#include <glib/gi18n-lib.h>

#include <gtk/gtk.h>

#include "burner-src-selection.h"
#include "burner-medium-selection.h"

#include "burner-track.h"
#include "burner-session.h"
#include "burner-track-disc.h"

#include "burner-drive.h"
#include "burner-volume.h"

typedef struct _BurnerSrcSelectionPrivate BurnerSrcSelectionPrivate;
struct _BurnerSrcSelectionPrivate
{
	BurnerBurnSession *session;
	BurnerTrackDisc *track;
};

#define BURNER_SRC_SELECTION_PRIVATE(o)  (G_TYPE_INSTANCE_GET_PRIVATE ((o), BURNER_TYPE_SRC_SELECTION, BurnerSrcSelectionPrivate))

enum {
	PROP_0,
	PROP_SESSION
};

G_DEFINE_TYPE (BurnerSrcSelection, burner_src_selection, BURNER_TYPE_MEDIUM_SELECTION);

static void
burner_src_selection_medium_changed (BurnerMediumSelection *selection,
				      BurnerMedium *medium)
{
	BurnerSrcSelectionPrivate *priv;
	BurnerDrive *drive = NULL;

	priv = BURNER_SRC_SELECTION_PRIVATE (selection);

	if (priv->session && priv->track) {
		drive = burner_medium_get_drive (medium);
		burner_track_disc_set_drive (priv->track, drive);
	}

	gtk_widget_set_sensitive (GTK_WIDGET (selection), drive != NULL);

	if (BURNER_MEDIUM_SELECTION_CLASS (burner_src_selection_parent_class)->medium_changed)
		BURNER_MEDIUM_SELECTION_CLASS (burner_src_selection_parent_class)->medium_changed (selection, medium);
}

GtkWidget *
burner_src_selection_new (BurnerBurnSession *session)
{
	g_return_val_if_fail (BURNER_IS_BURN_SESSION (session), NULL);
	return GTK_WIDGET (g_object_new (BURNER_TYPE_SRC_SELECTION,
					 "session", session,
					 NULL));
}

static void
burner_src_selection_constructed (GObject *object)
{
	G_OBJECT_CLASS (burner_src_selection_parent_class)->constructed (object);

	/* only show media with something to be read on them */
	burner_medium_selection_show_media_type (BURNER_MEDIUM_SELECTION (object),
						  BURNER_MEDIA_TYPE_AUDIO|
						  BURNER_MEDIA_TYPE_DATA);
}

static void
burner_src_selection_init (BurnerSrcSelection *object)
{
}

static void
burner_src_selection_finalize (GObject *object)
{
	BurnerSrcSelectionPrivate *priv;

	priv = BURNER_SRC_SELECTION_PRIVATE (object);

	if (priv->session) {
		g_object_unref (priv->session);
		priv->session = NULL;
	}

	if (priv->track) {
		g_object_unref (priv->track);
		priv->track = NULL;
	}

	G_OBJECT_CLASS (burner_src_selection_parent_class)->finalize (object);
}

static BurnerTrack *
_get_session_disc_track (BurnerBurnSession *session)
{
	BurnerTrack *track;
	GSList *tracks;
	guint num;

	tracks = burner_burn_session_get_tracks (session);
	num = g_slist_length (tracks);

	if (num != 1)
		return NULL;

	track = tracks->data;
	if (BURNER_IS_TRACK_DISC (track))
		return track;

	return NULL;
}

static void
burner_src_selection_set_property (GObject *object,
				    guint property_id,
				    const GValue *value,
				    GParamSpec *pspec)
{
	BurnerSrcSelectionPrivate *priv;
	BurnerBurnSession *session;

	priv = BURNER_SRC_SELECTION_PRIVATE (object);

	switch (property_id) {
	case PROP_SESSION:
	{
		BurnerMedium *medium;
		BurnerDrive *drive;
		BurnerTrack *track;

		session = g_value_get_object (value);

		priv->session = session;
		g_object_ref (session);

		if (priv->track)
			g_object_unref (priv->track);

		/* See if there was a track set; if so then use it */
		track = _get_session_disc_track (session);
		if (track) {
			priv->track = BURNER_TRACK_DISC (track);
			g_object_ref (track);
		}
		else {
			priv->track = burner_track_disc_new ();
			burner_burn_session_add_track (priv->session,
							BURNER_TRACK (priv->track),
							NULL);
		}

		drive = burner_track_disc_get_drive (priv->track);
		medium = burner_drive_get_medium (drive);
		if (!medium) {
			/* No medium set use set session medium source as the
			 * one currently active in the selection widget */
			medium = burner_medium_selection_get_active (BURNER_MEDIUM_SELECTION (object));
			burner_src_selection_medium_changed (BURNER_MEDIUM_SELECTION (object), medium);
		}
		else
			burner_medium_selection_set_active (BURNER_MEDIUM_SELECTION (object), medium);

		break;
	}

	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
	}
}

static void
burner_src_selection_get_property (GObject *object,
				    guint property_id,
				    GValue *value,
				    GParamSpec *pspec)
{
	BurnerSrcSelectionPrivate *priv;

	priv = BURNER_SRC_SELECTION_PRIVATE (object);

	switch (property_id) {
	case PROP_SESSION:
		g_value_set_object (value, priv->session);
		break;

	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
	}
}

static void
burner_src_selection_class_init (BurnerSrcSelectionClass *klass)
{
	GObjectClass* object_class = G_OBJECT_CLASS (klass);
	BurnerMediumSelectionClass *medium_selection_class = BURNER_MEDIUM_SELECTION_CLASS (klass);

	g_type_class_add_private (klass, sizeof (BurnerSrcSelectionPrivate));

	object_class->finalize = burner_src_selection_finalize;
	object_class->set_property = burner_src_selection_set_property;
	object_class->get_property = burner_src_selection_get_property;
	object_class->constructed = burner_src_selection_constructed;

	medium_selection_class->medium_changed = burner_src_selection_medium_changed;

	g_object_class_install_property (object_class,
					 PROP_SESSION,
					 g_param_spec_object ("session",
							      "The session to work with",
							      "The session to work with",
							      BURNER_TYPE_BURN_SESSION,
							      G_PARAM_READWRITE|G_PARAM_CONSTRUCT_ONLY));
}
