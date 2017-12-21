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
#include "burner-plugin-registration.h"


#define BURNER_TYPE_VOB             (burner_vob_get_type ())
#define BURNER_VOB(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), BURNER_TYPE_VOB, BurnerVob))
#define BURNER_VOB_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), BURNER_TYPE_VOB, BurnerVobClass))
#define BURNER_IS_VOB(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BURNER_TYPE_VOB))
#define BURNER_IS_VOB_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), BURNER_TYPE_VOB))
#define BURNER_VOB_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), BURNER_TYPE_VOB, BurnerVobClass))

BURNER_PLUGIN_BOILERPLATE (BurnerVob, burner_vob, BURNER_TYPE_JOB, BurnerJob);

typedef struct _BurnerVobPrivate BurnerVobPrivate;
struct _BurnerVobPrivate
{
	GstElement *pipeline;

	GstElement *audio;
	GstElement *video;

	GstElement *source;

	BurnerStreamFormat format;

	guint svcd:1;
	guint is_video_dvd:1;
};

#define BURNER_VOB_PRIVATE(o)  (G_TYPE_INSTANCE_GET_PRIVATE ((o), BURNER_TYPE_VOB, BurnerVobPrivate))

static GObjectClass *parent_class = NULL;


/* This is for 3 seconds of buffering (default is 1) */
#define MAX_SIZE_BUFFER		200		/* Use unlimited (0) if it does not work */
#define MAX_SIZE_BYTES		10485760	/* Use unlimited (0) if it does not work */
#define MAX_SIZE_TIME		3000000000LL    /* Use unlimited (0) if it does not work */

static void
burner_vob_stop_pipeline (BurnerVob *vob)
{
	BurnerVobPrivate *priv;

	priv = BURNER_VOB_PRIVATE (vob);
	if (!priv->pipeline)
		return;

	priv->source = NULL;

	gst_element_set_state (priv->pipeline, GST_STATE_NULL);
	gst_object_unref (GST_OBJECT (priv->pipeline));
	priv->pipeline = NULL;
}

static BurnerBurnResult
burner_vob_stop (BurnerJob *job,
		  GError **error)
{
	burner_vob_stop_pipeline (BURNER_VOB (job));
	return BURNER_BURN_OK;
}

static void
burner_vob_finished (BurnerVob *vob)
{
	BurnerTrackType *type = NULL;
	BurnerTrackStream *track;
	gchar *output = NULL;

	type = burner_track_type_new ();
	burner_job_get_output_type (BURNER_JOB (vob), type);
	burner_job_get_audio_output (BURNER_JOB (vob), &output);

	track = burner_track_stream_new ();
	burner_track_stream_set_source (track, output);
	burner_track_stream_set_format (track, burner_track_type_get_stream_format (type));

	burner_job_add_track (BURNER_JOB (vob), BURNER_TRACK (track));
	g_object_unref (track);

	burner_track_type_free (type);
	g_free (output);

	burner_job_finished_track (BURNER_JOB (vob));
}

static gboolean
burner_vob_bus_messages (GstBus *bus,
			  GstMessage *msg,
			  BurnerVob *vob)
{
	GError *error = NULL;
	gchar *debug;

	switch (GST_MESSAGE_TYPE (msg)) {
	case GST_MESSAGE_TAG:
		return TRUE;

	case GST_MESSAGE_ERROR:
		gst_message_parse_error (msg, &error, &debug);
		BURNER_JOB_LOG (vob, debug);
		g_free (debug);

	        burner_job_error (BURNER_JOB (vob), error);
		return FALSE;

	case GST_MESSAGE_EOS:
		BURNER_JOB_LOG (vob, "Transcoding finished");

		/* add a new track and terminate */
		burner_vob_finished (vob);
		return FALSE;

	case GST_MESSAGE_STATE_CHANGED:
		break;

	default:
		return TRUE;
	}

	return TRUE;
}

static void
burner_vob_error_on_pad_linking (BurnerVob *self,
                                  const gchar *function_name)
{
	BurnerVobPrivate *priv;
	GstMessage *message;
	GstBus *bus;

	priv = BURNER_VOB_PRIVATE (self);

	BURNER_JOB_LOG (self, "Error on pad linking");
	message = gst_message_new_error (GST_OBJECT (priv->pipeline),
					 g_error_new (BURNER_BURN_ERROR,
						      BURNER_BURN_ERROR_GENERAL,
						      /* Translators: This message is sent
						       * when burner could not link together
						       * two gstreamer plugins so that one
						       * sends its data to the second for further
						       * processing. This data transmission is
						       * done through a pad. Maybe this is a bit
						       * too technical and should be removed? */
						      _("Impossible to link plugin pads")),
					 function_name);

	bus = gst_pipeline_get_bus (GST_PIPELINE (priv->pipeline));
	gst_bus_post (bus, message);
	g_object_unref (bus);
}

