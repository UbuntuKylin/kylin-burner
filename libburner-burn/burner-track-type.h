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

#ifndef _BURN_TRACK_TYPE_H
#define _BURN_TRACK_TYPE_H

#include <glib.h>

#include <burner-enums.h>
#include <burner-media.h>

G_BEGIN_DECLS

typedef struct _BurnerTrackType BurnerTrackType;

BurnerTrackType *
burner_track_type_new (void);

void
burner_track_type_free (BurnerTrackType *type);

gboolean
burner_track_type_is_empty (const BurnerTrackType *type);
gboolean
burner_track_type_get_has_data (const BurnerTrackType *type);
gboolean
burner_track_type_get_has_image (const BurnerTrackType *type);
gboolean
burner_track_type_get_has_stream (const BurnerTrackType *type);
gboolean
burner_track_type_get_has_medium (const BurnerTrackType *type);

void
burner_track_type_set_has_data (BurnerTrackType *type);
void
burner_track_type_set_has_image (BurnerTrackType *type);
void
burner_track_type_set_has_stream (BurnerTrackType *type);
void
burner_track_type_set_has_medium (BurnerTrackType *type);

BurnerStreamFormat
burner_track_type_get_stream_format (const BurnerTrackType *type);
BurnerImageFormat
burner_track_type_get_image_format (const BurnerTrackType *type);
BurnerMedia
burner_track_type_get_medium_type (const BurnerTrackType *type);
BurnerImageFS
burner_track_type_get_data_fs (const BurnerTrackType *type);

void
burner_track_type_set_stream_format (BurnerTrackType *type,
				      BurnerStreamFormat format);
void
burner_track_type_set_image_format (BurnerTrackType *type,
				     BurnerImageFormat format);
void
burner_track_type_set_medium_type (BurnerTrackType *type,
				    BurnerMedia media);
void
burner_track_type_set_data_fs (BurnerTrackType *type,
				BurnerImageFS fs_type);

gboolean
burner_track_type_equal (const BurnerTrackType *type_A,
			  const BurnerTrackType *type_B);

G_END_DECLS

#endif
