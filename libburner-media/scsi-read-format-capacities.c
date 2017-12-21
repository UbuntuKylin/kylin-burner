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

#include <glib.h>

#include "burner-media-private.h"

#include "scsi-mmc2.h"

#include "scsi-error.h"
#include "scsi-utils.h"
#include "scsi-base.h"
#include "scsi-command.h"
#include "scsi-opcodes.h"
#include "scsi-read-format-capacities.h"

struct _BurnerRdFormatCapacitiesCDB {
	uchar opcode;
	uchar reserved		[6];
	uchar alloc_len		[2];
	uchar ctl;
};

typedef struct _BurnerRdFormatCapacitiesCDB BurnerRdFormatCapacitiesCDB;

BURNER_SCSI_COMMAND_DEFINE (BurnerRdFormatCapacitiesCDB,
			     READ_FORMAT_CAPACITIES,
			     BURNER_SCSI_READ);

BurnerScsiResult
burner_mmc2_read_format_capacities (BurnerDeviceHandle *handle,
				     BurnerScsiFormatCapacitiesHdr **data,
				     int *size,
				     BurnerScsiErrCode *error)
{
	BurnerScsiFormatCapacitiesHdr *buffer;
	BurnerScsiFormatCapacitiesHdr hdr;
	BurnerRdFormatCapacitiesCDB *cdb;
	BurnerScsiResult res;
	int request_size;

	g_return_val_if_fail (handle != NULL, BURNER_SCSI_FAILURE);

	if (!data || !size) {
		BURNER_SCSI_SET_ERRCODE (error, BURNER_SCSI_BAD_ARGUMENT);
		return BURNER_SCSI_FAILURE;
	}

	cdb = burner_scsi_command_new (&info, handle);
	BURNER_SET_16 (cdb->alloc_len, sizeof (BurnerScsiFormatCapacitiesHdr));

	memset (&hdr, 0, sizeof (BurnerScsiFormatCapacitiesHdr));
	res = burner_scsi_command_issue_sync (cdb,
					       &hdr,
					       sizeof (BurnerScsiFormatCapacitiesHdr),
					       error);
	if (res)
		goto end;

	request_size = hdr.len + sizeof (hdr.len) + G_STRUCT_OFFSET (BurnerScsiFormatCapacitiesHdr, len);
	buffer = (BurnerScsiFormatCapacitiesHdr *) g_new0 (uchar, request_size);

	BURNER_SET_16 (cdb->alloc_len, request_size);
	res = burner_scsi_command_issue_sync (cdb, buffer, request_size, error);
	if (res) {
		g_free (buffer);
		goto end;
	}

	if (request_size != buffer->len + sizeof (buffer->len) + G_STRUCT_OFFSET (BurnerScsiFormatCapacitiesHdr, len)) {
		BURNER_SCSI_SET_ERRCODE (error, BURNER_SCSI_SIZE_MISMATCH);

		res = BURNER_SCSI_FAILURE;
		g_free (buffer);
		goto end;
	}

	*data = buffer;
	*size = request_size;

end:

	burner_scsi_command_free (cdb);
	return res;
}
