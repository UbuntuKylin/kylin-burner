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
#include "scsi-mmc3.h"

#include "scsi-error.h"
#include "scsi-utils.h"
#include "scsi-base.h"
#include "scsi-command.h"
#include "scsi-opcodes.h"
#include "scsi-read-toc-pma-atip.h"

#if G_BYTE_ORDER == G_LITTLE_ENDIAN

struct _BurnerRdTocPmaAtipCDB {
	uchar opcode;

	uchar reserved0		:1;
	uchar msf		:1;
	uchar reserved1		:6;

	uchar format		:4;
	uchar reserved2		:4;

	uchar reserved3		[3];
	uchar track_session_num;

	uchar alloc_len		[2];
	uchar ctl;
};

#else

struct _BurnerRdTocPmaAtipCDB {
	uchar opcode;

	uchar reserved1		:6;
	uchar msf		:1;
	uchar reserved0		:1;

	uchar reserved2		:4;
	uchar format		:4;

	uchar reserved3		[3];
	uchar track_session_num;

	uchar alloc_len		[2];
	uchar ctl;
};

#endif

typedef struct _BurnerRdTocPmaAtipCDB BurnerRdTocPmaAtipCDB;

BURNER_SCSI_COMMAND_DEFINE (BurnerRdTocPmaAtipCDB,
			     READ_TOC_PMA_ATIP,
			     BURNER_SCSI_READ);

typedef enum {
BURNER_RD_TAP_FORMATTED_TOC		= 0x00,
BURNER_RD_TAP_MULTISESSION_INFO	= 0x01,
BURNER_RD_TAP_RAW_TOC			= 0x02,
BURNER_RD_TAP_PMA			= 0x03,
BURNER_RD_TAP_ATIP			= 0x04,
BURNER_RD_TAP_CD_TEXT			= 0x05	/* Introduced in MMC3 */
} BurnerReadTocPmaAtipType;

static BurnerScsiResult
burner_read_toc_pma_atip (BurnerRdTocPmaAtipCDB *cdb,
			   int desc_size,
			   BurnerScsiTocPmaAtipHdr **data,
			   int *size,
			   BurnerScsiErrCode *error)
{
	BurnerScsiTocPmaAtipHdr *buffer;
	BurnerScsiTocPmaAtipHdr hdr;
	BurnerScsiResult res;
	int request_size;
	int buffer_size;

	if (!data || !size) {
		BURNER_SCSI_SET_ERRCODE (error, BURNER_SCSI_BAD_ARGUMENT);
		return BURNER_SCSI_FAILURE;
	}

	BURNER_SET_16 (cdb->alloc_len, sizeof (BurnerScsiTocPmaAtipHdr));

	memset (&hdr, 0, sizeof (BurnerScsiTocPmaAtipHdr));
	res = burner_scsi_command_issue_sync (cdb,
					       &hdr,
					       sizeof (BurnerScsiTocPmaAtipHdr),
					       error);
	if (res) {
		*size = 0;
		return res;
	}

	request_size = BURNER_GET_16 (hdr.len) + sizeof (hdr.len);

	/* NOTE: if size is not valid use the maximum possible size */
	if ((request_size - sizeof (hdr)) % desc_size) {
		BURNER_MEDIA_LOG ("Unaligned data (%i) setting to max (65530)", request_size);
		request_size = 65530;
	}
	else if (request_size - sizeof (hdr) < desc_size) {
		BURNER_MEDIA_LOG ("Undersized data (%i) setting to max (65530)", request_size);
		request_size = 65530;
	}

	buffer = (BurnerScsiTocPmaAtipHdr *) g_new0 (uchar, request_size);

	BURNER_SET_16 (cdb->alloc_len, request_size);
	res = burner_scsi_command_issue_sync (cdb, buffer, request_size, error);
	if (res) {
		g_free (buffer);
		*size = 0;
		return res;
	}

	buffer_size = BURNER_GET_16 (buffer->len) + sizeof (buffer->len);

	*data = buffer;
	*size = MIN (buffer_size, request_size);

	return res;
}

