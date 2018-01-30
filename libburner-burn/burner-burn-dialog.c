/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/*
 * Libburner-burn
 * Copyright (C) Philippe Rouquier 2005-2009 <bonfire-app@wanadoo.fr>
 * Copyright (C) Canonical 2010
 * Copyright (C) 2017,Tianjin KYLIN Information Technology Co., Ltd.
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

#include <stdlib.h>
#include <string.h>
#include <errno.h>

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <glib.h>
#include <glib/gi18n-lib.h>
#include <glib/gstdio.h>

#include <gdk/gdk.h>
#include <gdk/gdkx.h>

#include <gtk/gtk.h>

#include <canberra-gtk.h>
#include <libnotify/notify.h>

#ifdef HAVE_UNITY
#include <unity.h>
#endif

#include "burner-burn-dialog.h"

#ifdef HAVE_APP_INDICATOR
#include "burner-app-indicator.h"
#endif
#include "burner-session-cfg.h"
#include "burner-session-helper.h"

#include "burn-basics.h"
#include "burn-debug.h"
#include "burner-progress.h"
#include "burner-cover.h"
#include "burner-track-type-private.h"

#include "burner-tags.h"
#include "burner-session.h"
#include "burner-track-image.h"

#include "burner-medium.h"
#include "burner-drive.h"

#include "burner-misc.h"
#include "burner-pk.h"
#include "burner-customize-title.h"

G_DEFINE_TYPE (BurnerBurnDialog, burner_burn_dialog, GTK_TYPE_DIALOG);

#ifdef HAVE_APP_INDICATOR
static void
burner_burn_dialog_indicator_cancel_cb (BurnerAppIndicator *indicator,
					 BurnerBurnDialog *dialog);

static void
burner_burn_dialog_indicator_show_dialog_cb (BurnerAppIndicator *indicator,
					      gboolean show,
					      GtkWidget *dialog);
#endif
static void
burner_burn_dialog_cancel_clicked_cb (GtkWidget *button,
				       BurnerBurnDialog *dialog);

typedef struct BurnerBurnDialogPrivate BurnerBurnDialogPrivate;
struct BurnerBurnDialogPrivate {
	BurnerBurn *burn;
	BurnerBurnSession *session;

	/* This is to remember some settins after ejection */
	BurnerTrackType input;
	BurnerMedia media;

	GtkWidget *progress;
	GtkWidget *header;
	GtkWidget *cancel;
	GtkWidget *image;

#ifdef HAVE_APP_INDICATOR
	BurnerAppIndicator *indicator;
#endif

	/* for our final statistics */
	GTimer *total_time;
	gint64 total_size;
	GSList *rates;

	GMainLoop *loop;
	gint wait_ready_state_id;
	GCancellable *cancel_plugin;

	gchar *initial_title;
	gchar *initial_icon;

	guint num_copies;

	guint is_writing:1;
	guint is_creating_image:1;

#ifdef HAVE_UNITY
    UnityLauncherEntry *launcher_entry;
#endif
};

#define BURNER_BURN_DIALOG_PRIVATE(o)  (G_TYPE_INSTANCE_GET_PRIVATE ((o), BURNER_TYPE_BURN_DIALOG, BurnerBurnDialogPrivate))

#define TIMEOUT	10000

static void
burner_burn_dialog_update_media (BurnerBurnDialog *dialog)
{
	BurnerBurnDialogPrivate *priv;
	BurnerMedia media;

	priv = BURNER_BURN_DIALOG_PRIVATE (dialog);

	if (burner_burn_session_is_dest_file (priv->session))
		media = BURNER_MEDIUM_FILE;
	else if (!burner_track_type_get_has_medium (&priv->input))
		media = burner_burn_session_get_dest_media (priv->session);
	else {
		BurnerMedium *medium;

		medium = burner_burn_session_get_src_medium (priv->session);
		if (!medium)
			media = burner_burn_session_get_dest_media (BURNER_BURN_SESSION (priv->session));
		else
			media = burner_medium_get_status (medium);
	}

	priv->media = media;
}

static void
burner_burn_dialog_notify_daemon_close (NotifyNotification *notification,
                                         BurnerBurnDialog *dialog)
{
	g_object_unref (notification);
}

static gboolean
burner_burn_dialog_notify_daemon (BurnerBurnDialog *dialog,
                                   const char *message)
{
        NotifyNotification *notification;
	GError *error = NULL;
        gboolean result;

	if (!notify_is_initted ()) {
		notify_init (_("Burner notification"));
	}

        notification = notify_notification_new (message,
                                                NULL,
                                                GTK_STOCK_CDROM);

	if (!notification)
                return FALSE;

	g_signal_connect (notification,
                          "closed",
                          G_CALLBACK (burner_burn_dialog_notify_daemon_close),
                          dialog);

	notify_notification_set_timeout (notification, 10000);
	notify_notification_set_urgency (notification, NOTIFY_URGENCY_NORMAL);
	notify_notification_set_hint_string (notification, "desktop-entry",
                                             "burner");

	result = notify_notification_show (notification, &error);
	if (error) {
		g_warning ("Error showing notification");
		g_error_free (error);
	}

	return result;
}

static void
on_exit_cb (GtkWidget *button, GtkWidget *dialog)
{
	if (dialog)
		gtk_dialog_response (dialog, GTK_RESPONSE_CANCEL);
}

static GtkWidget *
burner_burn_dialog_create_message (BurnerBurnDialog *dialog,
                                    GtkMessageType type,
                                    GtkButtonsType buttons,
                                    const gchar *main_message)
{
	const gchar *icon_name;
	GtkWidget *message;

	icon_name = gtk_window_get_icon_name (GTK_WINDOW (dialog));
	message = gtk_message_dialog_new (GTK_WINDOW (dialog),
	                                  GTK_DIALOG_DESTROY_WITH_PARENT|
	                                  GTK_DIALOG_MODAL,
	                                  type,
	                                  buttons,
	                                  "%s", main_message);

	burner_message_title(message);
	gtk_window_set_icon_name (GTK_WINDOW (message), icon_name);
	return message;
}

static gchar *
burner_burn_dialog_create_dialog_title_for_action (BurnerBurnDialog *dialog,
                                                    const gchar *action,
                                                    gint percent)
{
	gchar *tmp;
	BurnerBurnDialogPrivate *priv;

	priv = BURNER_BURN_DIALOG_PRIVATE (dialog);

	if (priv->initial_title)
		action = priv->initial_title;

	if (percent >= 0 && percent <= 100) {
		/* Translators: This string is used in the title bar %s is the action currently performed */
		tmp = g_strdup_printf (_("%s (%i%% Done)"),
				       action,
				       percent);
	}
	else 
		/* Translators: This string is used in the title bar %s is the action currently performed */
		tmp = g_strdup (action);

	return tmp;
}

static void
burner_burn_dialog_update_title (BurnerBurnDialog *dialog,
                                  BurnerTrackType *input)
{
	gchar *title = NULL;
	BurnerBurnDialogPrivate *priv;

	priv = BURNER_BURN_DIALOG_PRIVATE (dialog);

	if (priv->media == BURNER_MEDIUM_FILE)
		title = burner_burn_dialog_create_dialog_title_for_action (dialog,
		                                                            _("Creating Image"),
		                                                            -1);
	else if (priv->media & BURNER_MEDIUM_DVD) {
		if (!burner_track_type_get_has_medium (input))
			title = burner_burn_dialog_create_dialog_title_for_action (dialog,
										    _("Burning DVD"),
										    -1);
		else
			title = burner_burn_dialog_create_dialog_title_for_action (dialog,
										    _("Copying DVD"),
										    -1);
	}
	else if (priv->media & BURNER_MEDIUM_CD) {
		if (!burner_track_type_get_has_medium (input))
			title = burner_burn_dialog_create_dialog_title_for_action (dialog,
										    _("Burning CD"),
										    -1);
		else
			title = burner_burn_dialog_create_dialog_title_for_action (dialog,
										    _("Copying CD"),
										    -1);
	}
	else {
		if (!burner_track_type_get_has_medium (input))
			title = burner_burn_dialog_create_dialog_title_for_action (dialog,
										    _("Burning Disc"),
										    -1);
		else
			title = burner_burn_dialog_create_dialog_title_for_action (dialog,
										    _("Copying Disc"),
										    -1);
	}

	if (title) {
//		gtk_window_set_title (GTK_WINDOW (dialog), title);
		gtk_label_set_text(dlabel, title);
		g_free (title);
	}
}

/**
 * NOTE: if input is DISC then media is the media input
 */

