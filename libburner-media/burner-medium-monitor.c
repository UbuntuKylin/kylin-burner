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

#include <string.h>

#include <glib.h>
#include <glib/gi18n-lib.h>

#include <gio/gio.h>

#include "burner-media-private.h"

#include "burner-drive-priv.h"

#include "scsi-device.h"
#include "scsi-utils.h"
#include "scsi-spc1.h"

#include "burner-drive.h"
#include "burner-medium.h"
#include "burner-medium-monitor.h"

typedef struct _BurnerMediumMonitorPrivate BurnerMediumMonitorPrivate;
struct _BurnerMediumMonitorPrivate
{
	GSList *drives;
	GVolumeMonitor *gmonitor;

	GSList *waiting_removal;
	guint waiting_removal_id;

	gint probing;
};

#define BURNER_MEDIUM_MONITOR_PRIVATE(o)  (G_TYPE_INSTANCE_GET_PRIVATE ((o), BURNER_TYPE_MEDIUM_MONITOR, BurnerMediumMonitorPrivate))

enum
{
	MEDIUM_INSERTED,
	MEDIUM_REMOVED,
	DRIVE_ADDED,
	DRIVE_REMOVED,

	LAST_SIGNAL
};


static guint medium_monitor_signals[LAST_SIGNAL] = { 0 };

G_DEFINE_TYPE (BurnerMediumMonitor, burner_medium_monitor, G_TYPE_OBJECT);


/**
 * burner_medium_monitor_get_drive:
 * @monitor: a #BurnerMediumMonitor
 * @device: the path of the device
 *
 * Returns the #BurnerDrive object whose path is @path.
 *
 * Return value: a #BurnerDrive or NULL. It should be unreffed when no longer in use.
 **/
BurnerDrive *
burner_medium_monitor_get_drive (BurnerMediumMonitor *monitor,
				  const gchar *device)
{
	GSList *iter;
	BurnerMediumMonitorPrivate *priv;

	g_return_val_if_fail (monitor != NULL, NULL);
	g_return_val_if_fail (device != NULL, NULL);
	g_return_val_if_fail (BURNER_IS_MEDIUM_MONITOR (monitor), NULL);

	priv = BURNER_MEDIUM_MONITOR_PRIVATE (monitor);
	for (iter = priv->drives; iter; iter = iter->next) {
		BurnerDrive *drive;
		const gchar *drive_device;

		drive = iter->data;
		drive_device = burner_drive_get_device (drive);
		if (drive_device && !strcmp (drive_device, device)) {
			g_object_ref (drive);
			return drive;
		}
	}

	return NULL;
}

/**
 * burner_medium_monitor_is_probing:
 * @monitor: a #BurnerMediumMonitor
 *
 * Returns if the library is still probing some other media.
 *
 * Return value: %TRUE if it is still probing some media
 **/
gboolean
burner_medium_monitor_is_probing (BurnerMediumMonitor *monitor)
{
	GSList *iter;
	BurnerMediumMonitorPrivate *priv;

	g_return_val_if_fail (monitor != NULL, FALSE);
	g_return_val_if_fail (BURNER_IS_MEDIUM_MONITOR (monitor), FALSE);

	priv = BURNER_MEDIUM_MONITOR_PRIVATE (monitor);

	for (iter = priv->drives; iter; iter = iter->next) {
		BurnerDrive *drive;

		drive = iter->data;
		if (burner_drive_is_fake (drive))
			continue;

		if (burner_drive_probing (drive))
			return TRUE;
	}

	return FALSE;
}

/**
 * burner_medium_monitor_get_drives:
 * @monitor: a #BurnerMediumMonitor
 * @type: a #BurnerDriveType to tell what type of drives to include in the list
 *
 * Gets the list of available drives that are of the given type.
 *
 * Return value: (element-type BurnerMedia.Drive) (transfer full): a #GSList of  #BurnerDrive or NULL. The list must be freed and the element unreffed when finished.
 **/
