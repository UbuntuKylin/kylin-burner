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
#include <glib/gi18n-lib.h>
#include <gmodule.h>

#include <gst/gst.h>

#include "burner-tags.h"

#include "burn-job.h"
#include "burn-normalize.h"
#include "burner-plugin-registration.h"


#define BURNER_TYPE_NORMALIZE             (burner_normalize_get_type ())
#define BURNER_NORMALIZE(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), BURNER_TYPE_NORMALIZE, BurnerNormalize))
#define BURNER_NORMALIZE_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), BURNER_TYPE_NORMALIZE, BurnerNormalizeClass))
#define BURNER_IS_NORMALIZE(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BURNER_TYPE_NORMALIZE))
#define BURNER_IS_NORMALIZE_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), BURNER_TYPE_NORMALIZE))
#define BURNER_NORMALIZE_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), BURNER_TYPE_NORMALIZE, BurnerNormalizeClass))

BURNER_PLUGIN_BOILERPLATE (BurnerNormalize, burner_normalize, BURNER_TYPE_JOB, BurnerJob);

typedef struct _BurnerNormalizePrivate BurnerNormalizePrivate;
struct _BurnerNormalizePrivate
{
	GstElement *pipeline;
	GstElement *analysis;
	GstElement *decode;
	GstElement *resample;

	GSList *tracks;
	BurnerTrack *track;

	gdouble album_peak;
	gdouble album_gain;
	gdouble track_peak;
	gdouble track_gain;
};

#define BURNER_NORMALIZE_PRIVATE(o)  (G_TYPE_INSTANCE_GET_PRIVATE ((o), BURNER_TYPE_NORMALIZE, BurnerNormalizePrivate))

static GObjectClass *parent_class = NULL;


static gboolean
burner_normalize_bus_messages (GstBus *bus,
				GstMessage *msg,
				BurnerNormalize *normalize);

static void
burner_normalize_stop_pipeline (BurnerNormalize *normalize)
{
	BurnerNormalizePrivate *priv;

	priv = BURNER_NORMALIZE_PRIVATE (normalize);
	if (!priv->pipeline)
		return;

	gst_element_set_state (priv->pipeline, GST_STATE_NULL);
	gst_object_unref (GST_OBJECT (priv->pipeline));
	priv->pipeline = NULL;
	priv->resample = NULL;
	priv->analysis = NULL;
	priv->decode = NULL;
}

static void
burner_normalize_new_decoded_pad_cb (GstElement *decode,
				      GstPad *pad,
				      BurnerNormalize *normalize)
{
	GstPad *sink;
	GstCaps *caps;
	GstStructure *structure;
	BurnerNormalizePrivate *priv;

	priv = BURNER_NORMALIZE_PRIVATE (normalize);

	sink = gst_element_get_static_pad (priv->resample, "sink");
	if (GST_PAD_IS_LINKED (sink)) {
		BURNER_JOB_LOG (normalize, "New decoded pad already linked");
		return;
	}

	/* make sure we only have audio */
	/* FIXME: get_current_caps() doesn't always seem to work yet here */
	caps = gst_pad_query_caps (pad, NULL);
	if (!caps)
		return;

	structure = gst_caps_get_structure (caps, 0);
	if (structure && g_strrstr (gst_structure_get_name (structure), "audio")) {
		if (gst_pad_link (pad, sink) != GST_PAD_LINK_OK) {
			BURNER_JOB_LOG (normalize, "New decoded pad can't be linked");
			burner_job_error (BURNER_JOB (normalize), NULL);
		}
		else
			BURNER_JOB_LOG (normalize, "New decoded pad linked");
	}
	else
		BURNER_JOB_LOG (normalize, "New decoded pad with unsupported stream time");

	gst_object_unref (sink);
	gst_caps_unref (caps);
}

  static gboolean
