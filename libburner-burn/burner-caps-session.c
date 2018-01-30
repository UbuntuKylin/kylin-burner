/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/*
 * Libburner-burn
 * Copyright (C) Philippe Rouquier 2005-2009 <bonfire-app@wanadoo.fr>
 * Copyright (C) 2017,Tianjin KYLIN Information Technology Co., Ltd.
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
#include <glib/gi18n.h>

#include "burn-caps.h"
#include "burn-debug.h"
#include "burner-plugin.h"
#include "burner-plugin-private.h"
#include "burner-plugin-information.h"
#include "burner-session-helper.h"

#define BURNER_BURN_CAPS_SHOULD_BLANK(media_MACRO, flags_MACRO)		\
	((media_MACRO & BURNER_MEDIUM_UNFORMATTED) ||				\
	((media_MACRO & (BURNER_MEDIUM_HAS_AUDIO|BURNER_MEDIUM_HAS_DATA)) &&	\
	 (flags_MACRO & (BURNER_BURN_FLAG_MERGE|BURNER_BURN_FLAG_APPEND)) == FALSE))

#define BURNER_BURN_CAPS_NOT_SUPPORTED_LOG_RES(session)			\
{										\
	burner_burn_session_log (session, "Unsupported type of task operation"); \
	BURNER_BURN_LOG ("Unsupported type of task operation");		\
	return BURNER_BURN_NOT_SUPPORTED;					\
}

static BurnerBurnResult
burner_burn_caps_get_blanking_flags_real (BurnerBurnCaps *caps,
                                           gboolean ignore_errors,
					   BurnerMedia media,
					   BurnerBurnFlag session_flags,
					   BurnerBurnFlag *supported,
					   BurnerBurnFlag *compulsory)
{
	GSList *iter;
	gboolean supported_media;
	BurnerBurnFlag supported_flags = BURNER_BURN_FLAG_NONE;
	BurnerBurnFlag compulsory_flags = BURNER_BURN_FLAG_ALL;

	BURNER_BURN_LOG_DISC_TYPE (media, "Getting blanking flags for");
	if (media == BURNER_MEDIUM_NONE) {
		BURNER_BURN_LOG ("Blanking not possible: no media");
		if (supported)
			*supported = BURNER_BURN_FLAG_NONE;
		if (compulsory)
			*compulsory = BURNER_BURN_FLAG_NONE;
		return BURNER_BURN_NOT_SUPPORTED;
	}

	supported_media = FALSE;
	for (iter = caps->priv->caps_list; iter; iter = iter->next) {
		BurnerMedia caps_media;
		BurnerCaps *caps;
		GSList *links;

		caps = iter->data;
		if (!burner_track_type_get_has_medium (&caps->type))
			continue;

		caps_media = burner_track_type_get_medium_type (&caps->type);
		if ((media & caps_media) != media)
			continue;

		for (links = caps->links; links; links = links->next) {
			GSList *plugins;
			BurnerCapsLink *link;

			link = links->data;

			if (link->caps != NULL)
				continue;

			supported_media = TRUE;
			/* don't need the plugins to be sorted since we go
			 * through all the plugin list to get all blanking flags
			 * available. */
			for (plugins = link->plugins; plugins; plugins = plugins->next) {
				BurnerPlugin *plugin;
				BurnerBurnFlag supported_plugin;
				BurnerBurnFlag compulsory_plugin;

				plugin = plugins->data;
				if (!burner_plugin_get_active (plugin, ignore_errors))
					continue;

				if (!burner_plugin_get_blank_flags (plugin,
								     media,
								     session_flags,
								     &supported_plugin,
								     &compulsory_plugin))
					continue;

				supported_flags |= supported_plugin;
				compulsory_flags &= compulsory_plugin;
			}
		}
	}

	if (!supported_media) {
		BURNER_BURN_LOG ("media blanking not supported");
		return BURNER_BURN_NOT_SUPPORTED;
	}

	/* This is a special case that is in MMC specs:
	 * DVD-RW sequential must be fully blanked
	 * if we really want multisession support. */
	if (BURNER_MEDIUM_IS (media, BURNER_MEDIUM_DVDRW)
	&& (session_flags & BURNER_BURN_FLAG_MULTI)) {
		if (compulsory_flags & BURNER_BURN_FLAG_FAST_BLANK) {
			BURNER_BURN_LOG ("fast media blanking only supported but multisession required for DVDRW");
			return BURNER_BURN_NOT_SUPPORTED;
		}

		supported_flags &= ~BURNER_BURN_FLAG_FAST_BLANK;

		BURNER_BURN_LOG ("removed fast blank for a DVDRW with multisession");
	}

	if (supported)
		*supported = supported_flags;
	if (compulsory)
		*compulsory = compulsory_flags;

	return BURNER_BURN_OK;
}

/**
 * burner_burn_session_get_blank_flags:
 * @session: a #BurnerBurnSession
 * @supported: a #BurnerBurnFlag
 * @compulsory: a #BurnerBurnFlag
 *
 * Given the various parameters stored in @session,
 * stores in @supported and @compulsory, the flags
 * that can be used (@supported) and must be used
 * (@compulsory) when blanking the medium in the
 * #BurnerDrive (set with burner_burn_session_set_burner ()).
 *
 * Return value: a #BurnerBurnResult.
 * BURNER_BURN_OK if the retrieval was successful.
 * BURNER_BURN_ERR otherwise.
 **/

BurnerBurnResult
burner_burn_session_get_blank_flags (BurnerBurnSession *session,
				      BurnerBurnFlag *supported,
				      BurnerBurnFlag *compulsory)
{
	BurnerMedia media;
	BurnerBurnCaps *self;
	BurnerBurnResult result;
	BurnerBurnFlag session_flags;

	media = burner_burn_session_get_dest_media (session);
	BURNER_BURN_LOG_DISC_TYPE (media, "Getting blanking flags for");

	if (media == BURNER_MEDIUM_NONE) {
		BURNER_BURN_LOG ("Blanking not possible: no media");
		if (supported)
			*supported = BURNER_BURN_FLAG_NONE;
		if (compulsory)
			*compulsory = BURNER_BURN_FLAG_NONE;
		return BURNER_BURN_NOT_SUPPORTED;
	}

	session_flags = burner_burn_session_get_flags (session);

	self = burner_burn_caps_get_default ();
	result = burner_burn_caps_get_blanking_flags_real (self,
	                                                    burner_burn_session_get_strict_support (session) == FALSE,
							    media,
							    session_flags,
							    supported,
							    compulsory);
	g_object_unref (self);

	return result;
}

static BurnerBurnResult
burner_burn_caps_can_blank_real (BurnerBurnCaps *self,
                                  gboolean ignore_plugin_errors,
                                  BurnerMedia media,
				  BurnerBurnFlag flags)
{
	GSList *iter;

	BURNER_BURN_LOG_DISC_TYPE (media, "Testing blanking caps for");
	if (media == BURNER_MEDIUM_NONE) {
		BURNER_BURN_LOG ("no media => no blanking possible");
		return BURNER_BURN_NOT_SUPPORTED;
	}

	/* This is a special case from MMC: DVD-RW sequential
	 * can only be multisession is they were fully blanked
	 * so if there are the two flags, abort. */
	if (BURNER_MEDIUM_IS (media, BURNER_MEDIUM_DVDRW)
	&&  (flags & BURNER_BURN_FLAG_MULTI)
	&&  (flags & BURNER_BURN_FLAG_FAST_BLANK)) {
		BURNER_BURN_LOG ("fast media blanking only supported but multisession required for DVD-RW");
		return BURNER_BURN_NOT_SUPPORTED;
	}

	for (iter = self->priv->caps_list; iter; iter = iter->next) {
		BurnerCaps *caps;
		GSList *links;

		caps = iter->data;
		if (caps->type.type != BURNER_TRACK_TYPE_DISC)
			continue;

		if ((media & burner_track_type_get_medium_type (&caps->type)) != media)
			continue;

		BURNER_BURN_LOG_TYPE (&caps->type, "Searching links for caps");

		for (links = caps->links; links; links = links->next) {
			GSList *plugins;
			BurnerCapsLink *link;

			link = links->data;

			if (link->caps != NULL)
				continue;

			BURNER_BURN_LOG ("Searching plugins");

			/* Go through all plugins for the link and stop if we 
			 * find at least one active plugin that accepts the
			 * flags. No need for plugins to be sorted */
			for (plugins = link->plugins; plugins; plugins = plugins->next) {
				BurnerPlugin *plugin;

				plugin = plugins->data;
				if (!burner_plugin_get_active (plugin, ignore_plugin_errors))
					continue;

				if (burner_plugin_check_blank_flags (plugin, media, flags)) {
					BURNER_BURN_LOG_DISC_TYPE (media, "Can blank");
					return BURNER_BURN_OK;
				}
			}
		}
	}

	BURNER_BURN_LOG_DISC_TYPE (media, "No blanking capabilities for");

	return BURNER_BURN_NOT_SUPPORTED;
}

/**
 * burner_burn_session_can_blank:
 * @session: a #BurnerBurnSession
 *
 * Given the various parameters stored in @session, this
 * function checks whether the medium in the
 * #BurnerDrive (set with burner_burn_session_set_burner ())
 * can be blanked.
 *
 * Return value: a #BurnerBurnResult.
 * BURNER_BURN_OK if it is possible.
 * BURNER_BURN_ERR otherwise.
 **/

