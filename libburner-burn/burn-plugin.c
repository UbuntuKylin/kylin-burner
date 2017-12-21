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
#include <gmodule.h>
#include <glib/gi18n-lib.h>

#include <gst/gst.h>

#include <stdio.h>

#include "burner-media-private.h"

#include "burner-media.h"

#include "burn-basics.h"
#include "burn-debug.h"
#include "burner-plugin.h"
#include "burner-plugin-private.h"
#include "burner-plugin-information.h"
#include "burner-plugin-registration.h"
#include "burn-caps.h"

#define BURNER_SCHEMA_PLUGINS				"org.gnome.burner.plugins"
#define BURNER_PROPS_PRIORITY_KEY			"priority"

typedef struct _BurnerPluginFlagPair BurnerPluginFlagPair;

struct _BurnerPluginFlagPair {
	BurnerPluginFlagPair *next;
	BurnerBurnFlag supported;
	BurnerBurnFlag compulsory;
};

struct _BurnerPluginFlags {
	BurnerMedia media;
	BurnerPluginFlagPair *pairs;
};
typedef struct _BurnerPluginFlags BurnerPluginFlags;

struct _BurnerPluginConfOption {
	gchar *key;
	gchar *description;
	BurnerPluginConfOptionType type;

	union {
		struct {
			guint max;
			guint min;
		} range;

		GSList *suboptions;

		GSList *choices;
	} specifics;
};

typedef struct _BurnerPluginPrivate BurnerPluginPrivate;
struct _BurnerPluginPrivate
{
	GSettings *settings;

	gboolean active;
	guint group;

	GSList *options;

	GSList *errors;

	GType type;
	gchar *path;
	GModule *handle;

	gchar *name;
	gchar *display_name;
	gchar *author;
	gchar *description;
	gchar *copyright;
	gchar *website;

	guint notify_priority;
	guint priority_original;
	gint priority;

	GSList *flags;
	GSList *blank_flags;

	BurnerPluginProcessFlag process_flags;

	guint compulsory:1;
};

static const gchar *default_icon = "gtk-cdrom";

#define BURNER_PLUGIN_PRIVATE(o)  (G_TYPE_INSTANCE_GET_PRIVATE ((o), BURNER_TYPE_PLUGIN, BurnerPluginPrivate))
G_DEFINE_TYPE (BurnerPlugin, burner_plugin, G_TYPE_TYPE_MODULE);

enum
{
	PROP_0,
	PROP_PATH,
	PROP_PRIORITY
};

enum
{
	LOADED_SIGNAL,
	ACTIVATED_SIGNAL,
	LAST_SIGNAL
};

static GTypeModuleClass* parent_class = NULL;
static guint plugin_signals [LAST_SIGNAL] = { 0 };

static void
burner_plugin_error_free (BurnerPluginError *error)
{
	g_free (error->detail);
	g_free (error);
}

void
burner_plugin_add_error (BurnerPlugin *plugin,
                          BurnerPluginErrorType type,
                          const gchar *detail)
{
	BurnerPluginError *error;
	BurnerPluginPrivate *priv;

	g_return_if_fail (BURNER_IS_PLUGIN (plugin));

	priv = BURNER_PLUGIN_PRIVATE (plugin);

	error = g_new0 (BurnerPluginError, 1);
	error->detail = g_strdup (detail);
	error->type = type;

	priv->errors = g_slist_prepend (priv->errors, error);
}

void
burner_plugin_test_gstreamer_plugin (BurnerPlugin *plugin,
                                      const gchar *name)
{
	GstElement *element;

	/* Let's see if we've got the plugins we need */
	element = gst_element_factory_make (name, NULL);
	if (!element)
		burner_plugin_add_error (plugin,
		                          BURNER_PLUGIN_ERROR_MISSING_GSTREAMER_PLUGIN,
		                          name);
	else
		gst_object_unref (element);
}