GSList *
burner_medium_monitor_get_drives (BurnerMediumMonitor *monitor,
				   BurnerDriveType type)
{
	BurnerMediumMonitorPrivate *priv;
	GSList *drives = NULL;
	GSList *iter;

	g_return_val_if_fail (monitor != NULL, NULL);
	g_return_val_if_fail (BURNER_IS_MEDIUM_MONITOR (monitor), NULL);

	priv = BURNER_MEDIUM_MONITOR_PRIVATE (monitor);

	for (iter = priv->drives; iter; iter = iter->next) {
		BurnerDrive *drive;

		drive = iter->data;
		if (burner_drive_is_fake (drive)) {
			if (type & BURNER_DRIVE_TYPE_FILE)
				drives = g_slist_prepend (drives, drive);

			continue;
		}

		if (burner_drive_can_write (drive)
		&& (type & BURNER_DRIVE_TYPE_WRITER)) {
			drives = g_slist_prepend (drives, drive);
			continue;
		}

		if (type & BURNER_DRIVE_TYPE_READER) {
			drives = g_slist_prepend (drives, drive);
			continue;
		}
	}
	g_slist_foreach (drives, (GFunc) g_object_ref, NULL);

	return drives;
}

/**
 * burner_medium_monitor_get_media:
 * @monitor: a #BurnerMediumMonitor
 * @type: the type of #BurnerMedium that should be in the list
 *
 * Obtains the list of available media that are of the given type.
 *
 * Return value: (element-type BurnerMedia.Medium) (transfer full): a #GSList of  #BurnerMedium or NULL. The list must be freed and the element unreffed when finished.
 **/
GSList *
burner_medium_monitor_get_media (BurnerMediumMonitor *monitor,
				  BurnerMediaType type)
{
	GSList *iter;
	GSList *list = NULL;
	BurnerMediumMonitorPrivate *priv;

	g_return_val_if_fail (monitor != NULL, NULL);
	g_return_val_if_fail (BURNER_IS_MEDIUM_MONITOR (monitor), NULL);

	priv = BURNER_MEDIUM_MONITOR_PRIVATE (monitor);

	for (iter = priv->drives; iter; iter = iter->next) {
		BurnerMedium *medium;
		BurnerDrive *drive;

		drive = iter->data;

		medium = burner_drive_get_medium (drive);
		if (!medium)
			continue;

		if ((type & BURNER_MEDIA_TYPE_CD) == type
		&& (burner_medium_get_status (medium) & BURNER_MEDIUM_CD)) {
			/* If used alone, returns all CDs */
			list = g_slist_prepend (list, medium);
			g_object_ref (medium);
			continue;
		}

		if ((type & BURNER_MEDIA_TYPE_ANY_IN_BURNER)
		&&  (burner_drive_can_write (drive))) {
			if ((type & BURNER_MEDIA_TYPE_CD)) {
				if ((burner_medium_get_status (medium) & BURNER_MEDIUM_CD)) {
					list = g_slist_prepend (list, medium);
					g_object_ref (medium);
					continue;
				}
			}
			else {
				list = g_slist_prepend (list, medium);
				g_object_ref (medium);
				continue;
			}
			continue;
		}

		if ((type & BURNER_MEDIA_TYPE_AUDIO)
		&& !(burner_medium_get_status (medium) & BURNER_MEDIUM_FILE)
		&&  (burner_medium_get_status (medium) & BURNER_MEDIUM_HAS_AUDIO)) {
			if ((type & BURNER_MEDIA_TYPE_CD)) {
				if ((burner_medium_get_status (medium) & BURNER_MEDIUM_CD)) {
					list = g_slist_prepend (list, medium);
					g_object_ref (medium);
					continue;
				}
			}
			else {
				list = g_slist_prepend (list, medium);
				g_object_ref (medium);
				continue;
			}
			continue;
		}

		if ((type & BURNER_MEDIA_TYPE_DATA)
		&& !(burner_medium_get_status (medium) & BURNER_MEDIUM_FILE)
		&&  (burner_medium_get_status (medium) & BURNER_MEDIUM_HAS_DATA)) {
			if ((type & BURNER_MEDIA_TYPE_CD)) {
				if ((burner_medium_get_status (medium) & BURNER_MEDIUM_CD)) {
					list = g_slist_prepend (list, medium);
					g_object_ref (medium);
					continue;
				}
			}
			else {
				list = g_slist_prepend (list, medium);
				g_object_ref (medium);
				continue;
			}
			continue;
		}

		if (type & BURNER_MEDIA_TYPE_WRITABLE) {
			if (burner_medium_can_be_written (medium)) {
				if ((type & BURNER_MEDIA_TYPE_CD)) {
					if ((burner_medium_get_status (medium) & BURNER_MEDIUM_CD)) {
						list = g_slist_prepend (list, medium);
						g_object_ref (medium);
						continue;
					}
				}
				else {
					list = g_slist_prepend (list, medium);
					g_object_ref (medium);
					continue;
				}
			}
		}

		if (type & BURNER_MEDIA_TYPE_REWRITABLE) {
			if (burner_medium_can_be_rewritten (medium)) {
				if ((type & BURNER_MEDIA_TYPE_CD)) {
					if ((burner_medium_get_status (medium) & BURNER_MEDIUM_CD)) {
						list = g_slist_prepend (list, medium);
						g_object_ref (medium);
						continue;
					}
				}
				else {
					list = g_slist_prepend (list, medium);
					g_object_ref (medium);
					continue;
				}
			}
		}

		if (type & BURNER_MEDIA_TYPE_FILE) {
			/* make sure the drive is indeed a fake one
			 * since it can happen that some medium did
			 * not properly carry out their initialization 
			 * and are flagged as BURNER_MEDIUM_FILE
			 * whereas they are not */
			if (burner_drive_is_fake (drive)) {
				list = g_slist_prepend (list, medium);
				g_object_ref (medium);
			}
		}
	}

	return list;
}

