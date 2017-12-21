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
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <sys/ioctl.h>

#include <scsi/scsi.h>
#include <scsi/sg.h>

#include "burner-media-private.h"

#include "scsi-command.h"
#include "scsi-utils.h"
#include "scsi-error.h"
#include "scsi-sense-data.h"

struct _BurnerDeviceHandle {
	int fd;
};

struct _BurnerScsiCmd {
	uchar cmd [BURNER_SCSI_CMD_MAX_LEN];
	BurnerDeviceHandle *handle;

	const BurnerScsiCmdInfo *info;
};
typedef struct _BurnerScsiCmd BurnerScsiCmd;

#define BURNER_SCSI_CMD_OPCODE_OFF			0
#define BURNER_SCSI_CMD_SET_OPCODE(command)		(command->cmd [BURNER_SCSI_CMD_OPCODE_OFF] = command->info->opcode)

#define OPEN_FLAGS			O_RDWR /*|O_EXCL */|O_NONBLOCK

/**
 * This is to send a command
 */

static void
burner_sg_command_setup (struct sg_io_hdr *transport,
			  uchar *sense_data,
			  BurnerScsiCmd *cmd,
			  uchar *buffer,
			  int size)
{
	memset (sense_data, 0, BURNER_SENSE_DATA_SIZE);
	memset (transport, 0, sizeof (struct sg_io_hdr));
	
	transport->interface_id = 'S';				/* mandatory */
//	transport->flags = SG_FLAG_LUN_INHIBIT|SG_FLAG_DIRECT_IO;
	transport->cmdp = cmd->cmd;
	transport->cmd_len = cmd->info->size;
	transport->dxferp = buffer;
	transport->dxfer_len = size;

	/* where to output the scsi sense buffer */
	transport->sbp = sense_data;
	transport->mx_sb_len = BURNER_SENSE_DATA_SIZE;

	if (cmd->info->direction & BURNER_SCSI_READ)
		transport->dxfer_direction = SG_DXFER_FROM_DEV;
	else if (cmd->info->direction & BURNER_SCSI_WRITE)
		transport->dxfer_direction = SG_DXFER_TO_DEV;
}

BurnerScsiResult
burner_scsi_command_issue_sync (gpointer command,
				 gpointer buffer,
				 int size,
				 BurnerScsiErrCode *error)
{
	uchar sense_buffer [BURNER_SENSE_DATA_SIZE];
	struct sg_io_hdr transport;
	BurnerScsiResult res;
	BurnerScsiCmd *cmd;

	g_return_val_if_fail (command != NULL, BURNER_SCSI_FAILURE);

	cmd = command;
	burner_sg_command_setup (&transport,
				  sense_buffer,
				  cmd,
				  buffer,
				  size);

	/* NOTE on SG_IO: only for TEST UNIT READY, REQUEST/MODE SENSE, INQUIRY,
	 * READ CAPACITY, READ BUFFER, READ and LOG SENSE are allowed with it */
	res = ioctl (cmd->handle->fd, SG_IO, &transport);
	if (res) {
		BURNER_SCSI_SET_ERRCODE (error, BURNER_SCSI_ERRNO);
		return BURNER_SCSI_FAILURE;
	}

	if ((transport.info & SG_INFO_OK_MASK) == SG_INFO_OK)
		return BURNER_SCSI_OK;

	if ((transport.masked_status & CHECK_CONDITION) && transport.sb_len_wr)
		return burner_sense_data_process (sense_buffer, error);

	return BURNER_SCSI_FAILURE;
}

gpointer
burner_scsi_command_new (const BurnerScsiCmdInfo *info,
			  BurnerDeviceHandle *handle) 
{
	BurnerScsiCmd *cmd;

	g_return_val_if_fail (handle != NULL, NULL);

	/* make sure we can set the flags of the descriptor */

	/* allocate the command */
	cmd = g_new0 (BurnerScsiCmd, 1);
	cmd->info = info;
	cmd->handle = handle;

	BURNER_SCSI_CMD_SET_OPCODE (cmd);
	return cmd;
}

BurnerScsiResult
burner_scsi_command_free (gpointer cmd)
{
	g_free (cmd);
	return BURNER_SCSI_OK;
}

/**
 * This is to open a device
 */

BurnerDeviceHandle *
burner_device_handle_open (const gchar *path,
			    gboolean exclusive,
			    BurnerScsiErrCode *code)
{
	int fd;
	int flags = OPEN_FLAGS;
	BurnerDeviceHandle *handle;

	if (exclusive)
		flags |= O_EXCL;

	BURNER_MEDIA_LOG ("Getting handle");
	fd = open (path, flags);
	if (fd < 0) {
		BURNER_MEDIA_LOG ("No handle: %s", strerror (errno));
		if (code) {
			if (errno == EAGAIN
			||  errno == EWOULDBLOCK
			||  errno == EBUSY)
				*code = BURNER_SCSI_NOT_READY;
			else
				*code = BURNER_SCSI_ERRNO;
		}

		return NULL;
	}

	handle = g_new (BurnerDeviceHandle, 1);
	handle->fd = fd;

	BURNER_MEDIA_LOG ("Handle ready");
	return handle;
}

void
burner_device_handle_close (BurnerDeviceHandle *handle)
{
	close (handle->fd);
	g_free (handle);
}

char *
burner_device_get_bus_target_lun (const gchar *device)
{
	return strdup (device);
}