static void
burner_vob_new_decoded_pad_cb (GstElement *decode,
				GstPad *pad,
				BurnerVob *vob)
{
	GstPad *sink;
	GstCaps *caps;
	GstStructure *structure;
	BurnerVobPrivate *priv;

	priv = BURNER_VOB_PRIVATE (vob);

	/* make sure we only have audio */
	caps = gst_pad_query_caps (pad, NULL);
	if (!caps)
		return;

	structure = gst_caps_get_structure (caps, 0);
	if (structure) {
		if (g_strrstr (gst_structure_get_name (structure), "video")) {
			GstPadLinkReturn res;

			sink = gst_element_get_static_pad (priv->video, "sink");
			res = gst_pad_link (pad, sink);
			gst_object_unref (sink);

			if (res != GST_PAD_LINK_OK)
				burner_vob_error_on_pad_linking (vob, "Sent by burner_vob_new_decoded_pad_cb");

			gst_element_set_state (priv->video, GST_STATE_PLAYING);
		}

		if (g_strrstr (gst_structure_get_name (structure), "audio")) {
			GstPadLinkReturn res;

			sink = gst_element_get_static_pad (priv->audio, "sink");
			res = gst_pad_link (pad, sink);
			gst_object_unref (sink);

			if (res != GST_PAD_LINK_OK)
				burner_vob_error_on_pad_linking (vob, "Sent by burner_vob_new_decoded_pad_cb");

			gst_element_set_state (priv->audio, GST_STATE_PLAYING);
		}
	}

	gst_caps_unref (caps);
}

static gboolean
burner_vob_link_audio (BurnerVob *vob,
			GstElement *start,
			GstElement *end,
			GstElement *tee,
			GstElement *muxer)
{
	GstPad *srcpad;
	GstPad *sinkpad;
	GstPadLinkReturn res;

	srcpad = gst_element_get_request_pad (tee, "src_%u");
	sinkpad = gst_element_get_static_pad (start, "sink");
	res = gst_pad_link (srcpad, sinkpad);
	gst_object_unref (sinkpad);
	gst_object_unref (srcpad);

	BURNER_JOB_LOG (vob, "Linked audio bin to tee == %d", res);
	if (res != GST_PAD_LINK_OK)
		return FALSE;

	sinkpad = gst_element_get_request_pad (muxer, "audio_%u");
	srcpad = gst_element_get_static_pad (end, "src");
	res = gst_pad_link (srcpad, sinkpad);
	gst_object_unref (sinkpad);
	gst_object_unref (srcpad);

	BURNER_JOB_LOG (vob, "Linked audio bin to muxer == %d", res);
	if (res != GST_PAD_LINK_OK)
		return FALSE;

	return TRUE;
}

static gboolean
burner_vob_build_audio_pcm (BurnerVob *vob,
			     GstElement *tee,
			     GstElement *muxer,
			     GError **error)
{
	GstElement *queue;
	GstElement *queue1;
	GstElement *filter;
	GstElement *convert;
	GstCaps *filtercaps;
	GstElement *resample;
	BurnerVobPrivate *priv;

	priv = BURNER_VOB_PRIVATE (vob);

	/* NOTE: this can only be used for Video DVDs as PCM cannot be used for
	 * (S)VCDs. */

	/* queue */
	queue = gst_element_factory_make ("queue", NULL);
	if (queue == NULL) {
		g_set_error (error,
			     BURNER_BURN_ERROR,
			     BURNER_BURN_ERROR_GENERAL,
			     /* Translators: %s is the name of the object (as in
			      * GObject) from the Gstreamer library that could
			      * not be created */
			     _("%s element could not be created"),
			     "\"Queue\"");
		goto error;
	}
	gst_bin_add (GST_BIN (priv->pipeline), queue);
	g_object_set (queue,
		      "max-size-buffers", MAX_SIZE_BUFFER,
		      "max-size-bytes", MAX_SIZE_BYTES,
		      "max-size-time", MAX_SIZE_TIME,
		      NULL);

	/* audioresample */
	resample = gst_element_factory_make ("audioresample", NULL);
	if (resample == NULL) {
		g_set_error (error,
			     BURNER_BURN_ERROR,
			     BURNER_BURN_ERROR_GENERAL,
			     /* Translators: %s is the name of the object (as in
			      * GObject) from the Gstreamer library that could
			      * not be created */
			     _("%s element could not be created"),
			     "\"Audioresample\"");
		goto error;
	}
	gst_bin_add (GST_BIN (priv->pipeline), resample);

	/* audioconvert */
	convert = gst_element_factory_make ("audioconvert", NULL);
	if (convert == NULL) {
		g_set_error (error,
			     BURNER_BURN_ERROR,
			     BURNER_BURN_ERROR_GENERAL,
			     /* Translators: %s is the name of the object (as in
			      * GObject) from the Gstreamer library that could
			      * not be created */
			     _("%s element could not be created"),
			     "\"Audioconvert\"");
		goto error;
	}
	gst_bin_add (GST_BIN (priv->pipeline), convert);

	/* another queue */
	queue1 = gst_element_factory_make ("queue", NULL);
	if (queue1 == NULL) {
		g_set_error (error,
			     BURNER_BURN_ERROR,
			     BURNER_BURN_ERROR_GENERAL,
			     _("%s element could not be created"),
			     "\"Queue1\"");
		goto error;
	}
	gst_bin_add (GST_BIN (priv->pipeline), queue1);
	g_object_set (queue1,
		      "max-size-buffers", MAX_SIZE_BUFFER,
		      "max-size-bytes", MAX_SIZE_BYTES,
		      "max-size-time", MAX_SIZE_TIME,
		      NULL);

	/* create a filter */
	filter = gst_element_factory_make ("capsfilter", NULL);
	if (filter == NULL) {
		g_set_error (error,
			     BURNER_BURN_ERROR,
			     BURNER_BURN_ERROR_GENERAL,
			     _("%s element could not be created"),
			     "\"Filter\"");
		goto error;
	}
	gst_bin_add (GST_BIN (priv->pipeline), filter);

	/* NOTE: what about the number of channels (up to 6) ? */
	filtercaps = gst_caps_new_simple ("audio/x-raw",
					  "format", G_TYPE_STRING, "S16BE",
					  "rate", G_TYPE_INT, 48000,
					  NULL);

	g_object_set (GST_OBJECT (filter), "caps", filtercaps, NULL);
	gst_caps_unref (filtercaps);

	if (!gst_element_link_many (queue, resample, convert, filter, queue1, NULL)) {
		BURNER_JOB_LOG (vob, "Error while linking pads");
		g_set_error (error,
			     BURNER_BURN_ERROR,
			     BURNER_BURN_ERROR_GENERAL,
		             _("Impossible to link plugin pads"));
		goto error;
	}

	burner_vob_link_audio (vob, queue, queue1, tee, muxer);

	return TRUE;

error:

	return FALSE;
}