static void
burner_burn_dialog_update_info (BurnerBurnDialog *dialog,
				 BurnerTrackType *input,
                                 gboolean dummy)
{
	gchar *header = NULL;
	BurnerBurnDialogPrivate *priv;

	priv = BURNER_BURN_DIALOG_PRIVATE (dialog);

	if (priv->media == BURNER_MEDIUM_FILE) {
		/* we are creating an image to the hard drive */
		gtk_image_set_from_icon_name (GTK_IMAGE (priv->image),
					      "iso-image-new",
					      GTK_ICON_SIZE_DIALOG);

//		header = g_strdup_printf ("<big><b>%s</b></big>", _("Creating image"));
		header = g_strdup_printf (_("Creating image"));
	}
	else if (priv->media & BURNER_MEDIUM_DVD) {
		if (burner_track_type_get_has_stream (input)
		&&  BURNER_STREAM_FORMAT_HAS_VIDEO (input->subtype.stream_format)) {
			if (dummy)
//				header = g_strdup_printf ("<big><b>%s</b></big>", _("Simulation of video DVD burning"));
				header = g_strdup_printf (_("Simulation of video DVD burning"));
			else
//				header = g_strdup_printf ("<big><b>%s</b></big>", _("Burning video DVD"));
				header = g_strdup_printf (_("Burning video DVD"));

			gtk_image_set_from_icon_name (GTK_IMAGE (priv->image),
						      "media-optical-video-new",
						      GTK_ICON_SIZE_DIALOG);
		}
		else if (burner_track_type_get_has_data (input)) {
			if (dummy)
//				header = g_strdup_printf ("<big><b>%s</b></big>", _("Simulation of data DVD burning"));
				header = g_strdup_printf (_("Simulation of data DVD burning"));
			else
//				header = g_strdup_printf ("<big><b>%s</b></big>", _("Burning data DVD"));
				header = g_strdup_printf (_("Burning data DVD"));

			gtk_image_set_from_icon_name (GTK_IMAGE (priv->image),
						      "media-optical-data-new1",
						      GTK_ICON_SIZE_DIALOG);
		}
		else if (burner_track_type_get_has_image (input)) {
			if (dummy)
//				header = g_strdup_printf ("<big><b>%s</b></big>", _("Simulation of image to DVD burning"));
				header = g_strdup_printf (_("Simulation of data DVD copying"));
			else
//				header = g_strdup_printf ("<big><b>%s</b></big>", _("Burning image to DVD"));
				header = g_strdup_printf (_("Burning image to DVD"));
			gtk_image_set_from_icon_name (GTK_IMAGE (priv->image),
						      "media-optical",
						      GTK_ICON_SIZE_DIALOG);
		}
		else if (burner_track_type_get_has_medium (input)) {
			if (dummy)
//				header = g_strdup_printf ("<big><b>%s</b></big>", _("Simulation of data DVD copying"));
				header = g_strdup_printf (_("Simulation of data DVD copying"));
			else
//				header = g_strdup_printf ("<big><b>%s</b></big>", _("Copying data DVD"));
				header = g_strdup_printf (_("Copying data DVD"));

			gtk_image_set_from_icon_name (GTK_IMAGE (priv->image),
						      "media-optical-copy",
						      GTK_ICON_SIZE_DIALOG);
		}
	}
	else if (priv->media & BURNER_MEDIUM_CD) {
		if (burner_track_type_get_has_stream (input)
		&&  BURNER_STREAM_FORMAT_HAS_VIDEO (input->subtype.stream_format)) {
			if (dummy)
//				header = g_strdup_printf ("<big><b>%s</b></big>", _("Simulation of (S)VCD burning"));
				header = g_strdup_printf (_("Simulation of (S)VCD burning"));
			else
//				header = g_strdup_printf ("<big><b>%s</b></big>", _("Burning (S)VCD"));
				header = g_strdup_printf (_("Burning (S)VCD"));

			gtk_image_set_from_icon_name (GTK_IMAGE (priv->image),
						      "drive-removable-media",
						      GTK_ICON_SIZE_DIALOG);
		}
		else if (burner_track_type_get_has_stream (input)) {
			if (dummy)
//				header = g_strdup_printf ("<big><b>%s</b></big>", _("Simulation of audio CD burning"));
				header = g_strdup_printf (_("Simulation of audio CD burning"));
			else
//				header = g_strdup_printf ("<big><b>%s</b></big>", _("Burning audio CD"));
				header = g_strdup_printf (_("Burning audio CD"));

			gtk_image_set_from_icon_name (GTK_IMAGE (priv->image),
						      "media-optical-audio-new",
						      GTK_ICON_SIZE_DIALOG);
		}
		else if (burner_track_type_get_has_data (input)) {
			if (dummy)
//				header = g_strdup_printf ("<big><b>%s</b></big>", _("Simulation of data CD burning"));
				header = g_strdup_printf (_("Simulation of data CD burning"));
			else
//				header = g_strdup_printf ("<big><b>%s</b></big>", _("Burning data CD"));
				header = g_strdup_printf (_("Burning data CD"));

			gtk_image_set_from_icon_name (GTK_IMAGE (priv->image),
						      "media-optical-data-new",
						      GTK_ICON_SIZE_DIALOG);
		}
		else if (burner_track_type_get_has_medium (input)) {
			if (dummy)
//				header = g_strdup_printf ("<big><b>%s</b></big>", _("Simulation of CD copying"));
				header = g_strdup_printf (_("Simulation of CD copying"));
			else
//				header = g_strdup_printf ("<big><b>%s</b></big>", _("Copying CD"));
				header = g_strdup_printf (_("Copying CD"));

			gtk_image_set_from_icon_name (GTK_IMAGE (priv->image),
						      "media-optical-copy",
						      GTK_ICON_SIZE_DIALOG);
		}
		else if (burner_track_type_get_has_image (input)) {
			if (dummy)
//				header = g_strdup_printf ("<big><b>%s</b></big>", _("Simulation of image to CD burning"));
				header = g_strdup_printf (_("Simulation of image to CD burning"));
			else
//				header = g_strdup_printf ("<big><b>%s</b></big>", _("Burning image to CD"));
				header = g_strdup_printf (_("Burning image to CD"));		
			gtk_image_set_from_icon_name (GTK_IMAGE (priv->image),
						      "media-optical",
						      GTK_ICON_SIZE_DIALOG);
		}
	}
	else {
		if (burner_track_type_get_has_stream (input)
		&&  BURNER_STREAM_FORMAT_HAS_VIDEO (input->subtype.stream_format)) {
			if (dummy)
//				header = g_strdup_printf ("<big><b>%s</b></big>", _("Simulation of video disc burning"));
				header = g_strdup_printf (_("Simulation of video disc burning"));
			else
//				header = g_strdup_printf ("<big><b>%s</b></big>", _("Burning video disc"));
				header = g_strdup_printf (_("Burning video disc"));


			gtk_image_set_from_icon_name (GTK_IMAGE (priv->image),
						      "drive-removable-media",
						      GTK_ICON_SIZE_DIALOG);
		}
		else if (burner_track_type_get_has_stream (input)) {
			if (dummy)
//				header = g_strdup_printf ("<big><b>%s</b></big>", _("Simulation of audio CD burning"));
				header = g_strdup_printf (_("Simulation of audio CD burning"));

			else
//				header = g_strdup_printf ("<big><b>%s</b></big>", _("Burning audio CD"));
				header = g_strdup_printf (_("Burning audio CD"));

			gtk_image_set_from_icon_name (GTK_IMAGE (priv->image),
						      "drive-removable-media",
						      GTK_ICON_SIZE_DIALOG);
		}
		else if (burner_track_type_get_has_data (input)) {
			if (dummy)
//				header = g_strdup_printf ("<big><b>%s</b></big>", _("Simulation of data disc burning"));
				header = g_strdup_printf (_("Simulation of data disc burning"));
			else
//				header = g_strdup_printf ("<big><b>%s</b></big>", _("Burning data disc"));
				header = g_strdup_printf (_("Burning data disc"));

			gtk_image_set_from_icon_name (GTK_IMAGE (priv->image),
						      "drive-removable-media",
						      GTK_ICON_SIZE_DIALOG);
		}
		else if (burner_track_type_get_has_medium (input)) {
			if (dummy)
//				header = g_strdup_printf ("<big><b>%s</b></big>", _("Simulation of disc copying"));
				header = g_strdup_printf (_("Simulation of disc copying"));
			else
//				header = g_strdup_printf ("<big><b>%s</b></big>", _("Copying disc"));
				header = g_strdup_printf (_("Copying disc"));

			gtk_image_set_from_icon_name (GTK_IMAGE (priv->image),
						      "drive-removable-media",
						      GTK_ICON_SIZE_DIALOG);
		}
		else if (burner_track_type_get_has_image (input)) {
			if (dummy)
//				header = g_strdup_printf ("<big><b>%s</b></big>", _("Simulation of image to disc burning"));
				header = g_strdup_printf (_("Simulation of image to disc burning"));
			else
//				header = g_strdup_printf ("<big><b>%s</b></big>", _("Burning image to disc"));
				header = g_strdup_printf (_("Burning image to disc"));

			gtk_image_set_from_icon_name (GTK_IMAGE (priv->image),
						      "drive-removable-media",
						      GTK_ICON_SIZE_DIALOG);
		}
	}

	gtk_label_set_text (GTK_LABEL (priv->header), header);
	gtk_label_set_use_markup (GTK_LABEL (priv->header), TRUE);
	g_free (header);
}

static void
burner_burn_dialog_wait_for_insertion_cb (BurnerDrive *drive,
					   BurnerMedium *medium,
					   GtkDialog *message)
{
	/* we might have a dialog waiting for the 
	 * insertion of a disc if so close it */
	gtk_dialog_response (GTK_DIALOG (message), GTK_RESPONSE_OK);
}

static gint
burner_burn_dialog_wait_for_insertion (BurnerBurnDialog *dialog,
					BurnerDrive *drive,
					const gchar *main_message,
					const gchar *secondary_message,
                                        gboolean sound_clue)
{
	gint result;
	gulong added_id;
	GtkWidget *message;
	gboolean hide = FALSE;
	BurnerBurnDialogPrivate *priv;

	priv = BURNER_BURN_DIALOG_PRIVATE (dialog);

	if (!gtk_widget_get_visible (GTK_WIDGET (dialog))) {
		gtk_widget_show (GTK_WIDGET (dialog));
		hide = TRUE;
	}

	g_timer_stop (priv->total_time);

	if (secondary_message) {
		message = burner_burn_dialog_create_message (dialog,
		                                              GTK_MESSAGE_WARNING,
		                                              GTK_BUTTONS_CANCEL,
		                                              main_message);

		if (secondary_message)
			gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (message),
								  "%s", secondary_message);
	}
	else {
		gchar *string;

		message = burner_burn_dialog_create_message (dialog,
							      GTK_MESSAGE_WARNING,
							      GTK_BUTTONS_CANCEL,
							      NULL);

		string = g_strdup_printf ("<b><big>%s</big></b>", main_message);
		gtk_message_dialog_set_markup (GTK_MESSAGE_DIALOG (message), string);
		g_free (string);
	}

	/* connect to signals to be warned when media is inserted */
	added_id = g_signal_connect_after (drive,
					   "medium-added",
					   G_CALLBACK (burner_burn_dialog_wait_for_insertion_cb),
					   message);

	if (sound_clue) {
		gtk_widget_show (GTK_WIDGET (message));
		ca_gtk_play_for_widget (GTK_WIDGET (message), 0,
		                        CA_PROP_EVENT_ID, "complete-media-burn",
		                        CA_PROP_EVENT_DESCRIPTION, main_message,
		                        NULL);
	}

	burner_dialog_button_image(gtk_dialog_get_action_area(GTK_DIALOG (message)));
	result = gtk_dialog_run (GTK_DIALOG (message));

	g_signal_handler_disconnect (drive, added_id);
	gtk_widget_destroy (message);

	if (hide)
		gtk_widget_hide (GTK_WIDGET (dialog));

	g_timer_continue (priv->total_time);

	return result;
}

static gchar *
burner_burn_dialog_get_media_type_string (BurnerBurn *burn,
					   BurnerMedia type,
					   gboolean insert)
{
	gchar *message = NULL;

	if (type & BURNER_MEDIUM_HAS_DATA) {
		if (!insert) {
			if (type & BURNER_MEDIUM_REWRITABLE)
				message = g_strdup (_("Please replace the disc with a rewritable disc holding data."));
			else
				message = g_strdup (_("Please replace the disc with a disc holding data."));
		}
		else {
			if (type & BURNER_MEDIUM_REWRITABLE)
				message = g_strdup (_("Please insert a rewritable disc holding data."));
			else
				message = g_strdup (_("Please insert a disc holding data."));
		}
	}
	else if (type & BURNER_MEDIUM_WRITABLE) {
		gint64 isosize = 0;
	
		burner_burn_status (burn,
				     NULL,
				     &isosize,
				     NULL,
				     NULL);

		if ((type & BURNER_MEDIUM_CD) && !(type & BURNER_MEDIUM_DVD)) {
			if (!insert) {
				if (isosize > 0)
					message = g_strdup_printf (_("Please replace the disc with a writable CD with at least %i MiB of free space."), 
								   (int) (isosize / 1048576));
				else
					message = g_strdup (_("Please replace the disc with a writable CD."));
			}
			else {
				if (isosize > 0)
					message = g_strdup_printf (_("Please insert a writable CD with at least %i MiB of free space."), 
								   (int) (isosize / 1048576));
				else
					message = g_strdup (_("Please insert a writable CD."));
			}
		}
		else if (!(type & BURNER_MEDIUM_CD) && (type & BURNER_MEDIUM_DVD)) {
			if (!insert) {
				if (isosize > 0)
					message = g_strdup_printf (_("Please replace the disc with a writable DVD with at least %i MiB of free space."), 
								   (int) (isosize / 1048576));
				else
					message = g_strdup (_("Please replace the disc with a writable DVD."));
			}
			else {
				if (isosize > 0)
					message = g_strdup_printf (_("Please insert a writable DVD with at least %i MiB of free space."), 
								   (int) (isosize / 1048576));
				else
					message = g_strdup (_("Please insert a writable DVD."));
			}
		}
		else if (!insert) {
			if (isosize)
				message = g_strdup_printf (_("Please replace the disc with a writable CD or DVD with at least %i MiB of free space."), 
							   (int) (isosize / 1048576));
			else
				message = g_strdup (_("Please replace the disc with a writable CD or DVD."));
		}
		else {
			if (isosize)
				message = g_strdup_printf (_("Please insert a writable CD or DVD with at least %i MiB of free space."), 
							   (int) (isosize / 1048576));
			else
				message = g_strdup (_("Please insert a writable CD or DVD."));
		}
	}

	return message;
}

