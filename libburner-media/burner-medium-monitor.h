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

#ifndef _BURNER_MEDIUM_MONITOR_H_
#define _BURNER_MEDIUM_MONITOR_H_

#include <glib-object.h>

#include <burner-medium.h>
#include <burner-drive.h>

G_BEGIN_DECLS

#define BURNER_TYPE_MEDIUM_MONITOR             (burner_medium_monitor_get_type ())
#define BURNER_MEDIUM_MONITOR(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), BURNER_TYPE_MEDIUM_MONITOR, BurnerMediumMonitor))
#define BURNER_MEDIUM_MONITOR_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), BURNER_TYPE_MEDIUM_MONITOR, BurnerMediumMonitorClass))
#define BURNER_IS_MEDIUM_MONITOR(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BURNER_TYPE_MEDIUM_MONITOR))
#define BURNER_IS_MEDIUM_MONITOR_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), BURNER_TYPE_MEDIUM_MONITOR))
#define BURNER_MEDIUM_MONITOR_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), BURNER_TYPE_MEDIUM_MONITOR, BurnerMediumMonitorClass))

typedef struct _BurnerMediumMonitorClass BurnerMediumMonitorClass;
typedef struct _BurnerMediumMonitor BurnerMediumMonitor;


struct _BurnerMediumMonitorClass
{
	GObjectClass parent_class;

	/* Signals */
	void		(* drive_added)		(BurnerMediumMonitor *monitor,
						 BurnerDrive *drive);

	void		(* drive_removed)	(BurnerMediumMonitor *monitor,
						 BurnerDrive *drive);

	void		(* medium_added)	(BurnerMediumMonitor *monitor,
						 BurnerMedium *medium);

	void		(* medium_removed)	(BurnerMediumMonitor *monitor,
						 BurnerMedium *medium);
};

struct _BurnerMediumMonitor
{
	GObject parent_instance;
};

GType burner_medium_monitor_get_type (void) G_GNUC_CONST;

BurnerMediumMonitor *
burner_medium_monitor_get_default (void);

typedef enum {
	BURNER_MEDIA_TYPE_NONE				= 0,
	BURNER_MEDIA_TYPE_FILE				= 1,
	BURNER_MEDIA_TYPE_DATA				= 1 << 1,
	BURNER_MEDIA_TYPE_AUDIO			= 1 << 2,
	BURNER_MEDIA_TYPE_WRITABLE			= 1 << 3,
	BURNER_MEDIA_TYPE_REWRITABLE			= 1 << 4,
	BURNER_MEDIA_TYPE_ANY_IN_BURNER		= 1 << 5,

	/* If combined with other flags it will filter.
	 * if alone all CDs are returned.
	 * It can't be combined with FILE type. */
	BURNER_MEDIA_TYPE_CD					= 1 << 6,

	BURNER_MEDIA_TYPE_ALL_BUT_FILE			= 0xFE,
	BURNER_MEDIA_TYPE_ALL				= 0xFF
} BurnerMediaType;

typedef enum {
	BURNER_DRIVE_TYPE_NONE				= 0,
	BURNER_DRIVE_TYPE_FILE				= 1,
	BURNER_DRIVE_TYPE_WRITER			= 1 << 1,
	BURNER_DRIVE_TYPE_READER			= 1 << 2,
	BURNER_DRIVE_TYPE_ALL_BUT_FILE			= 0xFE,
	BURNER_DRIVE_TYPE_ALL				= 0xFF
} BurnerDriveType;

GSList *
burner_medium_monitor_get_media (BurnerMediumMonitor *monitor,
				  BurnerMediaType type);

GSList *
burner_medium_monitor_get_drives (BurnerMediumMonitor *monitor,
				   BurnerDriveType type);

BurnerDrive *
burner_medium_monitor_get_drive (BurnerMediumMonitor *monitor,
				  const gchar *device);

gboolean
burner_medium_monitor_is_probing (BurnerMediumMonitor *monitor);

G_END_DECLS

#endif /* _BURNER_MEDIUM_MONITOR_H_ */