/**
 * Returns TOC data for all the sessions starting with track_num
 */

BurnerScsiResult
burner_mmc1_read_toc_formatted (BurnerDeviceHandle *handle,
				 int track_num,
				 BurnerScsiFormattedTocData **data,
				 int *size,
				 BurnerScsiErrCode *error)
{
	BurnerRdTocPmaAtipCDB *cdb;
	BurnerScsiResult res;

	g_return_val_if_fail (handle != NULL, BURNER_SCSI_FAILURE);

	cdb = burner_scsi_command_new (&info, handle);
	cdb->format = BURNER_RD_TAP_FORMATTED_TOC;

	/* first track for which this function will return information */
	cdb->track_session_num = track_num;

	res = burner_read_toc_pma_atip (cdb,
					 sizeof (BurnerScsiTocDesc),
					(BurnerScsiTocPmaAtipHdr **) data,
					 size,
					 error);
	burner_scsi_command_free (cdb);
	return res;
}

/**
 * Returns RAW TOC data
 */

BurnerScsiResult
burner_mmc1_read_toc_raw (BurnerDeviceHandle *handle,
			   int session_num,
			   BurnerScsiRawTocData **data,
			   int *size,
			   BurnerScsiErrCode *error)
{
	BurnerRdTocPmaAtipCDB *cdb;
	BurnerScsiResult res;

	g_return_val_if_fail (handle != NULL, BURNER_SCSI_FAILURE);

	cdb = burner_scsi_command_new (&info, handle);
	cdb->format = BURNER_RD_TAP_RAW_TOC;

	/* first session for which this function will return information */
	cdb->track_session_num = session_num;

	res = burner_read_toc_pma_atip (cdb,
					 sizeof (BurnerScsiRawTocDesc),
					(BurnerScsiTocPmaAtipHdr **) data,
					 size,
					 error);
	burner_scsi_command_free (cdb);
	return res;
}

/**
 * Returns CD-TEXT information recorded in the leadin area as R-W sub-channel
 */

BurnerScsiResult
burner_mmc3_read_cd_text (BurnerDeviceHandle *handle,
			   BurnerScsiCDTextData **data,
			   int *size,
			   BurnerScsiErrCode *error)
{
	BurnerRdTocPmaAtipCDB *cdb;
	BurnerScsiResult res;

	g_return_val_if_fail (handle != NULL, BURNER_SCSI_FAILURE);

	cdb = burner_scsi_command_new (&info, handle);
	cdb->format = BURNER_RD_TAP_CD_TEXT;

	res = burner_read_toc_pma_atip (cdb,
					 sizeof (BurnerScsiCDTextPackData),
					(BurnerScsiTocPmaAtipHdr **) data,
					 size,
					 error);
	burner_scsi_command_free (cdb);
	return res;
}

/**
 * Returns ATIP information
 */

BurnerScsiResult
burner_mmc1_read_atip (BurnerDeviceHandle *handle,
			BurnerScsiAtipData **data,
			int *size,
			BurnerScsiErrCode *error)
{
	BurnerRdTocPmaAtipCDB *cdb;
	BurnerScsiResult res;

	g_return_val_if_fail (handle != NULL, BURNER_SCSI_FAILURE);

	/* In here we have to ask how many bytes the drive wants to return first
	 * indeed there is a difference in the descriptor size between MMC1/MMC2
	 * and MMC3. */
	cdb = burner_scsi_command_new (&info, handle);
	cdb->format = BURNER_RD_TAP_ATIP;
	cdb->msf = 1;				/* specs says it's compulsory */

	/* FIXME: sizeof (BurnerScsiTocDesc) is not really good here but that
	 * avoids the unaligned message */
	res = burner_read_toc_pma_atip (cdb,
					 sizeof (BurnerScsiTocDesc),
					(BurnerScsiTocPmaAtipHdr **) data,
					 size,
					 error);
	burner_scsi_command_free (cdb);
	return res;
}