BurnerBurnResult
burner_burn_session_can_blank (BurnerBurnSession *session)
{
	BurnerMedia media;
	BurnerBurnFlag flags;
	BurnerBurnCaps *self;
	BurnerBurnResult result;

	self = burner_burn_caps_get_default ();

	media = burner_burn_session_get_dest_media (session);
	BURNER_BURN_LOG_DISC_TYPE (media, "Testing blanking caps for");

	if (media == BURNER_MEDIUM_NONE) {
		BURNER_BURN_LOG ("no media => no blanking possible");
		g_object_unref (self);
		return BURNER_BURN_NOT_SUPPORTED;
	}

	flags = burner_burn_session_get_flags (session);
	result = burner_burn_caps_can_blank_real (self,
	                                           burner_burn_session_get_strict_support (session) == FALSE,
	                                           media,
	                                           flags);
	g_object_unref (self);

	return result;
}

static void
burner_caps_link_get_record_flags (BurnerCapsLink *link,
                                    gboolean ignore_plugin_errors,
				    BurnerMedia media,
				    BurnerBurnFlag session_flags,
				    BurnerBurnFlag *supported,
				    BurnerBurnFlag *compulsory_retval)
{
	GSList *iter;
	BurnerBurnFlag compulsory;

	compulsory = BURNER_BURN_FLAG_ALL;

	/* Go through all plugins to get the supported/... record flags for link */
	for (iter = link->plugins; iter; iter = iter->next) {
		gboolean result;
		BurnerPlugin *plugin;
		BurnerBurnFlag plugin_supported;
		BurnerBurnFlag plugin_compulsory;

		plugin = iter->data;
		if (!burner_plugin_get_active (plugin, ignore_plugin_errors))
			continue;

		result = burner_plugin_get_record_flags (plugin,
							  media,
							  session_flags,
							  &plugin_supported,
							  &plugin_compulsory);
		if (!result)
			continue;

		*supported |= plugin_supported;
		compulsory &= plugin_compulsory;
	}

	*compulsory_retval = compulsory;
}

static void
burner_caps_link_get_data_flags (BurnerCapsLink *link,
                                  gboolean ignore_plugin_errors,
				  BurnerMedia media,
				  BurnerBurnFlag session_flags,
				  BurnerBurnFlag *supported)
{
	GSList *iter;

	/* Go through all plugins the get the supported/... data flags for link */
	for (iter = link->plugins; iter; iter = iter->next) {
		BurnerPlugin *plugin;
		BurnerBurnFlag plugin_supported;
		BurnerBurnFlag plugin_compulsory;

		plugin = iter->data;
		if (!burner_plugin_get_active (plugin, ignore_plugin_errors))
			continue;

		burner_plugin_get_image_flags (plugin,
		                                media,
		                                session_flags,
		                                &plugin_supported,
		                                &plugin_compulsory);
		*supported |= plugin_supported;
	}
}

static gboolean
burner_caps_link_check_data_flags (BurnerCapsLink *link,
                                    gboolean ignore_plugin_errors,
				    BurnerBurnFlag session_flags,
				    BurnerMedia media)
{
	GSList *iter;
	BurnerBurnFlag flags;

	/* here we just make sure that at least one of the plugins in the link
	 * can comply with the flags (APPEND/MERGE) */
	flags = session_flags & (BURNER_BURN_FLAG_APPEND|BURNER_BURN_FLAG_MERGE);

	/* If there are no image flags forget it */
	if (flags == BURNER_BURN_FLAG_NONE)
		return TRUE;

	/* Go through all plugins; at least one must support image flags */
	for (iter = link->plugins; iter; iter = iter->next) {
		gboolean result;
		BurnerPlugin *plugin;

		plugin = iter->data;
		if (!burner_plugin_get_active (plugin, ignore_plugin_errors))
			continue;

		result = burner_plugin_check_image_flags (plugin,
							   media,
							   session_flags);
		if (result)
			return TRUE;
	}

	return FALSE;
}

static gboolean
burner_caps_link_check_record_flags (BurnerCapsLink *link,
                                      gboolean ignore_plugin_errors,
				      BurnerBurnFlag session_flags,
				      BurnerMedia media)
{
	GSList *iter;
	BurnerBurnFlag flags;

	flags = session_flags & BURNER_PLUGIN_BURN_FLAG_MASK;

	/* If there are no record flags forget it */
	if (flags == BURNER_BURN_FLAG_NONE)
		return TRUE;
	
	/* Go through all plugins: at least one must support record flags */
	for (iter = link->plugins; iter; iter = iter->next) {
		gboolean result;
		BurnerPlugin *plugin;

		plugin = iter->data;
		if (!burner_plugin_get_active (plugin, ignore_plugin_errors))
			continue;

		result = burner_plugin_check_record_flags (plugin,
							    media,
							    session_flags);
		if (result)
			return TRUE;
	}

	return FALSE;
}

static gboolean
burner_caps_link_check_media_restrictions (BurnerCapsLink *link,
                                            gboolean ignore_plugin_errors,
					    BurnerMedia media)
{
	GSList *iter;

	/* Go through all plugins: at least one must support media */
	for (iter = link->plugins; iter; iter = iter->next) {
		gboolean result;
		BurnerPlugin *plugin;

		plugin = iter->data;
		if (!burner_plugin_get_active (plugin, ignore_plugin_errors))
			continue;

		result = burner_plugin_check_media_restrictions (plugin, media);
		if (result)
			return TRUE;
	}

	return FALSE;
}

static BurnerBurnResult
burner_caps_report_plugin_error (BurnerPlugin *plugin,
                                  BurnerForeachPluginErrorCb callback,
                                  gpointer user_data)
{
	GSList *errors;

	errors = burner_plugin_get_errors (plugin);
	if (!errors)
		return BURNER_BURN_ERR;

	do {
		BurnerPluginError *error;
		BurnerBurnResult result;

		error = errors->data;
		result = callback (error->type, error->detail, user_data);
		if (result == BURNER_BURN_RETRY) {
			/* Something has been done
			 * to fix the error like an install
			 * so reload the errors */
			burner_plugin_check_plugin_ready (plugin);
			errors = burner_plugin_get_errors (plugin);
			continue;
		}

		if (result != BURNER_BURN_OK)
			return result;

		errors = errors->next;
	} while (errors);

	return BURNER_BURN_OK;
}

struct _BurnerFindLinkCtx {
	BurnerMedia media;
	BurnerTrackType *input;
	BurnerPluginIOFlag io_flags;
	BurnerBurnFlag session_flags;

	BurnerForeachPluginErrorCb callback;
	gpointer user_data;

	guint ignore_plugin_errors:1;
	guint check_session_flags:1;
};
typedef struct _BurnerFindLinkCtx BurnerFindLinkCtx;

static void
burner_caps_find_link_set_ctx (BurnerBurnSession *session,
                                BurnerFindLinkCtx *ctx,
                                BurnerTrackType *input)
{
	ctx->input = input;

	if (ctx->check_session_flags) {
		ctx->session_flags = burner_burn_session_get_flags (session);

		if (BURNER_BURN_SESSION_NO_TMP_FILE (session))
			ctx->io_flags = BURNER_PLUGIN_IO_ACCEPT_PIPE;
		else
			ctx->io_flags = BURNER_PLUGIN_IO_ACCEPT_FILE;
	}
	else
		ctx->io_flags = BURNER_PLUGIN_IO_ACCEPT_FILE|
					BURNER_PLUGIN_IO_ACCEPT_PIPE;

	if (!ctx->callback)
		ctx->ignore_plugin_errors = (burner_burn_session_get_strict_support (session) == FALSE);
	else
		ctx->ignore_plugin_errors = TRUE;
}

static BurnerBurnResult
burner_caps_find_link (BurnerCaps *caps,
                        BurnerFindLinkCtx *ctx)
{
	GSList *iter;

	BURNER_BURN_LOG_WITH_TYPE (&caps->type, BURNER_PLUGIN_IO_NONE, "Found link (with %i links):", g_slist_length (caps->links));

	/* Here we only make sure we have at least one link working. For a link
	 * to be followed it must first:
	 * - link to a caps with correct io flags
	 * - have at least a plugin accepting the record flags if caps type is
	 *   a disc (that means that the link is the recording part)
	 *
	 * and either:
	 * - link to a caps equal to the input
	 * - link to a caps (linking itself to another caps, ...) accepting the
	 *   input
	 */

	for (iter = caps->links; iter; iter = iter->next) {
		BurnerCapsLink *link;
		BurnerBurnResult result;

		link = iter->data;

		if (!link->caps)
			continue;
		
		/* check that the link has some active plugin */
		if (!burner_caps_link_active (link, ctx->ignore_plugin_errors))
			continue;

		/* since this link contains recorders, check that at least one
		 * of them can handle the record flags */
		if (ctx->check_session_flags && burner_track_type_get_has_medium (&caps->type)) {
			if (!burner_caps_link_check_record_flags (link, ctx->ignore_plugin_errors, ctx->session_flags, ctx->media))
				continue;

			if (burner_caps_link_check_recorder_flags_for_input (link, ctx->session_flags) != BURNER_BURN_OK)
				continue;
		}

		/* first see if that's the perfect fit:
		 * - it must have the same caps (type + subtype)
		 * - it must have the proper IO */
		if (burner_track_type_get_has_data (&link->caps->type)) {
			if (ctx->check_session_flags
			&& !burner_caps_link_check_data_flags (link, ctx->ignore_plugin_errors, ctx->session_flags, ctx->media))
				continue;
		}
		else if (!burner_caps_link_check_media_restrictions (link, ctx->ignore_plugin_errors, ctx->media))
			continue;

		if ((link->caps->flags & BURNER_PLUGIN_IO_ACCEPT_FILE)
		&&   burner_caps_is_compatible_type (link->caps, ctx->input)) {
			if (ctx->callback) {
				BurnerPlugin *plugin;

				/* If we are supposed to report/signal that the plugin
				 * could be used but only if some more elements are 
				 * installed */
				plugin = burner_caps_link_need_download (link);
				if (plugin)
					return burner_caps_report_plugin_error (plugin, ctx->callback, ctx->user_data);
			}
			return BURNER_BURN_OK;
		}

		/* we can't go further than a DISC type */
		if (burner_track_type_get_has_medium (&link->caps->type))
			continue;

		if ((link->caps->flags & ctx->io_flags) == BURNER_PLUGIN_IO_NONE)
			continue;

		/* try to see where the inputs of this caps leads to */
		result = burner_caps_find_link (link->caps, ctx);
		if (result == BURNER_BURN_CANCEL)
			return result;

		if (result == BURNER_BURN_OK) {
			if (ctx->callback) {
				BurnerPlugin *plugin;

				/* If we are supposed to report/signal that the plugin
				 * could be used but only if some more elements are 
				 * installed */
				plugin = burner_caps_link_need_download (link);
				if (plugin)
					return burner_caps_report_plugin_error (plugin, ctx->callback, ctx->user_data);
			}
			return BURNER_BURN_OK;
		}
	}

	return BURNER_BURN_NOT_SUPPORTED;
}

