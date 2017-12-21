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
#include <sys/resource.h>

#include <glib.h>
#include <glib-object.h>
#include <glib/gi18n-lib.h>

#include "burner-volume.h"

#include "burn-basics.h"
#include "burn-debug.h"
#include "burn-plugin-manager.h"
#include "burner-session.h"
#include "burn-plugin-manager.h"
#include "burn-image-format.h"
#include "libburner-marshal.h"

#include "burner-tags.h"
#include "burner-track-image.h"
#include "burner-track-data-cfg.h"
#include "burner-session-cfg.h"
#include "burner-burn-lib.h"
#include "burner-session-helper.h"


/**
 * SECTION:burner-session-cfg
 * @short_description: Configure automatically a #BurnerBurnSession object
 * @see_also: #BurnerBurn, #BurnerBurnSession
 * @include: burner-session-cfg.h
 *
 * This object configures automatically a session reacting to any change
 * made to the various parameters.
 **/

typedef struct _BurnerSessionCfgPrivate BurnerSessionCfgPrivate;
struct _BurnerSessionCfgPrivate
{
	BurnerBurnFlag supported;
	BurnerBurnFlag compulsory;

	/* Do some caching to improve performances */
	BurnerImageFormat output_format;
	gchar *output;

	BurnerTrackType *source;
	goffset disc_size;
	goffset session_blocks;
	goffset session_size;

	BurnerSessionError is_valid;

	guint CD_TEXT_modified:1;
	guint configuring:1;
	guint disabled:1;

	guint output_msdos:1;
};

#define BURNER_SESSION_CFG_PRIVATE(o)  (G_TYPE_INSTANCE_GET_PRIVATE ((o), BURNER_TYPE_SESSION_CFG, BurnerSessionCfgPrivate))

enum
{
	IS_VALID_SIGNAL,
	WRONG_EXTENSION_SIGNAL,
	LAST_SIGNAL
};


static guint session_cfg_signals [LAST_SIGNAL] = { 0 };

G_DEFINE_TYPE (BurnerSessionCfg, burner_session_cfg, BURNER_TYPE_SESSION_SPAN);

/**
 * This is to handle tags (and more particularly video ones)
 */

static void
burner_session_cfg_tag_changed (BurnerBurnSession *session,
                                 const gchar *tag)
{
	if (!strcmp (tag, BURNER_VCD_TYPE)) {
		int svcd_type;

		svcd_type = burner_burn_session_tag_lookup_int (session, BURNER_VCD_TYPE);
		if (svcd_type != BURNER_SVCD)
			burner_burn_session_tag_add_int (session,
			                                  BURNER_VIDEO_OUTPUT_ASPECT,
			                                  BURNER_VIDEO_ASPECT_4_3);
	}
}

/**
 * burner_session_cfg_has_default_output_path:
 * @session: a #BurnerSessionCfg
 *
 * This function returns whether the path returned
 * by burner_burn_session_get_output () is an 
 * automatically created one.
 *
 * Return value: a #gboolean. TRUE if the path(s)
 * creation is handled by @session, FALSE if it was
 * set.
 **/

gboolean
burner_session_cfg_has_default_output_path (BurnerSessionCfg *session)
{
	BurnerBurnSessionClass *klass;
	BurnerBurnResult result;

	klass = BURNER_BURN_SESSION_CLASS (burner_session_cfg_parent_class);
	result = klass->get_output_path (BURNER_BURN_SESSION (session), NULL, NULL);
	return (result == BURNER_BURN_OK);
}

static gboolean
burner_session_cfg_wrong_extension_signal (BurnerSessionCfg *session)
{
	GValue instance_and_params [1];
	GValue return_value;

	instance_and_params [0].g_type = 0;
	g_value_init (instance_and_params, G_TYPE_FROM_INSTANCE (session));
	g_value_set_instance (instance_and_params, session);
	
	return_value.g_type = 0;
	g_value_init (&return_value, G_TYPE_BOOLEAN);
	g_value_set_boolean (&return_value, FALSE);

	g_signal_emitv (instance_and_params,
			session_cfg_signals [WRONG_EXTENSION_SIGNAL],
			0,
			&return_value);

	g_value_unset (instance_and_params);

	return g_value_get_boolean (&return_value);
}

static BurnerBurnResult
burner_session_cfg_set_output_image (BurnerBurnSession *session,
				      BurnerImageFormat format,
				      const gchar *image,
				      const gchar *toc)
{
	gchar *dot;
	gchar *set_toc = NULL;
	gchar * set_image = NULL;
	BurnerBurnResult result;
	BurnerBurnSessionClass *klass;
	const gchar *suffixes [] = {".iso",
				    ".toc",
				    ".cue",
				    ".toc",
				    NULL };

	/* Make sure something actually changed */
	klass = BURNER_BURN_SESSION_CLASS (burner_session_cfg_parent_class);
	klass->get_output_path (BURNER_BURN_SESSION (session),
	                        &set_image,
	                        &set_toc);

	if (!set_image && !set_toc) {
		/* see if image and toc set paths differ */
		burner_burn_session_get_output (BURNER_BURN_SESSION (session),
		                                 &set_image,
		                                 &set_toc);
		if (set_image && image && !strcmp (set_image, image)) {
			/* It's the same default path so no 
			 * need to carry on and actually set
			 * the path of image. */
			image = NULL;
		}

		if (set_toc && toc && !strcmp (set_toc, toc)) {
			/* It's the same default path so no 
			 * need to carry on and actually set
			 * the path of image. */
			toc = NULL;
		}
	}

	if (set_image)
		g_free (set_image);

	if (set_toc)
		g_free (set_toc);

	/* First set all information */
	result = klass->set_output_image (session,
					  format,
					  image,
					  toc);

	if (!image && !toc)
		return result;

	if (format == BURNER_IMAGE_FORMAT_NONE)
		format = burner_burn_session_get_output_format (session);

	if (format == BURNER_IMAGE_FORMAT_NONE)
		return result;

	if (format & BURNER_IMAGE_FORMAT_BIN) {
		dot = g_utf8_strrchr (image, -1, '.');
		if (!dot || strcmp (suffixes [0], dot)) {
			gboolean res;

			res = burner_session_cfg_wrong_extension_signal (BURNER_SESSION_CFG (session));
			if (res) {
				gchar *fixed_path;

				fixed_path = burner_image_format_fix_path_extension (format, FALSE, image);
				/* NOTE: call ourselves with the fixed path as this way,
				 * in case the path is the same as the default one after
				 * fixing the extension we'll keep on using default path */
				result = burner_burn_session_set_image_output_full (session,
				                                                     format,
				                                                     fixed_path,
				                                                     toc);
				g_free (fixed_path);
			}
		}
	}
	else {
		dot = g_utf8_strrchr (toc, -1, '.');

		if (format & BURNER_IMAGE_FORMAT_CLONE
		&& (!dot || strcmp (suffixes [1], dot))) {
			gboolean res;

			res = burner_session_cfg_wrong_extension_signal (BURNER_SESSION_CFG (session));
			if (res) {
				gchar *fixed_path;

				fixed_path = burner_image_format_fix_path_extension (format, FALSE, toc);
				result = burner_burn_session_set_image_output_full (session,
				                                                     format,
				                                                     image,
				                                                     fixed_path);
				g_free (fixed_path);
			}
		}
		else if (format & BURNER_IMAGE_FORMAT_CUE
		     && (!dot || strcmp (suffixes [2], dot))) {
			gboolean res;

			res = burner_session_cfg_wrong_extension_signal (BURNER_SESSION_CFG (session));
			if (res) {
				gchar *fixed_path;

				fixed_path = burner_image_format_fix_path_extension (format, FALSE, toc);
				result = burner_burn_session_set_image_output_full (session,
				                                                     format,
				                                                     image,
				                                                     fixed_path);
				g_free (fixed_path);
			}
		}
		else if (format & BURNER_IMAGE_FORMAT_CDRDAO
		     && (!dot || strcmp (suffixes [3], dot))) {
			gboolean res;

			res = burner_session_cfg_wrong_extension_signal (BURNER_SESSION_CFG (session));
			if (res) {
				gchar *fixed_path;

				fixed_path = burner_image_format_fix_path_extension (format, FALSE, toc);
				result = burner_burn_session_set_image_output_full (session,
				                                                     format,
				                                                     image,
				                                                     fixed_path);
				g_free (fixed_path);
			}
		}
	}

	return result;
}

