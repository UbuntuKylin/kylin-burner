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

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <stdio.h>

#include <errno.h>
#include <string.h>

#include <glib.h>

#ifndef _BURN_ERROR_H
#define _BURN_ERROR_H

G_BEGIN_DECLS

typedef enum {
	BURNER_SCSI_ERROR_NONE	   = 0,
	BURNER_SCSI_ERR_UNKNOWN,
	BURNER_SCSI_SIZE_MISMATCH,
	BURNER_SCSI_TYPE_MISMATCH,
	BURNER_SCSI_BAD_ARGUMENT,
	BURNER_SCSI_NOT_READY,
	BURNER_SCSI_OUTRANGE_ADDRESS,
	BURNER_SCSI_INVALID_ADDRESS,
	BURNER_SCSI_INVALID_COMMAND,
	BURNER_SCSI_INVALID_PARAMETER,
	BURNER_SCSI_INVALID_FIELD,
	BURNER_SCSI_TIMEOUT,
	BURNER_SCSI_KEY_NOT_ESTABLISHED,
	BURNER_SCSI_INVALID_TRACK_MODE,
	BURNER_SCSI_ERRNO,
	BURNER_SCSI_NO_MEDIUM,
	BURNER_SCSI_ERROR_LAST
} BurnerScsiErrCode;

typedef enum {
	BURNER_SCSI_OK,
	BURNER_SCSI_FAILURE,
	BURNER_SCSI_RECOVERABLE,
} BurnerScsiResult;

const gchar *
burner_scsi_strerror (BurnerScsiErrCode code);

void
burner_scsi_set_error (GError **error, BurnerScsiErrCode code);

G_END_DECLS

#endif /* _BURN_ERROR_H */
