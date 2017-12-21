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

#include <unistd.h>
#include <string.h>

#ifdef HAVE_CAM_LIB_H
#include <errno.h>
#include <stdio.h>
#include <fcntl.h>
#include <camlib.h>
#endif

#include <glib.h>
#include <glib-object.h>
#include <glib/gi18n-lib.h>

#include <gio/gio.h>

#include "burner-media-private.h"
#include "burner-gio-operation.h"

#include "burner-medium.h"
#include "burner-volume.h"
#include "burner-drive.h"

#include "burner-drive-priv.h"
#include "scsi-device.h"
#include "scsi-utils.h"
#include "scsi-spc1.h"
#include "scsi-mmc1.h"
#include "scsi-mmc2.h"
#include "scsi-status-page.h"
#include "scsi-mode-pages.h"
#include "scsi-sbc.h"

typedef struct _BurnerDrivePrivate BurnerDrivePrivate;
struct _BurnerDrivePrivate
{
	GDrive *gdrive;

	GThread *probe;
	GMutex *mutex;
	GCond *cond;
	GCond *cond_probe;
	gint probe_id;

	BurnerMedium *medium;
	BurnerDriveCaps caps;

	gchar *udi;

	gchar *name;

	gchar *device;
	gchar *block_device;

	GCancellable *cancel;

	guint initial_probe:1;
	guint initial_probe_cancelled:1;

	guint has_medium:1;
	guint probe_cancelled:1;

	guint locked:1;
	guint ejecting:1;
	guint probe_waiting:1;
};

#define BURNER_DRIVE_PRIVATE(o)  (G_TYPE_INSTANCE_GET_PRIVATE ((o), BURNER_TYPE_DRIVE, BurnerDrivePrivate))

enum {
	MEDIUM_REMOVED,
	MEDIUM_INSERTED,
	LAST_SIGNAL
};
static gulong drive_signals [LAST_SIGNAL] = {0, };

enum {
	PROP_NONE	= 0,
	PROP_DEVICE,
	PROP_GDRIVE,
	PROP_UDI
};

G_DEFINE_TYPE (BurnerDrive, burner_drive, G_TYPE_OBJECT);

#define BURNER_DRIVE_OPEN_ATTEMPTS			5

static void
burner_drive_probe_inside (BurnerDrive *drive);

/**
 * burner_drive_get_gdrive:
 * @drive: a #BurnerDrive
 *
 * Returns the #GDrive corresponding to this #BurnerDrive
 *
 * Return value: a #GDrive or NULL. Unref after use.
 **/
GDrive *
burner_drive_get_gdrive (BurnerDrive *drive)
{
	BurnerDrivePrivate *priv;

	g_return_val_if_fail (drive != NULL, NULL);
	g_return_val_if_fail (BURNER_IS_DRIVE (drive), NULL);

	if (burner_drive_is_fake (drive))
		return NULL;

	priv = BURNER_DRIVE_PRIVATE (drive);

	if (!priv->gdrive)
		return NULL;

	return g_object_ref (priv->gdrive);
}

/**
 * burner_drive_can_eject:
 * @drive: #BurnerDrive
 *
 * Returns whether the drive can eject media.
 *
 * Return value: a #gboolean. TRUE if the media can be ejected, FALSE otherwise.
 *
 **/
gboolean
burner_drive_can_eject (BurnerDrive *drive)
{
	GVolume *volume;
	gboolean result;
	BurnerDrivePrivate *priv;

	g_return_val_if_fail (drive != NULL, FALSE);
	g_return_val_if_fail (BURNER_IS_DRIVE (drive), FALSE);

	priv = BURNER_DRIVE_PRIVATE (drive);

	if (!priv->gdrive) {
		BURNER_MEDIA_LOG ("No GDrive");
		goto last_resort;
	}

	if (!g_drive_can_eject (priv->gdrive)) {
		BURNER_MEDIA_LOG ("GDrive can't eject");
		goto last_resort;
	}

	return TRUE;

last_resort:

	if (!priv->medium)
		return FALSE;

	/* last resort */
	volume = burner_volume_get_gvolume (BURNER_VOLUME (priv->medium));
	if (!volume)
		return FALSE;

	result = g_volume_can_eject (volume);
	g_object_unref (volume);

	return result;
}

static void
burner_drive_cancel_probing (BurnerDrive *drive)
{
	BurnerDrivePrivate *priv;

	priv = BURNER_DRIVE_PRIVATE (drive);

	priv->probe_waiting = FALSE;

	g_mutex_lock (priv->mutex);
	if (priv->probe) {
		/* This to signal that we are cancelling */
		priv->probe_cancelled = TRUE;
		priv->initial_probe_cancelled = TRUE;

		/* This is to wake up the thread if it
		 * was asleep waiting to retry to get
		 * hold of a handle to probe the drive */
		g_cond_signal (priv->cond_probe);

		g_cond_wait (priv->cond, priv->mutex);
	}
	g_mutex_unlock (priv->mutex);

	if (priv->probe_id) {
		g_source_remove (priv->probe_id);
		priv->probe_id = 0;
	}
}

static void
burner_drive_wait_probing_thread (BurnerDrive *drive)
{
	BurnerDrivePrivate *priv;

	priv = BURNER_DRIVE_PRIVATE (drive);

	g_mutex_lock (priv->mutex);
	if (priv->probe) {
		/* This is to wake up the thread if it
		 * was asleep waiting to retry to get
		 * hold of a handle to probe the drive */
		g_cond_signal (priv->cond_probe);
		g_cond_wait (priv->cond, priv->mutex);
	}
	g_mutex_unlock (priv->mutex);
}

/**
 * burner_drive_eject:
 * @drive: #BurnerDrive
 * @wait: #gboolean whether to wait for the completion of the operation (with a GMainLoop)
 * @error: #GError
 *
 * Open the drive tray or ejects the media if there is any inside.
 *
 * Return value: a #gboolean. TRUE on success, FALSE otherwise.
 *
 **/