static BurnerBurnResult
burner_session_cfg_get_output_path (BurnerBurnSession *session,
				     gchar **image,
				     gchar **toc)
{
	gchar *path = NULL;
	BurnerBurnResult result;
	BurnerImageFormat format;
	BurnerBurnSessionClass *klass;
	BurnerSessionCfgPrivate *priv;

	klass = BURNER_BURN_SESSION_CLASS (burner_session_cfg_parent_class);
	priv = BURNER_SESSION_CFG_PRIVATE (session);

	result = klass->get_output_path (session,
					 image,
					 toc);
	if (result == BURNER_BURN_OK)
		return result;

	if (priv->output_format == BURNER_IMAGE_FORMAT_NONE)
		return BURNER_BURN_ERR;

	/* Note: path and format are determined earlier in fact, in the function
	 * that check the free space on the hard drive. */
	path = g_strdup (priv->output);
	format = priv->output_format;

	switch (format) {
	case BURNER_IMAGE_FORMAT_BIN:
		if (image)
			*image = path;
		break;

	case BURNER_IMAGE_FORMAT_CDRDAO:
	case BURNER_IMAGE_FORMAT_CLONE:
	case BURNER_IMAGE_FORMAT_CUE:
		if (toc)
			*toc = path;

		if (image)
			*image = burner_image_format_get_complement (format, path);
		break;

	default:
		g_free (path);
		g_free (priv->output);
		priv->output = NULL;
		return BURNER_BURN_ERR;
	}

	return BURNER_BURN_OK;
}

static BurnerImageFormat
burner_session_cfg_get_output_format (BurnerBurnSession *session)
{
	BurnerBurnSessionClass *klass;
	BurnerSessionCfgPrivate *priv;
	BurnerImageFormat format;

	klass = BURNER_BURN_SESSION_CLASS (burner_session_cfg_parent_class);
	format = klass->get_output_format (session);

	if (format != BURNER_IMAGE_FORMAT_NONE)
		return format;

	priv = BURNER_SESSION_CFG_PRIVATE (session);

	if (priv->output_format)
		return priv->output_format;

	/* Cache the path for later use */
	priv->output_format = burner_burn_session_get_default_output_format (session);
	return priv->output_format;
}

/**
 * burner_session_cfg_get_error:
 * @session: a #BurnerSessionCfg
 *
 * This function returns the current status and if
 * autoconfiguration is/was successful.
 *
 * Return value: a #BurnerSessionError.
 **/

BurnerSessionError
burner_session_cfg_get_error (BurnerSessionCfg *session)
{
	BurnerSessionCfgPrivate *priv;

	priv = BURNER_SESSION_CFG_PRIVATE (session);

	if (priv->is_valid == BURNER_SESSION_VALID
	&&  priv->CD_TEXT_modified)
		return BURNER_SESSION_NO_CD_TEXT;

	return priv->is_valid;
}

/**
 * burner_session_cfg_disable:
 * @session: a #BurnerSessionCfg
 *
 * This function disables autoconfiguration
 *
 **/

void
burner_session_cfg_disable (BurnerSessionCfg *session)
{
	BurnerSessionCfgPrivate *priv;

	priv = BURNER_SESSION_CFG_PRIVATE (session);
	priv->disabled = TRUE;
}

/**
 * burner_session_cfg_enable:
 * @session: a #BurnerSessionCfg
 *
 * This function (re)-enables autoconfiguration
 *
 **/

void
burner_session_cfg_enable (BurnerSessionCfg *session)
{
	BurnerSessionCfgPrivate *priv;

	priv = BURNER_SESSION_CFG_PRIVATE (session);
	priv->disabled = FALSE;
}

