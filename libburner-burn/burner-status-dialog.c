/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/*
 * Libburner-burn
 * Copyright (C) Philippe Rouquier 2005-2009 <bonfire-app@wanadoo.fr>
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

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <glib.h>
#include <glib-object.h>
#include <glib/gi18n-lib.h>

#include <gtk/gtk.h>

#include "burner-misc.h"

#include "burner-units.h"

#include "burner-track-data-cfg.h"

#include "burner-enums.h"
#include "burner-session.h"
#include "burner-status-dialog.h"
#include "burn-plugin-manager.h"
#include "burner-customize-title.h"

typedef struct _BurnerStatusDialogPrivate BurnerStatusDialogPrivate;
struct _BurnerStatusDialogPrivate
{
	BurnerBurnSession *session;
	GtkWidget *progress;
	GtkWidget *action;

	guint id;

	guint accept_2G_files:1;
	guint reject_2G_files:1;
	guint accept_deep_files:1;
	guint reject_deep_files:1;
};

#define BURNER_STATUS_DIALOG_PRIVATE(o)  (G_TYPE_INSTANCE_GET_PRIVATE ((o), BURNER_TYPE_STATUS_DIALOG, BurnerStatusDialogPrivate))

enum {
	PROP_0,
	PROP_SESSION
};

G_DEFINE_TYPE (BurnerStatusDialog, burner_status_dialog, GTK_TYPE_MESSAGE_DIALOG);

enum {
	USER_INTERACTION,
	LAST_SIGNAL
};
static guint burner_status_dialog_signals [LAST_SIGNAL] = { 0 };

static void
burner_status_dialog_update (BurnerStatusDialog *self,
			      BurnerStatus *status)
{
	gchar *string;
	gchar *size_str;
	goffset session_bytes;
	gchar *current_action;
	BurnerBurnResult res;
	BurnerTrackType *type;
	BurnerStatusDialogPrivate *priv;

	priv = BURNER_STATUS_DIALOG_PRIVATE (self);

	current_action = burner_status_get_current_action (status);
	if (current_action) {
		gchar *string;

		string = g_strdup_printf ("<i>%s</i>", current_action);
		gtk_label_set_markup (GTK_LABEL (priv->action), string);
		g_free (string);
	}
	else
		gtk_label_set_markup (GTK_LABEL (priv->action), "");

	g_free (current_action);

	if (burner_status_get_progress (status) < 0.0)
		gtk_progress_bar_pulse (GTK_PROGRESS_BAR (priv->progress));
	else
		gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (priv->progress),
					       burner_status_get_progress (status));

	res = burner_burn_session_get_size (priv->session,
					     NULL,
					     &session_bytes);
	if (res != BURNER_BURN_OK)
		return;

	type = burner_track_type_new ();
	burner_burn_session_get_input_type (priv->session, type);

	if (burner_track_type_get_has_stream (type)) {
		if (BURNER_STREAM_FORMAT_HAS_VIDEO (burner_track_type_get_stream_format (type))) {
			guint64 free_time;

			/* This is an embarassing problem: this is an approximation based on the fact that
			 * 2 hours = 4.3GiB */
			free_time = session_bytes * 72000LL / 47LL;
			size_str = burner_units_get_time_string (free_time,
			                                          TRUE,
			                                          TRUE);
		}
		else
			size_str = burner_units_get_time_string (session_bytes, TRUE, FALSE);
	}
	/* NOTE: this is perfectly fine as burner_track_type_get_medium_type ()
	 * will return BURNER_MEDIUM_NONE if this is not a MEDIUM track type */
	else if (burner_track_type_get_medium_type (type) & BURNER_MEDIUM_HAS_AUDIO)
		size_str = burner_units_get_time_string (session_bytes, TRUE, FALSE);
	else
		size_str = g_format_size (session_bytes);

	burner_track_type_free (type);

	string = g_strdup_printf (_("Estimated size: %s"), size_str);
	g_free (size_str);

	gtk_progress_bar_set_text (GTK_PROGRESS_BAR (priv->progress), string);
	g_free (string);
}

