/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/*
 * Burner
 * Copyright (C) Philippe Rouquier 2005-2009 <bonfire-app@wanadoo.fr>
 * Copyright (C) 2017,Tianjin KYLIN Information Technology Co., Ltd.
 *
 *  Burner is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 * burner is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with burner.  If not, write to:
 * 	The Free Software Foundation, Inc.,
 * 	51 Franklin Street, Fifth Floor
 * 	Boston, MA  02110-1301, USA.
 */

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <glib.h>
#include <glib/gstdio.h>
#include <glib/gi18n.h>

#include <gtk/gtk.h>
#include <gdk/gdkx.h>

#include "burner-misc.h"
#include "burner-io.h"

#include "burner-project.h"
#include "burner-app.h"
#include "burner-setting.h"
#include "burner-blank-dialog.h"
#include "burner-sum-dialog.h"
#include "burner-eject-dialog.h"
#include "burner-project-manager.h"
#include "burner-project-type-chooser.h"
#include "burner-pref.h"

#include "burner-drive.h"
#include "burner-medium.h"
#include "burner-volume.h"

#include "burner-tags.h"
#include "burner-burn.h"
#include "burner-track-disc.h"
#include "burner-track-image.h"
#include "burner-track-data-cfg.h"
#include "burner-track-stream-cfg.h"
#include "burner-track-image-cfg.h"
#include "burner-session.h"
#include "burner-burn-lib.h"

#include "burner-status-dialog.h"
#include "burner-burn-options.h"
#include "burner-burn-dialog.h"
#include "burner-jacket-edit.h"

#include "burn-plugin-manager.h"
#include "burner-drive-settings.h"
#include "burner-customize-title.h"

int nb_items;
GtkWidget *widget[10];
GtkWidget *statusbarbox;
GtkWidget *maximize_bt;
GtkWidget *unmaximize_bt;

static ItemDescription items [] = {
	{N_("W_elcom project"),
	N_("Create a data CD/DVD"),
	N_("Create a CD/DVD containing any type of data that can only be read on a computer"),
	"media-optical-welc-new",
	BURNER_PROJECT_TYPE_WELC},
//       {N_("Audi_o project"),
//      N_("Create a traditional audio CD"),
//      N_("Create a traditional audio CD that will be playable on computers and stereos"),
//      "media-optical-audio-new",
//      BURNER_PROJECT_TYPE_AUDIO},
	{N_("D_ata project"),
	N_("Create a data CD/DVD"),
	N_("Create a CD/DVD containing any type of data that can only be read on a computer"),
	"media-optical-data-new1",
	BURNER_PROJECT_TYPE_DATA},
//       {N_("_Video project"),
//      N_("Create a video DVD or an SVCD"),
//      N_("Create a video DVD or an SVCD that is readable on TV readers"),
//      "media-optical-video-new",
//      BURNER_PROJECT_TYPE_VIDEO},
//       {N_("Disc _copy"),
//    N_("Create 1:1 copy of a CD/DVD"),
//    N_("Create a 1:1 copy of an audio CD or a data CD/DVD on your hard disk or on another CD/DVD"),
//    "media-optical-copy",
//    BURNER_PROJECT_TYPE_COPY},
	{N_("Burn _image"),
	N_("Burn an existing CD/DVD image to disc"),
	N_("Burn an existing CD/DVD image to disc"),
	"iso-image-burn",
	BURNER_PROJECT_TYPE_ISO},
};

typedef struct _BurnerAppPrivate BurnerAppPrivate;
struct _BurnerAppPrivate
{
	GApplication *gapp;

	BurnerSetting *setting;

	GdkWindow *parent;

	GtkWidget *mainwin;

	GtkWidget *burn_dialog;
	GtkWidget *tool_dialog;

	/* This is the toplevel window currently displayed */
	GtkWidget *toplevel;

	GtkWidget *projects;
	GtkWidget *contents;
	GtkWidget *contents2;
	GtkWidget *contents3;
	GtkWidget *statusbar1;
	GtkWidget *statusbar2;
	GtkUIManager *manager;

	guint tooltip_ctx;

	gchar *saved_contents;

	guint is_maximized:1;
	guint mainwin_running:1;
};

#define BURNER_APP_PRIVATE(o)  (G_TYPE_INSTANCE_GET_PRIVATE ((o), BURNER_TYPE_APP, BurnerAppPrivate))


G_DEFINE_TYPE (BurnerApp, burner_app, G_TYPE_OBJECT);

enum {
	PROP_NONE,
	PROP_GAPP
};

/**
 * Menus and toolbar
 */

static void on_prefs_cb (GtkAction *action, BurnerApp *app);
static void on_eject_cb (GtkAction *action, BurnerApp *app);
static void on_erase_cb (GtkAction *action, BurnerApp *app);
static void on_integrity_check_cb (GtkAction *action, BurnerApp *app);

static void on_exit_cb (GtkAction *action, BurnerApp *app);

static void on_about_cb (GtkAction *action, BurnerApp *app);
static void on_help_cb (GtkAction *action, BurnerApp *app);

static GtkActionEntry entries[] = {
	{"ProjectMenu", NULL, N_("_Project")},
	{"ViewMenu", NULL, N_("_View")},
	{"EditMenu", NULL, N_("_Edit")},
	{"ToolMenu", NULL, N_("_Tools")},

	{"HelpMenu", NULL, N_("_Help")},

	{"Plugins", NULL, N_("P_lugins"), NULL,
	 N_("Choose plugins for Burner"), G_CALLBACK (on_prefs_cb)},

	{"Eject", "media-eject", N_("E_ject"), NULL,
	 N_("Eject a disc"), G_CALLBACK (on_eject_cb)},

	{"Blank", "media-optical-blank", N_("_Blank…"), NULL,
	 N_("Blank a disc"), G_CALLBACK (on_erase_cb)},

	{"Check", NULL, N_("_Check Integrity…"), NULL,
	 N_("Check data integrity of disc"), G_CALLBACK (on_integrity_check_cb)},

	{"Quit", GTK_STOCK_QUIT, NULL, NULL,
	 N_("Quit Burner"), G_CALLBACK (on_exit_cb)},

	{"Contents", GTK_STOCK_HELP, N_("_Contents"), "F1", N_("Display help"),
	 G_CALLBACK (on_help_cb)}, 

	{"About", GTK_STOCK_ABOUT, NULL, NULL, N_("About"),
	 G_CALLBACK (on_about_cb)},
};


static const gchar *description = {
	"<ui>"
	    "<menubar name='menubar' >"
	    "<menu action='ProjectMenu'>"
		"<placeholder name='ProjectPlaceholder'/>"
		"<separator/>"
		"<menuitem action='Quit'/>"
	    "</menu>"
	    "<menu action='EditMenu'>"
		"<placeholder name='EditPlaceholder'/>"
		"<separator/>"
		"<menuitem action='Plugins'/>"
	    "</menu>"
//	    "<menu action='ViewMenu'>"
//		"<placeholder name='ViewPlaceholder'/>"
//	    "</menu>"
	    "<menu action='ToolMenu'>"
		"<placeholder name='DiscPlaceholder'/>"
//		"<menuitem action='Eject'/>"
		"<menuitem action='Blank'/>"
		"<menuitem action='Check'/>"
	    "</menu>"
	    "<menu action='HelpMenu'>"
		"<menuitem action='Contents'/>"
		"<separator/>"
		"<menuitem action='About'/>"
	    "</menu>"
	    "</menubar>"
	"</ui>"
};

static gchar *
burner_app_get_path (const gchar *name)
{
	gchar *directory;
	gchar *retval;

	directory = g_build_filename (g_get_user_config_dir (),
				      "burner",
				      NULL);
	if (!g_file_test (directory, G_FILE_TEST_EXISTS))
		g_mkdir_with_parents (directory, S_IRWXU);

	retval = g_build_filename (directory, name, NULL);
	g_free (directory);
	return retval;
}

static gboolean
burner_app_load_window_state (BurnerApp *app)
{
	gint width;
	gint height;
	gint state = 0;
	gpointer value;

	GdkScreen *screen;
	GdkRectangle rect;
	gint monitor;

	BurnerAppPrivate *priv;

	priv = BURNER_APP_PRIVATE (app);

	/* Make sure that on first run the window has a default size of at least
	 * 85% of the screen (hardware not GTK+) */
	screen = gtk_window_get_screen (GTK_WINDOW (priv->mainwin));
	monitor = gdk_screen_get_monitor_at_window (screen, gtk_widget_get_window (GTK_WIDGET (priv->mainwin)));
	gdk_screen_get_monitor_geometry (screen, monitor, &rect);

	burner_setting_get_value (burner_setting_get_default (),
	                           BURNER_SETTING_WIN_WIDTH,
	                           &value);
	width = GPOINTER_TO_INT (value);

	burner_setting_get_value (burner_setting_get_default (),
	                           BURNER_SETTING_WIN_HEIGHT,
	                           &value);
	height = GPOINTER_TO_INT (value);

	burner_setting_get_value (burner_setting_get_default (),
	                           BURNER_SETTING_WIN_MAXIMIZED,
	                           &value);
	state = GPOINTER_TO_INT (value);

	if (width > 0 && height > 0)
		gtk_window_resize (GTK_WINDOW (priv->mainwin),
				   width,
				   height);
	else
		gtk_window_resize (GTK_WINDOW (priv->mainwin),
		                   rect.width / 100 *85,
		                   rect.height / 100 * 85);

	if (state)
		gtk_window_maximize (GTK_WINDOW (priv->mainwin));

	return TRUE;
}

/**
 * returns FALSE when nothing prevents the shutdown
 * returns TRUE when shutdown should be delayed
 */