gboolean
burner_drive_eject (BurnerDrive *drive,
		     gboolean wait,
		     GError **error)
{
	BurnerDrivePrivate *priv;
	GVolume *gvolume;
	gboolean res;

	g_return_val_if_fail (drive != NULL, FALSE);
	g_return_val_if_fail (BURNER_IS_DRIVE (drive), FALSE);

	priv = BURNER_DRIVE_PRIVATE (drive);

	/* reset if needed */
	if (g_cancellable_is_cancelled (priv->cancel)) {
		BURNER_MEDIA_LOG ("Resetting GCancellable object");
		g_cancellable_reset (priv->cancel);
	}

	BURNER_MEDIA_LOG ("Trying to eject drive");
	if (priv->gdrive) {
		/* Wait for any ongoing probing as it
		 * would prevent the door from being
		 * opened. */
		burner_drive_wait_probing_thread (drive);

		priv->ejecting = TRUE;
		res = burner_gio_operation_eject_drive (priv->gdrive,
							 priv->cancel,
							 wait,
							 error);
		priv->ejecting = FALSE;
		if (priv->probe_waiting)
			burner_drive_probe_inside (drive);

		if (res)
			return TRUE;

		if (g_cancellable_is_cancelled (priv->cancel))
			return FALSE;
	}
	else
		BURNER_MEDIA_LOG ("No GDrive");

	if (!priv->medium)
		return FALSE;

	/* reset if needed */
	if (g_cancellable_is_cancelled (priv->cancel)) {
		BURNER_MEDIA_LOG ("Resetting GCancellable object");
		g_cancellable_reset (priv->cancel);
	}

	gvolume = burner_volume_get_gvolume (BURNER_VOLUME (priv->medium));
	if (gvolume) {
		BURNER_MEDIA_LOG ("Trying to eject volume");

		/* Cancel any ongoing probing as it
		 * would prevent the door from being
		 * opened. */
		burner_drive_wait_probing_thread (drive);

		priv->ejecting = TRUE;
		res = burner_gio_operation_eject_volume (gvolume,
							  priv->cancel,
							  wait,
							  error);

		priv->ejecting = FALSE;
		if (priv->probe_waiting)
			burner_drive_probe_inside (drive);

		g_object_unref (gvolume);
	}

	return res;
}

/**
 * burner_drive_cancel_current_operation:
 * @drive: #BurnerDrive *
 *
 * Cancels all operations currently running for @drive
 *
 **/
void
burner_drive_cancel_current_operation (BurnerDrive *drive)
{
	BurnerDrivePrivate *priv;

	g_return_if_fail (drive != NULL);
	g_return_if_fail (BURNER_IS_DRIVE (drive));

	priv = BURNER_DRIVE_PRIVATE (drive);

	BURNER_MEDIA_LOG ("Cancelling GIO operation");
	g_cancellable_cancel (priv->cancel);
}

/**
 * burner_drive_get_bus_target_lun_string:
 * @drive: a #BurnerDrive
 *
 * Returns the bus, target, lun ("{bus},{target},{lun}") as a string which is
 * sometimes needed by some backends like cdrecord.
 *
 * NOTE: that function returns either bus/target/lun or the device path
 * according to OSes. Basically it returns bus/target/lun only for FreeBSD
 * which is the only OS in need for that. For all others it returns the device
 * path. 
 *
 * Return value: a string or NULL. The string must be freed when not needed
 *
 **/
gchar *
burner_drive_get_bus_target_lun_string (BurnerDrive *drive)
{
	g_return_val_if_fail (drive != NULL, NULL);
	g_return_val_if_fail (BURNER_IS_DRIVE (drive), NULL);

	return burner_device_get_bus_target_lun (burner_drive_get_device (drive));
}

/**
 * burner_drive_is_fake:
 * @drive: a #BurnerDrive
 *
 * Returns whether or not the drive is a fake one. There is only one and
 * corresponds to a file which is used when the user wants to burn to a file.
 *
 * Return value: %TRUE or %FALSE.
 **/
gboolean
burner_drive_is_fake (BurnerDrive *drive)
{
	BurnerDrivePrivate *priv;

	g_return_val_if_fail (drive != NULL, FALSE);
	g_return_val_if_fail (BURNER_IS_DRIVE (drive), FALSE);

	priv = BURNER_DRIVE_PRIVATE (drive);
	return (priv->device == NULL);
}

/**
 * burner_drive_is_door_open:
 * @drive: a #BurnerDrive
 *
 * Returns whether or not the drive door is open.
 *
 * Return value: %TRUE or %FALSE.
 **/
gboolean
burner_drive_is_door_open (BurnerDrive *drive)
{
	const gchar *device;
	BurnerDrivePrivate *priv;
	BurnerDeviceHandle *handle;
	BurnerScsiMechStatusHdr hdr;

	g_return_val_if_fail (drive != NULL, FALSE);
	g_return_val_if_fail (BURNER_IS_DRIVE (drive), FALSE);

	priv = BURNER_DRIVE_PRIVATE (drive);
	if (!priv->device)
		return FALSE;

	device = burner_drive_get_device (drive);
	handle = burner_device_handle_open (device, FALSE, NULL);
	if (!handle)
		return FALSE;

	burner_mmc1_mech_status (handle,
				  &hdr,
				  NULL);

	burner_device_handle_close (handle);

	return hdr.door_open;
}

/**
 * burner_drive_can_use_exclusively:
 * @drive: a #BurnerDrive
 *
 * Returns whether or not the drive can be used exclusively, that is whether or
 * not it is currently used by another application.
 *
 * Return value: %TRUE or %FALSE.
 **/
gboolean
burner_drive_can_use_exclusively (BurnerDrive *drive)
{
	BurnerDeviceHandle *handle;
	const gchar *device;

	g_return_val_if_fail (drive != NULL, FALSE);
	g_return_val_if_fail (BURNER_IS_DRIVE (drive), FALSE);

	device = burner_drive_get_device (drive);
	handle = burner_device_handle_open (device, TRUE, NULL);
	if (!handle)
		return FALSE;

	burner_device_handle_close (handle);
	return TRUE;
}

/**
 * burner_drive_is_locked:
 * @drive: a #BurnerDrive
 * @reason: a #gchar or NULL. A string to indicate what the drive was locked for if return value is %TRUE
 *
 * Checks whether a #BurnerDrive is currently locked. Manual ejection shouldn't be possible any more.
 *
 * Since 2.29.0
 *
 * Return value: %TRUE if the drive is locked or %FALSE.
 **/
