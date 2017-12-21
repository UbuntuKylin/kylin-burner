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
#include "scsi-get-configuration.h"

#if G_BYTE_ORDER == G_LITTLE_ENDIAN

struct _BurnerGetConfigCDB {
	uchar opcode		:8;

	uchar returned_data	:2;
	uchar reserved0		:6;

	uchar feature_num	[2];

	uchar reserved1		[3];

	uchar alloc_len		[2];

	uchar ctl;
};

#else

struct _BurnerGetConfigCDB {
	uchar opcode		:8;

	uchar reserved1		:6;
	uchar returned_data	:2;

	uchar feature_num	[2];

	uchar reserved0		[3];

	uchar alloc_len		[2];

	uchar ctl;
};

#endif

typedef struct _BurnerGetConfigCDB BurnerGetConfigCDB;

BURNER_SCSI_COMMAND_DEFINE (BurnerGetConfigCDB,
			     GET_CONFIGURATION,
			     BURNER_SCSI_READ);

typedef enum {
BURNER_GET_CONFIG_RETURN_ALL_FEATURES	= 0x00,
BURNER_GET_CONFIG_RETURN_ALL_CURRENT	= 0x01,
BURNER_GET_CONFIG_RETURN_ONLY_FEATURE	= 0x02,
} BurnerGetConfigReturnedData;

static BurnerScsiResult
burner_get_configuration (BurnerGetConfigCDB *cdb,
			   BurnerScsiGetConfigHdr **data,
			   int *size,
			   BurnerScsiErrCode *error)
{
	BurnerScsiGetConfigHdr *buffer;
	BurnerScsiGetConfigHdr hdr;
	BurnerScsiResult res;
	int request_size;
	int buffer_size;

	if (!data || !size) {
		BURNER_SCSI_SET_ERRCODE (error, BURNER_SCSI_BAD_ARGUMENT);
		return BURNER_SCSI_FAILURE;
	}

	/* first issue the command once ... */
	memset (&hdr, 0, sizeof (hdr));
	BURNER_SET_16 (cdb->alloc_len, sizeof (hdr));
	res = burner_scsi_command_issue_sync (cdb, &hdr, sizeof (hdr), error);
	if (res)
		return res;

	/* ... check the size returned ... */
	request_size = BURNER_GET_32 (hdr.len) +
		       G_STRUCT_OFFSET (BurnerScsiGetConfigHdr, len) +
		       sizeof (hdr.len);

	/* NOTE: if size is not valid use the maximum possible size */
	if ((request_size - sizeof (hdr)) % 8) {
		BURNER_MEDIA_LOG ("Unaligned data (%i) setting to max (65530)", request_size);
		request_size = 65530;
	}
	else if (request_size <= sizeof (hdr)) {
		/* NOTE: if there is a feature, the size must be larger than the
		 * header size. */
		BURNER_MEDIA_LOG ("Undersized data (%i) setting to max (65530)", request_size);
		request_size = 65530;
	}

	/* ... allocate a buffer and re-issue the command */
	buffer = (BurnerScsiGetConfigHdr *) g_new0 (uchar, request_size);

	BURNER_SET_16 (cdb->alloc_len, request_size);
	res = burner_scsi_command_issue_sync (cdb, buffer, request_size, error);
	if (res) {
		g_free (buffer);
		return res;
	}

	/* make sure the response has the requested size */
	buffer_size = BURNER_GET_32 (buffer->len) +
		      G_STRUCT_OFFSET (BurnerScsiGetConfigHdr, len) +
		      sizeof (hdr.len);

	if (buffer_size < sizeof (BurnerScsiGetConfigHdr) + 2) {
		/* we can't have a size less or equal to that of the header */
		BURNER_MEDIA_LOG ("Size of buffer is less or equal to size of header");
		g_free (buffer);
		return BURNER_SCSI_FAILURE;
	}

	if (buffer_size != request_size)
		BURNER_MEDIA_LOG ("Sizes mismatch asked %i / received %i",
				  request_size,
				  buffer_size);

	*data = buffer;
	*size = MIN (buffer_size, request_size);
	return BURNER_SCSI_OK;
}

BurnerScsiResult
burner_mmc2_get_configuration_feature (BurnerDeviceHandle *handle,
					BurnerScsiFeatureType type,
					BurnerScsiGetConfigHdr **data,
					int *size,
					BurnerScsiErrCode *error)
{
	BurnerScsiGetConfigHdr *hdr = NULL;
	BurnerGetConfigCDB *cdb;
	BurnerScsiResult res;
	int hdr_size = 0;

	g_return_val_if_fail (handle != NULL, BURNER_SCSI_FAILURE);
	g_return_val_if_fail (data != NULL, BURNER_SCSI_FAILURE);
	g_return_val_if_fail (size != NULL, BURNER_SCSI_FAILURE);

	cdb = burner_scsi_command_new (&info, handle);
	BURNER_SET_16 (cdb->feature_num, type);
	cdb->returned_data = BURNER_GET_CONFIG_RETURN_ONLY_FEATURE;

	res = burner_get_configuration (cdb, &hdr, &hdr_size, error);
	burner_scsi_command_free (cdb);

	/* make sure the desc is the one we want */
	if (hdr && BURNER_GET_16 (hdr->desc->code) != type) {
		BURNER_MEDIA_LOG ("Wrong type returned %d", hdr->desc->code);
		BURNER_SCSI_SET_ERRCODE (error, BURNER_SCSI_TYPE_MISMATCH);

		g_free (hdr);
		return BURNER_SCSI_FAILURE;
	}

	*data = hdr;
	*size = hdr_size;
	return res;
}

BurnerScsiResult
burner_mmc2_get_profile (BurnerDeviceHandle *handle,
			  BurnerScsiProfile *profile,
			  BurnerScsiErrCode *error)
{
	BurnerScsiGetConfigHdr hdr;
	BurnerGetConfigCDB *cdb;
	BurnerScsiResult res;

	g_return_val_if_fail (handle != NULL, BURNER_SCSI_FAILURE);
	g_return_val_if_fail (profile != NULL, BURNER_SCSI_FAILURE);

	cdb = burner_scsi_command_new (&info, handle);
	BURNER_SET_16 (cdb->feature_num, BURNER_SCSI_FEAT_CORE);
	cdb->returned_data = BURNER_GET_CONFIG_RETURN_ONLY_FEATURE;

	memset (&hdr, 0, sizeof (hdr));
	BURNER_SET_16 (cdb->alloc_len, sizeof (hdr));
	res = burner_scsi_command_issue_sync (cdb, &hdr, sizeof (hdr), error);
	burner_scsi_command_free (cdb);

	if (res)
		return res;

	*profile = BURNER_GET_16 (hdr.current_profile);
	return BURNER_SCSI_OK;
}