static void
burner_medium_monitor_medium_added_cb (BurnerDrive *drive,
					BurnerMedium *medium,
					BurnerMediumMonitor *self)
{
	g_signal_emit (self,
		       medium_monitor_signals [MEDIUM_INSERTED],
		       0,
		       medium);
}

static void
burner_medium_monitor_medium_removed_cb (BurnerDrive *drive,
					  BurnerMedium *medium,
					  BurnerMediumMonitor *self)
{
	g_signal_emit (self,
		       medium_monitor_signals [MEDIUM_REMOVED],
		       0,
		       medium);
}

static gboolean
burner_medium_monitor_is_drive (BurnerMediumMonitor *monitor,
                                 const gchar *device)
{
	BurnerDeviceHandle *handle;
	BurnerScsiErrCode code;
	gboolean result;

	BURNER_MEDIA_LOG ("Testing drive %s", device);

	handle = burner_device_handle_open (device, FALSE, &code);
	if (!handle)
		return FALSE;

	result = (burner_spc1_inquiry_is_optical_drive (handle, &code) == BURNER_SCSI_OK);
	burner_device_handle_close (handle);

	BURNER_MEDIA_LOG ("Drive %s", result? "is optical":"is not optical");

	return result;
}

static BurnerDrive *
burner_medium_monitor_drive_new (BurnerMediumMonitor *self,
                                  const gchar *device,
                                  GDrive *gdrive)
{
	BurnerMediumMonitorPrivate *priv;
	BurnerDrive *drive;

	if (!burner_medium_monitor_is_drive (self, device))
		return NULL;

	priv = BURNER_MEDIUM_MONITOR_PRIVATE (self);
	drive = g_object_new (BURNER_TYPE_DRIVE,
	                      "device", device,
			      "gdrive", gdrive,
			      NULL);

	priv->drives = g_slist_prepend (priv->drives, drive);

	g_signal_connect (drive,
			  "medium-added",
			  G_CALLBACK (burner_medium_monitor_medium_added_cb),
			  self);
	g_signal_connect (drive,
			  "medium-removed",
			  G_CALLBACK (burner_medium_monitor_medium_removed_cb),
			  self);

	return drive;
}