gboolean
burner_app_save_contents (BurnerApp *app,
			   gboolean cancellable)
{
	gboolean cancel;
	gchar *project_path;
	BurnerAppPrivate *priv;

	priv = BURNER_APP_PRIVATE (app);

	if (priv->burn_dialog) {
		if (cancellable)
			return (burner_burn_dialog_cancel (BURNER_BURN_DIALOG (priv->burn_dialog), FALSE) == FALSE);

		gtk_widget_destroy (priv->burn_dialog);
		return FALSE;
	}

	if (priv->tool_dialog) {
		if (cancellable) {
			if (BURNER_IS_TOOL_DIALOG (priv->tool_dialog))
				return (burner_tool_dialog_cancel (BURNER_TOOL_DIALOG (priv->tool_dialog)) == FALSE);
			else if (BURNER_IS_EJECT_DIALOG (priv->tool_dialog))
				return (burner_eject_dialog_cancel (BURNER_EJECT_DIALOG (priv->tool_dialog)) == FALSE);
		}

		gtk_widget_destroy (priv->tool_dialog);
		return FALSE;
	}

	/* If we are not having a main window there is no point in going further */
	if (!priv->mainwin)
		return FALSE;

	if (priv->saved_contents) {
		g_free (priv->saved_contents);
		priv->saved_contents = NULL;
	}

	project_path = burner_app_get_path (BURNER_SESSION_TMP_PROJECT_PATH);
	cancel = burner_project_manager_save_session (BURNER_PROJECT_MANAGER (priv->projects),
						       project_path,
						       &priv->saved_contents,
						       cancellable);
	g_free (project_path);

	return cancel;
}

const gchar *
burner_app_get_saved_contents (BurnerApp *app)
{
	BurnerAppPrivate *priv;

	priv = BURNER_APP_PRIVATE (app);
	return priv->saved_contents;
}

/**
 * These functions are only useful because they set the proper toplevel parent
 * for the message dialog. The following one also sets some properties in case
 * there isn't any toplevel parent (like show in taskbar, ...).
 **/

static void
burner_app_toplevel_destroyed_cb (GtkWidget *object,
				   BurnerApp *app)
{
	BurnerAppPrivate *priv;

	priv = BURNER_APP_PRIVATE (app);
	priv->toplevel = NULL;
}

GtkWidget *
burner_app_dialog (BurnerApp *app,
		    const gchar *primary_message,
		    GtkButtonsType button_type,
		    GtkMessageType msg_type)
{
	gboolean is_on_top = FALSE;
	BurnerAppPrivate *priv;
	GtkWindow *toplevel;
	GtkWidget *dialog;

	priv = BURNER_APP_PRIVATE (app);

	if (priv->mainwin) {
		toplevel = GTK_WINDOW (priv->mainwin);
		gtk_widget_show (priv->mainwin);
	}
	else if (!priv->toplevel) {
		is_on_top = TRUE;
		toplevel = NULL;
	}
	else
		toplevel = GTK_WINDOW (priv->toplevel);

	dialog = gtk_message_dialog_new (toplevel,
					 GTK_DIALOG_DESTROY_WITH_PARENT|
					 GTK_DIALOG_MODAL,
					 msg_type,
					 button_type,
					 "%s",
					 primary_message);

	burner_message_title(dialog);
	if (!toplevel && priv->parent) {
		gtk_widget_realize (GTK_WIDGET (dialog));
		gtk_window_set_modal (GTK_WINDOW (dialog), TRUE);
		gdk_window_set_transient_for (gtk_widget_get_window (GTK_WIDGET (dialog)), priv->parent);
	}

	if (is_on_top) {
		gtk_window_set_skip_pager_hint (GTK_WINDOW (dialog), FALSE);
		gtk_window_set_skip_taskbar_hint (GTK_WINDOW (dialog), FALSE);

		priv->toplevel = dialog;
		g_signal_connect (dialog,
				  "destroy",
				  G_CALLBACK (burner_app_toplevel_destroyed_cb),
				  app);
	}

	return dialog;
}

void
burner_app_alert (BurnerApp *app,
		   const gchar *primary_message,
		   const gchar *secondary_message,
		   GtkMessageType type)
{
	GtkWidget *parent = NULL;
	gboolean is_on_top= TRUE;
	BurnerAppPrivate *priv;
	GtkWidget *alert;

	priv = BURNER_APP_PRIVATE (app);

	/* Whatever happens, they need a parent or must be in the taskbar */
	if (priv->mainwin) {
		parent = GTK_WIDGET (priv->mainwin);
		is_on_top = FALSE;
	}
	else if (priv->toplevel) {
		parent = priv->toplevel;
		is_on_top = FALSE;
	}

	alert = burner_utils_create_message_dialog (parent,
						     primary_message,
						     secondary_message,
						     type);
	if (!parent && priv->parent) {
		is_on_top = FALSE;

		gtk_widget_realize (GTK_WIDGET (alert));
		gtk_window_set_modal (GTK_WINDOW (alert), TRUE);
		gdk_window_set_transient_for (gtk_widget_get_window (GTK_WIDGET (alert)), priv->parent);
	}

	if (is_on_top) {
		gtk_window_set_title (GTK_WINDOW (alert), _("Disc Burner"));
		gtk_window_set_skip_pager_hint (GTK_WINDOW (alert), FALSE);
		gtk_window_set_skip_taskbar_hint (GTK_WINDOW (alert), FALSE);
	}

	burner_dialog_button_image(gtk_dialog_get_action_area(GTK_DIALOG (alert)));
	gtk_dialog_run (GTK_DIALOG (alert));
	gtk_widget_destroy (alert);
}

GtkWidget *
burner_app_get_statusbar1 (BurnerApp *app)
{
	BurnerAppPrivate *priv;

	priv = BURNER_APP_PRIVATE (app);

	/* FIXME: change with future changes */
	return priv->statusbar1;
}

GtkWidget *
burner_app_get_statusbar2 (BurnerApp *app)
{
	BurnerAppPrivate *priv;

	priv = BURNER_APP_PRIVATE (app);
	return priv->statusbar2;
}

GtkWidget *
burner_app_get_project_manager (BurnerApp *app)
{
	BurnerAppPrivate *priv;

	priv = BURNER_APP_PRIVATE (app);
	return priv->projects;
}

static gboolean
on_destroy_cb (GtkWidget *window, BurnerApp *app)
{
	BurnerAppPrivate *priv;

	priv = BURNER_APP_PRIVATE (app);
	if (priv->mainwin)
		gtk_main_quit ();

	return FALSE;
}

static gboolean
on_delete_cb (GtkWidget *window, GdkEvent *event, BurnerApp *app)
{
	BurnerAppPrivate *priv;

	priv = BURNER_APP_PRIVATE (app);
	if (!priv->mainwin)
		return FALSE;

	if (burner_app_save_contents (app, TRUE))
		return TRUE;

	return FALSE;
}

static void
on_exit_cb (GtkAction *action, BurnerApp *app)
{
	BurnerAppPrivate *priv;

	priv = BURNER_APP_PRIVATE (app);

	if (burner_app_save_contents (app, TRUE))
		return;

	if (priv->mainwin)
		gtk_widget_destroy (GTK_WIDGET (priv->mainwin));
}

gboolean
burner_app_is_running (BurnerApp *app)
{
	BurnerAppPrivate *priv;

	priv = BURNER_APP_PRIVATE (app);
	return priv->mainwin_running;
}

void
burner_app_set_parent (BurnerApp *app,
			guint parent_xid)
{
	BurnerAppPrivate *priv;

	priv = BURNER_APP_PRIVATE (app);
#ifdef GDK_WINDOWING_X11
	if (GDK_IS_X11_DISPLAY (gdk_display_get_default ()))
		priv->parent = gdk_x11_window_foreign_new_for_display (gdk_display_get_default (), parent_xid);
#endif
}

gboolean
burner_app_burn (BurnerApp *app,
		  BurnerBurnSession *session,
		  gboolean multi)
{
	gboolean success;
	GtkWidget *dialog;
	BurnerAppPrivate *priv;

	priv = BURNER_APP_PRIVATE (app);

	/* now setup the burn dialog */
	dialog = burner_burn_dialog_new ();
	gtk_window_set_icon_name (GTK_WINDOW (dialog), "burner");

	priv->burn_dialog = dialog;

	burner_app_set_toplevel (app, GTK_WINDOW (dialog));
	burner_dialog_button_image(gtk_dialog_get_action_area(GTK_DIALOG (dialog)));
	if (!multi)
		success = burner_burn_dialog_run (BURNER_BURN_DIALOG (dialog),
						   BURNER_BURN_SESSION (session));
	else
		success = burner_burn_dialog_run_multi (BURNER_BURN_DIALOG (dialog),
							 BURNER_BURN_SESSION (session));
	priv->burn_dialog = NULL;

	/* The destruction of the dialog will bring the main window forward */
	gtk_widget_destroy (dialog);
	return success;
}

static BurnerBurnResult
burner_app_burn_options (BurnerApp *app,
			  BurnerSessionCfg *session)
{
	GtkWidget *dialog;
	GtkResponseType answer;

	dialog = burner_burn_options_new (session);
	burner_app_set_toplevel (app, GTK_WINDOW (dialog));
	gtk_window_set_icon_name (GTK_WINDOW (dialog), "burner");

	burner_dialog_button_image(gtk_dialog_get_action_area(GTK_DIALOG (dialog)));
	answer = gtk_dialog_run (GTK_DIALOG (dialog));

	/* The destruction of the dialog will bring the main window forward */
	gtk_widget_destroy (dialog);
	if (answer == GTK_RESPONSE_OK)
		return BURNER_BURN_OK;

	if (answer == GTK_RESPONSE_ACCEPT)
		return BURNER_BURN_RETRY;

	return BURNER_BURN_CANCEL;

}