static gboolean
burner_vob_build_audio_mp2 (BurnerVob *vob,
			     GstElement *tee,
			     GstElement *muxer,
			     GError **error)
{
	GstElement *queue;
	GstElement *queue1;
	GstElement *encode;
	GstElement *filter;
	GstElement *convert;
	GstCaps *filtercaps;
	GstElement *resample;
	BurnerVobPrivate *priv;

	priv = BURNER_VOB_PRIVATE (vob);

	/* queue */
	queue = gst_element_factory_make ("queue", NULL);
	if (queue == NULL) {
		g_set_error (error,
			     BURNER_BURN_ERROR,
			     BURNER_BURN_ERROR_GENERAL,
			     _("%s element could not be created"),
			     "\"Queue\"");
		goto error;
	}
	gst_bin_add (GST_BIN (priv->pipeline), queue);
	g_object_set (queue,
		      "max-size-buffers", MAX_SIZE_BUFFER,
		      "max-size-bytes", MAX_SIZE_BYTES,
		      "max-size-time", MAX_SIZE_TIME,
		      NULL);

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
	gst_bin_add (GST_BIN (priv->pipeline), convert);

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
	gst_bin_add (GST_BIN (priv->pipeline), resample);

	/* encoder */
	encode = gst_element_factory_make ("avenc_mp2", NULL);
	if (encode == NULL) {
		g_set_error (error,
			     BURNER_BURN_ERROR,
			     BURNER_BURN_ERROR_GENERAL,
			     _("%s element could not be created"),
			     "\"avenc_mp2\"");
		goto error;
	}
	gst_bin_add (GST_BIN (priv->pipeline), encode);

	/* another queue */
	queue1 = gst_element_factory_make ("queue", NULL);
	if (queue1 == NULL) {
		g_set_error (error,
			     BURNER_BURN_ERROR,
			     BURNER_BURN_ERROR_GENERAL,
			     _("%s element could not be created"),
			     "\"Queue1\"");
		goto error;
	}
	gst_bin_add (GST_BIN (priv->pipeline), queue1);
	g_object_set (queue1,
		      "max-size-buffers", MAX_SIZE_BUFFER,
		      "max-size-bytes", MAX_SIZE_BYTES,
		      "max-size-time", MAX_SIZE_TIME,
		      NULL);
	
	/* create a filter */
	filter = gst_element_factory_make ("capsfilter", NULL);
	if (filter == NULL) {
		g_set_error (error,
			     BURNER_BURN_ERROR,
			     BURNER_BURN_ERROR_GENERAL,
			     _("%s element could not be created"),
			     "\"Filter\"");
		goto error;
	}
	gst_bin_add (GST_BIN (priv->pipeline), filter);

	if (priv->is_video_dvd) {
		BURNER_JOB_LOG (vob, "Setting mp2 bitrate to 448000, 48000 khz");
		g_object_set (encode,
			      "bitrate", 448000, /* it could go up to 912k */
			      NULL);

		/* NOTE: what about the number of channels (up to 7.1) ? */
		filtercaps = gst_caps_new_simple ("audio/x-raw", "rate", G_TYPE_INT, 48000, NULL);
	}
	else if (!priv->svcd) {
		/* VCD */
		BURNER_JOB_LOG (vob, "Setting mp2 bitrate to 224000, 44100 khz");
		g_object_set (encode,
			      "bitrate", 224000,
			      NULL);

		/* 2 channels tops */
		filtercaps = gst_caps_new_simple ("audio/x-raw",
						  "channels", G_TYPE_INT, 2,
						  "rate", G_TYPE_INT, 44100,
						  NULL);
	}
	else {
		/* SVCDs */
		BURNER_JOB_LOG (vob, "Setting mp2 bitrate to 384000, 44100 khz");
		g_object_set (encode,
			      "bitrate", 384000,
			      NULL);

		/* NOTE: channels up to 5.1 or dual */
		filtercaps = gst_caps_new_simple ("audio/x-raw", "rate", G_TYPE_INT, 44100, NULL);
	}

	g_object_set (GST_OBJECT (filter), "caps", filtercaps, NULL);
	gst_caps_unref (filtercaps);

	if (!gst_element_link_many (queue, convert, resample, filter, encode, queue1, NULL)) {
		BURNER_JOB_LOG (vob, "Error while linking pads");
		g_set_error (error,
			     BURNER_BURN_ERROR,
			     BURNER_BURN_ERROR_GENERAL,
		             _("Impossible to link plugin pads"));
		goto error;
	}

	burner_vob_link_audio (vob, queue, queue1, tee, muxer);
	return TRUE;

error:

	return FALSE;
}

