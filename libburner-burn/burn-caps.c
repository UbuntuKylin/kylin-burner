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

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <string.h>

#include <glib.h>
#include <glib-object.h>
#include <glib/gi18n.h>

#include "burner-media-private.h"

#include "burn-basics.h"
#include "burn-debug.h"
#include "burner-drive.h"
#include "burner-medium.h"
#include "burner-session.h"
#include "burner-plugin.h"
#include "burner-plugin-information.h"
#include "burn-task.h"
#include "burn-caps.h"
#include "burner-track-type-private.h"

#define BURNER_ENGINE_GROUP_KEY	"engine-group"
#define BURNER_SCHEMA_CONFIG		"org.gnome.burner.config"

G_DEFINE_TYPE (BurnerBurnCaps, burner_burn_caps, G_TYPE_OBJECT);

static GObjectClass *parent_class = NULL;


static void
burner_caps_link_free (BurnerCapsLink *link)
{
	g_slist_free (link->plugins);
	g_free (link);
}

BurnerBurnResult
burner_caps_link_check_recorder_flags_for_input (BurnerCapsLink *link,
                                                  BurnerBurnFlag session_flags)
{
	if (burner_track_type_get_has_image (&link->caps->type)) {
		BurnerImageFormat format;

		format = burner_track_type_get_image_format (&link->caps->type);
		if (format == BURNER_IMAGE_FORMAT_CUE
		||  format == BURNER_IMAGE_FORMAT_CDRDAO) {
			if ((session_flags & BURNER_BURN_FLAG_DAO) == 0)
				return BURNER_BURN_NOT_SUPPORTED;
		}
		else if (format == BURNER_IMAGE_FORMAT_CLONE) {
			/* RAW write mode should (must) only be used in this case */
			if ((session_flags & BURNER_BURN_FLAG_RAW) == 0)
				return BURNER_BURN_NOT_SUPPORTED;
		}
	}

	return BURNER_BURN_OK;
}

gboolean
burner_caps_link_active (BurnerCapsLink *link,
                          gboolean ignore_plugin_errors)
{
	GSList *iter;

	/* See if link is active by going through all plugins. There must be at
	 * least one. */
	for (iter = link->plugins; iter; iter = iter->next) {
		BurnerPlugin *plugin;

		plugin = iter->data;
		if (burner_plugin_get_active (plugin, ignore_plugin_errors))
			return TRUE;
	}

	return FALSE;
}

BurnerPlugin *
burner_caps_link_need_download (BurnerCapsLink *link)
{
	GSList *iter;
	BurnerPlugin *plugin_ret = NULL;

	/* See if for link to be active, we need to 
	 * download additional apps/libs/.... */
	for (iter = link->plugins; iter; iter = iter->next) {
		BurnerPlugin *plugin;

		plugin = iter->data;

		/* If a plugin can be used without any
		 * error then that means that the link
		 * can be followed without additional
		 * download. */
		if (burner_plugin_get_active (plugin, FALSE))
			return NULL;

		if (burner_plugin_get_active (plugin, TRUE)) {
			if (!plugin_ret)
				plugin_ret = plugin;
			else if (burner_plugin_get_priority (plugin) > burner_plugin_get_priority (plugin_ret))
				plugin_ret = plugin;
		}
	}

	return plugin_ret;
}

static void
burner_caps_test_free (BurnerCapsTest *caps)
{
	g_slist_foreach (caps->links, (GFunc) burner_caps_link_free, NULL);
	g_slist_free (caps->links);
	g_free (caps);
}

static void
burner_caps_free (BurnerCaps *caps)
{
	g_slist_free (caps->modifiers);
	g_slist_foreach (caps->links, (GFunc) burner_caps_link_free, NULL);
	g_slist_free (caps->links);
	g_free (caps);
}

static gboolean
burner_caps_has_active_input (BurnerCaps *caps,
			       BurnerCaps *input)
{
	GSList *links;

	for (links = caps->links; links; links = links->next) {
		BurnerCapsLink *link;

		link = links->data;
		if (link->caps != input)
			continue;

		/* Ignore plugin errors */
		if (burner_caps_link_active (link, TRUE))
			return TRUE;
	}

	return FALSE;
}