gboolean
burner_drive_is_locked (BurnerDrive *drive,
                         gchar **reason)
{
	BurnerDrivePrivate *priv;

	g_return_val_if_fail (drive != NULL, FALSE);
	g_return_val_if_fail (BURNER_IS_DRIVE (drive), FALSE);

	priv = BURNER_DRIVE_PRIVATE (drive);
	return priv->locked;
}

/**
 * burner_drive_lock:
 * @drive: a #BurnerDrive
 * @reason: a string to indicate what the drive was locked for
 * @reason_for_failure: a string (or NULL) to hold the reason why the locking failed
 *
 * Locks a #BurnerDrive. Manual ejection shouldn't be possible any more.
 *
 * Return value: %TRUE if the drive was successfully locked or %FALSE.
 **/
gboolean
burner_drive_lock (BurnerDrive *drive,
		    const gchar *reason,
		    gchar **reason_for_failure)
{
	BurnerDeviceHandle *handle;
	BurnerDrivePrivate *priv;
	const gchar *device;
	gboolean result;

	g_return_val_if_fail (drive != NULL, FALSE);
	g_return_val_if_fail (BURNER_IS_DRIVE (drive), FALSE);

	priv = BURNER_DRIVE_PRIVATE (drive);
	if (!priv->device)
		return FALSE;

	device = burner_drive_get_device (drive);
	handle = burner_device_handle_open (device, FALSE, NULL);
	if (!handle)
		return FALSE;

	result = (burner_sbc_medium_removal (handle, 1, NULL) == BURNER_SCSI_OK);
	if (result) {
		BURNER_MEDIA_LOG ("Device locked");
		priv->locked = TRUE;
	}
	else
		BURNER_MEDIA_LOG ("Device failed to lock");

	burner_device_handle_close (handle);
	return result;
}

/**
 * burner_drive_unlock:
 * @drive: a #BurnerDrive
 *
 * Unlocks a #BurnerDrive.
 *
 * Return value: %TRUE if the drive was successfully unlocked or %FALSE.
 **/
gboolean
burner_drive_unlock (BurnerDrive *drive)
{
	BurnerDeviceHandle *handle;
	BurnerDrivePrivate *priv;
	const gchar *device;
	gboolean result;

	g_return_val_if_fail (drive != NULL, FALSE);
	g_return_val_if_fail (BURNER_IS_DRIVE (drive), FALSE);

	priv = BURNER_DRIVE_PRIVATE (drive);
	if (!priv->device)
		return FALSE;

	device = burner_drive_get_device (drive);
	handle = burner_device_handle_open (device, FALSE, NULL);
	if (!handle)
		return FALSE;

	result = (burner_sbc_medium_removal (handle, 0, NULL) == BURNER_SCSI_OK);
	if (result) {
		BURNER_MEDIA_LOG ("Device unlocked");
		priv->locked = FALSE;

		if (priv->probe_waiting) {
			BURNER_MEDIA_LOG ("Probe on hold");

			/* A probe was waiting */
			burner_drive_probe_inside (drive);
		}
	}
	else
		BURNER_MEDIA_LOG ("Device failed to unlock");

	burner_device_handle_close (handle);

	return result;
}

/**
 * burner_drive_get_display_name:
 * @drive: a #BurnerDrive
 *
 * Gets a string holding the name for the drive. That string can be then
 * displayed in a user interface.
 *
 * Return value: a string holding the name
 **/
gchar *
burner_drive_get_display_name (BurnerDrive *drive)
{
	BurnerDrivePrivate *priv;

	g_return_val_if_fail (drive != NULL, NULL);
	g_return_val_if_fail (BURNER_IS_DRIVE (drive), NULL);

	priv = BURNER_DRIVE_PRIVATE (drive);
	if (!priv->device) {
		/* Translators: This is a fake drive, a file, and means that
		 * when we're writing, we're writing to a file and create an
		 * image on the hard drive. */
		return g_strdup (_("Image File"));
	}

	return g_strdup (priv->name);
}

/**
 * burner_drive_get_device:
 * @drive: a #BurnerDrive
 *
 * Gets a string holding the device path for the drive.
 *
 * Return value: a string holding the device path.
 * On Solaris returns raw device.
 **/
const gchar *
burner_drive_get_device (BurnerDrive *drive)
{
	BurnerDrivePrivate *priv;

	g_return_val_if_fail (drive != NULL, NULL);
	g_return_val_if_fail (BURNER_IS_DRIVE (drive), NULL);

	priv = BURNER_DRIVE_PRIVATE (drive);
	return priv->device;
}

/**
 * burner_drive_get_block_device:
 * @drive: a #BurnerDrive
 *
 * Gets a string holding the block device path for the drive. This can be used on
 * some other OSes, like Solaris, for GIO operations instead of the device
 * path.
 *
 * Solaris uses block device for GIO operations and
 * uses raw device for system calls and backends
 * like cdrtool.
 *
 * If such a path is not available, it returns the device path.
 *
 * Return value: a string holding the block device path
 **/
const gchar *
burner_drive_get_block_device (BurnerDrive *drive)
{
	BurnerDrivePrivate *priv;

	g_return_val_if_fail (drive != NULL, NULL);
	g_return_val_if_fail (BURNER_IS_DRIVE (drive), NULL);

	priv = BURNER_DRIVE_PRIVATE (drive);
	return priv->block_device? priv->block_device:priv->device;
}

/**
 * burner_drive_get_udi:
 * @drive: a #BurnerDrive
 *
 * Gets a string holding the HAL udi corresponding to this device. It can be used
 * to uniquely identify the drive.
 *
 * Return value: a string holding the HAL udi or NULL. Not to be freed
 **/
const gchar *
burner_drive_get_udi (BurnerDrive *drive)
{
	BurnerDrivePrivate *priv;

	if (!drive)
		return NULL;

	g_return_val_if_fail (BURNER_IS_DRIVE (drive), NULL);

	priv = BURNER_DRIVE_PRIVATE (drive);
	if (!priv->device || !priv->gdrive)
		return NULL;

	if (priv->udi)
		return priv->udi;

	priv->udi = g_drive_get_identifier (priv->gdrive, G_VOLUME_IDENTIFIER_KIND_HAL_UDI);
	return priv->udi;
}

/**
 * burner_drive_get_medium:
 * @drive: a #BurnerDrive
 *
 * Gets the medium currently inserted in the drive. If there is no medium or if
 * the medium is not probed yet then it returns NULL.
 *
 * Return value: (transfer none): a #BurnerMedium object or NULL. No need to unref after use.
 **/
