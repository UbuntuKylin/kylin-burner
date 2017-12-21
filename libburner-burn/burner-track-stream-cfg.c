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

#include <glib.h>
#include <glib/gi18n-lib.h>
#include <glib-object.h>
#include <gio/gio.h>

#include "burner-misc.h"

#include "burn-debug.h"

#include "burner-track-stream-cfg.h"
#include "burner-io.h"
#include "burner-tags.h"

static BurnerIOJobCallbacks *io_methods = NULL;

typedef struct _BurnerTrackStreamCfgPrivate BurnerTrackStreamCfgPrivate;
struct _BurnerTrackStreamCfgPrivate
{
	BurnerIOJobBase *load_uri;

	GFileMonitor *monitor;

	GError *error;

	guint loading:1;
};

#define BURNER_TRACK_STREAM_CFG_PRIVATE(o)  (G_TYPE_INSTANCE_GET_PRIVATE ((o), BURNER_TYPE_TRACK_STREAM_CFG, BurnerTrackStreamCfgPrivate))

G_DEFINE_TYPE (BurnerTrackStreamCfg, burner_track_stream_cfg, BURNER_TYPE_TRACK_STREAM);

static void
burner_track_stream_cfg_file_changed (GFileMonitor *monitor,
								GFile *file,
								GFile *other_file,
								GFileMonitorEvent event,
								BurnerTrackStream *track)
{
	BurnerTrackStreamCfgPrivate *priv;
	gchar *name;

	priv = BURNER_TRACK_STREAM_CFG_PRIVATE (track);

        switch (event) {
 /*               case G_FILE_MONITOR_EVENT_CHANGED:
                        return;
*/
                case G_FILE_MONITOR_EVENT_DELETED:
                        g_object_unref (priv->monitor);
                        priv->monitor = NULL;

			name = g_file_get_basename (file);
			priv->error = g_error_new (BURNER_BURN_ERROR,
								  BURNER_BURN_ERROR_FILE_NOT_FOUND,
								  /* Translators: %s is the name of the file that has just been deleted */
								  _("\"%s\" was removed from the file system."),
								 name);
			g_free (name);
                        break;

                default:
                        return;
        }

        burner_track_changed (BURNER_TRACK (track));
}