static void
burner_app_session_burn (BurnerApp *app,
			  BurnerSessionCfg *session,
			  gboolean burn)
{
	BurnerDriveSettings *settings;

	/* Set saved temporary directory for the session.
	 * NOTE: BurnerBurnSession can cope with NULL path */
	settings = burner_drive_settings_new ();
	burner_drive_settings_set_session (settings, BURNER_BURN_SESSION (session));

	/* We need to have a drive to start burning immediately */
	if (burn && burner_burn_session_get_burner (BURNER_BURN_SESSION (session))) {
		BurnerStatus *status;
		BurnerBurnResult result;

		status = burner_status_new ();
		burner_burn_session_get_status (BURNER_BURN_SESSION (session), status);

		result = burner_status_get_result (status);
		if (result == BURNER_BURN_NOT_READY || result == BURNER_BURN_RUNNING) {
			GtkWidget *status_dialog;

			status_dialog = burner_status_dialog_new (BURNER_BURN_SESSION (session), NULL);
			burner_dialog_button_image(gtk_dialog_get_action_area(GTK_DIALOG (status_dialog)));
			gtk_dialog_run (GTK_DIALOG (status_dialog));
			gtk_widget_destroy (status_dialog);

			burner_burn_session_get_status (BURNER_BURN_SESSION (session), status);
			result = burner_status_get_result (status);
		}
		g_object_unref (status);

		if (result == BURNER_BURN_CANCEL) {
			g_object_unref (settings);
			return;
		}

		if (result != BURNER_BURN_OK) {
			GError *error;

			error = burner_status_get_error (status);
			burner_app_alert (app,
					   _("Error while burning."),
					   error? error->message:"",
					   GTK_MESSAGE_ERROR);
			if (error)
				g_error_free (error);

			g_object_unref (settings);
			return;
		}

		result = burner_app_burn (app,
		                           BURNER_BURN_SESSION (session),
		                           FALSE);
	}
	else {
		BurnerBurnResult result;

		result = burner_app_burn_options (app, session);
		if (result == BURNER_BURN_OK || result == BURNER_BURN_RETRY) {
			result = burner_app_burn (app,
			                           BURNER_BURN_SESSION (session),
			                           (result == BURNER_BURN_RETRY));
		}
	}

	g_object_unref (settings);
}

void
burner_app_copy_disc (BurnerApp *app,
		       BurnerDrive *burner,
		       const gchar *device,
		       const gchar *cover,
		       gboolean burn)
{
	BurnerTrackDisc *track = NULL;
	BurnerSessionCfg *session;
	BurnerDrive *drive = NULL;

	session = burner_session_cfg_new ();
	track = burner_track_disc_new ();
	burner_burn_session_add_track (BURNER_BURN_SESSION (session),
					BURNER_TRACK (track),
					NULL);
	g_object_unref (track);

	/* if a device is specified then get the corresponding medium */
	if (device) {
		BurnerMediumMonitor *monitor;

		monitor = burner_medium_monitor_get_default ();
		drive = burner_medium_monitor_get_drive (monitor, device);
		g_object_unref (monitor);

		if (!drive)
			return;

		burner_track_disc_set_drive (BURNER_TRACK_DISC (track), drive);
		g_object_unref (drive);
	}

	/* Set a cover if any. */
	if (cover) {
		GValue *value;

		value = g_new0 (GValue, 1);
		g_value_init (value, G_TYPE_STRING);
		g_value_set_string (value, cover);
		burner_burn_session_tag_add (BURNER_BURN_SESSION (session),
					      BURNER_COVER_URI,
					      value);
	}

	burner_burn_session_set_burner (BURNER_BURN_SESSION (session), burner);
	burner_app_session_burn (app, session, burn);
	g_object_unref (session);
}

void
burner_app_image (BurnerApp *app,
		   BurnerDrive *burner,
		   const gchar *uri_arg,
		   gboolean burn)
{
	BurnerSessionCfg *session;
	BurnerTrackImageCfg *track = NULL;

	/* setup, show, and run options dialog */
	session = burner_session_cfg_new ();
	track = burner_track_image_cfg_new ();
	burner_burn_session_add_track (BURNER_BURN_SESSION (session),
					BURNER_TRACK (track),
					NULL);
	g_object_unref (track);

	if (uri_arg) {
		GFile *file;
		gchar *uri;

		file = g_file_new_for_commandline_arg (uri_arg);
		uri = g_file_get_uri (file);
		g_object_unref (file);

		burner_track_image_cfg_set_source (track, uri);
		g_free (uri);
	}

	burner_burn_session_set_burner (BURNER_BURN_SESSION (session), burner);
	burner_app_session_burn (app, session, burn);
	g_object_unref (session);
}

static void
burner_app_process_session (BurnerApp *app,
			     BurnerSessionCfg *session,
			     gboolean burn)
{
	if (!burn) {
		GtkWidget *manager;
		BurnerAppPrivate *priv;

		priv = BURNER_APP_PRIVATE (app);
		if (!priv->mainwin)
			burner_app_create_mainwin (app);

		manager = burner_app_get_project_manager (app);
		burner_project_manager_open_session (BURNER_PROJECT_MANAGER (manager), session);
	}
	else
		burner_app_session_burn (app, session, TRUE);
}

void
burner_app_burn_uri (BurnerApp *app,
		      BurnerDrive *burner,
		      gboolean burn)
{
	GFileEnumerator *enumerator;
	BurnerSessionCfg *session;
	BurnerTrackDataCfg *track;
	GFileInfo *info = NULL;
	GError *error = NULL;
	GFile *file;

	/* Here we get the contents from the burn:// URI and add them
	 * individually to the data project. This is done in case it is
	 * empty no to start the "Getting Project Size" dialog and then
	 * show the "Project is empty" dialog. Do this synchronously as:
	 * - we only want the top nodes which reduces time needed
	 * - it's always local
	 * - windows haven't been shown yet
	 * NOTE: don't use any file specified on the command line. */
	file = g_file_new_for_uri ("burn://");
	enumerator = g_file_enumerate_children (file,
						G_FILE_ATTRIBUTE_STANDARD_NAME,
						G_FILE_QUERY_INFO_NONE,
						NULL,
						&error);
	if (!enumerator) {
		gchar *string;

		if (error) {
			string = g_strdup (error->message);
			g_error_free (error);
		}
		else
			string = g_strdup (_("An internal error occurred"));

		burner_app_alert (app,
				   _("Error while loading the project"),
				   string,
				   GTK_MESSAGE_ERROR);

		g_free (string);
		g_object_unref (file);
		return;
	}

	session = burner_session_cfg_new ();

	track = burner_track_data_cfg_new ();
	burner_burn_session_add_track (BURNER_BURN_SESSION (session), BURNER_TRACK (track), NULL);
	g_object_unref (track);

	while ((info = g_file_enumerator_next_file (enumerator, NULL, &error)) != NULL) {
		gchar *uri;

		uri = g_strconcat ("burn:///", g_file_info_get_name (info), NULL);
		g_object_unref (info);

		burner_track_data_cfg_add (track, uri, NULL);
		g_free (uri);
	}

	g_object_unref (enumerator);
	g_object_unref (file);

	if (error) {
		g_object_unref (session);

		/* NOTE: this check errors in g_file_enumerator_next_file () */
		burner_app_alert (app,
				   _("Error while loading the project"),
				   error->message,
				   GTK_MESSAGE_ERROR);
		return;
	}

	if (gtk_tree_model_iter_n_children (GTK_TREE_MODEL (track), NULL) == 0) {
	        g_object_unref (session);
		burner_app_alert (app,
				   _("Please add files to the project."),
				   _("The project is empty"),
				   GTK_MESSAGE_ERROR);
		return;
	}

	burner_burn_session_set_burner (BURNER_BURN_SESSION (session), burner);
	burner_app_process_session (app, session, burn);
	g_object_unref (session);
}

void
burner_app_data (BurnerApp *app,
		  BurnerDrive *burner,
		  gchar * const *uris,
		  gboolean burn)
{
	BurnerTrackDataCfg *track;
	BurnerSessionCfg *session;
	BurnerAppPrivate *priv;
	int i, num;

	priv = BURNER_APP_PRIVATE (app);

	if (!uris) {
		GtkWidget *manager;

		if (burn) {
			burner_app_alert (app,
					   _("Please add files to the project."),
					   _("The project is empty"),
					   GTK_MESSAGE_ERROR);
			return;
		}

		if (!priv->mainwin)
			burner_app_create_mainwin (app);

		manager = burner_app_get_project_manager (app);
		burner_project_manager_switch (BURNER_PROJECT_MANAGER (manager),
						BURNER_PROJECT_TYPE_DATA,
						TRUE);
		return;
	}

	session = burner_session_cfg_new ();
	track = burner_track_data_cfg_new ();
	burner_burn_session_add_track (BURNER_BURN_SESSION (session), BURNER_TRACK (track), NULL);
	g_object_unref (track);

	num = g_strv_length ((gchar **) uris);
	for (i = 0; i < num; i ++) {
		GFile *file;
		gchar *uri;

		file = g_file_new_for_commandline_arg (uris [i]);
		uri = g_file_get_uri (file);
		g_object_unref (file);

		/* Ignore the return value */
		burner_track_data_cfg_add (track, uri, NULL);
		g_free (uri);
	}

	burner_burn_session_set_burner (BURNER_BURN_SESSION (session), burner);
	burner_app_process_session (app, session, burn);
	g_object_unref (session);
}

void
burner_app_stream (BurnerApp *app,
		    BurnerDrive *burner,
		    gchar * const *uris,
		    gboolean is_video,
		    gboolean burn)
{
	BurnerSessionCfg *session;
	BurnerAppPrivate *priv;
	int i, num;

	priv = BURNER_APP_PRIVATE (app);

	session = burner_session_cfg_new ();

	if (!uris) {
		GtkWidget *manager;

		if (burn) {
			burner_app_alert (app,
					   _("Please add files to the project."),
					   _("The project is empty"),
					   GTK_MESSAGE_ERROR);
			return;
		}

		if (!priv->mainwin)
			burner_app_create_mainwin (app);

		manager = burner_app_get_project_manager (app);
		burner_project_manager_switch (BURNER_PROJECT_MANAGER (manager),
						is_video? BURNER_PROJECT_TYPE_VIDEO:BURNER_PROJECT_TYPE_AUDIO,
						TRUE);
		return;
	}

	num = g_strv_length ((gchar **) uris);
	for (i = 0; i < num; i ++) {
		BurnerTrackStreamCfg *track;
		GFile *file;
		gchar *uri;

		file = g_file_new_for_commandline_arg (uris [i]);
		uri = g_file_get_uri (file);
		g_object_unref (file);

		track = burner_track_stream_cfg_new ();
		burner_track_stream_set_source (BURNER_TRACK_STREAM (track), uri);
		g_free (uri);

		if (is_video)
			burner_track_stream_set_format (BURNER_TRACK_STREAM (track),
			                                 BURNER_VIDEO_FORMAT_UNDEFINED);

		burner_burn_session_add_track (BURNER_BURN_SESSION (session), BURNER_TRACK (track), NULL);
		g_object_unref (track);
	}

	burner_burn_session_set_burner (BURNER_BURN_SESSION (session), burner);
	burner_app_process_session (app, session, burn);
	g_object_unref (session);
}