static BurnerBurnResult
burner_burn_dialog_insert_disc_cb (BurnerBurn *burn,
				    BurnerDrive *drive,
				    BurnerBurnError error,
				    BurnerMedia type,
				    BurnerBurnDialog *dialog)
{
	gint result;
	gchar *drive_name;
	BurnerBurnDialogPrivate *priv;
	gchar *main_message = NULL, *secondary_message = NULL;

	priv = BURNER_BURN_DIALOG_PRIVATE (dialog);

	if (drive)
		drive_name = burner_drive_get_display_name (drive);
	else
		drive_name = NULL;

	if (error == BURNER_BURN_WARNING_INSERT_AFTER_COPY) {
		secondary_message = g_strdup (_("An image of the disc has been created on your hard drive."
						"\nBurning will begin as soon as a writable disc is inserted."));
		main_message = burner_burn_dialog_get_media_type_string (burn, type, FALSE);
	}
	else if (error == BURNER_BURN_WARNING_CHECKSUM) {
		secondary_message = g_strdup (_("A data integrity test will begin as soon as the disc is inserted."));
		main_message = g_strdup (_("Please re-insert the disc in the CD/DVD burner."));
	}
	else if (error == BURNER_BURN_ERROR_DRIVE_BUSY) {
		/* Translators: %s is the name of a drive */
		main_message = g_strdup_printf (_("\"%s\" is busy."), drive_name);
		secondary_message = g_strdup_printf ("%s.", _("Make sure another application is not using it"));
	} 
	else if (error == BURNER_BURN_ERROR_MEDIUM_NONE) {
		secondary_message = g_strdup_printf (_("There is no disc in \"%s\"."), drive_name);
		main_message = burner_burn_dialog_get_media_type_string (burn, type, TRUE);
	}
	else if (error == BURNER_BURN_ERROR_MEDIUM_INVALID) {
		secondary_message = g_strdup_printf (_("The disc in \"%s\" is not supported."), drive_name);
		main_message = burner_burn_dialog_get_media_type_string (burn, type, TRUE);
	}
	else if (error == BURNER_BURN_ERROR_MEDIUM_NOT_REWRITABLE) {
		secondary_message = g_strdup_printf (_("The disc in \"%s\" is not rewritable."), drive_name);
		main_message = burner_burn_dialog_get_media_type_string (burn, type, FALSE);
	}
	else if (error == BURNER_BURN_ERROR_MEDIUM_NO_DATA) {
		secondary_message = g_strdup_printf (_("The disc in \"%s\" is empty."), drive_name);
		main_message = burner_burn_dialog_get_media_type_string (burn, type, FALSE);
	}
	else if (error == BURNER_BURN_ERROR_MEDIUM_NOT_WRITABLE) {
		secondary_message = g_strdup_printf (_("The disc in \"%s\" is not writable."), drive_name);
		main_message = burner_burn_dialog_get_media_type_string (burn, type, FALSE);
	}
	else if (error == BURNER_BURN_ERROR_MEDIUM_SPACE) {
		secondary_message = g_strdup_printf (_("Not enough space available on the disc in \"%s\"."), drive_name);
		main_message = burner_burn_dialog_get_media_type_string (burn, type, FALSE);
	}
	else if (error == BURNER_BURN_ERROR_NONE) {
		main_message = burner_burn_dialog_get_media_type_string (burn, type, TRUE);
		secondary_message = NULL;
	}
	else if (error == BURNER_BURN_ERROR_MEDIUM_NEED_RELOADING) {
		secondary_message = g_strdup_printf (_("The disc in \"%s\" needs to be reloaded."), drive_name);
		main_message = g_strdup (_("Please eject the disc and reload it."));
	}

	g_free (drive_name);

	result = burner_burn_dialog_wait_for_insertion (dialog, drive, main_message, secondary_message, FALSE);
	g_free (main_message);
	g_free (secondary_message);

	if (result != GTK_RESPONSE_OK)
		return BURNER_BURN_CANCEL;

	burner_burn_dialog_update_media (dialog);
	burner_burn_dialog_update_title (dialog, &priv->input);
	burner_burn_dialog_update_info (dialog,
	                                 &priv->input,
	                                 (burner_burn_session_get_flags (priv->session) & BURNER_BURN_FLAG_DUMMY) != 0);

	return BURNER_BURN_OK;
}

static BurnerBurnResult
burner_burn_dialog_image_error (BurnerBurn *burn,
				 GError *error,
				 gboolean is_temporary,
				 BurnerBurnDialog *dialog)
{
	gint result;
	gchar *path;
	gchar *string;
	GtkWidget *message;
	gboolean hide = FALSE;
	BurnerBurnDialogPrivate *priv;

	priv = BURNER_BURN_DIALOG_PRIVATE (dialog);

	if (!gtk_widget_get_visible (GTK_WIDGET (dialog))) {
		gtk_widget_show (GTK_WIDGET (dialog));
		hide = TRUE;
	}

	g_timer_stop (priv->total_time);

	string = g_strdup_printf ("%s. %s",
				  is_temporary?
				  _("A file could not be created at the location specified for temporary files"):
				  _("The image could not be created at the specified location"),
				  _("Do you want to specify another location for this session or retry with the current location?"));

	message = burner_burn_dialog_create_message (dialog,
	                                              GTK_MESSAGE_ERROR,
	                                              GTK_BUTTONS_NONE,
	                                              string);
	g_free (string);

	if (error && error->code == BURNER_BURN_ERROR_DISK_SPACE)
		gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (message),
							 "%s.\n%s.",
							  error->message,
							  _("You may want to free some space on the disc and retry"));
	else
		gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (message),
							 "%s.",
							  error->message);

	gtk_dialog_add_buttons (GTK_DIALOG (message),
				_("_Keep Current Location"), GTK_RESPONSE_OK,
				GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
				_("_Change Location"), GTK_RESPONSE_ACCEPT,
				NULL);

	burner_dialog_button_image(gtk_dialog_get_action_area(GTK_DIALOG (message)));
	result = gtk_dialog_run (GTK_DIALOG (message));
	gtk_widget_destroy (message);

	if (hide)
		gtk_widget_hide (GTK_WIDGET (dialog));

	if (result == GTK_RESPONSE_OK) {
		g_timer_continue (priv->total_time);
		return BURNER_BURN_OK;
	}

	if (result != GTK_RESPONSE_ACCEPT) {
		g_timer_continue (priv->total_time);
		return BURNER_BURN_CANCEL;
	}

	/* Show a GtkFileChooserDialog */
	if (!is_temporary) {
		gtk_file_chooser_set_do_overwrite_confirmation (GTK_FILE_CHOOSER (message), TRUE);
		message = gtk_file_chooser_dialog_new (_("Location for Image File"),
						       GTK_WINDOW (dialog),
						       GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER,
						       GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
						       GTK_STOCK_SAVE, GTK_RESPONSE_OK,
						       NULL);
	}
	else
		message = gtk_file_chooser_dialog_new (_("Location for Temporary Files"),
						       GTK_WINDOW (dialog),
						       GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER,
						       GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
						       GTK_STOCK_SAVE, GTK_RESPONSE_OK,
						       NULL);

	gtk_file_chooser_set_local_only (GTK_FILE_CHOOSER (message), TRUE);
	gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER (message), g_get_home_dir ());

	burner_dialog_button_image(gtk_dialog_get_action_area(GTK_DIALOG (message)));
	result = gtk_dialog_run (GTK_DIALOG (message));
	if (result != GTK_RESPONSE_OK) {
		gtk_widget_destroy (message);
		g_timer_continue (priv->total_time);
		return BURNER_BURN_CANCEL;
	}

	path = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (message));
	gtk_widget_destroy (message);

	if (!is_temporary) {
		BurnerImageFormat format;
		gchar *image = NULL;
		gchar *toc = NULL;

		format = burner_burn_session_get_output_format (priv->session);
		burner_burn_session_get_output (priv->session,
						 &image,
						 &toc);

		if (toc) {
			gchar *name;

			name = g_path_get_basename (toc);
			g_free (toc);

			toc = g_build_filename (path, name, NULL);
			BURNER_BURN_LOG ("New toc location %s", toc);
		}

		if (image) {
			gchar *name;

			name = g_path_get_basename (image);
			g_free (image);

			image = g_build_filename (path, name, NULL);
			BURNER_BURN_LOG ("New image location %s", toc);
		}

		burner_burn_session_set_image_output_full (priv->session,
							    format,
							    image,
							    toc);
	}
	else
		burner_burn_session_set_tmpdir (priv->session, path);

	g_free (path);

	g_timer_continue (priv->total_time);
	return BURNER_BURN_OK;
}

static BurnerBurnResult
burner_burn_dialog_loss_warnings_cb (BurnerBurnDialog *dialog, 
				      const gchar *main_message,
				      const gchar *secondary_message,
				      const gchar *button_text,
				      const gchar *button_icon,
                                      const gchar *second_button_text,
                                      const gchar *second_button_icon)
{
	gint result;
	GtkWidget *button;
	GtkWidget *message;
	gboolean hide = FALSE;
	BurnerBurnDialogPrivate *priv;

	priv = BURNER_BURN_DIALOG_PRIVATE (dialog);

	if (!gtk_widget_get_visible (GTK_WIDGET (dialog))) {
		gtk_widget_show (GTK_WIDGET (dialog));
		hide = TRUE;
	}

	g_timer_stop (priv->total_time);

	message = burner_burn_dialog_create_message (dialog,
	                                              GTK_MESSAGE_WARNING,
	                                              GTK_BUTTONS_NONE,
	                                              main_message);

	gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (message),
						 "%s", secondary_message);

	if (second_button_text) {
		button = gtk_dialog_add_button (GTK_DIALOG (message),
						second_button_text,
						GTK_RESPONSE_YES);

		if (second_button_icon)
			gtk_button_set_image (GTK_BUTTON (button),
					      gtk_image_new_from_icon_name (second_button_icon,
									    GTK_ICON_SIZE_BUTTON));
	}

	button = gtk_dialog_add_button (GTK_DIALOG (message),
					_("_Replace Disc"),
					GTK_RESPONSE_ACCEPT);
	gtk_button_set_image (GTK_BUTTON (button),
			      gtk_image_new_from_stock (GTK_STOCK_REFRESH,
							GTK_ICON_SIZE_BUTTON));

	gtk_dialog_add_button (GTK_DIALOG (message),
			       GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL);

	button = gtk_dialog_add_button (GTK_DIALOG (message),
					button_text,
					GTK_RESPONSE_OK);
	gtk_button_set_image (GTK_BUTTON (button),
			      gtk_image_new_from_icon_name (button_icon,
							    GTK_ICON_SIZE_BUTTON));

	burner_dialog_button_image(gtk_dialog_get_action_area(GTK_DIALOG (message)));
	result = gtk_dialog_run (GTK_DIALOG (message));
	gtk_widget_destroy (message);

	if (hide)
		gtk_widget_hide (GTK_WIDGET (dialog));

	g_timer_continue (priv->total_time);

	if (result == GTK_RESPONSE_YES)
		return BURNER_BURN_RETRY;

	if (result == GTK_RESPONSE_ACCEPT)
		return BURNER_BURN_NEED_RELOAD;

	if (result != GTK_RESPONSE_OK)
		return BURNER_BURN_CANCEL;

	return BURNER_BURN_OK;
}

static BurnerBurnResult
burner_burn_dialog_data_loss_cb (BurnerBurn *burn,
				  BurnerBurnDialog *dialog)
{
	return burner_burn_dialog_loss_warnings_cb (dialog,
						     _("Do you really want to blank the current disc?"),
						     _("The disc in the drive holds data."),
	                                             /* Translators: Blank is a verb here */
						     _("_Blank Disc"),
						     "media-optical-blank",
	                                             NULL,
	                                             NULL);
}

static BurnerBurnResult
burner_burn_dialog_previous_session_loss_cb (BurnerBurn *burn,
					      BurnerBurnDialog *dialog)
{
	gchar *secondary;
	BurnerBurnResult result;

	secondary = g_strdup_printf ("%s\n%s",
				     _("If you import them you will be able to see and use them once the current selection of files is burned."),
	                             _("If you don't, they will be invisible (though still readable)."));
				     
	result = burner_burn_dialog_loss_warnings_cb (dialog,
						       _("There are files already burned on this disc. Would you like to import them?"),
						       secondary,
						       _("_Import"), NULL,
	                                               _("Only _Append"), NULL);
	g_free (secondary);
	return result;
}

static BurnerBurnResult
burner_burn_dialog_audio_to_appendable_cb (BurnerBurn *burn,
					    BurnerBurnDialog *dialog)
{
	gchar *secondary;
	BurnerBurnResult result;

	secondary = g_strdup_printf ("%s\n%s",
				     _("CD-RW audio discs may not play correctly in older CD players and CD-Text won't be written."),
				     _("Do you want to continue anyway?"));

	result = burner_burn_dialog_loss_warnings_cb (dialog,
						       _("Appending audio tracks to a CD is not advised."),
						       secondary,
						       _("_Continue"),
						       "media-optical-burn",
	                                               NULL,
	                                               NULL);
	g_free (secondary);
	return result;
}

