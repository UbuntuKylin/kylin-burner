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
#include <glib/gi18n-lib.h>

#include <gtk/gtk.h>

#include "burner-tool-dialog.h"
#include "burner-tool-dialog-private.h"

#include "burner-progress.h"
#include "burner-medium-selection.h"

#include "burner-misc.h"

#include "burner-burn.h"

#include "burner-medium.h"
#include "burner-drive.h"
#include "burner-customize-title.h"

G_DEFINE_TYPE (BurnerToolDialog, burner_tool_dialog, GTK_TYPE_DIALOG);

typedef struct _BurnerToolDialogPrivate BurnerToolDialogPrivate;
struct _BurnerToolDialogPrivate {
	GtkWidget *upper_box;
	GtkWidget *lower_box;
	GtkWidget *selector;
	GtkWidget *progress;
	GtkWidget *button;
	GtkWidget *options;
	GtkWidget *cancel;

	BurnerBurn *burn;

	gboolean running;
};

#define BURNER_TOOL_DIALOG_PRIVATE(o)  (G_TYPE_INSTANCE_GET_PRIVATE ((o), BURNER_TYPE_TOOL_DIALOG, BurnerToolDialogPrivate))

static GtkDialogClass *parent_class = NULL;

static void
burner_tool_dialog_media_error (BurnerToolDialog *self)
{
	burner_utils_message_dialog (GTK_WIDGET (self),
				     _("The operation cannot be performed."),
				     _("The disc is not supported"),
				     GTK_MESSAGE_ERROR);
}

static void
burner_tool_dialog_media_busy (BurnerToolDialog *self)
{
	gchar *string;

	string = g_strdup_printf ("%s. %s",
				  _("The drive is busy"),
				  _("Make sure another application is not using it"));
	burner_utils_message_dialog (GTK_WIDGET (self),
				     _("The operation cannot be performed."),
				     string,
				     GTK_MESSAGE_ERROR);
	g_free (string);
}

static void
burner_tool_dialog_no_media (BurnerToolDialog *self)
{
	burner_utils_message_dialog (GTK_WIDGET (self),
				     _("The operation cannot be performed."),
				     _("The drive is empty"),
				     GTK_MESSAGE_ERROR);
}

void
burner_tool_dialog_set_progress (BurnerToolDialog *self,
				  gdouble overall_progress,
				  gdouble task_progress,
				  glong remaining,
				  gint size_mb,
				  gint written_mb)
{
	BurnerToolDialogPrivate *priv;

	priv = BURNER_TOOL_DIALOG_PRIVATE (self);
	burner_burn_progress_set_status (BURNER_BURN_PROGRESS (priv->progress),
					  FALSE, /* no need for the media here since speed is not specified */
					  overall_progress,
					  task_progress,
					  remaining,
					  -1,
					  -1,
					  -1);
}

void
burner_tool_dialog_set_action (BurnerToolDialog *self,
				BurnerBurnAction action,
				const gchar *string)
{
	BurnerToolDialogPrivate *priv;

	priv = BURNER_TOOL_DIALOG_PRIVATE (self);
	burner_burn_progress_set_action (BURNER_BURN_PROGRESS (priv->progress),
					  action,
					  string);
}

static void
burner_tool_dialog_progress_changed (BurnerBurn *burn,
				      gdouble overall_progress,
				      gdouble progress,
				      glong time_remaining,
				      BurnerToolDialog *self)
{
	burner_tool_dialog_set_progress (self,
					  -1.0,
					  progress,
					  -1,
					  -1,
					  -1);
}

static void
burner_tool_dialog_action_changed (BurnerBurn *burn,
				    BurnerBurnAction action,
				    BurnerToolDialog *self)
{
	gchar *string;

	burner_burn_get_action_string (burn, action, &string);
	burner_tool_dialog_set_action (self,
					action,
					string);
	g_free (string);
}

BurnerBurn *
burner_tool_dialog_get_burn (BurnerToolDialog *self)
{
	BurnerToolDialogPrivate *priv;

	priv = BURNER_TOOL_DIALOG_PRIVATE (self);

	if (priv->burn) {
		burner_burn_cancel (priv->burn, FALSE);
		g_object_unref (priv->burn);
	}

	priv->burn = burner_burn_new ();
	g_signal_connect (priv->burn,
			  "progress-changed",
			  G_CALLBACK (burner_tool_dialog_progress_changed),
			  self);
	g_signal_connect (priv->burn,
			  "action-changed",
			  G_CALLBACK (burner_tool_dialog_action_changed),
			  self);

	return priv->burn;
}

