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

#ifndef _BURNER_MEDIUM_SELECTION_H_
#define _BURNER_MEDIUM_SELECTION_H_

#include <glib-object.h>

#include <gtk/gtk.h>

#include <burner-medium-monitor.h>
#include <burner-medium.h>
#include <burner-drive.h>

G_BEGIN_DECLS

#define BURNER_TYPE_MEDIUM_SELECTION             (burner_medium_selection_get_type ())
#define BURNER_MEDIUM_SELECTION(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), BURNER_TYPE_MEDIUM_SELECTION, BurnerMediumSelection))
#define BURNER_MEDIUM_SELECTION_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), BURNER_TYPE_MEDIUM_SELECTION, BurnerMediumSelectionClass))
#define BURNER_IS_MEDIUM_SELECTION(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BURNER_TYPE_MEDIUM_SELECTION))
#define BURNER_IS_MEDIUM_SELECTION_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), BURNER_TYPE_MEDIUM_SELECTION))
#define BURNER_MEDIUM_SELECTION_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), BURNER_TYPE_MEDIUM_SELECTION, BurnerMediumSelectionClass))

typedef struct _BurnerMediumSelectionClass BurnerMediumSelectionClass;
typedef struct _BurnerMediumSelection BurnerMediumSelection;

struct _BurnerMediumSelectionClass
{
	GtkComboBoxClass parent_class;

	/* Signals */
	void		(* medium_changed)		(BurnerMediumSelection *selection,
							 BurnerMedium *medium);

	/* virtual function */
	gchar *		(*format_medium_string)		(BurnerMediumSelection *selection,
							 BurnerMedium *medium);
};

/**
 * BurnerMediumSelection:
 *
 * Rename to: MediumSelection
 */

struct _BurnerMediumSelection
{
	GtkComboBox parent_instance;
};

G_MODULE_EXPORT GType burner_medium_selection_get_type (void) G_GNUC_CONST;

GtkWidget* burner_medium_selection_new (void);

BurnerMedium *
burner_medium_selection_get_active (BurnerMediumSelection *selector);

gboolean
burner_medium_selection_set_active (BurnerMediumSelection *selector,
				     BurnerMedium *medium);

void
burner_medium_selection_show_media_type (BurnerMediumSelection *selector,
					  BurnerMediaType type);

G_END_DECLS

#endif /* _BURNER_MEDIUM_SELECTION_H_ */