static gboolean
burner_vob_build_audio_ac3 (BurnerVob *vob,
			     GstElement *tee,
			     GstElement *muxer,
			     GError **error)
{
	GstElement *queue;
	GstElement *queue1;
	GstElement *filter;
	GstElement *encode;
	GstElement *convert;
	GstCaps *filtercaps;
	GstElement *resample;
	BurnerVobPrivate *priv;

	priv = BURNER_VOB_PRIVATE (vob);

	/* NOTE: This has to be a DVD since AC3 is not supported by (S)VCD */

	/* queue */
	queue = gst_element_factory_make ("queue", NULL);
	if (queue == NULL) {
		g_set_error (error,
			     BURNER_BURN_ERROR,
			     BURNER_BURN_ERROR_GENERAL,
			     _("%s element could not be created"),
			     "\"Queue\"");
		goto error;
	}
	gst_bin_add (GST_BIN (priv->pipeline), queue);;
	g_object_set (queue,
		      "max-size-buffers", MAX_SIZE_BUFFER,
		      "max-size-bytes", MAX_SIZE_BYTES,
		      "max-size-time", MAX_SIZE_TIME,
		      NULL);

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
	gst_bin_add (GST_BIN (priv->pipeline), convert);

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
	gst_bin_add (GST_BIN (priv->pipeline), resample);

	/* create a filter */
	filter = gst_element_factory_make ("capsfilter", NULL);
	if (filter == NULL) {
		g_set_error (error,
			     BURNER_BURN_ERROR,
			     BURNER_BURN_ERROR_GENERAL,
			     _("%s element could not be created"),
			     "\"Filter\"");
		goto error;
	}
	gst_bin_add (GST_BIN (priv->pipeline), filter);

	BURNER_JOB_LOG (vob, "Setting AC3 rate to 48000");
	/* NOTE: we may want to limit the number of channels. */
	filtercaps = gst_caps_new_simple ("audio/x-raw", "rate", G_TYPE_INT, 48000, NULL);

	g_object_set (GST_OBJECT (filter), "caps", filtercaps, NULL);
	gst_caps_unref (filtercaps);

	/* encoder */
	encode = gst_element_factory_make ("avenc_ac3", NULL);
	if (encode == NULL) {
		g_set_error (error,
			     BURNER_BURN_ERROR,
			     BURNER_BURN_ERROR_GENERAL,
			     _("%s element could not be created"),
			     "\"avenc_ac3\"");
		goto error;
	}
	gst_bin_add (GST_BIN (priv->pipeline), encode);

	BURNER_JOB_LOG (vob, "Setting AC3 bitrate to 448000");
	g_object_set (encode,
		      "bitrate", 448000, /* Maximum allowed, is it useful ? */
		      NULL);

	/* another queue */
	queue1 = gst_element_factory_make ("queue", NULL);
	if (queue1 == NULL) {
		g_set_error (error,
			     BURNER_BURN_ERROR,
			     BURNER_BURN_ERROR_GENERAL,
			     _("%s element could not be created"),
			     "\"Queue1\"");
		goto error;
	}
	gst_bin_add (GST_BIN (priv->pipeline), queue1);
	g_object_set (queue1,
		      "max-size-buffers", MAX_SIZE_BUFFER,
		      "max-size-bytes", MAX_SIZE_BYTES,
		      "max-size-time", MAX_SIZE_TIME,
		      NULL);

	if (!gst_element_link_many (queue, convert, resample, filter, encode, queue1, NULL)) {
		BURNER_JOB_LOG (vob, "Error while linking pads");
		g_set_error (error,
			     BURNER_BURN_ERROR,
			     BURNER_BURN_ERROR_GENERAL,
		             _("Impossible to link plugin pads"));
		goto error;
	}

	burner_vob_link_audio (vob, queue, queue1, tee, muxer);

	return TRUE;

error:

	return FALSE;
}

static GstElement *
burner_vob_build_audio_bins (BurnerVob *vob,
			      GstElement *muxer,
			      GError **error)
{
	GValue *value;
	GstElement *tee;
	BurnerVobPrivate *priv;

	priv = BURNER_VOB_PRIVATE (vob);

	/* tee */
	tee = gst_element_factory_make ("tee", NULL);
	if (tee == NULL) {
		g_set_error (error,
			     BURNER_BURN_ERROR,
			     BURNER_BURN_ERROR_GENERAL,
			     _("%s element could not be created"),
			     "\"Tee\"");
		goto error;
	}
	gst_bin_add (GST_BIN (priv->pipeline), tee);

	if (priv->is_video_dvd) {
		/* Get output format */
		value = NULL;
		burner_job_tag_lookup (BURNER_JOB (vob),
					BURNER_DVD_STREAM_FORMAT,
					&value);

		if (value)
			priv->format = g_value_get_int (value);

		if (priv->format == BURNER_AUDIO_FORMAT_NONE)
			priv->format = BURNER_AUDIO_FORMAT_RAW;

		/* FIXME: for the moment even if we use tee, we can't actually
		 * encode and use more than one audio stream */
		if (priv->format & BURNER_AUDIO_FORMAT_RAW) {
			/* PCM : on demand */
			BURNER_JOB_LOG (vob, "Adding PCM audio stream");
			if (!burner_vob_build_audio_pcm (vob, tee, muxer, error))
				goto error;
		}

		if (priv->format & BURNER_AUDIO_FORMAT_AC3) {
			/* AC3 : on demand */
			BURNER_JOB_LOG (vob, "Adding AC3 audio stream");
			if (!burner_vob_build_audio_ac3 (vob, tee, muxer, error))
				goto error;
		}

		if (priv->format & BURNER_AUDIO_FORMAT_MP2) {
			/* MP2 : on demand */
			BURNER_JOB_LOG (vob, "Adding MP2 audio stream");
			if (!burner_vob_build_audio_mp2 (vob, tee, muxer, error))
				goto error;
		}
	}
	else if (!burner_vob_build_audio_mp2 (vob, tee, muxer, error))
		goto error;

	return tee;

error:
	return NULL;
}

