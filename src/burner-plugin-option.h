/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/*
 * burner
 * Copyright (C) Philippe Rouquier 2007-2008 <bonfire-app@wanadoo.fr>
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

#ifndef _BURNER_PLUGIN_OPTION_H_
#define _BURNER_PLUGIN_OPTION_H_

#include <glib-object.h>

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define BURNER_TYPE_PLUGIN_OPTION             (burner_plugin_option_get_type ())
#define BURNER_PLUGIN_OPTION(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), BURNER_TYPE_PLUGIN_OPTION, BurnerPluginOption))
#define BURNER_PLUGIN_OPTION_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), BURNER_TYPE_PLUGIN_OPTION, BurnerPluginOptionClass))
#define BURNER_IS_PLUGIN_OPTION(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BURNER_TYPE_PLUGIN_OPTION))
#define BURNER_IS_PLUGIN_OPTION_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), BURNER_TYPE_PLUGIN_OPTION))
#define BURNER_PLUGIN_OPTION_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), BURNER_TYPE_PLUGIN_OPTION, BurnerPluginOptionClass))

typedef struct _BurnerPluginOptionClass BurnerPluginOptionClass;
typedef struct _BurnerPluginOption BurnerPluginOption;

struct _BurnerPluginOptionClass
{
	GtkDialogClass parent_class;
};

struct _BurnerPluginOption
{
	GtkDialog parent_instance;
};

GType burner_plugin_option_get_type (void) G_GNUC_CONST;

GtkWidget *
burner_plugin_option_new (void);

void
burner_plugin_option_set_plugin (BurnerPluginOption *dialog,
				  BurnerPlugin *plugin);

G_END_DECLS

#endif /* _BURNER_PLUGIN_OPTION_H_ */