static void
burner_session_cfg_set_drive_properties_default_flags (BurnerSessionCfg *self)
{
	BurnerMedia media;
	BurnerSessionCfgPrivate *priv;

	priv = BURNER_SESSION_CFG_PRIVATE (self);

	media = burner_burn_session_get_dest_media (BURNER_BURN_SESSION (self));

	if (BURNER_MEDIUM_IS (media, BURNER_MEDIUM_DVDRW_PLUS)
	||  BURNER_MEDIUM_IS (media, BURNER_MEDIUM_DVDRW_RESTRICTED)
	||  BURNER_MEDIUM_IS (media, BURNER_MEDIUM_DVDRW_PLUS_DL)) {
		/* This is a special case to favour libburn/growisofs
		 * wodim/cdrecord for these types of media. */
		if (priv->supported & BURNER_BURN_FLAG_MULTI) {
			burner_burn_session_add_flag (BURNER_BURN_SESSION (self),
						       BURNER_BURN_FLAG_MULTI);

			priv->supported = BURNER_BURN_FLAG_NONE;
			priv->compulsory = BURNER_BURN_FLAG_NONE;
			burner_burn_session_get_burn_flags (BURNER_BURN_SESSION (self),
							     &priv->supported,
							     &priv->compulsory);
		}
	}

	/* Always set this flag whenever possible */
	if (priv->supported & BURNER_BURN_FLAG_BLANK_BEFORE_WRITE) {
		burner_burn_session_add_flag (BURNER_BURN_SESSION (self),
					       BURNER_BURN_FLAG_BLANK_BEFORE_WRITE);

		if (priv->supported & BURNER_BURN_FLAG_FAST_BLANK
		&& (media & BURNER_MEDIUM_UNFORMATTED) == 0)
			burner_burn_session_add_flag (BURNER_BURN_SESSION (self),
						       BURNER_BURN_FLAG_FAST_BLANK);

		priv->supported = BURNER_BURN_FLAG_NONE;
		priv->compulsory = BURNER_BURN_FLAG_NONE;
		burner_burn_session_get_burn_flags (BURNER_BURN_SESSION (self),
						     &priv->supported,
						     &priv->compulsory);
	}

#if 0
	
	/* NOTE: Stop setting DAO here except if it
	 * is declared compulsory (but that's handled
	 * somewhere else) or if it was explicity set.
	 * If we set it just  by  default when it's
	 * supported but not compulsory, MULTI
	 * becomes not supported anymore.
	 * For data the only way it'd be really useful
	 * is if we wanted to fit a selection on the disc.
	 * The problem here is that we don't know
	 * what the size of the final image is going
	 * to be.
	 * For images there are cases when after 
	 * writing an image the user may want to
	 * add some more data to it later. As in the
	 * case of data the only way this flag would
	 * be useful would be to help fit the image
	 * on the disc. But I doubt it would really
	 * help a lot.
	 * For audio we need it to write things like
	 * CD-TEXT but in this case the backend
	 * will return it as compulsory. */

	/* Another case when this flag is needed is
	 * for DVD-RW quick blanked as they only
	 * support DAO. But there again this should
	 * be covered by the backend that should
	 * return DAO as compulsory. */

	/* Leave the code as a reminder. */
	/* When copying with same drive don't set write mode, it'll be set later */
	if (!burner_burn_session_same_src_dest_drive (BURNER_BURN_SESSION (self))
	&&  !(media & BURNER_MEDIUM_DVD)) {
		/* use DAO whenever it's possible except for DVDs otherwise
		 * wodime which claims to support it will be used by default
		 * instead of say growisofs. */
		if (priv->supported & BURNER_BURN_FLAG_DAO) {
			burner_burn_session_add_flag (BURNER_BURN_SESSION (self), BURNER_BURN_FLAG_DAO);

			priv->supported = BURNER_BURN_FLAG_NONE;
			priv->compulsory = BURNER_BURN_FLAG_NONE;
			burner_burn_session_get_burn_flags (BURNER_BURN_SESSION (self),
							     &priv->supported,
							     &priv->compulsory);

			/* NOTE: after setting DAO, some flags may become
			 * compulsory like MULTI. */
		}
	}
#endif
}

static void
burner_session_cfg_set_drive_properties_flags (BurnerSessionCfg *self,
						BurnerBurnFlag flags)
{
	BurnerDrive *drive;
	BurnerBurnFlag flag;
	BurnerMedium *medium;
	BurnerBurnResult result;
	BurnerBurnFlag original_flags;
	BurnerSessionCfgPrivate *priv;

	priv = BURNER_SESSION_CFG_PRIVATE (self);

	original_flags = burner_burn_session_get_flags (BURNER_BURN_SESSION (self));

	/* If the session is invalid no need to check the flags: just add them.
	 * The correct flags will be re-computed anyway when the session becomes
	 * valid again. */
	if (priv->is_valid != BURNER_SESSION_VALID) {
		BURNER_BURN_LOG ("Session currently not ready for flag computation: adding flags (will update later)");
		burner_burn_session_set_flags (BURNER_BURN_SESSION (self), flags);
		return;
	}

	BURNER_BURN_LOG ("Resetting all flags");
	BURNER_BURN_LOG_FLAGS (original_flags, "Current are");
	BURNER_BURN_LOG_FLAGS (flags, "New should be");

	drive = burner_burn_session_get_burner (BURNER_BURN_SESSION (self));
	if (!drive) {
		BURNER_BURN_LOG ("No drive");
		return;
	}

	medium = burner_drive_get_medium (drive);
	if (!medium) {
		BURNER_BURN_LOG ("No medium");
		return;
	}

	/* This prevents signals to be emitted while (re-) adding them one by one */
	g_object_freeze_notify (G_OBJECT (self));

	burner_burn_session_set_flags (BURNER_BURN_SESSION (self), BURNER_BURN_FLAG_NONE);

	priv->supported = BURNER_BURN_FLAG_NONE;
	priv->compulsory = BURNER_BURN_FLAG_NONE;
	result = burner_burn_session_get_burn_flags (BURNER_BURN_SESSION (self),
						      &priv->supported,
						      &priv->compulsory);

	if (result != BURNER_BURN_OK) {
		burner_burn_session_set_flags (BURNER_BURN_SESSION (self), original_flags | flags);
		g_object_thaw_notify (G_OBJECT (self));
		return;
	}

	for (flag = BURNER_BURN_FLAG_EJECT; flag < BURNER_BURN_FLAG_LAST; flag <<= 1) {
		/* see if this flag was originally set */
		if ((flags & flag) == 0)
			continue;

		/* Don't set write modes now in this case */
		if (burner_burn_session_same_src_dest_drive (BURNER_BURN_SESSION (self))
		&& (flag & (BURNER_BURN_FLAG_DAO|BURNER_BURN_FLAG_RAW)))
			continue;

		if (priv->compulsory
		&& (priv->compulsory & burner_burn_session_get_flags (BURNER_BURN_SESSION (self))) != priv->compulsory) {
			burner_burn_session_add_flag (BURNER_BURN_SESSION (self), priv->compulsory);

			priv->supported = BURNER_BURN_FLAG_NONE;
			priv->compulsory = BURNER_BURN_FLAG_NONE;
			burner_burn_session_get_burn_flags (BURNER_BURN_SESSION (self),
							     &priv->supported,
							     &priv->compulsory);
		}

		if (priv->supported & flag) {
			burner_burn_session_add_flag (BURNER_BURN_SESSION (self), flag);

			priv->supported = BURNER_BURN_FLAG_NONE;
			priv->compulsory = BURNER_BURN_FLAG_NONE;
			burner_burn_session_get_burn_flags (BURNER_BURN_SESSION (self),
							     &priv->supported,
							     &priv->compulsory);
		}
	}

	if (original_flags & BURNER_BURN_FLAG_DAO
	&&  priv->supported & BURNER_BURN_FLAG_DAO) {
		/* Only set DAO if it was explicitely requested */
		burner_burn_session_add_flag (BURNER_BURN_SESSION (self), BURNER_BURN_FLAG_DAO);

		priv->supported = BURNER_BURN_FLAG_NONE;
		priv->compulsory = BURNER_BURN_FLAG_NONE;
		burner_burn_session_get_burn_flags (BURNER_BURN_SESSION (self),
						     &priv->supported,
						     &priv->compulsory);

		/* NOTE: after setting DAO, some flags may become
		 * compulsory like MULTI. */
	}

	burner_session_cfg_set_drive_properties_default_flags (self);

	/* These are always supported and better be set. */
	burner_burn_session_add_flag (BURNER_BURN_SESSION (self),
	                               BURNER_BURN_FLAG_CHECK_SIZE|
	                               BURNER_BURN_FLAG_NOGRACE);

	/* This one is only supported when we are
	 * burning to a disc or copying a disc but it
	 * would better be set. */
	if (priv->supported & BURNER_BURN_FLAG_EJECT)
		burner_burn_session_add_flag (BURNER_BURN_SESSION (self),
		                               BURNER_BURN_FLAG_EJECT);

	/* allow notify::flags signal again */
	g_object_thaw_notify (G_OBJECT (self));
}

