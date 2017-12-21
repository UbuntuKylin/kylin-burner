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
#include <string.h>

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <glib.h>
#include <glib/gi18n-lib.h>
#include <glib/gstdio.h>

#include <gst/gst.h>
#include <gst/pbutils/pbutils.h>

#include "burner-io.h"

#include "burn-basics.h"
#include "burn-debug.h"
#include "burn-caps.h"
#include "burn-plugin-manager.h"
#include "burner-plugin-information.h"

#include "burner-drive.h"
#include "burner-medium-monitor.h"

#include "burner-burn-lib.h"
#include "burn-caps.h"

static BurnerPluginManager *plugin_manager = NULL;
static BurnerMediumMonitor *medium_manager = NULL;
static BurnerBurnCaps *default_caps = NULL;


GQuark
burner_burn_quark (void)
{
	static GQuark quark = 0;

	if (!quark)
		quark = g_quark_from_static_string ("BurnerBurnError");

	return quark;
}
 
const gchar *
burner_burn_action_to_string (BurnerBurnAction action)
{
	gchar *strings [BURNER_BURN_ACTION_LAST] = { 	"",
							N_("Getting size"),
							N_("Creating image"),
							N_("Writing"),
							N_("Blanking"),
							N_("Creating checksum"),
							N_("Copying disc"),
							N_("Copying file"),
							N_("Analysing audio files"),
							N_("Transcoding song"),
							N_("Preparing to write"),
							N_("Writing leadin"),
							N_("Writing CD-Text information"),
							N_("Finalizing"),
							N_("Writing leadout"),
						        N_("Starting to record"),
							N_("Success"),
							N_("Ejecting medium")};
	return _(strings [action]);
}

/**
 * utility functions
 */

gboolean
burner_check_flags_for_drive (BurnerDrive *drive,
			       BurnerBurnFlag flags)
{
	BurnerMedia media;
	BurnerMedium *medium;

	if (!drive)
		return TRUE;

	if (burner_drive_is_fake (drive))
		return TRUE;

	medium = burner_drive_get_medium (drive);
	if (!medium)
		return TRUE;

	media = burner_medium_get_status (medium);
	if (flags & BURNER_BURN_FLAG_DUMMY) {
		/* This is always FALSE */
		if (media & BURNER_MEDIUM_PLUS)
			return FALSE;

		if (media & BURNER_MEDIUM_DVD) {
			if (!burner_medium_can_use_dummy_for_sao (medium))
				return FALSE;
		}
		else if (flags & BURNER_BURN_FLAG_DAO) {
			if (!burner_medium_can_use_dummy_for_sao (medium))
				return FALSE;
		}
		else if (!burner_medium_can_use_dummy_for_tao (medium))
			return FALSE;
	}

	if (flags & BURNER_BURN_FLAG_BURNPROOF) {
		if (!burner_medium_can_use_burnfree (medium))
			return FALSE;
	}

	return TRUE;
}

gchar *
burner_string_get_localpath (const gchar *uri)
{
	gchar *localpath;
	gchar *realuri;
	GFile *file;

	if (!uri)
		return NULL;

	if (uri [0] == '/')
		return g_strdup (uri);

	if (strncmp (uri, "file://", 7))
		return NULL;

	file = g_file_new_for_commandline_arg (uri);
	realuri = g_file_get_uri (file);
	g_object_unref (file);

	localpath = g_filename_from_uri (realuri, NULL, NULL);
	g_free (realuri);

	return localpath;
}

gchar *
burner_string_get_uri (const gchar *uri)
{
	gchar *uri_return;
	GFile *file;

	if (!uri)
		return NULL;

	if (uri [0] != '/')
		return g_strdup (uri);

	file = g_file_new_for_commandline_arg (uri);
	uri_return = g_file_get_uri (file);
	g_object_unref (file);

	return uri_return;
}

static void
burner_caps_list_dump (void)
{
	GSList *iter;
	BurnerBurnCaps *self;

	self = burner_burn_caps_get_default ();
	for (iter = self->priv->caps_list; iter; iter = iter->next) {
		BurnerCaps *caps;

		caps = iter->data;
		BURNER_BURN_LOG_WITH_TYPE (&caps->type,
					    caps->flags,
					    "Created %i links pointing to",
					    g_slist_length (caps->links));
	}

	g_object_unref (self);
}

/**
 * burner_burn_library_start:
 * @argc: an #int.
 * @argv: a #char **.
 *
 * Starts the library. This function must be called
 * before using any of the functions.
 *
 * Rename to: init
 *
 * Returns: a #gboolean
 **/

gboolean
burner_burn_library_start (int *argc,
                            char **argv [])
{
	BURNER_BURN_LOG ("Initializing Burner-burn %i.%i.%i",
			  BURNER_MAJOR_VERSION,
			  BURNER_MINOR_VERSION,
			  BURNER_SUB);

#if defined(HAVE_STRUCT_USCSI_CMD)
	/* Work around: because on OpenSolaris burner possibly be run
	 * as root for a user with 'Primary Administrator' profile,
	 * a root dbus session will be autospawned at that time.
	 * This fix is to work around
	 * http://bugzilla.gnome.org/show_bug.cgi?id=526454
	 */
	g_setenv ("DBUS_SESSION_BUS_ADDRESS", "autolaunch:", TRUE);
#endif

	/* Initialize external libraries (threads...) */
	if (!g_thread_supported ())
		g_thread_init (NULL);

	/* ... and GStreamer) */
	if (!gst_init_check (argc, argv, NULL))
		return FALSE;

	/* This is for missing codec automatic install */
	gst_pb_utils_init ();

	/* initialize the media library */
	burner_media_library_start ();

	/* initialize all device list */
	if (!medium_manager)
		medium_manager = burner_medium_monitor_get_default ();

	/* initialize plugins */
	if (!default_caps)
		default_caps = BURNER_BURNCAPS (g_object_new (BURNER_TYPE_BURNCAPS, NULL));

	if (!plugin_manager)
		plugin_manager = burner_plugin_manager_get_default ();

	burner_caps_list_dump ();
	return TRUE;
}

