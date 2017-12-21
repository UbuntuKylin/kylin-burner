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

#ifndef _BURNER_STATUS_DIALOG_H_
#define _BURNER_STATUS_DIALOG_H_

#include <glib-object.h>
#include <gtk/gtk.h>

#include "burner-enums.h"
#include "burner-session.h"

G_BEGIN_DECLS

#define BURNER_TYPE_STATUS_DIALOG             (burner_status_dialog_get_type ())
#define BURNER_STATUS_DIALOG(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), BURNER_TYPE_STATUS_DIALOG, BurnerStatusDialog))
#define BURNER_STATUS_DIALOG_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), BURNER_TYPE_STATUS_DIALOG, BurnerStatusDialogClass))
#define BURNER_IS_STATUS_DIALOG(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BURNER_TYPE_STATUS_DIALOG))
#define BURNER_IS_STATUS_DIALOG_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), BURNER_TYPE_STATUS_DIALOG))
#define BURNER_STATUS_DIALOG_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), BURNER_TYPE_STATUS_DIALOG, BurnerStatusDialogClass))

typedef struct _BurnerStatusDialogClass BurnerStatusDialogClass;
typedef struct _BurnerStatusDialog BurnerStatusDialog;

struct _BurnerStatusDialogClass
{
	GtkMessageDialogClass parent_class;
};

struct _BurnerStatusDialog
{
	GtkMessageDialog parent_instance;
};

GType burner_status_dialog_get_type (void) G_GNUC_CONST;

GtkWidget *
burner_status_dialog_new (BurnerBurnSession *session,
			   GtkWidget *parent);

G_END_DECLS

#endif /* _BURNER_STATUS_DIALOG_H_ */
