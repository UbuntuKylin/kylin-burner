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

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <string.h>

#include <glib.h>
#include <glib/gi18n.h>

#include <gtk/gtk.h>

#include "burner-eject-dialog.h"
#include "burner-drive-selection.h"
#include "burner-medium.h"
#include "burner-drive.h"
#include "burner-volume.h"
#include "burner-utils.h"
#include "burner-burn.h"
#include "burner-misc.h"
#include "burner-app.h"

typedef struct _BurnerEjectDialogPrivate BurnerEjectDialogPrivate;
struct _BurnerEjectDialogPrivate {
	GtkWidget *selector;
	GtkWidget *eject_button;
	gboolean cancelled;
};

#define BURNER_EJECT_DIALOG_PRIVATE(o)  (G_TYPE_INSTANCE_GET_PRIVATE ((o), BURNER_TYPE_EJECT_DIALOG, BurnerEjectDialogPrivate))

G_DEFINE_TYPE (BurnerEjectDialog, burner_eject_dialog, GTK_TYPE_DIALOG);

static void
burner_eject_dialog_activate (GtkDialog *dialog,
			       GtkResponseType answer)
{
	BurnerDrive *drive;
	GError *error = NULL;
	BurnerEjectDialogPrivate *priv;

	if (answer != GTK_RESPONSE_OK)
		return;

	priv = BURNER_EJECT_DIALOG_PRIVATE (dialog);

	gtk_widget_set_sensitive (GTK_WIDGET (priv->selector), FALSE);
	gtk_widget_set_sensitive (priv->eject_button, FALSE);

	/* In here we could also remove the lock held by any app (including 
	 * burner) through burner_drive_unlock. We'd need a warning
	 * dialog though which would identify why the lock is held and even
	 * better which application is holding the lock so the user does know
	 * if he can take the risk to remove the lock. */

	/* NOTE 2: we'd need also the ability to reset the drive through a SCSI
	 * command. The problem is burner may need to be privileged then as
	 * cdrecord/cdrdao seem to be. */
	drive = burner_drive_selection_get_active (BURNER_DRIVE_SELECTION (priv->selector));
	burner_drive_unlock (drive);

	/*if (burner_volume_is_mounted (BURNER_VOLUME (medium))
	&& !burner_volume_umount (BURNER_VOLUME (medium), TRUE, &error)) {
		BURNER_BURN_LOG ("Error unlocking medium: %s", error?error->message:"Unknown error");
		return TRUE;
	}*/
	if (!burner_drive_eject (drive, TRUE, &error)) {
		gchar *string;
		gchar *display_name;

		if (!priv->cancelled || error) {
			display_name = burner_drive_get_display_name (drive);
			string = g_strdup_printf (_("The disc in \"%s\" cannot be ejected"), display_name);
			g_free (display_name);

			burner_app_alert (burner_app_get_default (),
			                   string,
			                   error?error->message:_("An unknown error occurred"),
			                   GTK_MESSAGE_ERROR);

			g_free (string);
		}

		g_clear_error (&error);
	}

	g_object_unref (drive);
}

gboolean
burner_eject_dialog_cancel (BurnerEjectDialog *dialog)
{
	BurnerEjectDialogPrivate *priv;
	BurnerDrive *drive;

	priv = BURNER_EJECT_DIALOG_PRIVATE (dialog);
	drive = burner_drive_selection_get_active (BURNER_DRIVE_SELECTION (priv->selector));

	if (drive) {
		priv->cancelled = TRUE;
		burner_drive_cancel_current_operation (drive);
		g_object_unref (drive);
	}

	return TRUE;
}

static void
burner_eject_dialog_cancel_cb (GtkWidget *button_cancel,
                                BurnerEjectDialog *dialog)
{
	burner_eject_dialog_cancel (dialog);
}

static void
burner_eject_dialog_class_init (BurnerEjectDialogClass *klass)
{
	GtkDialogClass *dialog_class = GTK_DIALOG_CLASS (klass);

	g_type_class_add_private (klass, sizeof (BurnerEjectDialogPrivate));

	dialog_class->response = burner_eject_dialog_activate;
}

static void
burner_eject_dialog_init (BurnerEjectDialog *obj)
{
	gchar *title_str;
	GtkWidget *box;
	GtkWidget *hbox;
	GtkWidget *label;
	GtkWidget *button;
	BurnerEjectDialogPrivate *priv;

	priv = BURNER_EJECT_DIALOG_PRIVATE (obj);
	priv->cancelled = FALSE;

	box = gtk_dialog_get_content_area (GTK_DIALOG (obj));

	priv->selector = burner_drive_selection_new ();
	gtk_widget_show (GTK_WIDGET (priv->selector));

	title_str = g_strdup (_("Select a disc"));

	label = gtk_label_new (title_str);
	gtk_label_set_use_markup (GTK_LABEL (label), TRUE);
	gtk_widget_show (label);

	hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 8);
	gtk_container_set_border_width (GTK_CONTAINER (hbox), 8);
	gtk_widget_show (hbox);
	gtk_box_pack_start (GTK_BOX (box), hbox, FALSE, TRUE, 0);

	gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
	gtk_box_pack_start (GTK_BOX (hbox), priv->selector, FALSE, TRUE, 0);
	g_free (title_str);

	burner_drive_selection_show_type (BURNER_DRIVE_SELECTION (priv->selector),
	                                   BURNER_DRIVE_TYPE_ALL_BUT_FILE);

	button = gtk_dialog_add_button (GTK_DIALOG (obj),
	                                GTK_STOCK_CANCEL,
	                                GTK_RESPONSE_CANCEL);
	g_signal_connect (button,
	                  "clicked",
	                  G_CALLBACK (burner_eject_dialog_cancel_cb),
	                  obj);

	button = burner_utils_make_button (_("_Eject"),
					    NULL,
					    "media-eject",
					    GTK_ICON_SIZE_BUTTON);
	gtk_dialog_add_action_widget (GTK_DIALOG (obj),
	                              button,
	                              GTK_RESPONSE_OK);
	gtk_widget_show (button);
	priv->eject_button = button;
}

GtkWidget *
burner_eject_dialog_new ()
{
	return g_object_new (BURNER_TYPE_EJECT_DIALOG,
			     "title", (_("Eject Disc")),
			     NULL);
}

