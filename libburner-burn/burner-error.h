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

#ifndef _BURNER_ERROR_H_
#define _BURNER_ERROR_H_

#include <glib.h>

G_BEGIN_DECLS

typedef enum {
	BURNER_BURN_ERROR_NONE,
	BURNER_BURN_ERROR_GENERAL,

	BURNER_BURN_ERROR_PLUGIN_MISBEHAVIOR,

	BURNER_BURN_ERROR_SLOW_DMA,
	BURNER_BURN_ERROR_PERMISSION,
	BURNER_BURN_ERROR_DRIVE_BUSY,
	BURNER_BURN_ERROR_DISK_SPACE,

	BURNER_BURN_ERROR_EMPTY,
	BURNER_BURN_ERROR_INPUT_INVALID,

	BURNER_BURN_ERROR_OUTPUT_NONE,

	BURNER_BURN_ERROR_FILE_INVALID,
	BURNER_BURN_ERROR_FILE_FOLDER,
	BURNER_BURN_ERROR_FILE_PLAYLIST,
	BURNER_BURN_ERROR_FILE_NOT_FOUND,
	BURNER_BURN_ERROR_FILE_NOT_LOCAL,

	BURNER_BURN_ERROR_WRITE_MEDIUM,
	BURNER_BURN_ERROR_WRITE_IMAGE,

	BURNER_BURN_ERROR_IMAGE_INVALID,
	BURNER_BURN_ERROR_IMAGE_JOLIET,
	BURNER_BURN_ERROR_IMAGE_LAST_SESSION,

	BURNER_BURN_ERROR_MEDIUM_NONE,
	BURNER_BURN_ERROR_MEDIUM_INVALID,
	BURNER_BURN_ERROR_MEDIUM_SPACE,
	BURNER_BURN_ERROR_MEDIUM_NO_DATA,
	BURNER_BURN_ERROR_MEDIUM_NOT_WRITABLE,
	BURNER_BURN_ERROR_MEDIUM_NOT_REWRITABLE,
	BURNER_BURN_ERROR_MEDIUM_NEED_RELOADING,

	BURNER_BURN_ERROR_BAD_CHECKSUM,

	BURNER_BURN_ERROR_MISSING_APP_AND_PLUGIN,

	/* these are not necessarily error */
	BURNER_BURN_WARNING_CHECKSUM,
	BURNER_BURN_WARNING_INSERT_AFTER_COPY,

	BURNER_BURN_ERROR_TMP_DIRECTORY,
	BURNER_BURN_ERROR_ENCRYPTION_KEY
} BurnerBurnError;

/**
 * Error handling and defined return values
 */

GQuark burner_burn_quark (void);

#define BURNER_BURN_ERROR				\
	burner_burn_quark()

G_END_DECLS

#endif /* _BURNER_ERROR_H_ */