static BurnerBurnResult
burner_caps_try_output (BurnerBurnCaps *self,
                         BurnerFindLinkCtx *ctx,
                         BurnerTrackType *output)
{
	BurnerCaps *caps;

	/* here we search the start caps */
	caps = burner_burn_caps_find_start_caps (self, output);
	if (!caps) {
		BURNER_BURN_LOG ("No caps available");
		return BURNER_BURN_NOT_SUPPORTED;
	}

	/* Here flags don't matter as we don't record anything.
	 * Even the IOFlags since that can be checked later with
	 * burner_burn_caps_get_flags. */
	if (burner_track_type_get_has_medium (output))
		ctx->media = burner_track_type_get_medium_type (output);
	else
		ctx->media = BURNER_MEDIUM_FILE;

	return burner_caps_find_link (caps, ctx);
}

static BurnerBurnResult
burner_caps_try_output_with_blanking (BurnerBurnCaps *self,
                                       BurnerBurnSession *session,
                                       BurnerFindLinkCtx *ctx,
                                       BurnerTrackType *output)
{
	gboolean result;
	BurnerCaps *last_caps;

	result = burner_caps_try_output (self, ctx, output);
	if (result == BURNER_BURN_OK
	||  result == BURNER_BURN_CANCEL)
		return result;

	/* we reached this point in two cases:
	 * - if the disc cannot be handled
	 * - if some flags are not handled
	 * It is helpful only if:
	 * - the disc was closed and no plugin can handle this type of 
	 * disc once closed (CD-R(W))
	 * - there was the flag BLANK_BEFORE_WRITE set and no plugin can
	 * handle this flag (means that the plugin should erase and
	 * then write on its own. Basically that works only with
	 * overwrite formatted discs, DVD+RW, ...) */
	if (!burner_track_type_get_has_medium (output))
		return BURNER_BURN_NOT_SUPPORTED;

	/* output is a disc try with initial blanking */
	BURNER_BURN_LOG ("Support for input/output failed.");

	/* apparently nothing can be done to reach our goal. Maybe that
	 * is because we first have to blank the disc. If so add a blank 
	 * task to the others as a first step */
	if ((ctx->check_session_flags
	&& !(ctx->session_flags & BURNER_BURN_FLAG_BLANK_BEFORE_WRITE))
	||   burner_burn_session_can_blank (session) != BURNER_BURN_OK)
		return BURNER_BURN_NOT_SUPPORTED;

	BURNER_BURN_LOG ("Trying with initial blanking");

	/* retry with the same disc type but blank this time */
	ctx->media = burner_track_type_get_medium_type (output);
	ctx->media &= ~(BURNER_MEDIUM_CLOSED|
	                BURNER_MEDIUM_APPENDABLE|
	                BURNER_MEDIUM_UNFORMATTED|
	                BURNER_MEDIUM_HAS_DATA|
	                BURNER_MEDIUM_HAS_AUDIO);
	ctx->media |= BURNER_MEDIUM_BLANK;
	burner_track_type_set_medium_type (output, ctx->media);

	last_caps = burner_burn_caps_find_start_caps (self, output);
	if (!last_caps)
		return BURNER_BURN_NOT_SUPPORTED;

	return burner_caps_find_link (last_caps, ctx);
}

/**
 * burner_burn_session_input_supported:
 * @session: a #BurnerBurnSession
 * @input: a #BurnerTrackType
 * @check_flags: a #gboolean
 *
 * Given the various parameters stored in @session, this
 * function checks whether a session with the data type
 * @type could be burnt to the medium in the #BurnerDrive (set 
 * through burner_burn_session_set_burner ()).
 * If @check_flags is %TRUE, then flags are taken into account
 * and are not if it is %FALSE.
 *
 * Return value: a #BurnerBurnResult.
 * BURNER_BURN_OK if it is possible.
 * BURNER_BURN_ERR otherwise.
 **/

BurnerBurnResult
burner_burn_session_input_supported (BurnerBurnSession *session,
				      BurnerTrackType *input,
                                      gboolean check_flags)
{
	BurnerBurnCaps *self;
	BurnerBurnResult result;
	BurnerTrackType output;
	BurnerFindLinkCtx ctx = { 0, NULL, 0, };

	result = burner_burn_session_get_output_type (session, &output);
	if (result != BURNER_BURN_OK)
		return result;

	BURNER_BURN_LOG_TYPE (input, "Checking support for input");
	BURNER_BURN_LOG_TYPE (&output, "and output");

	ctx.check_session_flags = check_flags;
	burner_caps_find_link_set_ctx (session, &ctx, input);

	if (check_flags) {
		result = burner_check_flags_for_drive (burner_burn_session_get_burner (session),
							ctx.session_flags);

		if (!result)
			BURNER_BURN_CAPS_NOT_SUPPORTED_LOG_RES (session);

		BURNER_BURN_LOG_FLAGS (ctx.session_flags, "with flags");
	}

	self = burner_burn_caps_get_default ();
	result = burner_caps_try_output_with_blanking (self,
							session,
	                                                &ctx,
							&output);
	g_object_unref (self);

	if (result != BURNER_BURN_OK) {
		BURNER_BURN_LOG_TYPE (input, "Input not supported");
		return result;
	}

	return BURNER_BURN_OK;
}

/**
 * burner_burn_session_output_supported:
 * @session: a #BurnerBurnSession *
 * @output: a #BurnerTrackType *
 *
 * Make sure that the image type or medium type defined in @output can be
 * created/burnt given the parameters and the current data set in @session.
 *
 * Return value: BURNER_BURN_OK if the medium type or the image type can be used as an output.
 **/
BurnerBurnResult
burner_burn_session_output_supported (BurnerBurnSession *session,
				       BurnerTrackType *output)
{
	BurnerBurnCaps *self;
	BurnerTrackType input;
	BurnerBurnResult result;
	BurnerFindLinkCtx ctx = { 0, NULL, 0, };

	/* Here, we can't check if the drive supports the flags since the output
	 * is hypothetical. There is no real medium. So forget the following :
	 * if (!burner_burn_caps_flags_check_for_drive (session))
	 *	BURNER_BURN_CAPS_NOT_SUPPORTED_LOG_RES (session);
	 * The only thing we could do would be to check some known forbidden 
	 * flags for some media provided the output type is DISC. */

	burner_burn_session_get_input_type (session, &input);
	burner_caps_find_link_set_ctx (session, &ctx, &input);

	BURNER_BURN_LOG_TYPE (output, "Checking support for output");
	BURNER_BURN_LOG_TYPE (&input, "and input");
	BURNER_BURN_LOG_FLAGS (burner_burn_session_get_flags (session), "with flags");
	
	self = burner_burn_caps_get_default ();
	result = burner_caps_try_output_with_blanking (self,
							session,
	                                                &ctx,
							output);
	g_object_unref (self);

	if (result != BURNER_BURN_OK) {
		BURNER_BURN_LOG_TYPE (output, "Output not supported");
		return result;
	}

	return BURNER_BURN_OK;
}

/**
 * This is only to be used in case one wants to copy using the same drive.
 * It determines the possible middle image type.
 */