void
burner_app_blank (BurnerApp *app,
		   BurnerDrive *burner,
		   gboolean burn)
{
	BurnerBlankDialog *dialog;
	BurnerAppPrivate *priv;

	priv = BURNER_APP_PRIVATE (app);
	dialog = burner_blank_dialog_new ();
	gtk_window_set_icon_name (GTK_WINDOW (dialog), "burner");
	burner_dialog_title(GTK_DIALOG(dialog),gtk_window_get_title (GTK_WINDOW (dialog)));

	if (burner) {
		BurnerMedium *medium;

		medium = burner_drive_get_medium (burner);
		burner_tool_dialog_set_medium (BURNER_TOOL_DIALOG (dialog), medium);
	}

	priv->tool_dialog = GTK_WIDGET (dialog);
	if (!priv->mainwin) {
		gtk_widget_realize (GTK_WIDGET (dialog));

		if (priv->parent) {
			gtk_window_set_modal (GTK_WINDOW (dialog), TRUE);
			gdk_window_set_transient_for (gtk_widget_get_window (GTK_WIDGET (dialog)), priv->parent);
		}
	}
	else {
		GtkWidget *toplevel;

		/* FIXME! This is a bad idea and needs fixing */
		toplevel = gtk_widget_get_toplevel (GTK_WIDGET (priv->mainwin));

		gtk_window_set_transient_for (GTK_WINDOW (dialog), GTK_WINDOW (toplevel));
		gtk_window_set_modal (GTK_WINDOW (dialog), TRUE);
		gtk_window_set_position (GTK_WINDOW (dialog), GTK_WIN_POS_CENTER_ON_PARENT);
	}

	burner_dialog_button_image(gtk_dialog_get_action_area(GTK_DIALOG (dialog)));
	gtk_dialog_run (GTK_DIALOG (dialog));
	gtk_widget_destroy (GTK_WIDGET (dialog));
	priv->tool_dialog = NULL;
}

static void
on_erase_cb (GtkAction *action, BurnerApp *app)
{
	burner_app_blank (app, NULL, FALSE);
}

static void
on_eject_cb (GtkAction *action, BurnerApp *app)
{
	GtkWidget *dialog;
	GtkWidget *toplevel;
	BurnerAppPrivate *priv;

	priv = BURNER_APP_PRIVATE (app);

	dialog = burner_eject_dialog_new ();
	gtk_window_set_icon_name (GTK_WINDOW (dialog), "burner");

	/* FIXME! This is a bad idea and needs fixing */
	toplevel = gtk_widget_get_toplevel (GTK_WIDGET (priv->mainwin));

	gtk_window_set_transient_for (GTK_WINDOW (dialog), GTK_WINDOW (toplevel));
	gtk_window_set_modal (GTK_WINDOW (dialog), TRUE);
	gtk_window_set_position (GTK_WINDOW (dialog), GTK_WIN_POS_CENTER_ON_PARENT);

	priv->tool_dialog = dialog;
	burner_dialog_button_image(gtk_dialog_get_action_area(GTK_DIALOG (dialog)));
	gtk_dialog_run (GTK_DIALOG (dialog));
	priv->tool_dialog = NULL;

	gtk_widget_destroy (dialog);
}

void
burner_app_check (BurnerApp *app,
		   BurnerDrive *burner,
		   gboolean burn)
{
	BurnerSumDialog *dialog;
	BurnerAppPrivate *priv;

	priv = BURNER_APP_PRIVATE (app);

	dialog = burner_sum_dialog_new ();
	gtk_window_set_icon_name (GTK_WINDOW (dialog), "burner");

	burner_dialog_title(GTK_DIALOG(dialog),gtk_window_get_title (GTK_WINDOW (dialog)));
	priv->tool_dialog = GTK_WIDGET (dialog);

	if (burner) {
		BurnerMedium *medium;

		medium = burner_drive_get_medium (burner);
		burner_tool_dialog_set_medium (BURNER_TOOL_DIALOG (dialog), medium);
	}

	if (!priv->mainwin) {
		gtk_widget_realize (GTK_WIDGET (dialog));

		if (priv->parent) {
			gtk_window_set_modal (GTK_WINDOW (dialog), TRUE);
			gdk_window_set_transient_for (gtk_widget_get_window (GTK_WIDGET (dialog)), priv->parent);
		}
	}
	else {
		GtkWidget *toplevel;

		/* FIXME! This is a bad idea and needs fixing */
		toplevel = gtk_widget_get_toplevel (GTK_WIDGET (priv->mainwin));

		gtk_window_set_transient_for (GTK_WINDOW (dialog), GTK_WINDOW (toplevel));
		gtk_window_set_modal (GTK_WINDOW (dialog), TRUE);
		gtk_window_set_position (GTK_WINDOW (dialog), GTK_WIN_POS_CENTER_ON_PARENT);
	}

	burner_dialog_button_image(gtk_dialog_get_action_area(GTK_DIALOG (dialog)));
	gtk_dialog_run (GTK_DIALOG (dialog));
	gtk_widget_destroy (GTK_WIDGET (dialog));
	priv->tool_dialog = NULL;
}

static void
on_integrity_check_cb (GtkAction *action, BurnerApp *app)
{
	burner_app_check (app, NULL, FALSE);
}

static void
burner_app_current_toplevel_destroyed (GtkWidget *widget,
					BurnerApp *app)
{
	BurnerAppPrivate *priv;

	priv = BURNER_APP_PRIVATE (app);
	if (priv->mainwin_running)
		gtk_widget_show (GTK_WIDGET (priv->mainwin));
}

void
burner_app_set_toplevel (BurnerApp *app, GtkWindow *window)
{
	BurnerAppPrivate *priv;

	priv = BURNER_APP_PRIVATE (app);

	if (!priv->mainwin_running) {
		if (priv->parent) {
			gtk_widget_realize (GTK_WIDGET (window));
			gdk_window_set_transient_for (gtk_widget_get_window (GTK_WIDGET (window)), priv->parent);
		}
		else {
			gtk_window_set_skip_taskbar_hint (GTK_WINDOW (window), FALSE);
			gtk_window_set_skip_pager_hint (GTK_WINDOW (window), FALSE);
			gtk_window_set_position (GTK_WINDOW (window), GTK_WIN_POS_CENTER);
		}
	}
	else {
		/* hide main dialog if it is shown */
//		gtk_widget_hide (GTK_WIDGET (priv->mainwin));

//		gtk_window_set_skip_taskbar_hint (GTK_WINDOW (window), FALSE);
//		gtk_window_set_skip_pager_hint (GTK_WINDOW (window), FALSE);
		gtk_window_set_skip_taskbar_hint (GTK_WINDOW (window), TRUE);
		gtk_window_set_skip_pager_hint (GTK_WINDOW (window), TRUE);
		gtk_window_set_position (GTK_WINDOW (window), GTK_WIN_POS_CENTER);
	}

	gtk_widget_show (GTK_WIDGET (window));
	g_signal_connect (window,
			  "destroy",
			  G_CALLBACK (burner_app_current_toplevel_destroyed),
			  app);
}

static void
on_prefs_cb (GtkAction *action, BurnerApp *app)
{
	GtkWidget *dialog;
	GtkWidget *toplevel;
	BurnerAppPrivate *priv;

	priv = BURNER_APP_PRIVATE (app);

	dialog = burner_pref_new ();

	/* FIXME! This is a bad idea and needs fixing */
	toplevel = gtk_widget_get_toplevel (GTK_WIDGET (priv->mainwin));

	gtk_window_set_transient_for (GTK_WINDOW (dialog), GTK_WINDOW (toplevel));
	gtk_window_set_modal (GTK_WINDOW (dialog), TRUE);
	gtk_window_set_position (GTK_WINDOW (dialog), GTK_WIN_POS_CENTER_ON_PARENT);

	gtk_widget_show_all (dialog);
	burner_dialog_button_image(gtk_dialog_get_action_area(GTK_DIALOG (dialog)));
	gtk_dialog_run (GTK_DIALOG (dialog));
	gtk_widget_destroy (dialog);
}

static void
on_about_cb (GtkAction *action, BurnerApp *app)
{
	const gchar *authors[] = {
		"Philippe Rouquier <bonfire-app@wanadoo.fr>",
		"Luis Medinas <lmedinas@gmail.com>",
		NULL
	};

	const gchar *documenters[] = {
		"Phil Bull <philbull@gmail.com>",
		"Milo Casagrande <milo_casagrande@yahoo.it>",
		"Andrew Stabeno <stabeno@gmail.com>",
		NULL
	};

	const gchar *license_part[] = {
		N_("Burner is free software; you can redistribute "
		   "it and/or modify it under the terms of the GNU "
		   "General Public License as published by the Free "
		   "Software Foundation; either version 2 of the "
		   "License, or (at your option) any later version."),
                N_("Burner is distributed in the hope that it will "
		   "be useful, but WITHOUT ANY WARRANTY; without even "
		   "the implied warranty of MERCHANTABILITY or FITNESS "
		   "FOR A PARTICULAR PURPOSE.  See the GNU General "
		   "Public License for more details."),
                N_("You should have received a copy of the GNU General "
		   "Public License along with Burner; if not, write "
		   "to the Free Software Foundation, Inc., "
                   "51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA"),
		NULL
        };

	gchar  *license, *comments;
	BurnerAppPrivate *priv;

	priv = BURNER_APP_PRIVATE (app);

	comments = g_strdup (_("A simple to use CD/DVD burning application for GNOME"));

	license = g_strjoin ("\n\n",
                             _(license_part[0]),
                             _(license_part[1]),
                             _(license_part[2]),
			     NULL);

	/* This can only be shown from the main window so no need for toplevel */
	gtk_show_about_dialog (GTK_WINDOW (GTK_WIDGET (priv->mainwin)),
			       "program-name", _("Burner"),
			       "comments", comments,
			       "version", VERSION,
			       "copyright", "Copyright © 2005-2010 Philippe Rouquier",
			       "authors", authors,
			       "documenters", documenters,
			       "website", "http://www.gnome.org/projects/burner",
			       "website-label", _("Burner Homepage"),
			       "license", license,
			       "wrap-license", TRUE,
			       "logo-icon-name", "burner",
			       /* Translators: This is a special message that shouldn't be translated
                                 * literally. It is used in the about box to give credits to
                                 * the translators.
                                 * Thus, you should translate it to your name and email address.
                                 * You should also include other translators who have contributed to
                                 * this translation; in that case, please write each of them on a separate
                                 * line seperated by newlines (\n).
                                 */
                               "translator-credits", _("translator-credits"),
			       NULL);

	g_free (comments);
	g_free (license);
}