static void
burner_track_stream_cfg_results_cb (GObject *obj,
				     GError *error,
				     const gchar *uri,
				     GFileInfo *info,
				     gpointer user_data)
{
	GFile *file;
	guint64 len;
	GObject *snapshot;
	BurnerTrackStreamCfgPrivate *priv;

	priv = BURNER_TRACK_STREAM_CFG_PRIVATE (obj);
	priv->loading = FALSE;

	/* Check the return status for this file */
	if (error) {
		priv->error = g_error_copy (error);
		burner_track_changed (BURNER_TRACK (obj));
		return;
	}

	/* FIXME: we don't know whether it's audio or video that is required */
	if (g_file_info_get_file_type (info) == G_FILE_TYPE_DIRECTORY) {
		/* This error is special as it can be recovered from */
		priv->error = g_error_new (BURNER_BURN_ERROR,
					   BURNER_BURN_ERROR_FILE_FOLDER,
					   _("Directories cannot be added to video or audio discs"));
		burner_track_changed (BURNER_TRACK (obj));
		return;
	}

	if (g_file_info_get_file_type (info) == G_FILE_TYPE_REGULAR
	&&  (!strcmp (g_file_info_get_content_type (info), "audio/x-scpls")
	||   !strcmp (g_file_info_get_content_type (info), "audio/x-ms-asx")
	||   !strcmp (g_file_info_get_content_type (info), "audio/x-mp3-playlist")
	||   !strcmp (g_file_info_get_content_type (info), "audio/x-mpegurl"))) {
		/* This error is special as it can be recovered from */
		priv->error = g_error_new (BURNER_BURN_ERROR,
					   BURNER_BURN_ERROR_FILE_PLAYLIST,
					   _("Playlists cannot be added to video or audio discs"));

		burner_track_changed (BURNER_TRACK (obj));
		return;
	}

	if (g_file_info_get_file_type (info) != G_FILE_TYPE_REGULAR
	|| (!g_file_info_get_attribute_boolean (info, BURNER_IO_HAS_VIDEO)
	&&  !g_file_info_get_attribute_boolean (info, BURNER_IO_HAS_AUDIO))) {
		gchar *name;

		BURNER_GET_BASENAME_FOR_DISPLAY (uri, name);
		priv->error = g_error_new (BURNER_BURN_ERROR,
					   BURNER_BURN_ERROR_GENERAL,
					   /* Translators: %s is the name of the file */
					   _("\"%s\" is not suitable for audio or video media"),
					   name);
		g_free (name);

		burner_track_changed (BURNER_TRACK (obj));
		return;
	}

	/* Also make sure it's duration is appropriate (!= 0) */
	len = g_file_info_get_attribute_uint64 (info, BURNER_IO_LEN);
	if (len <= 0) {
		gchar *name;

		BURNER_GET_BASENAME_FOR_DISPLAY (uri, name);
		priv->error = g_error_new (BURNER_BURN_ERROR,
					   BURNER_BURN_ERROR_GENERAL,
					   /* Translators: %s is the name of the file */
					   _("\"%s\" is not suitable for audio or video media"),
					   name);
		g_free (name);

		burner_track_changed (BURNER_TRACK (obj));
		return;
	}

	if (g_file_info_get_is_symlink (info)) {
		gchar *sym_uri;

		sym_uri = g_strconcat ("file://", g_file_info_get_symlink_target (info), NULL);
		if (BURNER_TRACK_STREAM_CLASS (burner_track_stream_cfg_parent_class)->set_source)
			BURNER_TRACK_STREAM_CLASS (burner_track_stream_cfg_parent_class)->set_source (BURNER_TRACK_STREAM (obj), sym_uri);

		g_free (sym_uri);
	}

	/* Check whether the stream is wav+dts as it can be burnt as such */
	if (g_file_info_get_attribute_boolean (info, BURNER_IO_HAS_DTS)) {
		BURNER_BURN_LOG ("Track has DTS");
		BURNER_TRACK_STREAM_CLASS (burner_track_stream_cfg_parent_class)->set_format (BURNER_TRACK_STREAM (obj), 
		                                                                                BURNER_AUDIO_FORMAT_DTS|
		                                                                                BURNER_METADATA_INFO);
	}
	else if (BURNER_TRACK_STREAM_CLASS (burner_track_stream_cfg_parent_class)->set_format)
		BURNER_TRACK_STREAM_CLASS (burner_track_stream_cfg_parent_class)->set_format (BURNER_TRACK_STREAM (obj),
		                                                                                (g_file_info_get_attribute_boolean (info, BURNER_IO_HAS_VIDEO)?
		                                                                                 BURNER_VIDEO_FORMAT_UNDEFINED:BURNER_AUDIO_FORMAT_NONE)|
		                                                                                (g_file_info_get_attribute_boolean (info, BURNER_IO_HAS_AUDIO)?
		                                                                                 BURNER_AUDIO_FORMAT_UNDEFINED:BURNER_AUDIO_FORMAT_NONE)|
		                                                                                BURNER_METADATA_INFO);

	/* Size/length. Only set when end value has not been already set.
	 * Fix #607752 -  audio track start and end points are overwritten after
	 * being read from a project file.
	 * We don't want to set a new len if one has been set already. Nevertheless
	 * if the length we detected is smaller than the one that was set we go
	 * for the new one. */
	if (BURNER_TRACK_STREAM_CLASS (burner_track_stream_cfg_parent_class)->set_boundaries) {
		gint64 min_start;

		/* Make sure that the start value is coherent */
		min_start = (len - BURNER_MIN_STREAM_LENGTH) >= 0? (len - BURNER_MIN_STREAM_LENGTH):0;
		if (min_start && burner_track_stream_get_start (BURNER_TRACK_STREAM (obj)) > min_start) {
			BURNER_TRACK_STREAM_CLASS (burner_track_stream_cfg_parent_class)->set_boundaries (BURNER_TRACK_STREAM (obj),
													    min_start,
													    -1,
													    -1);
		}

		if (burner_track_stream_get_end (BURNER_TRACK_STREAM (obj)) > len
		||  burner_track_stream_get_end (BURNER_TRACK_STREAM (obj)) <= 0) {
			/* Don't set either gap or start to make sure we don't remove
			 * values set by project parser or values set from the beginning
			 * Fix #607752 -  audio track start and end points are overwritten
			 * after being read from a project file */
			BURNER_TRACK_STREAM_CLASS (burner_track_stream_cfg_parent_class)->set_boundaries (BURNER_TRACK_STREAM (obj),
													    -1,
													    len,
													    -1);
		}
	}

	snapshot = g_file_info_get_attribute_object (info, BURNER_IO_THUMBNAIL);
	if (snapshot) {
		GValue *value;

		value = g_new0 (GValue, 1);
		g_value_init (value, GDK_TYPE_PIXBUF);
		g_value_set_object (value, g_object_ref (snapshot));
		burner_track_tag_add (BURNER_TRACK (obj),
				       BURNER_TRACK_STREAM_THUMBNAIL_TAG,
				       value);
	}

	if (g_file_info_get_content_type (info)) {
		const gchar *icon_string = "text-x-preview";
		GtkIconTheme *theme;
		GIcon *icon;

		theme = gtk_icon_theme_get_default ();

		/* NOTE: implemented in glib 2.15.6 (not for windows though) */
		icon = g_content_type_get_icon (g_file_info_get_content_type (info));
		if (G_IS_THEMED_ICON (icon)) {
			const gchar * const *names = NULL;

			names = g_themed_icon_get_names (G_THEMED_ICON (icon));
			if (names) {
				gint i;

				for (i = 0; names [i]; i++) {
					if (gtk_icon_theme_has_icon (theme, names [i])) {
						icon_string = names [i];
						break;
					}
				}
			}
		}

		burner_track_tag_add_string (BURNER_TRACK (obj),
					      BURNER_TRACK_STREAM_MIME_TAG,
					      icon_string);
		g_object_unref (icon);
	}

	/* Get the song info */
	if (g_file_info_get_attribute_string (info, BURNER_IO_TITLE)
	&& !burner_track_tag_lookup_string (BURNER_TRACK (obj), BURNER_TRACK_STREAM_TITLE_TAG))
		burner_track_tag_add_string (BURNER_TRACK (obj),
					      BURNER_TRACK_STREAM_TITLE_TAG,
					      g_file_info_get_attribute_string (info, BURNER_IO_TITLE));
	if (g_file_info_get_attribute_string (info, BURNER_IO_ARTIST)
	&& !burner_track_tag_lookup_string (BURNER_TRACK (obj), BURNER_TRACK_STREAM_ARTIST_TAG))
		burner_track_tag_add_string (BURNER_TRACK (obj),
					      BURNER_TRACK_STREAM_ARTIST_TAG,
					      g_file_info_get_attribute_string (info, BURNER_IO_ARTIST));
	if (g_file_info_get_attribute_string (info, BURNER_IO_ALBUM)
	&& !burner_track_tag_lookup_string (BURNER_TRACK (obj), BURNER_TRACK_STREAM_ALBUM_TAG))
		burner_track_tag_add_string (BURNER_TRACK (obj),
					      BURNER_TRACK_STREAM_ALBUM_TAG,
					      g_file_info_get_attribute_string (info, BURNER_IO_ALBUM));
	if (g_file_info_get_attribute_string (info, BURNER_IO_COMPOSER)
	&& !burner_track_tag_lookup_string (BURNER_TRACK (obj), BURNER_TRACK_STREAM_COMPOSER_TAG))
		burner_track_tag_add_string (BURNER_TRACK (obj),
					      BURNER_TRACK_STREAM_COMPOSER_TAG,
					      g_file_info_get_attribute_string (info, BURNER_IO_COMPOSER));
	if (g_file_info_get_attribute_string (info, BURNER_IO_ISRC)
	&& !burner_track_tag_lookup_string (BURNER_TRACK (obj), BURNER_TRACK_STREAM_ISRC_TAG))
		burner_track_tag_add_string (BURNER_TRACK (obj),
					      BURNER_TRACK_STREAM_ISRC_TAG,
					      g_file_info_get_attribute_string (info, BURNER_IO_ISRC));

	/* Start monitoring it */
	file = g_file_new_for_uri (uri);
	priv->monitor = g_file_monitor_file (file,
	                                     G_FILE_MONITOR_NONE,
	                                     NULL,
	                                     NULL);
	g_object_unref (file);

	g_signal_connect (priv->monitor,
	                  "changed",
	                  G_CALLBACK (burner_track_stream_cfg_file_changed),
	                  obj);

	burner_track_changed (BURNER_TRACK (obj));
}

