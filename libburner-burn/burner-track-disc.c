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
#include <glib-object.h>

#include "burner-track-disc.h"

typedef struct _BurnerTrackDiscPrivate BurnerTrackDiscPrivate;
struct _BurnerTrackDiscPrivate
{
	BurnerDrive *drive;

	guint track_num;

	glong src_removed_sig;
	glong src_added_sig;
};

#define BURNER_TRACK_DISC_PRIVATE(o)  (G_TYPE_INSTANCE_GET_PRIVATE ((o), BURNER_TYPE_TRACK_DISC, BurnerTrackDiscPrivate))

G_DEFINE_TYPE (BurnerTrackDisc, burner_track_disc, BURNER_TYPE_TRACK);

/**
 * burner_track_disc_set_track_num:
 * @track: a #BurnerTrackDisc
 * @num: a #guint
 *
 * Sets a track number which can be used
 * to copy only one specific session on a multisession disc
 *
 * Return value: a #BurnerBurnResult.
 * BURNER_BURN_OK if it was successful,
 * BURNER_BURN_ERR otherwise.
 **/

BurnerBurnResult
burner_track_disc_set_track_num (BurnerTrackDisc *track,
				  guint num)
{
	BurnerTrackDiscPrivate *priv;

	g_return_val_if_fail (BURNER_IS_TRACK_DISC (track), BURNER_BURN_ERR);

	priv = BURNER_TRACK_DISC_PRIVATE (track);
	priv->track_num = num;

	return BURNER_BURN_OK;
}

/**
 * burner_track_disc_get_track_num:
 * @track: a #BurnerTrackDisc
 *
 * Gets the track number which will be used
 * to copy only one specific session on a multisession disc
 *
 * Return value: a #guint. 0 if none is set, any other number otherwise.
 **/

guint
burner_track_disc_get_track_num (BurnerTrackDisc *track)
{
	BurnerTrackDiscPrivate *priv;

	g_return_val_if_fail (BURNER_IS_TRACK_DISC (track), BURNER_BURN_ERR);

	priv = BURNER_TRACK_DISC_PRIVATE (track);
	return priv->track_num;
}

static void
burner_track_disc_remove_drive (BurnerTrackDisc *track)
{
	BurnerTrackDiscPrivate *priv;

	priv = BURNER_TRACK_DISC_PRIVATE (track);

	if (priv->src_added_sig) {
		g_signal_handler_disconnect (priv->drive, priv->src_added_sig);
		priv->src_added_sig = 0;
	}

	if (priv->src_removed_sig) {
		g_signal_handler_disconnect (priv->drive, priv->src_removed_sig);
		priv->src_removed_sig = 0;
	}

	if (priv->drive) {
		g_object_unref (priv->drive);
		priv->drive = NULL;
	}
}

static void
burner_track_disc_medium_changed (BurnerDrive *drive,
				   BurnerMedium *medium,
				   BurnerTrack *track)
{
	burner_track_changed (track);
}

/**
 * burner_track_disc_set_drive:
 * @track: a #BurnerTrackDisc
 * @drive: a #BurnerDrive
 *
 * Sets @drive to be the #BurnerDrive that will be used
 * as the source when copying
 *
 * Return value: a #BurnerBurnResult.
 * BURNER_BURN_OK if it was successful,
 * BURNER_BURN_ERR otherwise.
 **/

BurnerBurnResult
burner_track_disc_set_drive (BurnerTrackDisc *track,
			      BurnerDrive *drive)
{
	BurnerTrackDiscPrivate *priv;

	g_return_val_if_fail (BURNER_IS_TRACK_DISC (track), BURNER_BURN_ERR);

	priv = BURNER_TRACK_DISC_PRIVATE (track);

	burner_track_disc_remove_drive (track);
	if (!drive) {
		burner_track_changed (BURNER_TRACK (track));
		return BURNER_BURN_OK;
	}

	priv->drive = drive;
	g_object_ref (drive);

	priv->src_added_sig = g_signal_connect (drive,
						"medium-added",
						G_CALLBACK (burner_track_disc_medium_changed),
						track);
	priv->src_removed_sig = g_signal_connect (drive,
						  "medium-removed",
						  G_CALLBACK (burner_track_disc_medium_changed),
						  track);

	burner_track_changed (BURNER_TRACK (track));

	return BURNER_BURN_OK;
}

