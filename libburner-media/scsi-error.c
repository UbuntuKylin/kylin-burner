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

#include <errno.h>

#include <glib.h>
#include <glib/gi18n-lib.h>

#include "burner-media.h"
#include "scsi-error.h"

static const gchar *error_string [] = {	N_("Unknown error"),
					N_("Size mismatch"),
					N_("Type mismatch"),
					N_("Bad argument"),
					N_("The drive is busy"),
					N_("Outrange address"),
					N_("Invalid address"),
					N_("Invalid command"),
					N_("Invalid parameter in command"),
					N_("Invalid field in command"),
					N_("The device timed out"),
					N_("Key not established"),
					N_("Invalid track mode"),
					NULL	};	/* errno */

const gchar *
burner_scsi_strerror (BurnerScsiErrCode code)
{
	if (code > BURNER_SCSI_ERROR_LAST || code < 0)
		return NULL;

	/* FIXME: this is for errors that don't have any message yet */
	if (code > BURNER_SCSI_ERRNO)
		return NULL;

	if (code == BURNER_SCSI_ERRNO)
		return g_strerror (errno);

	return _(error_string [code]);
}

void
burner_scsi_set_error (GError **error, BurnerScsiErrCode code)
{
	g_set_error (error,
		     BURNER_MEDIA_ERROR,
		     BURNER_MEDIA_ERROR_GENERAL,
		     "%s",
		     burner_scsi_strerror (code));
}