void
burner_plugin_test_app (BurnerPlugin *plugin,
                         const gchar *name,
                         const gchar *version_arg,
                         const gchar *version_format,
                         gint version [3])
{
	gchar *standard_output = NULL;
	gchar *standard_error = NULL;
	guint major, minor, sub;
	gchar *prog_path;
	GPtrArray *argv;
	gboolean res;
	int i;

	/* First see if this plugin can be used, i.e. if cdrecord is in
	 * the path */
	prog_path = g_find_program_in_path (name);
	if (!prog_path) {
		burner_plugin_add_error (plugin,
		                          BURNER_PLUGIN_ERROR_MISSING_APP,
		                          name);
		return;
	}

	if (!g_file_test (prog_path, G_FILE_TEST_IS_EXECUTABLE)) {
		g_free (prog_path);
		burner_plugin_add_error (plugin,
		                          BURNER_PLUGIN_ERROR_MISSING_APP,
		                          name);
		return;
	}

	/* make sure that's not a symlink pointing to something with another
	 * name like wodim.
	 * NOTE: we used to test the target and see if it had the same name as
	 * the symlink with GIO. The problem is, when the symlink pointed to
	 * another symlink, then GIO didn't follow that other symlink. And in
	 * the end it didn't work. So forbid all symlink. */
	if (g_file_test (prog_path, G_FILE_TEST_IS_SYMLINK)) {
		burner_plugin_add_error (plugin,
		                          BURNER_PLUGIN_ERROR_SYMBOLIC_LINK_APP,
		                          name);
		g_free (prog_path);
		return;
	}
	/* Make sure it's a regular file */
	else if (!g_file_test (prog_path, G_FILE_TEST_IS_REGULAR)) {
		burner_plugin_add_error (plugin,
		                          BURNER_PLUGIN_ERROR_MISSING_APP,
		                          name);
		g_free (prog_path);
		return;
	}

	if (!version_arg) {
		g_free (prog_path);
		return;
	}

	/* Check version */
	argv = g_ptr_array_new ();
	g_ptr_array_add (argv, prog_path);
	g_ptr_array_add (argv, (gchar *) version_arg);
	g_ptr_array_add (argv, NULL);

	res = g_spawn_sync (NULL,
	                    (gchar **) argv->pdata,
	                    NULL,
	                    0,
	                    NULL,
	                    NULL,
	                    &standard_output,
	                    &standard_error,
	                    NULL,
	                    NULL);

	g_ptr_array_free (argv, TRUE);
	g_free (prog_path);

	if (!res) {
		burner_plugin_add_error (plugin,
		                          BURNER_PLUGIN_ERROR_WRONG_APP_VERSION,
		                          name);
		return;
	}

	for (i = 0; i < 3 && version [i] >= 0; i++);

	if ((standard_output && sscanf (standard_output, version_format, &major, &minor, &sub) == i)
	||  (standard_error && sscanf (standard_error, version_format, &major, &minor, &sub) == i)) {
		if (major < version [0]
		||  (version [1] >= 0 && minor < version [1])
		||  (version [2] >= 0 && sub < version [2]))
			burner_plugin_add_error (plugin,
						  BURNER_PLUGIN_ERROR_WRONG_APP_VERSION,
						  name);
	}
	else
		burner_plugin_add_error (plugin,
		                          BURNER_PLUGIN_ERROR_WRONG_APP_VERSION,
		                          name);

	g_free (standard_output);
	g_free (standard_error);
}

void
burner_plugin_set_compulsory (BurnerPlugin *self,
			       gboolean compulsory)
{
	BurnerPluginPrivate *priv;

	priv = BURNER_PLUGIN_PRIVATE (self);
	priv->compulsory = compulsory;
}

gboolean
burner_plugin_get_compulsory (BurnerPlugin *self)
{
	BurnerPluginPrivate *priv;

	priv = BURNER_PLUGIN_PRIVATE (self);
	return priv->compulsory;
}

void
burner_plugin_set_active (BurnerPlugin *self, gboolean active)
{
	BurnerPluginPrivate *priv;
	gboolean was_active;
	gboolean now_active;

	priv = BURNER_PLUGIN_PRIVATE (self);

	was_active = burner_plugin_get_active (self, FALSE);
	priv->active = active;

	now_active = burner_plugin_get_active (self, FALSE);
	if (was_active == now_active)
		return;

	BURNER_BURN_LOG ("Plugin %s is %s",
			  burner_plugin_get_name (self),
			  now_active?"active":"inactive");

	g_signal_emit (self,
		       plugin_signals [ACTIVATED_SIGNAL],
		       0,
		       now_active);
}

gboolean
burner_plugin_get_active (BurnerPlugin *plugin,
                           gboolean ignore_errors)
{
	BurnerPluginPrivate *priv;

	priv = BURNER_PLUGIN_PRIVATE (plugin);

	if (priv->type == G_TYPE_NONE)
		return FALSE;

	if (priv->priority < 0)
		return FALSE;

	if (priv->errors) {
		if (!ignore_errors)
			return FALSE;
	}

	return priv->active;
}

static void
burner_plugin_cleanup_definition (BurnerPlugin *self)
{
	BurnerPluginPrivate *priv;

	priv = BURNER_PLUGIN_PRIVATE (self);

	g_free (priv->name);
	priv->name = NULL;
	g_free (priv->author);
	priv->author = NULL;
	g_free (priv->description);
	priv->description = NULL;
	g_free (priv->copyright);
	priv->copyright = NULL;
	g_free (priv->website);
	priv->website = NULL;
}

/**
 * Plugin configure options
 */

static void
burner_plugin_conf_option_choice_pair_free (BurnerPluginChoicePair *pair)
{
	g_free (pair->string);
	g_free (pair);
}

static void
burner_plugin_conf_option_free (BurnerPluginConfOption *option)
{
	if (option->type == BURNER_PLUGIN_OPTION_BOOL)
		g_slist_free (option->specifics.suboptions);

	if (option->type == BURNER_PLUGIN_OPTION_CHOICE) {
		g_slist_foreach (option->specifics.choices, (GFunc) burner_plugin_conf_option_choice_pair_free, NULL);
		g_slist_free (option->specifics.choices);
	}

	g_free (option->key);
	g_free (option->description);

	g_free (option);
}

