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

#include "scsi-sbc.h"

#include "scsi-error.h"
#include "scsi-utils.h"
#include "scsi-base.h"
#include "scsi-command.h"
#include "scsi-opcodes.h"

#if G_BYTE_ORDER == G_LITTLE_ENDIAN

struct _BurnerRead10CDB {
	uchar opcode;

	uchar reladr		:1;
	uchar reserved1		:2;
	uchar FUA		:1;
	uchar DPO		:1;
	uchar reserved2		:3;

	uchar start_address	[4];

	uchar reserved3;

	uchar len		[2];	

	uchar ctl;
};

#else

struct _BurnerRead10CDB {
	uchar opcode;

	uchar reserved2		:3;
	uchar DPO		:1;
	uchar FUA		:1;
	uchar reserved1		:2;
	uchar reladr		:1;

	uchar start_address	[4];

	uchar reserved3;

	uchar len		[2];	

	uchar ctl;
};

#endif

typedef struct _BurnerRead10CDB BurnerRead10CDB;

BURNER_SCSI_COMMAND_DEFINE (BurnerRead10CDB,
			     READ10,
			     BURNER_SCSI_READ);

BurnerScsiResult
burner_sbc_read10_block (BurnerDeviceHandle *handle,
			  int start,
			  int num_blocks,
			  unsigned char *buffer,
			  int buffer_size,
			  BurnerScsiErrCode *error)
{
	BurnerRead10CDB *cdb;
	BurnerScsiResult res;

	g_return_val_if_fail (handle != NULL, BURNER_SCSI_FAILURE);

	cdb = burner_scsi_command_new (&info, handle);
	BURNER_SET_32 (cdb->start_address, start);

	/* NOTE: if we just want to test if block is readable len can be 0 */
	BURNER_SET_16 (cdb->len, num_blocks);

	/* reladr should be 0 */
	/* DPO should be 0 */

	/* This is to force reading media ==> no caching (set to 1) */
	/* On the other hand caching improves dramatically the performances. */
	cdb->FUA = 0;

	memset (buffer, 0, buffer_size);
	res = burner_scsi_command_issue_sync (cdb,
					       buffer,
					       buffer_size,
					       error);
	burner_scsi_command_free (cdb);
	return res;
}