static BurnerBurnResult
burner_burn_dialog_rewritable_cb (BurnerBurn *burn,
				   BurnerBurnDialog *dialog)
{
	gchar *secondary;
	BurnerBurnResult result;

	secondary = g_strdup_printf ("%s\n%s",
				     _("CD-RW audio discs may not play correctly in older CD players."),
				     _("Do you want to continue anyway?"));

	result = burner_burn_dialog_loss_warnings_cb (dialog,
						       _("Recording audio tracks on a rewritable disc is not advised."),
						       secondary,
						       _("_Continue"),
						       "media-optical-burn",
	                                               NULL,
	                                               NULL);
	g_free (secondary);
	return result;
}

static void
burner_burn_dialog_wait_for_ejection_cb (BurnerDrive *drive,
                                          BurnerMedium *medium,
                                          GtkDialog *message)
{
	/* we might have a dialog waiting for the 
	 * insertion of a disc if so close it */
	gtk_dialog_response (GTK_DIALOG (message), GTK_RESPONSE_OK);
}

static BurnerBurnResult
burner_burn_dialog_eject_failure_cb (BurnerBurn *burn,
                                      BurnerDrive *drive,
                                      BurnerBurnDialog *dialog)
{
	gint result;
	gchar *name;
	gchar *string;
	gint removal_id;
	GtkWidget *message;
	gboolean hide = FALSE;
	BurnerBurnDialogPrivate *priv;

	priv = BURNER_BURN_DIALOG_PRIVATE (dialog);
	
	if (!gtk_widget_get_visible (GTK_WIDGET (dialog))) {
		gtk_widget_show (GTK_WIDGET (dialog));
		hide = TRUE;
	}

	g_timer_stop (priv->total_time);

	name = burner_drive_get_display_name (drive);
	/* Translators: %s is the name of a drive */
	string = g_strdup_printf (_("Please eject the disc from \"%s\" manually."), name);
	g_free (name);
	message = burner_burn_dialog_create_message (dialog,
	                                              GTK_MESSAGE_WARNING,
	                                              GTK_BUTTONS_NONE,
	                                              string);
	g_free (string);

	gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (message),
	                                          _("The disc could not be ejected though it needs to be removed for the current operation to continue."));

	gtk_dialog_add_button (GTK_DIALOG (message),
			       GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL);

	/* connect to signals to be warned when media is removed */
	removal_id = g_signal_connect_after (drive,
	                                     "medium-removed",
	                                     G_CALLBACK (burner_burn_dialog_wait_for_ejection_cb),
	                                     message);

	burner_dialog_button_image(gtk_dialog_get_action_area(GTK_DIALOG (message)));
	result = gtk_dialog_run (GTK_DIALOG (message));

	g_signal_handler_disconnect (drive, removal_id);
	gtk_widget_destroy (message);

	if (hide)
		gtk_widget_hide (GTK_WIDGET (dialog));

	g_timer_continue (priv->total_time);

	if (result == GTK_RESPONSE_ACCEPT)
		return BURNER_BURN_OK;

	return BURNER_BURN_CANCEL;
}

static BurnerBurnResult
burner_burn_dialog_continue_question (BurnerBurnDialog *dialog,
                                       const gchar *primary_message,
                                       const gchar *secondary_message,
                                       const gchar *button_message)
{
	gint result;
	GtkWidget *button;
	GtkWidget *message;
	gboolean hide = FALSE;
	BurnerBurnDialogPrivate *priv;

	priv = BURNER_BURN_DIALOG_PRIVATE (dialog);

	if (!gtk_widget_get_visible (GTK_WIDGET (dialog))) {
		gtk_widget_show (GTK_WIDGET (dialog));
		hide = TRUE;
	}

	g_timer_stop (priv->total_time);

	message = burner_burn_dialog_create_message (dialog,
	                                              GTK_MESSAGE_WARNING,
	                                              GTK_BUTTONS_NONE,
	                                              primary_message);

	gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (message),
						  "%s",
	                                          secondary_message);

	gtk_dialog_add_button (GTK_DIALOG (message),
			       GTK_STOCK_CANCEL,
	                       GTK_RESPONSE_CANCEL);

	button = gtk_dialog_add_button (GTK_DIALOG (message),
					button_message,
					GTK_RESPONSE_OK);
	gtk_button_set_image (GTK_BUTTON (button),
			      gtk_image_new_from_stock (GTK_STOCK_OK,
							GTK_ICON_SIZE_BUTTON));
	
	burner_dialog_button_image(gtk_dialog_get_action_area(GTK_DIALOG (message)));
	result = gtk_dialog_run (GTK_DIALOG (message));
	gtk_widget_destroy (message);

	if (hide)
		gtk_widget_hide (GTK_WIDGET (dialog));

	g_timer_continue (priv->total_time);

	if (result != GTK_RESPONSE_OK)
		return BURNER_BURN_CANCEL;

	return BURNER_BURN_OK;	
}

static BurnerBurnResult
burner_burn_dialog_blank_failure_cb (BurnerBurn *burn,
                                      BurnerBurnDialog *dialog)
{
	return burner_burn_dialog_continue_question (dialog,
	                                              _("Do you want to replace the disc and continue?"),
	                                              _("The currently inserted disc could not be blanked."),
	                                              _("_Replace Disc"));
}

static BurnerBurnResult
burner_burn_dialog_disable_joliet_cb (BurnerBurn *burn,
				       BurnerBurnDialog *dialog)
{
	return burner_burn_dialog_continue_question (dialog,
	                                              _("Do you want to continue with full Windows compatibility disabled?"),
	                                              _("Some files don't have a suitable name for a fully Windows-compatible CD."),
	                                              _("C_ontinue"));
}

static void
burner_burn_dialog_update_title_writing_progress (BurnerBurnDialog *dialog,
						   BurnerTrackType *input,
						   BurnerMedia media,
						   guint percent)
{
	gchar *title = NULL;
	gchar *icon_name;
	guint remains;

	/* This is used only when actually writing to a disc */
	if (media == BURNER_MEDIUM_FILE)
		title = burner_burn_dialog_create_dialog_title_for_action (dialog,
		                                                            _("Creating Image"),
		                                                            percent);
	else if (media & BURNER_MEDIUM_DVD) {
		if (burner_track_type_get_has_medium (input))
			title = burner_burn_dialog_create_dialog_title_for_action (dialog,
			                                                            _("Copying DVD"),
			                                                            percent);
		else
			title = burner_burn_dialog_create_dialog_title_for_action (dialog,
			                                                            _("Burning DVD"),
			                                                            percent);
	}
	else if (media & BURNER_MEDIUM_CD) {
		if (burner_track_type_get_has_medium (input))
			title = burner_burn_dialog_create_dialog_title_for_action (dialog,
			                                                            _("Copying CD"),
			                                                            percent);
		else
			title = burner_burn_dialog_create_dialog_title_for_action (dialog,
			                                                            _("Burning CD"),
			                                                            percent);
	}
	else {
		if (burner_track_type_get_has_medium (input))
			title = burner_burn_dialog_create_dialog_title_for_action (dialog,
			                                                            _("Copying Disc"),
			                                                            percent);
		else
			title = burner_burn_dialog_create_dialog_title_for_action (dialog,
			                                                            _("Burning Disc"),
			                                                            percent);
	}

//	gtk_window_set_title (GTK_WINDOW (dialog), title);
	gtk_label_set_text(dlabel, title);
	g_free (title);

	/* also update the icon */
	remains = percent % 5;
	if (remains > 3)
		percent += 5 - remains;
	else
		percent -= remains;

	if (percent < 0 || percent > 100)
		return;

	icon_name = g_strdup_printf ("burner-disc-%02i", percent);
	gtk_window_set_icon_name (GTK_WINDOW (dialog), icon_name);
	g_free (icon_name);
}

static void
burner_burn_dialog_progress_changed_real (BurnerBurnDialog *dialog,
					   gint64 written,
					   gint64 isosize,
					   gint64 rate,
					   gdouble overall_progress,
					   gdouble task_progress,
					   glong remaining,
					   BurnerMedia media)
{
	gint mb_isosize = -1;
	gint mb_written = -1;
	BurnerBurnDialogPrivate *priv;

	priv = BURNER_BURN_DIALOG_PRIVATE (dialog);

	if (written >= 0)
		mb_written = (gint) (written / 1048576);
	
	if (isosize > 0)
		mb_isosize = (gint) (isosize / 1048576);

	if (task_progress >= 0.0 && priv->is_writing)
		burner_burn_dialog_update_title_writing_progress (dialog,
								   &priv->input,
								   media,
								   (guint) (task_progress * 100.0));

	burner_burn_progress_set_status (BURNER_BURN_PROGRESS (priv->progress),
					  media,
					  overall_progress,
					  task_progress,
					  remaining,
					  mb_isosize,
					  mb_written,
					  rate);

	if (rate > 0 && priv->is_writing)
		priv->rates = g_slist_prepend (priv->rates, GINT_TO_POINTER ((gint) rate));
}

static void
burner_burn_dialog_progress_changed_cb (BurnerBurn *burn, 
					 gdouble overall_progress,
					 gdouble task_progress,
					 glong remaining,
					 BurnerBurnDialog *dialog)
{
	BurnerMedia media = BURNER_MEDIUM_NONE;
	BurnerBurnDialogPrivate *priv;
	goffset isosize = -1;
	goffset written = -1;
	guint64 rate = -1;

	priv = BURNER_BURN_DIALOG_PRIVATE (dialog);

	burner_burn_status (priv->burn,
			     &media,
			     &isosize,
			     &written,
			     &rate);

#ifdef HAVE_APP_INDICATOR
	burner_app_indicator_set_progress (BURNER_APPINDICATOR (priv->indicator),
					    task_progress,
					    remaining);
#endif

	burner_burn_dialog_progress_changed_real (dialog,
						   written,
						   isosize,
						   rate,
						   overall_progress,
						   task_progress,
						   remaining,
						   media);
	if ((priv->is_writing || priv->is_creating_image) && isosize > 0)
		priv->total_size = isosize;

#ifdef HAVE_UNITY
    unity_launcher_entry_set_progress(priv->launcher_entry, task_progress);
    unity_launcher_entry_set_progress_visible(priv->launcher_entry, TRUE);
#endif
}

static void
burner_burn_dialog_action_changed_real (BurnerBurnDialog *dialog,
					 BurnerBurnAction action,
					 const gchar *string)
{
	BurnerBurnDialogPrivate *priv;

	priv = BURNER_BURN_DIALOG_PRIVATE (dialog);

	burner_burn_progress_set_action (BURNER_BURN_PROGRESS (priv->progress),
					  action,
					  string);

#ifdef HAVE_APP_INDICATOR
	burner_app_indicator_set_action (BURNER_APPINDICATOR (priv->indicator),
					  action,
					  string);
#endif
}

static void
burner_burn_dialog_action_changed_cb (BurnerBurn *burn, 
				       BurnerBurnAction action,
				       BurnerBurnDialog *dialog)
{
	BurnerBurnDialogPrivate *priv;
	gchar *string = NULL;
	gboolean is_writing;

	priv = BURNER_BURN_DIALOG_PRIVATE (dialog);

	is_writing = (action == BURNER_BURN_ACTION_RECORDING ||
		      action == BURNER_BURN_ACTION_DRIVE_COPY ||
		      action == BURNER_BURN_ACTION_RECORDING_CD_TEXT ||
		      action == BURNER_BURN_ACTION_LEADIN ||
		      action == BURNER_BURN_ACTION_LEADOUT ||
		      action == BURNER_BURN_ACTION_FIXATING);

	if (action == BURNER_BURN_ACTION_START_RECORDING
	|| (priv->is_writing && !is_writing)) {
		BurnerMedia media = BURNER_MEDIUM_NONE;

		burner_burn_status (burn, &media, NULL, NULL, NULL);
	}

	priv->is_creating_image = (action == BURNER_BURN_ACTION_CREATING_IMAGE);
	priv->is_writing = is_writing;

	burner_burn_get_action_string (priv->burn, action, &string);
	burner_burn_dialog_action_changed_real (dialog, action, string);
	g_free (string);
}