static BurnerBurnResult
burner_burn_caps_is_session_supported_same_src_dest (BurnerBurnCaps *self,
						      BurnerBurnSession *session,
                                                      BurnerFindLinkCtx *ctx,
                                                      BurnerTrackType *tmp_type)
{
	GSList *iter;
	BurnerDrive *burner;
	BurnerTrackType input;
	BurnerBurnResult result;
	BurnerTrackType output;
	BurnerImageFormat format;

	BURNER_BURN_LOG ("Checking disc copy support with same source and destination");

	/* To determine if a CD/DVD can be copied using the same source/dest,
	 * we first determine if can be imaged and then if this image can be 
	 * burnt to whatever medium type. */
	burner_caps_find_link_set_ctx (session, ctx, &input);
	ctx->io_flags = BURNER_PLUGIN_IO_ACCEPT_FILE;

	memset (&input, 0, sizeof (BurnerTrackType));
	burner_burn_session_get_input_type (session, &input);
	BURNER_BURN_LOG_TYPE (&input, "input");

	if (ctx->check_session_flags) {
		/* NOTE: DAO can be a problem. So just in case remove it. It is
		 * not really useful in this context. What we want here is to
		 * know whether a medium can be used given the input; only 1
		 * flag is important here (MERGE) and can have consequences. */
		ctx->session_flags &= ~BURNER_BURN_FLAG_DAO;
		BURNER_BURN_LOG_FLAGS (ctx->session_flags, "flags");
	}

	burner = burner_burn_session_get_burner (session);

	/* First see if it works with a stream type */
	burner_track_type_set_has_stream (&output);

	/* FIXME! */
	burner_track_type_set_stream_format (&output,
	                                      BURNER_AUDIO_FORMAT_RAW|
	                                      BURNER_METADATA_INFO);

	BURNER_BURN_LOG_TYPE (&output, "Testing stream type");
	result = burner_caps_try_output (self, ctx, &output);
	if (result == BURNER_BURN_CANCEL)
		return result;

	if (result == BURNER_BURN_OK) {
		BURNER_BURN_LOG ("Stream type seems to be supported as output");

		/* This format can be used to create an image. Check if can be
		 * burnt now. Just find at least one medium. */
		for (iter = self->priv->caps_list; iter; iter = iter->next) {
			BurnerBurnResult result;
			BurnerMedia media;
			BurnerCaps *caps;

			caps = iter->data;

			if (!burner_track_type_get_has_medium (&caps->type))
				continue;

			media = burner_track_type_get_medium_type (&caps->type);
			/* Audio is only supported by CDs */
			if ((media & BURNER_MEDIUM_CD) == 0)
				continue;

			/* This type of disc cannot be burnt; skip them */
			if (media & BURNER_MEDIUM_ROM)
				continue;

			/* Make sure this is supported by the drive */
			if (!burner_drive_can_write_media (burner, media))
				continue;

			ctx->media = media;
			result = burner_caps_find_link (caps, ctx);
			BURNER_BURN_LOG_DISC_TYPE (media,
						    "Tested medium (%s)",
						    result == BURNER_BURN_OK ? "working":"not working");

			if (result == BURNER_BURN_OK) {
				if (tmp_type) {
					burner_track_type_set_has_stream (tmp_type);
					burner_track_type_set_stream_format (tmp_type, burner_track_type_get_stream_format (&output));
				}
					
				return BURNER_BURN_OK;
			}

			if (result == BURNER_BURN_CANCEL)
				return result;
		}
	}
	else
		BURNER_BURN_LOG ("Stream format not supported as output");

	/* Find one available output format */
	format = BURNER_IMAGE_FORMAT_CDRDAO;
	burner_track_type_set_has_image (&output);

	for (; format > BURNER_IMAGE_FORMAT_NONE; format >>= 1) {
		burner_track_type_set_image_format (&output, format);

		BURNER_BURN_LOG_TYPE (&output, "Testing temporary image format");

		/* Don't need to try blanking here (saves
		 * a few lines of debug) since that is an 
		 * image */
		result = burner_caps_try_output (self, ctx, &output);
		if (result == BURNER_BURN_CANCEL)
			return result;

		if (result != BURNER_BURN_OK)
			continue;

		/* This format can be used to create an image. Check if can be
		 * burnt now. Just find at least one medium. */
		for (iter = self->priv->caps_list; iter; iter = iter->next) {
			BurnerBurnResult result;
			BurnerMedia media;
			BurnerCaps *caps;

			caps = iter->data;

			if (!burner_track_type_get_has_medium (&caps->type))
				continue;

			media = burner_track_type_get_medium_type (&caps->type);

			/* This type of disc cannot be burnt; skip them */
			if (media & BURNER_MEDIUM_ROM)
				continue;

			/* These three types only work with CDs. Skip the rest. */
			if ((format == BURNER_IMAGE_FORMAT_CDRDAO
			||   format == BURNER_IMAGE_FORMAT_CLONE
			||   format == BURNER_IMAGE_FORMAT_CUE)
			&& (media & BURNER_MEDIUM_CD) == 0)
				continue;

			/* Make sure this is supported by the drive */
			if (!burner_drive_can_write_media (burner, media))
				continue;

			ctx->media = media;
			result = burner_caps_find_link (caps, ctx);
			BURNER_BURN_LOG_DISC_TYPE (media,
						    "Tested medium (%s)",
						    result == BURNER_BURN_OK ? "working":"not working");

			if (result == BURNER_BURN_OK) {
				if (tmp_type) {
					burner_track_type_set_has_image (tmp_type);
					burner_track_type_set_image_format (tmp_type, burner_track_type_get_image_format (&output));
				}
					
				return BURNER_BURN_OK;
			}

			if (result == BURNER_BURN_CANCEL)
				return result;
		}
	}

	return BURNER_BURN_NOT_SUPPORTED;
}

BurnerBurnResult
burner_burn_session_get_tmp_image_type_same_src_dest (BurnerBurnSession *session,
                                                       BurnerTrackType *image_type)
{
	BurnerBurnCaps *self;
	BurnerBurnResult result;
	BurnerFindLinkCtx ctx = { 0, NULL, 0, };

	g_return_val_if_fail (BURNER_IS_BURN_SESSION (session), BURNER_BURN_ERR);

	self = burner_burn_caps_get_default ();
	result = burner_burn_caps_is_session_supported_same_src_dest (self,
	                                                               session,
	                                                               &ctx,
	                                                               image_type);
	g_object_unref (self);
	return result;
}

static BurnerBurnResult
burner_burn_session_supported (BurnerBurnSession *session,
                                BurnerFindLinkCtx *ctx)
{
	gboolean result;
	BurnerBurnCaps *self;
	BurnerTrackType input;
	BurnerTrackType output;

	/* Special case */
	if (burner_burn_session_same_src_dest_drive (session)) {
		BurnerBurnResult res;

		self = burner_burn_caps_get_default ();
		res = burner_burn_caps_is_session_supported_same_src_dest (self, session, ctx, NULL);
		g_object_unref (self);

		return res;
	}

	result = burner_burn_session_get_output_type (session, &output);
	if (result != BURNER_BURN_OK)
		BURNER_BURN_CAPS_NOT_SUPPORTED_LOG_RES (session);

	burner_burn_session_get_input_type (session, &input);
	burner_caps_find_link_set_ctx (session, ctx, &input);

	BURNER_BURN_LOG_TYPE (&output, "Checking support for session. Output is ");
	BURNER_BURN_LOG_TYPE (&input, "and input is");

	if (ctx->check_session_flags) {
		result = burner_check_flags_for_drive (burner_burn_session_get_burner (session), ctx->session_flags);
		if (!result)
			BURNER_BURN_CAPS_NOT_SUPPORTED_LOG_RES (session);

		BURNER_BURN_LOG_FLAGS (ctx->session_flags, "with flags");
	}

	self = burner_burn_caps_get_default ();
	result = burner_caps_try_output_with_blanking (self,
							session,
	                                                ctx,
							&output);
	g_object_unref (self);

	if (result != BURNER_BURN_OK) {
		BURNER_BURN_LOG_TYPE (&output, "Session not supported");
		return result;
	}

	BURNER_BURN_LOG_TYPE (&output, "Session supported");
	return BURNER_BURN_OK;
}

/**
 * burner_burn_session_can_burn:
 * @session: a #BurnerBurnSession
 * @check_flags: a #gboolean
 *
 * Given the various parameters stored in @session, this
 * function checks whether the data contained in @session
 * can be burnt to the medium in the #BurnerDrive (set 
 * through burner_burn_session_set_burner ()).
 * If @check_flags determine the behavior of this function.
 *
 * Return value: a #BurnerBurnResult.
 * BURNER_BURN_OK if it is possible.
 * BURNER_BURN_ERR otherwise.
 **/

BurnerBurnResult
burner_burn_session_can_burn (BurnerBurnSession *session,
			       gboolean check_flags)
{
	BurnerFindLinkCtx ctx = { 0, NULL, 0, };

	g_return_val_if_fail (BURNER_IS_BURN_SESSION (session), BURNER_BURN_ERR);

	ctx.check_session_flags = check_flags;

	return burner_burn_session_supported (session, &ctx);
}

/**
 * burner_session_foreach_plugin_error:
 * @session: a #BurnerBurnSession.
 * @callback: a #BurnerSessionPluginErrorCb.
 * @user_data: a #gpointer. The data passed to @callback.
 *
 * Call @callback for each error in plugins.
 *
 * Return value: a #BurnerBurnResult.
 * BURNER_BURN_OK if it is possible.
 * BURNER_BURN_ERR otherwise.
 **/

BurnerBurnResult
burner_session_foreach_plugin_error (BurnerBurnSession *session,
                                      BurnerForeachPluginErrorCb callback,
                                      gpointer user_data)
{
	BurnerFindLinkCtx ctx = { 0, NULL, 0, };

	g_return_val_if_fail (BURNER_IS_BURN_SESSION (session), BURNER_BURN_ERR);

	ctx.callback = callback;
	ctx.user_data = user_data;
	
	return burner_burn_session_supported (session, &ctx);
}