BurnerPluginConfOption *
burner_plugin_get_next_conf_option (BurnerPlugin *self,
				     BurnerPluginConfOption *current)
{
	BurnerPluginPrivate *priv;
	GSList *node;

	priv = BURNER_PLUGIN_PRIVATE (self);
	if (!priv->options)
		return NULL;

	if (!current)
		return priv->options->data;

	node = g_slist_find (priv->options, current);
	if (!node)
		return NULL;

	if (!node->next)
		return NULL;

	return node->next->data;
}

BurnerBurnResult
burner_plugin_conf_option_get_info (BurnerPluginConfOption *option,
				     gchar **key,
				     gchar **description,
				     BurnerPluginConfOptionType *type)
{
	g_return_val_if_fail (option != NULL, BURNER_BURN_ERR);

	if (key)
		*key = g_strdup (option->key);

	if (description)
		*description = g_strdup (option->description);

	if (type)
		*type = option->type;

	return BURNER_BURN_OK;
}

BurnerPluginConfOption *
burner_plugin_conf_option_new (const gchar *key,
				const gchar *description,
				BurnerPluginConfOptionType type)
{
	BurnerPluginConfOption *option;

	g_return_val_if_fail (key != NULL, NULL);
	g_return_val_if_fail (description != NULL, NULL);
	g_return_val_if_fail (type != BURNER_PLUGIN_OPTION_NONE, NULL);

	option = g_new0 (BurnerPluginConfOption, 1);
	option->key = g_strdup (key);
	option->description = g_strdup (description);
	option->type = type;

	return option;
}

BurnerBurnResult
burner_plugin_add_conf_option (BurnerPlugin *self,
				BurnerPluginConfOption *option)
{
	BurnerPluginPrivate *priv;

	priv = BURNER_PLUGIN_PRIVATE (self);
	priv->options = g_slist_append (priv->options, option);

	return BURNER_BURN_OK;
}

BurnerBurnResult
burner_plugin_conf_option_bool_add_suboption (BurnerPluginConfOption *option,
					       BurnerPluginConfOption *suboption)
{
	if (option->type != BURNER_PLUGIN_OPTION_BOOL)
		return BURNER_BURN_ERR;

	option->specifics.suboptions = g_slist_prepend (option->specifics.suboptions,
						        suboption);
	return BURNER_BURN_OK;
}

BurnerBurnResult
burner_plugin_conf_option_int_set_range (BurnerPluginConfOption *option,
					  gint min,
					  gint max)
{
	if (option->type != BURNER_PLUGIN_OPTION_INT)
		return BURNER_BURN_ERR;

	option->specifics.range.max = max;
	option->specifics.range.min = min;
	return BURNER_BURN_OK;
}

BurnerBurnResult
burner_plugin_conf_option_choice_add (BurnerPluginConfOption *option,
				       const gchar *string,
				       gint value)
{
	BurnerPluginChoicePair *pair;

	if (option->type != BURNER_PLUGIN_OPTION_CHOICE)
		return BURNER_BURN_ERR;

	pair = g_new0 (BurnerPluginChoicePair, 1);
	pair->value = value;
	pair->string = g_strdup (string);
	option->specifics.choices = g_slist_append (option->specifics.choices, pair);

	return BURNER_BURN_OK;
}

GSList *
burner_plugin_conf_option_bool_get_suboptions (BurnerPluginConfOption *option)
{
	if (option->type != BURNER_PLUGIN_OPTION_BOOL)
		return NULL;
	return option->specifics.suboptions;
}

gint
burner_plugin_conf_option_int_get_max (BurnerPluginConfOption *option) 
{
	if (option->type != BURNER_PLUGIN_OPTION_INT)
		return -1;
	return option->specifics.range.max;
}

gint
burner_plugin_conf_option_int_get_min (BurnerPluginConfOption *option)
{
	if (option->type != BURNER_PLUGIN_OPTION_INT)
		return -1;
	return option->specifics.range.min;
}

GSList *
burner_plugin_conf_option_choice_get (BurnerPluginConfOption *option)
{
	if (option->type != BURNER_PLUGIN_OPTION_CHOICE)
		return NULL;
	return option->specifics.choices;
}

/**
 * Used to set the caps of plugin
 */

void
burner_plugin_define (BurnerPlugin *self,
		       const gchar *name,
                       const gchar *display_name,
                       const gchar *description,
		       const gchar *author,
		       guint priority)
{
	BurnerPluginPrivate *priv;

	priv = BURNER_PLUGIN_PRIVATE (self);

	burner_plugin_cleanup_definition (self);

	priv->name = g_strdup (name);
	priv->display_name = g_strdup (display_name);
	priv->author = g_strdup (author);
	priv->description = g_strdup (description);
	priv->priority_original = priority;
}

void
burner_plugin_set_group (BurnerPlugin *self,
			  gint group_id)
{
	BurnerPluginPrivate *priv;

	priv = BURNER_PLUGIN_PRIVATE (self);
	priv->group = group_id;
}

guint
burner_plugin_get_group (BurnerPlugin *self)
{
	BurnerPluginPrivate *priv;

	priv = BURNER_PLUGIN_PRIVATE (self);
	return priv->group;
}

/**
 * burner_plugin_get_errors:
 * @plugin: a #BurnerPlugin.
 *
 * This function returns a list of all errors that
 * prevents the plugin from working properly.
 *
 * Returns: a #GSList of #BurnerPluginError structures or %NULL.
 * It must not be freed.
 **/