static gboolean
burner_burn_dialog_dummy_success_timeout (gpointer data)
{
	GtkDialog *dialog = data;
	gtk_dialog_response (dialog, GTK_RESPONSE_OK);
	return FALSE;
}

static BurnerBurnResult
burner_burn_dialog_dummy_success_cb (BurnerBurn *burn,
				      BurnerBurnDialog *dialog)
{
	BurnerBurnDialogPrivate *priv;
	GtkResponseType answer;
	GtkWidget *message;
	GtkWidget *button;
	gboolean hide;
	gint id;

	priv = BURNER_BURN_DIALOG_PRIVATE (dialog);

	if (!gtk_widget_get_mapped (GTK_WIDGET (dialog))) {
		gtk_widget_show (GTK_WIDGET (dialog));
		hide = TRUE;
	} else
		hide = FALSE;

	g_timer_stop (priv->total_time);

	message = burner_burn_dialog_create_message (dialog,
	                                              GTK_MESSAGE_INFO,
	                                              GTK_BUTTONS_CANCEL,
	                                              _("The simulation was successful."));

	gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (message),
						  _("Real disc burning will take place in 10 seconds."));

	button = gtk_dialog_add_button (GTK_DIALOG (message),
					_("Burn _Now"),
					GTK_RESPONSE_OK);
	gtk_button_set_image (GTK_BUTTON (button),
			      gtk_image_new_from_icon_name ("media-optical-burn",
							    GTK_ICON_SIZE_BUTTON));
	id = g_timeout_add_seconds (10,
				    burner_burn_dialog_dummy_success_timeout,
				    message);

	gtk_widget_show (GTK_WIDGET (dialog));
	gtk_window_set_urgency_hint (GTK_WINDOW (dialog), TRUE);

	gtk_widget_show (GTK_WIDGET (message));
	ca_gtk_play_for_widget (GTK_WIDGET (message), 0,
	                        CA_PROP_EVENT_ID, "complete-media-burn-test",
	                        CA_PROP_EVENT_DESCRIPTION, _("The simulation was successful."),
	                        NULL);

	burner_dialog_button_image(gtk_dialog_get_action_area(GTK_DIALOG (message)));
	answer = gtk_dialog_run (GTK_DIALOG (message));
	gtk_widget_destroy (message);

	gtk_window_set_urgency_hint (GTK_WINDOW (dialog), FALSE);

	if (hide)
		gtk_widget_hide (GTK_WIDGET (dialog));

	g_timer_continue (priv->total_time);

	if (answer == GTK_RESPONSE_OK) {
		if (priv->initial_icon)
			gtk_window_set_icon_name (GTK_WINDOW (dialog), priv->initial_icon);
		else
			gtk_window_set_icon_name (GTK_WINDOW (dialog), "burner-00.png");

		burner_burn_dialog_update_info (dialog,
		                                 &priv->input,
		                                 FALSE);
		burner_burn_dialog_update_title (dialog, &priv->input);

		if (id)
			g_source_remove (id);

		return BURNER_BURN_OK;
	}

	if (id)
		g_source_remove (id);

	return BURNER_BURN_CANCEL;
}

static void
burner_burn_dialog_activity_start (BurnerBurnDialog *dialog)
{
	GdkCursor *cursor;
	GdkWindow *window;
	BurnerBurnDialogPrivate *priv;

	priv = BURNER_BURN_DIALOG_PRIVATE (dialog);

	window = gtk_widget_get_window (GTK_WIDGET (dialog));
	if (window) {
		cursor = gdk_cursor_new (GDK_WATCH);
		gdk_window_set_cursor (window, cursor);
		g_object_unref (cursor);
	}

	gtk_button_set_use_stock (GTK_BUTTON (priv->cancel), TRUE);
	gtk_button_set_label (GTK_BUTTON (priv->cancel), GTK_STOCK_CANCEL);
	gtk_window_set_urgency_hint (GTK_WINDOW (dialog), FALSE);

	g_signal_connect (priv->cancel,
			  "clicked",
			  G_CALLBACK (burner_burn_dialog_cancel_clicked_cb),
			  dialog);

	/* Reset or set the speed info */
	g_object_set (priv->progress,
		      "show-info", TRUE,
		      NULL);
	burner_burn_progress_set_status (BURNER_BURN_PROGRESS (priv->progress),
					  FALSE,
					  0.0,
					  0.0,
					  -1,
					  -1,
					  -1,
					  -1);

#ifdef HAVE_APP_INDICATOR
	burner_app_indicator_set_progress (BURNER_APPINDICATOR (priv->indicator),
					    0.0,
					    -1);
#endif

#ifdef HAVE_UNITY
    unity_launcher_entry_set_progress_visible(priv->launcher_entry, TRUE);
#endif
}

static void
burner_burn_dialog_activity_stop (BurnerBurnDialog *dialog,
				   const gchar *message)
{
	gchar *markup;
	BurnerBurnDialogPrivate *priv;

	priv = BURNER_BURN_DIALOG_PRIVATE (dialog);

	gdk_window_set_cursor (gtk_widget_get_window (GTK_WIDGET (dialog)), NULL);

	markup = g_strdup_printf ("<b><big>%s</big></b>", message);
	gtk_label_set_text (GTK_LABEL (priv->header), markup);
	gtk_label_set_use_markup (GTK_LABEL (priv->header), TRUE);
	g_free (markup);

	gtk_button_set_use_stock (GTK_BUTTON (priv->cancel), TRUE);
	gtk_button_set_label (GTK_BUTTON (priv->cancel), GTK_STOCK_CLOSE);

	g_signal_handlers_disconnect_by_func (priv->cancel,
					      burner_burn_dialog_cancel_clicked_cb,
					      dialog);

	burner_burn_progress_set_status (BURNER_BURN_PROGRESS (priv->progress),
					  FALSE,
					  1.0,
					  1.0,
					  -1,
					  -1,
					  -1,
					  -1);

#ifdef HAVE_APP_INDICATOR
	burner_app_indicator_hide (priv->indicator);
#endif

#ifdef HAVE_UNITY
    unity_launcher_entry_set_progress_visible(priv->launcher_entry, FALSE);
#endif

	/* Restore title */
	if (priv->initial_title)
//		gtk_window_set_title (GTK_WINDOW (dialog), priv->initial_title);
		gtk_label_set_text(dlabel, priv->initial_title);
	else
		burner_burn_dialog_update_title (dialog, &priv->input);

	/* Restore icon */
	if (priv->initial_icon)
		gtk_window_set_icon_name (GTK_WINDOW (dialog), priv->initial_icon);

	gtk_widget_show (GTK_WIDGET (dialog));
	gtk_window_set_urgency_hint (GTK_WINDOW (dialog), TRUE);
}

static BurnerBurnResult
burner_burn_dialog_install_missing_cb (BurnerBurn *burn,
					BurnerPluginErrorType type,
					const gchar *detail,
					gpointer user_data)
{
	BurnerBurnDialogPrivate *priv = BURNER_BURN_DIALOG_PRIVATE (user_data);
	GCancellable *cancel;
	BurnerPK *package;
	GdkWindow *window;
	gboolean res;
	int xid = 0;

	gtk_widget_set_sensitive (GTK_WIDGET (user_data), FALSE);

	/* Get the xid */
	window = gtk_widget_get_window (user_data);
	xid = gdk_x11_window_get_xid (window);

	package = burner_pk_new ();
	cancel = g_cancellable_new ();
	priv->cancel_plugin = cancel;
	switch (type) {
		case BURNER_PLUGIN_ERROR_MISSING_APP:
			res = burner_pk_install_missing_app (package, detail, xid, cancel);
			break;

		case BURNER_PLUGIN_ERROR_MISSING_LIBRARY:
			res = burner_pk_install_missing_library (package, detail, xid, cancel);
			break;

		case BURNER_PLUGIN_ERROR_MISSING_GSTREAMER_PLUGIN:
			res = burner_pk_install_gstreamer_plugin (package, detail, xid, cancel);
			break;

		default:
			res = FALSE;
			break;
	}

	if (package) {
		g_object_unref (package);
		package = NULL;
	}

	gtk_widget_set_sensitive (GTK_WIDGET (user_data), TRUE);
	if (g_cancellable_is_cancelled (cancel)) {
		g_object_unref (cancel);
		return BURNER_BURN_CANCEL;
	}

	priv->cancel_plugin = NULL;
	g_object_unref (cancel);

	if (!res)
		return BURNER_BURN_ERR;

	return BURNER_BURN_RETRY;
}

static BurnerBurnResult
burner_burn_dialog_setup_session (BurnerBurnDialog *dialog,
				   GError **error)
{
	BurnerBurnDialogPrivate *priv;

	priv = BURNER_BURN_DIALOG_PRIVATE (dialog);

	burner_burn_session_start (priv->session);

	priv->burn = burner_burn_new ();
	g_signal_connect (priv->burn,
			  "install-missing",
			  G_CALLBACK (burner_burn_dialog_install_missing_cb),
			  dialog);
	g_signal_connect (priv->burn,
			  "insert-media",
			  G_CALLBACK (burner_burn_dialog_insert_disc_cb),
			  dialog);
	g_signal_connect (priv->burn,
			  "blank-failure",
			  G_CALLBACK (burner_burn_dialog_blank_failure_cb),
			  dialog);
	g_signal_connect (priv->burn,
			  "eject-failure",
			  G_CALLBACK (burner_burn_dialog_eject_failure_cb),
			  dialog);
	g_signal_connect (priv->burn,
			  "location-request",
			  G_CALLBACK (burner_burn_dialog_image_error),
			  dialog);
	g_signal_connect (priv->burn,
			  "warn-data-loss",
			  G_CALLBACK (burner_burn_dialog_data_loss_cb),
			  dialog);
	g_signal_connect (priv->burn,
			  "warn-previous-session-loss",
			  G_CALLBACK (burner_burn_dialog_previous_session_loss_cb),
			  dialog);
	g_signal_connect (priv->burn,
			  "warn-audio-to-appendable",
			  G_CALLBACK (burner_burn_dialog_audio_to_appendable_cb),
			  dialog);
	g_signal_connect (priv->burn,
			  "warn-rewritable",
			  G_CALLBACK (burner_burn_dialog_rewritable_cb),
			  dialog);
	g_signal_connect (priv->burn,
			  "disable-joliet",
			  G_CALLBACK (burner_burn_dialog_disable_joliet_cb),
			  dialog);
	g_signal_connect (priv->burn,
			  "progress-changed",
			  G_CALLBACK (burner_burn_dialog_progress_changed_cb),
			  dialog);
	g_signal_connect (priv->burn,
			  "action-changed",
			  G_CALLBACK (burner_burn_dialog_action_changed_cb),
			  dialog);
	g_signal_connect (priv->burn,
			  "dummy-success",
			  G_CALLBACK (burner_burn_dialog_dummy_success_cb),
			  dialog);

	burner_burn_progress_set_status (BURNER_BURN_PROGRESS (priv->progress),
					  FALSE,
					  0.0,
					  -1.0,
					  -1,
					  -1,
					  -1,
					  -1);

	burner_burn_progress_set_action (BURNER_BURN_PROGRESS (priv->progress),
					  BURNER_BURN_ACTION_NONE,
					  NULL);

#ifdef HAVE_APP_INDICATOR
	burner_app_indicator_set_action (BURNER_APPINDICATOR (priv->indicator),
					  BURNER_BURN_ACTION_NONE,
					  NULL);
#endif

	g_timer_continue (priv->total_time);

	return BURNER_BURN_OK;
}