static gboolean
burner_tool_dialog_run (BurnerToolDialog *self)
{
	BurnerToolDialogPrivate *priv;
	BurnerToolDialogClass *klass;
	gboolean close = FALSE;
	BurnerMedium *medium;
	BurnerMedia media;
	GdkCursor *cursor;
	GdkWindow *window;

	priv = BURNER_TOOL_DIALOG_PRIVATE (self);
	medium = burner_medium_selection_get_active (BURNER_MEDIUM_SELECTION (priv->selector));

	/* set up */
	gtk_widget_set_sensitive (priv->upper_box, FALSE);
	gtk_widget_set_sensitive (priv->lower_box, TRUE);
	gtk_widget_set_sensitive (GTK_WIDGET (priv->button), FALSE);

	cursor = gdk_cursor_new (GDK_WATCH);
	window = gtk_widget_get_window (GTK_WIDGET (self));
	gdk_window_set_cursor (window, cursor);
	g_object_unref (cursor);

	gtk_button_set_label (GTK_BUTTON (priv->cancel), GTK_STOCK_CANCEL);

	/* check the contents of the drive */
	media = burner_medium_get_status (medium);
	if (media == BURNER_MEDIUM_NONE) {
		burner_tool_dialog_no_media (self);
		gtk_widget_set_sensitive (GTK_WIDGET (priv->button), TRUE);
		goto end;
	}
	else if (media == BURNER_MEDIUM_UNSUPPORTED) {
		/* error out */
		gtk_widget_set_sensitive (GTK_WIDGET (priv->button), TRUE);
		burner_tool_dialog_media_error (self);
		goto end;
	}
	else if (media == BURNER_MEDIUM_BUSY) {
		gtk_widget_set_sensitive (GTK_WIDGET (priv->button), TRUE);
		burner_tool_dialog_media_busy (self);
		goto end;
	}

	priv->running = TRUE;

	klass = BURNER_TOOL_DIALOG_GET_CLASS (self);
	if (klass->activate)
		close = klass->activate (self, medium);

	priv->running = FALSE;

	if (medium)
		g_object_unref (medium);

	if (close)
		return TRUE;

end:

	gdk_window_set_cursor (window, NULL);
	gtk_button_set_label (GTK_BUTTON (priv->cancel), GTK_STOCK_CLOSE);

	gtk_widget_set_sensitive (priv->upper_box, TRUE);
	gtk_widget_set_sensitive (priv->lower_box, FALSE);

	burner_burn_progress_reset (BURNER_BURN_PROGRESS (priv->progress));

	return FALSE;
}

void
burner_tool_dialog_pack_options (BurnerToolDialog *self,
				  ...)
{
	gchar *title;
	va_list vlist;
	GtkWidget *child;
	GSList *list = NULL;
	BurnerToolDialogPrivate *priv;

	priv = BURNER_TOOL_DIALOG_PRIVATE (self);

	va_start (vlist, self);
	while ((child = va_arg (vlist, GtkWidget *)))
		list = g_slist_prepend (list, child);
	va_end (vlist);

//	title = g_strdup_printf ("<b>%s</b>", _("Options"));
	title = g_strdup_printf (_("Options"));
	priv->options = burner_utils_pack_properties_list (title, list);
	g_free (title);

	g_slist_free (list);

	gtk_widget_show_all (priv->options);
	gtk_box_pack_start (GTK_BOX (priv->upper_box),
			    priv->options,
			    FALSE,
			    FALSE,
			    0);
}

void
burner_tool_dialog_set_button (BurnerToolDialog *self,
				const gchar *text,
				const gchar *image,
				const gchar *theme)
{
	GtkWidget *button;
	BurnerToolDialogPrivate *priv;

	priv = BURNER_TOOL_DIALOG_PRIVATE (self);

	if (priv->button)
		g_object_unref (priv->button);

	button = burner_utils_make_button (text,
					    image,
					    theme,
					    GTK_ICON_SIZE_BUTTON);
	gtk_widget_show_all (button);
	gtk_dialog_add_action_widget (GTK_DIALOG (self),
				      button,
				      GTK_RESPONSE_OK);

	priv->button = button;
}

void
burner_tool_dialog_set_valid (BurnerToolDialog *self,
			       gboolean valid)
{
	BurnerToolDialogPrivate *priv;

	priv = BURNER_TOOL_DIALOG_PRIVATE (self);
	gtk_widget_set_sensitive (priv->button, valid);
}

void
burner_tool_dialog_set_medium_type_shown (BurnerToolDialog *self,
					   BurnerMediaType media_type)
{
	BurnerToolDialogPrivate *priv;

	priv = BURNER_TOOL_DIALOG_PRIVATE (self);
	burner_medium_selection_show_media_type (BURNER_MEDIUM_SELECTION (priv->selector),
						  media_type);
}

