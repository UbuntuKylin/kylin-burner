/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/*
 * Libburner-burn
 * Copyright (C) Canonical 2010
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
#include <glib/gi18n.h>

#include <gtk/gtk.h>

#include <libappindicator/app-indicator.h>

#include "burn-basics.h"
#include "burner-app-indicator.h"

static void burner_app_indicator_class_init (BurnerAppIndicatorClass *klass);
static void burner_app_indicator_init (BurnerAppIndicator *sp);
static void burner_app_indicator_finalize (GObject *object);

static void
burner_app_indicator_cancel_cb (GtkAction *action, BurnerAppIndicator *indicator);

static void
burner_app_indicator_show_cb (GtkAction *action, BurnerAppIndicator *indicator);

struct BurnerAppIndicatorPrivate {
	AppIndicator *indicator;
	BurnerBurnAction action;
	gchar *action_string;

	GtkUIManager *manager;

	int rounded_percent;
	int percent;
};

typedef enum {
	CANCEL_SIGNAL,
	CLOSE_AFTER_SIGNAL,
	SHOW_DIALOG_SIGNAL,
	LAST_SIGNAL
} BurnerAppIndicatorSignalType;

static guint burner_app_indicator_signals[LAST_SIGNAL] = { 0 };
static GObjectClass *parent_class = NULL;

static GtkActionEntry entries[] = {
	{"ContextualMenu", NULL, N_("Menu")},
	{"Progress", NULL, "Progress"},
	{"Cancel", GTK_STOCK_CANCEL, NULL, NULL, N_("Cancel ongoing burning"),
	 G_CALLBACK (burner_app_indicator_cancel_cb)},
};

static GtkToggleActionEntry toggle_entries[] = {
	{"Show", NULL, N_("Show _Dialog"), NULL, N_("Show dialog"),
	 G_CALLBACK (burner_app_indicator_show_cb), TRUE,},
};

static const char *description = {
	"<ui>"
	"<popup action='ContextMenu'>"
		"<menuitem action='Progress'/>"
		"<separator/>"
		"<menuitem action='Show'/>"
		"<separator/>"
		"<menuitem action='Cancel'/>"
	"</popup>"
	"</ui>"
};

static const gchar *accessible_desc = NULL;

G_DEFINE_TYPE (BurnerAppIndicator, burner_app_indicator, G_TYPE_OBJECT);

static void
burner_app_indicator_class_init (BurnerAppIndicatorClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS(klass);

	parent_class = g_type_class_peek_parent(klass);
	object_class->finalize = burner_app_indicator_finalize;

	burner_app_indicator_signals[SHOW_DIALOG_SIGNAL] =
	    g_signal_new ("show_dialog",
			  G_OBJECT_CLASS_TYPE (object_class),
			  G_SIGNAL_RUN_FIRST | G_SIGNAL_ACTION,
			  G_STRUCT_OFFSET (BurnerAppIndicatorClass,
					   show_dialog), NULL, NULL,
			  g_cclosure_marshal_VOID__BOOLEAN,
			  G_TYPE_NONE,
			  1,
			  G_TYPE_BOOLEAN);
	burner_app_indicator_signals[CANCEL_SIGNAL] =
	    g_signal_new ("cancel",
			  G_OBJECT_CLASS_TYPE (object_class),
			  G_SIGNAL_RUN_FIRST | G_SIGNAL_ACTION,
			  G_STRUCT_OFFSET (BurnerAppIndicatorClass,
					   cancel), NULL, NULL,
			  g_cclosure_marshal_VOID__VOID,
			  G_TYPE_NONE,
			  0);
	burner_app_indicator_signals[CLOSE_AFTER_SIGNAL] =
	    g_signal_new ("close_after",
			  G_OBJECT_CLASS_TYPE (object_class),
			  G_SIGNAL_RUN_FIRST | G_SIGNAL_ACTION,
			  G_STRUCT_OFFSET (BurnerAppIndicatorClass,
					   close_after), NULL, NULL,
			  g_cclosure_marshal_VOID__BOOLEAN,
			  G_TYPE_NONE,
			  1,
			  G_TYPE_BOOLEAN);
}

static GtkWidget *
burner_app_indicator_build_menu (BurnerAppIndicator *indicator)
{
	GtkActionGroup *action_group;
	GtkWidget *menu = NULL,
		*menu_item = NULL;
	GError *error = NULL;

	action_group = gtk_action_group_new ("MenuAction");
	gtk_action_group_set_translation_domain (action_group, GETTEXT_PACKAGE);
	gtk_action_group_add_actions (action_group,
				      entries,
				      G_N_ELEMENTS (entries),
				      indicator);
	gtk_action_group_add_toggle_actions (action_group,
					     toggle_entries,
					     G_N_ELEMENTS (toggle_entries),
					     indicator);

	indicator->priv->manager = gtk_ui_manager_new ();
	gtk_ui_manager_insert_action_group (indicator->priv->manager,
					    action_group,
					    0);

	if (!gtk_ui_manager_add_ui_from_string (indicator->priv->manager,
						description,
						-1,
						&error)) {
		g_message ("building menus failed: %s", error->message);
		g_error_free (error);
	} else {
		menu = gtk_ui_manager_get_widget (indicator->priv->manager, "/ContextMenu");
		menu_item = gtk_ui_manager_get_widget (indicator->priv->manager, "/ContextMenu/Progress");
		gtk_widget_set_sensitive (menu_item, FALSE);

		menu_item = gtk_ui_manager_get_widget (indicator->priv->manager, "/ContextMenu/Cancel");
		gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (menu_item), GTK_WIDGET (gtk_image_new ()));
	}

	return menu;
}

void
burner_app_indicator_hide (BurnerAppIndicator *indicator)
{
	app_indicator_set_status (indicator->priv->indicator, APP_INDICATOR_STATUS_PASSIVE);
}

static void
burner_app_indicator_init (BurnerAppIndicator *obj)
{
	GtkWidget *indicator_menu;

	obj->priv = g_new0 (BurnerAppIndicatorPrivate, 1);
	indicator_menu =  burner_app_indicator_build_menu (obj);

	if (indicator_menu != NULL) {
		obj->priv->indicator = app_indicator_new_with_path ("burner",
								    "burner-disc-00",
								    APP_INDICATOR_CATEGORY_APPLICATION_STATUS,
								    BURNER_DATADIR "/icons");

		app_indicator_set_status (obj->priv->indicator,
					  APP_INDICATOR_STATUS_ACTIVE);

		app_indicator_set_menu (obj->priv->indicator, GTK_MENU (indicator_menu));
	}
}

static void
burner_app_indicator_finalize (GObject *object)
{
	BurnerAppIndicator *cobj;

	cobj = BURNER_APPINDICATOR (object);

	if (cobj->priv->action_string) {
		g_free (cobj->priv->action_string);
		cobj->priv->action_string = NULL;
	}

	g_object_unref (cobj->priv->indicator);
	g_free (cobj->priv);
	G_OBJECT_CLASS (parent_class)->finalize (object);
}

BurnerAppIndicator *
burner_app_indicator_new ()
{
	BurnerAppIndicator *obj;

	obj = BURNER_APPINDICATOR (g_object_new (BURNER_TYPE_APPINDICATOR, NULL));

	return obj;
}

static void
burner_app_indicator_set_progress_menu_text (BurnerAppIndicator *indicator,
					      glong remaining)
{
	gchar *text;
	GtkWidget *progress;
	const gchar *action_string;

	if (!indicator->priv->action_string)
		action_string = burner_burn_action_to_string (indicator->priv->action);
	else
		action_string = indicator->priv->action_string;

	if (remaining > 0) {
		gchar *remaining_string;

		remaining_string = burner_units_get_time_string ((double) remaining * 1000000000, TRUE, FALSE);
		text = g_strdup_printf (_("%s, %d%% done, %s remaining"),
					action_string,
					indicator->priv->percent,
					remaining_string);
		g_free (remaining_string);
	}
	else if (indicator->priv->percent > 0)
		text = g_strdup_printf (_("%s, %d%% done"),
					action_string,
					indicator->priv->percent);
	else
		text = g_strdup (action_string);

	/* Set the text on the menu */
	progress = gtk_ui_manager_get_widget (indicator->priv->manager,
					      "/ContextMenu/Progress");
	gtk_menu_item_set_label (GTK_MENU_ITEM (progress), text);
	accessible_desc = g_strdup_printf(_("Burner Disc Burner: %s"), text);
	g_free (text);
}