static void
burner_status_dialog_session_ready (BurnerStatusDialog *dialog)
{
	gtk_dialog_response (GTK_DIALOG (dialog), GTK_RESPONSE_OK);
}

static gboolean
burner_status_dialog_wait_for_ready_state (BurnerStatusDialog *dialog)
{
	BurnerStatusDialogPrivate *priv;
	BurnerBurnResult result;
	BurnerStatus *status;

	priv = BURNER_STATUS_DIALOG_PRIVATE (dialog);

	status = burner_status_new ();
	result = burner_burn_session_get_status (priv->session, status);

	if (result != BURNER_BURN_NOT_READY && result != BURNER_BURN_RUNNING) {
		burner_status_dialog_session_ready (dialog);
		g_object_unref (status);
		priv->id = 0;
		return FALSE;
	}

	burner_status_dialog_update (dialog, status);
	g_object_unref (status);
	return TRUE;
}

static gboolean
burner_status_dialog_deep_directory_cb (BurnerTrackDataCfg *project,
					 const gchar *name,
					 BurnerStatusDialog *dialog)
{
	gint answer;
	gchar *string;
	GtkWidget *message;
	GtkWindow *transient_win;
	BurnerStatusDialogPrivate *priv;

	priv = BURNER_STATUS_DIALOG_PRIVATE (dialog);

	if (priv->accept_deep_files)
		return TRUE;

	if (priv->reject_deep_files)
		return FALSE;

	g_signal_emit (dialog,
	               burner_status_dialog_signals [USER_INTERACTION],
	               0);

	gtk_widget_hide (GTK_WIDGET (dialog));

	string = g_strdup_printf (_("Do you really want to add \"%s\" to the selection?"), name);
	transient_win = gtk_window_get_transient_for (GTK_WINDOW (dialog));
	message = gtk_message_dialog_new (transient_win,
	                                  GTK_DIALOG_DESTROY_WITH_PARENT|
					  GTK_DIALOG_MODAL,
					  GTK_MESSAGE_WARNING,
					  GTK_BUTTONS_NONE,
					  "%s",
					  string);
	g_free (string);

	burner_message_title(message);	
	if (gtk_window_get_icon_name (GTK_WINDOW (dialog)))
		gtk_window_set_icon_name (GTK_WINDOW (message),
					  gtk_window_get_icon_name (GTK_WINDOW (dialog)));
	else if (transient_win)
		gtk_window_set_icon_name (GTK_WINDOW (message),
					  gtk_window_get_icon_name (transient_win));

	gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (message),
						  _("The children of this directory will have 7 parent directories."
						    "\nBurner can create an image of such a file hierarchy and burn it but the disc may not be readable on all operating systems."
						    "\nNote: Such a file hierarchy is known to work on Linux."));

	gtk_dialog_add_button (GTK_DIALOG (message), _("Ne_ver Add Such File"), GTK_RESPONSE_REJECT);
	gtk_dialog_add_button (GTK_DIALOG (message), _("Al_ways Add Such File"), GTK_RESPONSE_ACCEPT);

	burner_dialog_button_image(gtk_dialog_get_action_area(GTK_DIALOG (message)));
	answer = gtk_dialog_run (GTK_DIALOG (message));
	gtk_widget_destroy (message);

	gtk_widget_show (GTK_WIDGET (dialog));

	priv->accept_deep_files = (answer == GTK_RESPONSE_ACCEPT);
	priv->reject_deep_files = (answer == GTK_RESPONSE_REJECT);

	return (answer != GTK_RESPONSE_YES && answer != GTK_RESPONSE_ACCEPT);
}