BurnerMedium *
burner_drive_get_medium (BurnerDrive *drive)
{
	BurnerDrivePrivate *priv;

	if (!drive)
		return NULL;

	g_return_val_if_fail (BURNER_IS_DRIVE (drive), NULL);

	priv = BURNER_DRIVE_PRIVATE (drive);
	if (burner_drive_probing (drive))
		return NULL;

	return priv->medium;
}

/**
 * burner_drive_get_caps:
 * @drive: a #BurnerDrive
 *
 * Returns what type(s) of disc the drive can write to.
 *
 * Return value: a #BurnerDriveCaps.
 **/
BurnerDriveCaps
burner_drive_get_caps (BurnerDrive *drive)
{
	BurnerDrivePrivate *priv;

	g_return_val_if_fail (drive != NULL, BURNER_DRIVE_CAPS_NONE);
	g_return_val_if_fail (BURNER_IS_DRIVE (drive), BURNER_DRIVE_CAPS_NONE);

	priv = BURNER_DRIVE_PRIVATE (drive);
	return priv->caps;
}

/**
 * burner_drive_can_write_media:
 * @drive: a #BurnerDrive
 * @media: a #BurnerMedia
 *
 * Returns whether the disc can burn a specific media type.
 *
 * Since 2.29.0
 *
 * Return value: a #gboolean. TRUE if the drive can write this type of media and FALSE otherwise
 **/
gboolean
burner_drive_can_write_media (BurnerDrive *drive,
                               BurnerMedia media)
{
	BurnerDrivePrivate *priv;

	g_return_val_if_fail (drive != NULL, FALSE);
	g_return_val_if_fail (BURNER_IS_DRIVE (drive), FALSE);

	priv = BURNER_DRIVE_PRIVATE (drive);

	if (!(media & BURNER_MEDIUM_REWRITABLE)
	&&   (media & BURNER_MEDIUM_CLOSED))
		return FALSE;

	if (media & BURNER_MEDIUM_FILE)
		return FALSE;

	if (BURNER_MEDIUM_IS (media, BURNER_MEDIUM_CDR))
		return (priv->caps & BURNER_DRIVE_CAPS_CDR) != 0;

	if (BURNER_MEDIUM_IS (media, BURNER_MEDIUM_DVDR))
		return (priv->caps & BURNER_DRIVE_CAPS_DVDR) != 0;

	if (BURNER_MEDIUM_IS (media, BURNER_MEDIUM_DVDR_PLUS))
		return (priv->caps & BURNER_DRIVE_CAPS_DVDR_PLUS) != 0;

	if (BURNER_MEDIUM_IS (media, BURNER_MEDIUM_CDRW))
		return (priv->caps & BURNER_DRIVE_CAPS_CDRW) != 0;

	if (BURNER_MEDIUM_IS (media, BURNER_MEDIUM_DVDRW))
		return (priv->caps & BURNER_DRIVE_CAPS_DVDRW) != 0;

	if (BURNER_MEDIUM_IS (media, BURNER_MEDIUM_DVDRW_RESTRICTED))
		return (priv->caps & BURNER_DRIVE_CAPS_DVDRW) != 0;

	if (BURNER_MEDIUM_IS (media, BURNER_MEDIUM_DVDRW_PLUS))
		return (priv->caps & BURNER_DRIVE_CAPS_DVDRW_PLUS) != 0;

	if (BURNER_MEDIUM_IS (media, BURNER_MEDIUM_DVDR_PLUS_DL))
		return (priv->caps & BURNER_DRIVE_CAPS_DVDR_PLUS_DL) != 0;

	if (BURNER_MEDIUM_IS (media, BURNER_MEDIUM_DVDRW_PLUS_DL))
		return (priv->caps & BURNER_DRIVE_CAPS_DVDRW_PLUS_DL) != 0;

	if (BURNER_MEDIUM_IS (media, BURNER_MEDIUM_DVD_RAM))
		return (priv->caps & BURNER_DRIVE_CAPS_DVDRAM) != 0;

	/* All types of BD-R */
	if (BURNER_MEDIUM_IS (media, BURNER_MEDIUM_BD|BURNER_MEDIUM_WRITABLE))
		return (priv->caps & BURNER_DRIVE_CAPS_BDR) != 0;

	if (BURNER_MEDIUM_IS (media, BURNER_MEDIUM_BDRE))
		return (priv->caps & BURNER_DRIVE_CAPS_BDRW) != 0;

	return FALSE;
}

/**
 * burner_drive_can_write:
 * @drive: a #BurnerDrive
 *
 * Returns whether the disc can burn any disc at all.
 *
 * Return value: a #gboolean. TRUE if the drive can write a disc and FALSE otherwise
 **/
gboolean
burner_drive_can_write (BurnerDrive *drive)
{
	BurnerDrivePrivate *priv;

	g_return_val_if_fail (drive != NULL, FALSE);
	g_return_val_if_fail (BURNER_IS_DRIVE (drive), FALSE);

	priv = BURNER_DRIVE_PRIVATE (drive);
	return (priv->caps & (BURNER_DRIVE_CAPS_CDR|
			      BURNER_DRIVE_CAPS_DVDR|
			      BURNER_DRIVE_CAPS_DVDR_PLUS|
			      BURNER_DRIVE_CAPS_CDRW|
			      BURNER_DRIVE_CAPS_DVDRW|
			      BURNER_DRIVE_CAPS_DVDRW_PLUS|
			      BURNER_DRIVE_CAPS_DVDR_PLUS_DL|
			      BURNER_DRIVE_CAPS_DVDRW_PLUS_DL));
}

static void
burner_drive_medium_probed (BurnerMedium *medium,
			     BurnerDrive *self)
{
	BurnerDrivePrivate *priv;

	priv = BURNER_DRIVE_PRIVATE (self);

	/* only when it is probed */
	/* NOTE: BurnerMedium calls GDK_THREADS_ENTER/LEAVE() around g_signal_emit () */
	if (burner_medium_get_status (priv->medium) == BURNER_MEDIUM_NONE) {
		g_object_unref (priv->medium);
		priv->medium = NULL;
		return;
	}

	g_signal_emit (self,
		       drive_signals [MEDIUM_INSERTED],
		       0,
		       priv->medium);
}