GSList *
burner_plugin_get_errors (BurnerPlugin *plugin)
{
	BurnerPluginPrivate *priv;

	g_return_val_if_fail (BURNER_IS_PLUGIN (plugin), NULL);
	priv = BURNER_PLUGIN_PRIVATE (plugin);
	return priv->errors;
}

gchar *
burner_plugin_get_error_string (BurnerPlugin *plugin)
{
	gchar *error_string = NULL;
	BurnerPluginPrivate *priv;
	GString *string;
	GSList *iter;

	g_return_val_if_fail (BURNER_IS_PLUGIN (plugin), NULL);

	priv = BURNER_PLUGIN_PRIVATE (plugin);

	string = g_string_new (NULL);
	for (iter = priv->errors; iter; iter = iter->next) {
		BurnerPluginError *error;

		error = iter->data;
		switch (error->type) {
			case BURNER_PLUGIN_ERROR_MISSING_APP:
				g_string_append_c (string, '\n');
				g_string_append_printf (string, _("\"%s\" could not be found in the path"), error->detail);
				break;
			case BURNER_PLUGIN_ERROR_MISSING_GSTREAMER_PLUGIN:
				g_string_append_c (string, '\n');
				g_string_append_printf (string, _("\"%s\" GStreamer plugin could not be found"), error->detail);
				break;
			case BURNER_PLUGIN_ERROR_WRONG_APP_VERSION:
				g_string_append_c (string, '\n');
				g_string_append_printf (string, _("The version of \"%s\" is too old"), error->detail);
				break;
			case BURNER_PLUGIN_ERROR_SYMBOLIC_LINK_APP:
				g_string_append_c (string, '\n');
				g_string_append_printf (string, _("\"%s\" is a symbolic link pointing to another program"), error->detail);
				break;
			case BURNER_PLUGIN_ERROR_MISSING_LIBRARY:
				g_string_append_c (string, '\n');
				g_string_append_printf (string, _("\"%s\" could not be found"), error->detail);
				break;
			case BURNER_PLUGIN_ERROR_LIBRARY_VERSION:
				g_string_append_c (string, '\n');
				g_string_append_printf (string, _("The version of \"%s\" is too old"), error->detail);
				break;
			case BURNER_PLUGIN_ERROR_MODULE:
				g_string_append_c (string, '\n');
				g_string_append (string, error->detail);
				break;

			default:
				break;
		}
	}

	error_string = string->str;
	g_string_free (string, FALSE);
	return error_string;
}

static BurnerPluginFlags *
burner_plugin_get_flags (GSList *flags,
			  BurnerMedia media)
{
	GSList *iter;

	for (iter = flags; iter; iter = iter->next) {
		BurnerPluginFlags *flags;

		flags = iter->data;
		if ((media & flags->media) == media)
			return flags;
	}

	return NULL;
}

static GSList *
burner_plugin_set_flags_real (GSList *flags_list,
			       BurnerMedia media,
			       BurnerBurnFlag supported,
			       BurnerBurnFlag compulsory)
{
	BurnerPluginFlags *flags;
	BurnerPluginFlagPair *pair;

	flags = burner_plugin_get_flags (flags_list, media);
	if (!flags) {
		flags = g_new0 (BurnerPluginFlags, 1);
		flags->media = media;
		flags_list = g_slist_prepend (flags_list, flags);
	}
	else for (pair = flags->pairs; pair; pair = pair->next) {
		/* have a look at the BurnerPluginFlagPair to see if there
		 * is an exactly similar pair of flags or at least which
		 * encompasses it to avoid redundancy. */
		if ((pair->supported & supported) == supported
		&&  (pair->compulsory & compulsory) == compulsory)
			return flags_list;
	}

	pair = g_new0 (BurnerPluginFlagPair, 1);
	pair->supported = supported;
	pair->compulsory = compulsory;

	pair->next = flags->pairs;
	flags->pairs = pair;

	return flags_list;
}

void
burner_plugin_set_flags (BurnerPlugin *self,
			  BurnerMedia media,
			  BurnerBurnFlag supported,
			  BurnerBurnFlag compulsory)
{
	BurnerPluginPrivate *priv;
	GSList *list;
	GSList *iter;

	priv = BURNER_PLUGIN_PRIVATE (self);

	list = burner_media_get_all_list (media);
	for (iter = list; iter; iter = iter->next) {
		BurnerMedia medium;

		medium = GPOINTER_TO_INT (iter->data);
		priv->flags = burner_plugin_set_flags_real (priv->flags,
							     medium,
							     supported,
							     compulsory);
	}
	g_slist_free (list);
}