static GstElement *
burner_vob_build_video_bin (BurnerVob *vob,
			     GstElement *muxer,
			     GError **error)
{
	GValue *value;
	GstPad *srcpad;
	GstPad *sinkpad;
	GstElement *scale;
	GstElement *queue;
	GstElement *queue1;
	GstElement *filter;
	GstElement *encode;
	GstPadLinkReturn res;
	GstElement *framerate;
	GstElement *colorspace;
	BurnerVobPrivate *priv;
	BurnerBurnResult result;

	priv = BURNER_VOB_PRIVATE (vob);

	queue = gst_element_factory_make ("queue", NULL);
	if (queue == NULL) {
		g_set_error (error,
			     BURNER_BURN_ERROR,
			     BURNER_BURN_ERROR_GENERAL,
			     _("%s element could not be created"),
			     "\"Queue\"");
		goto error;
	}
	gst_bin_add (GST_BIN (priv->pipeline), queue);
	g_object_set (queue,
		      "max-size-buffers", MAX_SIZE_BUFFER,
		      "max-size-bytes", MAX_SIZE_BYTES,
		      "max-size-time", MAX_SIZE_TIME,
		      NULL);

	/* framerate and video type control */
	framerate = gst_element_factory_make ("videorate", NULL);
	if (framerate == NULL) {
		g_set_error (error,
			     BURNER_BURN_ERROR,
			     BURNER_BURN_ERROR_GENERAL,
			     _("%s element could not be created"),
			     "\"Framerate\"");
		goto error;
	}
	gst_bin_add (GST_BIN (priv->pipeline), framerate);
	g_object_set (framerate,
		      "silent", TRUE,
		      NULL);

	/* size scaling */
	scale = gst_element_factory_make ("videoscale", NULL);
	if (scale == NULL) {
		g_set_error (error,
			     BURNER_BURN_ERROR,
			     BURNER_BURN_ERROR_GENERAL,
			     _("%s element could not be created"),
			     "\"Videoscale\"");
		goto error;
	}
	gst_bin_add (GST_BIN (priv->pipeline), scale);

	/* create a filter */
	filter = gst_element_factory_make ("capsfilter", NULL);
	if (filter == NULL) {
		g_set_error (error,
			     BURNER_BURN_ERROR,
			     BURNER_BURN_ERROR_GENERAL,
			     _("%s element could not be created"),
			     "\"Filter\"");
		goto error;
	}
	gst_bin_add (GST_BIN (priv->pipeline), filter);

	colorspace = gst_element_factory_make ("videoconvert", NULL);
	if (colorspace == NULL) {
		g_set_error (error,
			     BURNER_BURN_ERROR,
			     BURNER_BURN_ERROR_GENERAL,
			     _("%s element could not be created"),
			     "\"videoconvert\"");
		goto error;
	}
	gst_bin_add (GST_BIN (priv->pipeline), colorspace);

	encode = gst_element_factory_make ("mpeg2enc", NULL);
	if (encode == NULL) {
		g_set_error (error,
			     BURNER_BURN_ERROR,
			     BURNER_BURN_ERROR_GENERAL,
			     _("%s element could not be created"),
			     "\"Mpeg2enc\"");
		goto error;
	}
	gst_bin_add (GST_BIN (priv->pipeline), encode);

	if (priv->is_video_dvd)
		g_object_set (encode,
			      "format", 8,
			      NULL);
	else if (priv->svcd) {
		/* Option to improve compatibility with vcdimager */
		g_object_set (encode,
			      "dummy-svcd-sof", TRUE,
			      NULL);
		g_object_set (encode,
			      "format", 4,
			      NULL);
	}
	else
		g_object_set (encode,
			      "format", 1,
			      NULL);

	/* settings */
	value = NULL;
	result = burner_job_tag_lookup (BURNER_JOB (vob),
					 BURNER_VIDEO_OUTPUT_FRAMERATE,
					 &value);

	if (result == BURNER_BURN_OK && value) {
		gint rate;
		GstCaps *filtercaps = NULL;

		rate = g_value_get_int (value);

		if (rate == BURNER_VIDEO_FRAMERATE_NTSC) {
			g_object_set (encode,
				      "norm", 110,
				      "framerate", 4,
				      NULL);

			if (priv->is_video_dvd)
				filtercaps = gst_caps_new_full (gst_structure_new ("video/x-raw-yuv",
										   "framerate", GST_TYPE_FRACTION, 30000, 1001,
										   "width", G_TYPE_INT, 720,
										   "height", G_TYPE_INT, 480,
										   NULL),
								gst_structure_new ("video/x-raw-rgb",
										   "framerate", GST_TYPE_FRACTION, 30000, 1001,
										   "width", G_TYPE_INT, 720,
										   "height", G_TYPE_INT, 480,
										   NULL),
								NULL);
			else if (priv->svcd)
				filtercaps = gst_caps_new_full (gst_structure_new ("video/x-raw-yuv",
										   "framerate", GST_TYPE_FRACTION, 30000, 1001,
										   "width", G_TYPE_INT, 480,
										   "height", G_TYPE_INT, 480,
										   NULL),
								gst_structure_new ("video/x-raw-rgb",
										   "framerate", GST_TYPE_FRACTION, 30000, 1001,
										   "width", G_TYPE_INT, 480,
										   "height", G_TYPE_INT, 480,
										   NULL),
								NULL);
			else
				filtercaps = gst_caps_new_full (gst_structure_new ("video/x-raw-yuv",
										   "framerate", GST_TYPE_FRACTION, 30000, 1001,
										   "width", G_TYPE_INT, 352,
										   "height", G_TYPE_INT, 240,
										   NULL),
								gst_structure_new ("video/x-raw-rgb",
										   "framerate", GST_TYPE_FRACTION, 30000, 1001,
										   "width", G_TYPE_INT, 352,
										   "height", G_TYPE_INT, 240,
										   NULL),
								NULL);
		}
		else if (rate == BURNER_VIDEO_FRAMERATE_PAL_SECAM) {
			g_object_set (encode,
				      "norm", 112,
				      "framerate", 3,
				      NULL);

			if (priv->is_video_dvd)
				filtercaps = gst_caps_new_full (gst_structure_new ("video/x-raw-yuv",
										   "framerate", GST_TYPE_FRACTION, 25, 1,
										   "width", G_TYPE_INT, 720,
										   "height", G_TYPE_INT, 576,
										   NULL),
								gst_structure_new ("video/x-raw-rgb",
										   "framerate", GST_TYPE_FRACTION, 25, 1,
										   "width", G_TYPE_INT, 720,
										   "height", G_TYPE_INT, 576,
										   NULL),
								NULL);
			else if (priv->svcd)
				filtercaps = gst_caps_new_full (gst_structure_new ("video/x-raw-yuv",
										   "framerate", GST_TYPE_FRACTION, 25, 1,
										   "width", G_TYPE_INT, 480,
										   "height", G_TYPE_INT, 576,
										   NULL),
								gst_structure_new ("video/x-raw-rgb",
										   "framerate", GST_TYPE_FRACTION, 25, 1,
										   "width", G_TYPE_INT, 480,
										   "height", G_TYPE_INT, 576,
										   NULL),
								NULL);
			else
				filtercaps = gst_caps_new_full (gst_structure_new ("video/x-raw-yuv",
										   "framerate", GST_TYPE_FRACTION, 25, 1,
										   "width", G_TYPE_INT, 352,
										   "height", G_TYPE_INT, 288,
										   NULL),
								gst_structure_new ("video/x-raw-rgb",
										   "framerate", GST_TYPE_FRACTION, 25, 1,
										   "width", G_TYPE_INT, 352,
										   "height", G_TYPE_INT, 288,
										   NULL),
								NULL);
		}

		if (filtercaps) {
			g_object_set (GST_OBJECT (filter), "caps", filtercaps, NULL);
			gst_caps_unref (filtercaps);
		}
	}

	if (priv->is_video_dvd || priv->svcd) {
		value = NULL;
		result = burner_job_tag_lookup (BURNER_JOB (vob),
						 BURNER_VIDEO_OUTPUT_ASPECT,
						 &value);
		if (result == BURNER_BURN_OK && value) {
			gint aspect;

			aspect = g_value_get_int (value);
			if (aspect == BURNER_VIDEO_ASPECT_4_3) {
				BURNER_JOB_LOG (vob, "Setting ratio 4:3");
				g_object_set (encode,
					      "aspect", 2,
					      NULL);
			}
			else if (aspect == BURNER_VIDEO_ASPECT_16_9) {
				BURNER_JOB_LOG (vob, "Setting ratio 16:9");
				g_object_set (encode,
					      "aspect", 3,
					      NULL);	
			}
		}
		else if (priv->svcd) {
			/* This format only supports 4:3 or 16:9. Enforce one of
			 * the two.
			 * FIXME: there should be a way to choose the closest.*/
			BURNER_JOB_LOG (vob, "Setting ratio 4:3");
					 g_object_set (encode,
						       "aspect", 2,
						       NULL);
		}
	}
	else {
		/* VCDs only support 4:3 */
		BURNER_JOB_LOG (vob, "Setting ratio 4:3");
		g_object_set (encode,
			      "aspect", 2,
			      NULL);
	}

	/* another queue */
	queue1 = gst_element_factory_make ("queue", NULL);
	if (queue1 == NULL) {
		g_set_error (error,
			     BURNER_BURN_ERROR,
			     BURNER_BURN_ERROR_GENERAL,
			     _("%s element could not be created"),
			     "\"Queue1\"");
		goto error;
	}
	gst_bin_add (GST_BIN (priv->pipeline), queue1);
	g_object_set (queue1,
		      "max-size-buffers", MAX_SIZE_BUFFER,
		      "max-size-bytes", MAX_SIZE_BYTES,
		      "max-size-time", MAX_SIZE_TIME,
		      NULL);

	if (!gst_element_link_many (queue, framerate, scale, colorspace, filter, encode, queue1, NULL)) {
		BURNER_JOB_LOG (vob, "Error while linking pads");
		g_set_error (error,
			     BURNER_BURN_ERROR,
			     BURNER_BURN_ERROR_GENERAL,
		             _("Impossible to link plugin pads"));
		goto error;
	}

	srcpad = gst_element_get_static_pad (queue1, "src");
	sinkpad = gst_element_get_request_pad (muxer, "video_%u");
	res = gst_pad_link (srcpad, sinkpad);
	BURNER_JOB_LOG (vob, "Linked video bin to muxer == %d", res)
	gst_object_unref (sinkpad);
	gst_object_unref (srcpad);

	return queue;

error:

	return NULL;
}