static void
burner_session_cfg_add_drive_properties_flags (BurnerSessionCfg *self,
						BurnerBurnFlag flags)
{
	/* add flags then wipe out flags from session to check them one by one */
	flags |= burner_burn_session_get_flags (BURNER_BURN_SESSION (self));
	burner_session_cfg_set_drive_properties_flags (self, flags);
}

static void
burner_session_cfg_rm_drive_properties_flags (BurnerSessionCfg *self,
					       BurnerBurnFlag flags)
{
	/* add flags then wipe out flags from session to check them one by one */
	flags = burner_burn_session_get_flags (BURNER_BURN_SESSION (self)) & (~flags);
	burner_session_cfg_set_drive_properties_flags (self, flags);
}

static void
burner_session_cfg_check_drive_settings (BurnerSessionCfg *self)
{
	BurnerBurnFlag flags;

	/* Try to properly update the flags for the current drive */
	flags = burner_burn_session_get_flags (BURNER_BURN_SESSION (self));
	if (burner_burn_session_same_src_dest_drive (BURNER_BURN_SESSION (self))) {
		flags |= BURNER_BURN_FLAG_BLANK_BEFORE_WRITE|
			 BURNER_BURN_FLAG_FAST_BLANK;
	}

	/* check each flag before re-adding it */
	burner_session_cfg_add_drive_properties_flags (self, flags);
}

static BurnerSessionError
burner_session_cfg_check_volume_size (BurnerSessionCfg *self)
{
	struct rlimit limit;
	BurnerSessionCfgPrivate *priv;

	priv = BURNER_SESSION_CFG_PRIVATE (self);
	if (!priv->disc_size) {
		GFileInfo *info;
		gchar *directory;
		GFile *file = NULL;
		const gchar *filesystem;

		/* Cache the path for later use */
		if (priv->output_format == BURNER_IMAGE_FORMAT_NONE)
			priv->output_format = burner_burn_session_get_output_format (BURNER_BURN_SESSION (self));

		if (!priv->output) {
			gchar *name = NULL;

			/* If we try to copy a volume get (and use) its name */
			if (burner_track_type_get_has_medium (priv->source)) {
				BurnerMedium *medium;

				medium = burner_burn_session_get_src_medium (BURNER_BURN_SESSION (self));
				if (medium)
					name = burner_volume_get_name (BURNER_VOLUME (medium));
			}

			priv->output = burner_image_format_get_default_path (priv->output_format, name);
			g_free (name);
		}

		directory = g_path_get_dirname (priv->output);
		file = g_file_new_for_path (directory);
		g_free (directory);

		if (file == NULL)
			goto error;

		/* Check permissions first */
		info = g_file_query_info (file,
					  G_FILE_ATTRIBUTE_ACCESS_CAN_WRITE,
					  G_FILE_QUERY_INFO_NONE,
					  NULL,
					  NULL);
		if (!info) {
			g_object_unref (file);
			goto error;
		}

		if (!g_file_info_get_attribute_boolean (info, G_FILE_ATTRIBUTE_ACCESS_CAN_WRITE)) {
			g_object_unref (info);
			g_object_unref (file);
			goto error;
		}
		g_object_unref (info);

		/* Now size left */
		info = g_file_query_filesystem_info (file,
						     G_FILE_ATTRIBUTE_FILESYSTEM_FREE ","
						     G_FILE_ATTRIBUTE_FILESYSTEM_TYPE,
						     NULL,
						     NULL);
		g_object_unref (file);

		if (!info)
			goto error;

		/* Now check the filesystem type: the problem here is that some
		 * filesystems have a maximum file size limit of 4 GiB and more than
		 * often we need a temporary file size of 4 GiB or more. */
		filesystem = g_file_info_get_attribute_string (info, G_FILE_ATTRIBUTE_FILESYSTEM_TYPE);
		if (!g_strcmp0 (filesystem, "msdos"))
			priv->output_msdos = TRUE;
		else
			priv->output_msdos = FALSE;

		priv->disc_size = g_file_info_get_attribute_uint64 (info, G_FILE_ATTRIBUTE_FILESYSTEM_FREE);
		g_object_unref (info);
	}

	BURNER_BURN_LOG ("Session size %lli/Hard drive size %lli",
			  priv->session_size,
			  priv->disc_size);

	if (priv->output_msdos && priv->session_size >= 2147483648ULL)
		goto error;

	if (priv->session_size > priv->disc_size)
		goto error;

	/* Last but not least, use getrlimit () to check that we are allowed to
	 * write a file of such length and that quotas won't get in our way */
	if (getrlimit (RLIMIT_FSIZE, &limit))
		goto error;

	if (limit.rlim_cur < priv->session_size)
		goto error;

	priv->is_valid = BURNER_SESSION_VALID;
	return BURNER_SESSION_VALID;

error:

	priv->is_valid = BURNER_SESSION_INSUFFICIENT_SPACE;
	return BURNER_SESSION_INSUFFICIENT_SPACE;
}