static void
on_help_cb (GtkAction *action, BurnerApp *app)
{
	GError *error = NULL;
	BurnerAppPrivate *priv;

	priv = BURNER_APP_PRIVATE (app);

	gtk_show_uri (NULL, "help:burner", gtk_get_current_event_time (), &error);
   	if (error) {
		GtkWidget *d;
        
		d = gtk_message_dialog_new (GTK_WINDOW (priv->mainwin),
					    GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
					    GTK_MESSAGE_ERROR, GTK_BUTTONS_OK,
					    "%s", error->message);
		burner_message_title(d);

		burner_dialog_button_image(gtk_dialog_get_action_area(GTK_DIALOG (d)));
		gtk_dialog_run (GTK_DIALOG(d));
		gtk_widget_destroy (d);
		g_error_free (error);
		error = NULL;
	}
}

static gboolean
on_window_state_changed_cb (GtkWidget *widget,
			    GdkEventWindowState *event,
			    BurnerApp *app)
{
	BurnerAppPrivate *priv;

	priv = BURNER_APP_PRIVATE (app);

	if (event->new_window_state & GDK_WINDOW_STATE_MAXIMIZED) {
		priv->is_maximized = 1;
		burner_setting_set_value (burner_setting_get_default (),
		                           BURNER_SETTING_WIN_MAXIMIZED,
		                           GINT_TO_POINTER (1));
	}
	else {
		priv->is_maximized = 0;
		burner_setting_set_value (burner_setting_get_default (),
		                           BURNER_SETTING_WIN_MAXIMIZED,
		                           GINT_TO_POINTER (0));
	}

	return FALSE;
}

static gboolean
on_configure_event_cb (GtkWidget *widget,
		       GdkEventConfigure *event,
		       BurnerApp *app)
{
	BurnerAppPrivate *priv;

	priv = BURNER_APP_PRIVATE (app);

	if (!priv->is_maximized) {
		burner_setting_set_value (burner_setting_get_default (),
		                           BURNER_SETTING_WIN_WIDTH,
		                           GINT_TO_POINTER (event->width));
		burner_setting_set_value (burner_setting_get_default (),
		                           BURNER_SETTING_WIN_HEIGHT,
		                           GINT_TO_POINTER (event->height));
	}

	return FALSE;
}

gboolean
burner_app_open_project (BurnerApp *app,
			  BurnerDrive *burner,
                          const gchar *uri,
                          gboolean is_playlist,
                          gboolean warn_user,
                          gboolean burn)
{
	BurnerSessionCfg *session;

	session = burner_session_cfg_new ();

#ifdef BUILD_PLAYLIST

	if (is_playlist) {
		if (!burner_project_open_audio_playlist_project (uri,
								  BURNER_BURN_SESSION (session),
								  warn_user))
			return FALSE;
	}
	else

#endif
	
	if (!burner_project_open_project_xml (uri,
					       BURNER_BURN_SESSION (session),
					       warn_user))
		return FALSE;

	burner_app_process_session (app, session, burn);
	g_object_unref (session);

	return TRUE;
}

static gboolean
burner_app_open_by_mime (BurnerApp *app,
                          const gchar *uri,
                          const gchar *mime,
                          gboolean warn_user)
{
	if (!mime) {
		/* that can happen when the URI could not be identified */
		return FALSE;
	}

	/* When our files/description of x-burner mime type is not properly 
	 * installed, it's returned as application/xml, so check that too. */
	if (!strcmp (mime, "application/x-burner")
	||  !strcmp (mime, "application/xml"))
		return burner_app_open_project (app,
						 NULL,
						 uri,
						 FALSE,
						 warn_user,
						 FALSE);

#ifdef BUILD_PLAYLIST

	else if (!strcmp (mime, "audio/x-scpls")
	     ||  !strcmp (mime, "audio/x-ms-asx")
	     ||  !strcmp (mime, "audio/x-mp3-playlist")
	     ||  !strcmp (mime, "audio/x-mpegurl"))
		return burner_app_open_project (app,
						 NULL,
						 uri,
						 TRUE,
						 warn_user,
						 FALSE);

#endif

	else if (!strcmp (mime, "application/x-cd-image")
	     ||  !strcmp (mime, "application/x-cdrdao-toc")
	     ||  !strcmp (mime, "application/x-toc")
	     ||  !strcmp (mime, "application/x-cue")) {
		burner_app_image (app, NULL, uri, FALSE);
		return TRUE;
	}

	return FALSE;
}

static gboolean
burner_app_open_uri_file (BurnerApp *app,
                           GFile *file,
                           GFileInfo *info,
                           gboolean warn_user)
{
	BurnerProjectType type;
	gchar *uri = NULL;

	/* if that's a symlink, redo it on its target to get the real mime type
	 * that usually also depends on the extension of the target:
	 * ex: an iso file with the extension .iso will be seen as octet-stream
	 * if the symlink hasn't got any extention at all */
	while (g_file_info_get_is_symlink (info)) {
		const gchar *target;
		GFileInfo *tmp_info;
		GFile *tmp_file;
		GError *error = NULL;

		target = g_file_info_get_symlink_target (info);
		if (!g_path_is_absolute (target)) {
			gchar *parent;
			gchar *tmp;

			tmp = g_file_get_path (file);
			parent = g_path_get_dirname (tmp);
			g_free (tmp);

			target = g_build_filename (parent, target, NULL);
			g_free (parent);
		}

		tmp_file = g_file_new_for_commandline_arg (target);
		tmp_info = g_file_query_info (tmp_file,
					      G_FILE_ATTRIBUTE_STANDARD_CONTENT_TYPE ","
					      G_FILE_ATTRIBUTE_STANDARD_IS_SYMLINK ","
					      G_FILE_ATTRIBUTE_STANDARD_SYMLINK_TARGET,
					      G_FILE_QUERY_INFO_NOFOLLOW_SYMLINKS,
					      NULL,
					      &error);
		if (!tmp_info) {
			g_object_unref (tmp_file);
			break;
		}

		g_object_unref (info);
		g_object_unref (file);

		info = tmp_info;
		file = tmp_file;
	}

	uri = g_file_get_uri (file);
	if (g_file_query_exists (file, NULL)
	&&  g_file_info_get_content_type (info)) {
		const gchar *mime;

		mime = g_file_info_get_content_type (info);
	  	type = burner_app_open_by_mime (app, uri, mime, warn_user);
        } 
	else if (warn_user) {
		gchar *string;

		string = g_strdup_printf (_("The project \"%s\" does not exist"), uri);
		burner_app_alert (app,
				   _("Error while loading the project"),
				   string,
				   GTK_MESSAGE_ERROR);
		g_free (string);

		type = BURNER_PROJECT_TYPE_INVALID;
	}
	else
		type = BURNER_PROJECT_TYPE_INVALID;

	g_free (uri);
	return (type != BURNER_PROJECT_TYPE_INVALID);
}

gboolean
burner_app_open_uri (BurnerApp *app,
                      const gchar *uri_arg,
                      gboolean warn_user)
{
	GFile *file;
	GFileInfo *info;
	gboolean retval;

	/* FIXME: make that asynchronous */
	/* NOTE: don't follow symlink because we want to identify them */
	file = g_file_new_for_commandline_arg (uri_arg);
	if (!file)
		return FALSE;

	info = g_file_query_info (file,
				  G_FILE_ATTRIBUTE_STANDARD_CONTENT_TYPE ","
				  G_FILE_ATTRIBUTE_STANDARD_IS_SYMLINK ","
				  G_FILE_ATTRIBUTE_STANDARD_SYMLINK_TARGET,
				  G_FILE_QUERY_INFO_NOFOLLOW_SYMLINKS,
				  NULL,
				  NULL);

	if (!info) {
		g_object_unref (file);
		return FALSE;
	}

	retval = burner_app_open_uri_file (app, file, info, warn_user);

	g_object_unref (file);
	g_object_unref (info);

	return retval;
}

