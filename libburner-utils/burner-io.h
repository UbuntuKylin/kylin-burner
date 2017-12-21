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

#ifndef _BURNER_IO_H_
#define _BURNER_IO_H_

#include <glib-object.h>
#include <gtk/gtk.h>

#include "burner-async-task-manager.h"

G_BEGIN_DECLS

typedef enum {
	BURNER_IO_INFO_NONE			= 0,
	BURNER_IO_INFO_MIME			= 1,
	BURNER_IO_INFO_ICON			= 1,
	BURNER_IO_INFO_PERM			= 1 << 1,
	BURNER_IO_INFO_METADATA		= 1 << 2,
	BURNER_IO_INFO_METADATA_THUMBNAIL	= 1 << 3,
	BURNER_IO_INFO_RECURSIVE		= 1 << 4,
	BURNER_IO_INFO_CHECK_PARENT_SYMLINK	= 1 << 5,
	BURNER_IO_INFO_METADATA_MISSING_CODEC	= 1 << 6,

	BURNER_IO_INFO_FOLLOW_SYMLINK		= 1 << 7,

	BURNER_IO_INFO_URGENT			= 1 << 9,
	BURNER_IO_INFO_IDLE			= 1 << 10
} BurnerIOFlags;


typedef enum {
	BURNER_IO_PHASE_START		= 0,
	BURNER_IO_PHASE_DOWNLOAD,
	BURNER_IO_PHASE_END
} BurnerIOPhase;

#define BURNER_IO_XFER_DESTINATION	"xfer::destination"

#define BURNER_IO_PLAYLIST_TITLE	"playlist::title"
#define BURNER_IO_IS_PLAYLIST		"playlist::is_playlist"
#define BURNER_IO_PLAYLIST_ENTRIES_NUM	"playlist::entries_num"

#define BURNER_IO_COUNT_NUM		"count::num"
#define BURNER_IO_COUNT_SIZE		"count::size"
#define BURNER_IO_COUNT_INVALID	"count::invalid"

#define BURNER_IO_THUMBNAIL		"metadata::thumbnail"

#define BURNER_IO_LEN			"metadata::length"
#define BURNER_IO_ISRC			"metadata::isrc"
#define BURNER_IO_TITLE		"metadata::title"
#define BURNER_IO_ARTIST		"metadata::artist"
#define BURNER_IO_ALBUM		"metadata::album"
#define BURNER_IO_GENRE		"metadata::genre"
#define BURNER_IO_COMPOSER		"metadata::composer"
#define BURNER_IO_HAS_AUDIO		"metadata::has_audio"
#define BURNER_IO_HAS_VIDEO		"metadata::has_video"
#define BURNER_IO_IS_SEEKABLE		"metadata::is_seekable"

#define BURNER_IO_HAS_DTS			"metadata::audio::wav::has_dts"

#define BURNER_IO_CHANNELS		"metadata::audio::channels"
#define BURNER_IO_RATE				"metadata::audio::rate"

#define BURNER_IO_DIR_CONTENTS_ADDR	"image::directory::address"

typedef struct _BurnerIOJobProgress BurnerIOJobProgress;

typedef void		(*BurnerIOResultCallback)	(GObject *object,
							 GError *error,
							 const gchar *uri,
							 GFileInfo *info,
							 gpointer callback_data);

typedef void		(*BurnerIOProgressCallback)	(GObject *object,
							 BurnerIOJobProgress *info,
							 gpointer callback_data);

typedef void		(*BurnerIODestroyCallback)	(GObject *object,
							 gboolean cancel,
							 gpointer callback_data);

typedef gboolean	(*BurnerIOCompareCallback)	(gpointer data,
							 gpointer user_data);


struct _BurnerIOJobCallbacks {
	BurnerIOResultCallback callback;
	BurnerIODestroyCallback destroy;
	BurnerIOProgressCallback progress;

	guint ref;

	/* Whether we are returning something for this base */
	guint in_use:1;
};
typedef struct _BurnerIOJobCallbacks BurnerIOJobCallbacks;

struct _BurnerIOJobBase {
	GObject *object;
	BurnerIOJobCallbacks *methods;
};
typedef struct _BurnerIOJobBase BurnerIOJobBase;

struct _BurnerIOResultCallbackData {
	gpointer callback_data;
	gint ref;
};
typedef struct _BurnerIOResultCallbackData BurnerIOResultCallbackData;

struct _BurnerIOJob {
	gchar *uri;
	BurnerIOFlags options;

	const BurnerIOJobBase *base;
	BurnerIOResultCallbackData *callback_data;
};
typedef struct _BurnerIOJob BurnerIOJob;

#define BURNER_IO_JOB(data)	((BurnerIOJob *) (data))

void
burner_io_job_free (gboolean cancelled,
		     BurnerIOJob *job);

void
burner_io_set_job (BurnerIOJob *self,
		    const BurnerIOJobBase *base,
		    const gchar *uri,
		    BurnerIOFlags options,
		    BurnerIOResultCallbackData *callback_data);

void
burner_io_push_job (BurnerIOJob *job,
		     const BurnerAsyncTaskType *type);

void
burner_io_return_result (const BurnerIOJobBase *base,
			  const gchar *uri,
			  GFileInfo *info,
			  GError *error,
			  BurnerIOResultCallbackData *callback_data);


typedef GtkWindow *	(* BurnerIOGetParentWinCb)	(gpointer user_data);

void
burner_io_set_parent_window_callback (BurnerIOGetParentWinCb callback,
                                       gpointer user_data);

void
burner_io_shutdown (void);

/* NOTE: The split in methods and objects was
 * done to prevent jobs sharing the same methods
 * to return their results concurently. In other
 * words only one job among those sharing the
 * same methods can return its results. */
 
BurnerIOJobBase *
burner_io_register (GObject *object,
		     BurnerIOResultCallback callback,
		     BurnerIODestroyCallback destroy,
		     BurnerIOProgressCallback progress);

BurnerIOJobBase *
burner_io_register_with_methods (GObject *object,
                                  BurnerIOJobCallbacks *methods);

BurnerIOJobCallbacks *
burner_io_register_job_methods (BurnerIOResultCallback callback,
                                 BurnerIODestroyCallback destroy,
                                 BurnerIOProgressCallback progress);

void
burner_io_job_base_free (BurnerIOJobBase *base);

void
burner_io_cancel_by_base (BurnerIOJobBase *base);

void
burner_io_find_urgent (const BurnerIOJobBase *base,
			BurnerIOCompareCallback callback,
			gpointer callback_data);			

void
burner_io_load_directory (const gchar *uri,
			   const BurnerIOJobBase *base,
			   BurnerIOFlags options,
			   gpointer callback_data);
void
burner_io_get_file_info (const gchar *uri,
			  const BurnerIOJobBase *base,
			  BurnerIOFlags options,
			  gpointer callback_data);
void
burner_io_get_file_count (GSList *uris,
			   const BurnerIOJobBase *base,
			   BurnerIOFlags options,
			   gpointer callback_data);
void
burner_io_parse_playlist (const gchar *uri,
			   const BurnerIOJobBase *base,
			   BurnerIOFlags options,
			   gpointer callback_data);

guint64
burner_io_job_progress_get_read (BurnerIOJobProgress *progress);

guint64
burner_io_job_progress_get_total (BurnerIOJobProgress *progress);

BurnerIOPhase
burner_io_job_progress_get_phase (BurnerIOJobProgress *progress);

guint
burner_io_job_progress_get_file_processed (BurnerIOJobProgress *progress);

G_END_DECLS

#endif /* _BURNER_IO_H_ */