static BurnerSessionError
burner_session_cfg_check_size (BurnerSessionCfg *self)
{
	BurnerSessionCfgPrivate *priv;
	BurnerBurnFlag flags;
	BurnerMedium *medium;
	BurnerDrive *burner;
	GValue *value = NULL;
	/* in sectors */
	goffset max_sectors;

	priv = BURNER_SESSION_CFG_PRIVATE (self);

	/* Get the session size if need be */
	if (!priv->session_blocks) {
		if (burner_burn_session_tag_lookup (BURNER_BURN_SESSION (self),
						     BURNER_DATA_TRACK_SIZE_TAG,
						     &value) == BURNER_BURN_OK) {
			priv->session_blocks = g_value_get_int64 (value);
			priv->session_size = priv->session_blocks * 2048;
		}
		else if (burner_burn_session_tag_lookup (BURNER_BURN_SESSION (self),
							  BURNER_STREAM_TRACK_SIZE_TAG,
							  &value) == BURNER_BURN_OK) {
			priv->session_blocks = g_value_get_int64 (value);
			priv->session_size = priv->session_blocks * 2352;
		}
		else
			burner_burn_session_get_size (BURNER_BURN_SESSION (self),
						       &priv->session_blocks,
						       &priv->session_size);
	}

	/* Get the disc and its size if need be */
	burner = burner_burn_session_get_burner (BURNER_BURN_SESSION (self));
	if (!burner) {
		priv->is_valid = BURNER_SESSION_NO_OUTPUT;
		return BURNER_SESSION_NO_OUTPUT;
	}

	if (burner_drive_is_fake (burner))
		return burner_session_cfg_check_volume_size (self);

	medium = burner_drive_get_medium (burner);
	if (!medium) {
		priv->is_valid = BURNER_SESSION_NO_OUTPUT;
		return BURNER_SESSION_NO_OUTPUT;
	}

	/* Get both sizes if need be */
	if (!priv->disc_size) {
		priv->disc_size = burner_burn_session_get_available_medium_space (BURNER_BURN_SESSION (self));
		if (priv->disc_size < 0)
			priv->disc_size = 0;
	}

	BURNER_BURN_LOG ("Session size %lli/Disc size %lli",
			  priv->session_blocks,
			  priv->disc_size);

	if (priv->session_blocks < priv->disc_size) {
		priv->is_valid = BURNER_SESSION_VALID;
		return BURNER_SESSION_VALID;
	}

	/* Overburn is only for CDs */
	if ((burner_medium_get_status (medium) & BURNER_MEDIUM_CD) == 0) {
		priv->is_valid = BURNER_SESSION_INSUFFICIENT_SPACE;
		return BURNER_SESSION_INSUFFICIENT_SPACE;
	}

	/* The idea would be to test write the disc with cdrecord from /dev/null
	 * until there is an error and see how much we were able to write. So,
	 * when we propose overburning to the user, we could ask if he wants
	 * us to determine how much data can be written to a particular disc
	 * provided he has chosen a real disc. */
	max_sectors = priv->disc_size * 103 / 100;
	if (max_sectors < priv->session_blocks) {
		priv->is_valid = BURNER_SESSION_INSUFFICIENT_SPACE;
		return BURNER_SESSION_INSUFFICIENT_SPACE;
	}

	flags = burner_burn_session_get_flags (BURNER_BURN_SESSION (self));
	if (!(flags & BURNER_BURN_FLAG_OVERBURN)) {
		BurnerSessionCfgPrivate *priv;

		priv = BURNER_SESSION_CFG_PRIVATE (self);

		if (!(priv->supported & BURNER_BURN_FLAG_OVERBURN)) {
			priv->is_valid = BURNER_SESSION_INSUFFICIENT_SPACE;
			return BURNER_SESSION_INSUFFICIENT_SPACE;
		}

		priv->is_valid = BURNER_SESSION_OVERBURN_NECESSARY;
		return BURNER_SESSION_OVERBURN_NECESSARY;
	}

	priv->is_valid = BURNER_SESSION_VALID;
	return BURNER_SESSION_VALID;
}

static void
burner_session_cfg_set_tracks_audio_format (BurnerBurnSession *session,
					     BurnerStreamFormat format)
{
	GSList *tracks;
	GSList *iter;

	tracks = burner_burn_session_get_tracks (session);
	for (iter = tracks; iter; iter = iter->next) {
		BurnerTrack *track;

		track = iter->data;
		if (!BURNER_IS_TRACK_STREAM (track))
			continue;

		burner_track_stream_set_format (BURNER_TRACK_STREAM (track), format);
	}
}

static gboolean
burner_session_cfg_can_update (BurnerSessionCfg *self)
{
	BurnerSessionCfgPrivate *priv;
	BurnerBurnResult result;
	BurnerStatus *status;

	priv = BURNER_SESSION_CFG_PRIVATE (self);

	if (priv->disabled)
		return FALSE;

	if (priv->configuring)
		return FALSE;

	/* Make sure the session is ready */
	status = burner_status_new ();
	result = burner_burn_session_get_status (BURNER_BURN_SESSION (self), status);
	if (result == BURNER_BURN_NOT_READY || result == BURNER_BURN_RUNNING) {
		g_object_unref (status);

		priv->is_valid = BURNER_SESSION_NOT_READY;
		g_signal_emit (self,
			       session_cfg_signals [IS_VALID_SIGNAL],
			       0);
		return FALSE;
	}

	if (result == BURNER_BURN_ERR) {
		GError *error;

		error = burner_status_get_error (status);
		if (error) {
			if (error->code == BURNER_BURN_ERROR_EMPTY) {
				g_object_unref (status);
				g_error_free (error);

				priv->is_valid = BURNER_SESSION_EMPTY;
				g_signal_emit (self,
					       session_cfg_signals [IS_VALID_SIGNAL],
					       0);
				return FALSE;
			}

			g_error_free (error);
		}
	}
	g_object_unref (status);
	return TRUE;
}

