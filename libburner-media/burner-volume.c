/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/*
 * Burner
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
#include "burner-volume.h"
#include "burner-gio-operation.h"

typedef struct _BurnerVolumePrivate BurnerVolumePrivate;
struct _BurnerVolumePrivate
{
	GCancellable *cancel;
};

#define BURNER_VOLUME_PRIVATE(o)  (G_TYPE_INSTANCE_GET_PRIVATE ((o), BURNER_TYPE_VOLUME, BurnerVolumePrivate))

G_DEFINE_TYPE (BurnerVolume, burner_volume, BURNER_TYPE_MEDIUM);

/**
 * burner_volume_get_gvolume:
 * @volume: #BurnerVolume
 *
 * Gets the corresponding #GVolume for @volume.
 *
 * Return value: a #GVolume *.
 *
 **/
GVolume *
burner_volume_get_gvolume (BurnerVolume *volume)
{
	const gchar *volume_path = NULL;
	GVolumeMonitor *monitor;
	GVolume *gvolume = NULL;
	BurnerDrive *drive;
	GList *volumes;
	GList *iter;

	g_return_val_if_fail (volume != NULL, NULL);
	g_return_val_if_fail (BURNER_IS_VOLUME (volume), NULL);

	drive = burner_medium_get_drive (BURNER_MEDIUM (volume));

	/* This returns the block device which is the
	 * same as the device for all OSes except
	 * Solaris where the device is the raw device. */
	volume_path = burner_drive_get_block_device (drive);

	/* NOTE: medium-monitor already holds a reference for GVolumeMonitor */
	monitor = g_volume_monitor_get ();
	volumes = g_volume_monitor_get_volumes (monitor);
	g_object_unref (monitor);

	for (iter = volumes; iter; iter = iter->next) {
		gchar *device_path;
		GVolume *tmp;

		tmp = iter->data;
		device_path = g_volume_get_identifier (tmp, G_VOLUME_IDENTIFIER_KIND_UNIX_DEVICE);
		if (!device_path)
			continue;

		BURNER_MEDIA_LOG ("Found volume %s", device_path);
		if (!strcmp (device_path, volume_path)) {
			gvolume = tmp;
			g_free (device_path);
			g_object_ref (gvolume);
			break;
		}

		g_free (device_path);
	}
	g_list_foreach (volumes, (GFunc) g_object_unref, NULL);
	g_list_free (volumes);

	if (!gvolume)
		BURNER_MEDIA_LOG ("No volume found for medium");

	return gvolume;
}

/**
 * burner_volume_is_mounted:
 * @volume: #BurnerVolume
 *
 * Returns whether the volume is currently mounted.
 *
 * Return value: a #gboolean. TRUE if it is mounted.
 *
 **/
gboolean
burner_volume_is_mounted (BurnerVolume *volume)
{
	gchar *path;

	g_return_val_if_fail (volume != NULL, FALSE);
	g_return_val_if_fail (BURNER_IS_VOLUME (volume), FALSE);

	/* NOTE: that's the surest way to know if a drive is really mounted. For
	 * GIO a blank medium can be mounted to burn:/// which is not really 
	 * what we're interested in. So the mount path must be also local. */
	path = burner_volume_get_mount_point (volume, NULL);
	if (path) {
		g_free (path);
		return TRUE;
	}

	return FALSE;
}

/**
 * burner_volume_get_mount_point:
 * @volume: #BurnerVolume
 * @error: #GError **
 *
 * Returns the path for mount point for @volume.
 *
 * Return value: a #gchar *
 *
 **/
gchar *
burner_volume_get_mount_point (BurnerVolume *volume,
				GError **error)
{
	gchar *local_path = NULL;
	GVolume *gvolume;
	GMount *mount;
	GFile *root;

	g_return_val_if_fail (volume != NULL, NULL);
	g_return_val_if_fail (BURNER_IS_VOLUME (volume), NULL);

	gvolume = burner_volume_get_gvolume (volume);
	if (!gvolume)
		return NULL;

	/* get the uri for the mount point */
	mount = g_volume_get_mount (gvolume);
	g_object_unref (gvolume);
	if (!mount)
		return NULL;

	root = g_mount_get_root (mount);
	g_object_unref (mount);

	if (!root) {
		g_set_error (error,
			     BURNER_MEDIA_ERROR,
			     BURNER_MEDIA_ERROR_GENERAL,
			     _("The disc mount point could not be retrieved"));
	}
	else {
		local_path = g_file_get_path (root);
		g_object_unref (root);
		BURNER_MEDIA_LOG ("Mount point is %s", local_path);
	}

	return local_path;
}