static void
burner_medium_monitor_device_added (BurnerMediumMonitor *self,
                                     const gchar *device,
                                     GDrive *gdrive)
{
	BurnerMediumMonitorPrivate *priv;
	BurnerDrive *drive = NULL;

	priv = BURNER_MEDIUM_MONITOR_PRIVATE (self);

	/* See if the drive is waiting removal.
	 * This is necessary as GIO behaves strangely sometimes
	 * since it sends the "disconnected" signal when a medium
	 * is removed soon followed by a "connected" signal */
	drive = burner_medium_monitor_get_drive (self, device);
	if (drive) {
		/* Just in case that drive was waiting removal */
		priv->waiting_removal = g_slist_remove (priv->waiting_removal, drive);

		BURNER_MEDIA_LOG ("Added signal was emitted but the drive is in the removal list. Updating GDrive associated object.");
		g_object_set (drive,
		              "gdrive", gdrive,
		              NULL);

		g_object_unref (drive);
		return;
	}

	/* Make sure it's an optical drive */
	drive = burner_medium_monitor_drive_new (self, device, gdrive);
	if (!drive)
		return;

	BURNER_MEDIA_LOG ("New drive added");
	g_signal_emit (self,
		       medium_monitor_signals [DRIVE_ADDED],
		       0,
		       drive);

	/* check if a medium is inserted */
	if (burner_drive_get_medium (drive))
		g_signal_emit (self,
			       medium_monitor_signals [MEDIUM_INSERTED],
			       0,
			       burner_drive_get_medium (drive));
}

static void
burner_medium_monitor_connected_cb (GVolumeMonitor *monitor,
                                     GDrive *gdrive,
                                     BurnerMediumMonitor *self)
{
	gchar *device;

	BURNER_MEDIA_LOG ("GDrive addition signal");

	device = g_drive_get_identifier (gdrive, G_VOLUME_IDENTIFIER_KIND_UNIX_DEVICE);
	burner_medium_monitor_device_added (self, device, gdrive);
	g_free (device);
}

static void
burner_medium_monitor_volume_added_cb (GVolumeMonitor *monitor,
                                        GVolume *gvolume,
                                        BurnerMediumMonitor *self)
{
	gchar *device;
	GDrive *gdrive;

	BURNER_MEDIA_LOG ("GVolume addition signal");

	/* No need to signal that addition if the GVolume
	 * object has an associated GDrive as this is just
	 * meant to trap blank discs which have no GDrive
	 * associated but a GVolume. */
	gdrive = g_volume_get_drive (gvolume);
	if (gdrive) {
		BURNER_MEDIA_LOG ("Existing GDrive skipping");
		g_object_unref (gdrive);
		return;
	}

	device = g_volume_get_identifier (gvolume, G_VOLUME_IDENTIFIER_KIND_UNIX_DEVICE);
	if  (!device)
		return;

	burner_medium_monitor_device_added (self, device, NULL);
	g_free (device);
}

