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

#ifndef BURNER_SUM_DIALOG_H
#define BURNER_SUM_DIALOG_H

#include <glib.h>
#include <glib-object.h>

#include <gtk/gtk.h>

#include <burner-tool-dialog.h>

G_BEGIN_DECLS

#define BURNER_TYPE_SUM_DIALOG         (burner_sum_dialog_get_type ())
#define BURNER_SUM_DIALOG(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), BURNER_TYPE_SUM_DIALOG, BurnerSumDialog))
#define BURNER_SUM_DIALOG_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k), BURNER_TYPE_SUM_DIALOG, BurnerSumDialogClass))
#define BURNER_IS_SUM_DIALOG(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), BURNER_TYPE_SUM_DIALOG))
#define BURNER_IS_SUM_DIALOG_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), BURNER_TYPE_SUM_DIALOG))
#define BURNER_SUM_DIALOG_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), BURNER_TYPE_SUM_DIALOG, BurnerSumDialogClass))

typedef struct _BurnerSumDialog BurnerSumDialog;
typedef struct _BurnerSumDialogClass BurnerSumDialogClass;

struct _BurnerSumDialog {
	BurnerToolDialog parent;
};

struct _BurnerSumDialogClass {
	BurnerToolDialogClass parent_class;
};

GType burner_sum_dialog_get_type (void);

BurnerSumDialog *burner_sum_dialog_new (void);

G_END_DECLS

#endif /* BURNER_SUM_DIALOG_H */
