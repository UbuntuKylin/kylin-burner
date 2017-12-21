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

#include "scsi-mmc1.h"

#include "scsi-error.h"
#include "scsi-utils.h"
#include "scsi-base.h"
#include "scsi-command.h"
#include "scsi-opcodes.h"
#include "scsi-read-track-information.h"

#if G_BYTE_ORDER == G_LITTLE_ENDIAN

struct _BurnerRdTrackInfoCDB {
	uchar opcode;

	uchar addr_num_type		:2;
	uchar open			:1;	/* MMC5 field only */
	uchar reserved0			:5;

	uchar blk_addr_trk_ses_num	[4];

	uchar reserved1;

	uchar alloc_len			[2];
	uchar ctl;
};

#else

struct _BurnerRdTrackInfoCDB {
	uchar opcode;

	uchar reserved0			:5;
	uchar open			:1;
	uchar addr_num_type		:2;

	uchar blk_addr_trk_ses_num	[4];

	uchar reserved1;

	uchar alloc_len			[2];
	uchar ctl;
};

#endif

typedef struct _BurnerRdTrackInfoCDB BurnerRdTrackInfoCDB;

BURNER_SCSI_COMMAND_DEFINE (BurnerRdTrackInfoCDB,
			     READ_TRACK_INFORMATION,
			     BURNER_SCSI_READ);

typedef enum {
BURNER_FIELD_LBA			= 0x00,
BURNER_FIELD_TRACK_NUM			= 0x01,
BURNER_FIELD_SESSION_NUM		= 0x02,
	/* reserved */
} BurnerFieldType;

#define BURNER_SCSI_INCOMPLETE_TRACK	0xFF

static BurnerScsiResult
burner_read_track_info (BurnerRdTrackInfoCDB *cdb,
			 BurnerScsiTrackInfo *info,
			 int *size,
			 BurnerScsiErrCode *error)
{
	BurnerScsiTrackInfo hdr;
	BurnerScsiResult res;
	int datasize;

	if (!info || !size) {
		BURNER_SCSI_SET_ERRCODE (error, BURNER_SCSI_BAD_ARGUMENT);
		return BURNER_SCSI_FAILURE;
	}

	/* first ask the drive how long should the data be and then ... */
	datasize = 4;
	memset (&hdr, 0, sizeof (info));
	BURNER_SET_16 (cdb->alloc_len, datasize);
	res = burner_scsi_command_issue_sync (cdb, &hdr, datasize, error);
	if (res)
		return res;

	/* ... check the size in case of a buggy firmware ... */
	if (BURNER_GET_16 (hdr.len) + sizeof (hdr.len) >= datasize) {
		datasize = BURNER_GET_16 (hdr.len) + sizeof (hdr.len);

		if (datasize > *size) {
			/* it must not be over sizeof (BurnerScsiTrackInfo) */
			if (datasize > sizeof (BurnerScsiTrackInfo)) {
				BURNER_MEDIA_LOG ("Oversized data received (%i) setting to %i", datasize, *size);
				datasize = *size;
			}
			else
				*size = datasize;
		}
		else if (*size < datasize) {
			BURNER_MEDIA_LOG ("Oversized data required (%i) setting to %i", *size, datasize);
			*size = datasize;
		}
	}
	else {
		BURNER_MEDIA_LOG ("Undersized data received (%i) setting to %i", datasize, *size);
		datasize = *size;
	}

	/* ... and re-issue the command */
	memset (info, 0, sizeof (BurnerScsiTrackInfo));
	BURNER_SET_16 (cdb->alloc_len, datasize);
	res = burner_scsi_command_issue_sync (cdb, info, datasize, error);
	if (res == BURNER_SCSI_OK) {
		if (datasize != BURNER_GET_16 (info->len) + sizeof (info->len))
			BURNER_MEDIA_LOG ("Sizes mismatch asked %i / received %i",
					  datasize,
					  BURNER_GET_16 (info->len) + sizeof (info->len));

		*size = MIN (datasize, BURNER_GET_16 (info->len) + sizeof (info->len));
	}

	return res;
}

/**
 * 
 * NOTE: if media is a CD and track_num = 0 then returns leadin
 * but since the other media don't have a leadin they error out.
 * if track_num = 255 returns last incomplete track.
 */
 
BurnerScsiResult
burner_mmc1_read_track_info (BurnerDeviceHandle *handle,
			      int track_num,
			      BurnerScsiTrackInfo *track_info,
			      int *size,
			      BurnerScsiErrCode *error)
{
	BurnerRdTrackInfoCDB *cdb;
	BurnerScsiResult res;

	g_return_val_if_fail (handle != NULL, BURNER_SCSI_FAILURE);

	cdb = burner_scsi_command_new (&info, handle);
	cdb->addr_num_type = BURNER_FIELD_TRACK_NUM;
	BURNER_SET_32 (cdb->blk_addr_trk_ses_num, track_num);

	res = burner_read_track_info (cdb, track_info, size, error);
	burner_scsi_command_free (cdb);

	return res;
}
