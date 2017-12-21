/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/*
 * Libburner-misc
 * Copyright (C) Philippe Rouquier 2005-2009 <bonfire-app@wanadoo.fr>
 *
 * Libburner-misc is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * The Libburner-misc authors hereby grant permission for non-GPL compatible
 * GStreamer plugins to be used and distributed together with GStreamer
 * and Libburner-misc. This permission is above and beyond the permissions granted
 * by the GPL license by which Libburner-burn is covered. If you modify this code
 * you may extend this exception to your version of the code, but you are not
 * obligated to do so. If you do not wish to do so, delete this exception
 * statement from your version.
 * 
 * Libburner-misc is distributed in the hope that it will be useful,
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

#ifndef _METADATA_H
#define _METADATA_H

#include <glib.h>
#include <glib-object.h>
#include <gio/gio.h>

#include <gdk-pixbuf/gdk-pixbuf.h>

#include <gst/gst.h>

G_BEGIN_DECLS

typedef enum {
	BURNER_METADATA_FLAG_NONE			= 0,
	BURNER_METADATA_FLAG_SILENCES			= 1 << 1,
	BURNER_METADATA_FLAG_MISSING			= 1 << 2,
	BURNER_METADATA_FLAG_THUMBNAIL			= 1 << 3
} BurnerMetadataFlag;

#define BURNER_TYPE_METADATA         (burner_metadata_get_type ())
#define BURNER_METADATA(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), BURNER_TYPE_METADATA, BurnerMetadata))
#define BURNER_METADATA_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k), BURNER_TYPE_METADATA, BurnerMetadataClass))
#define BURNER_IS_METADATA(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), BURNER_TYPE_METADATA))
#define BURNER_IS_METADATA_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), BURNER_TYPE_METADATA))
#define BURNER_METADATA_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), BURNER_TYPE_METADATA, BurnerMetadataClass))

typedef struct {
	gint64 start;
	gint64 end;
} BurnerMetadataSilence;

typedef struct {
	gchar *uri;
	gchar *type;
	gchar *title;
	gchar *artist;
	gchar *album;
	gchar *genre;
	gchar *composer;
	gchar *musicbrainz_id;
	gchar *isrc;
	guint64 len;

	gint channels;
	gint rate;

	GSList *silences;

	GdkPixbuf *snapshot;

	guint is_seekable:1;
	guint has_audio:1;
	guint has_video:1;
	guint has_dts:1;
} BurnerMetadataInfo;

void
burner_metadata_info_copy (BurnerMetadataInfo *dest,
			    BurnerMetadataInfo *src);
void
burner_metadata_info_clear (BurnerMetadataInfo *info);
void
burner_metadata_info_free (BurnerMetadataInfo *info);

typedef struct _BurnerMetadataClass BurnerMetadataClass;
typedef struct _BurnerMetadata BurnerMetadata;

struct _BurnerMetadataClass {
	GObjectClass parent_class;

	void		(*completed)	(BurnerMetadata *meta,
					 const GError *error);
	void		(*progress)	(BurnerMetadata *meta,
					 gdouble progress);

};

struct _BurnerMetadata {
	GObject parent;
};

GType burner_metadata_get_type (void) G_GNUC_CONST;

BurnerMetadata *burner_metadata_new (void);

gboolean
burner_metadata_get_info_async (BurnerMetadata *metadata,
				 const gchar *uri,
				 BurnerMetadataFlag flags);

void
burner_metadata_cancel (BurnerMetadata *metadata);

gboolean
burner_metadata_set_uri (BurnerMetadata *metadata,
			  BurnerMetadataFlag flags,
			  const gchar *uri,
			  GError **error);

void
burner_metadata_wait (BurnerMetadata *metadata,
		       GCancellable *cancel);
void
burner_metadata_increase_listener_number (BurnerMetadata *metadata);

gboolean
burner_metadata_decrease_listener_number (BurnerMetadata *metadata);

const gchar *
burner_metadata_get_uri (BurnerMetadata *metadata);

BurnerMetadataFlag
burner_metadata_get_flags (BurnerMetadata *metadata);

gboolean
burner_metadata_get_result (BurnerMetadata *metadata,
			     BurnerMetadataInfo *info,
			     GError **error);

typedef int	(*BurnerMetadataGetXidCb)	(gpointer user_data);

void
burner_metadata_set_get_xid_callback (BurnerMetadata *metadata,
                                       BurnerMetadataGetXidCb callback,
                                       gpointer user_data);
G_END_DECLS

#endif				/* METADATA_H */
