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
#include <glib/gi18n-lib.h>

#include "burn-debug.h"
#include "burn-basics.h"
#include "burner-track-stream.h"

typedef struct _BurnerTrackStreamPrivate BurnerTrackStreamPrivate;
struct _BurnerTrackStreamPrivate
{
        GFileMonitor *monitor;
	gchar *uri;

	BurnerStreamFormat format;

	guint64 gap;
	guint64 start;
	guint64 end;
};

#define BURNER_TRACK_STREAM_PRIVATE(o)  (G_TYPE_INSTANCE_GET_PRIVATE ((o), BURNER_TYPE_TRACK_STREAM, BurnerTrackStreamPrivate))

G_DEFINE_TYPE (BurnerTrackStream, burner_track_stream, BURNER_TYPE_TRACK);

static BurnerBurnResult
burner_track_stream_set_source_real (BurnerTrackStream *track,
				      const gchar *uri)
{
	BurnerTrackStreamPrivate *priv;

	priv = BURNER_TRACK_STREAM_PRIVATE (track);

	if (priv->uri)
		g_free (priv->uri);

	priv->uri = g_strdup (uri);

	/* Since that's a new URI chances are, the end point is different */
	priv->end = 0;

	return BURNER_BURN_OK;
}

/**
 * burner_track_stream_set_source:
 * @track: a #BurnerTrackStream
 * @uri: a #gchar
 *
 * Sets the stream (song or video) uri.
 *
 * Note: it resets the end point of the track to 0 but keeps start point and gap
 * unchanged.
 *
 * Return value: a #BurnerBurnResult. BURNER_BURN_OK if it is successful.
 **/

BurnerBurnResult
burner_track_stream_set_source (BurnerTrackStream *track,
				 const gchar *uri)
{
	BurnerTrackStreamClass *klass;
	BurnerBurnResult res;

	g_return_val_if_fail (BURNER_IS_TRACK_STREAM (track), BURNER_BURN_ERR);

	klass = BURNER_TRACK_STREAM_GET_CLASS (track);
	if (!klass->set_source)
		return BURNER_BURN_ERR;

	res = klass->set_source (track, uri);
	if (res != BURNER_BURN_OK)
		return res;

	burner_track_changed (BURNER_TRACK (track));
	return BURNER_BURN_OK;
}

/**
 * burner_track_stream_get_format:
 * @track: a #BurnerTrackStream
 *
 * This function returns the format of the stream.
 *
 * Return value: a #BurnerStreamFormat.
 **/

BurnerStreamFormat
burner_track_stream_get_format (BurnerTrackStream *track)
{
	BurnerTrackStreamPrivate *priv;

	g_return_val_if_fail (BURNER_IS_TRACK_STREAM (track), BURNER_AUDIO_FORMAT_NONE);

	priv = BURNER_TRACK_STREAM_PRIVATE (track);

	return priv->format;
}

static BurnerBurnResult
burner_track_stream_set_format_real (BurnerTrackStream *track,
				      BurnerStreamFormat format)
{
	BurnerTrackStreamPrivate *priv;

	priv = BURNER_TRACK_STREAM_PRIVATE (track);

	if (format == BURNER_AUDIO_FORMAT_NONE)
		BURNER_BURN_LOG ("Setting a NONE audio format with a valid uri");

	priv->format = format;
	return BURNER_BURN_OK;
}

/**
 * burner_track_stream_set_format:
 * @track: a #BurnerTrackStream
 * @format: a #BurnerStreamFormat
 *
 * Sets the format of the stream.
 *
 * Return value: a #BurnerBurnResult. BURNER_BURN_OK if it is successful.
 **/

BurnerBurnResult
burner_track_stream_set_format (BurnerTrackStream *track,
				 BurnerStreamFormat format)
{
	BurnerTrackStreamClass *klass;
	BurnerBurnResult res;

	g_return_val_if_fail (BURNER_IS_TRACK_STREAM (track), BURNER_BURN_ERR);

	klass = BURNER_TRACK_STREAM_GET_CLASS (track);
	if (!klass->set_format)
		return BURNER_BURN_ERR;

	res = klass->set_format (track, format);
	if (res != BURNER_BURN_OK)
		return res;

	burner_track_changed (BURNER_TRACK (track));
	return BURNER_BURN_OK;
}