gboolean
burner_app_open_uri_drive_detection (BurnerApp *app,
                                      BurnerDrive *burner,
                                      const gchar *uri_arg,
                                      const gchar *cover_project,
                                      gboolean burn_immediately)
{
	gchar *uri;
	GFile *file;
	GFileInfo *info;
	gboolean retval = FALSE;

	file = g_file_new_for_commandline_arg (uri_arg);
	if (!file)
		return FALSE;

	/* Note: if the path is the path of a mounted volume the uri returned
	 * will be entirely different like if /path/to/somewhere is where
	 * an audio CD is mounted will return cdda://sr0/ */
	uri = g_file_get_uri (file);
	info = g_file_query_info (file,
	                          G_FILE_ATTRIBUTE_STANDARD_TYPE ","
	                          G_FILE_ATTRIBUTE_STANDARD_CONTENT_TYPE ","
				  G_FILE_ATTRIBUTE_STANDARD_IS_SYMLINK ","
				  G_FILE_ATTRIBUTE_STANDARD_SYMLINK_TARGET,
				  G_FILE_QUERY_INFO_NOFOLLOW_SYMLINKS,
				  NULL,
				  NULL);
	if (!info) {
		g_object_unref (file);
		g_free (uri);
		return FALSE;
	}

	if (g_file_info_get_file_type (info) == G_FILE_TYPE_SPECIAL) {
		/* It could be a block device, try */
		if (!g_strcmp0 (g_file_info_get_content_type (info), "inode/blockdevice")) {
			gchar *device;
			BurnerMedia media;
			BurnerDrive *drive;
			BurnerMedium *medium;
			BurnerMediumMonitor *monitor;

			g_object_unref (info);
			g_free (uri);

			monitor = burner_medium_monitor_get_default ();
			while (burner_medium_monitor_is_probing (monitor))
				sleep (1);

			device = g_file_get_path (file);
			drive = burner_medium_monitor_get_drive (monitor, device);
			g_object_unref (monitor);

			if (!drive) {
				/* This is not a known optical drive to us. */
				g_object_unref (file);
				return FALSE;
			}

			medium = burner_drive_get_medium (drive);
			if (!medium) {
				g_object_unref (file);
				g_object_unref (drive);
				return FALSE;
			}

			media = burner_medium_get_status (medium);
			if (BURNER_MEDIUM_IS (media, BURNER_MEDIUM_BLANK)) {
				/* This medium is blank so it rules out blanking
				 * copying, checksumming. Open a data project. */
				g_object_unref (file);
				g_object_unref (drive);
				return FALSE;
			}
			g_object_unref (drive);

			/* It seems that we are expected to copy the disc */
			device = g_strdup (g_file_get_path (file));
			g_object_unref (file);
			burner_app_copy_disc (app,
					       burner,
					       device,
					       cover_project,
					       burn_immediately != 0);
			g_free (device);
			return TRUE;
		}

		/* The rest are unsupported */
	}
	else if (g_str_has_prefix (uri, "cdda:")) {
		GFile *child;
		gchar *device;
		BurnerMediumMonitor *monitor;

		/* Make sure we are talking of the root */
		child = g_file_get_parent (file);
		if (child) {
			g_object_unref (child);
			g_object_unref (info);
			g_object_unref (file);
			g_free (uri);
			return FALSE;
		}

		/* We need to wait for the monitor to be ready */
		monitor = burner_medium_monitor_get_default ();
		while (burner_medium_monitor_is_probing (monitor))
			sleep (1);
		g_object_unref (monitor);

		if (g_str_has_suffix (uri, "/"))
			device = g_strdup_printf ("/dev/%.*s",
				                  (int) (strrchr (uri, '/') - uri - 7),
				                  uri + 7);
		else
			device = g_strdup_printf ("/dev/%s", uri + 7);
		burner_app_copy_disc (app,
				       burner,
				       device,
				       cover_project,
				       burn_immediately != 0);
		g_free (device);

		retval = TRUE;
	}
	else if (g_str_has_prefix (uri, "file:/")
	     &&  g_file_info_get_file_type (info) == G_FILE_TYPE_DIRECTORY) {
		BurnerMediumMonitor *monitor;
		gchar *directory_path;
		GSList *drives;
		GSList *iter;

		/* Try to detect a mounted optical disc */
		monitor = burner_medium_monitor_get_default ();
		while (burner_medium_monitor_is_probing (monitor))
			sleep (1);

		/* Check if this is a mount point for an optical disc */
		directory_path = g_file_get_path (file);
		drives = burner_medium_monitor_get_drives (monitor, BURNER_DRIVE_TYPE_ALL_BUT_FILE);
		for (iter = drives; iter; iter = iter->next) {
			gchar *mountpoint;
			BurnerDrive *drive;
			BurnerMedium *medium;

			drive = iter->data;
			medium = burner_drive_get_medium (drive);
			mountpoint = burner_volume_get_mount_point (BURNER_VOLUME (medium), NULL);
			if (!mountpoint)
				continue;

			if (!g_strcmp0 (mountpoint, directory_path)) {
				g_free (mountpoint);
				burner_app_copy_disc (app,
						       burner,
						       burner_drive_get_device (drive),
						       cover_project,
						       burn_immediately != 0);
				retval = TRUE;
				break;
			}
			g_free (mountpoint);
		}
		g_slist_foreach (drives, (GFunc) g_object_unref, NULL);
		g_slist_free (drives);

		g_free (directory_path);
	}

	g_object_unref (info);
	g_object_unref (file);
	g_free (uri);
	return retval;
}

static void
burner_app_recent_open (GtkRecentChooser *chooser,
			 BurnerApp *app)
{
	gchar *uri;
    	const gchar *mime;
    	GtkRecentInfo *item;
	GtkRecentManager *manager;

	/* This is a workaround since following code doesn't work */
	/*
    	item = gtk_recent_chooser_get_current_item (GTK_RECENT_CHOOSER (chooser));
	if (!item)
		return;
	*/

	uri = gtk_recent_chooser_get_current_uri (GTK_RECENT_CHOOSER (chooser));
	if (!uri)
		return;

	manager = gtk_recent_manager_get_default ();
	item = gtk_recent_manager_lookup_item (manager, uri, NULL);

	if (!item) {
		g_free (uri);
		return;
	}

	mime = gtk_recent_info_get_mime_type (item);

	if (!mime) {
		g_free (uri);
		g_warning ("Unrecognized mime type");
		return;
	}

	/* Make sure it is no longer one shot */
	burner_app_open_by_mime (app,
	                          uri,
	                          mime,
	                          TRUE);
	gtk_recent_info_unref (item);
	g_free (uri);
}

static void
burner_app_add_recent (BurnerApp *app,
			GtkActionGroup *group)
{
	GtkRecentManager *recent;
	GtkRecentFilter *filter;
	GtkAction *action;

	recent = gtk_recent_manager_get_default ();
	action = gtk_recent_action_new_for_manager ("RecentProjects",
						    _("_Recent Projects"),
						    _("Display the projects recently opened"),
						    NULL,
						    recent);
	filter = gtk_recent_filter_new ();

	gtk_recent_filter_set_name (filter, _("_Recent Projects"));
	gtk_recent_filter_add_mime_type (filter, "application/x-burner");
	gtk_recent_filter_add_mime_type (filter, "application/x-cd-image");
	gtk_recent_filter_add_mime_type (filter, "application/x-cdrdao-toc");
	gtk_recent_filter_add_mime_type (filter, "application/x-toc");
	gtk_recent_filter_add_mime_type (filter, "application/x-cue");
	gtk_recent_filter_add_mime_type (filter, "audio/x-scpls");
	gtk_recent_filter_add_mime_type (filter, "audio/x-ms-asx");
	gtk_recent_filter_add_mime_type (filter, "audio/x-mp3-playlist");
	gtk_recent_filter_add_mime_type (filter, "audio/x-mpegurl");

	gtk_recent_chooser_add_filter (GTK_RECENT_CHOOSER (action), filter);
	gtk_recent_chooser_set_filter (GTK_RECENT_CHOOSER (action), filter);

	gtk_recent_chooser_set_local_only (GTK_RECENT_CHOOSER (action), TRUE);

	gtk_recent_chooser_set_limit (GTK_RECENT_CHOOSER (action), 5);

	gtk_recent_chooser_set_show_tips (GTK_RECENT_CHOOSER (action), TRUE);

	gtk_recent_chooser_set_show_icons (GTK_RECENT_CHOOSER (action), TRUE);

	gtk_recent_chooser_set_sort_type (GTK_RECENT_CHOOSER (action), GTK_RECENT_SORT_MRU);

	gtk_action_group_add_action (group, action);
	g_object_unref (action);
	g_signal_connect (action,
			  "item-activated",
			  G_CALLBACK (burner_app_recent_open),
			  app);
}

static void
burner_menu_item_selected_cb (GtkMenuItem *proxy,
			       BurnerApp *app)
{
	BurnerAppPrivate *priv;
	GtkAction *action;
	gchar *message;

	priv = BURNER_APP_PRIVATE (app);

	action = gtk_activatable_get_related_action (GTK_ACTIVATABLE (proxy));
	g_return_if_fail (action != NULL);

	g_object_get (G_OBJECT (action), "tooltip", &message, NULL);
	if (message) {
		gtk_statusbar_push (GTK_STATUSBAR (priv->statusbar2),
				    priv->tooltip_ctx,
				    message);
		g_free (message);

		gtk_statusbar_push (GTK_STATUSBAR (priv->statusbar1),
				    priv->tooltip_ctx,
				    "");
	}
}

static void
burner_menu_item_deselected_cb (GtkMenuItem *proxy,
				 BurnerApp *app)
{
	BurnerAppPrivate *priv;

	priv = BURNER_APP_PRIVATE (app);

	gtk_statusbar_pop (GTK_STATUSBAR (priv->statusbar2),
			   priv->tooltip_ctx);
	gtk_statusbar_pop (GTK_STATUSBAR (priv->statusbar1),
			   priv->tooltip_ctx);
}

static void
burner_connect_ui_manager_proxy_cb (GtkUIManager *manager,
				     GtkAction *action,
				     GtkWidget *proxy,
				     BurnerApp *app)
{
	if (!GTK_IS_MENU_ITEM (proxy))
		return;

	g_signal_connect (proxy,
			  "select",
			  G_CALLBACK (burner_menu_item_selected_cb),
			  app);
	g_signal_connect (proxy,
			  "deselect",
			  G_CALLBACK (burner_menu_item_deselected_cb),
			  app);
}

static void
burner_disconnect_ui_manager_proxy_cb (GtkUIManager *manager,
					GtkAction *action,
					GtkWidget *proxy,
					BurnerApp *app)
{
	if (!GTK_IS_MENU_ITEM (proxy))
		return;

	g_signal_handlers_disconnect_by_func (proxy,
					      G_CALLBACK (burner_menu_item_selected_cb),
					      app);
	g_signal_handlers_disconnect_by_func (proxy,
					      G_CALLBACK (burner_menu_item_deselected_cb),
					      app);
}

static void
burner_caps_changed_cb (BurnerPluginManager *manager,
			 BurnerApp *app)
{
	BurnerAppPrivate *priv;
	GtkWidget *widget;

	priv = BURNER_APP_PRIVATE (app);

	widget = gtk_ui_manager_get_widget (priv->manager, "/menubar/ToolMenu/Check");

	if (!burner_burn_library_can_checksum ())
		gtk_widget_set_sensitive (widget, FALSE);
	else
		gtk_widget_set_sensitive (widget, TRUE);
}

