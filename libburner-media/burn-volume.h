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

#include <glib.h>

#ifndef BURN_VOLUME_H
#define BURN_VOLUME_H

G_BEGIN_DECLS

#include "burn-volume-source.h"

struct _BurnerVolDesc {
	guchar type;
	gchar id			[5];
	guchar version;
};
typedef struct _BurnerVolDesc BurnerVolDesc;

struct _BurnerVolFileExtent {
	guint block;
	guint size;
};
typedef struct _BurnerVolFileExtent BurnerVolFileExtent;

typedef struct _BurnerVolFile BurnerVolFile;
struct _BurnerVolFile {
	BurnerVolFile *parent;

	gchar *name;
	gchar *rr_name;

	union {

	struct {
		GSList *extents;
		guint64 size_bytes;
	} file;

	struct {
		GList *children;
		guint address;
	} dir;

	} specific;

	guint isdir:1;
	guint isdir_loaded:1;

	/* mainly used internally */
	guint has_RR:1;
	guint relocated:1;
};

gboolean
burner_volume_is_valid (BurnerVolSrc *src,
			 GError **error);

gboolean
burner_volume_get_size (BurnerVolSrc *src,
			 gint64 block,
			 gint64 *nb_blocks,
			 GError **error);

BurnerVolFile *
burner_volume_get_files (BurnerVolSrc *src,
			  gint64 block,
			  gchar **label,
			  gint64 *nb_blocks,
			  gint64 *data_blocks,
			  GError **error);

BurnerVolFile *
burner_volume_get_file (BurnerVolSrc *src,
			 const gchar *path,
			 gint64 volume_start_block,
			 GError **error);

GList *
burner_volume_load_directory_contents (BurnerVolSrc *vol,
					gint64 session_block,
					gint64 block,
					GError **error);


#define BURNER_VOLUME_FILE_NAME(file)			((file)->rr_name?(file)->rr_name:(file)->name)
#define BURNER_VOLUME_FILE_SIZE(file)			((file)->isdir?0:(file)->specific.file.size_bytes)

void
burner_volume_file_free (BurnerVolFile *file);

gchar *
burner_volume_file_to_path (BurnerVolFile *file);

BurnerVolFile *
burner_volume_file_from_path (const gchar *ptr,
			       BurnerVolFile *parent);

gint64
burner_volume_file_size (BurnerVolFile *file);

BurnerVolFile *
burner_volume_file_merge (BurnerVolFile *file1,
			   BurnerVolFile *file2);

G_END_DECLS

#endif /* BURN_VOLUME_H */