static void
burner_burn_dialog_save_log (BurnerBurnDialog *dialog)
{
	GError *error;
	gchar *contents;
	gchar *path = NULL;
	GtkWidget *chooser;
	GtkResponseType answer;
	BurnerBurnDialogPrivate *priv;

	priv = BURNER_BURN_DIALOG_PRIVATE (dialog);

#ifdef HAVE_APP_INDICATOR
	burner_app_indicator_set_show_dialog (BURNER_APPINDICATOR (priv->indicator),
					       FALSE);
#endif

	chooser = gtk_file_chooser_dialog_new (_("Save Current Session"),
					       GTK_WINDOW (dialog),
					       GTK_FILE_CHOOSER_ACTION_SAVE,
					       GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
					       GTK_STOCK_SAVE, GTK_RESPONSE_OK,
					       NULL);
	gtk_window_set_icon_name (GTK_WINDOW (chooser), gtk_window_get_icon_name (GTK_WINDOW (dialog)));

	gtk_file_chooser_set_local_only (GTK_FILE_CHOOSER (chooser), TRUE);
	gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER (chooser),
					     g_get_home_dir ());
	gtk_file_chooser_set_current_name (GTK_FILE_CHOOSER (chooser),
					   "burner-session.log");
	gtk_file_chooser_set_do_overwrite_confirmation (GTK_FILE_CHOOSER (chooser), TRUE);

	gtk_widget_show (chooser);
	burner_dialog_button_image(gtk_dialog_get_action_area(GTK_DIALOG (chooser)));
	answer = gtk_dialog_run (GTK_DIALOG (chooser));
	if (answer != GTK_RESPONSE_OK) {
		gtk_widget_destroy (chooser);
		return;
	}

	path = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (chooser));
	gtk_widget_destroy (chooser);

	if (!path)
		return;

	if (path && *path == '\0') {
		g_free (path);
		return;
	}

	error = NULL;
	if (!g_file_get_contents (burner_burn_session_get_log_path (priv->session),
	                          &contents,
	                          NULL,
	                          &error)) {
		g_warning ("Error while saving log file: %s\n", error? error->message:"none");
		g_error_free (error);
		g_free (path);
		return;
	}
	
	g_file_set_contents (path, contents, -1, NULL);

	g_free (contents);
	g_free (path);
}

static void
burner_burn_dialog_notify_error (BurnerBurnDialog *dialog,
				  GError *error)
{
	gchar *secondary;
	GtkWidget *button;
	GtkWidget *message;
	GtkResponseType response;
	BurnerBurnDialogPrivate *priv;

	priv = BURNER_BURN_DIALOG_PRIVATE (dialog);

	/* Restore title */
	if (priv->initial_title)
//		gtk_window_set_title (GTK_WINDOW (dialog), priv->initial_title);
		gtk_label_set_text(dlabel, priv->initial_title);

	/* Restore icon */
	if (priv->initial_icon)
		gtk_window_set_icon_name (GTK_WINDOW (dialog), priv->initial_icon);

	if (error) {
		secondary =  g_strdup (error->message);
		g_error_free (error);
	}
	else
		secondary = g_strdup (_("An unknown error occurred."));

	if (!gtk_widget_get_visible (GTK_WIDGET (dialog)))
		gtk_widget_show (GTK_WIDGET (dialog));

	message = burner_burn_dialog_create_message (dialog,
	                                              GTK_MESSAGE_ERROR,
	                                              GTK_BUTTONS_NONE,
	                                              _("Error while burning."));

	gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (message),
						  "%s",
						  secondary);
	g_free (secondary);

	button = gtk_dialog_add_button (GTK_DIALOG (message),
					_("_Save Log"),
					GTK_RESPONSE_OK);
	gtk_button_set_image (GTK_BUTTON (button),
			      gtk_image_new_from_stock (GTK_STOCK_SAVE_AS,
							GTK_ICON_SIZE_BUTTON));

	gtk_dialog_add_button (GTK_DIALOG (message),
			       GTK_STOCK_CLOSE,
			       GTK_RESPONSE_CLOSE);

	burner_burn_dialog_notify_daemon (dialog, _("Error while burning."));

	burner_dialog_button_image(gtk_dialog_get_action_area(GTK_DIALOG (message)));
	response = gtk_dialog_run (GTK_DIALOG (message));
	while (response == GTK_RESPONSE_OK) {
		burner_burn_dialog_save_log (dialog);
		response = gtk_dialog_run (GTK_DIALOG (message));
	}

	gtk_widget_destroy (message);
}

static gchar *
burner_burn_dialog_get_success_message (BurnerBurnDialog *dialog)
{
	BurnerBurnDialogPrivate *priv;
	BurnerDrive *drive;

	priv = BURNER_BURN_DIALOG_PRIVATE (dialog);

	drive = burner_burn_session_get_burner (priv->session);

	if (burner_track_type_get_has_stream (&priv->input)) {
		if (!burner_drive_is_fake (drive)) {
			if (BURNER_STREAM_FORMAT_HAS_VIDEO (burner_track_type_get_stream_format (&priv->input))) {
				if (priv->media & BURNER_MEDIUM_DVD)
					return g_strdup (_("Video DVD successfully burned"));

				return g_strdup (_("(S)VCD successfully burned"));
			}
			else
				return g_strdup (_("Audio CD successfully burned"));
		}
		return g_strdup (_("Image successfully created"));
	}
	else if (burner_track_type_get_has_medium (&priv->input)) {
		if (!burner_drive_is_fake (drive)) {
			if (priv->media & BURNER_MEDIUM_DVD)
				return g_strdup (_("DVD successfully copied"));
			else
				return g_strdup (_("CD successfully copied"));
		}
		else {
			if (priv->media & BURNER_MEDIUM_DVD)
				return g_strdup (_("Image of DVD successfully created"));
			else
				return g_strdup (_("Image of CD successfully created"));
		}
	}
	else if (burner_track_type_get_has_image (&priv->input)) {
		if (!burner_drive_is_fake (drive)) {
			if (priv->media & BURNER_MEDIUM_DVD)
				return g_strdup (_("Image successfully burned to DVD"));
			else
				return g_strdup (_("Image successfully burned to CD"));
		}
	}
	else if (burner_track_type_get_has_data (&priv->input)) {
		if (!burner_drive_is_fake (drive)) {
			if (priv->media & BURNER_MEDIUM_DVD)
				return g_strdup (_("Data DVD successfully burned"));
			else
				return g_strdup (_("Data CD successfully burned"));
		}
		return g_strdup (_("Image successfully created"));
	}

	return NULL;
}

static void
burner_burn_dialog_update_session_info (BurnerBurnDialog *dialog)
{
	gint64 rate;
	gchar *primary = NULL;
	BurnerBurnDialogPrivate *priv;

	priv = BURNER_BURN_DIALOG_PRIVATE (dialog);

	primary = burner_burn_dialog_get_success_message (dialog);
	burner_burn_dialog_activity_stop (dialog, primary);
	g_free (primary);

	/* show total required time and average speed */
	rate = 0;
	if (priv->rates) {
		int num = 0;
		GSList *iter;

		for (iter = priv->rates; iter; iter = iter->next) {
			rate += GPOINTER_TO_INT (iter->data);
			num ++;
		}
		rate /= num;
	}

	burner_burn_progress_display_session_info (BURNER_BURN_PROGRESS (priv->progress),
						    g_timer_elapsed (priv->total_time, NULL),
						    rate,
						    priv->media,
						    priv->total_size);
}

static gboolean
burner_burn_dialog_notify_copy_finished (BurnerBurnDialog *dialog,
                                          GError *error)
{
	gulong added_id;
	BurnerDrive *drive;
	GtkWidget *message;
	gchar *main_message;
	GtkResponseType response;
	BurnerBurnDialogPrivate *priv;

	priv = BURNER_BURN_DIALOG_PRIVATE (dialog);

	burner_burn_dialog_update_session_info (dialog);

	if (!gtk_widget_get_visible (GTK_WIDGET (dialog)))
		gtk_widget_show (GTK_WIDGET (dialog));

	main_message = g_strdup_printf (_("Copy #%i has been burned successfully."), priv->num_copies ++);
	message = burner_burn_dialog_create_message (dialog,
	                                              GTK_MESSAGE_INFO,
	                                              GTK_BUTTONS_CANCEL,
	                                              main_message);

	gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (message),
						  "%s",
						  _("Another copy will start as soon as you insert a new writable disc. If you do not want to burn another copy, press \"Cancel\"."));

	/* connect to signals to be warned when media is inserted */
	drive = burner_burn_session_get_burner (priv->session);
	added_id = g_signal_connect_after (drive,
					   "medium-added",
					   G_CALLBACK (burner_burn_dialog_wait_for_insertion_cb),
					   message);

	gtk_widget_show (GTK_WIDGET (message));
	ca_gtk_play_for_widget (GTK_WIDGET (message), 0,
	                        CA_PROP_EVENT_ID, "complete-media-burn",
	                        CA_PROP_EVENT_DESCRIPTION, main_message,
	                        NULL);

	burner_burn_dialog_notify_daemon (dialog, main_message);
	g_free (main_message);

	burner_dialog_button_image(gtk_dialog_get_action_area(GTK_DIALOG (message)));
	response = gtk_dialog_run (GTK_DIALOG (message));

	g_signal_handler_disconnect (drive, added_id);
	gtk_widget_destroy (message);

	return (response == GTK_RESPONSE_OK);
}

static gboolean
burner_burn_dialog_success_run (BurnerBurnDialog *dialog)
{
	gint answer;
	BurnerBurnDialogPrivate *priv;

	priv = BURNER_BURN_DIALOG_PRIVATE (dialog);

	burner_dialog_button_image(gtk_dialog_get_action_area(GTK_DIALOG (dialog)));
	answer = gtk_dialog_run (GTK_DIALOG (dialog));
	if (answer == GTK_RESPONSE_CLOSE) {
		GtkWidget *window;

		window = burner_session_edit_cover (priv->session, GTK_WIDGET (dialog));
		/* This strange hack is a way to workaround #568358.
		 * At one point we'll need to hide the dialog which means it
		 * will anwer with a GTK_RESPONSE_NONE */
		while (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_NONE)
			gtk_widget_show (GTK_WIDGET (dialog));

		gtk_widget_destroy (window);
		return FALSE;
	}

	return (answer == GTK_RESPONSE_OK);
}

static gboolean
burner_burn_dialog_notify_success (BurnerBurnDialog *dialog)
{
	gboolean res;
	gchar *primary = NULL;
	BurnerBurnDialogPrivate *priv;
	GtkWidget *create_cover = NULL;
	GtkWidget *make_another = NULL;

	priv = BURNER_BURN_DIALOG_PRIVATE (dialog);

	burner_burn_dialog_update_session_info (dialog);

	/* Don't show the "Make _More Copies" button if:
	 * - we wrote to a file
	 * - we wrote a merged session
	 * - we were not already asked for a series of copy */
	if (!priv->num_copies
	&&  !burner_burn_session_is_dest_file (priv->session)
	&& !(burner_burn_session_get_flags (priv->session) & BURNER_BURN_FLAG_MERGE)) {
		/* Useful button but it shouldn't be used for images */
/*		make_another = gtk_dialog_add_button (GTK_DIALOG (dialog),
						      _("Make _More Copies"),
						      GTK_RESPONSE_OK);
*/
	}

	if (burner_track_type_get_has_stream (&priv->input)
	|| (burner_track_type_get_has_medium (&priv->input)
	&& (burner_track_type_get_medium_type (&priv->input) & BURNER_MEDIUM_HAS_AUDIO))) {
		/* since we succeed offer the possibility to create cover if that's an audio disc */
		create_cover = gtk_dialog_add_button (GTK_DIALOG (dialog),
						      _("Create Co_ver"),
						      GTK_RESPONSE_CLOSE);
	}

	primary = burner_burn_dialog_get_success_message (dialog);
	gtk_widget_show(GTK_WIDGET(dialog));
	ca_gtk_play_for_widget(GTK_WIDGET(dialog), 0,
			       CA_PROP_EVENT_ID, "complete-media-burn",
			       CA_PROP_EVENT_DESCRIPTION, primary,
			       NULL);

	burner_burn_dialog_notify_daemon (dialog, primary);
	g_free (primary);

	res = burner_burn_dialog_success_run (dialog);

	if (make_another)
		gtk_widget_destroy (make_another);

	if (create_cover)
		gtk_widget_destroy (create_cover);

	return res;
}