static void
burner_track_stream_cfg_get_info (BurnerTrackStreamCfg *track)
{
	BurnerTrackStreamCfgPrivate *priv;
	gchar *uri;

	priv = BURNER_TRACK_STREAM_CFG_PRIVATE (track);

	if (priv->error) {
		g_error_free (priv->error);
		priv->error = NULL;
	}

	/* get info async for the file */
	if (!priv->load_uri) {
		if (!io_methods)
			io_methods = burner_io_register_job_methods (burner_track_stream_cfg_results_cb,
			                                              NULL,
			                                              NULL);

		priv->load_uri = burner_io_register_with_methods (G_OBJECT (track), io_methods);
	}

	priv->loading = TRUE;
	uri = burner_track_stream_get_source (BURNER_TRACK_STREAM (track), TRUE);
	burner_io_get_file_info (uri,
				  priv->load_uri,
				  BURNER_IO_INFO_PERM|
				  BURNER_IO_INFO_MIME|
				  BURNER_IO_INFO_URGENT|
				  BURNER_IO_INFO_METADATA|
				  BURNER_IO_INFO_METADATA_MISSING_CODEC|
				  BURNER_IO_INFO_METADATA_THUMBNAIL,
				  track);
	g_free (uri);
}

static BurnerBurnResult
burner_track_stream_cfg_set_source (BurnerTrackStream *track,
				     const gchar *uri)
{
	BurnerTrackStreamCfgPrivate *priv;

	priv = BURNER_TRACK_STREAM_CFG_PRIVATE (track);
	if (priv->monitor) {
		g_object_unref (priv->monitor);
		priv->monitor = NULL;
	}

	if (priv->load_uri)
		burner_io_cancel_by_base (priv->load_uri);

	if (BURNER_TRACK_STREAM_CLASS (burner_track_stream_cfg_parent_class)->set_source)
		BURNER_TRACK_STREAM_CLASS (burner_track_stream_cfg_parent_class)->set_source (track, uri);

	burner_track_stream_cfg_get_info (BURNER_TRACK_STREAM_CFG (track));
	burner_track_changed (BURNER_TRACK (track));
	return BURNER_BURN_OK;
}