static gboolean drag_handle(GtkWidget *widget, GdkEventButton *event, GdkWindowEdge edge)
{
	if (event->button == 1) {//左键单击
		gtk_window_begin_move_drag(GTK_WINDOW(gtk_widget_get_toplevel(widget)), event->button, event->x_root, event->y_root, event->time);
	}
 
	return FALSE;
}

static void
on_minimize_cb(GtkButton *button,BurnerApp *app)
{
	BurnerAppPrivate *priv;

	priv = BURNER_APP_PRIVATE (app);

	gtk_window_iconify(priv->mainwin);
}

static void
on_maximize_cb(GtkButton *button,BurnerApp *app)
{
	BurnerAppPrivate *priv;

	priv = BURNER_APP_PRIVATE (app);
	printf("on_maximize_cb: %d \n",priv->is_maximized);
	if(priv->is_maximized == 0)
	{
		gtk_widget_show (maximize_bt);
		gtk_widget_hide (unmaximize_bt);
		gtk_window_maximize(priv->mainwin);
	}
	else
	{
		gtk_widget_show (unmaximize_bt);
		gtk_widget_hide (maximize_bt);
		gtk_window_unmaximize(priv->mainwin);
	}

}

void
burner_app_create_mainwin (BurnerApp *app)
{
	GtkWidget *hbox;
	GtkWidget *vbox;
	GtkWidget *menubar;
	GError *error = NULL;
	BurnerAppPrivate *priv;
	GtkAccelGroup *accel_group;
	GtkActionGroup *action_group;
	BurnerPluginManager *plugin_manager;
	GtkWidget *image;

	GtkWidget *minimize_bt;
	GtkWidget *close_bt;
	GtkWidget *alignment;
	GtkWidget *project_box;
	GtkWidget *separator;
	GtkWidget *table;
	GtkWidget *label;
	GtkWidget *main_title;
	int nb_rows = 1;
	gchar *string;
	int rows;
	int i;
	GdkRGBA contents_color;

	priv = BURNER_APP_PRIVATE (app);

	if (priv->mainwin)
		return;

	/* New window */
	priv->mainwin = gtk_window_new (GTK_WINDOW_TOPLEVEL);
	gtk_window_set_default_size(priv->mainwin,1000,750);
	gtk_container_set_border_width (GTK_CONTAINER (priv->mainwin), 2);
	gtk_style_context_add_class ( gtk_widget_get_style_context (priv->mainwin), "mainwin");
	gtk_window_set_decorated(priv->mainwin,FALSE);
	gtk_window_set_icon_name (GTK_WINDOW (priv->mainwin), "burner");

	g_signal_connect (G_OBJECT (priv->mainwin),
			  "delete-event",
			  G_CALLBACK (on_delete_cb),
			  app);
	g_signal_connect (G_OBJECT (priv->mainwin),
			  "destroy",
			  G_CALLBACK (on_destroy_cb),
			  app);

	/* contents */
	priv->contents = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
	gtk_style_context_add_class ( gtk_widget_get_style_context (priv->contents), "contents");
	gtk_widget_show (priv->contents);

	gtk_container_add (GTK_CONTAINER (priv->mainwin), priv->contents);

	/*main title */
	main_title = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
	gtk_style_context_add_class ( gtk_widget_get_style_context (main_title), "main_title");
	gtk_widget_set_size_request(GTK_WIDGET(main_title),1000,36);
	gtk_box_pack_start (GTK_BOX (priv->contents),main_title , FALSE, TRUE, 0);
	gtk_widget_show (main_title);
	gtk_widget_add_events(priv->mainwin, GDK_BUTTON_PRESS_MASK);
	g_signal_connect(G_OBJECT(priv->mainwin), "button-press-event", G_CALLBACK (drag_handle), NULL);
	image = gtk_image_new_from_icon_name ("burner", GTK_ICON_SIZE_SMALL_TOOLBAR);

	gtk_widget_show (image);
	gtk_misc_set_alignment (GTK_MISC (image), 0.5, 0.5);
 	gtk_box_pack_start (GTK_BOX (main_title), image, FALSE, FALSE, 10);

	label = gtk_label_new (_("Burner"));
	gtk_widget_show (label);
	gtk_misc_set_alignment (GTK_MISC (label), 0.5, 0.5);
//    gtk_misc_set_padding (GTK_MISC (label), 6.0, 0.0);
	gtk_box_pack_start (GTK_BOX (main_title), label, FALSE, FALSE, 0);

	vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
	gtk_widget_show (vbox);
	hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
	gtk_widget_show (hbox);
	gtk_box_pack_end (GTK_BOX (main_title), vbox, TRUE, TRUE, 0);
	alignment = gtk_alignment_new (1.0, 0.5, 0.0, 0.0);
	gtk_widget_show (alignment);
	gtk_container_add (GTK_CONTAINER (vbox), alignment);
	gtk_container_add (GTK_CONTAINER (alignment), hbox);

	/* maximize minimize and close */
	maximize_bt = gtk_button_new ();
	gtk_button_set_alignment(GTK_BUTTON(maximize_bt),0.5,0.5);
	gtk_widget_set_size_request(GTK_WIDGET(maximize_bt),45,30);
	if(priv->is_maximized == 1)
		gtk_widget_show (maximize_bt);
	else
		gtk_widget_hide (maximize_bt);
	gtk_style_context_add_class ( gtk_widget_get_style_context (maximize_bt), "maximize_bt");

	unmaximize_bt = gtk_button_new ();
	gtk_button_set_alignment(GTK_BUTTON(unmaximize_bt),0.5,0.5);
	gtk_widget_set_size_request(GTK_WIDGET(unmaximize_bt),45,30);
	if(priv->is_maximized == 0)
		gtk_widget_show (unmaximize_bt);
	else
		gtk_widget_hide (unmaximize_bt);
	gtk_style_context_add_class ( gtk_widget_get_style_context (unmaximize_bt), "unmaximize_bt");

	minimize_bt = gtk_button_new ();
	gtk_button_set_alignment(GTK_BUTTON(minimize_bt),0.5,0.5);
	gtk_widget_set_size_request(GTK_WIDGET(minimize_bt),45,30);
	gtk_widget_show (minimize_bt);
	gtk_style_context_add_class ( gtk_widget_get_style_context (minimize_bt), "minimize_bt");

	close_bt = gtk_button_new ();
	gtk_button_set_alignment(GTK_BUTTON(close_bt),0.5,0.5);
	gtk_widget_set_size_request(GTK_WIDGET(close_bt),45,30);
	gtk_widget_show (close_bt);
	gtk_style_context_add_class ( gtk_widget_get_style_context (close_bt), "close_bt");

	gtk_box_pack_start (GTK_BOX (hbox), minimize_bt, FALSE, TRUE, 2);
	gtk_box_pack_start (GTK_BOX (hbox), maximize_bt, FALSE, TRUE, 2);
 	gtk_box_pack_start (GTK_BOX (hbox), unmaximize_bt, FALSE, TRUE, 2);
	gtk_box_pack_end (GTK_BOX (hbox), close_bt, FALSE, TRUE, 2);

	g_signal_connect (minimize_bt,
		"clicked",
		G_CALLBACK (on_minimize_cb),
		app);
	g_signal_connect (maximize_bt,
		"clicked",
		G_CALLBACK (on_maximize_cb),
		app);
	g_signal_connect (unmaximize_bt,
		"clicked",
		G_CALLBACK (on_maximize_cb),
		app);
	g_signal_connect (close_bt,
		"clicked",
		G_CALLBACK (on_exit_cb),
		app);

	/* menu and toolbar */
	priv->manager = gtk_ui_manager_new ();
	g_signal_connect (priv->manager,
			  "connect-proxy",
			  G_CALLBACK (burner_connect_ui_manager_proxy_cb),
			  app);
	g_signal_connect (priv->manager,
			  "disconnect-proxy",
			  G_CALLBACK (burner_disconnect_ui_manager_proxy_cb),
			  app);

	action_group = gtk_action_group_new ("MenuActions");
	gtk_action_group_set_translation_domain (action_group, GETTEXT_PACKAGE);
	gtk_action_group_add_actions (action_group,
				      entries,
				      G_N_ELEMENTS (entries),
				      app);

	gtk_ui_manager_insert_action_group (priv->manager, action_group, 0);

	burner_app_add_recent (app, action_group);

	if (!gtk_ui_manager_add_ui_from_string (priv->manager, description, -1, &error)) {
		g_message ("building menus failed: %s", error->message);
		g_error_free (error);
	}

	menubar = gtk_ui_manager_get_widget (priv->manager, "/menubar");
	gtk_widget_set_size_request(GTK_WIDGET(menubar),1000,28);
	gtk_style_context_add_class ( gtk_widget_get_style_context (menubar), "main_menubar");
	gtk_box_pack_start (GTK_BOX (priv->contents), menubar, FALSE, FALSE, 0);

	/* window contents */
//	priv->projects = burner_project_manager_new ();
//	gtk_widget_show (priv->projects);

//	gtk_box_pack_start (GTK_BOX (priv->contents), priv->projects, TRUE, TRUE, 0);

	/* wenbo, left project */
	priv->contents2 = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
	gtk_widget_show (priv->contents2);
	gtk_style_context_add_class ( gtk_widget_get_style_context (priv->contents2), "contents2");
	gtk_box_pack_end (GTK_BOX (priv->contents),priv->contents2 , TRUE, TRUE, 0);


	project_box = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
	gtk_widget_show (project_box);
//    gtk_style_context_add_class ( gtk_widget_get_style_context (project_box), "project_left_box");
	gtk_box_pack_start (GTK_BOX (priv->contents2), project_box, FALSE, TRUE, 0);

//    separator = gtk_separator_new (GTK_ORIENTATION_VERTICAL);
//    gtk_widget_show (separator);
//    gtk_box_pack_start (GTK_BOX (priv->contents2), separator, FALSE, TRUE, 0);


	priv->contents3 = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
	gtk_widget_show (priv->contents3);
	gtk_box_pack_end (GTK_BOX (priv->contents2),priv->contents3 , TRUE, TRUE, 0);
	gtk_style_context_add_class ( gtk_widget_get_style_context (priv->contents3), "project_right_box");

	/* window contents */
	priv->projects = burner_project_manager_new ();
	gtk_widget_show (priv->projects);

	gtk_box_pack_start (GTK_BOX (priv->contents3), priv->projects, TRUE, TRUE, 0);

//    string = g_strdup_printf ("<span size='x-large'><b>%s</b></span>", _("Create a new project:"));
//    label = gtk_label_new (string);
//    g_free (string);

//    gtk_widget_show (label);
//    gtk_label_set_use_markup (GTK_LABEL (label), TRUE);
//    gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.0);
//    gtk_misc_set_padding (GTK_MISC (label), 6.0, 0.0);
//    gtk_box_pack_start (GTK_BOX (project_box), label, FALSE, TRUE, 0);

	/* get the number of rows */
	nb_items = sizeof (items) / sizeof (ItemDescription);
	rows = nb_items / nb_rows;
	if (nb_items % nb_rows)
		rows ++;

	table = gtk_table_new (rows, nb_rows, TRUE);
	gtk_container_set_border_width (GTK_CONTAINER (table), 1);
//    gtk_container_set_border_width (GTK_CONTAINER (table), 0);
//    gtk_style_context_add_class ( gtk_widget_get_style_context (table), "left_table");
	gtk_box_pack_start (GTK_BOX (project_box), table, FALSE, TRUE, 1);

	gtk_table_set_col_spacings (GTK_TABLE (table), 0);
	gtk_table_set_row_spacings (GTK_TABLE (table), 0);


	for (i = 0; i < nb_items; i ++) {
		widget[i] = burner_project_type_chooser_new_item (type, items + i);
		gtk_table_attach (GTK_TABLE (table),
			widget[i],
			i % nb_rows,
			i % nb_rows + 1,
			i / nb_rows,
			i / nb_rows + 1,
			GTK_EXPAND|GTK_FILL,
			GTK_FILL,
			0,
			0);
	}
	gtk_widget_show_all (table);

	/* status bar to display the size of selected files */
/*	hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
	gtk_widget_show (hbox);
	gtk_box_pack_end (GTK_BOX (priv->contents), hbox, FALSE, TRUE, 0);

	priv->statusbar2 = gtk_statusbar_new ();
	gtk_widget_show (priv->statusbar2);
	priv->tooltip_ctx = gtk_statusbar_get_context_id (GTK_STATUSBAR (priv->statusbar2), "tooltip_info");
	gtk_box_pack_start (GTK_BOX (hbox), priv->statusbar2, FALSE, TRUE, 0);

	priv->statusbar1 = gtk_statusbar_new ();
	gtk_widget_show (priv->statusbar1);
	gtk_box_pack_start (GTK_BOX (hbox), priv->statusbar1, FALSE, TRUE, 0);
*/
	statusbarbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
//      gtk_widget_show (statusbarbox);
	gtk_box_pack_end (GTK_BOX (priv->contents3), statusbarbox, FALSE, TRUE, 0);

	priv->statusbar2 = gtk_statusbar_new ();
	gtk_widget_show (priv->statusbar2);
	priv->tooltip_ctx = gtk_statusbar_get_context_id (GTK_STATUSBAR (priv->statusbar2), "tooltip_info");
	gtk_box_pack_start (GTK_BOX (statusbarbox), priv->statusbar2, FALSE, TRUE, 0);

	priv->statusbar1 = gtk_statusbar_new ();
	gtk_widget_show (priv->statusbar1);
	gtk_box_pack_start (GTK_BOX (statusbarbox), priv->statusbar1, FALSE, TRUE, 0);

	vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
	gtk_widget_show (vbox);
	gtk_box_pack_end (GTK_BOX (statusbarbox), vbox, TRUE, TRUE, 0);

//    gdk_rgba_parse(&contents_color,"red");
//    gtk_widget_override_background_color(GTK_WIDGET(vbox),GTK_STATE_NORMAL,&contents_color);
//    gtk_widget_set_size_request(GTK_WIDGET(statusbarbox),840,50);
	alignment = gtk_alignment_new (0.95, 0.5, 0.0, 0.0);
	gtk_widget_show (alignment);
	gtk_container_add (GTK_CONTAINER (vbox), alignment);
	gtk_container_add (GTK_CONTAINER (alignment), burn_button);
 
 
//    gtk_box_pack_end (GTK_BOX (statusbarbox), burn_button, FALSE, TRUE, 0);

	/* Update everything */
	burner_project_manager_register_ui (BURNER_PROJECT_MANAGER (priv->projects),
					     priv->manager);

	gtk_ui_manager_ensure_update (priv->manager);

	/* check if we can use checksums (we need plugins enabled) */
	if (!burner_burn_library_can_checksum ()) {
		GtkWidget *widget;

		widget = gtk_ui_manager_get_widget (priv->manager, "/menubar/ToolMenu/Check");
		gtk_widget_set_sensitive (widget, FALSE);
	}

	plugin_manager = burner_plugin_manager_get_default ();
	g_signal_connect (plugin_manager,
			  "caps-changed",
			  G_CALLBACK (burner_caps_changed_cb),
			  app);

	/* add accelerators */
	accel_group = gtk_ui_manager_get_accel_group (priv->manager);
	gtk_window_add_accel_group (GTK_WINDOW (priv->mainwin), accel_group);

	/* set up the window geometry */
	gtk_window_set_position (GTK_WINDOW (priv->mainwin),
				 GTK_WIN_POS_CENTER);

	g_signal_connect (priv->mainwin,
			  "window-state-event",
			  G_CALLBACK (on_window_state_changed_cb),
			  app);
	g_signal_connect (priv->mainwin,
			  "configure-event",
			  G_CALLBACK (on_configure_event_cb),
			  app);

	gtk_widget_realize (GTK_WIDGET (priv->mainwin));

	if (priv->parent) {
		gtk_window_set_modal (GTK_WINDOW (priv->mainwin), TRUE);
		gdk_window_set_transient_for (gtk_widget_get_window (GTK_WIDGET (priv->mainwin)), priv->parent);
	}

	burner_app_load_window_state (app);
}