static void
burner_burn_dialog_add_track_to_recent (BurnerTrack *track)
{
	gchar *uri = NULL;
	GtkRecentManager *recent;
	BurnerImageFormat format;
	gchar *groups [] = { "burner", NULL };
	gchar *mimes [] = { "application/x-cd-image",
			    "application/x-cue",
			    "application/x-toc",
			    "application/x-cdrdao-toc" };
	GtkRecentData recent_data = { NULL,
				      NULL,
				      NULL,
				      "burner",
				      "burner -p %u",
				      groups,
				      FALSE };

	if (!BURNER_IS_TRACK_IMAGE (track))
		return;

	format = burner_track_image_get_format (BURNER_TRACK_IMAGE (track));
	if (format == BURNER_IMAGE_FORMAT_NONE)
		return;

	/* Add it to recent file manager */
	switch (format) {
	case BURNER_IMAGE_FORMAT_BIN:
		recent_data.mime_type = mimes [0];
		uri = burner_track_image_get_source (BURNER_TRACK_IMAGE (track), TRUE);
		break;

	case BURNER_IMAGE_FORMAT_CUE:
		recent_data.mime_type = mimes [1];
		uri = burner_track_image_get_toc_source (BURNER_TRACK_IMAGE (track), TRUE);
		break;

	case BURNER_IMAGE_FORMAT_CLONE:
		recent_data.mime_type = mimes [2];
		uri = burner_track_image_get_toc_source (BURNER_TRACK_IMAGE (track), TRUE);
		break;

	case BURNER_IMAGE_FORMAT_CDRDAO:
		recent_data.mime_type = mimes [3];
		uri = burner_track_image_get_toc_source (BURNER_TRACK_IMAGE (track), TRUE);
		break;

	default:
		break;
	}

	if (!uri)
		return;

	recent = gtk_recent_manager_get_default ();
	gtk_recent_manager_add_full (recent,
				     uri,
				     &recent_data);
	g_free (uri);
}

static gboolean
burner_burn_dialog_end_session (BurnerBurnDialog *dialog,
				 BurnerBurnResult result,
				 GError *error)
{
	gboolean retry = FALSE;
	BurnerBurnDialogPrivate *priv;

	priv = BURNER_BURN_DIALOG_PRIVATE (dialog);

	g_timer_stop (priv->total_time);

	if (result == BURNER_BURN_CANCEL) {
		/* nothing to do */
	}
	else if (error || result != BURNER_BURN_OK) {
		burner_burn_dialog_notify_error (dialog, error);
	}
	else if (priv->num_copies) {
		retry = burner_burn_dialog_notify_copy_finished (dialog, error);
		if (!retry)
			burner_burn_dialog_notify_success (dialog);
	}
	else {
		/* see if an image was created. If so, add it to GtkRecent. */
		if (burner_burn_session_is_dest_file (priv->session)) {
			GSList *tracks;

			tracks = burner_burn_session_get_tracks (priv->session);
			for (; tracks; tracks = tracks->next) {
				BurnerTrack *track;

				track = tracks->data;
				burner_burn_dialog_add_track_to_recent (track);
			}
		}

		retry = burner_burn_dialog_notify_success (dialog);
		priv->num_copies = retry * 2;
	}

	if (priv->burn) {
		g_object_unref (priv->burn);
		priv->burn = NULL;
	}

	if (priv->rates) {
		g_slist_free (priv->rates);
		priv->rates = NULL;
	}

	burner_burn_session_set_flags (BURNER_BURN_SESSION (priv->session), BURNER_BURN_FLAG_NONE);

	return retry;
}

static BurnerBurnResult
burner_burn_dialog_record_spanned_session (BurnerBurnDialog *dialog,
					    GError **error)
{
	BurnerDrive *burner;
	BurnerTrackType *type;
	gchar *success_message;
	BurnerBurnResult result;
	BurnerBurnDialogPrivate *priv;
	gchar *secondary_message = NULL;

	priv = BURNER_BURN_DIALOG_PRIVATE (dialog);
	burner = burner_burn_session_get_burner (priv->session);

	/* Get the messages now as they can change */
	type = burner_track_type_new ();
	burner_burn_session_get_input_type (priv->session, type);
	success_message = burner_burn_dialog_get_success_message (dialog);
	if (burner_track_type_get_has_data (type)) {
		secondary_message = g_strdup_printf ("%s.\n%s.",
						     success_message,
						     _("There are some files left to burn"));
		g_free (success_message);
	}
	else if (burner_track_type_get_has_stream (type)) {
		if (BURNER_STREAM_FORMAT_HAS_VIDEO (burner_track_type_get_stream_format (type)))
			secondary_message = g_strdup_printf ("%s.\n%s.",
							     success_message,
							     _("There are some more videos left to burn"));
		else
			secondary_message = g_strdup_printf ("%s.\n%s.",
							     success_message,
							     _("There are some more songs left to burn"));
		g_free (success_message);
	}
	else
		secondary_message = success_message;

	burner_track_type_free (type);

	do {
		gint res;

		result = burner_burn_record (priv->burn,
					      priv->session,
					      error);
		if (result != BURNER_BURN_OK) {
			g_free (secondary_message);
			return result;
		}

		/* See if we have more data to burn and ask for a new medium */
		result = burner_session_span_again (BURNER_SESSION_SPAN (priv->session));
		if (result == BURNER_BURN_OK)
			break;

		res = burner_burn_dialog_wait_for_insertion (dialog,
							      burner,
							      _("Please insert a writable CD or DVD."),
							      secondary_message, 
		                                              TRUE);

		if (res != GTK_RESPONSE_OK) {
			g_free (secondary_message);
			return BURNER_BURN_CANCEL;
		}

		result = burner_session_span_next (BURNER_SESSION_SPAN (priv->session));
		while (result == BURNER_BURN_ERR) {
			burner_drive_eject (burner, FALSE, NULL);
			res = burner_burn_dialog_wait_for_insertion (dialog,
								      burner,
								      _("Please insert a writable CD or DVD."),
								      _("Not enough space available on the disc"),
			                                              FALSE);
			if (res != GTK_RESPONSE_OK) {
				g_free (secondary_message);
				return BURNER_BURN_CANCEL;
			}
			result = burner_session_span_next (BURNER_SESSION_SPAN (priv->session));
		}

	} while (result == BURNER_BURN_RETRY);

	g_free (secondary_message);
	burner_session_span_stop (BURNER_SESSION_SPAN (priv->session));
	return result;
}

static BurnerBurnResult
burner_burn_dialog_record_session (BurnerBurnDialog *dialog)
{
	gboolean retry;
	GError *error = NULL;
	BurnerBurnResult result;
	BurnerBurnDialogPrivate *priv;

	priv = BURNER_BURN_DIALOG_PRIVATE (dialog);

	/* Update info */
	burner_burn_dialog_update_info (dialog,
	                                 &priv->input,
	                                 (burner_burn_session_get_flags (priv->session) & BURNER_BURN_FLAG_DUMMY) != 0);
	burner_burn_dialog_update_title (dialog, &priv->input);

	/* Start the recording session */
	burner_burn_dialog_activity_start (dialog);
	burner_dialog_button_image(gtk_dialog_get_action_area(GTK_DIALOG (dialog)));
	result = burner_burn_dialog_setup_session (dialog, &error);
	if (result != BURNER_BURN_OK)
		return result;

	if (BURNER_IS_SESSION_SPAN (priv->session))
		result = burner_burn_dialog_record_spanned_session (dialog, &error);
	else
		result = burner_burn_record (priv->burn,
					      priv->session,
					      &error);

	retry = burner_burn_dialog_end_session (dialog,
						 result,
						 error);
	if (result == BURNER_BURN_RETRY)
		return result;

	if (retry)
		return BURNER_BURN_RETRY;

	return BURNER_BURN_OK;
}

static gboolean
burner_burn_dialog_wait_for_ready_state_cb (BurnerBurnDialog *dialog)
{
	BurnerBurnDialogPrivate *priv;
	BurnerStatus *status;

	priv = BURNER_BURN_DIALOG_PRIVATE (dialog);

	status = burner_status_new ();
	burner_burn_session_get_status (priv->session, status);

	if (burner_status_get_result (status) == BURNER_BURN_NOT_READY
	||  burner_status_get_result (status) == BURNER_BURN_RUNNING) {
		gdouble progress;
		gchar *action;

		action = burner_status_get_current_action (status);
		burner_burn_dialog_action_changed_real (dialog,
							 BURNER_BURN_ACTION_GETTING_SIZE,
							 action);
		g_free (action);

		progress = burner_status_get_progress (status);
		burner_burn_dialog_progress_changed_real (dialog,
							   0,
							   0,
							   0,
							   progress,
							   progress,
							   -1.0,
							   priv->media);
		g_object_unref (status);

		/* Continue */
		return TRUE;
	}

	if (priv->loop)
		g_main_loop_quit (priv->loop);

	priv->wait_ready_state_id = 0;

	g_object_unref (status);
	return FALSE;
}

static gboolean
burner_burn_dialog_wait_for_ready_state (BurnerBurnDialog *dialog)
{
	BurnerBurnDialogPrivate *priv;
	BurnerBurnResult result;
	BurnerStatus *status;

	priv = BURNER_BURN_DIALOG_PRIVATE (dialog);

	status = burner_status_new ();
	result = burner_burn_session_get_status (priv->session, status);
	if (result == BURNER_BURN_NOT_READY || result == BURNER_BURN_RUNNING) {
		GMainLoop *loop;

		loop = g_main_loop_new (NULL, FALSE);
		priv->loop = loop;

		priv->wait_ready_state_id = g_timeout_add_seconds (1,
								   (GSourceFunc) burner_burn_dialog_wait_for_ready_state_cb,
								   dialog);
		g_main_loop_run (loop);

		priv->loop = NULL;

		if (priv->wait_ready_state_id) {
			g_source_remove (priv->wait_ready_state_id);
			priv->wait_ready_state_id = 0;
		}

		g_main_loop_unref (loop);

		/* Get the final status */
		result = burner_burn_session_get_status (priv->session, status);
	}

	g_object_unref (status);

	return (result == BURNER_BURN_OK);
}

static gboolean
burner_burn_dialog_run_real (BurnerBurnDialog *dialog,
			      BurnerBurnSession *session)
{
	BurnerBurnResult result;
	BurnerBurnDialogPrivate *priv;

	priv = BURNER_BURN_DIALOG_PRIVATE (dialog);

	g_object_ref (session);
	priv->session = session;

	/* update what we should display */
	burner_burn_session_get_input_type (session, &priv->input);
	burner_burn_dialog_update_media (dialog);

	/* show it early */
	gtk_widget_show (GTK_WIDGET (dialog));

	/* wait for ready state */
	if (!burner_burn_dialog_wait_for_ready_state (dialog))
		return FALSE;

	/* disable autoconfiguration */
	if (BURNER_IS_SESSION_CFG (priv->session))
		burner_session_cfg_disable (BURNER_SESSION_CFG (priv->session));

	priv->total_time = g_timer_new ();
	g_timer_stop (priv->total_time);

	priv->initial_title = g_strdup (gtk_window_get_title (GTK_WINDOW (dialog)));
	priv->initial_icon = g_strdup (gtk_window_get_icon_name (GTK_WINDOW (dialog)));

	do {
		if (!gtk_widget_get_visible (GTK_WIDGET (dialog)))
			gtk_widget_show (GTK_WIDGET (dialog));

		result = burner_burn_dialog_record_session (dialog);
	} while (result == BURNER_BURN_RETRY);

	if (priv->initial_title) {
		g_free (priv->initial_title);
		priv->initial_title = NULL;
	}

	if (priv->initial_icon) {
		g_free (priv->initial_icon);
		priv->initial_icon = NULL;
	}

	g_timer_destroy (priv->total_time);
	priv->total_time = NULL;

	priv->session = NULL;
	g_object_unref (session);

	/* re-enable autoconfiguration */
	if (BURNER_IS_SESSION_CFG (priv->session))
		burner_session_cfg_enable (BURNER_SESSION_CFG (priv->session));

	return (result == BURNER_BURN_OK);
}

/**
 * burner_burn_dialog_run_multi:
 * @dialog: a #BurnerBurnDialog
 * @session: a #BurnerBurnSession
 *
 * Start burning the contents of @session. Once a disc is burnt, a dialog
 * will be shown to the user and wait for a new insertion before starting to burn
 * again.
 *
 * Return value: a #gboolean. TRUE if the operation was successfully carried out, FALSE otherwise.
 **/
