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

#include "burner-medium.h"
#include "burner-drive.h"
#include "burner-track-type.h"
#include "burner-track-type-private.h"

/**
 * burner_track_type_new:
 *
 * Creates a new #BurnerTrackType structure.
 * Free it with burner_track_type_free ().
 *
 * Return value: a #BurnerTrackType pointer.
 **/

BurnerTrackType *
burner_track_type_new (void)
{
	return g_new0 (BurnerTrackType, 1);
}

/**
 * burner_track_type_free:
 * @type: a #BurnerTrackType.
 *
 * Frees #BurnerTrackType structure.
 *
 **/

void
burner_track_type_free (BurnerTrackType *type)
{
	if (!type)
		return;

	g_free (type);
}

/**
 * burner_track_type_get_image_format:
 * @type: a #BurnerTrackType.
 *
 * Returns the format of an image when
 * burner_track_type_get_has_image () returned
 * TRUE.
 *
 * Return value: a #BurnerImageFormat
 **/

BurnerImageFormat
burner_track_type_get_image_format (const BurnerTrackType *type) 
{
	g_return_val_if_fail (type != NULL, BURNER_IMAGE_FORMAT_NONE);

	if (type->type != BURNER_TRACK_TYPE_IMAGE)
		return BURNER_IMAGE_FORMAT_NONE;

	return type->subtype.img_format;
}

/**
 * burner_track_type_get_data_fs:
 * @type: a #BurnerTrackType.
 *
 * Returns the parameters for the image generation
 * when burner_track_type_get_has_data () returned
 * TRUE.
 *
 * Return value: a #BurnerImageFS
 **/

BurnerImageFS
burner_track_type_get_data_fs (const BurnerTrackType *type) 
{
	g_return_val_if_fail (type != NULL, BURNER_IMAGE_FS_NONE);

	if (type->type != BURNER_TRACK_TYPE_DATA)
		return BURNER_IMAGE_FS_NONE;

	return type->subtype.fs_type;
}

/**
 * burner_track_type_get_stream_format:
 * @type: a #BurnerTrackType.
 *
 * Returns the format for a stream (song or video)
 * when burner_track_type_get_has_stream () returned
 * TRUE.
 *
 * Return value: a #BurnerStreamFormat
 **/

BurnerStreamFormat
burner_track_type_get_stream_format (const BurnerTrackType *type) 
{
	g_return_val_if_fail (type != NULL, BURNER_AUDIO_FORMAT_NONE);

	if (type->type != BURNER_TRACK_TYPE_STREAM)
		return BURNER_AUDIO_FORMAT_NONE;

	return type->subtype.stream_format;
}

/**
 * burner_track_type_get_medium_type:
 * @type: a #BurnerTrackType.
 *
 * Returns the medium type
 * when burner_track_type_get_has_medium () returned
 * TRUE.
 *
 * Return value: a #BurnerMedia
 **/

BurnerMedia
burner_track_type_get_medium_type (const BurnerTrackType *type) 
{
	g_return_val_if_fail (type != NULL, BURNER_MEDIUM_NONE);

	if (type->type != BURNER_TRACK_TYPE_DISC)
		return BURNER_MEDIUM_NONE;

	return type->subtype.media;
}

/**
 * burner_track_type_set_image_format:
 * @type: a #BurnerTrackType.
 * @format: a #BurnerImageFormat
 *
 * Sets the #BurnerImageFormat. Must be called
 * after burner_track_type_set_has_image ().
 *
 **/

void
burner_track_type_set_image_format (BurnerTrackType *type,
				     BurnerImageFormat format) 
{
	g_return_if_fail (type != NULL);

	if (type->type != BURNER_TRACK_TYPE_IMAGE)
		return;

	type->subtype.img_format = format;
}