void
burner_app_indicator_set_action (BurnerAppIndicator *indicator,
				  BurnerBurnAction action,
				  const gchar *string)
{
	indicator->priv->action = action;
	if (indicator->priv->action_string)
		g_free (indicator->priv->action_string);

	if (string)
		indicator->priv->action_string = g_strdup (string);
	else
		indicator->priv->action_string = NULL;

	burner_app_indicator_set_progress_menu_text (indicator, -1);
}

void
burner_app_indicator_set_progress (BurnerAppIndicator *indicator,
				    gdouble fraction,
				    glong remaining)
{
	gint percent;
	gint remains;
	gchar *icon_name;

	percent = fraction * 100;
	indicator->priv->percent = percent;

	/* set the tooltip */
	burner_app_indicator_set_progress_menu_text (indicator, remaining);

	/* change image if need be */
	remains = percent % 5;
	if (remains > 3)
		percent += 5 - remains;
	else
		percent -= remains;

	if (indicator->priv->rounded_percent == percent
	||  percent < 0 || percent > 100)
		return;

	indicator->priv->rounded_percent = percent;

	icon_name = g_strdup_printf ("burner-disc-%02i", percent);
	app_indicator_set_icon_full(indicator->priv->indicator,
			       icon_name, accessible_desc);
	g_free (icon_name);
}

static void
burner_app_indicator_change_show_dialog_state (BurnerAppIndicator *indicator)
{
	GtkAction *action;
	gboolean active;

	/* update menu */
	action = gtk_ui_manager_get_action (indicator->priv->manager, "/ContextMenu/Show");
	active = gtk_toggle_action_get_active (GTK_TOGGLE_ACTION (action));

	/* signal show dialog was requested the dialog again */
	g_signal_emit (indicator,
		       burner_app_indicator_signals [SHOW_DIALOG_SIGNAL],
		       0,
		       active);
}

static void
burner_app_indicator_cancel_cb (GtkAction *action, BurnerAppIndicator *indicator)
{
	g_signal_emit (indicator,
		       burner_app_indicator_signals [CANCEL_SIGNAL],
		       0);
}

static void
burner_app_indicator_show_cb (GtkAction *action, BurnerAppIndicator *indicator)
{
	burner_app_indicator_change_show_dialog_state (indicator);
}

void
burner_app_indicator_set_show_dialog (BurnerAppIndicator *indicator, gboolean show)
{
	GtkAction *action;

	/* update menu */
	action = gtk_ui_manager_get_action (indicator->priv->manager, "/ContextMenu/Show");
	gtk_toggle_action_set_active (GTK_TOGGLE_ACTION (action), show);
}
