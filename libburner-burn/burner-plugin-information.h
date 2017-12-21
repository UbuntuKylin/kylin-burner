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
 
#ifndef _BURN_PLUGIN_REGISTRATION_H
#define _BURN_PLUGIN_REGISTRATION_H

#include <glib.h>
#include <glib-object.h>

#include "burner-session.h"
#include "burner-plugin.h"

G_BEGIN_DECLS

void
burner_plugin_set_active (BurnerPlugin *plugin, gboolean active);

gboolean
burner_plugin_get_active (BurnerPlugin *plugin,
                           gboolean ignore_errors);

const gchar *
burner_plugin_get_name (BurnerPlugin *plugin);

const gchar *
burner_plugin_get_display_name (BurnerPlugin *plugin);

const gchar *
burner_plugin_get_author (BurnerPlugin *plugin);

guint
burner_plugin_get_group (BurnerPlugin *plugin);

const gchar *
burner_plugin_get_copyright (BurnerPlugin *plugin);

const gchar *
burner_plugin_get_website (BurnerPlugin *plugin);

const gchar *
burner_plugin_get_description (BurnerPlugin *plugin);

const gchar *
burner_plugin_get_icon_name (BurnerPlugin *plugin);

typedef struct _BurnerPluginError BurnerPluginError;
struct _BurnerPluginError {
	BurnerPluginErrorType type;
	gchar *detail;
};

GSList *
burner_plugin_get_errors (BurnerPlugin *plugin);

gchar *
burner_plugin_get_error_string (BurnerPlugin *plugin);

gboolean
burner_plugin_get_compulsory (BurnerPlugin *plugin);

guint
burner_plugin_get_priority (BurnerPlugin *plugin);

/** 
 * This is to find out what are the capacities of a plugin 
 */

BurnerBurnResult
burner_plugin_can_burn (BurnerPlugin *plugin);

BurnerBurnResult
burner_plugin_can_image (BurnerPlugin *plugin);

BurnerBurnResult
burner_plugin_can_convert (BurnerPlugin *plugin);


/**
 * Plugin configuration options
 */

BurnerPluginConfOption *
burner_plugin_get_next_conf_option (BurnerPlugin *plugin,
				     BurnerPluginConfOption *current);

BurnerBurnResult
burner_plugin_conf_option_get_info (BurnerPluginConfOption *option,
				     gchar **key,
				     gchar **description,
				     BurnerPluginConfOptionType *type);

GSList *
burner_plugin_conf_option_bool_get_suboptions (BurnerPluginConfOption *option);

gint
burner_plugin_conf_option_int_get_min (BurnerPluginConfOption *option);
gint
burner_plugin_conf_option_int_get_max (BurnerPluginConfOption *option);


struct _BurnerPluginChoicePair {
	gchar *string;
	guint value;
};
typedef struct _BurnerPluginChoicePair BurnerPluginChoicePair;

GSList *
burner_plugin_conf_option_choice_get (BurnerPluginConfOption *option);

G_END_DECLS

#endif
 