static gboolean
burner_vob_build_pipeline (BurnerVob *vob,
			    GError **error)
{
	gchar *uri;
	GstBus *bus;
	gchar *output;
	GstElement *sink;
	GstElement *muxer;
	GstElement *source;
	GstElement *decode;
	BurnerTrack *track;
	GstElement *pipeline;
	BurnerVobPrivate *priv;

	priv = BURNER_VOB_PRIVATE (vob);

	BURNER_JOB_LOG (vob, "Creating new pipeline");

	pipeline = gst_pipeline_new (NULL);
	priv->pipeline = pipeline;

	/* source */
	burner_job_get_current_track (BURNER_JOB (vob), &track);
	uri = burner_track_stream_get_source (BURNER_TRACK_STREAM (track), TRUE);
	source = gst_element_make_from_uri (GST_URI_SRC, uri, NULL, NULL);
	if (!source) {
		g_set_error (error,
			     BURNER_BURN_ERROR,
			     BURNER_BURN_ERROR_GENERAL,
			     _("%s element could not be created"),
			     "\"Source\"");
		goto error;
	}
	gst_bin_add (GST_BIN (pipeline), source);
	g_object_set (source,
		      "typefind", FALSE,
		      NULL);

	priv->source = source;

	/* decode */
	decode = gst_element_factory_make ("decodebin", NULL);
	if (!decode) {
		g_set_error (error,
			     BURNER_BURN_ERROR,
			     BURNER_BURN_ERROR_GENERAL,
			     _("%s element could not be created"),
			     "\"Decodebin\"");
		goto error;
	}
	gst_bin_add (GST_BIN (pipeline), decode);

	if (!gst_element_link (source, decode)) {
		BURNER_JOB_LOG (vob, "Error while linking pads");
		g_set_error (error,
			     BURNER_BURN_ERROR,
			     BURNER_BURN_ERROR_GENERAL,
		             _("Impossible to link plugin pads"));
		goto error;
	}

	/* muxer: "mplex" */
	muxer = gst_element_factory_make ("mplex", NULL);
	if (muxer == NULL) {
		g_set_error (error,
			     BURNER_BURN_ERROR,
			     BURNER_BURN_ERROR_GENERAL,
			     _("%s element could not be created"),
			     "\"Mplex\"");
		goto error;
	}
	gst_bin_add (GST_BIN (pipeline), muxer);

	if (priv->is_video_dvd)
		g_object_set (muxer,
			      "format", 8,
			      NULL);
	else if (priv->svcd)
		g_object_set (muxer,
			      "format", 4,
			      NULL);
	else	/* This should be 1 but it causes frame drops */
		g_object_set (muxer,
			      "format", 4,
			      NULL);

	/* create sink */
	output = NULL;
	burner_job_get_audio_output (BURNER_JOB (vob), &output);
	sink = gst_element_factory_make ("filesink", NULL);
	if (sink == NULL) {
		g_set_error (error,
			     BURNER_BURN_ERROR,
			     BURNER_BURN_ERROR_GENERAL,
			     _("%s element could not be created"),
			     "\"Sink\"");
		goto error;
	}
	g_object_set (sink,
		      "location", output,
		      NULL);

	gst_bin_add (GST_BIN (pipeline), sink);
	if (!gst_element_link (muxer, sink)) {
		BURNER_JOB_LOG (vob, "Error while linking pads");
		g_set_error (error,
			     BURNER_BURN_ERROR,
			     BURNER_BURN_ERROR_GENERAL,
		             _("Impossible to link plugin pads"));
		goto error;
	}

	/* video encoding */
	priv->video = burner_vob_build_video_bin (vob, muxer, error);
	if (!priv->video)
		goto error;

	/* audio encoding */
	priv->audio = burner_vob_build_audio_bins (vob, muxer, error);
	if (!priv->audio)
		goto error;

	/* to be able to link everything */
	g_signal_connect (G_OBJECT (decode),
			  "pad-added",
			  G_CALLBACK (burner_vob_new_decoded_pad_cb),
			  vob);

	/* connect to the bus */	
	bus = gst_pipeline_get_bus (GST_PIPELINE (pipeline));
	gst_bus_add_watch (bus,
			   (GstBusFunc) burner_vob_bus_messages,
			   vob);
	gst_object_unref (bus);

	return TRUE;

error:

	if (error && (*error))
		BURNER_JOB_LOG (vob,
				 "can't create object : %s \n",
				 (*error)->message);

	gst_object_unref (GST_OBJECT (pipeline));
	priv->pipeline = NULL;
	return FALSE;
}