burner_normalize_build_pipeline (BurnerNormalize *normalize,
                                  const gchar *uri,
                                  GstElement *analysis,
                                  GError **error)
{
	GstBus *bus = NULL;
	GstElement *source;
	GstElement *decode;
	GstElement *pipeline;
	GstElement *sink = NULL;
	GstElement *convert = NULL;
	GstElement *resample = NULL;
	BurnerNormalizePrivate *priv;

	priv = BURNER_NORMALIZE_PRIVATE (normalize);

	BURNER_JOB_LOG (normalize, "Creating new pipeline");

	/* create filesrc ! decodebin ! audioresample ! audioconvert ! rganalysis ! fakesink */
	pipeline = gst_pipeline_new (NULL);
	priv->pipeline = pipeline;

	/* a new source is created */
	source = gst_element_make_from_uri (GST_URI_SRC, uri, NULL, NULL);
	if (source == NULL) {
		g_set_error (error,
			     BURNER_BURN_ERROR,
			     BURNER_BURN_ERROR_GENERAL,
			     _("%s element could not be created"),
			     "\"Source\"");
		goto error;
	}
	gst_bin_add (GST_BIN (priv->pipeline), source);
	g_object_set (source,
		      "typefind", FALSE,
		      NULL);

	/* decode */
	decode = gst_element_factory_make ("decodebin", NULL);
	if (decode == NULL) {
		g_set_error (error,
			     BURNER_BURN_ERROR,
			     BURNER_BURN_ERROR_GENERAL,
			     _("%s element could not be created"),
			     "\"Decodebin\"");
		goto error;
	}
	gst_bin_add (GST_BIN (pipeline), decode);
	priv->decode = decode;

	if (!gst_element_link (source, decode)) {
		BURNER_JOB_LOG (normalize, "Elements could not be linked");
		g_set_error (error,
			     BURNER_BURN_ERROR,
			     BURNER_BURN_ERROR_GENERAL,
		             _("Impossible to link plugin pads"));
		goto error;
	}

	/* audioconvert */
	convert = gst_element_factory_make ("audioconvert", NULL);
	if (convert == NULL) {
		g_set_error (error,
			     BURNER_BURN_ERROR,
			     BURNER_BURN_ERROR_GENERAL,
			     _("%s element could not be created"),
			     "\"Audioconvert\"");
		goto error;
	}
	gst_bin_add (GST_BIN (pipeline), convert);

	/* audioresample */
	resample = gst_element_factory_make ("audioresample", NULL);
	if (resample == NULL) {
		g_set_error (error,
			     BURNER_BURN_ERROR,
			     BURNER_BURN_ERROR_GENERAL,
			     _("%s element could not be created"),
			     "\"Audioresample\"");
		goto error;
	}
	gst_bin_add (GST_BIN (pipeline), resample);
	priv->resample = resample;

	/* rganalysis: set the number of tracks to be expected */
	priv->analysis = analysis;
	gst_bin_add (GST_BIN (pipeline), analysis);

	/* sink */
	sink = gst_element_factory_make ("fakesink", NULL);
	if (!sink) {
		g_set_error (error,
			     BURNER_BURN_ERROR,
			     BURNER_BURN_ERROR_GENERAL,
			     _("%s element could not be created"),
			     "\"Fakesink\"");
		goto error;
	}
	gst_bin_add (GST_BIN (pipeline), sink);
	g_object_set (sink,
		      "sync", FALSE,
		      NULL);

	/* link everything */
	g_signal_connect (G_OBJECT (decode),
	                  "pad-added",
	                  G_CALLBACK (burner_normalize_new_decoded_pad_cb),
	                  normalize);
	if (!gst_element_link_many (resample,
	                            convert,
	                            analysis,
	                            sink,
	                            NULL)) {
		g_set_error (error,
			     BURNER_BURN_ERROR,
			     BURNER_BURN_ERROR_GENERAL,
		             _("Impossible to link plugin pads"));
	}

	/* connect to the bus */	
	bus = gst_pipeline_get_bus (GST_PIPELINE (priv->pipeline));
	gst_bus_add_watch (bus,
			   (GstBusFunc) burner_normalize_bus_messages,
			   normalize);
	gst_object_unref (bus);

	gst_element_set_state (priv->pipeline, GST_STATE_PLAYING);

	return TRUE;

error:

	if (error && (*error))
		BURNER_JOB_LOG (normalize,
				 "can't create object : %s \n",
				 (*error)->message);

	gst_object_unref (GST_OBJECT (pipeline));
	return FALSE;
}