BurnerBurnCaps *
burner_burn_caps_get_default ()
{
	if (!default_caps)
		g_error ("You must call burner_burn_library_start () before using API from libburner-burn");

	g_object_ref (default_caps);
	return default_caps;
}

/**
 * burner_burn_library_get_plugins_list:
 * 
 * This function returns the list of plugins that 
 * are available to libburner-burn.
 *
 * Returns: (element-type GObject.Object) (transfer full):a #GSList that must be destroyed when not needed and each object unreffed.
 **/

GSList *
burner_burn_library_get_plugins_list (void)
{
	plugin_manager = burner_plugin_manager_get_default ();
	return burner_plugin_manager_get_plugins_list (plugin_manager);
}

/**
 * burner_burn_library_stop:
 *
 * Stop the library. Don't use any of the functions or
 * objects afterwards
 *
 * Rename to: deinit
 *
 **/
void
burner_burn_library_stop (void)
{
	if (plugin_manager) {
		g_object_unref (plugin_manager);
		plugin_manager = NULL;
	}

	if (medium_manager) {
		g_object_unref (medium_manager);
		medium_manager = NULL;
	}

	if (default_caps) {
		g_object_unref (default_caps);
		default_caps = NULL;
	}

	/* Cleanup the io thing */
	burner_io_shutdown ();
}

/**
 * burner_burn_library_can_checksum:
 *
 * Checks whether the library can do any kind of
 * checksum at all.
 *
 * Returns: a #gboolean
 */

gboolean
burner_burn_library_can_checksum (void)
{
	GSList *iter;
	BurnerBurnCaps *self;

	self = burner_burn_caps_get_default ();

	if (self->priv->tests == NULL) {
		g_object_unref (self);
		return FALSE;
	}

	for (iter = self->priv->tests; iter; iter = iter->next) {
		BurnerCapsTest *tmp;
		GSList *links;

		tmp = iter->data;
		for (links = tmp->links; links; links = links->next) {
			BurnerCapsLink *link;

			link = links->data;
			if (burner_caps_link_active (link, 0)) {
				g_object_unref (self);
				return TRUE;
			}
		}
	}

	g_object_unref (self);
	return FALSE;
}

/**
 * burner_burn_library_input_supported:
 * @type: a #BurnerTrackType
 *
 * Checks whether @type can be used as input.
 *
 * Returns: a #BurnerBurnResult
 */

BurnerBurnResult
burner_burn_library_input_supported (BurnerTrackType *type)
{
	GSList *iter;
	BurnerBurnCaps *self;

	g_return_val_if_fail (type != NULL, BURNER_BURN_ERR);

	self = burner_burn_caps_get_default ();

	for (iter = self->priv->caps_list; iter; iter = iter->next) {
		BurnerCaps *caps;

		caps = iter->data;

		if (burner_caps_is_compatible_type (caps, type)
		&&  burner_burn_caps_is_input (self, caps)) {
			g_object_unref (self);
			return BURNER_BURN_OK;
		}
	}

	g_object_unref (self);
	return BURNER_BURN_ERR;
}

/**
 * burner_burn_library_get_media_capabilities:
 * @media: a #BurnerMedia
 *
 * Used to test what the library can do based on the medium type.
 * Returns BURNER_MEDIUM_WRITABLE if the disc can be written
 * and / or BURNER_MEDIUM_REWRITABLE if the disc can be erased.
 *
 * Returns: a #BurnerMedia
 */

BurnerMedia
burner_burn_library_get_media_capabilities (BurnerMedia media)
{
	GSList *iter;
	GSList *links;
	BurnerMedia retval;
	BurnerBurnCaps *self;
	BurnerCaps *caps = NULL;

	self = burner_burn_caps_get_default ();

	retval = BURNER_MEDIUM_NONE;
	BURNER_BURN_LOG_DISC_TYPE (media, "checking media caps for");

	/* we're only interested in DISC caps. There should be only one caps fitting */
	for (iter = self->priv->caps_list; iter; iter = iter->next) {
		caps = iter->data;
		if (caps->type.type != BURNER_TRACK_TYPE_DISC)
			continue;

		if ((media & caps->type.subtype.media) == media)
			break;

		caps = NULL;
	}

	if (!caps) {
		g_object_unref (self);
		return BURNER_MEDIUM_NONE;
	}

	/* check the links */
	for (links = caps->links; links; links = links->next) {
		GSList *plugins;
		gboolean active;
		BurnerCapsLink *link;

		link = links->data;

		/* this link must have at least one active plugin to be valid
		 * plugins are not sorted but in this case we don't need them
		 * to be. we just need one active if another is with a better
		 * priority all the better. */
		active = FALSE;
		for (plugins = link->plugins; plugins; plugins = plugins->next) {
			BurnerPlugin *plugin;

			plugin = plugins->data;
			/* Ignore plugin errors */
			if (burner_plugin_get_active (plugin, TRUE)) {
				/* this link is valid */
				active = TRUE;
				break;
			}
		}

		if (!active)
			continue;

		if (!link->caps) {
			/* means that it can be blanked */
			retval |= BURNER_MEDIUM_REWRITABLE;
			continue;
		}

		/* means it can be written. NOTE: if this disc has already some
		 * data on it, it even means it can be appended */
		retval |= BURNER_MEDIUM_WRITABLE;
	}

	g_object_unref (self);
	return retval;
}

