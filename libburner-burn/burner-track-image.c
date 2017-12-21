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

#include "burner-track-image.h"
#include "burner-enums.h"
#include "burner-track.h"

#include "burn-debug.h"
#include "burn-image-format.h"

typedef struct _BurnerTrackImagePrivate BurnerTrackImagePrivate;
struct _BurnerTrackImagePrivate
{
	gchar *image;
	gchar *toc;

	guint64 blocks;

	BurnerImageFormat format;
};

#define BURNER_TRACK_IMAGE_PRIVATE(o)  (G_TYPE_INSTANCE_GET_PRIVATE ((o), BURNER_TYPE_TRACK_IMAGE, BurnerTrackImagePrivate))


G_DEFINE_TYPE (BurnerTrackImage, burner_track_image, BURNER_TYPE_TRACK);

static BurnerBurnResult
burner_track_image_set_source_real (BurnerTrackImage *track,
				     const gchar *image,
				     const gchar *toc,
				     BurnerImageFormat format)
{
	BurnerTrackImagePrivate *priv;

	priv = BURNER_TRACK_IMAGE_PRIVATE (track);

	priv->format = format;

	if (priv->image)
		g_free (priv->image);

	if (priv->toc)
		g_free (priv->toc);

	priv->image = g_strdup (image);
	priv->toc = g_strdup (toc);

	return BURNER_BURN_OK;
}

/**
 * burner_track_image_set_source:
 * @track: a #BurnerTrackImage
 * @image: a #gchar or NULL
 * @toc: a #gchar or NULL
 * @format: a #BurnerImageFormat
 *
 * Sets the image source path (and its toc if need be)
 * as well as its format.
 *
 * Return value: a #BurnerBurnResult. BURNER_BURN_OK if it is successful.
 **/

BurnerBurnResult
burner_track_image_set_source (BurnerTrackImage *track,
				const gchar *image,
				const gchar *toc,
				BurnerImageFormat format)
{
	BurnerTrackImageClass *klass;
	BurnerBurnResult res;

	g_return_val_if_fail (BURNER_IS_TRACK_IMAGE (track), BURNER_BURN_ERR);

	/* See if it has changed */
	klass = BURNER_TRACK_IMAGE_GET_CLASS (track);
	if (!klass->set_source)
		return BURNER_BURN_ERR;

	res = klass->set_source (track, image, toc, format);
	if (res != BURNER_BURN_OK)
		return res;

	burner_track_changed (BURNER_TRACK (track));
	return BURNER_BURN_OK;
}

static BurnerBurnResult
burner_track_image_set_block_num_real (BurnerTrackImage *track,
					goffset blocks)
{
	BurnerTrackImagePrivate *priv;

	priv = BURNER_TRACK_IMAGE_PRIVATE (track);
	priv->blocks = blocks;
	return BURNER_BURN_OK;
}

/**
 * burner_track_image_set_block_num:
 * @track: a #BurnerTrackImage
 * @blocks: a #goffset
 *
 * Sets the image size (in sectors).
 *
 * Return value: a #BurnerBurnResult. BURNER_BURN_OK if it is successful.
 **/

BurnerBurnResult
burner_track_image_set_block_num (BurnerTrackImage *track,
				   goffset blocks)
{
	BurnerTrackImagePrivate *priv;
	BurnerTrackImageClass *klass;
	BurnerBurnResult res;

	g_return_val_if_fail (BURNER_IS_TRACK_IMAGE (track), BURNER_BURN_ERR);

	priv = BURNER_TRACK_IMAGE_PRIVATE (track);
	if (priv->blocks == blocks)
		return BURNER_BURN_OK;

	klass = BURNER_TRACK_IMAGE_GET_CLASS (track);
	if (!klass->set_block_num)
		return BURNER_BURN_ERR;

	res = klass->set_block_num (track, blocks);
	if (res != BURNER_BURN_OK)
		return res;

	burner_track_changed (BURNER_TRACK (track));
	return BURNER_BURN_OK;
}

/**
 * burner_track_image_get_source:
 * @track: a #BurnerTrackImage
 * @uri: a #gboolean
 *
 * This function returns the path or the URI (if @uri is TRUE) of the
 * source image file.
 *
 * Return value: a #gchar
 **/

gchar *
burner_track_image_get_source (BurnerTrackImage *track,
				gboolean uri)
{
	BurnerTrackImagePrivate *priv;

	g_return_val_if_fail (BURNER_IS_TRACK_IMAGE (track), NULL);

	priv = BURNER_TRACK_IMAGE_PRIVATE (track);

	if (!priv->image) {
		gchar *complement;
		gchar *retval;
		gchar *toc;

		if (!priv->toc) {
			BURNER_BURN_LOG ("Image nor toc were set");
			return NULL;
		}

		toc = burner_string_get_localpath (priv->toc);
		complement = burner_image_format_get_complement (priv->format, toc);
		g_free (toc);

		if (!complement) {
			BURNER_BURN_LOG ("No complement could be retrieved");
			return NULL;
		}

		BURNER_BURN_LOG ("Complement file retrieved %s", complement);
		if (uri)
			retval = burner_string_get_uri (complement);
		else
			retval = burner_string_get_localpath (complement);

		g_free (complement);
		return retval;
	}

	if (uri)
		return burner_string_get_uri (priv->image);
	else
		return burner_string_get_localpath (priv->image);
}