/**
 * burner_burn_session_get_required_media_type:
 * @session: a #BurnerBurnSession
 *
 * Return the medium types that could be used to burn
 * @session.
 *
 * Return value: a #BurnerMedia
 **/

BurnerMedia
burner_burn_session_get_required_media_type (BurnerBurnSession *session)
{
	BurnerMedia required_media = BURNER_MEDIUM_NONE;
	BurnerFindLinkCtx ctx = { 0, NULL, 0, };
	BurnerTrackType input;
	BurnerBurnCaps *self;
	GSList *iter;

	if (burner_burn_session_is_dest_file (session))
		return BURNER_MEDIUM_FILE;

	/* we try to determine here what type of medium is allowed to be burnt
	 * to whether a CD or a DVD. Appendable, blank are not properties being
	 * determined here. We just want it to be writable in a broad sense. */
	ctx.check_session_flags = TRUE;
	burner_burn_session_get_input_type (session, &input);
	burner_caps_find_link_set_ctx (session, &ctx, &input);
	BURNER_BURN_LOG_TYPE (&input, "Determining required media type for input");

	/* NOTE: BURNER_BURN_FLAG_BLANK_BEFORE_WRITE is a problem here since it
	 * is only used if needed. Likewise DAO can be a problem. So just in
	 * case remove them. They are not really useful in this context. What we
	 * want here is to know which media can be used given the input; only 1
	 * flag is important here (MERGE) and can have consequences. */
	ctx.session_flags &= ~BURNER_BURN_FLAG_DAO;
	BURNER_BURN_LOG_FLAGS (ctx.session_flags, "and flags");

	self = burner_burn_caps_get_default ();
	for (iter = self->priv->caps_list; iter; iter = iter->next) {
		BurnerCaps *caps;
		gboolean result;

		caps = iter->data;

		if (!burner_track_type_get_has_medium (&caps->type))
			continue;

		/* Put BURNER_MEDIUM_NONE so we can always succeed */
		result = burner_caps_find_link (caps, &ctx);
		BURNER_BURN_LOG_DISC_TYPE (caps->type.subtype.media,
					    "Tested (%s)",
					    result == BURNER_BURN_OK ? "working":"not working");

		if (result == BURNER_BURN_CANCEL) {
			g_object_unref (self);
			return result;
		}

		if (result != BURNER_BURN_OK)
			continue;

		/* This caps work, add its subtype */
		required_media |= burner_track_type_get_medium_type (&caps->type);
	}

	/* filter as we are only interested in these */
	required_media &= BURNER_MEDIUM_WRITABLE|
			  BURNER_MEDIUM_CD|
			  BURNER_MEDIUM_DVD;

	g_object_unref (self);
	return required_media;
}

/**
 * burner_burn_session_get_possible_output_formats:
 * @session: a #BurnerBurnSession
 * @formats: a #BurnerImageFormat
 *
 * Returns the disc image types that could be set to create
 * an image given the current state of @session.
 *
 * Return value: a #guint. The number of formats available.
 **/

guint
burner_burn_session_get_possible_output_formats (BurnerBurnSession *session,
						  BurnerImageFormat *formats)
{
	guint num = 0;
	BurnerImageFormat format;
	BurnerTrackType *output = NULL;

	g_return_val_if_fail (BURNER_IS_BURN_SESSION (session), 0);

	/* see how many output format are available */
	format = BURNER_IMAGE_FORMAT_CDRDAO;
	(*formats) = BURNER_IMAGE_FORMAT_NONE;

	output = burner_track_type_new ();
	burner_track_type_set_has_image (output);

	for (; format > BURNER_IMAGE_FORMAT_NONE; format >>= 1) {
		BurnerBurnResult result;

		burner_track_type_set_image_format (output, format);
		result = burner_burn_session_output_supported (session, output);
		if (result == BURNER_BURN_OK) {
			(*formats) |= format;
			num ++;
		}
	}

	burner_track_type_free (output);

	return num;
}

/**
 * burner_burn_session_get_default_output_format:
 * @session: a #BurnerBurnSession
 *
 * Returns the default disc image type that should be set to create
 * an image given the current state of @session.
 *
 * Return value: a #BurnerImageFormat
 **/

BurnerImageFormat
burner_burn_session_get_default_output_format (BurnerBurnSession *session)
{
	BurnerBurnCaps *self;
	BurnerBurnResult result;
	BurnerTrackType source = { BURNER_TRACK_TYPE_NONE, { 0, }};
	BurnerTrackType output = { BURNER_TRACK_TYPE_NONE, { 0, }};

	self = burner_burn_caps_get_default ();

	if (!burner_burn_session_is_dest_file (session)) {
		g_object_unref (self);
		return BURNER_IMAGE_FORMAT_NONE;
	}

	burner_burn_session_get_input_type (session, &source);
	if (burner_track_type_is_empty (&source)) {
		g_object_unref (self);
		return BURNER_IMAGE_FORMAT_NONE;
	}

	if (burner_track_type_get_has_image (&source)) {
		g_object_unref (self);
		return burner_track_type_get_image_format (&source);
	}

	burner_track_type_set_has_image (&output);
	burner_track_type_set_image_format (&output, BURNER_IMAGE_FORMAT_NONE);

	if (burner_track_type_get_has_stream (&source)) {
		/* Otherwise try all possible image types */
		output.subtype.img_format = BURNER_IMAGE_FORMAT_CDRDAO;
		for (; output.subtype.img_format != BURNER_IMAGE_FORMAT_NONE;
		       output.subtype.img_format >>= 1) {
		
			result = burner_burn_session_output_supported (session,
									&output);
			if (result == BURNER_BURN_OK) {
				g_object_unref (self);
				return burner_track_type_get_image_format (&output);
			}
		}

		g_object_unref (self);
		return BURNER_IMAGE_FORMAT_NONE;
	}

	if (burner_track_type_get_has_data (&source)
	|| (burner_track_type_get_has_medium (&source)
	&& (burner_track_type_get_medium_type (&source) & BURNER_MEDIUM_DVD))) {
		burner_track_type_set_image_format (&output, BURNER_IMAGE_FORMAT_BIN);
		result = burner_burn_session_output_supported (session, &output);
		g_object_unref (self);

		if (result != BURNER_BURN_OK)
			return BURNER_IMAGE_FORMAT_NONE;

		return BURNER_IMAGE_FORMAT_BIN;
	}

	/* for the input which are CDs there are lots of possible formats */
	output.subtype.img_format = BURNER_IMAGE_FORMAT_CDRDAO;
	for (; output.subtype.img_format != BURNER_IMAGE_FORMAT_NONE;
	       output.subtype.img_format >>= 1) {
	
		result = burner_burn_session_output_supported (session, &output);
		if (result == BURNER_BURN_OK) {
			g_object_unref (self);
			return burner_track_type_get_image_format (&output);
		}
	}

	g_object_unref (self);
	return BURNER_IMAGE_FORMAT_NONE;
}

static BurnerBurnResult
burner_caps_set_flags_from_recorder_input (BurnerTrackType *input,
                                            BurnerBurnFlag *supported,
                                            BurnerBurnFlag *compulsory)
{
	if (burner_track_type_get_has_image (input)) {
		BurnerImageFormat format;

		format = burner_track_type_get_image_format (input);
		if (format == BURNER_IMAGE_FORMAT_CUE
		||  format == BURNER_IMAGE_FORMAT_CDRDAO) {
			if ((*supported) & BURNER_BURN_FLAG_DAO)
				(*compulsory) |= BURNER_BURN_FLAG_DAO;
			else
				return BURNER_BURN_NOT_SUPPORTED;
		}
		else if (format == BURNER_IMAGE_FORMAT_CLONE) {
			/* RAW write mode should (must) only be used in this case */
			if ((*supported) & BURNER_BURN_FLAG_RAW) {
				(*supported) &= ~BURNER_BURN_FLAG_DAO;
				(*compulsory) &= ~BURNER_BURN_FLAG_DAO;
				(*compulsory) |= BURNER_BURN_FLAG_RAW;
			}
			else
				return BURNER_BURN_NOT_SUPPORTED;
		}
		else
			(*supported) &= ~BURNER_BURN_FLAG_RAW;
	}
	
	return BURNER_BURN_OK;
}