static gboolean
burner_status_dialog_2G_file_cb (BurnerTrackDataCfg *track,
				  const gchar *name,
				  BurnerStatusDialog *dialog)
{
	gint answer;
	gchar *string;
	GtkWidget *message;
	GtkWindow *transient_win;
	BurnerStatusDialogPrivate *priv;

	priv = BURNER_STATUS_DIALOG_PRIVATE (dialog);

	if (priv->accept_2G_files)
		return TRUE;

	if (priv->reject_2G_files)
		return FALSE;

	g_signal_emit (dialog,
	               burner_status_dialog_signals [USER_INTERACTION],
	               0);

	gtk_widget_hide (GTK_WIDGET (dialog));

	string = g_strdup_printf (_("Do you really want to add \"%s\" to the selection and use the third version of the ISO9660 standard to support it?"), name);
	transient_win = gtk_window_get_transient_for (GTK_WINDOW (dialog));
	message = gtk_message_dialog_new (transient_win,
	                                  GTK_DIALOG_DESTROY_WITH_PARENT|
					  GTK_DIALOG_MODAL,
					  GTK_MESSAGE_WARNING,
					  GTK_BUTTONS_NONE,
					  "%s",
					  string);
	g_free (string);

	burner_message_title(message);
	if (gtk_window_get_icon_name (GTK_WINDOW (dialog)))
		gtk_window_set_icon_name (GTK_WINDOW (message),
					  gtk_window_get_icon_name (GTK_WINDOW (dialog)));
	else if (transient_win)
		gtk_window_set_icon_name (GTK_WINDOW (message),
					  gtk_window_get_icon_name (transient_win));

	gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (message),
						  _("The size of the file is over 2 GiB. Files larger than 2 GiB are not supported by the ISO9660 standard in its first and second versions (the most widespread ones)."
						    "\nIt is recommended to use the third version of the ISO9660 standard, which is supported by most operating systems, including Linux and all versions of Windows™."
						    "\nHowever, Mac OS X cannot read images created with version 3 of the ISO9660 standard."));

	gtk_dialog_add_button (GTK_DIALOG (message), _("Ne_ver Add Such File"), GTK_RESPONSE_REJECT);
	gtk_dialog_add_button (GTK_DIALOG (message), _("Al_ways Add Such File"), GTK_RESPONSE_ACCEPT);

	burner_dialog_button_image(gtk_dialog_get_action_area(GTK_DIALOG (message)));
	answer = gtk_dialog_run (GTK_DIALOG (message));
	gtk_widget_destroy (message);

	gtk_widget_show (GTK_WIDGET (dialog));

	priv->accept_2G_files = (answer == GTK_RESPONSE_ACCEPT);
	priv->reject_2G_files = (answer == GTK_RESPONSE_REJECT);

	return (answer != GTK_RESPONSE_YES && answer != GTK_RESPONSE_ACCEPT);
}

static void
burner_status_dialog_joliet_rename_cb (BurnerTrackData *track,
					BurnerStatusDialog *dialog)
{
	GtkResponseType answer;
	GtkWindow *transient_win;
	GtkWidget *message;
	gchar *secondary;

	g_signal_emit (dialog,
	               burner_status_dialog_signals [USER_INTERACTION],
	               0);

	gtk_widget_hide (GTK_WIDGET (dialog));

	transient_win = gtk_window_get_transient_for (GTK_WINDOW (dialog));
	message = gtk_message_dialog_new (transient_win,
					  GTK_DIALOG_DESTROY_WITH_PARENT|
					  GTK_DIALOG_MODAL,
					  GTK_MESSAGE_WARNING,
					  GTK_BUTTONS_NONE,
					  "%s",
					  _("Should files be renamed to be fully Windows-compatible?"));

	burner_message_title(message);
	if (gtk_window_get_icon_name (GTK_WINDOW (dialog)))
		gtk_window_set_icon_name (GTK_WINDOW (message),
					  gtk_window_get_icon_name (GTK_WINDOW (dialog)));
	else if (transient_win)
		gtk_window_set_icon_name (GTK_WINDOW (message),
					  gtk_window_get_icon_name (transient_win));

	secondary = g_strdup_printf ("%s\n%s",
				     _("Some files don't have a suitable name for a fully Windows-compatible CD."),
				     _("Those names should be changed and truncated to 64 characters."));
	gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (message), "%s", secondary);
	g_free (secondary);

	gtk_dialog_add_button (GTK_DIALOG (message),
			       _("_Disable Full Windows Compatibility"),
			       GTK_RESPONSE_CANCEL);
	gtk_dialog_add_button (GTK_DIALOG (message),
			       _("_Rename for Full Windows Compatibility"),
			       GTK_RESPONSE_YES);

	burner_dialog_button_image(gtk_dialog_get_action_area(GTK_DIALOG (message)));
	answer = gtk_dialog_run (GTK_DIALOG (message));
	gtk_widget_destroy (message);

	if (answer != GTK_RESPONSE_YES)
		burner_track_data_rm_fs (track, BURNER_IMAGE_FS_JOLIET);
	else
		burner_track_data_add_fs (track, BURNER_IMAGE_FS_JOLIET);

	gtk_widget_show (GTK_WIDGET (dialog));
}