static BurnerBurnResult
burner_track_stream_set_boundaries_real (BurnerTrackStream *track,
					  gint64 start,
					  gint64 end,
					  gint64 gap)
{
	BurnerTrackStreamPrivate *priv;

	priv = BURNER_TRACK_STREAM_PRIVATE (track);

	if (gap >= 0)
		priv->gap = gap;

	if (end > 0)
		priv->end = end;

	if (start >= 0)
		priv->start = start;

	return BURNER_BURN_OK;
}

/**
 * burner_track_stream_set_boundaries:
 * @track: a #BurnerTrackStream
 * @start: a #gint64 or -1 to ignore
 * @end: a #gint64 or -1 to ignore
 * @gap: a #gint64 or -1 to ignore
 *
 * Sets the boundaries of the stream (where it starts, ends in the file;
 * how long is the gap with the next track) in nano seconds.
 *
 * Return value: a #BurnerBurnResult. BURNER_BURN_OK if it is successful.
 **/

BurnerBurnResult
burner_track_stream_set_boundaries (BurnerTrackStream *track,
				     gint64 start,
				     gint64 end,
				     gint64 gap)
{
	BurnerTrackStreamClass *klass;
	BurnerBurnResult res;

	g_return_val_if_fail (BURNER_IS_TRACK_STREAM (track), BURNER_BURN_ERR);

	klass = BURNER_TRACK_STREAM_GET_CLASS (track);
	if (!klass->set_boundaries)
		return BURNER_BURN_ERR;

	res = klass->set_boundaries (track, start, end, gap);
	if (res != BURNER_BURN_OK)
		return res;

	burner_track_changed (BURNER_TRACK (track));
	return BURNER_BURN_OK;
}

/**
 * burner_track_stream_get_source:
 * @track: a #BurnerTrackStream
 * @uri: a #gboolean
 *
 * This function returns the path or the URI (if @uri is TRUE)
 * of the stream (song or video file).
 *
 * Note: this function resets any length previously set to 0.
 * Return value: a #gchar.
 **/

gchar *
burner_track_stream_get_source (BurnerTrackStream *track,
				 gboolean uri)
{
	BurnerTrackStreamPrivate *priv;

	g_return_val_if_fail (BURNER_IS_TRACK_STREAM (track), NULL);

	priv = BURNER_TRACK_STREAM_PRIVATE (track);
	if (uri)
		return burner_string_get_uri (priv->uri);
	else
		return burner_string_get_localpath (priv->uri);
}

/**
 * burner_track_stream_get_gap:
 * @track: a #BurnerTrackStream
 *
 * This function returns length of the gap (in nano seconds).
 *
 * Return value: a #guint64.
 **/

guint64
burner_track_stream_get_gap (BurnerTrackStream *track)
{
	BurnerTrackStreamPrivate *priv;

	g_return_val_if_fail (BURNER_IS_TRACK_STREAM (track), 0);

	priv = BURNER_TRACK_STREAM_PRIVATE (track);
	return priv->gap;
}

/**
 * burner_track_stream_get_start:
 * @track: a #BurnerTrackStream
 *
 * This function returns start time in the stream (in nano seconds).
 *
 * Return value: a #guint64.
 **/

guint64
burner_track_stream_get_start (BurnerTrackStream *track)
{
	BurnerTrackStreamPrivate *priv;

	g_return_val_if_fail (BURNER_IS_TRACK_STREAM (track), 0);

	priv = BURNER_TRACK_STREAM_PRIVATE (track);
	return priv->start;
}

/**
 * burner_track_stream_get_end:
 * @track: a #BurnerTrackStream
 *
 * This function returns end time in the stream (in nano seconds).
 *
 * Return value: a #guint64.
 **/

guint64
burner_track_stream_get_end (BurnerTrackStream *track)
{
	BurnerTrackStreamPrivate *priv;

	g_return_val_if_fail (BURNER_IS_TRACK_STREAM (track), 0);

	priv = BURNER_TRACK_STREAM_PRIVATE (track);
	return priv->end;
}