static BurnerBurnResult
burner_normalize_set_next_track (BurnerJob *job,
                                  GError **error)
{
	gchar *uri;
	GValue *value;
	GstElement *analysis;
	BurnerTrackType *type;
	BurnerTrack *track = NULL;
	gboolean dts_allowed = FALSE;
	BurnerNormalizePrivate *priv;

	priv = BURNER_NORMALIZE_PRIVATE (job);

	/* See if dts is allowed */
	value = NULL;
	burner_job_tag_lookup (job, BURNER_SESSION_STREAM_AUDIO_FORMAT, &value);
	if (value)
		dts_allowed = (g_value_get_int (value) & BURNER_AUDIO_FORMAT_DTS) != 0;

	type = burner_track_type_new ();
	while (priv->tracks && priv->tracks->data) {
		track = priv->tracks->data;
		priv->tracks = g_slist_remove (priv->tracks, track);

		burner_track_get_track_type (track, type);
		if (burner_track_type_get_has_stream (type)) {
			if (!dts_allowed)
				break;

			/* skip DTS tracks as we won't modify them */
			if ((burner_track_type_get_stream_format (type) & BURNER_AUDIO_FORMAT_DTS) == 0) 
				break;

			BURNER_JOB_LOG (job, "Skipped DTS track");
		}

		track = NULL;
	}
	burner_track_type_free (type);

	if (!track)
		return BURNER_BURN_OK;

	if (!priv->analysis) {
		analysis = gst_element_factory_make ("rganalysis", NULL);
		if (analysis == NULL) {
			g_set_error (error,
				     BURNER_BURN_ERROR,
				     BURNER_BURN_ERROR_GENERAL,
				     _("%s element could not be created"),
				     "\"Rganalysis\"");
			return BURNER_BURN_ERR;
		}

		g_object_set (analysis,
			      "num-tracks", g_slist_length (priv->tracks),
			      NULL);
	}
	else {
		/* destroy previous pipeline but save our plugin */
		analysis = g_object_ref (priv->analysis);

		/* NOTE: why lock state? because otherwise analysis would lose all 
		 * information about tracks already analysed by going into the NULL
		 * state. */
		gst_element_set_locked_state (analysis, TRUE);
		gst_bin_remove (GST_BIN (priv->pipeline), analysis);
		burner_normalize_stop_pipeline (BURNER_NORMALIZE (job));
		gst_element_set_locked_state (analysis, FALSE);
	}

	/* create a new one */
	priv->track = track;
	uri = burner_track_stream_get_source (BURNER_TRACK_STREAM (track), TRUE);
	BURNER_JOB_LOG (job, "Analysing track %s", uri);

	if (!burner_normalize_build_pipeline (BURNER_NORMALIZE (job), uri, analysis, error)) {
		g_free (uri);
		return BURNER_BURN_ERR;
	}

	g_free (uri);
	return BURNER_BURN_RETRY;
}

static BurnerBurnResult
burner_normalize_stop (BurnerJob *job,
			GError **error)
{
	BurnerNormalizePrivate *priv;

	priv = BURNER_NORMALIZE_PRIVATE (job);

	burner_normalize_stop_pipeline (BURNER_NORMALIZE (job));
	if (priv->tracks) {
		g_slist_free (priv->tracks);
		priv->tracks = NULL;
	}

	priv->track = NULL;

	return BURNER_BURN_OK;
}

static void
foreach_tag (const GstTagList *list,
	     const gchar *tag,
	     BurnerNormalize *normalize)
{
	gdouble value = 0.0;
	BurnerNormalizePrivate *priv;

	priv = BURNER_NORMALIZE_PRIVATE (normalize);

	/* Those next two are generated at the end only */
	if (!strcmp (tag, GST_TAG_ALBUM_GAIN)) {
		gst_tag_list_get_double (list, tag, &value);
		priv->album_gain = value;
	}
	else if (!strcmp (tag, GST_TAG_ALBUM_PEAK)) {
		gst_tag_list_get_double (list, tag, &value);
		priv->album_peak = value;
	}
	else if (!strcmp (tag, GST_TAG_TRACK_PEAK)) {
		gst_tag_list_get_double (list, tag, &value);
		priv->track_peak = value;
	}
	else if (!strcmp (tag, GST_TAG_TRACK_GAIN)) {
		gst_tag_list_get_double (list, tag, &value);
		priv->track_gain = value;
	}
}