/**
 * This is not public API. Defined in burner-drive-priv.h.
 */
gboolean
burner_drive_probing (BurnerDrive *drive)
{
	BurnerDrivePrivate *priv;

	g_return_val_if_fail (drive != NULL, FALSE);
	g_return_val_if_fail (BURNER_IS_DRIVE (drive), FALSE);

	priv = BURNER_DRIVE_PRIVATE (drive);
	if (priv->probe != NULL)
		return TRUE;

	if (priv->medium)
		return burner_medium_probing (priv->medium);

	return FALSE;
}

static void
burner_drive_update_medium (BurnerDrive *drive)
{
	BurnerDrivePrivate *priv;

	priv = BURNER_DRIVE_PRIVATE (drive);

	if (priv->has_medium) {
		if (priv->medium) {
			BURNER_MEDIA_LOG ("Already a medium. Skipping");
			return;
		}

		BURNER_MEDIA_LOG ("Probing new medium");
		priv->medium = g_object_new (BURNER_TYPE_VOLUME,
					     "drive", drive,
					     NULL);

		g_signal_connect (priv->medium,
				  "probed",
				  G_CALLBACK (burner_drive_medium_probed),
				  drive);
	}
	else if (priv->medium) {
		BurnerMedium *medium;

		BURNER_MEDIA_LOG ("Medium removed");

		medium = priv->medium;
		priv->medium = NULL;

		g_signal_emit (drive,
			       drive_signals [MEDIUM_REMOVED],
			       0,
			       medium);

		g_object_unref (medium);
	}
}

static gboolean
burner_drive_probed_inside (gpointer data)
{
	BurnerDrive *self;
	BurnerDrivePrivate *priv;

	self = BURNER_DRIVE (data);
	priv = BURNER_DRIVE_PRIVATE (self);

	if (!g_mutex_trylock (priv->mutex))
		return TRUE;

	priv->probe_id = 0;
	g_mutex_unlock (priv->mutex);

	burner_drive_update_medium (self);
	return FALSE;
}

static gpointer
burner_drive_probe_inside_thread (gpointer data)
{
	gint counter = 0;
	GTimeVal wait_time;
	const gchar *device;
	BurnerScsiErrCode code;
	BurnerDrivePrivate *priv;
	BurnerDeviceHandle *handle = NULL;
	BurnerDrive *drive = BURNER_DRIVE (data);

	priv = BURNER_DRIVE_PRIVATE (drive);

	/* the drive might be busy (a burning is going on) so we don't block
	 * but we re-try to open it every second */
	device = burner_drive_get_device (drive);
	BURNER_MEDIA_LOG ("Trying to open device %s", device);

	priv->has_medium = FALSE;

	handle = burner_device_handle_open (device, FALSE, &code);
	while (!handle && counter <= BURNER_DRIVE_OPEN_ATTEMPTS) {
		sleep (1);

		if (priv->probe_cancelled) {
			BURNER_MEDIA_LOG ("Open () cancelled");
			goto end;
		}

		counter ++;
		handle = burner_device_handle_open (device, FALSE, &code);
	}

	if (!handle) {
		BURNER_MEDIA_LOG ("Open () failed: medium busy");
		goto end;
	}

	if (priv->probe_cancelled) {
		BURNER_MEDIA_LOG ("Open () cancelled");

		burner_device_handle_close (handle);
		goto end;
	}

	while (burner_spc1_test_unit_ready (handle, &code) != BURNER_SCSI_OK) {
		if (code == BURNER_SCSI_NO_MEDIUM) {
			BURNER_MEDIA_LOG ("No medium inserted");

			burner_device_handle_close (handle);
			goto end;
		}

		if (code != BURNER_SCSI_NOT_READY) {
			BURNER_MEDIA_LOG ("Device does not respond");

			burner_device_handle_close (handle);
			goto end;
		}

		g_get_current_time (&wait_time);
		g_time_val_add (&wait_time, 2000000);

		g_mutex_lock (priv->mutex);
		g_cond_timed_wait (priv->cond_probe,
		                   priv->mutex,
		                   &wait_time);
		g_mutex_unlock (priv->mutex);

		if (priv->probe_cancelled) {
			BURNER_MEDIA_LOG ("Device probing cancelled");

			burner_device_handle_close (handle);
			goto end;
		}
	}

	BURNER_MEDIA_LOG ("Medium inserted");
	burner_device_handle_close (handle);

	priv->has_medium = TRUE;

end:

	g_mutex_lock (priv->mutex);

	if (!priv->probe_cancelled)
		priv->probe_id = g_idle_add (burner_drive_probed_inside, drive);

	priv->probe = NULL;
	g_cond_broadcast (priv->cond);
	g_mutex_unlock (priv->mutex);

	g_thread_exit (0);

	return NULL;
}

static void
burner_drive_probe_inside (BurnerDrive *drive)
{
	BurnerDrivePrivate *priv;

	priv = BURNER_DRIVE_PRIVATE (drive);

	if (priv->initial_probe) {
		BURNER_MEDIA_LOG ("Still initializing the drive properties");
		return;
	}

	/* Check that a probe is not already being performed */
	if (priv->probe) {
		BURNER_MEDIA_LOG ("Ongoing probe");
		burner_drive_cancel_probing (drive);
	}

	BURNER_MEDIA_LOG ("Setting new probe");

	g_mutex_lock (priv->mutex);

	priv->probe_waiting = FALSE;
	priv->probe_cancelled = FALSE;

	priv->probe = g_thread_create (burner_drive_probe_inside_thread,
	                               drive,
				       FALSE,
				       NULL);

	g_mutex_unlock (priv->mutex);
}

static void
burner_drive_medium_gdrive_changed_cb (BurnerDrive *gdrive,
					BurnerDrive *drive)
{
	BurnerDrivePrivate *priv;

	priv = BURNER_DRIVE_PRIVATE (drive);
	if (priv->locked || priv->ejecting) {
		BURNER_MEDIA_LOG ("Waiting for next unlocking of the drive to probe");

		/* Since the drive was locked, it should
		 * not be possible that the medium
		 * actually changed.
		 * This allows to avoid probing while
		 * we are burning something.
		 * Delay the probe until burner_drive_unlock ()
		 * is called.  */
		priv->probe_waiting = TRUE;
		return;
	}

	BURNER_MEDIA_LOG ("GDrive changed");
	burner_drive_probe_inside (drive);
}