static void
burner_session_cfg_update (BurnerSessionCfg *self)
{
	BurnerSessionCfgPrivate *priv;
	BurnerBurnResult result;
	BurnerDrive *burner;

	priv = BURNER_SESSION_CFG_PRIVATE (self);

	/* Make sure there is a source */
	if (priv->source) {
		burner_track_type_free (priv->source);
		priv->source = NULL;
	}

	priv->source = burner_track_type_new ();
	burner_burn_session_get_input_type (BURNER_BURN_SESSION (self), priv->source);

	if (burner_track_type_is_empty (priv->source)) {
		priv->is_valid = BURNER_SESSION_EMPTY;
		g_signal_emit (self,
			       session_cfg_signals [IS_VALID_SIGNAL],
			       0);
		return;
	}

	/* it can be empty with just an empty track */
	if (burner_track_type_get_has_medium (priv->source)
	&&  burner_track_type_get_medium_type (priv->source) == BURNER_MEDIUM_NONE) {
		priv->is_valid = BURNER_SESSION_NO_INPUT_MEDIUM;
		g_signal_emit (self,
			       session_cfg_signals [IS_VALID_SIGNAL],
			       0);
		return;
	}

	if (burner_track_type_get_has_image (priv->source)
	&&  burner_track_type_get_image_format (priv->source) == BURNER_IMAGE_FORMAT_NONE) {
		gchar *uri;
		GSList *tracks;

		tracks = burner_burn_session_get_tracks (BURNER_BURN_SESSION (self));

		/* It can be two cases:
		 * - no image set
		 * - image format cannot be detected */
		if (tracks) {
			BurnerTrack *track;

			track = tracks->data;
			uri = burner_track_image_get_source (BURNER_TRACK_IMAGE (track), TRUE);
			if (uri) {
				priv->is_valid = BURNER_SESSION_UNKNOWN_IMAGE;
				g_free (uri);
			}
			else
				priv->is_valid = BURNER_SESSION_NO_INPUT_IMAGE;
		}
		else
			priv->is_valid = BURNER_SESSION_NO_INPUT_IMAGE;

		g_signal_emit (self,
			       session_cfg_signals [IS_VALID_SIGNAL],
			       0);
		return;
	}

	/* make sure there is an output set */
	burner = burner_burn_session_get_burner (BURNER_BURN_SESSION (self));
	if (!burner) {
		priv->is_valid = BURNER_SESSION_NO_OUTPUT;
		g_signal_emit (self,
			       session_cfg_signals [IS_VALID_SIGNAL],
			       0);
		return;
	}

	/* In case the output was an image remove the path cache. It will be
	 * re-computed on demand. */
	if (priv->output) {
		g_free (priv->output);
		priv->output = NULL;
	}

	if (priv->output_format)
		priv->output_format = BURNER_IMAGE_FORMAT_NONE;

	/* Check that current input and output work */
	if (burner_track_type_get_has_stream (priv->source)) {
		if (priv->CD_TEXT_modified) {
			/* Try to redo what we undid (after all a new plugin
			 * could have been activated in the mean time ...) and
			 * see what happens */
			burner_track_type_set_stream_format (priv->source,
							      BURNER_METADATA_INFO|
							      burner_track_type_get_stream_format (priv->source));
			result = burner_burn_session_input_supported (BURNER_BURN_SESSION (self), priv->source, FALSE);
			if (result == BURNER_BURN_OK) {
				priv->CD_TEXT_modified = FALSE;

				priv->configuring = TRUE;
				burner_session_cfg_set_tracks_audio_format (BURNER_BURN_SESSION (self),
									     burner_track_type_get_stream_format (priv->source));
				priv->configuring = FALSE;
			}
			else {
				/* No, nothing's changed */
				burner_track_type_set_stream_format (priv->source,
								      (~BURNER_METADATA_INFO) &
								      burner_track_type_get_stream_format (priv->source));
				result = burner_burn_session_input_supported (BURNER_BURN_SESSION (self), priv->source, FALSE);
			}
		}
		else {
			result = burner_burn_session_can_burn (BURNER_BURN_SESSION (self), FALSE);

			if (result != BURNER_BURN_OK
			&& (burner_track_type_get_stream_format (priv->source) & BURNER_METADATA_INFO)) {
				/* Another special case in case some burning backends 
				 * don't support CD-TEXT for audio (libburn). If no
				 * other backend is available remove CD-TEXT option but
				 * tell user... */
				burner_track_type_set_stream_format (priv->source,
								      (~BURNER_METADATA_INFO) &
								      burner_track_type_get_stream_format (priv->source));

				result = burner_burn_session_input_supported (BURNER_BURN_SESSION (self), priv->source, FALSE);

				BURNER_BURN_LOG ("Tested support without Metadata information (result %d)", result);
				if (result == BURNER_BURN_OK) {
					priv->CD_TEXT_modified = TRUE;

					priv->configuring = TRUE;
					burner_session_cfg_set_tracks_audio_format (BURNER_BURN_SESSION (self),
										     burner_track_type_get_has_stream (priv->source));
					priv->configuring = FALSE;
				}
			}
		}
	}
	else if (burner_track_type_get_has_medium (priv->source)
	&&  (burner_track_type_get_medium_type (priv->source) & BURNER_MEDIUM_HAS_AUDIO)) {
		BurnerImageFormat format = BURNER_IMAGE_FORMAT_NONE;

		/* If we copy an audio disc check the image
		 * type we're writing to as it may mean that
		 * CD-TEXT cannot be copied.
		 * Make sure that the writing backend
		 * supports writing CD-TEXT?
		 * no, if a backend reports it supports an
		 * image type it should be able to burn all
		 * its data. */
		if (!burner_burn_session_is_dest_file (BURNER_BURN_SESSION (self))) {
			BurnerTrackType *tmp_type;

			tmp_type = burner_track_type_new ();

			/* NOTE: this is the same as burner_burn_session_supported () */
			result = burner_burn_session_get_tmp_image_type_same_src_dest (BURNER_BURN_SESSION (self), tmp_type);
			if (result == BURNER_BURN_OK) {
				if (burner_track_type_get_has_image (tmp_type)) {
					format = burner_track_type_get_image_format (tmp_type);
					priv->CD_TEXT_modified = (format & (BURNER_IMAGE_FORMAT_CDRDAO|BURNER_IMAGE_FORMAT_CUE)) == 0;
				}
				else if (burner_track_type_get_has_stream (tmp_type)) {
					/* FIXME: for the moment
					 * we consider that this
					 * type will always allow
					 * to copy CD-TEXT */
					priv->CD_TEXT_modified = FALSE;
				}
				else
					priv->CD_TEXT_modified = TRUE;
			}
			else
				priv->CD_TEXT_modified = TRUE;

			burner_track_type_free (tmp_type);

			BURNER_BURN_LOG ("Temporary image type %i", format);
		}
		else {
			result = burner_burn_session_can_burn (BURNER_BURN_SESSION (self), FALSE);
			format = burner_burn_session_get_output_format (BURNER_BURN_SESSION (self));
			priv->CD_TEXT_modified = (format & (BURNER_IMAGE_FORMAT_CDRDAO|BURNER_IMAGE_FORMAT_CUE)) == 0;
		}
	}
	else {
		/* Don't use flags as they'll be adapted later. */
		priv->CD_TEXT_modified = FALSE;
		result = burner_burn_session_can_burn (BURNER_BURN_SESSION (self), FALSE);
	}

	if (result != BURNER_BURN_OK) {
		if (burner_track_type_get_has_medium (priv->source)
		&& (burner_track_type_get_medium_type (priv->source) & BURNER_MEDIUM_PROTECTED)
		&&  burner_burn_library_input_supported (priv->source) != BURNER_BURN_OK) {
			/* This is a special case to display a helpful message */
			priv->is_valid = BURNER_SESSION_DISC_PROTECTED;
			g_signal_emit (self,
				       session_cfg_signals [IS_VALID_SIGNAL],
				       0);
		}
		else {
			priv->is_valid = BURNER_SESSION_NOT_SUPPORTED;
			g_signal_emit (self,
				       session_cfg_signals [IS_VALID_SIGNAL],
				       0);
		}

		return;
	}

	/* Special case for video projects */
	if (burner_track_type_get_has_stream (priv->source)
	&&  BURNER_STREAM_FORMAT_HAS_VIDEO (burner_track_type_get_stream_format (priv->source))) {
		/* Only set if it was not already set */
		if (burner_burn_session_tag_lookup (BURNER_BURN_SESSION (self), BURNER_VCD_TYPE, NULL) != BURNER_BURN_OK)
			burner_burn_session_tag_add_int (BURNER_BURN_SESSION (self),
							  BURNER_VCD_TYPE,
							  BURNER_SVCD);
	}

	/* Configure flags */
	priv->configuring = TRUE;

	if (burner_drive_is_fake (burner))
		/* Remove some impossible flags */
		burner_burn_session_remove_flag (BURNER_BURN_SESSION (self),
						  BURNER_BURN_FLAG_DUMMY|
						  BURNER_BURN_FLAG_NO_TMP_FILES);

	priv->configuring = FALSE;

	/* Finally check size */
	if (burner_burn_session_same_src_dest_drive (BURNER_BURN_SESSION (self))) {
		priv->is_valid = BURNER_SESSION_VALID;
		g_signal_emit (self,
			       session_cfg_signals [IS_VALID_SIGNAL],
			       0);
	}
	else {
		burner_session_cfg_check_size (self);
		g_signal_emit (self,
			       session_cfg_signals [IS_VALID_SIGNAL],
			       0);
	}
}

