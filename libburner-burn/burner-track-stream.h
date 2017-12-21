/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/*
 * Libburner-media
 * Copyright (C) Philippe Rouquier 2005-2009 <bonfire-app@wanadoo.fr>
 *
 * Libburner-media is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * The Libburner-media authors hereby grant permission for non-GPL compatible
 * GStreamer plugins to be used and distributed together with GStreamer
 * and Libburner-media. This permission is above and beyond the permissions granted
 * by the GPL license by which Libburner-media is covered. If you modify this code
 * you may extend this exception to your version of the code, but you are not
 * obligated to do so. If you do not wish to do so, delete this exception
 * statement from your version.
 * 
 * Libburner-media is distributed in the hope that it will be useful,
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

#ifndef _BURNER_TRACK_STREAM_H_
#define _BURNER_TRACK_STREAM_H_

#include <glib-object.h>

#include <burner-enums.h>
#include <burner-track.h>

G_BEGIN_DECLS

#define BURNER_TYPE_TRACK_STREAM             (burner_track_stream_get_type ())
#define BURNER_TRACK_STREAM(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), BURNER_TYPE_TRACK_STREAM, BurnerTrackStream))
#define BURNER_TRACK_STREAM_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), BURNER_TYPE_TRACK_STREAM, BurnerTrackStreamClass))
#define BURNER_IS_TRACK_STREAM(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BURNER_TYPE_TRACK_STREAM))
#define BURNER_IS_TRACK_STREAM_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), BURNER_TYPE_TRACK_STREAM))
#define BURNER_TRACK_STREAM_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), BURNER_TYPE_TRACK_STREAM, BurnerTrackStreamClass))

typedef struct _BurnerTrackStreamClass BurnerTrackStreamClass;
typedef struct _BurnerTrackStream BurnerTrackStream;

struct _BurnerTrackStreamClass
{
	BurnerTrackClass parent_class;

	/* Virtual functions */
	BurnerBurnResult       (*set_source)		(BurnerTrackStream *track,
							 const gchar *uri);

	BurnerBurnResult       (*set_format)		(BurnerTrackStream *track,
							 BurnerStreamFormat format);

	BurnerBurnResult       (*set_boundaries)       (BurnerTrackStream *track,
							 gint64 start,
							 gint64 end,
							 gint64 gap);
};

struct _BurnerTrackStream
{
	BurnerTrack parent_instance;
};

GType burner_track_stream_get_type (void) G_GNUC_CONST;

BurnerTrackStream *
burner_track_stream_new (void);

BurnerBurnResult
burner_track_stream_set_source (BurnerTrackStream *track,
				 const gchar *uri);

BurnerBurnResult
burner_track_stream_set_format (BurnerTrackStream *track,
				 BurnerStreamFormat format);

BurnerBurnResult
burner_track_stream_set_boundaries (BurnerTrackStream *track,
				     gint64 start,
				     gint64 end,
				     gint64 gap);

gchar *
burner_track_stream_get_source (BurnerTrackStream *track,
				 gboolean uri);

BurnerBurnResult
burner_track_stream_get_length (BurnerTrackStream *track,
				 guint64 *length);

guint64
burner_track_stream_get_start (BurnerTrackStream *track);

guint64
burner_track_stream_get_end (BurnerTrackStream *track);

guint64
burner_track_stream_get_gap (BurnerTrackStream *track);

BurnerStreamFormat
burner_track_stream_get_format (BurnerTrackStream *track);

G_END_DECLS

#endif /* _BURNER_TRACK_STREAM_H_ */
