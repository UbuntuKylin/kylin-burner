/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/*
 * Libburner-burn
 * Copyright (C) Philippe Rouquier 2005-2009 <bonfire-app@wanadoo.fr>
 *
 * Libburner-burn is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * The Libburner-burn authors hereby grant permission for non-GPL compatible
 * GStreamer plugins to be used and distributed together with GStreamer
 * and Libburner-burn. This permission is above and beyond the permissions granted
 * by the GPL license by which Libburner-burn is covered. If you modify this code
 * you may extend this exception to your version of the code, but you are not
 * obligated to do so. If you do not wish to do so, delete this exception
 * statement from your version.
 * 
 * Libburner-burn is distributed in the hope that it will be useful,
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

#ifndef BURNER_TOOL_DIALOG_H
#define BURNER_TOOL_DIALOG_H

#include <glib.h>
#include <glib-object.h>

#include <gtk/gtk.h>

#include <burner-medium.h>
#include <burner-medium-monitor.h>

#include <burner-session.h>
#include <burner-burn.h>


G_BEGIN_DECLS

#define BURNER_TYPE_TOOL_DIALOG         (burner_tool_dialog_get_type ())
#define BURNER_TOOL_DIALOG(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), BURNER_TYPE_TOOL_DIALOG, BurnerToolDialog))
#define BURNER_TOOL_DIALOG_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k), BURNER_TYPE_TOOL_DIALOG, BurnerToolDialogClass))
#define BURNER_IS_TOOL_DIALOG(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), BURNER_TYPE_TOOL_DIALOG))
#define BURNER_IS_TOOL_DIALOG_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), BURNER_TYPE_TOOL_DIALOG))
#define BURNER_TOOL_DIALOG_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), BURNER_TYPE_TOOL_DIALOG, BurnerToolDialogClass))

typedef struct _BurnerToolDialog BurnerToolDialog;
typedef struct _BurnerToolDialogClass BurnerToolDialogClass;

struct _BurnerToolDialog {
	GtkDialog parent;
};

struct _BurnerToolDialogClass {
	GtkDialogClass parent_class;

	/* Virtual functions */
	gboolean	(*activate)		(BurnerToolDialog *dialog,
						 BurnerMedium *medium);
	gboolean	(*cancel)		(BurnerToolDialog *dialog);
	void		(*medium_changed)	(BurnerToolDialog *dialog,
						 BurnerMedium *medium);
};

GType burner_tool_dialog_get_type (void);

gboolean
burner_tool_dialog_cancel (BurnerToolDialog *dialog);

gboolean
burner_tool_dialog_set_medium (BurnerToolDialog *dialog,
				BurnerMedium *medium);

G_END_DECLS

#endif /* BURNER_TOOL_DIALOG_H */