/**
 * burner_track_type_set_data_fs:
 * @type: a #BurnerTrackType.
 * @fs_type: a #BurnerImageFS
 *
 * Sets the #BurnerImageFS. Must be called
 * after burner_track_type_set_has_data ().
 *
 **/

void
burner_track_type_set_data_fs (BurnerTrackType *type,
				BurnerImageFS fs_type) 
{
	g_return_if_fail (type != NULL);

	if (type->type != BURNER_TRACK_TYPE_DATA)
		return;

	type->subtype.fs_type = fs_type;
}

/**
 * burner_track_type_set_stream_format:
 * @type: a #BurnerTrackType.
 * @format: a #BurnerImageFormat
 *
 * Sets the #BurnerStreamFormat. Must be called
 * after burner_track_type_set_has_stream ().
 *
 **/

void
burner_track_type_set_stream_format (BurnerTrackType *type,
				      BurnerStreamFormat format) 
{
	g_return_if_fail (type != NULL);

	if (type->type != BURNER_TRACK_TYPE_STREAM)
		return;

	type->subtype.stream_format = format;
}

/**
 * burner_track_type_set_medium_type:
 * @type: a #BurnerTrackType.
 * @media: a #BurnerMedia
 *
 * Sets the #BurnerMedia. Must be called
 * after burner_track_type_set_has_medium ().
 *
 **/

void
burner_track_type_set_medium_type (BurnerTrackType *type,
				    BurnerMedia media) 
{
	g_return_if_fail (type != NULL);

	if (type->type != BURNER_TRACK_TYPE_DISC)
		return;

	type->subtype.media = media;
}

/**
 * burner_track_type_is_empty:
 * @type: a #BurnerTrackType.
 *
 * Returns TRUE if no type was set.
 *
 * Return value: a #gboolean
 **/

gboolean
burner_track_type_is_empty (const BurnerTrackType *type)
{
	g_return_val_if_fail (type != NULL, FALSE);

	return (type->type == BURNER_TRACK_TYPE_NONE);
}

/**
 * burner_track_type_get_has_data:
 * @type: a #BurnerTrackType.
 *
 * Returns TRUE if DATA type (see burner_track_data_new ()) was set.
 *
 * Return value: a #gboolean
 **/

gboolean
burner_track_type_get_has_data (const BurnerTrackType *type)
{
	g_return_val_if_fail (type != NULL, FALSE);

	return type->type == BURNER_TRACK_TYPE_DATA;
}

/**
 * burner_track_type_get_has_image:
 * @type: a #BurnerTrackType.
 *
 * Returns TRUE if IMAGE type (see burner_track_image_new ()) was set.
 *
 * Return value: a #gboolean
 **/

gboolean
burner_track_type_get_has_image (const BurnerTrackType *type)
{
	g_return_val_if_fail (type != NULL, FALSE);

	return type->type == BURNER_TRACK_TYPE_IMAGE;
}

/**
 * burner_track_type_get_has_stream:
 * @type: a #BurnerTrackType.
 *
 * This function returns %TRUE if IMAGE type (see burner_track_stream_new ()) was set.
 *
 * Return value: a #gboolean
 **/

gboolean
burner_track_type_get_has_stream (const BurnerTrackType *type)
{
	g_return_val_if_fail (type != NULL, FALSE);

	return type->type == BURNER_TRACK_TYPE_STREAM;
}

/**
 * burner_track_type_get_has_medium:
 * @type: a #BurnerTrackType.
 *
 * Returns TRUE if MEDIUM type (see burner_track_disc_new ()) was set.
 *
 * Return value: a #gboolean
 **/

gboolean
burner_track_type_get_has_medium (const BurnerTrackType *type)
{
	g_return_val_if_fail (type != NULL, FALSE);

	return type->type == BURNER_TRACK_TYPE_DISC;
}

/**
 * burner_track_type_set_has_data:
 * @type: a #BurnerTrackType.
 *
 * Set DATA type for @type.
 *
 **/