/**
 * burner_track_image_get_toc_source:
 * @track: a #BurnerTrackImage
 * @uri: a #gboolean
 *
 * This function returns the path or the URI (if @uri is TRUE) of the
 * source toc file.
 *
 * Return value: a #gchar
 **/

gchar *
burner_track_image_get_toc_source (BurnerTrackImage *track,
				    gboolean uri)
{
	BurnerTrackImagePrivate *priv;

	g_return_val_if_fail (BURNER_IS_TRACK_IMAGE (track), NULL);

	priv = BURNER_TRACK_IMAGE_PRIVATE (track);

	/* Don't use file complement retrieval here as it's not possible */
	if (uri)
		return burner_string_get_uri (priv->toc);
	else
		return burner_string_get_localpath (priv->toc);
}

/**
 * burner_track_image_get_format:
 * @track: a #BurnerTrackImage
 *
 * This function returns the format of the
 * source image.
 *
 * Return value: a #BurnerImageFormat
 **/

BurnerImageFormat
burner_track_image_get_format (BurnerTrackImage *track)
{
	BurnerTrackImagePrivate *priv;

	g_return_val_if_fail (BURNER_IS_TRACK_IMAGE (track), BURNER_IMAGE_FORMAT_NONE);

	priv = BURNER_TRACK_IMAGE_PRIVATE (track);
	return priv->format;
}

/**
 * burner_track_image_need_byte_swap:
 * @track: a #BurnerTrackImage
 *
 * This function returns whether the data bytes need swapping. Some .bin files
 * associated with .cue files are little endian for audio whereas they should
 * be big endian.
 *
 * Return value: a #gboolean
 **/

gboolean
burner_track_image_need_byte_swap (BurnerTrackImage *track)
{
	BurnerTrackImagePrivate *priv;
	gchar *cueuri;
	gboolean res;

	g_return_val_if_fail (BURNER_IS_TRACK_IMAGE (track), BURNER_IMAGE_FORMAT_NONE);

	priv = BURNER_TRACK_IMAGE_PRIVATE (track);
	if (priv->format != BURNER_IMAGE_FORMAT_CUE)
		return FALSE;

	cueuri = burner_string_get_uri (priv->toc);
	res = burner_image_format_cue_bin_byte_swap (cueuri, NULL, NULL);
	g_free (cueuri);

	return res;
}

static BurnerBurnResult
burner_track_image_get_track_type (BurnerTrack *track,
				    BurnerTrackType *type)
{
	BurnerTrackImagePrivate *priv;

	priv = BURNER_TRACK_IMAGE_PRIVATE (track);

	burner_track_type_set_has_image (type);
	burner_track_type_set_image_format (type, priv->format);

	return BURNER_BURN_OK;
}

static BurnerBurnResult
burner_track_image_get_size (BurnerTrack *track,
			      goffset *blocks,
			      goffset *block_size)
{
	BurnerTrackImagePrivate *priv;

	priv = BURNER_TRACK_IMAGE_PRIVATE (track);

	if (priv->format == BURNER_IMAGE_FORMAT_BIN) {
		if (block_size)
			*block_size = 2048;
	}
	else if (priv->format == BURNER_IMAGE_FORMAT_CLONE) {
		if (block_size)
			*block_size = 2448;
	}
	else if (priv->format == BURNER_IMAGE_FORMAT_CDRDAO) {
		if (block_size)
			*block_size = 2352;
	}
	else if (priv->format == BURNER_IMAGE_FORMAT_CUE) {
		if (block_size)
			*block_size = 2352;
	}
	else if (block_size)
		*block_size = 0;

	if (blocks)
		*blocks = priv->blocks;

	return BURNER_BURN_OK;
}

static void
burner_track_image_init (BurnerTrackImage *object)
{ }

static void
burner_track_image_finalize (GObject *object)
{
	BurnerTrackImagePrivate *priv;

	priv = BURNER_TRACK_IMAGE_PRIVATE (object);
	if (priv->image) {
		g_free (priv->image);
		priv->image = NULL;
	}

	if (priv->toc) {
		g_free (priv->toc);
		priv->toc = NULL;
	}

	G_OBJECT_CLASS (burner_track_image_parent_class)->finalize (object);
}

static void
burner_track_image_class_init (BurnerTrackImageClass *klass)
{
	GObjectClass* object_class = G_OBJECT_CLASS (klass);
	BurnerTrackClass *track_class = BURNER_TRACK_CLASS (klass);

	g_type_class_add_private (klass, sizeof (BurnerTrackImagePrivate));

	object_class->finalize = burner_track_image_finalize;

	track_class->get_size = burner_track_image_get_size;
	track_class->get_type = burner_track_image_get_track_type;

	klass->set_source = burner_track_image_set_source_real;
	klass->set_block_num = burner_track_image_set_block_num_real;
}

/**
 * burner_track_image_new:
 *
 * Creates a new #BurnerTrackImage object.
 *
 * This type of tracks is used to burn disc images.
 *
 * Return value: a #BurnerTrackImage object.
 **/

BurnerTrackImage *
burner_track_image_new (void)
{
	return g_object_new (BURNER_TYPE_TRACK_IMAGE, NULL);
}