static BurnerBurnResult
burner_track_stream_cfg_get_status (BurnerTrack *track,
				     BurnerStatus *status)
{
	BurnerTrackStreamCfgPrivate *priv;

	priv = BURNER_TRACK_STREAM_CFG_PRIVATE (track);

	if (priv->error) {
		burner_status_set_error (status, g_error_copy (priv->error));
		return BURNER_BURN_ERR;
	}

	if (priv->loading) {
		if (status)
			burner_status_set_not_ready (status,
						      -1.0,
						      _("Analysing video files"));

		return BURNER_BURN_NOT_READY;
	}

	if (status)
		burner_status_set_completed (status);

	return BURNER_TRACK_CLASS (burner_track_stream_cfg_parent_class)->get_status (track, status);
}

static void
burner_track_stream_cfg_init (BurnerTrackStreamCfg *object)
{ }

static void
burner_track_stream_cfg_finalize (GObject *object)
{
	BurnerTrackStreamCfgPrivate *priv;

	priv = BURNER_TRACK_STREAM_CFG_PRIVATE (object);

	if (priv->load_uri) {
		burner_io_cancel_by_base (priv->load_uri);

		if (io_methods->ref == 1)
			io_methods = NULL;

		burner_io_job_base_free (priv->load_uri);
		priv->load_uri = NULL;
	}

	if (priv->monitor) {
		g_object_unref (priv->monitor);
		priv->monitor = NULL;
	}

	if (priv->error) {
		g_error_free (priv->error);
		priv->error = NULL;
	}

	G_OBJECT_CLASS (burner_track_stream_cfg_parent_class)->finalize (object);
}

static void
burner_track_stream_cfg_class_init (BurnerTrackStreamCfgClass *klass)
{
	GObjectClass* object_class = G_OBJECT_CLASS (klass);
	BurnerTrackClass* track_class = BURNER_TRACK_CLASS (klass);
	BurnerTrackStreamClass *parent_class = BURNER_TRACK_STREAM_CLASS (klass);

	g_type_class_add_private (klass, sizeof (BurnerTrackStreamCfgPrivate));

	object_class->finalize = burner_track_stream_cfg_finalize;

	track_class->get_status = burner_track_stream_cfg_get_status;

	parent_class->set_source = burner_track_stream_cfg_set_source;
}

/**
 * burner_track_stream_cfg_new:
 *
 *  Creates a new #BurnerTrackStreamCfg object.
 *
 * Return value: a #BurnerTrackStreamCfg object.
 **/

BurnerTrackStreamCfg *
burner_track_stream_cfg_new (void)
{
	return g_object_new (BURNER_TYPE_TRACK_STREAM_CFG, NULL);
}