static BurnerBurnResult
burner_vob_start (BurnerJob *job,
		   GError **error)
{
	BurnerVobPrivate *priv;
	BurnerJobAction action;
	BurnerTrackType *output = NULL;

	burner_job_get_action (job, &action);
	if (action != BURNER_JOB_ACTION_IMAGE)
		return BURNER_BURN_NOT_SUPPORTED;

	priv = BURNER_VOB_PRIVATE (job);

	/* get destination medium type */
	output = burner_track_type_new ();
	burner_job_get_output_type (job, output);

	if (burner_track_type_get_stream_format (output) & BURNER_VIDEO_FORMAT_VCD) {
		GValue *value = NULL;

		priv->is_video_dvd = FALSE;
		burner_job_tag_lookup (job,
					BURNER_VCD_TYPE,
					&value);
		if (value)
			priv->svcd = (g_value_get_int (value) == BURNER_SVCD);
	}
	else
		priv->is_video_dvd = TRUE;

	BURNER_JOB_LOG (job,
			 "Got output type (is DVD %i, is SVCD %i)",
			 priv->is_video_dvd,
			 priv->svcd);

	burner_track_type_free (output);

	if (!burner_vob_build_pipeline (BURNER_VOB (job), error))
		return BURNER_BURN_ERR;

	/* ready to go */
	burner_job_set_current_action (job,
					BURNER_BURN_ACTION_ANALYSING,
					_("Converting video file to MPEG2"),
					FALSE);
	burner_job_start_progress (job, FALSE);

	gst_element_set_state (priv->pipeline, GST_STATE_PLAYING);

	return BURNER_BURN_OK;
}