static void
burner_status_dialog_wait_for_session (BurnerStatusDialog *dialog)
{
	BurnerStatus *status;
	BurnerBurnResult result;
	BurnerTrackType *track_type;
	BurnerStatusDialogPrivate *priv;

	priv = BURNER_STATUS_DIALOG_PRIVATE (dialog);

	/* Make sure we really need to run this dialog */
	status = burner_status_new ();
	result = burner_burn_session_get_status (priv->session, status);
	if (result != BURNER_BURN_NOT_READY && result != BURNER_BURN_RUNNING) {
		burner_status_dialog_session_ready (dialog);
		g_object_unref (status);
		return;
	}

	gtk_window_set_position (GTK_WINDOW (dialog), GTK_WIN_POS_CENTER_ON_PARENT);

	track_type = burner_track_type_new ();
	burner_burn_session_get_input_type (priv->session, track_type);
	if (burner_track_type_get_has_data (track_type)) {
		GSList *tracks;
		BurnerTrack *track;

		tracks = burner_burn_session_get_tracks (priv->session);
		track = tracks->data;

		if (BURNER_IS_TRACK_DATA_CFG (track)) {
			g_signal_connect (track,
					  "joliet-rename",
					  G_CALLBACK (burner_status_dialog_joliet_rename_cb),
					  dialog);
			g_signal_connect (track,
					  "2G-file",
					  G_CALLBACK (burner_status_dialog_2G_file_cb),
					  dialog);
			g_signal_connect (track,
					  "deep-directory",
					  G_CALLBACK (burner_status_dialog_deep_directory_cb),
					  dialog);
		}
	}
	burner_track_type_free (track_type);

	burner_status_dialog_update (dialog, status);
	g_object_unref (status);
	priv->id = g_timeout_add (200,
				  (GSourceFunc) burner_status_dialog_wait_for_ready_state,
				  dialog);
}

static void
burner_status_dialog_init (BurnerStatusDialog *object)
{
	BurnerStatusDialogPrivate *priv;
	GtkWidget *main_box;
	GtkWidget *box;

	priv = BURNER_STATUS_DIALOG_PRIVATE (object);

	gtk_dialog_add_button (GTK_DIALOG (object),
			       GTK_STOCK_CANCEL,
			       GTK_RESPONSE_CANCEL);

	box = gtk_box_new (GTK_ORIENTATION_VERTICAL, 4);
	gtk_widget_show (box);
	main_box = gtk_dialog_get_content_area (GTK_DIALOG (object));
	gtk_box_pack_end (GTK_BOX (main_box),
			  box,
			  TRUE,
			  TRUE,
			  0);

	priv->progress = gtk_progress_bar_new ();
	gtk_widget_show (priv->progress);
	gtk_progress_bar_set_text (GTK_PROGRESS_BAR (priv->progress), " ");
	gtk_box_pack_start (GTK_BOX (box),
			    priv->progress,
			    TRUE,
			    TRUE,
			    0);

	priv->action = gtk_label_new ("");
	gtk_widget_show (priv->action);
	gtk_label_set_use_markup (GTK_LABEL (priv->action), TRUE);
	gtk_misc_set_alignment (GTK_MISC (priv->action), 0.0, 0.5);
	gtk_box_pack_start (GTK_BOX (box),
			    priv->action,
			    FALSE,
			    TRUE,
			    0);
}