/**
 * burner_volume_umount:
 * @volume: #BurnerVolume
 * @wait: #gboolean
 * @error: #GError **
 *
 * Unmount @volume. If wait is set to TRUE, then block (in a GMainLoop) until
 * the operation finishes.
 *
 * Return value: a #gboolean. TRUE if the operation succeeded.
 *
 **/
gboolean
burner_volume_umount (BurnerVolume *volume,
		       gboolean wait,
		       GError **error)
{
	gboolean result;
	GVolume *gvolume;
	BurnerVolumePrivate *priv;

	if (!volume)
		return TRUE;

	g_return_val_if_fail (BURNER_IS_VOLUME (volume), FALSE);

	priv = BURNER_VOLUME_PRIVATE (volume);

	gvolume = burner_volume_get_gvolume (volume);
	if (!gvolume)
		return TRUE;

	if (g_cancellable_is_cancelled (priv->cancel)) {
		BURNER_MEDIA_LOG ("Resetting GCancellable object");
		g_cancellable_reset (priv->cancel);
	}

	result = burner_gio_operation_umount (gvolume,
					       priv->cancel,
					       wait,
					       error);
	g_object_unref (gvolume);

	return result;
}

/**
 * burner_volume_mount:
 * @volume: #BurnerVolume *
 * @parent_window: #GtkWindow *
 * @wait: #gboolean
 * @error: #GError **
 *
 * Mount @volume. If wait is set to TRUE, then block (in a GMainLoop) until
 * the operation finishes.
 * @parent_window is used if an authentification is needed. Then the authentification
 * dialog will be set modal.
 *
 * Return value: a #gboolean. TRUE if the operation succeeded.
 *
 **/
gboolean
burner_volume_mount (BurnerVolume *volume,
		      GtkWindow *parent_window,
		      gboolean wait,
		      GError **error)
{
	gboolean result;
	GVolume *gvolume;
	BurnerVolumePrivate *priv;

	if (!volume)
		return TRUE;

	g_return_val_if_fail (BURNER_IS_VOLUME (volume), FALSE);

	priv = BURNER_VOLUME_PRIVATE (volume);

	gvolume = burner_volume_get_gvolume (volume);
	if (!gvolume)
		return TRUE;

	if (g_cancellable_is_cancelled (priv->cancel)) {
		BURNER_MEDIA_LOG ("Resetting GCancellable object");
		g_cancellable_reset (priv->cancel);
	}

	result = burner_gio_operation_mount (gvolume,
					      parent_window,
					      priv->cancel,
					      wait,
					      error);
	g_object_unref (gvolume);

	return result;
}

/**
 * burner_volume_cancel_current_operation:
 * @volume: #BurnerVolume *
 *
 * Cancels all operations currently running for @volume
 *
 **/
void
burner_volume_cancel_current_operation (BurnerVolume *volume)
{
	BurnerVolumePrivate *priv;

	g_return_if_fail (volume != NULL);
	g_return_if_fail (BURNER_IS_VOLUME (volume));

	priv = BURNER_VOLUME_PRIVATE (volume);

	BURNER_MEDIA_LOG ("Cancelling volume operation");

	g_cancellable_cancel (priv->cancel);
}

/**
 * burner_volume_get_icon:
 * @volume: #BurnerVolume *
 *
 * Returns a GIcon pointer for the volume.
 *
 * Return value: a #GIcon*
 *
 **/
