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
 
#ifndef _BURN_PLUGIN_PRIVATE_H
#define _BURN_PLUGIN_PRIVATE_H

#include <glib.h>

#include "burner-media.h"

#include "burner-enums.h"
#include "burner-plugin.h"

G_BEGIN_DECLS

BurnerPlugin *
burner_plugin_new (const gchar *path);

void
burner_plugin_set_group (BurnerPlugin *plugin, gint group_id);

gboolean
burner_plugin_get_image_flags (BurnerPlugin *plugin,
			        BurnerMedia media,
				BurnerBurnFlag current,
			        BurnerBurnFlag *supported,
			        BurnerBurnFlag *compulsory);
gboolean
burner_plugin_get_blank_flags (BurnerPlugin *plugin,
				BurnerMedia media,
				BurnerBurnFlag current,
				BurnerBurnFlag *supported,
				BurnerBurnFlag *compulsory);
gboolean
burner_plugin_get_record_flags (BurnerPlugin *plugin,
				 BurnerMedia media,
				 BurnerBurnFlag current,
				 BurnerBurnFlag *supported,
				 BurnerBurnFlag *compulsory);

gboolean
burner_plugin_get_process_flags (BurnerPlugin *plugin,
				  BurnerPluginProcessFlag *flags);

gboolean
burner_plugin_check_image_flags (BurnerPlugin *plugin,
				  BurnerMedia media,
				  BurnerBurnFlag current);
gboolean
burner_plugin_check_blank_flags (BurnerPlugin *plugin,
				  BurnerMedia media,
				  BurnerBurnFlag current);
gboolean
burner_plugin_check_record_flags (BurnerPlugin *plugin,
				   BurnerMedia media,
				   BurnerBurnFlag current);
gboolean
burner_plugin_check_media_restrictions (BurnerPlugin *plugin,
					 BurnerMedia media);

void
burner_plugin_check_plugin_ready (BurnerPlugin *plugin);

G_END_DECLS

#endif