static gboolean
burner_plugin_get_all_flags (GSList *flags_list,
			      gboolean check_compulsory,
			      BurnerMedia media,
			      BurnerBurnFlag mask,
			      BurnerBurnFlag current,
			      BurnerBurnFlag *supported_retval,
			      BurnerBurnFlag *compulsory_retval)
{
	gboolean found;
	BurnerPluginFlags *flags;
	BurnerPluginFlagPair *iter;
	BurnerBurnFlag supported = BURNER_BURN_FLAG_NONE;
	BurnerBurnFlag compulsory = (BURNER_BURN_FLAG_ALL & mask);

	flags = burner_plugin_get_flags (flags_list, media);
	if (!flags) {
		if (supported_retval)
			*supported_retval = BURNER_BURN_FLAG_NONE;
		if (compulsory_retval)
			*compulsory_retval = BURNER_BURN_FLAG_NONE;
		return FALSE;
	}

	/* Find all sets of flags that support the current flags */
	found = FALSE;
	for (iter = flags->pairs; iter; iter = iter->next) {
		BurnerBurnFlag compulsory_masked;

		if ((current & iter->supported) != current)
			continue;

		compulsory_masked = (iter->compulsory & mask);
		if (check_compulsory
		&& (current & compulsory_masked) != compulsory_masked)
			continue;

		supported |= iter->supported;
		compulsory &= compulsory_masked;
		found = TRUE;
	}

	if (!found) {
		if (supported_retval)
			*supported_retval = BURNER_BURN_FLAG_NONE;
		if (compulsory_retval)
			*compulsory_retval = BURNER_BURN_FLAG_NONE;
		return FALSE;
	}

	if (supported_retval)
		*supported_retval = supported;
	if (compulsory_retval)
		*compulsory_retval = compulsory;

	return TRUE;
}

gboolean
burner_plugin_check_record_flags (BurnerPlugin *self,
				   BurnerMedia media,
				   BurnerBurnFlag current)
{
	BurnerPluginPrivate *priv;

	priv = BURNER_PLUGIN_PRIVATE (self);
	current &= BURNER_PLUGIN_BURN_FLAG_MASK;

	return burner_plugin_get_all_flags (priv->flags,
					     TRUE,
					     media,
					     BURNER_PLUGIN_BURN_FLAG_MASK,
					     current,
					     NULL,
					     NULL);
}

gboolean
burner_plugin_check_image_flags (BurnerPlugin *self,
				  BurnerMedia media,
				  BurnerBurnFlag current)
{
	BurnerPluginPrivate *priv;

	priv = BURNER_PLUGIN_PRIVATE (self);

	current &= BURNER_PLUGIN_IMAGE_FLAG_MASK;

	/* If there is no flag that's no use checking anything. If there is no
	 * flag we don't care about the media and therefore it's always possible
	 * NOTE: that's no the case for other operation like burn/blank. */
	if (current == BURNER_BURN_FLAG_NONE)
		return TRUE;

	return burner_plugin_get_all_flags (priv->flags,
					     TRUE,
					     media,
					     BURNER_PLUGIN_IMAGE_FLAG_MASK,
					     current,
					     NULL,
					     NULL);
}

gboolean
burner_plugin_check_media_restrictions (BurnerPlugin *self,
					 BurnerMedia media)
{
	BurnerPluginPrivate *priv;

	priv = BURNER_PLUGIN_PRIVATE (self);

	/* no restrictions */
	if (!priv->flags)
		return TRUE;

	return (burner_plugin_get_flags (priv->flags, media) != NULL);
}

gboolean
burner_plugin_get_record_flags (BurnerPlugin *self,
				 BurnerMedia media,
				 BurnerBurnFlag current,
				 BurnerBurnFlag *supported,
				 BurnerBurnFlag *compulsory)
{
	BurnerPluginPrivate *priv;
	gboolean result;

	priv = BURNER_PLUGIN_PRIVATE (self);
	current &= BURNER_PLUGIN_BURN_FLAG_MASK;

	result = burner_plugin_get_all_flags (priv->flags,
					       FALSE,
					       media,
					       BURNER_PLUGIN_BURN_FLAG_MASK,
					       current,
					       supported,
					       compulsory);
	if (!result)
		return FALSE;

	if (supported)
		*supported &= BURNER_PLUGIN_BURN_FLAG_MASK;
	if (compulsory)
		*compulsory &= BURNER_PLUGIN_BURN_FLAG_MASK;

	return TRUE;
}

gboolean
burner_plugin_get_image_flags (BurnerPlugin *self,
				BurnerMedia media,
				BurnerBurnFlag current,
				BurnerBurnFlag *supported,
				BurnerBurnFlag *compulsory)
{
	BurnerPluginPrivate *priv;
	gboolean result;

	priv = BURNER_PLUGIN_PRIVATE (self);
	current &= BURNER_PLUGIN_IMAGE_FLAG_MASK;

	result = burner_plugin_get_all_flags (priv->flags,
					       FALSE,
					       media,
					       BURNER_PLUGIN_IMAGE_FLAG_MASK,
					       current,
					       supported,
					       compulsory);
	if (!result)
		return FALSE;

	if (supported)
		*supported &= BURNER_PLUGIN_IMAGE_FLAG_MASK;
	if (compulsory)
		*compulsory &= BURNER_PLUGIN_IMAGE_FLAG_MASK;

	return TRUE;
}

void
burner_plugin_set_blank_flags (BurnerPlugin *self,
				BurnerMedia media,
				BurnerBurnFlag supported,
				BurnerBurnFlag compulsory)
{
	BurnerPluginPrivate *priv;

	priv = BURNER_PLUGIN_PRIVATE (self);
	priv->blank_flags = burner_plugin_set_flags_real (priv->blank_flags,
							   media,
							   supported,
							   compulsory);
}