gboolean
burner_burn_caps_is_input (BurnerBurnCaps *self,
			    BurnerCaps *input)
{
	GSList *iter;

	for (iter = self->priv->caps_list; iter; iter = iter->next) {
		BurnerCaps *tmp;

		tmp = iter->data;
		if (tmp == input)
			continue;

		if (burner_caps_has_active_input (tmp, input))
			return TRUE;
	}

	return FALSE;
}

gboolean
burner_caps_is_compatible_type (const BurnerCaps *caps,
				 const BurnerTrackType *type)
{
	if (caps->type.type != type->type)
		return FALSE;

	switch (type->type) {
	case BURNER_TRACK_TYPE_DATA:
		if ((caps->type.subtype.fs_type & type->subtype.fs_type) != type->subtype.fs_type)
			return FALSE;
		break;
	
	case BURNER_TRACK_TYPE_DISC:
		if (type->subtype.media == BURNER_MEDIUM_NONE)
			return FALSE;

		/* Reminder: we now create every possible types */
		if (caps->type.subtype.media != type->subtype.media)
			return FALSE;
		break;
	
	case BURNER_TRACK_TYPE_IMAGE:
		if (type->subtype.img_format == BURNER_IMAGE_FORMAT_NONE)
			return FALSE;

		if ((caps->type.subtype.img_format & type->subtype.img_format) != type->subtype.img_format)
			return FALSE;
		break;

	case BURNER_TRACK_TYPE_STREAM:
		/* There is one small special case here with video. */
		if ((caps->type.subtype.stream_format & (BURNER_VIDEO_FORMAT_UNDEFINED|
							 BURNER_VIDEO_FORMAT_VCD|
							 BURNER_VIDEO_FORMAT_VIDEO_DVD))
		&& !(type->subtype.stream_format & (BURNER_VIDEO_FORMAT_UNDEFINED|
						   BURNER_VIDEO_FORMAT_VCD|
						   BURNER_VIDEO_FORMAT_VIDEO_DVD)))
			return FALSE;

		if ((caps->type.subtype.stream_format & type->subtype.stream_format) != type->subtype.stream_format)
			return FALSE;
		break;

	default:
		break;
	}

	return TRUE;
}

BurnerCaps *
burner_burn_caps_find_start_caps (BurnerBurnCaps *self,
				   BurnerTrackType *output)
{
	GSList *iter;

	for (iter = self->priv->caps_list; iter; iter = iter->next) {
		BurnerCaps *caps;

		caps = iter->data;

		if (!burner_caps_is_compatible_type (caps, output))
			continue;

		if (burner_track_type_get_has_medium (&caps->type)
		|| (caps->flags & BURNER_PLUGIN_IO_ACCEPT_FILE))
			return caps;
	}

	return NULL;
}

static void
burner_burn_caps_finalize (GObject *object)
{
	BurnerBurnCaps *cobj;

	cobj = BURNER_BURNCAPS (object);
	
	if (cobj->priv->groups) {
		g_hash_table_destroy (cobj->priv->groups);
		cobj->priv->groups = NULL;
	}

	g_slist_foreach (cobj->priv->caps_list, (GFunc) burner_caps_free, NULL);
	g_slist_free (cobj->priv->caps_list);

	if (cobj->priv->group_str) {
		g_free (cobj->priv->group_str);
		cobj->priv->group_str = NULL;
	}

	if (cobj->priv->tests) {
		g_slist_foreach (cobj->priv->tests, (GFunc) burner_caps_test_free, NULL);
		g_slist_free (cobj->priv->tests);
		cobj->priv->tests = NULL;
	}

	g_free (cobj->priv);
	G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
burner_burn_caps_class_init (BurnerBurnCapsClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	parent_class = g_type_class_peek_parent (klass);
	object_class->finalize = burner_burn_caps_finalize;
}

static void
burner_burn_caps_init (BurnerBurnCaps *obj)
{
	GSettings *settings;

	obj->priv = g_new0 (BurnerBurnCapsPrivate, 1);

	settings = g_settings_new (BURNER_SCHEMA_CONFIG);
	obj->priv->group_str = g_settings_get_string (settings, BURNER_ENGINE_GROUP_KEY);
	g_object_unref (settings);
}
