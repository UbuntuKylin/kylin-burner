/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/*
 * Libburner-burn
 * Copyright (C) Philippe Rouquier 2005-2009 <bonfire-app@wanadoo.fr>
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

#ifndef _BURN_PLUGIN_H_
#define _BURN_PLUGIN_H_

#include <glib.h>
#include <glib-object.h>
#include <gmodule.h>

G_BEGIN_DECLS

#define BURNER_TYPE_PLUGIN             (burner_plugin_get_type ())
#define BURNER_PLUGIN(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), BURNER_TYPE_PLUGIN, BurnerPlugin))
#define BURNER_PLUGIN_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), BURNER_TYPE_PLUGIN, BurnerPluginClass))
#define BURNER_IS_PLUGIN(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BURNER_TYPE_PLUGIN))
#define BURNER_IS_PLUGIN_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), BURNER_TYPE_PLUGIN))
#define BURNER_PLUGIN_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), BURNER_TYPE_PLUGIN, BurnerPluginClass))

typedef struct _BurnerPluginClass BurnerPluginClass;
typedef struct _BurnerPlugin BurnerPlugin;

struct _BurnerPluginClass {
	GTypeModuleClass parent_class;

	/* Signals */
	void	(* loaded)	(BurnerPlugin *plugin);
	void	(* activated)	(BurnerPlugin *plugin,
			                  gboolean active);
};

struct _BurnerPlugin {
	GTypeModule parent_instance;
};

GType burner_plugin_get_type (void) G_GNUC_CONST;

GType
burner_plugin_get_gtype (BurnerPlugin *self);

/**
 * Plugin configure options
 */

typedef struct _BurnerPluginConfOption BurnerPluginConfOption;
typedef enum {
	BURNER_PLUGIN_OPTION_NONE	= 0,
	BURNER_PLUGIN_OPTION_BOOL,
	BURNER_PLUGIN_OPTION_INT,
	BURNER_PLUGIN_OPTION_STRING,
	BURNER_PLUGIN_OPTION_CHOICE
} BurnerPluginConfOptionType;

typedef enum {
	BURNER_PLUGIN_RUN_NEVER		= 0,

	/* pre-process initial track */
	BURNER_PLUGIN_RUN_PREPROCESSING	= 1,

	/* run before final image/disc is created */
	BURNER_PLUGIN_RUN_BEFORE_TARGET	= 1 << 1,

	/* run after final image/disc is created: post-processing */
	BURNER_PLUGIN_RUN_AFTER_TARGET		= 1 << 2,
} BurnerPluginProcessFlag;

G_END_DECLS

#endif /* _BURN_PLUGIN_H_ */
