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

#include <glib-object.h>
#include <gio/gio.h>

#ifndef _BURN_DRIVE_H_
#define _BURN_DRIVE_H_

#include <burner-medium.h>

G_BEGIN_DECLS

typedef enum {
	BURNER_DRIVE_CAPS_NONE			= 0,
	BURNER_DRIVE_CAPS_CDR			= 1,
	BURNER_DRIVE_CAPS_CDRW			= 1 << 1,
	BURNER_DRIVE_CAPS_DVDR			= 1 << 2,
	BURNER_DRIVE_CAPS_DVDRW		= 1 << 3,
	BURNER_DRIVE_CAPS_DVDR_PLUS		= 1 << 4,
	BURNER_DRIVE_CAPS_DVDRW_PLUS		= 1 << 5,
	BURNER_DRIVE_CAPS_DVDR_PLUS_DL		= 1 << 6,
	BURNER_DRIVE_CAPS_DVDRW_PLUS_DL	= 1 << 7,
	BURNER_DRIVE_CAPS_DVDRAM		= 1 << 10,
	BURNER_DRIVE_CAPS_BDR			= 1 << 8,
	BURNER_DRIVE_CAPS_BDRW			= 1 << 9
} BurnerDriveCaps;

#define BURNER_TYPE_DRIVE             (burner_drive_get_type ())
#define BURNER_DRIVE(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), BURNER_TYPE_DRIVE, BurnerDrive))
#define BURNER_DRIVE_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), BURNER_TYPE_DRIVE, BurnerDriveClass))
#define BURNER_IS_DRIVE(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BURNER_TYPE_DRIVE))
#define BURNER_IS_DRIVE_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), BURNER_TYPE_DRIVE))
#define BURNER_DRIVE_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), BURNER_TYPE_DRIVE, BurnerDriveClass))

typedef struct _BurnerDriveClass BurnerDriveClass;

struct _BurnerDriveClass
{
	GObjectClass parent_class;

	/* Signals */
	void		(* medium_added)	(BurnerDrive *drive,
						 BurnerMedium *medium);

	void		(* medium_removed)	(BurnerDrive *drive,
						 BurnerMedium *medium);
};

struct _BurnerDrive
{
	GObject parent_instance;
};

GType burner_drive_get_type (void) G_GNUC_CONST;

void
burner_drive_reprobe (BurnerDrive *drive);

BurnerMedium *
burner_drive_get_medium (BurnerDrive *drive);

GDrive *
burner_drive_get_gdrive (BurnerDrive *drive);

const gchar *
burner_drive_get_udi (BurnerDrive *drive);

gboolean
burner_drive_is_fake (BurnerDrive *drive);

gchar *
burner_drive_get_display_name (BurnerDrive *drive);

const gchar *
burner_drive_get_device (BurnerDrive *drive);

const gchar *
burner_drive_get_block_device (BurnerDrive *drive);

gchar *
burner_drive_get_bus_target_lun_string (BurnerDrive *drive);

BurnerDriveCaps
burner_drive_get_caps (BurnerDrive *drive);

gboolean
burner_drive_can_write_media (BurnerDrive *drive,
                               BurnerMedia media);

gboolean
burner_drive_can_write (BurnerDrive *drive);

gboolean
burner_drive_can_eject (BurnerDrive *drive);

gboolean
burner_drive_eject (BurnerDrive *drive,
		     gboolean wait,
		     GError **error);

void
burner_drive_cancel_current_operation (BurnerDrive *drive);

gboolean
burner_drive_is_door_open (BurnerDrive *drive);

gboolean
burner_drive_can_use_exclusively (BurnerDrive *drive);

gboolean
burner_drive_lock (BurnerDrive *drive,
		    const gchar *reason,
		    gchar **reason_for_failure);
gboolean
burner_drive_unlock (BurnerDrive *drive);

gboolean
burner_drive_is_locked (BurnerDrive *drive,
                         gchar **reason);

G_END_DECLS

#endif /* _BURN_DRIVE_H_ */