static gdouble
burner_vob_get_progress_from_element (BurnerJob *job,
				       GstElement *element)
{
	gint64 position = 0;
	gint64 duration = 0;

	gst_element_query_duration (element, GST_FORMAT_TIME, &duration);
	gst_element_query_position (element, GST_FORMAT_TIME, &position);

	/* FIXME: could also try GST_FORMAT_PERCENT position query */
	if (duration <= 0 || position < 0) {
		duration = 0;
		position = 0;
		gst_element_query_duration (element, GST_FORMAT_BYTES, &duration);
		gst_element_query_position (element, GST_FORMAT_BYTES, &position);
	}

	if (duration > 0 && position >= 0) {
		gdouble progress;

		progress = (gdouble) position / (gdouble) duration;
		burner_job_set_progress (job, progress);
		return TRUE;
	}

	return FALSE;
}

static BurnerBurnResult
burner_vob_clock_tick (BurnerJob *job)
{

	BurnerVobPrivate *priv;

	priv = BURNER_VOB_PRIVATE (job);

	if (burner_vob_get_progress_from_element (job, priv->pipeline))
		return BURNER_BURN_OK;

	BURNER_JOB_LOG (job, "Pipeline failed to report position");

	if (burner_vob_get_progress_from_element (job, priv->source))
		return BURNER_BURN_OK;

	BURNER_JOB_LOG (job, "Source failed to report position");

	return BURNER_BURN_OK;
}

static void
burner_vob_init (BurnerVob *object)
{}

static void
burner_vob_finalize (GObject *object)
{
	BurnerVobPrivate *priv;

	priv = BURNER_VOB_PRIVATE (object);

	if (priv->pipeline) {
		gst_object_unref (priv->pipeline);
		priv->pipeline = NULL;
	}

	G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
burner_vob_class_init (BurnerVobClass *klass)
{
	GObjectClass* object_class = G_OBJECT_CLASS (klass);
	BurnerJobClass* job_class = BURNER_JOB_CLASS (klass);

	g_type_class_add_private (klass, sizeof (BurnerVobPrivate));

	object_class->finalize = burner_vob_finalize;

	job_class->start = burner_vob_start;
	job_class->clock_tick = burner_vob_clock_tick;
	job_class->stop = burner_vob_stop;
}

static void
burner_vob_export_caps (BurnerPlugin *plugin)
{
	GSList *input;
	GSList *output;

	burner_plugin_define (plugin,
			       "transcode2vob",
	                       NULL,
			       _("Converts any video file into a format suitable for video DVDs"),
			       "Philippe Rouquier",
			       0);

	input = burner_caps_audio_new (BURNER_PLUGIN_IO_ACCEPT_FILE,
					BURNER_AUDIO_FORMAT_UNDEFINED|
					BURNER_VIDEO_FORMAT_UNDEFINED|
					BURNER_METADATA_INFO);
	output = burner_caps_audio_new (BURNER_PLUGIN_IO_ACCEPT_FILE,
					 BURNER_AUDIO_FORMAT_MP2|
					 BURNER_METADATA_INFO|
					 BURNER_VIDEO_FORMAT_VCD);
	burner_plugin_link_caps (plugin, output, input);
	g_slist_free (output);

	output = burner_caps_audio_new (BURNER_PLUGIN_IO_ACCEPT_FILE,
					 BURNER_AUDIO_FORMAT_AC3|
					 BURNER_AUDIO_FORMAT_MP2|
					 BURNER_AUDIO_FORMAT_RAW|
					 BURNER_METADATA_INFO|
					 BURNER_VIDEO_FORMAT_VIDEO_DVD);
	burner_plugin_link_caps (plugin, output, input);
	g_slist_free (output);
	g_slist_free (input);

	input = burner_caps_audio_new (BURNER_PLUGIN_IO_ACCEPT_FILE,
					BURNER_AUDIO_FORMAT_UNDEFINED|
					BURNER_VIDEO_FORMAT_UNDEFINED);
	output = burner_caps_audio_new (BURNER_PLUGIN_IO_ACCEPT_FILE,
					 BURNER_AUDIO_FORMAT_MP2|
					 BURNER_VIDEO_FORMAT_VCD);
	burner_plugin_link_caps (plugin, output, input);
	g_slist_free (output);

	output = burner_caps_audio_new (BURNER_PLUGIN_IO_ACCEPT_FILE,
					 BURNER_AUDIO_FORMAT_AC3|
					 BURNER_AUDIO_FORMAT_MP2|
					 BURNER_AUDIO_FORMAT_RAW|
					 BURNER_VIDEO_FORMAT_VIDEO_DVD);
	burner_plugin_link_caps (plugin, output, input);
	g_slist_free (output);
	g_slist_free (input);
}

G_MODULE_EXPORT void
burner_plugin_check_config (BurnerPlugin *plugin)
{
	/* Let's see if we've got the plugins we need */
	burner_plugin_test_gstreamer_plugin (plugin, "avenc_mpeg2video");
	burner_plugin_test_gstreamer_plugin (plugin, "avenc_ac3");
	burner_plugin_test_gstreamer_plugin (plugin, "avenc_mp2");
	burner_plugin_test_gstreamer_plugin (plugin, "mplex");
}