/**
 * burner_track_stream_get_length:
 * @track: a #BurnerTrackStream
 * @length: a #guint64
 *
 * This function returns the length of the stream (in nano seconds)
 * taking into account the start and end time as well as the length
 * of the gap. It stores it in @length.
 *
 * Return value: a #BurnerBurnResult. BURNER_BURN_OK if @length was set.
 **/

BurnerBurnResult
burner_track_stream_get_length (BurnerTrackStream *track,
				 guint64 *length)
{
	BurnerTrackStreamPrivate *priv;

	g_return_val_if_fail (BURNER_IS_TRACK_STREAM (track), BURNER_BURN_ERR);

	priv = BURNER_TRACK_STREAM_PRIVATE (track);

	if (priv->start < 0 || priv->end <= 0)
		return BURNER_BURN_ERR;

	*length = BURNER_STREAM_LENGTH (priv->start, priv->end + priv->gap);
	return BURNER_BURN_OK;
}

static BurnerBurnResult
burner_track_stream_get_size (BurnerTrack *track,
			       goffset *blocks,
			       goffset *block_size)
{
	BurnerStreamFormat format;

	format = burner_track_stream_get_format (BURNER_TRACK_STREAM (track));
	if (!BURNER_STREAM_FORMAT_HAS_VIDEO (format)) {
		if (blocks) {
			guint64 length = 0;

			burner_track_stream_get_length (BURNER_TRACK_STREAM (track), &length);
			*blocks = length * 75LL / 1000000000LL;
		}

		if (block_size)
			*block_size = 2352;
	}
	else {
		if (blocks) {
			guint64 length = 0;

			/* This is based on a simple formula:
			 * 4700000000 bytes means 2 hours */
			burner_track_stream_get_length (BURNER_TRACK_STREAM (track), &length);
			*blocks = length * 47LL / 72000LL / 2048LL;
		}

		if (block_size)
			*block_size = 2048;
	}

	return BURNER_BURN_OK;
}

static BurnerBurnResult
burner_track_stream_get_status (BurnerTrack *track,
				 BurnerStatus *status)
{
	BurnerTrackStreamPrivate *priv;

	priv = BURNER_TRACK_STREAM_PRIVATE (track);
	if (!priv->uri) {
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
burner_track_stream_get_track_type (BurnerTrack *track,
				     BurnerTrackType *type)
{
	BurnerTrackStreamPrivate *priv;

	priv = BURNER_TRACK_STREAM_PRIVATE (track);

	burner_track_type_set_has_stream (type);
	burner_track_type_set_stream_format (type, priv->format);

	return BURNER_BURN_OK;
}

static void
burner_track_stream_init (BurnerTrackStream *object)
{ }

static void
burner_track_stream_finalize (GObject *object)
{
	BurnerTrackStreamPrivate *priv;

	priv = BURNER_TRACK_STREAM_PRIVATE (object);
	if (priv->uri) {
		g_free (priv->uri);
		priv->uri = NULL;
	}

	G_OBJECT_CLASS (burner_track_stream_parent_class)->finalize (object);
}

static void
burner_track_stream_class_init (BurnerTrackStreamClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);
	BurnerTrackClass *track_class = BURNER_TRACK_CLASS (klass);

	g_type_class_add_private (klass, sizeof (BurnerTrackStreamPrivate));

	object_class->finalize = burner_track_stream_finalize;

	track_class->get_size = burner_track_stream_get_size;
	track_class->get_status = burner_track_stream_get_status;
	track_class->get_type = burner_track_stream_get_track_type;

	klass->set_source = burner_track_stream_set_source_real;
	klass->set_format = burner_track_stream_set_format_real;
	klass->set_boundaries = burner_track_stream_set_boundaries_real;
}

/**
 * burner_track_stream_new:
 *
 *  Creates a new #BurnerTrackStream object.
 *
 * This type of tracks is used to burn audio or
 * video files.
 *
 * Return value: a #BurnerTrackStream object.
 **/

BurnerTrackStream *
burner_track_stream_new (void)
{
	return g_object_new (BURNER_TYPE_TRACK_STREAM, NULL);
}