static void
burner_app_activate (GApplication *gapp,
                      BurnerApp *app)
{
	BurnerAppPrivate *priv;

	priv = BURNER_APP_PRIVATE (app);

	/* Except if we are supposed to quit, show the window */
	if (priv->mainwin_running) {
		gtk_widget_show (priv->mainwin);
		gtk_window_present (GTK_WINDOW (priv->mainwin));
	}
}

gboolean
burner_app_run_mainwin (BurnerApp *app)
{
	BurnerAppPrivate *priv;

	priv = BURNER_APP_PRIVATE (app);

	if (!priv->mainwin)
		return FALSE;

	if (priv->mainwin_running)
		return TRUE;

	priv->mainwin_running = 1;
	gtk_widget_show (GTK_WIDGET (priv->mainwin));

	if (priv->gapp)
		g_signal_connect (priv->gapp,
				  "activate",
				  G_CALLBACK (burner_app_activate),
				  app);
	gtk_main ();
	return TRUE;
}

static GtkWindow *
burner_app_get_io_parent_window (gpointer user_data)
{
	BurnerAppPrivate *priv;

	priv = BURNER_APP_PRIVATE (user_data);
	if (!priv->mainwin) {
		if (priv->parent)
			return GTK_WINDOW (priv->parent);
	}
	else {
		GtkWidget *toplevel;

		toplevel = gtk_widget_get_toplevel (GTK_WIDGET (priv->mainwin));
		return GTK_WINDOW (toplevel);
	}

	return NULL;
}

static void
burner_app_init (BurnerApp *object)
{
	BurnerAppPrivate *priv;

	priv = BURNER_APP_PRIVATE (object);

	priv->mainwin = NULL;

	/* Load settings */
	priv->setting = burner_setting_get_default ();
	burner_setting_load (priv->setting);

	g_set_application_name (_("Disc Burner"));
	gtk_window_set_default_icon_name ("burner");

	burner_io_set_parent_window_callback (burner_app_get_io_parent_window, object);
}

static void
burner_app_finalize (GObject *object)
{
	BurnerAppPrivate *priv;

	priv = BURNER_APP_PRIVATE (object);

	burner_setting_save (priv->setting);
	g_object_unref (priv->setting);
	priv->setting = NULL;

	if (priv->saved_contents) {
		g_free (priv->saved_contents);
		priv->saved_contents = NULL;
	}

	G_OBJECT_CLASS (burner_app_parent_class)->finalize (object);
}
static void
burner_app_set_property (GObject *object,
                          guint prop_id,
                          const GValue *value,
                          GParamSpec *pspec)
{
	BurnerAppPrivate *priv;

	g_return_if_fail (BURNER_IS_APP (object));

	priv = BURNER_APP_PRIVATE (object);

	switch (prop_id)
	{
	case PROP_GAPP:
		priv->gapp = g_value_dup_object (value);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
		break;
	}
}

static void
burner_app_get_property (GObject *object,
			  guint prop_id,
			  GValue *value,
			  GParamSpec *pspec)
{
	BurnerAppPrivate *priv;

	g_return_if_fail (BURNER_IS_APP (object));

	priv = BURNER_APP_PRIVATE (object);

	switch (prop_id)
	{
	case PROP_GAPP:
		g_value_set_object (value, priv->gapp);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
		break;
	}
}

static void
burner_app_class_init (BurnerAppClass *klass)
{
	GObjectClass* object_class = G_OBJECT_CLASS (klass);

	g_type_class_add_private (klass, sizeof (BurnerAppPrivate));

	object_class->finalize = burner_app_finalize;
	object_class->set_property = burner_app_set_property;
	object_class->get_property = burner_app_get_property;

	g_object_class_install_property (object_class,
	                                 PROP_GAPP,
	                                 g_param_spec_object("gapp",
	                                                     "GApplication",
	                                                     "The GApplication object",
	                                                     G_TYPE_APPLICATION,
	                                                     G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
}

BurnerApp *
burner_app_new (GApplication *gapp)
{
	return g_object_new (BURNER_TYPE_APP,
	                     "gapp", gapp,
	                     NULL);
}