gboolean
burner_plugin_check_blank_flags (BurnerPlugin *self,
				  BurnerMedia media,
				  BurnerBurnFlag current)
{
	BurnerPluginPrivate *priv;

	priv = BURNER_PLUGIN_PRIVATE (self);
	current &= BURNER_PLUGIN_BLANK_FLAG_MASK;

	return burner_plugin_get_all_flags (priv->blank_flags,
					     TRUE,
					     media,
					     BURNER_PLUGIN_BLANK_FLAG_MASK,
					     current,
					     NULL,
					     NULL);
}

gboolean
burner_plugin_get_blank_flags (BurnerPlugin *self,
				BurnerMedia media,
				BurnerBurnFlag current,
			        BurnerBurnFlag *supported,
			        BurnerBurnFlag *compulsory)
{
	BurnerPluginPrivate *priv;
	gboolean result;

	priv = BURNER_PLUGIN_PRIVATE (self);
	current &= BURNER_PLUGIN_BLANK_FLAG_MASK;

	result = burner_plugin_get_all_flags (priv->blank_flags,
					       FALSE,
					       media,
					       BURNER_PLUGIN_BLANK_FLAG_MASK,
					       current,
					       supported,
					       compulsory);
	if (!result)
		return FALSE;

	if (supported)
		*supported &= BURNER_PLUGIN_BLANK_FLAG_MASK;
	if (compulsory)
		*compulsory &= BURNER_PLUGIN_BLANK_FLAG_MASK;

	return TRUE;
}

void
burner_plugin_set_process_flags (BurnerPlugin *plugin,
				  BurnerPluginProcessFlag flags)
{
	BurnerPluginPrivate *priv;

	priv = BURNER_PLUGIN_PRIVATE (plugin);
	priv->process_flags = flags;
}

gboolean
burner_plugin_get_process_flags (BurnerPlugin *plugin,
				  BurnerPluginProcessFlag *flags)
{
	BurnerPluginPrivate *priv;

	g_return_val_if_fail (flags != NULL, FALSE);

	priv = BURNER_PLUGIN_PRIVATE (plugin);
	*flags = priv->process_flags;
	return TRUE;
}

const gchar *
burner_plugin_get_name (BurnerPlugin *plugin)
{
	BurnerPluginPrivate *priv;

	priv = BURNER_PLUGIN_PRIVATE (plugin);
	return priv->name;
}

const gchar *
burner_plugin_get_display_name (BurnerPlugin *plugin)
{
	BurnerPluginPrivate *priv;

	priv = BURNER_PLUGIN_PRIVATE (plugin);
	return priv->display_name ? priv->display_name:priv->name;

}

const gchar *
burner_plugin_get_author (BurnerPlugin *plugin)
{
	BurnerPluginPrivate *priv;

	priv = BURNER_PLUGIN_PRIVATE (plugin);
	return priv->author;
}

const gchar *
burner_plugin_get_copyright (BurnerPlugin *plugin)
{
	BurnerPluginPrivate *priv;

	priv = BURNER_PLUGIN_PRIVATE (plugin);
	return priv->copyright;
}

const gchar *
burner_plugin_get_website (BurnerPlugin *plugin)
{
	BurnerPluginPrivate *priv;

	priv = BURNER_PLUGIN_PRIVATE (plugin);
	return priv->website;
}

const gchar *
burner_plugin_get_description (BurnerPlugin *plugin)
{
	BurnerPluginPrivate *priv;

	priv = BURNER_PLUGIN_PRIVATE (plugin);
	return priv->description;
}

const gchar *
burner_plugin_get_icon_name (BurnerPlugin *plugin)
{
	return default_icon;
}

guint
burner_plugin_get_priority (BurnerPlugin *self)
{
	BurnerPluginPrivate *priv;

	priv = BURNER_PLUGIN_PRIVATE (self);

	if (priv->priority > 0)
		return priv->priority;

	return priv->priority_original;
}

GType
burner_plugin_get_gtype (BurnerPlugin *self)
{
	BurnerPluginPrivate *priv;

	priv = BURNER_PLUGIN_PRIVATE (self);

	if (priv->errors)
		return G_TYPE_NONE;

	return priv->type;
}

/**
 * Function to initialize and load
 */

static void
burner_plugin_unload (GTypeModule *module)
{
	BurnerPluginPrivate *priv;

	priv = BURNER_PLUGIN_PRIVATE (module);
	if (!priv->handle)
		return;

	g_module_close (priv->handle);
	priv->handle = NULL;
}

static gboolean
burner_plugin_load_real (BurnerPlugin *plugin) 
{
	BurnerPluginPrivate *priv;
	BurnerPluginRegisterType register_func;

	priv = BURNER_PLUGIN_PRIVATE (plugin);

	if (!priv->path)
		return FALSE;

	if (priv->handle)
		return TRUE;

	priv->handle = g_module_open (priv->path, G_MODULE_BIND_LAZY);
	if (!priv->handle) {
		burner_plugin_add_error (plugin, BURNER_PLUGIN_ERROR_MODULE, g_module_error ());
		BURNER_BURN_LOG ("Module %s can't be loaded: g_module_open failed ()", priv->name);
		return FALSE;
	}

	if (!g_module_symbol (priv->handle, "burner_plugin_register", (gpointer) &register_func)) {
		BURNER_BURN_LOG ("it doesn't appear to be a valid burner plugin");
		burner_plugin_unload (G_TYPE_MODULE (plugin));
		return FALSE;
	}

	priv->type = register_func (plugin);
	burner_burn_debug_setup_module (priv->handle);
	return TRUE;
}