/**
 * burner_tool_dialog_get_medium:
 * @dialog: a #BurnerToolDialog
 *
 * This function returns the currently selected medium.
 *
 * Return value: a #BurnerMedium or NULL if none is set.
 **/

BurnerMedium *
burner_tool_dialog_get_medium (BurnerToolDialog *self)
{
	BurnerToolDialogPrivate *priv;

	priv = BURNER_TOOL_DIALOG_PRIVATE (self);
	return burner_medium_selection_get_active (BURNER_MEDIUM_SELECTION (priv->selector));
}

/**
 * burner_tool_dialog_set_medium:
 * @dialog: a #BurnerToolDialog
 * @medium: a #BurnerMedium
 *
 * Selects the medium that should be currently selected.
 *
 * Return value: a #gboolean. TRUE if it was successful.
 **/

gboolean
burner_tool_dialog_set_medium (BurnerToolDialog *self,
				BurnerMedium *medium)
{
	BurnerToolDialogPrivate *priv;

	if (!medium)
		return FALSE;

	priv = BURNER_TOOL_DIALOG_PRIVATE (self);

	return burner_medium_selection_set_active (BURNER_MEDIUM_SELECTION (priv->selector), medium);
}

static void
burner_tool_dialog_drive_changed_cb (BurnerMediumSelection *selection,
				      BurnerMedium *medium,
				      BurnerToolDialog *self)
{
	BurnerToolDialogClass *klass;

	klass = BURNER_TOOL_DIALOG_GET_CLASS (self);
	if (klass->medium_changed)
		klass->medium_changed (self, medium);
}

static gboolean
burner_tool_dialog_cancel_dialog (GtkWidget *toplevel)
{
	gint result;
	GtkWidget *button;
	GtkWidget *message;

	message = gtk_message_dialog_new (GTK_WINDOW (toplevel),
					  GTK_DIALOG_DESTROY_WITH_PARENT|
					  GTK_DIALOG_MODAL,
					  GTK_MESSAGE_WARNING,
					  GTK_BUTTONS_NONE,
					  _("Do you really want to quit?"));

	gtk_window_set_icon_name (GTK_WINDOW (message),
	                          gtk_window_get_icon_name (GTK_WINDOW (toplevel)));

	burner_message_title(message);

	gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (message),
						  _("Interrupting the process may make disc unusable."));
	gtk_dialog_add_buttons (GTK_DIALOG (message),
				GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
				NULL);

	button = burner_utils_make_button (_("_Continue"),
					    GTK_STOCK_OK,
					    NULL,
					    GTK_ICON_SIZE_BUTTON);
	gtk_widget_show_all (button);
	gtk_dialog_add_action_widget (GTK_DIALOG (message),
				      button, GTK_RESPONSE_OK);

	burner_dialog_button_image(gtk_dialog_get_action_area(GTK_DIALOG (message)));
	result = gtk_dialog_run (GTK_DIALOG (message));
	gtk_widget_destroy (message);

	if (result != GTK_RESPONSE_OK)
		return TRUE;

	return FALSE;
}

/**
 * burner_tool_dialog_cancel:
 * @dialog: a #BurnerToolDialog
 *
 * Cancels any ongoing operation.
 *
 * Return value: a #gboolean. TRUE when cancellation was successful. FALSE otherwise.
 **/

gboolean
burner_tool_dialog_cancel (BurnerToolDialog *self)
{
	BurnerToolDialogClass *klass;
	BurnerToolDialogPrivate *priv;

	klass = BURNER_TOOL_DIALOG_GET_CLASS (self);
	if (klass->cancel) {
		gboolean res;

		res = klass->cancel (self);
		if (!res)
			return FALSE;
	}

	priv = BURNER_TOOL_DIALOG_PRIVATE (self);
	if (!priv->burn)
		return TRUE;

	if (burner_burn_cancel (priv->burn, TRUE) == BURNER_BURN_DANGEROUS) {
		if (!burner_tool_dialog_cancel_dialog (GTK_WIDGET (self)))
			return FALSE;

		burner_burn_cancel (priv->burn, FALSE);
	}

	return TRUE;
}

static gboolean
burner_tool_dialog_delete (GtkWidget *widget, GdkEventAny *event)
{
	BurnerToolDialog *self;

	self = BURNER_TOOL_DIALOG (widget);

	return (burner_tool_dialog_cancel (self) != TRUE);
}