static void
burner_status_dialog_set_property (GObject *object,
				    guint prop_id,
				    const GValue *value,
				    GParamSpec *pspec)
{
	BurnerStatusDialogPrivate *priv;

	g_return_if_fail (BURNER_IS_STATUS_DIALOG (object));

	priv = BURNER_STATUS_DIALOG_PRIVATE (object);

	switch (prop_id)
	{
	case PROP_SESSION: /* Readable and only writable at creation time */
		priv->session = BURNER_BURN_SESSION (g_value_get_object (value));
		g_object_ref (priv->session);
		burner_status_dialog_wait_for_session (BURNER_STATUS_DIALOG (object));
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
		break;
	}
}

static void
burner_status_dialog_get_property (GObject *object,
				    guint prop_id,
				    GValue *value,
				    GParamSpec *pspec)
{
	BurnerStatusDialogPrivate *priv;

	g_return_if_fail (BURNER_IS_STATUS_DIALOG (object));

	priv = BURNER_STATUS_DIALOG_PRIVATE (object);

	switch (prop_id)
	{
	case PROP_SESSION:
		g_value_set_object (value, priv->session);
		g_object_ref (priv->session);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
		break;
	}
}

static void
burner_status_dialog_finalize (GObject *object)
{
	BurnerStatusDialogPrivate *priv;

	priv = BURNER_STATUS_DIALOG_PRIVATE (object);
	if (priv->session) {
		g_object_unref (priv->session);
		priv->session = NULL;
	}

	if (priv->id) {
		g_source_remove (priv->id);
		priv->id = 0;
	}

	G_OBJECT_CLASS (burner_status_dialog_parent_class)->finalize (object);
}

static void
burner_status_dialog_class_init (BurnerStatusDialogClass *klass)
{
	GObjectClass* object_class = G_OBJECT_CLASS (klass);

	g_type_class_add_private (klass, sizeof (BurnerStatusDialogPrivate));

	object_class->finalize = burner_status_dialog_finalize;
	object_class->set_property = burner_status_dialog_set_property;
	object_class->get_property = burner_status_dialog_get_property;

	g_object_class_install_property (object_class,
					 PROP_SESSION,
					 g_param_spec_object ("session",
							      "The session",
							      "The session to work with",
							      BURNER_TYPE_BURN_SESSION,
							      G_PARAM_READWRITE|G_PARAM_CONSTRUCT_ONLY));

	burner_status_dialog_signals [USER_INTERACTION] =
	    g_signal_new ("user_interaction",
			  BURNER_TYPE_STATUS_DIALOG,
			  G_SIGNAL_RUN_LAST|G_SIGNAL_ACTION|G_SIGNAL_NO_RECURSE,
			  0,
			  NULL,
			  NULL,
			  g_cclosure_marshal_VOID__VOID,
			  G_TYPE_NONE,
			  0);
}

GtkWidget *
burner_status_dialog_new (BurnerBurnSession *session,
			   GtkWidget *parent)
{
	return g_object_new (BURNER_TYPE_STATUS_DIALOG,
			     "session", session,
			     "transient-for", parent,
			     "modal", TRUE,
			     "title",  _("Size Estimation"),
			     "message-type", GTK_MESSAGE_OTHER,
			     "text", _("Please wait until the estimation of the size is completed."),
			     "secondary-text", _("All files need to be analysed to complete this operation."),
			     NULL);
}