static void
burner_session_cfg_session_loaded (BurnerTrackDataCfg *track,
				    BurnerMedium *medium,
				    gboolean is_loaded,
				    BurnerSessionCfg *session)
{
	BurnerBurnFlag session_flags;
	BurnerSessionCfgPrivate *priv;

	priv = BURNER_SESSION_CFG_PRIVATE (session);
	if (priv->disabled)
		return;
	
	priv->session_blocks = 0;
	priv->session_size = 0;

	session_flags = burner_burn_session_get_flags (BURNER_BURN_SESSION (session));
	if (is_loaded) {
		/* Set the correct medium and add the flag */
		burner_burn_session_set_burner (BURNER_BURN_SESSION (session),
						 burner_medium_get_drive (medium));

		if ((session_flags & BURNER_BURN_FLAG_MERGE) == 0)
			burner_session_cfg_add_drive_properties_flags (session, BURNER_BURN_FLAG_MERGE);
	}
	else if ((session_flags & BURNER_BURN_FLAG_MERGE) != 0)
		burner_session_cfg_rm_drive_properties_flags (session, BURNER_BURN_FLAG_MERGE);
}

static void
burner_session_cfg_track_added (BurnerBurnSession *session,
				 BurnerTrack *track)
{
	BurnerSessionCfgPrivate *priv;

	if (!burner_session_cfg_can_update (BURNER_SESSION_CFG (session)))
		return;

	priv = BURNER_SESSION_CFG_PRIVATE (session);
	priv->session_blocks = 0;
	priv->session_size = 0;

	if (BURNER_IS_TRACK_DATA_CFG (track))
		g_signal_connect (track,
				  "session-loaded",
				  G_CALLBACK (burner_session_cfg_session_loaded),
				  session);

	/* A track was added: 
	 * - check if all flags are supported
	 * - check available formats for path
	 * - set one path */
	burner_session_cfg_update (BURNER_SESSION_CFG (session));
	burner_session_cfg_check_drive_settings (BURNER_SESSION_CFG (session));
}

static void
burner_session_cfg_track_removed (BurnerBurnSession *session,
				   BurnerTrack *track,
				   guint former_position)
{
	BurnerSessionCfgPrivate *priv;

	if (!burner_session_cfg_can_update (BURNER_SESSION_CFG (session)))
		return;

	priv = BURNER_SESSION_CFG_PRIVATE (session);
	priv->session_blocks = 0;
	priv->session_size = 0;

	/* Just in case */
	g_signal_handlers_disconnect_by_func (track,
					      burner_session_cfg_session_loaded,
					      session);

	/* If there were several tracks and at least one remained there is no
	 * use checking flags since the source type has not changed anyway.
	 * If there is no more track, there is no use checking flags anyway. */
	burner_session_cfg_update (BURNER_SESSION_CFG (session));
}

static void
burner_session_cfg_track_changed (BurnerBurnSession *session,
				   BurnerTrack *track)
{
	BurnerSessionCfgPrivate *priv;
	BurnerTrackType *current;

	if (!burner_session_cfg_can_update (BURNER_SESSION_CFG (session)))
		return;

	priv = BURNER_SESSION_CFG_PRIVATE (session);
	priv->session_blocks = 0;
	priv->session_size = 0;

	current = burner_track_type_new ();
	burner_burn_session_get_input_type (session, current);
	if (burner_track_type_equal (current, priv->source)) {
		/* This is a shortcut if the source type has not changed */
		burner_track_type_free (current);
		burner_session_cfg_check_size (BURNER_SESSION_CFG (session));
		g_signal_emit (session,
			       session_cfg_signals [IS_VALID_SIGNAL],
			       0);
 		return;
	}
	burner_track_type_free (current);

	/* when that happens it's mostly because a medium source changed, or
	 * a new image was set. 
	 * - check if all flags are supported
	 * - check available formats for path
	 * - set one path if need be */
	burner_session_cfg_update (BURNER_SESSION_CFG (session));
	burner_session_cfg_check_drive_settings (BURNER_SESSION_CFG (session));
}

static void
burner_session_cfg_output_changed (BurnerBurnSession *session,
				    BurnerMedium *former)
{
	BurnerSessionCfgPrivate *priv;

	if (!burner_session_cfg_can_update (BURNER_SESSION_CFG (session)))
		return;

	priv = BURNER_SESSION_CFG_PRIVATE (session);
	priv->disc_size = 0;

	/* Case for video project */
	if (priv->source
	&&  burner_track_type_get_has_stream (priv->source)
	&&  BURNER_STREAM_FORMAT_HAS_VIDEO (burner_track_type_get_stream_format (priv->source))) {
		BurnerMedia media;

		media = burner_burn_session_get_dest_media (session);
		if (media & BURNER_MEDIUM_DVD)
			burner_burn_session_tag_add_int (session,
			                                  BURNER_DVD_STREAM_FORMAT,
			                                  BURNER_AUDIO_FORMAT_AC3);
		else if (media & BURNER_MEDIUM_CD)
			burner_burn_session_tag_add_int (session,
							  BURNER_DVD_STREAM_FORMAT,
							  BURNER_AUDIO_FORMAT_MP2);
		else {
			BurnerImageFormat format;

			format = burner_burn_session_get_output_format (session);
			if (format == BURNER_IMAGE_FORMAT_CUE)
				burner_burn_session_tag_add_int (session,
								  BURNER_DVD_STREAM_FORMAT,
								  BURNER_AUDIO_FORMAT_MP2);
			else
				burner_burn_session_tag_add_int (session,
								  BURNER_DVD_STREAM_FORMAT,
								  BURNER_AUDIO_FORMAT_AC3);
		}
	}

	/* In this case need to :
	 * - check if all flags are supported
	 * - for images, set a path if it wasn't already set */
	burner_session_cfg_update (BURNER_SESSION_CFG (session));
	burner_session_cfg_check_drive_settings (BURNER_SESSION_CFG (session));
}