static void
burner_drive_update_gdrive (BurnerDrive *drive,
                             GDrive *gdrive)
{
	BurnerDrivePrivate *priv;

	priv = BURNER_DRIVE_PRIVATE (drive);
	if (priv->gdrive) {
		g_signal_handlers_disconnect_by_func (priv->gdrive,
						      burner_drive_medium_gdrive_changed_cb,
						      drive);

		/* Stop any ongoing GIO operation */
		g_cancellable_cancel (priv->cancel);
	
		g_object_unref (priv->gdrive);
		priv->gdrive = NULL;
	}

	BURNER_MEDIA_LOG ("Setting GDrive %p", gdrive);

	if (gdrive) {
		priv->gdrive = g_object_ref (gdrive);

		/* If it's not a fake drive then connect to signal for any
		 * change and check medium inside */
		g_signal_connect (priv->gdrive,
				  "changed",
				  G_CALLBACK (burner_drive_medium_gdrive_changed_cb),
				  drive);
	}

	if (priv->locked || priv->ejecting) {
		BURNER_MEDIA_LOG ("Waiting for next unlocking of the drive to probe");

		/* Since the drive was locked, it should
		 * not be possible that the medium
		 * actually changed.
		 * This allows to avoid probing while
		 * we are burning something.
		 * Delay the probe until burner_drive_unlock ()
		 * is called.  */
		priv->probe_waiting = TRUE;
		return;
	}

	burner_drive_probe_inside (drive);
}

/**
 * burner_drive_reprobe:
 * @drive: a #BurnerDrive
 *
 * Reprobes the drive contents. Useful when an operation has just been performed
 * (blanking, burning, ...) and medium status should be updated.
 *
 * NOTE: This operation does not block.
 *
 **/

void
burner_drive_reprobe (BurnerDrive *drive)
{
	BurnerDrivePrivate *priv;
	BurnerMedium *medium;

	g_return_if_fail (drive != NULL);
	g_return_if_fail (BURNER_IS_DRIVE (drive));

	priv = BURNER_DRIVE_PRIVATE (drive);
	
	if (priv->gdrive) {
		/* reprobe the contents of the drive system wide */
		g_drive_poll_for_media (priv->gdrive, NULL, NULL, NULL);
	}

	priv->probe_waiting = FALSE;

	BURNER_MEDIA_LOG ("Reprobing inserted medium");
	if (priv->medium) {
		/* remove current medium */
		medium = priv->medium;
		priv->medium = NULL;

		g_signal_emit (drive,
			       drive_signals [MEDIUM_REMOVED],
			       0,
			       medium);
		g_object_unref (medium);
	}

	burner_drive_probe_inside (drive);
}

static gboolean
burner_drive_get_caps_profiles (BurnerDrive *self,
                                 BurnerDeviceHandle *handle,
                                 BurnerScsiErrCode *code)
{
	BurnerScsiGetConfigHdr *hdr = NULL;
	BurnerScsiProfileDesc *profiles;
	BurnerScsiFeatureDesc *desc;
	BurnerDrivePrivate *priv;
	BurnerScsiResult result;
	int profiles_num;
	int size;

	priv = BURNER_DRIVE_PRIVATE (self);

	BURNER_MEDIA_LOG ("Checking supported profiles");
	result = burner_mmc2_get_configuration_feature (handle,
	                                                 BURNER_SCSI_FEAT_PROFILES,
	                                                 &hdr,
	                                                 &size,
	                                                 code);
	if (result != BURNER_SCSI_OK) {
		BURNER_MEDIA_LOG ("GET CONFIGURATION failed");
		return FALSE;
	}

	BURNER_MEDIA_LOG ("Dectected medium is 0x%x", BURNER_GET_16 (hdr->current_profile));

	/* Go through all features available */
	desc = hdr->desc;
	profiles = (BurnerScsiProfileDesc *) desc->data;
	profiles_num = desc->add_len / sizeof (BurnerScsiProfileDesc);

	while (profiles_num) {
		switch (BURNER_GET_16 (profiles->number)) {
			case BURNER_SCSI_PROF_CDR:
				priv->caps |= BURNER_DRIVE_CAPS_CDR;
				break;
			case BURNER_SCSI_PROF_CDRW:
				priv->caps |= BURNER_DRIVE_CAPS_CDRW;
				break;
			case BURNER_SCSI_PROF_DVD_R: 
				priv->caps |= BURNER_DRIVE_CAPS_DVDR;
				break;
			case BURNER_SCSI_PROF_DVD_RW_SEQUENTIAL: 
			case BURNER_SCSI_PROF_DVD_RW_RESTRICTED: 
				priv->caps |= BURNER_DRIVE_CAPS_DVDRW;
				break;
			case BURNER_SCSI_PROF_DVD_RAM: 
				priv->caps |= BURNER_DRIVE_CAPS_DVDRAM;
				break;
			case BURNER_SCSI_PROF_DVD_R_PLUS_DL:
				priv->caps |= BURNER_DRIVE_CAPS_DVDR_PLUS_DL;
				break;
			case BURNER_SCSI_PROF_DVD_RW_PLUS_DL:
				priv->caps |= BURNER_DRIVE_CAPS_DVDRW_PLUS_DL;
				break;
			case BURNER_SCSI_PROF_DVD_R_PLUS:
				priv->caps |= BURNER_DRIVE_CAPS_DVDR_PLUS;
				break;
			case BURNER_SCSI_PROF_DVD_RW_PLUS:
				priv->caps |= BURNER_DRIVE_CAPS_DVDRW_PLUS;
				break;
			case BURNER_SCSI_PROF_BR_R_SEQUENTIAL:
			case BURNER_SCSI_PROF_BR_R_RANDOM:
				priv->caps |= BURNER_DRIVE_CAPS_BDR;
				break;
			case BURNER_SCSI_PROF_BD_RW:
				priv->caps |= BURNER_DRIVE_CAPS_BDRW;
				break;
			default:
				break;
		}

		if (priv->initial_probe_cancelled)
			break;

		/* Move the pointer to the next features */
		profiles ++;
		profiles_num --;
	}

	g_free (hdr);
	return TRUE;
}