static gboolean
burner_medium_monitor_disconnected_real (gpointer data)
{
	BurnerMediumMonitor *self = BURNER_MEDIUM_MONITOR (data);
	BurnerMediumMonitorPrivate *priv;
	BurnerMedium *medium;
	BurnerDrive *drive;

	priv = BURNER_MEDIUM_MONITOR_PRIVATE (self);

	if (!priv->waiting_removal) {
		priv->waiting_removal_id = 0;
		return FALSE;
	}

	drive = priv->waiting_removal->data;
	priv->waiting_removal = g_slist_remove (priv->waiting_removal, drive);

	BURNER_MEDIA_LOG ("Drive removed");
	medium = burner_drive_get_medium (drive);

	/* disconnect the signal handlers to avoid having the "medium-removed" fired twice */
	g_signal_handlers_disconnect_by_func (drive,
	                                      burner_medium_monitor_medium_added_cb,
	                                      self);
	g_signal_handlers_disconnect_by_func (drive,
	                                      burner_medium_monitor_medium_removed_cb,
	                                      self);

	if (medium)
		g_signal_emit (self,
			       medium_monitor_signals [MEDIUM_REMOVED],
			       0,
			       medium);

	priv->drives = g_slist_remove (priv->drives, drive);
	g_signal_emit (self,
		       medium_monitor_signals [DRIVE_REMOVED],
		       0,
		       drive);
	g_object_unref (drive);

	/* in case there are more */
	return TRUE;
}

static void
burner_medium_monitor_device_removed (BurnerMediumMonitor *self,
                                       const gchar *device,
                                       GDrive *gdrive)
{
	BurnerMediumMonitorPrivate *priv;
	GDrive *associated_gdrive;
	BurnerDrive *drive;

	priv = BURNER_MEDIUM_MONITOR_PRIVATE (self);

	/* Make sure it's one already detected */
	/* GIO behaves strangely: every time a medium 
	 * is removed from a drive it emits the disconnected
	 * signal (which IMO it shouldn't) soon followed by
	 * a connected signal.
	 * So delay the removal by one or two seconds. */

	drive = burner_medium_monitor_get_drive (self, device);
	if (!drive)
		return;

	if (G_UNLIKELY (g_slist_find (priv->waiting_removal, drive) != NULL)) {
		g_object_unref (drive);
		return;
	}

	associated_gdrive = burner_drive_get_gdrive (drive);
	if (associated_gdrive == gdrive) {
		BURNER_MEDIA_LOG ("Found device to remove");
		priv->waiting_removal = g_slist_append (priv->waiting_removal, drive);

		if (!priv->waiting_removal_id)
			priv->waiting_removal_id = g_timeout_add_seconds (2,
			                                                  burner_medium_monitor_disconnected_real, 
			                                                  self);
	}
	/* else do nothing and wait for a "drive-disconnected" signal */

	if (associated_gdrive)
		g_object_unref (associated_gdrive);

	g_object_unref (drive);
}

static void
burner_medium_monitor_volume_removed_cb (GVolumeMonitor *monitor,
                                          GVolume *gvolume,
                                          BurnerMediumMonitor *self)
{
	gchar *device;
	GDrive *gdrive;

	BURNER_MEDIA_LOG ("Volume removal signal");

	/* No need to signal that removal if the GVolume
	 * object has an associated GDrive as this is just
	 * meant to trap blank discs which have no GDrive
	 * associated but a GVolume. */
	gdrive = g_volume_get_drive (gvolume);
	if (gdrive) {
		g_object_unref (gdrive);
		return;
	}

	device = g_volume_get_identifier (gvolume, G_VOLUME_IDENTIFIER_KIND_UNIX_DEVICE);
	if (!device)
		return;

	burner_medium_monitor_device_removed (self, device, NULL);
	g_free (device);
}

static void
burner_medium_monitor_disconnected_cb (GVolumeMonitor *monitor,
                                        GDrive *gdrive,
                                        BurnerMediumMonitor *self)
{
	gchar *device;

	BURNER_MEDIA_LOG ("Drive removal signal");

	device = g_drive_get_identifier (gdrive, G_VOLUME_IDENTIFIER_KIND_UNIX_DEVICE);
	burner_medium_monitor_device_removed (self, device, gdrive);
	g_free (device);
}