void
burner_track_type_set_has_data (BurnerTrackType *type)
{
	g_return_if_fail (type != NULL);

	type->type = BURNER_TRACK_TYPE_DATA;
}

/**
 * burner_track_type_set_has_image:
 * @type: a #BurnerTrackType.
 *
 * Set IMAGE type for @type.
 *
 **/

void
burner_track_type_set_has_image (BurnerTrackType *type)
{
	g_return_if_fail (type != NULL);

	type->type = BURNER_TRACK_TYPE_IMAGE;
}

/**
 * burner_track_type_set_has_stream:
 * @type: a #BurnerTrackType.
 *
 * Set STREAM type for @type
 *
 **/

void
burner_track_type_set_has_stream (BurnerTrackType *type)
{
	g_return_if_fail (type != NULL);

	type->type = BURNER_TRACK_TYPE_STREAM;
}

/**
 * burner_track_type_set_has_medium:
 * @type: a #BurnerTrackType.
 *
 * Set MEDIUM type for @type.
 *
 **/

void
burner_track_type_set_has_medium (BurnerTrackType *type)
{
	g_return_if_fail (type != NULL);

	type->type = BURNER_TRACK_TYPE_DISC;
}

/**
 * burner_track_type_equal:
 * @type_A: a #BurnerTrackType.
 * @type_B: a #BurnerTrackType.
 *
 * Returns TRUE if @type_A and @type_B represents
 * the same type and subtype.
 *
 * Return value: a #gboolean
 **/

gboolean
burner_track_type_equal (const BurnerTrackType *type_A,
			  const BurnerTrackType *type_B)
{
	g_return_val_if_fail (type_A != NULL, FALSE);
	g_return_val_if_fail (type_B != NULL, FALSE);

	if (type_A->type != type_B->type)
		return FALSE;

	switch (type_A->type) {
	case BURNER_TRACK_TYPE_DATA:
		if (type_A->subtype.fs_type != type_B->subtype.fs_type)
			return FALSE;
		break;
	
	case BURNER_TRACK_TYPE_DISC:
		if (type_B->subtype.media != type_A->subtype.media)
			return FALSE;
		break;
	
	case BURNER_TRACK_TYPE_IMAGE:
		if (type_A->subtype.img_format != type_B->subtype.img_format)
			return FALSE;
		break;

	case BURNER_TRACK_TYPE_STREAM:
		if (type_A->subtype.stream_format != type_B->subtype.stream_format)
			return FALSE;
		break;

	default:
		break;
	}

	return TRUE;
}

#if 0
/**
 * burner_track_type_match:
 * @type_A: a #BurnerTrackType.
 * @type_B: a #BurnerTrackType.
 *
 * Returns TRUE if @type_A and @type_B match.
 *
 * (Used internally)
 *
 * Return value: a #gboolean
 **/

gboolean
burner_track_type_match (const BurnerTrackType *type_A,
			  const BurnerTrackType *type_B)
{
	g_return_val_if_fail (type_A != NULL, FALSE);
	g_return_val_if_fail (type_B != NULL, FALSE);

	if (type_A->type != type_B->type)
		return FALSE;

	switch (type_A->type) {
	case BURNER_TRACK_TYPE_DATA:
		if (!(type_A->subtype.fs_type & type_B->subtype.fs_type))
			return FALSE;
		break;
	
	case BURNER_TRACK_TYPE_DISC:
		if (!(type_A->subtype.media & type_B->subtype.media))
			return FALSE;
		break;
	
	case BURNER_TRACK_TYPE_IMAGE:
		if (!(type_A->subtype.img_format & type_B->subtype.img_format))
			return FALSE;
		break;

	case BURNER_TRACK_TYPE_STREAM:
		if (!(type_A->subtype.stream_format & type_B->subtype.stream_format))
			return FALSE;
		break;

	default:
		break;
	}

	return TRUE;
}

#endif