static void
burner_normalize_song_end_reached (BurnerNormalize *normalize)
{
	GValue *value;
	GError *error = NULL;
	BurnerBurnResult result;
	BurnerNormalizePrivate *priv;

	priv = BURNER_NORMALIZE_PRIVATE (normalize);
	
	/* finished track: set tags */
	BURNER_JOB_LOG (normalize,
			 "Setting track peak (%lf) and gain (%lf)",
			 priv->track_peak,
			 priv->track_gain);

	value = g_new0 (GValue, 1);
	g_value_init (value, G_TYPE_DOUBLE);
	g_value_set_double (value, priv->track_peak);
	burner_track_tag_add (priv->track,
			       BURNER_TRACK_PEAK_VALUE,
			       value);

	value = g_new0 (GValue, 1);
	g_value_init (value, G_TYPE_DOUBLE);
	g_value_set_double (value, priv->track_gain);
	burner_track_tag_add (priv->track,
			       BURNER_TRACK_GAIN_VALUE,
			       value);

	priv->track_peak = 0.0;
	priv->track_gain = 0.0;

	result = burner_normalize_set_next_track (BURNER_JOB (normalize), &error);
	if (result == BURNER_BURN_OK) {
		BURNER_JOB_LOG (normalize,
				 "Setting album peak (%lf) and gain (%lf)",
				 priv->album_peak,
				 priv->album_gain);

		/* finished: set tags */
		value = g_new0 (GValue, 1);
		g_value_init (value, G_TYPE_DOUBLE);
		g_value_set_double (value, priv->album_peak);
		burner_job_tag_add (BURNER_JOB (normalize),
				     BURNER_ALBUM_PEAK_VALUE,
				     value);

		value = g_new0 (GValue, 1);
		g_value_init (value, G_TYPE_DOUBLE);
		g_value_set_double (value, priv->album_gain);
		burner_job_tag_add (BURNER_JOB (normalize),
				     BURNER_ALBUM_GAIN_VALUE,
				     value);

		burner_job_finished_session (BURNER_JOB (normalize));
		return;
	}

	/* jump to next track */
	if (result == BURNER_BURN_ERR) {
		burner_job_error (BURNER_JOB (normalize), error);
		return;
	}
}

static gboolean
burner_normalize_bus_messages (GstBus *bus,
				GstMessage *msg,
				BurnerNormalize *normalize)
{
	GstTagList *tags = NULL;
	GError *error = NULL;
	gchar *debug;

	switch (GST_MESSAGE_TYPE (msg)) {
	case GST_MESSAGE_TAG:
		/* This is the information we've been waiting for.
		 * NOTE: levels for whole album is delivered at the end */
		gst_message_parse_tag (msg, &tags);
		gst_tag_list_foreach (tags, (GstTagForeachFunc) foreach_tag, normalize);
		gst_tag_list_free (tags);
		return TRUE;

	case GST_MESSAGE_ERROR:
		gst_message_parse_error (msg, &error, &debug);
		BURNER_JOB_LOG (normalize, debug);
		g_free (debug);

	        burner_job_error (BURNER_JOB (normalize), error);
		return FALSE;

	case GST_MESSAGE_EOS:
		burner_normalize_song_end_reached (normalize);
		return FALSE;

	case GST_MESSAGE_STATE_CHANGED:
		break;

	default:
		return TRUE;
	}

	return TRUE;
}

static BurnerBurnResult
burner_normalize_start (BurnerJob *job,
			 GError **error)
{
	BurnerNormalizePrivate *priv;
	BurnerBurnResult result;

	priv = BURNER_NORMALIZE_PRIVATE (job);

	priv->album_gain = -1.0;
	priv->album_peak = -1.0;

	/* get tracks */
	burner_job_get_tracks (job, &priv->tracks);
	if (!priv->tracks)
		return BURNER_BURN_ERR;

	priv->tracks = g_slist_copy (priv->tracks);

	result = burner_normalize_set_next_track (job, error);
	if (result == BURNER_BURN_ERR)
		return BURNER_BURN_ERR;

	if (result == BURNER_BURN_OK)
		return BURNER_BURN_NOT_RUNNING;

	/* ready to go */
	burner_job_set_current_action (job,
					BURNER_BURN_ACTION_ANALYSING,
					_("Normalizing tracks"),
					FALSE);

	return BURNER_BURN_OK;
}

