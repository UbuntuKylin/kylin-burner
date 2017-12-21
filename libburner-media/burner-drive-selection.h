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

#ifndef _BURNER_DRIVE_SELECTION_H_
#define _BURNER_DRIVE_SELECTION_H_

#include <glib-object.h>

#include <gtk/gtk.h>

#include <burner-medium-monitor.h>
#include <burner-drive.h>

G_BEGIN_DECLS

#define BURNER_TYPE_DRIVE_SELECTION             (burner_drive_selection_get_type ())
#define BURNER_DRIVE_SELECTION(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), BURNER_TYPE_DRIVE_SELECTION, BurnerDriveSelection))
#define BURNER_DRIVE_SELECTION_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), BURNER_TYPE_DRIVE_SELECTION, BurnerDriveSelectionClass))
#define BURNER_IS_DRIVE_SELECTION(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BURNER_TYPE_DRIVE_SELECTION))
#define BURNER_IS_DRIVE_SELECTION_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), BURNER_TYPE_DRIVE_SELECTION))
#define BURNER_DRIVE_SELECTION_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), BURNER_TYPE_DRIVE_SELECTION, BurnerDriveSelectionClass))

typedef struct _BurnerDriveSelectionClass BurnerDriveSelectionClass;
typedef struct _BurnerDriveSelection BurnerDriveSelection;

struct _BurnerDriveSelectionClass
{
	GtkComboBoxClass parent_class;

	/* Signals */
	void		(* drive_changed)		(BurnerDriveSelection *selector,
							 BurnerDrive *drive);
};

struct _BurnerDriveSelection
{
	GtkComboBox parent_instance;
};

G_MODULE_EXPORT GType burner_drive_selection_get_type (void) G_GNUC_CONST;

GtkWidget* burner_drive_selection_new (void);

BurnerDrive *
burner_drive_selection_get_active (BurnerDriveSelection *selector);

gboolean
burner_drive_selection_set_active (BurnerDriveSelection *selector,
				    BurnerDrive *drive);

void
burner_drive_selection_show_type (BurnerDriveSelection *selector,
				   BurnerDriveType type);

G_END_DECLS

#endif /* _BURNER_DRIVE_SELECTION_H_ */