static void
burner_session_cfg_caps_changed (BurnerPluginManager *manager,
				  BurnerSessionCfg *self)
{
	BurnerSessionCfgPrivate *priv;

	if (!burner_session_cfg_can_update (self))
		return;
 
	priv = BURNER_SESSION_CFG_PRIVATE (self);
	priv->disc_size = 0;
	priv->session_blocks = 0;
	priv->session_size = 0;

	/* In this case we need to check if:
	 * - flags are supported or not supported anymore
	 * - image types as input/output are supported
	 * - if the current set of input/output still works */
	burner_session_cfg_update (self);
	burner_session_cfg_check_drive_settings (self);
}

/**
 * burner_session_cfg_add_flags:
 * @session: a #BurnerSessionCfg
 * @flags: a #BurnerBurnFlag
 *
 * Adds all flags from @flags that are supported.
 *
 **/

void
burner_session_cfg_add_flags (BurnerSessionCfg *session,
			       BurnerBurnFlag flags)
{
	BurnerSessionCfgPrivate *priv;

	priv = BURNER_SESSION_CFG_PRIVATE (session);

	if ((priv->supported & flags) != flags)
		return;

	if ((burner_burn_session_get_flags (BURNER_BURN_SESSION (session)) & flags) == flags)
		return;

	burner_session_cfg_add_drive_properties_flags (session, flags);

	if (burner_session_cfg_can_update (session))
		burner_session_cfg_update (session);
}

/**
 * burner_session_cfg_remove_flags:
 * @session: a #BurnerSessionCfg
 * @flags: a #BurnerBurnFlag
 *
 * Removes all flags that are not compulsory.
 *
 **/

void
burner_session_cfg_remove_flags (BurnerSessionCfg *session,
				  BurnerBurnFlag flags)
{
	burner_burn_session_remove_flag (BURNER_BURN_SESSION (session), flags);

	/* For this case reset all flags as some flags could
	 * be made available after the removal of one flag
	 * Example: After the removal of MULTI, FAST_BLANK
	 * becomes available again for DVDRW sequential */
	burner_session_cfg_set_drive_properties_default_flags (session);

	if (burner_session_cfg_can_update (session))
		burner_session_cfg_update (session);
}

/**
 * burner_session_cfg_is_supported:
 * @session: a #BurnerSessionCfg
 * @flag: a #BurnerBurnFlag
 *
 * Checks whether a particular flag is supported.
 *
 * Return value: a #gboolean. TRUE if it is supported;
 * FALSE otherwise.
 **/

gboolean
burner_session_cfg_is_supported (BurnerSessionCfg *session,
				  BurnerBurnFlag flag)
{
	BurnerSessionCfgPrivate *priv;

	priv = BURNER_SESSION_CFG_PRIVATE (session);
	return (priv->supported & flag) == flag;
}

/**
 * burner_session_cfg_is_compulsory:
 * @session: a #BurnerSessionCfg
 * @flag: a #BurnerBurnFlag
 *
 * Checks whether a particular flag is compulsory.
 *
 * Return value: a #gboolean. TRUE if it is compulsory;
 * FALSE otherwise.
 **/

gboolean
burner_session_cfg_is_compulsory (BurnerSessionCfg *session,
				   BurnerBurnFlag flag)
{
	BurnerSessionCfgPrivate *priv;

	priv = BURNER_SESSION_CFG_PRIVATE (session);
	return (priv->compulsory & flag) == flag;
}

static void
burner_session_cfg_init (BurnerSessionCfg *object)
{
	BurnerSessionCfgPrivate *priv;
	BurnerPluginManager *manager;

	priv = BURNER_SESSION_CFG_PRIVATE (object);

	priv->is_valid = BURNER_SESSION_EMPTY;
	manager = burner_plugin_manager_get_default ();
	g_signal_connect (manager,
	                  "caps-changed",
	                  G_CALLBACK (burner_session_cfg_caps_changed),
	                  object);
}

static void
burner_session_cfg_finalize (GObject *object)
{
	BurnerPluginManager *manager;
	BurnerSessionCfgPrivate *priv;
	GSList *tracks;

	priv = BURNER_SESSION_CFG_PRIVATE (object);

	tracks = burner_burn_session_get_tracks (BURNER_BURN_SESSION (object));
	for (; tracks; tracks = tracks->next) {
		BurnerTrack *track;

		track = tracks->data;
		g_signal_handlers_disconnect_by_func (track,
						      burner_session_cfg_session_loaded,
						      object);
	}

	manager = burner_plugin_manager_get_default ();
	g_signal_handlers_disconnect_by_func (manager,
	                                      burner_session_cfg_caps_changed,
	                                      object);

	if (priv->source) {
		burner_track_type_free (priv->source);
		priv->source = NULL;
	}

	if (priv->output) {
		g_free (priv->output);
		priv->output = NULL;
	}

	G_OBJECT_CLASS (burner_session_cfg_parent_class)->finalize (object);
}

static void
burner_session_cfg_class_init (BurnerSessionCfgClass *klass)
{
	GObjectClass* object_class = G_OBJECT_CLASS (klass);
	BurnerBurnSessionClass *session_class = BURNER_BURN_SESSION_CLASS (klass);

	g_type_class_add_private (klass, sizeof (BurnerSessionCfgPrivate));

	object_class->finalize = burner_session_cfg_finalize;

	session_class->get_output_path = burner_session_cfg_get_output_path;
	session_class->get_output_format = burner_session_cfg_get_output_format;
	session_class->set_output_image = burner_session_cfg_set_output_image;

	session_class->track_added = burner_session_cfg_track_added;
	session_class->track_removed = burner_session_cfg_track_removed;
	session_class->track_changed = burner_session_cfg_track_changed;
	session_class->output_changed = burner_session_cfg_output_changed;
	session_class->tag_changed = burner_session_cfg_tag_changed;

	session_cfg_signals [WRONG_EXTENSION_SIGNAL] =
		g_signal_new ("wrong_extension",
		              G_OBJECT_CLASS_TYPE (klass),
		              G_SIGNAL_RUN_LAST | G_SIGNAL_RUN_CLEANUP | G_SIGNAL_ACTION,
		              0,
		              NULL, NULL,
		              burner_marshal_BOOLEAN__VOID,
		              G_TYPE_BOOLEAN,
			      0,
		              G_TYPE_NONE);
	session_cfg_signals [IS_VALID_SIGNAL] =
		g_signal_new ("is_valid",
		              G_OBJECT_CLASS_TYPE (klass),
		              G_SIGNAL_RUN_LAST | G_SIGNAL_RUN_CLEANUP | G_SIGNAL_ACTION,
		              0,
		              NULL, NULL,
		              g_cclosure_marshal_VOID__VOID,
		              G_TYPE_NONE,
			      0,
		              G_TYPE_NONE);
}

/**
 * burner_session_cfg_new:
 *
 *  Creates a new #BurnerSessionCfg object.
 *
 * Return value: a #BurnerSessionCfg object.
 **/

BurnerSessionCfg *
burner_session_cfg_new (void)
{
	return g_object_new (BURNER_TYPE_SESSION_CFG, NULL);
}