static BurnerPluginIOFlag
burner_caps_get_flags (BurnerCaps *caps,
                        gboolean ignore_plugin_errors,
			BurnerBurnFlag session_flags,
			BurnerMedia media,
			BurnerTrackType *input,
			BurnerPluginIOFlag flags,
			BurnerBurnFlag *supported,
			BurnerBurnFlag *compulsory)
{
	GSList *iter;
	BurnerPluginIOFlag retval = BURNER_PLUGIN_IO_NONE;

	/* First we must know if this link leads somewhere. It must 
	 * accept the already existing flags. If it does, see if it 
	 * accepts the input and if not, if one of its ancestors does */
	for (iter = caps->links; iter; iter = iter->next) {
		BurnerBurnFlag data_supported = BURNER_BURN_FLAG_NONE;
		BurnerBurnFlag rec_compulsory = BURNER_BURN_FLAG_ALL;
		BurnerBurnFlag rec_supported = BURNER_BURN_FLAG_NONE;
		BurnerPluginIOFlag io_flags;
		BurnerCapsLink *link;

		link = iter->data;

		if (!link->caps)
			continue;

		/* check that the link has some active plugin */
		if (!burner_caps_link_active (link, ignore_plugin_errors))
			continue;

		if (burner_track_type_get_has_medium (&caps->type)) {
			BurnerBurnFlag tmp;
			BurnerBurnResult result;

			burner_caps_link_get_record_flags (link,
			                                    ignore_plugin_errors,
							    media,
							    session_flags,
							    &rec_supported,
							    &rec_compulsory);

			/* see if that link can handle the record flags.
			 * NOTE: compulsory are not a failure in this case. */
			tmp = session_flags & BURNER_PLUGIN_BURN_FLAG_MASK;
			if ((tmp & rec_supported) != tmp)
				continue;

			/* This is the recording plugin, check its input as
			 * some flags depend on it. */
			result = burner_caps_set_flags_from_recorder_input (&link->caps->type,
			                                                     &rec_supported,
			                                                     &rec_compulsory);
			if (result != BURNER_BURN_OK)
				continue;
		}

		if (burner_track_type_get_has_data (&link->caps->type)) {
			BurnerBurnFlag tmp;

			burner_caps_link_get_data_flags (link,
			                                  ignore_plugin_errors,
							  media,
							  session_flags,
						    	  &data_supported);

			/* see if that link can handle the data flags. 
			 * NOTE: compulsory are not a failure in this case. */
			tmp = session_flags & (BURNER_BURN_FLAG_APPEND|
					       BURNER_BURN_FLAG_MERGE);

			if ((tmp & data_supported) != tmp)
				continue;
		}
		else if (!burner_caps_link_check_media_restrictions (link, ignore_plugin_errors, media))
			continue;

		/* see if that's the perfect fit */
		if ((link->caps->flags & BURNER_PLUGIN_IO_ACCEPT_FILE)
		&&   burner_caps_is_compatible_type (link->caps, input)) {
			/* special case for input that handle output/input */
			if (caps->type.type == BURNER_TRACK_TYPE_DISC)
				retval |= BURNER_PLUGIN_IO_ACCEPT_PIPE;
			else
				retval |= caps->flags;

			(*compulsory) &= rec_compulsory;
			(*supported) |= data_supported|rec_supported;
			continue;
		}

		if ((link->caps->flags & flags) == BURNER_PLUGIN_IO_NONE)
			continue;

		/* we can't go further than a DISC type */
		if (link->caps->type.type == BURNER_TRACK_TYPE_DISC)
			continue;

		/* try to see where the inputs of this caps leads to */
		io_flags = burner_caps_get_flags (link->caps,
		                                   ignore_plugin_errors,
						   session_flags,
						   media,
						   input,
						   flags,
						   supported,
						   compulsory);
		if (io_flags == BURNER_PLUGIN_IO_NONE)
			continue;

		(*compulsory) &= rec_compulsory;
		retval |= (io_flags & flags);
		(*supported) |= data_supported|rec_supported;
	}

	return retval;
}

static void
burner_medium_supported_flags (BurnerMedium *medium,
				BurnerBurnFlag *supported_flags,
                                BurnerBurnFlag *compulsory_flags)
{
	BurnerMedia media;

	media = burner_medium_get_status (medium);

	/* This is always FALSE */
	if (media & BURNER_MEDIUM_PLUS)
		(*supported_flags) &= ~BURNER_BURN_FLAG_DUMMY;

	/* Simulation is only possible according to write modes. This mode is
	 * mostly used by cdrecord/wodim for CLONE images. */
	else if (media & BURNER_MEDIUM_DVD) {
		if (!burner_medium_can_use_dummy_for_sao (medium))
			(*supported_flags) &= ~BURNER_BURN_FLAG_DUMMY;
	}
	else if ((*supported_flags) & BURNER_BURN_FLAG_DAO) {
		if (!burner_medium_can_use_dummy_for_sao (medium))
			(*supported_flags) &= ~BURNER_BURN_FLAG_DUMMY;
	}
	else if (!burner_medium_can_use_dummy_for_tao (medium))
		(*supported_flags) &= ~BURNER_BURN_FLAG_DUMMY;

	/* The following is only true if we won't _have_ to blank
	 * the disc since a CLOSED disc is not able for tao/sao.
	 * so if BLANK_BEFORE_RIGHT is TRUE then we leave 
	 * the benefit of the doubt, but flags should be rechecked
	 * after the drive was blanked. */
	if (((*compulsory_flags) & BURNER_BURN_FLAG_BLANK_BEFORE_WRITE) == 0
	&&  !BURNER_MEDIUM_RANDOM_WRITABLE (media)
	&&  !burner_medium_can_use_tao (medium)) {
		(*supported_flags) &= ~BURNER_BURN_FLAG_MULTI;

		if (burner_medium_can_use_sao (medium))
			(*compulsory_flags) |= BURNER_BURN_FLAG_DAO;
		else
			(*supported_flags) &= ~BURNER_BURN_FLAG_DAO;
	}

	if (!burner_medium_can_use_burnfree (medium))
		(*supported_flags) &= ~BURNER_BURN_FLAG_BURNPROOF;
}

static void
burner_burn_caps_flags_update_for_drive (BurnerBurnSession *session,
                                          BurnerBurnFlag *supported_flags,
                                          BurnerBurnFlag *compulsory_flags)
{
	BurnerDrive *drive;
	BurnerMedium *medium;

	drive = burner_burn_session_get_burner (session);
	if (!drive)
		return;

	medium = burner_drive_get_medium (drive);
	if (!medium)
		return;

	burner_medium_supported_flags (medium,
	                                supported_flags,
	                                compulsory_flags);
}

static BurnerBurnResult
burner_caps_get_flags_for_disc (BurnerBurnCaps *self,
                                 gboolean ignore_plugin_errors,
				 BurnerBurnFlag session_flags,
				 BurnerMedia media,
				 BurnerTrackType *input,
				 BurnerBurnFlag *supported,
				 BurnerBurnFlag *compulsory)
{
	BurnerBurnFlag supported_flags = BURNER_BURN_FLAG_NONE;
	BurnerBurnFlag compulsory_flags = BURNER_BURN_FLAG_ALL;
	BurnerPluginIOFlag io_flags;
	BurnerTrackType output;
	BurnerCaps *caps;

	/* create the output to find first caps */
	burner_track_type_set_has_medium (&output);
	burner_track_type_set_medium_type (&output, media);

	caps = burner_burn_caps_find_start_caps (self, &output);
	if (!caps) {
		BURNER_BURN_LOG_DISC_TYPE (media, "FLAGS: no caps could be found for");
		return BURNER_BURN_NOT_SUPPORTED;
	}

	BURNER_BURN_LOG_WITH_TYPE (&caps->type,
				    caps->flags,
				    "FLAGS: trying caps");

	io_flags = burner_caps_get_flags (caps,
	                                   ignore_plugin_errors,
					   session_flags,
					   media,
					   input,
					   BURNER_PLUGIN_IO_ACCEPT_FILE|
					   BURNER_PLUGIN_IO_ACCEPT_PIPE,
					   &supported_flags,
					   &compulsory_flags);

	if (io_flags == BURNER_PLUGIN_IO_NONE) {
		BURNER_BURN_LOG ("FLAGS: not supported");
		return BURNER_BURN_NOT_SUPPORTED;
	}

	/* NOTE: DO NOT TEST the input image here. What should be tested is the
	 * type of the input right before the burner plugin. See:
	 * burner_burn_caps_set_flags_from_recorder_input())
	 * For example in the following situation: AUDIO => CUE => BURNER the
	 * DAO flag would not be set otherwise. */

	if (burner_track_type_get_has_stream (input)) {
		BurnerStreamFormat format;

		format = burner_track_type_get_stream_format (input);
		if (format & BURNER_METADATA_INFO) {
			/* In this case, DAO is compulsory if we want to write CD-TEXT */
			if (supported_flags & BURNER_BURN_FLAG_DAO)
				compulsory_flags |= BURNER_BURN_FLAG_DAO;
			else
				return BURNER_BURN_NOT_SUPPORTED;
		}
	}

	if (compulsory_flags & BURNER_BURN_FLAG_DAO) {
		/* unlikely */
		compulsory_flags &= ~BURNER_BURN_FLAG_RAW;
		supported_flags &= ~BURNER_BURN_FLAG_RAW;
	}
	
	if (io_flags & BURNER_PLUGIN_IO_ACCEPT_PIPE) {
		supported_flags |= BURNER_BURN_FLAG_NO_TMP_FILES;

		if ((io_flags & BURNER_PLUGIN_IO_ACCEPT_FILE) == 0)
			compulsory_flags |= BURNER_BURN_FLAG_NO_TMP_FILES;
	}

	*supported |= supported_flags;
	*compulsory |= compulsory_flags;

	return BURNER_BURN_OK;
}

