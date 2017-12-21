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

#ifndef _BURN_TRACK_H
#define _BURN_TRACK_H

#include <glib.h>
#include <glib-object.h>

#include <burner-drive.h>
#include <burner-medium.h>

#include <burner-enums.h>
#include <burner-error.h>
#include <burner-status.h>

#include <burner-track-type.h>

G_BEGIN_DECLS

#define BURNER_TYPE_TRACK             (burner_track_get_type ())
#define BURNER_TRACK(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), BURNER_TYPE_TRACK, BurnerTrack))
#define BURNER_TRACK_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), BURNER_TYPE_TRACK, BurnerTrackClass))
#define BURNER_IS_TRACK(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BURNER_TYPE_TRACK))
#define BURNER_IS_TRACK_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), BURNER_TYPE_TRACK))
#define BURNER_TRACK_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), BURNER_TYPE_TRACK, BurnerTrackClass))

typedef struct _BurnerTrackClass BurnerTrackClass;
typedef struct _BurnerTrack BurnerTrack;

struct _BurnerTrackClass
{
	GObjectClass parent_class;

	/* Virtual functions */
	BurnerBurnResult	(* get_status)		(BurnerTrack *track,
							 BurnerStatus *status);

	BurnerBurnResult	(* get_size)		(BurnerTrack *track,
							 goffset *blocks,
							 goffset *block_size);

	BurnerBurnResult	(* get_type)		(BurnerTrack *track,
							 BurnerTrackType *type);

	/* Signals */
	void			(* changed)		(BurnerTrack *track);
};

struct _BurnerTrack
{
	GObject parent_instance;
};

GType burner_track_get_type (void) G_GNUC_CONST;

void
burner_track_changed (BurnerTrack *track);



BurnerBurnResult
burner_track_get_size (BurnerTrack *track,
			goffset *blocks,
			goffset *bytes);

BurnerBurnResult
burner_track_get_track_type (BurnerTrack *track,
			      BurnerTrackType *type);

BurnerBurnResult
burner_track_get_status (BurnerTrack *track,
			  BurnerStatus *status);


/** 
 * Checksums
 */

typedef enum {
	BURNER_CHECKSUM_NONE			= 0,
	BURNER_CHECKSUM_DETECT			= 1,		/* means the plugin handles detection of checksum type */
	BURNER_CHECKSUM_MD5			= 1 << 1,
	BURNER_CHECKSUM_MD5_FILE		= 1 << 2,
	BURNER_CHECKSUM_SHA1			= 1 << 3,
	BURNER_CHECKSUM_SHA1_FILE		= 1 << 4,
	BURNER_CHECKSUM_SHA256			= 1 << 5,
	BURNER_CHECKSUM_SHA256_FILE		= 1 << 6,
} BurnerChecksumType;

BurnerBurnResult
burner_track_set_checksum (BurnerTrack *track,
			    BurnerChecksumType type,
			    const gchar *checksum);

const gchar *
burner_track_get_checksum (BurnerTrack *track);

BurnerChecksumType
burner_track_get_checksum_type (BurnerTrack *track);

BurnerBurnResult
burner_track_tag_add (BurnerTrack *track,
		       const gchar *tag,
		       GValue *value);

BurnerBurnResult
burner_track_tag_lookup (BurnerTrack *track,
			  const gchar *tag,
			  GValue **value);

void
burner_track_tag_copy_missing (BurnerTrack *dest,
				BurnerTrack *src);

/**
 * Convenience functions for tags
 */

BurnerBurnResult
burner_track_tag_add_string (BurnerTrack *track,
			      const gchar *tag,
			      const gchar *string);

const gchar *
burner_track_tag_lookup_string (BurnerTrack *track,
				 const gchar *tag);

BurnerBurnResult
burner_track_tag_add_int (BurnerTrack *track,
			   const gchar *tag,
			   int value);

int
burner_track_tag_lookup_int (BurnerTrack *track,
			      const gchar *tag);

G_END_DECLS

#endif /* _BURN_TRACK_H */

 
