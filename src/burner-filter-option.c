/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/*
 * burner
 * Copyright (C) Philippe Rouquier 2005-2008 <bonfire-app@wanadoo.fr>
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
#include <glib/gi18n-lib.h>
#include <gio/gio.h>

#include <gtk/gtk.h>

#include "burner-misc.h"

#include "burner-filter-option.h"
#include "burner-data-vfs.h"

typedef struct _BurnerFilterOptionPrivate BurnerFilterOptionPrivate;
struct _BurnerFilterOptionPrivate
{
	GSettings *settings;
};

#define BURNER_FILTER_OPTION_PRIVATE(o)  (G_TYPE_INSTANCE_GET_PRIVATE ((o), BURNER_TYPE_FILTER_OPTION, BurnerFilterOptionPrivate))

G_DEFINE_TYPE (BurnerFilterOption, burner_filter_option, GTK_TYPE_BOX);

static void
burner_filter_option_init (BurnerFilterOption *object)
{
	gchar *string;
	GtkWidget *frame;
	GtkWidget *button_sym;
	GtkWidget *button_broken;
	GtkWidget *button_hidden;
	BurnerFilterOptionPrivate *priv;

	priv = BURNER_FILTER_OPTION_PRIVATE (object);

	priv->settings = g_settings_new (BURNER_SCHEMA_FILTER);

	gtk_orientable_set_orientation (GTK_ORIENTABLE (object), GTK_ORIENTATION_VERTICAL);

	/* filter hidden files */
	button_hidden = gtk_check_button_new_with_mnemonic (_("Filter _hidden files"));
	g_settings_bind (priv->settings, BURNER_PROPS_FILTER_HIDDEN,
	                 button_hidden, "active",
	                 G_SETTINGS_BIND_DEFAULT);
	gtk_style_context_add_class ( gtk_widget_get_style_context (button_hidden), "filter_option_bt1");
	gtk_widget_show (button_hidden);

	/* replace symlink */
	button_sym = gtk_check_button_new_with_mnemonic (_("Re_place symbolic links"));
	g_settings_bind (priv->settings, BURNER_PROPS_FILTER_REPLACE_SYMLINK,
	                 button_sym, "active",
	                 G_SETTINGS_BIND_DEFAULT);
	gtk_style_context_add_class ( gtk_widget_get_style_context (button_sym), "filter_option_bt2");
	gtk_widget_show (button_sym);

	/* filter broken symlink button */
	button_broken = gtk_check_button_new_with_mnemonic (_("Filter _broken symbolic links"));
	g_settings_bind (priv->settings, BURNER_PROPS_FILTER_BROKEN,
	                 button_broken, "active",
	                 G_SETTINGS_BIND_DEFAULT);
	gtk_style_context_add_class ( gtk_widget_get_style_context (button_broken), "filter_option_bt3");
	gtk_widget_show (button_broken);

//	string = g_strdup_printf ("<b>%s</b>", _("Filtering options"));
//	frame = burner_utils_pack_properties (string,
	frame = burner_utils_pack_properties (NULL,
					       button_sym,
					       button_broken,
					       button_hidden,
					       NULL);
	g_free (string);

	gtk_box_pack_start (GTK_BOX (object),
			    frame,
			    FALSE,
			    FALSE,
			    0);
}

static void
burner_filter_option_finalize (GObject *object)
{
	BurnerFilterOptionPrivate *priv;

	priv = BURNER_FILTER_OPTION_PRIVATE (object);

	if (priv->settings) {
		g_object_unref (priv->settings);
		priv->settings = NULL;
	}

	G_OBJECT_CLASS (burner_filter_option_parent_class)->finalize (object);
}

static void
burner_filter_option_class_init (BurnerFilterOptionClass *klass)
{
	GObjectClass* object_class = G_OBJECT_CLASS (klass);

	g_type_class_add_private (klass, sizeof (BurnerFilterOptionPrivate));

	object_class->finalize = burner_filter_option_finalize;
}

GtkWidget *
burner_filter_option_new (void)
{
	return g_object_new (BURNER_TYPE_FILTER_OPTION, NULL);
}