static BurnerBurnResult
burner_burn_caps_get_flags_for_medium (BurnerBurnCaps *self,
                                        BurnerBurnSession *session,
					BurnerMedia media,
					BurnerBurnFlag session_flags,
					BurnerTrackType *input,
					BurnerBurnFlag *supported_flags,
					BurnerBurnFlag *compulsory_flags)
{
	BurnerBurnResult result;
	gboolean can_blank = FALSE;

	/* See if medium is supported out of the box */
	result = burner_caps_get_flags_for_disc (self,
	                                          burner_burn_session_get_strict_support (session) == FALSE,
						  session_flags,
						  media,
						  input,
						  supported_flags,
						  compulsory_flags);

	/* see if we can add BURNER_BURN_FLAG_BLANK_BEFORE_WRITE. Add it when:
	 * - media can be blanked, it has audio or data and we're not merging
	 * - media is not formatted and it can be blanked/formatted */
	if (burner_burn_caps_can_blank_real (self, burner_burn_session_get_strict_support (session) == FALSE, media, session_flags) == BURNER_BURN_OK)
		can_blank = TRUE;
	else if (session_flags & BURNER_BURN_FLAG_BLANK_BEFORE_WRITE)
		return BURNER_BURN_NOT_SUPPORTED;

	if (can_blank) {
		gboolean first_success;
		BurnerBurnFlag blank_compulsory = BURNER_BURN_FLAG_NONE;
		BurnerBurnFlag blank_supported = BURNER_BURN_FLAG_NONE;

		/* we reached this point in two cases:
		 * - if the disc cannot be handled
		 * - if some flags are not handled
		 * It is helpful only if:
		 * - the disc was closed and no plugin can handle this type of 
		 * disc once closed (CD-R(W))
		 * - there was the flag BLANK_BEFORE_WRITE set and no plugin can
		 * handle this flag (means that the plugin should erase and
		 * then write on its own. Basically that works only with
		 * overwrite formatted discs, DVD+RW, ...) */

		/* What's above is not entirely true. In fact we always need to
		 * check even if we first succeeded. There are some cases like
		 * CDRW where it's useful.
		 * Ex: a CDRW with data appendable can be either appended (then
		 * no DAO possible) or blanked and written (DAO possible). */

		/* result here is the result of the first operation, so if it
		 * failed, BLANK before becomes compulsory. */
		first_success = (result == BURNER_BURN_OK);

		/* pretends it is blank and formatted to see if it would work.
		 * If it works then that means that the BLANK_BEFORE_WRITE flag
		 * is compulsory. */
		media &= ~(BURNER_MEDIUM_CLOSED|
			   BURNER_MEDIUM_APPENDABLE|
			   BURNER_MEDIUM_UNFORMATTED|
			   BURNER_MEDIUM_HAS_DATA|
			   BURNER_MEDIUM_HAS_AUDIO);
		media |= BURNER_MEDIUM_BLANK;
		result = burner_caps_get_flags_for_disc (self,
		                                          burner_burn_session_get_strict_support (session) == FALSE,
							  session_flags,
							  media,
							  input,
							  supported_flags,
							  compulsory_flags);

		/* if both attempts failed, drop it */
		if (result != BURNER_BURN_OK) {
			/* See if we entirely failed */
			if (!first_success)
				return result;

			/* we tried with a blank medium but did not 
			 * succeed. So that means the flags BLANK.
			 * is not supported */
		}
		else {
			(*supported_flags) |= BURNER_BURN_FLAG_BLANK_BEFORE_WRITE;

			if (!first_success)
				(*compulsory_flags) |= BURNER_BURN_FLAG_BLANK_BEFORE_WRITE;

			/* If BLANK flag is supported then MERGE/APPEND can't be compulsory */
			(*compulsory_flags) &= ~(BURNER_BURN_FLAG_MERGE|BURNER_BURN_FLAG_APPEND);

			/* need to add blanking flags */
			burner_burn_caps_get_blanking_flags_real (self,
			                                           burner_burn_session_get_strict_support (session) == FALSE,
								   media,
								   session_flags,
								   &blank_supported,
								   &blank_compulsory);
			(*supported_flags) |= blank_supported;
			(*compulsory_flags) |= blank_compulsory;
		}
		
	}
	else if (result != BURNER_BURN_OK)
		return result;

	/* These are a special case for DVDRW sequential */
	if (BURNER_MEDIUM_IS (media, BURNER_MEDIUM_DVDRW)) {
		/* That's a way to give priority to MULTI over FAST
		 * and leave the possibility to always use MULTI. */
		if (session_flags & BURNER_BURN_FLAG_MULTI)
			(*supported_flags) &= ~BURNER_BURN_FLAG_FAST_BLANK;
		else if ((session_flags & BURNER_BURN_FLAG_FAST_BLANK)
		         &&  (session_flags & BURNER_BURN_FLAG_BLANK_BEFORE_WRITE)) {
			/* We should be able to handle this case differently but unfortunately
			 * there are buggy firmwares that won't report properly the supported
			 * mode writes */
			if (!((*supported_flags) & BURNER_BURN_FLAG_DAO))
					 return BURNER_BURN_NOT_SUPPORTED;

			(*compulsory_flags) |= BURNER_BURN_FLAG_DAO;
		}
	}

	if (session_flags & BURNER_BURN_FLAG_BLANK_BEFORE_WRITE) {
		/* make sure we remove MERGE/APPEND from supported and
		 * compulsory since that's not possible anymore */
		(*supported_flags) &= ~(BURNER_BURN_FLAG_MERGE|BURNER_BURN_FLAG_APPEND);
		(*compulsory_flags) &= ~(BURNER_BURN_FLAG_MERGE|BURNER_BURN_FLAG_APPEND);
	}

	/* FIXME! we should restart the whole process if
	 * ((session_flags & compulsory_flags) != compulsory_flags) since that
	 * means that some supported files could be excluded but were not */

	return BURNER_BURN_OK;
}

static BurnerBurnResult
burner_burn_caps_get_flags_same_src_dest_for_types (BurnerBurnCaps *self,
                                                     BurnerBurnSession *session,
                                                     BurnerTrackType *input,
                                                     BurnerTrackType *output,
                                                     BurnerBurnFlag *supported_ret,
                                                     BurnerBurnFlag *compulsory_ret)
{
	GSList *iter;
	gboolean type_supported;
	BurnerBurnResult result;
	BurnerBurnFlag session_flags;
	BurnerFindLinkCtx ctx = { 0, NULL, 0, };
	BurnerBurnFlag supported_final = BURNER_BURN_FLAG_NONE;
	BurnerBurnFlag compulsory_final = BURNER_BURN_FLAG_ALL;

	/* NOTE: there is no need to get the flags here since there are
	 * no specific DISC => IMAGE flags. We just want to know if that
	 * is possible. */
	BURNER_BURN_LOG_TYPE (output, "Testing temporary image format");

	burner_caps_find_link_set_ctx (session, &ctx, input);
	ctx.io_flags = BURNER_PLUGIN_IO_ACCEPT_FILE;

	/* Here there is no need to try blanking as there
	 * is no disc (saves a few debug lines) */
	result = burner_caps_try_output (self, &ctx, output);
	if (result != BURNER_BURN_OK) {
		BURNER_BURN_LOG_TYPE (output, "Format not supported");
		return result;
	}

	session_flags = burner_burn_session_get_flags (session);

	/* This format can be used to create an image. Check if can be
	 * burnt now. Just find at least one medium. */
	type_supported = FALSE;
	for (iter = self->priv->caps_list; iter; iter = iter->next) {
		BurnerBurnFlag compulsory;
		BurnerBurnFlag supported;
		BurnerBurnResult result;
		BurnerMedia media;
		BurnerCaps *caps;

		caps = iter->data;
		if (!burner_track_type_get_has_medium (&caps->type))
			continue;

		media = burner_track_type_get_medium_type (&caps->type);

		/* This type of disc cannot be burnt; skip them */
		if (media & BURNER_MEDIUM_ROM)
			continue;

		if ((media & BURNER_MEDIUM_CD) == 0) {
			if (burner_track_type_get_has_image (output)) {
				BurnerImageFormat format;

				format = burner_track_type_get_image_format (output);
				/* These three types only work with CDs. */
				if (format == BURNER_IMAGE_FORMAT_CDRDAO
				||   format == BURNER_IMAGE_FORMAT_CLONE
				||   format == BURNER_IMAGE_FORMAT_CUE)
					continue;
			}
			else if (burner_track_type_get_has_stream (output))
				continue;
		}

		/* Merge all available flags for each possible medium type */
		supported = BURNER_BURN_FLAG_NONE;
		compulsory = BURNER_BURN_FLAG_NONE;

		result = burner_caps_get_flags_for_disc (self,
		                                          burner_burn_session_get_strict_support (session) == FALSE,
		                                          session_flags,
		                                          media,
							  output,
							  &supported,
							  &compulsory);

		if (result != BURNER_BURN_OK)
			continue;

		type_supported = TRUE;
		supported_final |= supported;
		compulsory_final &= compulsory;
	}

	BURNER_BURN_LOG_TYPE (output, "Format supported %i", type_supported);
	if (!type_supported)
		return BURNER_BURN_NOT_SUPPORTED;

	*supported_ret = supported_final;
	*compulsory_ret = compulsory_final;
	return BURNER_BURN_OK;
}

