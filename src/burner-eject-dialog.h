/***************************************************************************
 *            
 *
 *  Copyright  2008  Philippe Rouquier <burner-app@wanadoo.fr>
 *  Copyright  2008  Luis Medinas <lmedinas@gmail.com>
 *
 *
 *  Burner is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  Burner is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Library General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to:
 * 	The Free Software Foundation, Inc.,
 * 	51 Franklin Street, Fifth Floor
 * 	Boston, MA  02110-1301, USA.
 *
 */

#ifndef BURNER_EJECT_DIALOG_H
#define BURNER_EJECT_DIALOG_H

#include <glib.h>
#include <glib-object.h>

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define BURNER_TYPE_EJECT_DIALOG         (burner_eject_dialog_get_type ())
#define BURNER_EJECT_DIALOG(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), BURNER_TYPE_EJECT_DIALOG, BurnerEjectDialog))
#define BURNER_EJECT_DIALOG_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k), BURNER_TYPE_EJECT_DIALOG, BurnerEjectDialogClass))
#define BURNER_IS_EJECT_DIALOG(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), BURNER_TYPE_EJECT_DIALOG))
#define BURNER_IS_EJECT_DIALOG_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), BURNER_TYPE_EJECT_DIALOG))
#define BURNER_EJECT_DIALOG_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), BURNER_TYPE_EJECT_DIALOG, BurnerEjectDialogClass))

typedef struct _BurnerEjectDialog BurnerEjectDialog;
typedef struct _BurnerEjectDialogClass BurnerEjectDialogClass;

struct _BurnerEjectDialog {
	GtkDialog parent;
};

struct _BurnerEjectDialogClass {
	GtkDialogClass parent_class;
};

GType burner_eject_dialog_get_type (void);
GtkWidget *burner_eject_dialog_new (void);

gboolean
burner_eject_dialog_cancel (BurnerEjectDialog *dialog);

G_END_DECLS

#endif /* BURNER_Eject_DIALOG_H */
