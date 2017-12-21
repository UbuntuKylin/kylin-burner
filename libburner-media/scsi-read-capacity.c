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

#include "scsi-mmc2.h"

#include "scsi-error.h"
#include "scsi-base.h"
#include "scsi-command.h"
#include "scsi-opcodes.h"
#include "scsi-read-capacity.h"

#if G_BYTE_ORDER == G_LITTLE_ENDIAN

struct _BurnerReadCapacityCDB {
	uchar opcode;

	uchar relative_address		:1;
	uchar reserved0			:7;

	uchar lba			[4];
	uchar reserved1			[2];

	uchar pmi			:1;
	uchar reserved2			:7;

	uchar ctl;
};

#else

struct _BurnerReadCapacityCDB {
	uchar opcode;

	uchar reserved0			:7;
	uchar relative_address		:1;

	uchar lba			[4];
	uchar reserved1			[2];

	uchar reserved2			:7;
	uchar pmi			:1;

	uchar ctl;
};

#endif

typedef struct _BurnerReadCapacityCDB BurnerReadCapacityCDB;

BURNER_SCSI_COMMAND_DEFINE (BurnerReadCapacityCDB,
			     READ_CAPACITY,
			     BURNER_SCSI_READ);

BurnerScsiResult
burner_mmc2_read_capacity (BurnerDeviceHandle *handle,
			    BurnerScsiReadCapacityData *data,
			    int size,
			    BurnerScsiErrCode *error)
{
	BurnerReadCapacityCDB *cdb;
	BurnerScsiResult res;

	g_return_val_if_fail (handle != NULL, BURNER_SCSI_FAILURE);

	/* NOTE: all the fields are ignored by MM drives */
	cdb = burner_scsi_command_new (&info, handle);

	memset (data, 0, size);
	res = burner_scsi_command_issue_sync (cdb, data, size, error);
	burner_scsi_command_free (cdb);

	return res;
}