static void
burner_drive_get_caps_2A (BurnerDrive *self,
                           BurnerDeviceHandle *handle,
                           BurnerScsiErrCode *code)
{
	BurnerScsiStatusPage *page_2A = NULL;
	BurnerScsiModeData *data = NULL;
	BurnerDrivePrivate *priv;
	BurnerScsiResult result;
	int size = 0;

	priv = BURNER_DRIVE_PRIVATE (self);

	result = burner_spc1_mode_sense_get_page (handle,
						   BURNER_SPC_PAGE_STATUS,
						   &data,
						   &size,
						   code);
	if (result != BURNER_SCSI_OK) {
		BURNER_MEDIA_LOG ("MODE SENSE failed");
		return;
	}

	page_2A = (BurnerScsiStatusPage *) &data->page;

	if (page_2A->wr_CDR != 0)
		priv->caps |= BURNER_DRIVE_CAPS_CDR;
	if (page_2A->wr_CDRW != 0)
		priv->caps |= BURNER_DRIVE_CAPS_CDRW;
	if (page_2A->wr_DVDR != 0)
		priv->caps |= BURNER_DRIVE_CAPS_DVDR;
	if (page_2A->wr_DVDRAM != 0)
		priv->caps |= BURNER_DRIVE_CAPS_DVDRAM;

	g_free (data);
}

static gpointer
burner_drive_probe_thread (gpointer data)
{
	gint counter = 0;
	GTimeVal wait_time;
	const gchar *device;
	BurnerScsiResult res;
	BurnerScsiInquiry hdr;
	BurnerScsiErrCode code;
	BurnerDrivePrivate *priv;
	BurnerDeviceHandle *handle;
	BurnerDrive *drive = BURNER_DRIVE (data);

	priv = BURNER_DRIVE_PRIVATE (drive);

	/* the drive might be busy (a burning is going on) so we don't block
	 * but we re-try to open it every second */
	device = burner_drive_get_device (drive);
	BURNER_MEDIA_LOG ("Trying to open device %s", device);

	handle = burner_device_handle_open (device, FALSE, &code);
	while (!handle && counter <= BURNER_DRIVE_OPEN_ATTEMPTS) {
		sleep (1);

		if (priv->initial_probe_cancelled) {
			BURNER_MEDIA_LOG ("Open () cancelled");
			goto end;
		}

		counter ++;
		handle = burner_device_handle_open (device, FALSE, &code);
	}

	if (priv->initial_probe_cancelled) {
		BURNER_MEDIA_LOG ("Open () cancelled");
		goto end;
	}

	if (!handle) {
		BURNER_MEDIA_LOG ("Open () failed: medium busy");
		goto end;
	}

	while (burner_spc1_test_unit_ready (handle, &code) != BURNER_SCSI_OK) {
		if (code == BURNER_SCSI_NO_MEDIUM) {
			BURNER_MEDIA_LOG ("No medium inserted");
			goto capabilities;
		}

		if (code != BURNER_SCSI_NOT_READY) {
			burner_device_handle_close (handle);
			BURNER_MEDIA_LOG ("Device does not respond");
			goto end;
		}

		g_get_current_time (&wait_time);
		g_time_val_add (&wait_time, 2000000);

		g_mutex_lock (priv->mutex);
		g_cond_timed_wait (priv->cond_probe,
		                   priv->mutex,
		                   &wait_time);
		g_mutex_unlock (priv->mutex);

		if (priv->initial_probe_cancelled) {
			burner_device_handle_close (handle);
			BURNER_MEDIA_LOG ("Device probing cancelled");
			goto end;
		}
	}

	BURNER_MEDIA_LOG ("Device ready");
	priv->has_medium = TRUE;

capabilities:

	/* get additional information like the name */
	res = burner_spc1_inquiry (handle, &hdr, NULL);
	if (res == BURNER_SCSI_OK) {
		gchar *name_utf8;
		gchar *vendor;
		gchar *model;
		gchar *name;

		vendor = g_strndup ((gchar *) hdr.vendor, sizeof (hdr.vendor));
		model = g_strndup ((gchar *) hdr.name, sizeof (hdr.name));
		name = g_strdup_printf ("%s %s", g_strstrip (vendor), g_strstrip (model));
		g_free (vendor);
		g_free (model);

		/* make sure that's proper UTF-8 */
		name_utf8 = g_convert_with_fallback (name,
		                                     -1,
		                                     "ASCII",
		                                     "UTF-8",
		                                     "_",
		                                     NULL,
		                                     NULL,
		                                     NULL);
		g_free (name);

		priv->name = name_utf8;
	}

	/* Get supported medium types */
	if (!burner_drive_get_caps_profiles (drive, handle, &code))
		burner_drive_get_caps_2A (drive, handle, &code);

	burner_device_handle_close (handle);

	BURNER_MEDIA_LOG ("Drive caps are %d", priv->caps);

end:

	g_mutex_lock (priv->mutex);

	burner_drive_update_medium (drive);

	priv->probe = NULL;
	priv->initial_probe = FALSE;

	g_cond_broadcast (priv->cond);
	g_mutex_unlock (priv->mutex);

	g_thread_exit (0);

	return NULL;
}

static void
burner_drive_init_real_device (BurnerDrive *drive,
                                const gchar *device)
{
	BurnerDrivePrivate *priv;

	priv = BURNER_DRIVE_PRIVATE (drive);

#if defined(HAVE_STRUCT_USCSI_CMD)
	/* On Solaris path points to raw device, block_path points to the block device. */
	g_assert(g_str_has_prefix(device, "/dev/dsk/"));
	priv->device = g_strdup_printf ("/dev/rdsk/%s", device + 9);
	priv->block_device = g_strdup (device);
	BURNER_MEDIA_LOG ("Initializing block drive %s", priv->block_device);
#else
	priv->device = g_strdup (device);
#endif

	BURNER_MEDIA_LOG ("Initializing drive %s from device", priv->device);

	/* NOTE: why a thread? Because in case of a damaged medium, burner can
	 * block on some functions until timeout and if we do this in the main
	 * thread then our whole UI blocks. This medium won't be exported by the
	 * BurnerDrive that exported until it returns PROBED signal.
	 * One (good) side effect is that it also improves start time. */
	g_mutex_lock (priv->mutex);

	priv->initial_probe = TRUE;
	priv->probe = g_thread_create (burner_drive_probe_thread,
				       drive,
				       FALSE,
				       NULL);

	g_mutex_unlock (priv->mutex);
}