static void
burner_tool_dialog_response (GtkDialog *dialog,
			      GtkResponseType response,
			      gpointer NULL_data)
{
	if (response == GTK_RESPONSE_CANCEL) {
		if (!burner_tool_dialog_cancel (BURNER_TOOL_DIALOG (dialog)))
			g_signal_stop_emission_by_name (dialog, "response");
	}
	else if (response == GTK_RESPONSE_OK) {
		if (!burner_tool_dialog_run (BURNER_TOOL_DIALOG (dialog)))
			g_signal_stop_emission_by_name (dialog, "response");
	}
}

static void
burner_tool_dialog_finalize (GObject *object)
{
	G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
burner_tool_dialog_constructed (GObject *object)
{
	BurnerToolDialogPrivate *priv;

	G_OBJECT_CLASS (burner_tool_dialog_parent_class)->constructed (object);

	priv = BURNER_TOOL_DIALOG_PRIVATE (object);

	burner_medium_selection_show_media_type (BURNER_MEDIUM_SELECTION (priv->selector),
						  BURNER_MEDIA_TYPE_REWRITABLE |
						  BURNER_MEDIA_TYPE_WRITABLE |
						  BURNER_MEDIA_TYPE_AUDIO |
						  BURNER_MEDIA_TYPE_DATA);
}

static void
burner_tool_dialog_class_init (BurnerToolDialogClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);
	GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

	g_type_class_add_private (klass, sizeof (BurnerToolDialogPrivate));

	parent_class = g_type_class_peek_parent(klass);
	object_class->finalize = burner_tool_dialog_finalize;
	object_class->constructed = burner_tool_dialog_constructed;

	widget_class->delete_event = burner_tool_dialog_delete;
}

static void
burner_tool_dialog_init (BurnerToolDialog *obj)
{
	GtkWidget *title;
	GtkWidget *content_area;
	gchar *title_str;
	BurnerToolDialogPrivate *priv;

	priv = BURNER_TOOL_DIALOG_PRIVATE (obj);

	/* upper part */
	priv->upper_box = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
	gtk_widget_show (GTK_WIDGET (priv->upper_box));

	priv->selector = burner_medium_selection_new ();
	gtk_widget_show (GTK_WIDGET (priv->selector));

//	title_str = g_strdup_printf ("<b>%s</b>", _("Select a disc"));
	title_str = g_strdup_printf (_("Select a disc"));
	gtk_box_pack_start (GTK_BOX (priv->upper_box),
			    burner_utils_pack_properties (title_str,
							   priv->selector,
							   NULL),
			    FALSE, FALSE, 0);
	g_free (title_str);

	content_area = gtk_dialog_get_content_area (GTK_DIALOG (obj));
	gtk_box_pack_start (GTK_BOX (content_area),
			    priv->upper_box,
			    FALSE,
			    FALSE,
			    0);

	/* lower part */
	priv->lower_box = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
	gtk_container_set_border_width (GTK_CONTAINER (priv->lower_box), 12);
	gtk_widget_set_sensitive (priv->lower_box, FALSE);
	gtk_widget_show (priv->lower_box);

	gtk_box_pack_start (GTK_BOX (content_area),
			    priv->lower_box,
			    FALSE,
			    FALSE,
			    0);

//	title_str = g_strdup_printf ("<b>%s</b>", _("Progress"));
	title_str = g_strdup_printf (_("Progress"));
	title = gtk_label_new (title_str);
	g_free (title_str);

	gtk_label_set_use_markup (GTK_LABEL (title), TRUE);
	gtk_misc_set_alignment (GTK_MISC (title), 0.0, 0.5);
	gtk_misc_set_padding(GTK_MISC (title), 0, 6);
	gtk_widget_show (title);
	gtk_box_pack_start (GTK_BOX (priv->lower_box),
			    title,
			    FALSE,
			    FALSE,
			    0);

	priv->progress = burner_burn_progress_new ();
	gtk_widget_show (priv->progress);
	g_object_set (G_OBJECT (priv->progress),
		      "show-info", FALSE,
		      NULL);

	gtk_box_pack_start (GTK_BOX (content_area),
			    priv->progress,
			    FALSE,
			    FALSE,
			    0);

	/* buttons */
	priv->cancel = gtk_button_new_from_stock (GTK_STOCK_CLOSE);
	gtk_widget_show (priv->cancel);
	gtk_dialog_add_action_widget (GTK_DIALOG (obj),
				      priv->cancel,
				      GTK_RESPONSE_CANCEL);

	g_signal_connect (G_OBJECT (priv->selector),
			  "medium-changed",
			  G_CALLBACK (burner_tool_dialog_drive_changed_cb),
			  obj);

	g_signal_connect (obj,
			  "response",
			  G_CALLBACK (burner_tool_dialog_response),
			  NULL);

	gtk_window_resize (GTK_WINDOW (obj), 10, 10);
}