static BurnerBurnResult
burner_normalize_activate (BurnerJob *job,
			    GError **error)
{
	GSList *tracks;
	BurnerJobAction action;

	burner_job_get_action (job, &action);
	if (action != BURNER_JOB_ACTION_IMAGE)
		return BURNER_BURN_NOT_RUNNING;

	/* check we have more than one track */
	burner_job_get_tracks (job, &tracks);
	if (g_slist_length (tracks) < 2)
		return BURNER_BURN_NOT_RUNNING;

	return BURNER_BURN_OK;
}

static BurnerBurnResult
burner_normalize_clock_tick (BurnerJob *job)
{
	gint64 position = 0.0;
	gint64 duration = 0.0;
	BurnerNormalizePrivate *priv;

	priv = BURNER_NORMALIZE_PRIVATE (job);

	gst_element_query_duration (priv->pipeline, GST_FORMAT_TIME, &duration);
	gst_element_query_position (priv->pipeline, GST_FORMAT_TIME, &position);

	if (duration > 0) {
		GSList *tracks;
		gdouble progress;

		burner_job_get_tracks (job, &tracks);
		progress = (gdouble) position / (gdouble) duration;

		if (tracks) {
			gdouble num_tracks;

			num_tracks = g_slist_length (tracks);
			progress = (gdouble) (num_tracks - 1.0 - (gdouble) g_slist_length (priv->tracks) + progress) / (gdouble) num_tracks;
			burner_job_set_progress (job, progress);
		}
	}

	return BURNER_BURN_OK;
}

static void
burner_normalize_init (BurnerNormalize *object)
{}

static void
burner_normalize_finalize (GObject *object)
{
	G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
burner_normalize_class_init (BurnerNormalizeClass *klass)
{
	GObjectClass* object_class = G_OBJECT_CLASS (klass);
	BurnerJobClass *job_class = BURNER_JOB_CLASS (klass);

	g_type_class_add_private (klass, sizeof (BurnerNormalizePrivate));

	object_class->finalize = burner_normalize_finalize;

	job_class->activate = burner_normalize_activate;
	job_class->start = burner_normalize_start;
	job_class->clock_tick = burner_normalize_clock_tick;
	job_class->stop = burner_normalize_stop;
}

static void
burner_normalize_export_caps (BurnerPlugin *plugin)
{
	GSList *input;

	burner_plugin_define (plugin,
	                       "normalize",
			       N_("Normalization"),
			       _("Sets consistent sound levels between tracks"),
			       "Philippe Rouquier",
			       0);

	/* Add dts to make sure that when they are mixed with regular songs
	 * this plugin will be called for the regular tracks */
	input = burner_caps_audio_new (BURNER_PLUGIN_IO_ACCEPT_FILE,
					BURNER_AUDIO_FORMAT_UNDEFINED|
	                                BURNER_AUDIO_FORMAT_DTS|
					BURNER_METADATA_INFO);
	burner_plugin_process_caps (plugin, input);
	g_slist_free (input);

	/* Add dts to make sure that when they are mixed with regular songs
	 * this plugin will be called for the regular tracks */
	input = burner_caps_audio_new (BURNER_PLUGIN_IO_ACCEPT_FILE,
					BURNER_AUDIO_FORMAT_UNDEFINED|
	                                BURNER_AUDIO_FORMAT_DTS);
	burner_plugin_process_caps (plugin, input);
	g_slist_free (input);

	/* We should run first... unfortunately since the gstreamer-1 port
	 * we're unable to process more than a single track with rganalysis
	 * and the GStreamer pipeline becomes stopped indefinitely.
	 * Disable normalisation until this is resolved.
	 * See https://bugzilla.gnome.org/show_bug.cgi?id=699599 */
	burner_plugin_set_process_flags (plugin, BURNER_PLUGIN_RUN_NEVER);

	burner_plugin_set_compulsory (plugin, FALSE);
}

G_MODULE_EXPORT void
burner_plugin_check_config (BurnerPlugin *plugin)
{
	burner_plugin_test_gstreamer_plugin (plugin, "rgvolume");
	burner_plugin_test_gstreamer_plugin (plugin, "rganalysis");
}