GIcon *
burner_volume_get_icon (BurnerVolume *volume)
{
	GVolume *gvolume;
	GMount *mount;
	GIcon *icon;

	if (!volume)
		return g_themed_icon_new_with_default_fallbacks ("drive-optical");

	g_return_val_if_fail (BURNER_IS_VOLUME (volume), NULL);

	if (burner_medium_get_status (BURNER_MEDIUM (volume)) == BURNER_MEDIUM_FILE)
		return g_themed_icon_new_with_default_fallbacks ("iso-image-new");

	gvolume = burner_volume_get_gvolume (volume);
	if (!gvolume)
		return g_themed_icon_new_with_default_fallbacks ("drive-optical");

	mount = g_volume_get_mount (gvolume);
	if (mount) {
		icon = g_mount_get_icon (mount);
		g_object_unref (mount);
	}
	else
		icon = g_volume_get_icon (gvolume);

	g_object_unref (gvolume);

	return icon;
}

/**
 * burner_volume_get_name:
 * @volume: #BurnerVolume *
 *
 * Returns a string that can be displayed to represent the volume²
 *
 * Return value: a #gchar *. Free when not needed anymore.
 *
 **/
gchar *
burner_volume_get_name (BurnerVolume *volume)
{
	BurnerMedia media;
	const gchar *type;
	GVolume *gvolume;
	gchar *name;

	g_return_val_if_fail (volume != NULL, NULL);
	g_return_val_if_fail (BURNER_IS_VOLUME (volume), NULL);

	media = burner_medium_get_status (BURNER_MEDIUM (volume));
	if (media & BURNER_MEDIUM_FILE) {
		/* Translators: This is a fake drive, a file, and means that
		 * when we're writing, we're writing to a file and create an
		 * image on the hard drive. */
		return g_strdup (_("Image File"));
	}

	if (media & BURNER_MEDIUM_HAS_AUDIO) {
		const gchar *audio_name;

		audio_name = burner_medium_get_CD_TEXT_title (BURNER_MEDIUM (volume));
		if (audio_name)
			return g_strdup (audio_name);
	}

	gvolume = burner_volume_get_gvolume (volume);
	if (!gvolume)
		goto last_chance;

	name = g_volume_get_name (gvolume);
	g_object_unref (gvolume);

	if (name)
		return name;

last_chance:

	type = burner_medium_get_type_string (BURNER_MEDIUM (volume));
	name = NULL;
	if (media & BURNER_MEDIUM_BLANK) {
		/* NOTE for translators: the first %s is the disc type and Blank is an adjective. */
		name = g_strdup_printf (_("Blank disc (%s)"), type);
	}
	else if (BURNER_MEDIUM_IS (media, BURNER_MEDIUM_HAS_AUDIO|BURNER_MEDIUM_HAS_DATA)) {
		/* NOTE for translators: the first %s is the disc type. */
		name = g_strdup_printf (_("Audio and data disc (%s)"), type);
	}
	else if (media & BURNER_MEDIUM_HAS_AUDIO) {
		/* NOTE for translators: the first %s is the disc type. */
		name = g_strdup_printf (_("Audio disc (%s)"), type);
	}
	else if (media & BURNER_MEDIUM_HAS_DATA) {
		/* NOTE for translators: the first %s is the disc type. */
		name = g_strdup_printf (_("Data disc (%s)"), type);
	}
	else {
		name = g_strdup (type);
	}

	return name;
}

static void
burner_volume_init (BurnerVolume *object)
{
	BurnerVolumePrivate *priv;

	priv = BURNER_VOLUME_PRIVATE (object);
	priv->cancel = g_cancellable_new ();
}

static void
burner_volume_finalize (GObject *object)
{
	BurnerVolumePrivate *priv;

	priv = BURNER_VOLUME_PRIVATE (object);

	BURNER_MEDIA_LOG ("Finalizing Volume object");
	if (priv->cancel) {
		g_cancellable_cancel (priv->cancel);
		g_object_unref (priv->cancel);
		priv->cancel = NULL;
	}

	G_OBJECT_CLASS (burner_volume_parent_class)->finalize (object);
}

static void
burner_volume_class_init (BurnerVolumeClass *klass)
{
	GObjectClass* object_class = G_OBJECT_CLASS (klass);

	g_type_class_add_private (klass, sizeof (BurnerVolumePrivate));

	object_class->finalize = burner_volume_finalize;
}
