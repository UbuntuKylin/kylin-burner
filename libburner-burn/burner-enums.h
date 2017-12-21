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

#ifndef _BURNER_ENUM_H_
#define _BURNER_ENUM_H_

#include <glib.h>

G_BEGIN_DECLS

typedef enum {
	BURNER_BURN_OK,
	BURNER_BURN_ERR,
	BURNER_BURN_RETRY,
	BURNER_BURN_CANCEL,
	BURNER_BURN_RUNNING,
	BURNER_BURN_DANGEROUS,
	BURNER_BURN_NOT_READY,
	BURNER_BURN_NOT_RUNNING,
	BURNER_BURN_NEED_RELOAD,
	BURNER_BURN_NOT_SUPPORTED,
} BurnerBurnResult;

/* These flags are sorted by importance. That's done to solve the problem of
 * exclusive flags: that way MERGE will always win over any other flag if they
 * are exclusive. On the other hand DAO will always lose. */
typedef enum {
	BURNER_BURN_FLAG_NONE			= 0,

	/* These flags should always be supported */
	BURNER_BURN_FLAG_CHECK_SIZE		= 1,
	BURNER_BURN_FLAG_NOGRACE		= 1 << 1,
	BURNER_BURN_FLAG_EJECT			= 1 << 2,

	/* These are of great importance for the result */
	BURNER_BURN_FLAG_MERGE			= 1 << 3,
	BURNER_BURN_FLAG_MULTI			= 1 << 4,
	BURNER_BURN_FLAG_APPEND		= 1 << 5,

	BURNER_BURN_FLAG_BURNPROOF		= 1 << 6,
	BURNER_BURN_FLAG_NO_TMP_FILES		= 1 << 7,
	BURNER_BURN_FLAG_DUMMY			= 1 << 8,

	BURNER_BURN_FLAG_OVERBURN		= 1 << 9,

	BURNER_BURN_FLAG_BLANK_BEFORE_WRITE	= 1 << 10,
	BURNER_BURN_FLAG_FAST_BLANK		= 1 << 11,

	/* NOTE: these two are contradictory? */
	BURNER_BURN_FLAG_DAO			= 1 << 13,
	BURNER_BURN_FLAG_RAW			= 1 << 14,

	BURNER_BURN_FLAG_LAST
} BurnerBurnFlag;

typedef enum {
	BURNER_BURN_ACTION_NONE		= 0,
	BURNER_BURN_ACTION_GETTING_SIZE,
	BURNER_BURN_ACTION_CREATING_IMAGE,
	BURNER_BURN_ACTION_RECORDING,
	BURNER_BURN_ACTION_BLANKING,
	BURNER_BURN_ACTION_CHECKSUM,
	BURNER_BURN_ACTION_DRIVE_COPY,
	BURNER_BURN_ACTION_FILE_COPY,
	BURNER_BURN_ACTION_ANALYSING,
	BURNER_BURN_ACTION_TRANSCODING,
	BURNER_BURN_ACTION_PREPARING,
	BURNER_BURN_ACTION_LEADIN,
	BURNER_BURN_ACTION_RECORDING_CD_TEXT,
	BURNER_BURN_ACTION_FIXATING,
	BURNER_BURN_ACTION_LEADOUT,
	BURNER_BURN_ACTION_START_RECORDING,
	BURNER_BURN_ACTION_FINISHED,
	BURNER_BURN_ACTION_EJECTING,
	BURNER_BURN_ACTION_LAST
} BurnerBurnAction;

typedef enum {
	BURNER_IMAGE_FS_NONE			= 0,
	BURNER_IMAGE_FS_ISO			= 1,
	BURNER_IMAGE_FS_UDF			= 1 << 1,
	BURNER_IMAGE_FS_JOLIET			= 1 << 2,
	BURNER_IMAGE_FS_VIDEO			= 1 << 3,

	/* The following one conflict with UDF and JOLIET */
	BURNER_IMAGE_FS_SYMLINK		= 1 << 4,

	BURNER_IMAGE_ISO_FS_LEVEL_3		= 1 << 5,
	BURNER_IMAGE_ISO_FS_DEEP_DIRECTORY	= 1 << 6,
	BURNER_IMAGE_FS_ANY			= BURNER_IMAGE_FS_ISO|
						  BURNER_IMAGE_FS_UDF|
						  BURNER_IMAGE_FS_JOLIET|
						  BURNER_IMAGE_FS_SYMLINK|
						  BURNER_IMAGE_ISO_FS_LEVEL_3|
						  BURNER_IMAGE_FS_VIDEO|
						  BURNER_IMAGE_ISO_FS_DEEP_DIRECTORY
} BurnerImageFS;