static void
burner_drive_set_property (GObject *object,
			    guint prop_id,
			    const GValue *value,
			    GParamSpec *pspec)
{
	BurnerDrivePrivate *priv;
	GDrive *gdrive = NULL;

	g_return_if_fail (BURNER_IS_DRIVE (object));

	priv = BURNER_DRIVE_PRIVATE (object);

	switch (prop_id)
	{
	case PROP_UDI:
		break;
	case PROP_GDRIVE:
		if (!priv->device)
			break;

		gdrive = g_value_get_object (value);
		burner_drive_update_gdrive (BURNER_DRIVE (object), gdrive);
		break;
	case PROP_DEVICE:
		/* The first case is only a fake drive/medium */
		if (!g_value_get_string (value))
			priv->medium = g_object_new (BURNER_TYPE_VOLUME,
						     "drive", object,
						     NULL);
		else
			burner_drive_init_real_device (BURNER_DRIVE (object), g_value_get_string (value));
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
		break;
	}
}

static void
burner_drive_get_property (GObject *object,
			    guint prop_id,
			    GValue *value,
			    GParamSpec *pspec)
{
	BurnerDrivePrivate *priv;

	g_return_if_fail (BURNER_IS_DRIVE (object));

	priv = BURNER_DRIVE_PRIVATE (object);

	switch (prop_id)
	{
	case PROP_UDI:
		break;
	case PROP_GDRIVE:
		g_value_set_object (value, priv->gdrive);
		break;
	case PROP_DEVICE:
		g_value_set_string (value, priv->device);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
		break;
	}
}

static void
burner_drive_init (BurnerDrive *object)
{
	BurnerDrivePrivate *priv;

	priv = BURNER_DRIVE_PRIVATE (object);
	priv->cancel = g_cancellable_new ();

	priv->mutex = g_mutex_new ();
	priv->cond = g_cond_new ();
	priv->cond_probe = g_cond_new ();
}

static void
burner_drive_finalize (GObject *object)
{
	BurnerDrivePrivate *priv;

	priv = BURNER_DRIVE_PRIVATE (object);

	BURNER_MEDIA_LOG ("Finalizing BurnerDrive");

	burner_drive_cancel_probing (BURNER_DRIVE (object));

	if (priv->mutex) {
		g_mutex_free (priv->mutex);
		priv->mutex = NULL;
	}

	if (priv->cond) {
		g_cond_free (priv->cond);
		priv->cond = NULL;
	}

	if (priv->cond_probe) {
		g_cond_free (priv->cond_probe);
		priv->cond_probe = NULL;
	}

	if (priv->medium) {
		g_signal_emit (object,
			       drive_signals [MEDIUM_REMOVED],
			       0,
			       priv->medium);
		g_object_unref (priv->medium);
		priv->medium = NULL;
	}

	if (priv->name) {
		g_free (priv->name);
		priv->name = NULL;
	}

	if (priv->device) {
		g_free (priv->device);
		priv->device = NULL;
	}

	if (priv->block_device) {
		g_free (priv->block_device);
		priv->block_device = NULL;
	}

	if (priv->udi) {
		g_free (priv->udi);
		priv->udi = NULL;
	}

	if (priv->gdrive) {
		g_signal_handlers_disconnect_by_func (priv->gdrive,
						      burner_drive_medium_gdrive_changed_cb,
						      object);
		g_object_unref (priv->gdrive);
		priv->gdrive = NULL;
	}

	if (priv->cancel) {
		g_cancellable_cancel (priv->cancel);
		g_object_unref (priv->cancel);
		priv->cancel = NULL;
	}

	G_OBJECT_CLASS (burner_drive_parent_class)->finalize (object);
}

static void
burner_drive_class_init (BurnerDriveClass *klass)
{
	GObjectClass* object_class = G_OBJECT_CLASS (klass);

	g_type_class_add_private (klass, sizeof (BurnerDrivePrivate));

	object_class->finalize = burner_drive_finalize;
	object_class->set_property = burner_drive_set_property;
	object_class->get_property = burner_drive_get_property;

	/**
 	* BurnerDrive::medium-added:
 	* @drive: the object which received the signal
  	* @medium: the new medium which was added
	*
 	* This signal gets emitted when a new medium was detected
 	*
 	*/
	drive_signals[MEDIUM_INSERTED] =
		g_signal_new ("medium_added",
		              G_OBJECT_CLASS_TYPE (klass),
		              G_SIGNAL_RUN_LAST | G_SIGNAL_NO_RECURSE,
		              G_STRUCT_OFFSET (BurnerDriveClass, medium_added),
		              NULL, NULL,
		              g_cclosure_marshal_VOID__OBJECT,
		              G_TYPE_NONE, 1,
		              BURNER_TYPE_MEDIUM);

	/**
 	* BurnerDrive::medium-removed:
 	* @drive: the object which received the signal
  	* @medium: the medium which was removed
	*
 	* This signal gets emitted when a medium is not longer available
 	*
 	*/
	drive_signals[MEDIUM_REMOVED] =
		g_signal_new ("medium_removed",
		              G_OBJECT_CLASS_TYPE (klass),
		              G_SIGNAL_RUN_LAST | G_SIGNAL_NO_RECURSE,
		              G_STRUCT_OFFSET (BurnerDriveClass, medium_removed),
		              NULL, NULL,
		              g_cclosure_marshal_VOID__OBJECT,
		              G_TYPE_NONE, 1,
		              BURNER_TYPE_MEDIUM);

	g_object_class_install_property (object_class,
	                                 PROP_UDI,
	                                 g_param_spec_string("udi",
	                                                     "udi",
	                                                     "HAL udi as a string (Deprecated)",
	                                                     NULL,
	                                                     G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
	g_object_class_install_property (object_class,
	                                 PROP_GDRIVE,
	                                 g_param_spec_object ("gdrive",
	                                                      "GDrive",
	                                                      "A GDrive object for the drive",
	                                                      G_TYPE_DRIVE,
	                                                     G_PARAM_READWRITE));
	g_object_class_install_property (object_class,
	                                 PROP_DEVICE,
	                                 g_param_spec_string ("device",
	                                                      "Device",
	                                                      "Device path for the drive",
	                                                      NULL,
	                                                      G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
}