/**
 * burner_track_disc_get_drive:
 * @track: a #BurnerTrackDisc
 *
 * Gets the #BurnerDrive object that will be used as
 * the source when copying.
 *
 * Return value: a #BurnerDrive or NULL. Don't unref or free it.
 **/

BurnerDrive *
burner_track_disc_get_drive (BurnerTrackDisc *track)
{
	BurnerTrackDiscPrivate *priv;

	g_return_val_if_fail (BURNER_IS_TRACK_DISC (track), NULL);

	priv = BURNER_TRACK_DISC_PRIVATE (track);
	return priv->drive;
}

/**
 * burner_track_disc_get_medium_type:
 * @track: a #BurnerTrackDisc
 *
 * Gets the #BurnerMedia for the medium that is
 * currently inserted into the drive assigned for @track
 * with burner_track_disc_set_drive ().
 *
 * Return value: a #BurnerMedia.
 **/

BurnerMedia
burner_track_disc_get_medium_type (BurnerTrackDisc *track)
{
	BurnerTrackDiscPrivate *priv;
	BurnerMedium *medium;

	g_return_val_if_fail (BURNER_IS_TRACK_DISC (track), BURNER_MEDIUM_NONE);

	priv = BURNER_TRACK_DISC_PRIVATE (track);
	medium = burner_drive_get_medium (priv->drive);
	if (!medium)
		return BURNER_MEDIUM_NONE;

	return burner_medium_get_status (medium);
}

static BurnerBurnResult
burner_track_disc_get_size (BurnerTrack *track,
			     goffset *blocks,
			     goffset *block_size)
{
	BurnerMedium *medium;
	goffset medium_size = 0;
	goffset medium_blocks = 0;
	BurnerTrackDiscPrivate *priv;

	priv = BURNER_TRACK_DISC_PRIVATE (track);
	medium = burner_drive_get_medium (priv->drive);
	if (!medium)
		return BURNER_BURN_NOT_READY;

	burner_medium_get_data_size (medium, &medium_size, &medium_blocks);

	if (blocks)
		*blocks = medium_blocks;

	if (block_size)
		*block_size = medium_blocks? (medium_size / medium_blocks):0;

	return BURNER_BURN_OK;
}

static BurnerBurnResult
burner_track_disc_get_track_type (BurnerTrack *track,
				   BurnerTrackType *type)
{
	BurnerTrackDiscPrivate *priv;
	BurnerMedium *medium;

	priv = BURNER_TRACK_DISC_PRIVATE (track);

	medium = burner_drive_get_medium (priv->drive);

	burner_track_type_set_has_medium (type);
	burner_track_type_set_medium_type (type, burner_medium_get_status (medium));

	return BURNER_BURN_OK;
}

static void
burner_track_disc_init (BurnerTrackDisc *object)
{ }

static void
burner_track_disc_finalize (GObject *object)
{
	burner_track_disc_remove_drive (BURNER_TRACK_DISC (object));

	G_OBJECT_CLASS (burner_track_disc_parent_class)->finalize (object);
}

static void
burner_track_disc_class_init (BurnerTrackDiscClass *klass)
{
	GObjectClass* object_class = G_OBJECT_CLASS (klass);
	BurnerTrackClass* track_class = BURNER_TRACK_CLASS (klass);

	g_type_class_add_private (klass, sizeof (BurnerTrackDiscPrivate));

	object_class->finalize = burner_track_disc_finalize;

	track_class->get_size = burner_track_disc_get_size;
	track_class->get_type = burner_track_disc_get_track_type;
}

/**
 * burner_track_disc_new:
 *
 * Creates a new #BurnerTrackDisc object.
 *
 * This type of tracks is used to copy media either
 * to a disc image file or to another medium.
 *
 * Return value: a #BurnerTrackDisc.
 **/

BurnerTrackDisc *
burner_track_disc_new (void)
{
	return g_object_new (BURNER_TYPE_TRACK_DISC, NULL);
}