static void
burner_medium_monitor_init (BurnerMediumMonitor *object)
{
	GList *iter;
	GList *drives;
	GList *volumes;
	BurnerDrive *drive;
	BurnerMediumMonitorPrivate *priv;

	priv = BURNER_MEDIUM_MONITOR_PRIVATE (object);

	BURNER_MEDIA_LOG ("Probing drives and media");

	priv->gmonitor = g_volume_monitor_get ();

	drives = g_volume_monitor_get_connected_drives (priv->gmonitor);
	BURNER_MEDIA_LOG ("Found %d drives", g_list_length (drives));

	for (iter = drives; iter; iter = iter->next) {
		GDrive *gdrive;
		gchar *device;

		gdrive = iter->data;

		device = g_drive_get_identifier (gdrive, G_VOLUME_IDENTIFIER_KIND_UNIX_DEVICE);
		burner_medium_monitor_drive_new (object, device, gdrive);
		g_free (device);
	}
	g_list_foreach (drives, (GFunc) g_object_unref, NULL);
	g_list_free (drives);

	volumes = g_volume_monitor_get_volumes (priv->gmonitor);
	BURNER_MEDIA_LOG ("Found %d volumes", g_list_length (volumes));

	for (iter = volumes; iter; iter = iter->next) {
		GVolume *gvolume;
		gchar *device;

		gvolume = iter->data;
		device = g_volume_get_identifier (gvolume, G_VOLUME_IDENTIFIER_KIND_UNIX_DEVICE);
		if (!device)
			continue;

		/* make sure it isn't already in our list */
		drive = burner_medium_monitor_get_drive (object, device);
		if (drive) {
			g_free (device);
			g_object_unref (drive);
			continue;
		}

		burner_medium_monitor_drive_new (object, device, NULL);
		g_free (device);
	}
	g_list_foreach (volumes, (GFunc) g_object_unref, NULL);
	g_list_free (volumes);

	g_signal_connect (priv->gmonitor,
			  "volume-added",
			  G_CALLBACK (burner_medium_monitor_volume_added_cb),
			  object);
	g_signal_connect (priv->gmonitor,
			  "volume-removed",
			  G_CALLBACK (burner_medium_monitor_volume_removed_cb),
			  object);
	g_signal_connect (priv->gmonitor,
			  "drive-connected",
			  G_CALLBACK (burner_medium_monitor_connected_cb),
			  object);
	g_signal_connect (priv->gmonitor,
			  "drive-disconnected",
			  G_CALLBACK (burner_medium_monitor_disconnected_cb),
			  object);

	/* add fake/file drive */
	drive = g_object_new (BURNER_TYPE_DRIVE,
	                      "device", NULL,
	                      NULL);
	priv->drives = g_slist_prepend (priv->drives, drive);

	return;
}

static void
burner_medium_monitor_finalize (GObject *object)
{
	BurnerMediumMonitorPrivate *priv;

	priv = BURNER_MEDIUM_MONITOR_PRIVATE (object);

	if (priv->waiting_removal_id) {
		g_source_remove (priv->waiting_removal_id);
		priv->waiting_removal_id = 0;
	}

	if (priv->waiting_removal) {
		g_slist_free (priv->waiting_removal);
		priv->waiting_removal = NULL;
	}

	if (priv->drives) {
		g_slist_foreach (priv->drives, (GFunc) g_object_unref, NULL);
		g_slist_free (priv->drives);
		priv->drives = NULL;
	}

	if (priv->gmonitor) {
		g_signal_handlers_disconnect_by_func (priv->gmonitor,
		                                      burner_medium_monitor_volume_added_cb,
		                                      object);
		g_signal_handlers_disconnect_by_func (priv->gmonitor,
		                                      burner_medium_monitor_volume_removed_cb,
		                                      object);
		g_signal_handlers_disconnect_by_func (priv->gmonitor,
		                                      burner_medium_monitor_connected_cb,
		                                      object);
		g_signal_handlers_disconnect_by_func (priv->gmonitor,
		                                      burner_medium_monitor_disconnected_cb,
		                                      object);
		g_object_unref (priv->gmonitor);
		priv->gmonitor = NULL;
	}

	G_OBJECT_CLASS (burner_medium_monitor_parent_class)->finalize (object);
}