gboolean
burner_burn_dialog_run_multi (BurnerBurnDialog *dialog,
			       BurnerBurnSession *session)
{
	BurnerBurnResult result;
	BurnerBurnDialogPrivate *priv;

	priv = BURNER_BURN_DIALOG_PRIVATE (dialog);
	priv->num_copies = 1;

	result = burner_burn_dialog_run_real (dialog, session);
	return (result == BURNER_BURN_OK);
}


/**
 * burner_burn_dialog_run:
 * @dialog: a #BurnerBurnDialog
 * @session: a #BurnerBurnSession
 *
 * Start burning the contents of @session.
 *
 * Return value: a #gboolean. TRUE if the operation was successfully carried out, FALSE otherwise.
 **/
gboolean
burner_burn_dialog_run (BurnerBurnDialog *dialog,
			 BurnerBurnSession *session)
{
	BurnerBurnResult result;
	BurnerBurnDialogPrivate *priv;

	priv = BURNER_BURN_DIALOG_PRIVATE (dialog);
	priv->num_copies = 0;

	result = burner_burn_dialog_run_real (dialog, session);
	return (result == BURNER_BURN_OK);
}

static gboolean
burner_burn_dialog_cancel_dialog (BurnerBurnDialog *toplevel)
{
	gint result;
	GtkWidget *button;
	GtkWidget *message;

	message = burner_burn_dialog_create_message (toplevel,
	                                              GTK_MESSAGE_WARNING,
	                                              GTK_BUTTONS_NONE,
	                                              _("Do you really want to quit?"));

	gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG
						  (message),
						  _("Interrupting the process may make disc unusable."));

	button = gtk_dialog_add_button (GTK_DIALOG (message),
					_("C_ontinue Burning"),
					GTK_RESPONSE_OK);
	gtk_button_set_image (GTK_BUTTON (button),
			      gtk_image_new_from_stock (GTK_STOCK_OK,
							GTK_ICON_SIZE_BUTTON));

	button = gtk_dialog_add_button (GTK_DIALOG (message),
					_("_Cancel Burning"),
					GTK_RESPONSE_CANCEL);
	gtk_button_set_image (GTK_BUTTON (button),
			      gtk_image_new_from_stock (GTK_STOCK_CANCEL,
							GTK_ICON_SIZE_BUTTON));
	
	burner_dialog_button_image(gtk_dialog_get_action_area(GTK_DIALOG (message)));
	result = gtk_dialog_run (GTK_DIALOG (message));
	gtk_widget_destroy (message);

	return (result != GTK_RESPONSE_OK);
}

/**
 * burner_burn_dialog_cancel:
 * @dialog: a #BurnerBurnDialog
 * @force_cancellation: a #gboolean
 *
 * Cancel the ongoing operation run by @dialog; if @force_cancellation is FALSE then it can
 * happen that the operation won't be cancelled if there is a risk to make a disc unusable.
 *
 * Return value: a #gboolean. TRUE if it was sucessfully cancelled, FALSE otherwise.
 **/
gboolean
burner_burn_dialog_cancel (BurnerBurnDialog *dialog,
			    gboolean force_cancellation)
{
	BurnerBurnDialogPrivate *priv;

	priv = BURNER_BURN_DIALOG_PRIVATE (dialog);

	if (priv->loop) {
		g_main_loop_quit (priv->loop);
		return TRUE;
	}

	if (!priv->burn)
		return TRUE;

	if (burner_burn_cancel (priv->burn, (force_cancellation == TRUE)) == BURNER_BURN_DANGEROUS) {
		if (!burner_burn_dialog_cancel_dialog (dialog))
			return FALSE;

		burner_burn_cancel (priv->burn, FALSE);
	}

	return TRUE;
}

static void
burner_burn_dialog_cancel_clicked_cb (GtkWidget *button,
				       BurnerBurnDialog *dialog)
{
	/* a burning is ongoing cancel it */
	burner_burn_dialog_cancel (dialog, FALSE);
}

#ifdef HAVE_APP_INDICATOR
static void
burner_burn_dialog_indicator_cancel_cb (BurnerAppIndicator *indicator,
					 BurnerBurnDialog *dialog)
{
	burner_burn_dialog_cancel (dialog, FALSE);
}

static void
burner_burn_dialog_indicator_show_dialog_cb (BurnerAppIndicator *indicator,
					      gboolean show,
					      GtkWidget *dialog)
{
	BurnerBurnDialogPrivate *priv;

	priv = BURNER_BURN_DIALOG_PRIVATE (dialog);

	/* we prevent to show the burn dialog once the success dialog has been
	 * shown to avoid the following strange behavior:
	 * Steps:
	 * - start burning
	 * - move to another workspace (ie, virtual desktop)
	 * - when the burning finishes, double-click the notification icon
	 * - you'll be unable to dismiss the dialogues normally and their behaviour will
	 *   be generally strange */
	if (!priv->burn)
		return;

	if (show)
		gtk_widget_show (dialog);
	else
		gtk_widget_hide (dialog);
}
#endif

static void
on_exit_dialog (GtkWidget *button, GtkWidget *dialog)
{
	if (dialog)
		gtk_dialog_response (dialog, GTK_RESPONSE_CANCEL);
}

static void
burner_burn_dialog_init (BurnerBurnDialog * obj)
{
	GtkWidget *box;
	GtkWidget *vbox;
	GtkWidget *alignment;
	BurnerBurnDialogPrivate *priv;
//    GtkWidget *box;
//    GtkWidget *vbox;
	GtkWidget *title;
	GtkWidget *close_bt;
//    GtkWidget *alignment;

	priv = BURNER_BURN_DIALOG_PRIVATE (obj);

	gtk_window_set_default_size (GTK_WINDOW (obj), 500, 0);

//    gtk_widget_set_size_request(GTK_WIDGET(dialog),280,200);
	gtk_window_set_decorated(GTK_WINDOW(obj),FALSE);
	box = gtk_dialog_get_content_area (GTK_DIALOG (obj));
	gtk_style_context_add_class ( gtk_widget_get_style_context (box), "app_burn_dialog");

	vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
	gtk_widget_show (vbox);
	alignment = gtk_alignment_new (0.5, 0.5, 1.0, 0.9);
	gtk_widget_show (alignment);
	gtk_container_add (GTK_CONTAINER (box), alignment);
	gtk_container_add (GTK_CONTAINER (alignment), vbox);

	title = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
	gtk_box_pack_start (GTK_BOX (vbox),title , FALSE, TRUE, 4);
	gtk_widget_show (title);
//    gtk_widget_add_events(priv->mainwin, GDK_BUTTON_PRESS_MASK);
//    g_signal_connect(G_OBJECT(priv->mainwin), "button-press-event", G_CALLBACK (drag_handle), NULL);

	dlabel = gtk_label_new (_(priv->initial_title));
	gtk_widget_show (dlabel);
	gtk_misc_set_alignment (GTK_MISC (dlabel), 0.5, 0.5);
	gtk_box_pack_start (GTK_BOX (title), dlabel, FALSE, FALSE, 5);

	close_bt = gtk_button_new ();
	gtk_button_set_alignment(GTK_BUTTON(close_bt),0.5,0.5);
	gtk_widget_set_size_request(GTK_WIDGET(close_bt),45,30);
	gtk_widget_show (close_bt);
	gtk_style_context_add_class ( gtk_widget_get_style_context (close_bt), "close_bt");
	gtk_box_pack_end (GTK_BOX (title), close_bt, FALSE, TRUE, 5);
	gtk_container_set_border_width(GTK_CONTAINER(obj), 5);
	g_signal_connect (close_bt,
			"clicked",
			G_CALLBACK (burner_burn_dialog_indicator_cancel_cb),
			obj);

#ifdef HAVE_APP_INDICATOR
	priv->indicator = burner_app_indicator_new ();
	g_signal_connect (priv->indicator,
			  "cancel",
			  G_CALLBACK (burner_burn_dialog_indicator_cancel_cb),
			  obj);
	g_signal_connect (priv->indicator,
			  "show-dialog",
			  G_CALLBACK (burner_burn_dialog_indicator_show_dialog_cb),
			  obj);
#endif

#ifdef HAVE_UNITY
    priv->launcher_entry = unity_launcher_entry_get_for_desktop_id("burner.desktop");
#endif

	alignment = gtk_alignment_new (0.5, 0.5, 1.0, 1.0);
	gtk_widget_show (alignment);
	gtk_alignment_set_padding (GTK_ALIGNMENT (alignment), 6, 8, 6, 6);
//	gtk_box_pack_start (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (obj))),
	gtk_box_pack_start (GTK_BOX (vbox),
			    alignment,
			    TRUE,
			    TRUE,
			    0);

	vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
	gtk_widget_show (vbox);
	gtk_container_add (GTK_CONTAINER (alignment), vbox);

	box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
	gtk_widget_show (box);
	gtk_box_pack_start (GTK_BOX (vbox), box, FALSE, TRUE, 0);

	priv->header = gtk_label_new (NULL);
	gtk_widget_show (priv->header);
	gtk_misc_set_alignment (GTK_MISC (priv->header), 0.0, 0.5);
	gtk_misc_set_padding (GTK_MISC (priv->header), 0, 18);
	gtk_box_pack_start (GTK_BOX (box), priv->header, FALSE, TRUE, 0);

	priv->image = gtk_image_new ();
	gtk_misc_set_alignment (GTK_MISC (priv->image), 1.0, 0.5);
	gtk_widget_show (priv->image);
	gtk_box_pack_start (GTK_BOX (box), priv->image, TRUE, TRUE, 0);

	priv->progress = burner_burn_progress_new ();
	gtk_widget_show (priv->progress);
	gtk_box_pack_start (GTK_BOX (vbox),
			    priv->progress,
			    FALSE,
			    TRUE,
			    0);

	/* buttons */
	priv->cancel = gtk_dialog_add_button (GTK_DIALOG (obj),
					      GTK_STOCK_CANCEL,
					      GTK_RESPONSE_CANCEL);
	burner_dialog_button_image(gtk_dialog_get_action_area(GTK_DIALOG (obj)));
}

static void
burner_burn_dialog_finalize (GObject * object)
{
	BurnerBurnDialogPrivate *priv;

	priv = BURNER_BURN_DIALOG_PRIVATE (object);

	if (priv->wait_ready_state_id) {
		g_source_remove (priv->wait_ready_state_id);
		priv->wait_ready_state_id = 0;
	}

	if (priv->cancel_plugin) {
		g_cancellable_cancel (priv->cancel_plugin);
		priv->cancel_plugin = NULL;
	}

	if (priv->initial_title) {
		g_free (priv->initial_title);
		priv->initial_title = NULL;
	}

	if (priv->initial_icon) {
		g_free (priv->initial_icon);
		priv->initial_icon = NULL;
	}

	if (priv->burn) {
		burner_burn_cancel (priv->burn, TRUE);
		g_object_unref (priv->burn);
		priv->burn = NULL;
	}

#ifdef HAVE_APP_INDICATOR
	if (priv->indicator) {
		g_object_unref (priv->indicator);
		priv->indicator = NULL;
	}
#endif

	if (priv->session) {
		g_object_unref (priv->session);
		priv->session = NULL;
	}

	if (priv->total_time) {
		g_timer_destroy (priv->total_time);
		priv->total_time = NULL;
	}

	if (priv->rates) {
		g_slist_free (priv->rates);
		priv->rates = NULL;
	}

	G_OBJECT_CLASS (burner_burn_dialog_parent_class)->finalize (object);
}

static void
burner_burn_dialog_class_init (BurnerBurnDialogClass * klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	g_type_class_add_private (klass, sizeof (BurnerBurnDialogPrivate));

	object_class->finalize = burner_burn_dialog_finalize;
}

/**
 * burner_burn_dialog_new:
 *
 * Creates a new #BurnerBurnDialog object
 *
 * Return value: a #GtkWidget. Unref when it is not needed anymore.
 **/

GtkWidget *
burner_burn_dialog_new (void)
{
	BurnerBurnDialog *obj;

	obj = BURNER_BURN_DIALOG (g_object_new (BURNER_TYPE_BURN_DIALOG, NULL));

	return GTK_WIDGET (obj);
}