static gboolean
burner_plugin_load (GTypeModule *module) 
{
	if (!burner_plugin_load_real (BURNER_PLUGIN (module)))
		return FALSE;

	g_signal_emit (BURNER_PLUGIN (module),
		       plugin_signals [LOADED_SIGNAL],
		       0);
	return TRUE;
}

static void
burner_plugin_priority_changed (GSettings *settings,
                                 const gchar *key,
                                 BurnerPlugin *self)
{
	BurnerPluginPrivate *priv;
	gboolean is_active;

	priv = BURNER_PLUGIN_PRIVATE (self);

	/* At the moment it can only be the priority key */
	priv->priority = g_settings_get_int (settings, BURNER_PROPS_PRIORITY_KEY);

	is_active = burner_plugin_get_active (self, FALSE);

	g_object_notify (G_OBJECT (self), "priority");
	if (is_active != burner_plugin_get_active (self, FALSE))
		g_signal_emit (self,
			       plugin_signals [ACTIVATED_SIGNAL],
			       0,
			       is_active);
}

typedef void	(* BurnerPluginCheckConfig)	(BurnerPlugin *plugin);

/**
 * burner_plugin_check_plugin_ready:
 * @plugin: a #BurnerPlugin.
 *
 * Ask a plugin to check whether it can operate.
 * burner_plugin_can_operate () should be called
 * afterwards to know whether it can operate or not.
 *
 **/
void
burner_plugin_check_plugin_ready (BurnerPlugin *plugin)
{
	GModule *handle;
	BurnerPluginPrivate *priv;
	BurnerPluginCheckConfig function = NULL;

	g_return_if_fail (BURNER_IS_PLUGIN (plugin));
	priv = BURNER_PLUGIN_PRIVATE (plugin);

	if (priv->errors) {
		g_slist_foreach (priv->errors, (GFunc) burner_plugin_error_free, NULL);
		g_slist_free (priv->errors);
		priv->errors = NULL;
	}

	handle = g_module_open (priv->path, 0);
	if (!handle) {
		burner_plugin_add_error (plugin, BURNER_PLUGIN_ERROR_MODULE, g_module_error ());
		BURNER_BURN_LOG ("Module %s can't be loaded: g_module_open failed ()", priv->name);
		return;
	}

	if (!g_module_symbol (handle, "burner_plugin_check_config", (gpointer) &function)) {
		g_module_close (handle);
		BURNER_BURN_LOG ("Module %s has no check config function", priv->name);
		return;
	}

	function (BURNER_PLUGIN (plugin));
	g_module_close (handle);
}

static void
burner_plugin_init_real (BurnerPlugin *object)
{
	GModule *handle;
	gchar *settings_path;
	BurnerPluginPrivate *priv;
	BurnerPluginRegisterType function = NULL;

	priv = BURNER_PLUGIN_PRIVATE (object);

	g_type_module_set_name (G_TYPE_MODULE (object), priv->name);

	handle = g_module_open (priv->path, 0);
	if (!handle) {
		burner_plugin_add_error (object, BURNER_PLUGIN_ERROR_MODULE, g_module_error ());
		BURNER_BURN_LOG ("Module %s (at %s) can't be loaded: g_module_open failed ()", priv->name, priv->path);
		return;
	}

	if (!g_module_symbol (handle, "burner_plugin_register", (gpointer) &function)) {
		g_module_close (handle);
		BURNER_BURN_LOG ("Module %s can't be loaded: no register function, priv->name", priv->name);
		return;
	}

	priv->type = function (object);
	if (priv->type == G_TYPE_NONE) {
		g_module_close (handle);
		BURNER_BURN_LOG ("Module %s encountered an error while registering its capabilities", priv->name);
		return;
	}

	/* now see if we need to override the hardcoded priority of the plugin */
	settings_path = g_strconcat ("/org/gnome/burner/plugins/",
	                             priv->name,
	                             G_DIR_SEPARATOR_S,
	                             NULL);
	priv->settings = g_settings_new_with_path (BURNER_SCHEMA_PLUGINS,
	                                           settings_path);
	g_free (settings_path);

	priv->priority = g_settings_get_int (priv->settings, BURNER_PROPS_PRIORITY_KEY);

	/* Make sure a key is created for each plugin */
	if (!priv->priority)
		g_settings_set_int (priv->settings, BURNER_PROPS_PRIORITY_KEY, 0);

	g_signal_connect (priv->settings,
	                  "changed",
	                  G_CALLBACK (burner_plugin_priority_changed),
	                  object);

	/* Check if it can operate */
	burner_plugin_check_plugin_ready (object);

	g_module_close (handle);
}