typedef enum {
	BURNER_AUDIO_FORMAT_NONE		= 0,
	BURNER_AUDIO_FORMAT_UNDEFINED		= 1,
	BURNER_AUDIO_FORMAT_DTS		= 1 << 1,
	BURNER_AUDIO_FORMAT_RAW		= 1 << 2,
	BURNER_AUDIO_FORMAT_AC3		= 1 << 3,
	BURNER_AUDIO_FORMAT_MP2		= 1 << 4,

	BURNER_AUDIO_FORMAT_44100		= 1 << 5,
	BURNER_AUDIO_FORMAT_48000		= 1 << 6,


	BURNER_VIDEO_FORMAT_UNDEFINED		= 1 << 7,
	BURNER_VIDEO_FORMAT_VCD		= 1 << 8,
	BURNER_VIDEO_FORMAT_VIDEO_DVD		= 1 << 9,

	BURNER_METADATA_INFO			= 1 << 10,

	BURNER_AUDIO_FORMAT_RAW_LITTLE_ENDIAN  = 1 << 11,
} BurnerStreamFormat;

#define BURNER_STREAM_FORMAT_AUDIO(stream_FORMAT)	((stream_FORMAT) & 0x087F)
#define BURNER_STREAM_FORMAT_VIDEO(stream_FORMAT)	((stream_FORMAT) & 0x0380)

#define	BURNER_MIN_STREAM_LENGTH			((gint64) 6 * 1000000000LL)
#define BURNER_STREAM_LENGTH(start_MACRO, end_MACRO)					\
	((end_MACRO) - (start_MACRO) > BURNER_MIN_STREAM_LENGTH) ?			\
	((end_MACRO) - (start_MACRO)) : BURNER_MIN_STREAM_LENGTH

#define BURNER_STREAM_FORMAT_HAS_VIDEO(format_MACRO)				\
	 ((format_MACRO) & (BURNER_VIDEO_FORMAT_UNDEFINED|			\
	 		    BURNER_VIDEO_FORMAT_VCD|				\
	 		    BURNER_VIDEO_FORMAT_VIDEO_DVD))

typedef enum {
	BURNER_IMAGE_FORMAT_NONE = 0,
	BURNER_IMAGE_FORMAT_BIN = 1,
	BURNER_IMAGE_FORMAT_CUE = 1 << 1,
	BURNER_IMAGE_FORMAT_CLONE = 1 << 2,
	BURNER_IMAGE_FORMAT_CDRDAO = 1 << 3,
	BURNER_IMAGE_FORMAT_ANY = BURNER_IMAGE_FORMAT_BIN|
	BURNER_IMAGE_FORMAT_CUE|
	BURNER_IMAGE_FORMAT_CDRDAO|
	BURNER_IMAGE_FORMAT_CLONE,
} BurnerImageFormat; 

typedef enum {
	BURNER_PLUGIN_ERROR_NONE					= 0,
	BURNER_PLUGIN_ERROR_MODULE,
	BURNER_PLUGIN_ERROR_MISSING_APP,
	BURNER_PLUGIN_ERROR_WRONG_APP_VERSION,
	BURNER_PLUGIN_ERROR_SYMBOLIC_LINK_APP,
	BURNER_PLUGIN_ERROR_MISSING_LIBRARY,
	BURNER_PLUGIN_ERROR_LIBRARY_VERSION,
	BURNER_PLUGIN_ERROR_MISSING_GSTREAMER_PLUGIN,
} BurnerPluginErrorType;

G_END_DECLS

#endif