static void
burner_medium_monitor_class_init (BurnerMediumMonitorClass *klass)
{
	GObjectClass* object_class = G_OBJECT_CLASS (klass);

	g_type_class_add_private (klass, sizeof (BurnerMediumMonitorPrivate));

	object_class->finalize = burner_medium_monitor_finalize;

	/**
 	* BurnerMediumMonitor::medium-added:
 	* @monitor: the object which received the signal
  	* @medium: the new medium which was added
	*
 	* This signal gets emitted when a new medium was detected
 	**/
	medium_monitor_signals[MEDIUM_INSERTED] =
		g_signal_new ("medium_added",
		              G_OBJECT_CLASS_TYPE (klass),
		              G_SIGNAL_RUN_LAST | G_SIGNAL_NO_RECURSE,
		              G_STRUCT_OFFSET (BurnerMediumMonitorClass, medium_added),
		              NULL, NULL,
		              g_cclosure_marshal_VOID__OBJECT,
		              G_TYPE_NONE, 1,
		              BURNER_TYPE_MEDIUM);

	/**
 	* BurnerMediumMonitor::medium-removed:
 	* @monitor: the object which received the signal
  	* @medium: the medium which was removed
	*
 	* This signal gets emitted when a medium is not longer available
 	**/
	medium_monitor_signals[MEDIUM_REMOVED] =
		g_signal_new ("medium_removed",
		              G_OBJECT_CLASS_TYPE (klass),
		              G_SIGNAL_RUN_LAST | G_SIGNAL_NO_RECURSE,
		              G_STRUCT_OFFSET (BurnerMediumMonitorClass, medium_removed),
		              NULL, NULL,
		              g_cclosure_marshal_VOID__OBJECT,
		              G_TYPE_NONE, 1,
		              BURNER_TYPE_MEDIUM);

	/**
 	* BurnerMediumMonitor::drive-added:
 	* @monitor: the object which received the signal
  	* @medium: the new medium which was added
	*
 	* This signal gets emitted when a new drive was detected
 	**/
	medium_monitor_signals[DRIVE_ADDED] =
		g_signal_new ("drive_added",
		              G_OBJECT_CLASS_TYPE (klass),
		              G_SIGNAL_RUN_LAST | G_SIGNAL_NO_RECURSE,
		              G_STRUCT_OFFSET (BurnerMediumMonitorClass, drive_added),
		              NULL, NULL,
		              g_cclosure_marshal_VOID__OBJECT,
		              G_TYPE_NONE, 1,
		              BURNER_TYPE_DRIVE);

	/**
 	* BurnerMediumMonitor::drive-removed:
 	* @monitor: the object which received the signal
  	* @medium: the medium which was removed
	*
 	* This signal gets emitted when a drive is not longer available
 	**/
	medium_monitor_signals[DRIVE_REMOVED] =
		g_signal_new ("drive_removed",
		              G_OBJECT_CLASS_TYPE (klass),
		              G_SIGNAL_RUN_LAST | G_SIGNAL_NO_RECURSE,
		              G_STRUCT_OFFSET (BurnerMediumMonitorClass, drive_removed),
		              NULL, NULL,
		              g_cclosure_marshal_VOID__OBJECT,
		              G_TYPE_NONE, 1,
		              BURNER_TYPE_DRIVE);
}

static BurnerMediumMonitor *singleton = NULL;

/**
 * burner_medium_monitor_get_default:
 *
 * Gets the currently active monitor.
 *
 * Return value: a #BurnerMediumMonitor. Unref when it is not needed anymore.
 **/
BurnerMediumMonitor *
burner_medium_monitor_get_default (void)
{
	if (singleton) {
		g_object_ref (singleton);
		return singleton;
	}

	singleton = g_object_new (BURNER_TYPE_MEDIUM_MONITOR, NULL);

	/* keep a reference */
	g_object_ref (singleton);
	return singleton;
}