static void
burner_plugin_finalize (GObject *object)
{
	BurnerPluginPrivate *priv;

	priv = BURNER_PLUGIN_PRIVATE (object);

	if (priv->options) {
		g_slist_foreach (priv->options, (GFunc) burner_plugin_conf_option_free, NULL);
		g_slist_free (priv->options);
		priv->options = NULL;
	}

	if (priv->handle) {
		burner_plugin_unload (G_TYPE_MODULE (object));
		priv->handle = NULL;
	}

	if (priv->path) {
		g_free (priv->path);
		priv->path = NULL;
	}

	g_free (priv->name);
	g_free (priv->display_name);
	g_free (priv->author);
	g_free (priv->description);

	g_slist_foreach (priv->flags, (GFunc) g_free, NULL);
	g_slist_free (priv->flags);

	g_slist_foreach (priv->blank_flags, (GFunc) g_free, NULL);
	g_slist_free (priv->blank_flags);

	if (priv->settings) {
		g_object_unref (priv->settings);
		priv->settings = NULL;
	}

	if (priv->errors) {
		g_slist_foreach (priv->errors, (GFunc) burner_plugin_error_free, NULL);
		g_slist_free (priv->errors);
		priv->errors = NULL;
	}

	G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
burner_plugin_set_property (GObject *object,
			     guint prop_id,
			     const GValue *value,
			     GParamSpec *pspec)
{
	BurnerPlugin *self;
	BurnerPluginPrivate *priv;

	g_return_if_fail (BURNER_IS_PLUGIN (object));

	self = BURNER_PLUGIN (object);
	priv = BURNER_PLUGIN_PRIVATE (self);

	switch (prop_id)
	{
	case PROP_PATH:
		/* NOTE: this property can only be set once */
		priv->path = g_strdup (g_value_get_string (value));
		burner_plugin_init_real (self);
		break;
	case PROP_PRIORITY:
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
		break;
	}
}

static void
burner_plugin_get_property (GObject *object,
			     guint prop_id,
			     GValue *value,
			     GParamSpec *pspec)
{
	BurnerPlugin *self;
	BurnerPluginPrivate *priv;

	g_return_if_fail (BURNER_IS_PLUGIN (object));

	self = BURNER_PLUGIN (object);
	priv = BURNER_PLUGIN_PRIVATE (self);

	switch (prop_id)
	{
	case PROP_PATH:
		g_value_set_string (value, priv->path);
		break;
	case PROP_PRIORITY:
		g_value_set_int (value, priv->priority);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
		break;
	}
}

static void
burner_plugin_init (BurnerPlugin *object)
{
	BurnerPluginPrivate *priv;

	priv = BURNER_PLUGIN_PRIVATE (object);
	priv->type = G_TYPE_NONE;
	priv->compulsory = TRUE;
}

static void
burner_plugin_class_init (BurnerPluginClass *klass)
{
	GObjectClass* object_class = G_OBJECT_CLASS (klass);
	GTypeModuleClass *module_class = G_TYPE_MODULE_CLASS (klass);

	g_type_class_add_private (klass, sizeof (BurnerPluginPrivate));

	object_class->finalize = burner_plugin_finalize;
	object_class->set_property = burner_plugin_set_property;
	object_class->get_property = burner_plugin_get_property;

	module_class->load = burner_plugin_load;
	module_class->unload = burner_plugin_unload;

	g_object_class_install_property (object_class,
	                                 PROP_PATH,
	                                 g_param_spec_string ("path",
	                                                      "Path",
	                                                      "Path for the module",
	                                                      NULL,
	                                                      G_PARAM_STATIC_NAME|G_PARAM_READABLE|G_PARAM_WRITABLE|G_PARAM_CONSTRUCT_ONLY));
	g_object_class_install_property (object_class,
	                                 PROP_PRIORITY,
	                                 g_param_spec_int ("priority",
							   "Priority",
							   "Priority of the module",
							   1,
							   G_MAXINT,
							   1,
							   G_PARAM_STATIC_NAME|G_PARAM_READABLE));

	plugin_signals [LOADED_SIGNAL] =
		g_signal_new ("loaded",
		              G_OBJECT_CLASS_TYPE (klass),
		              G_SIGNAL_RUN_LAST | G_SIGNAL_NO_RECURSE,
		              G_STRUCT_OFFSET (BurnerPluginClass, loaded),
		              NULL, NULL,
		              g_cclosure_marshal_VOID__VOID,
		              G_TYPE_NONE, 0);

	plugin_signals [ACTIVATED_SIGNAL] =
		g_signal_new ("activated",
		              G_OBJECT_CLASS_TYPE (klass),
		              G_SIGNAL_RUN_LAST | G_SIGNAL_NO_RECURSE,
		              G_STRUCT_OFFSET (BurnerPluginClass, activated),
		              NULL, NULL,
		              g_cclosure_marshal_VOID__BOOLEAN,
		              G_TYPE_NONE, 1,
			      G_TYPE_BOOLEAN);
}

BurnerPlugin *
burner_plugin_new (const gchar *path)
{
	if (!path)
		return NULL;

	return BURNER_PLUGIN (g_object_new (BURNER_TYPE_PLUGIN,
					     "path", path,
					     NULL));
}
