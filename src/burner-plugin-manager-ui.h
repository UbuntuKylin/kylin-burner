/*
 * burner-plugin-manager.h
 * This file is part of burner
 *
 * Copyright (C) 2007 Philippe Rouquier
 *
 * Based on burner code (burner/burner-plugin-manager.c) by: 
 * 	- Paolo Maggi <paolo@gnome.org>
 *
 * Libburner-media is free software; you can redistribute it and/or modify
fy
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * Burner is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to:
 * 	The Free Software Foundation, Inc.,
 * 	51 Franklin Street, Fifth Floor
 * 	Boston, MA  02110-1301, USA.
 */

#ifndef __BURNER_PLUGIN_MANAGER_UI_H__
#define __BURNER_PLUGIN_MANAGER_UI_H__

#include <gtk/gtk.h>

G_BEGIN_DECLS

/*
 * Type checking and casting macros
 */
#define BURNER_TYPE_PLUGIN_MANAGER_UI              (burner_plugin_manager_ui_get_type())
#define BURNER_PLUGIN_MANAGER_UI(obj)              (G_TYPE_CHECK_INSTANCE_CAST((obj), BURNER_TYPE_PLUGIN_MANAGER_UI, BurnerPluginManagerUI))
#define BURNER_PLUGIN_MANAGER_UI_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST((klass), BURNER_TYPE_PLUGIN_MANAGER_UI, BurnerPluginManagerUIClass))
#define BURNER_IS_PLUGIN_MANAGER_UI(obj)           (G_TYPE_CHECK_INSTANCE_TYPE((obj), BURNER_TYPE_PLUGIN_MANAGER_UI))
#define BURNER_IS_PLUGIN_MANAGER_UI_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), BURNER_TYPE_PLUGIN_MANAGER_UI))
#define BURNER_PLUGIN_MANAGER_UI_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS((obj), BURNER_TYPE_PLUGIN_MANAGER_UI, BurnerPluginManagerUIClass))

/* Private structure type */
typedef struct _BurnerPluginManagerUIPrivate BurnerPluginManagerUIPrivate;

/*
 * Main object structure
 */
typedef struct _BurnerPluginManagerUI BurnerPluginManagerUI;

struct _BurnerPluginManagerUI 
{
	GtkBox vbox;
};

/*
 * Class definition
 */
typedef struct _BurnerPluginManagerUIClass BurnerPluginManagerUIClass;

struct _BurnerPluginManagerUIClass 
{
	GtkBoxClass parent_class;
};

/*
 * Public methods
 */
GType		 burner_plugin_manager_ui_get_type		(void) G_GNUC_CONST;

GtkWidget	*burner_plugin_manager_ui_new		(void);
   
G_END_DECLS

#endif  /* __BURNER_PLUGIN_MANAGER_UI_H__  */