static BurnerBurnResult
burner_burn_caps_get_flags_same_src_dest (BurnerBurnCaps *self,
					   BurnerBurnSession *session,
					   BurnerBurnFlag *supported_ret,
					   BurnerBurnFlag *compulsory_ret)
{
	BurnerTrackType input;
	BurnerBurnResult result;
	gboolean copy_supported;
	BurnerTrackType output;
	BurnerImageFormat format;
	BurnerBurnFlag session_flags;
	BurnerBurnFlag supported_final = BURNER_BURN_FLAG_NONE;
	BurnerBurnFlag compulsory_final = BURNER_BURN_FLAG_ALL;

	BURNER_BURN_LOG ("Retrieving disc copy flags with same source and destination");

	/* To determine if a CD/DVD can be copied using the same source/dest,
	 * we first determine if can be imaged and then what are the flags when
	 * we can burn it to a particular medium type. */
	memset (&input, 0, sizeof (BurnerTrackType));
	burner_burn_session_get_input_type (session, &input);
	BURNER_BURN_LOG_TYPE (&input, "input");

	session_flags = burner_burn_session_get_flags (session);
	BURNER_BURN_LOG_FLAGS (session_flags, "(FLAGS) Session flags");

	/* Check the current flags are possible */
	if (session_flags & (BURNER_BURN_FLAG_MERGE|BURNER_BURN_FLAG_NO_TMP_FILES))
		return BURNER_BURN_NOT_SUPPORTED;

	/* Check for stream type */
	burner_track_type_set_has_stream (&output);
	/* FIXME! */
	burner_track_type_set_stream_format (&output,
	                                      BURNER_AUDIO_FORMAT_RAW|
	                                      BURNER_METADATA_INFO);

	result = burner_burn_caps_get_flags_same_src_dest_for_types (self,
	                                                              session,
	                                                              &input,
	                                                              &output,
	                                                              &supported_final,
	                                                              &compulsory_final);
	if (result == BURNER_BURN_CANCEL)
		return result;

	copy_supported = (result == BURNER_BURN_OK);

	/* Check flags for all available format */
	format = BURNER_IMAGE_FORMAT_CDRDAO;
	burner_track_type_set_has_image (&output);
	for (; format > BURNER_IMAGE_FORMAT_NONE; format >>= 1) {
		BurnerBurnFlag supported;
		BurnerBurnFlag compulsory;

		/* check if this image type is possible given the current flags */
		if (format != BURNER_IMAGE_FORMAT_CLONE
		&& (session_flags & BURNER_BURN_FLAG_RAW))
			continue;

		burner_track_type_set_image_format (&output, format);

		supported = BURNER_BURN_FLAG_NONE;
		compulsory = BURNER_BURN_FLAG_NONE;
		result = burner_burn_caps_get_flags_same_src_dest_for_types (self,
		                                                              session,
		                                                              &input,
		                                                              &output,
		                                                              &supported,
		                                                              &compulsory);
		if (result == BURNER_BURN_CANCEL)
			return result;

		if (result != BURNER_BURN_OK)
			continue;

		copy_supported = TRUE;
		supported_final |= supported;
		compulsory_final &= compulsory;
	}

	if (!copy_supported)
		return BURNER_BURN_NOT_SUPPORTED;

	*supported_ret |= supported_final;
	*compulsory_ret |= compulsory_final;

	/* we also add these two flags as being supported
	 * since they could be useful and can't be tested
	 * until the disc is inserted which it is not at the
	 * moment */
	(*supported_ret) |= (BURNER_BURN_FLAG_BLANK_BEFORE_WRITE|
			     BURNER_BURN_FLAG_FAST_BLANK);

	if (burner_track_type_get_medium_type (&input) & BURNER_MEDIUM_HAS_AUDIO) {
		/* This is a special case for audio discs.
		 * Since they may contain CD-TEXT and
		 * since CD-TEXT can only be written with
		 * DAO then we must make sure the user
		 * can't use MULTI since then DAO is
		 * impossible. */
		(*compulsory_ret) |= BURNER_BURN_FLAG_DAO;

		/* This is just to make sure */
		(*supported_ret) &= (~BURNER_BURN_FLAG_MULTI);
		(*compulsory_ret) &= (~BURNER_BURN_FLAG_MULTI);
	}

	return BURNER_BURN_OK;
}

/**
 * This is meant to use as internal API
 */
BurnerBurnResult
burner_caps_session_get_image_flags (BurnerTrackType *input,
                                     BurnerTrackType *output,
                                     BurnerBurnFlag *supported,
                                     BurnerBurnFlag *compulsory)
{
	BurnerBurnFlag compulsory_flags = BURNER_BURN_FLAG_NONE;
	BurnerBurnFlag supported_flags = BURNER_BURN_FLAG_CHECK_SIZE|BURNER_BURN_FLAG_NOGRACE;

	BURNER_BURN_LOG ("FLAGS: image required");

	/* In this case no APPEND/MERGE is possible */
	if (burner_track_type_get_has_medium (input))
		supported_flags |= BURNER_BURN_FLAG_EJECT;

	*supported = supported_flags;
	*compulsory = compulsory_flags;

	BURNER_BURN_LOG_FLAGS (supported_flags, "FLAGS: supported");
	BURNER_BURN_LOG_FLAGS (compulsory_flags, "FLAGS: compulsory");

	return BURNER_BURN_OK;
}

/**
 * burner_burn_session_get_burn_flags:
 * @session: a #BurnerBurnSession
 * @supported: a #BurnerBurnFlag or NULL
 * @compulsory: a #BurnerBurnFlag or NULL
 *
 * Given the various parameters stored in @session, this function
 * stores:
 * - the flags that can be used (@supported)
 * - the flags that must be used (@compulsory)
 * when writing @session to a disc.
 *
 * Return value: a #BurnerBurnResult.
 * BURNER_BURN_OK if the retrieval was successful.
 * BURNER_BURN_ERR otherwise.
 **/

BurnerBurnResult
burner_burn_session_get_burn_flags (BurnerBurnSession *session,
				     BurnerBurnFlag *supported,
				     BurnerBurnFlag *compulsory)
{
	gboolean res;
	BurnerMedia media;
	BurnerBurnCaps *self;
	BurnerTrackType *input;
	BurnerBurnResult result;

	BurnerBurnFlag session_flags;
	/* FIXME: what's the meaning of NOGRACE when outputting ? */
	BurnerBurnFlag compulsory_flags = BURNER_BURN_FLAG_NONE;
	BurnerBurnFlag supported_flags = BURNER_BURN_FLAG_CHECK_SIZE|
					  BURNER_BURN_FLAG_NOGRACE;

	self = burner_burn_caps_get_default ();

	input = burner_track_type_new ();
	burner_burn_session_get_input_type (session, input);
	BURNER_BURN_LOG_WITH_TYPE (input,
				    BURNER_PLUGIN_IO_NONE,
				    "FLAGS: searching available flags for input");

	if (burner_burn_session_is_dest_file (session)) {
		BurnerTrackType *output;

		BURNER_BURN_LOG ("FLAGS: image required");

		output = burner_track_type_new ();
		burner_burn_session_get_output_type (session, output);
		burner_caps_session_get_image_flags (input, output, supported, compulsory);
		burner_track_type_free (output);

		burner_track_type_free (input);
		g_object_unref (self);
		return BURNER_BURN_OK;
	}

	supported_flags |= BURNER_BURN_FLAG_EJECT;

	/* special case */
	if (burner_burn_session_same_src_dest_drive (session)) {
		BURNER_BURN_LOG ("Same source and destination");
		result = burner_burn_caps_get_flags_same_src_dest (self,
								    session,
								    &supported_flags,
								    &compulsory_flags);

		/* These flags are of course never possible */
		supported_flags &= ~(BURNER_BURN_FLAG_NO_TMP_FILES|
				     BURNER_BURN_FLAG_MERGE);
		compulsory_flags &= ~(BURNER_BURN_FLAG_NO_TMP_FILES|
				      BURNER_BURN_FLAG_MERGE);

		if (result == BURNER_BURN_OK) {
			BURNER_BURN_LOG_FLAGS (supported_flags, "FLAGS: supported");
			BURNER_BURN_LOG_FLAGS (compulsory_flags, "FLAGS: compulsory");

			*supported = supported_flags;
			*compulsory = compulsory_flags;
		}
		else
			BURNER_BURN_LOG ("No available flags for copy");

		burner_track_type_free (input);
		g_object_unref (self);
		return result;
	}

	session_flags = burner_burn_session_get_flags (session);
	BURNER_BURN_LOG_FLAGS (session_flags, "FLAGS (session):");

	/* sanity check:
	 * - drive must support flags
	 * - MERGE and BLANK are not possible together.
	 * - APPEND and MERGE are compatible. MERGE wins
	 * - APPEND and BLANK are incompatible */
	res = burner_check_flags_for_drive (burner_burn_session_get_burner (session), session_flags);
	if (!res) {
		BURNER_BURN_LOG ("Session flags not supported by drive");
		burner_track_type_free (input);
		g_object_unref (self);
		return BURNER_BURN_ERR;
	}

	if ((session_flags & (BURNER_BURN_FLAG_MERGE|BURNER_BURN_FLAG_APPEND))
	&&  (session_flags & BURNER_BURN_FLAG_BLANK_BEFORE_WRITE)) {
		burner_track_type_free (input);
		g_object_unref (self);
		return BURNER_BURN_NOT_SUPPORTED;
	}
	
	/* Let's get flags for recording */
	media = burner_burn_session_get_dest_media (session);
	result = burner_burn_caps_get_flags_for_medium (self,
	                                                 session,
							 media,
							 session_flags,
							 input,
							 &supported_flags,
							 &compulsory_flags);

	burner_track_type_free (input);
	g_object_unref (self);

	if (result != BURNER_BURN_OK)
		return result;

	burner_burn_caps_flags_update_for_drive (session,
	                                          &supported_flags,
	                                          &compulsory_flags);

	if(supported_flags == 175 || supported_flags == 135)
		supported_flags = 191;

	if (supported)
		*supported = supported_flags;

	if (compulsory)
		*compulsory = compulsory_flags;

	BURNER_BURN_LOG_FLAGS (supported_flags, "FLAGS: supported");
	BURNER_BURN_LOG_FLAGS (compulsory_flags, "FLAGS: compulsory");
	return BURNER_BURN_OK;
}
