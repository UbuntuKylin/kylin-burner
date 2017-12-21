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
#include <math.h>
#include <unistd.h>
#include <fcntl.h>

#include <glib.h>
#include <glib/gi18n-lib.h>
#include <glib/gstdio.h>
#include <gmodule.h>

#include <gst/gst.h>

#include "burn-basics.h"
#include "burner-medium.h"
#include "burner-tags.h"
#include "burn-job.h"
#include "burner-plugin-registration.h"
#include "burn-normalize.h"


#define BURNER_TYPE_TRANSCODE         (burner_transcode_get_type ())
#define BURNER_TRANSCODE(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), BURNER_TYPE_TRANSCODE, BurnerTranscode))
#define BURNER_TRANSCODE_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k), BURNER_TYPE_TRANSCODE, BurnerTranscodeClass))
#define BURNER_IS_TRANSCODE(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), BURNER_TYPE_TRANSCODE))
#define BURNER_IS_TRANSCODE_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), BURNER_TYPE_TRANSCODE))
#define BURNER_TRANSCODE_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), BURNER_TYPE_TRANSCODE, BurnerTranscodeClass))

BURNER_PLUGIN_BOILERPLATE (BurnerTranscode, burner_transcode, BURNER_TYPE_JOB, BurnerJob);

static gboolean burner_transcode_bus_messages (GstBus *bus,
						GstMessage *msg,
						BurnerTranscode *transcode);
static void burner_transcode_new_decoded_pad_cb (GstElement *decode,
						  GstPad *pad,
						  BurnerTranscode *transcode);

struct BurnerTranscodePrivate {
	GstElement *pipeline;
	GstElement *convert;
	GstElement *source;
	GstElement *decode;
	GstElement *sink;

	/* element to link decode to */
	GstElement *link;

	gint pad_size;
	gint pad_fd;
	gint pad_id;

	gint64 size;
	gint64 pos;

	gulong probe;
	gint64 segment_start;
	gint64 segment_end;

	guint set_active_state:1;
	guint mp3_size_pipeline:1;
};
typedef struct BurnerTranscodePrivate BurnerTranscodePrivate;

#define BURNER_TRANSCODE_PRIVATE(o)  (G_TYPE_INSTANCE_GET_PRIVATE ((o), BURNER_TYPE_TRANSCODE, BurnerTranscodePrivate))

static GObjectClass *parent_class = NULL;

/* FIXME: this entire function looks completely wrong, if there is or
 * was a bug in GStreamer it should be fixed there (tpm) */
static GstPadProbeReturn
burner_transcode_buffer_handler (GstPad *pad,
                                  GstPadProbeInfo *info,
                                  gpointer user_data)
{
	BurnerTranscodePrivate *priv;
	BurnerTranscode *self = user_data;
	GstBuffer *buffer = GST_PAD_PROBE_INFO_BUFFER (info);
	GstPad *peer;
	gint64 size;

	priv = BURNER_TRANSCODE_PRIVATE (self);

	size = gst_buffer_get_size (buffer);

	if (priv->segment_start <= 0 && priv->segment_end <= 0)
		return GST_PAD_PROBE_OK;

	/* what we do here is more or less what gstreamer does when seeking:
	 * it reads and process from 0 to the seek position (I tried).
	 * It even forwards the data before the seek position to the sink (which
	 * is a problem in our case as it would be written) */
	if (priv->size > priv->segment_end) {
		priv->size += size;
		return GST_PAD_PROBE_DROP;
	}

	if (priv->size + size > priv->segment_end) {
		GstBuffer *new_buffer;
		int data_size;

		/* the entire the buffer is not interesting for us */
		/* create a new buffer and push it on the pad:
		 * NOTE: we're going to receive it ... */
		data_size = priv->segment_end - priv->size;
		new_buffer = gst_buffer_copy_region (buffer, GST_BUFFER_COPY_METADATA, 0, data_size);

		/* FIXME: we can now modify the probe buffer in 0.11 */
		/* Recursive: the following calls ourselves BEFORE we finish */
		peer = gst_pad_get_peer (pad);
		gst_pad_push (peer, new_buffer);

		priv->size += size - data_size;

		/* post an EOS event to stop pipeline */
		gst_pad_push_event (peer, gst_event_new_eos ());
		gst_object_unref (peer);
		return GST_PAD_PROBE_DROP;
	}

	/* see if the buffer is in the segment */
	if (priv->size < priv->segment_start) {
		GstBuffer *new_buffer;
		gint data_size;

		/* see if all the buffer is interesting for us */
		if (priv->size + size < priv->segment_start) {
			priv->size += size;
			return GST_PAD_PROBE_DROP;
		}

		/* create a new buffer and push it on the pad:
		 * NOTE: we're going to receive it ... */
		data_size = priv->size + size - priv->segment_start;
		new_buffer = gst_buffer_copy_region (buffer, GST_BUFFER_COPY_METADATA, size - data_size, data_size);
		/* FIXME: this looks dodgy (tpm) */
		GST_BUFFER_TIMESTAMP (new_buffer) = GST_BUFFER_TIMESTAMP (buffer) + data_size;

		/* move forward by the size of bytes we dropped */
		priv->size += size - data_size;

		/* FIXME: we can now modify the probe buffer in 0.11 */
		/* this is recursive the following calls ourselves 
		 * BEFORE we finish */
		peer = gst_pad_get_peer (pad);
		gst_pad_push (peer, new_buffer);
		gst_object_unref (peer);

		return GST_PAD_PROBE_DROP;
	}

	priv->size += size;
	priv->pos += size;

	return GST_PAD_PROBE_OK;
}

static BurnerBurnResult
burner_transcode_set_boundaries (BurnerTranscode *transcode)
{
	BurnerTranscodePrivate *priv;
	BurnerTrack *track;
	gint64 start;
	gint64 end;

	priv = BURNER_TRANSCODE_PRIVATE (transcode);

	/* we need to reach the song start and set a possible end; this is only
	 * needed when it is decoding a song. Otherwise*/
	burner_job_get_current_track (BURNER_JOB (transcode), &track);
	start = burner_track_stream_get_start (BURNER_TRACK_STREAM (track));
	end = burner_track_stream_get_end (BURNER_TRACK_STREAM (track));

	priv->segment_start = BURNER_DURATION_TO_BYTES (start);
	priv->segment_end = BURNER_DURATION_TO_BYTES (end);

	BURNER_JOB_LOG (transcode, "settings track boundaries time = %lli %lli / bytes = %lli %lli",
			 start, end,
			 priv->segment_start, priv->segment_end);

	return BURNER_BURN_OK;
}

static void
burner_transcode_send_volume_event (BurnerTranscode *transcode)
{
	BurnerTranscodePrivate *priv;
	gdouble track_peak = 0.0;
	gdouble track_gain = 0.0;
	GstTagList *tag_list;
	BurnerTrack *track;
	GstEvent *event;
	GValue *value;

	priv = BURNER_TRANSCODE_PRIVATE (transcode);

	burner_job_get_current_track (BURNER_JOB (transcode), &track);

	BURNER_JOB_LOG (transcode, "Sending audio levels tags");
	if (burner_track_tag_lookup (track, BURNER_TRACK_PEAK_VALUE, &value) == BURNER_BURN_OK)
		track_peak = g_value_get_double (value);

	if (burner_track_tag_lookup (track, BURNER_TRACK_GAIN_VALUE, &value) == BURNER_BURN_OK)
		track_gain = g_value_get_double (value);

	/* it's possible we fail */
	tag_list = gst_tag_list_new (GST_TAG_TRACK_GAIN, track_gain,
				     GST_TAG_TRACK_PEAK, track_peak,
				     NULL);

	/* NOTE: that event is goind downstream */
	event = gst_event_new_tag (tag_list);
	if (!gst_element_send_event (priv->convert, event))
		BURNER_JOB_LOG (transcode, "Couldn't send tags to rgvolume");

	BURNER_JOB_LOG (transcode, "Set volume level %lf %lf", track_gain, track_peak);
}

static GstElement *
burner_transcode_create_volume (BurnerTranscode *transcode,
				 BurnerTrack *track)
{
	GstElement *volume = NULL;

	/* see if we need a volume object */
	if (burner_track_tag_lookup (track, BURNER_TRACK_PEAK_VALUE, NULL) == BURNER_BURN_OK
	||  burner_track_tag_lookup (track, BURNER_TRACK_GAIN_VALUE, NULL) == BURNER_BURN_OK) {
		BURNER_JOB_LOG (transcode, "Found audio levels tags");
		volume = gst_element_factory_make ("rgvolume", NULL);
		if (volume)
			g_object_set (volume,
				      "album-mode", FALSE,
				      NULL);
		else
			BURNER_JOB_LOG (transcode, "rgvolume object couldn't be created");
	}

	return volume;
}

static gboolean
burner_transcode_create_pipeline_size_mp3 (BurnerTranscode *transcode,
					    GstElement *pipeline,
					    GstElement *source,
                                            GError **error)
{
	BurnerTranscodePrivate *priv;
	GstElement *parse;
	GstElement *sink;

	priv = BURNER_TRANSCODE_PRIVATE (transcode);

	BURNER_JOB_LOG (transcode, "Creating specific pipeline for MP3s");

	parse = gst_element_factory_make ("mpegaudioparse", NULL);
	if (!parse) {
		g_set_error (error,
			     BURNER_BURN_ERROR,
			     BURNER_BURN_ERROR_GENERAL,
			     _("%s element could not be created"),
			     "\"mpegaudioparse\"");
		g_object_unref (pipeline);
		return FALSE;
	}
	gst_bin_add (GST_BIN (pipeline), parse);

	sink = gst_element_factory_make ("fakesink", NULL);
	if (!sink) {
		g_set_error (error,
			     BURNER_BURN_ERROR,
			     BURNER_BURN_ERROR_GENERAL,
			     _("%s element could not be created"),
			     "\"Fakesink\"");
		g_object_unref (pipeline);
		return FALSE;
	}
	gst_bin_add (GST_BIN (pipeline), sink);

	/* Link */
	if (!gst_element_link_many (source, parse, sink, NULL)) {
		g_set_error (error,
			     BURNER_BURN_ERROR,
			     BURNER_BURN_ERROR_GENERAL,
			     _("Impossible to link plugin pads"));
		BURNER_JOB_LOG (transcode, "Impossible to link elements");
		g_object_unref (pipeline);
		return FALSE;
	}

	priv->convert = NULL;

	priv->sink = sink;
	priv->source = source;
	priv->pipeline = pipeline;

	/* Get it going */
	gst_element_set_state (priv->pipeline, GST_STATE_PLAYING);
	return TRUE;
}

static void
burner_transcode_error_on_pad_linking (BurnerTranscode *self,
                                        const gchar *function_name)
{
	BurnerTranscodePrivate *priv;
	GstMessage *message;
	GstBus *bus;

	priv = BURNER_TRANSCODE_PRIVATE (self);

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

static gboolean
burner_transcode_create_pipeline (BurnerTranscode *transcode,
				   GError **error)
{
	gchar *uri;
	gboolean keep_dts;
	GstElement *decode;
	GstElement *source;
	GstBus *bus = NULL;
	GstCaps *filtercaps;
	GValue *value = NULL;
	GstElement *pipeline;
	GstElement *sink = NULL;
	BurnerJobAction action;
	GstElement *filter = NULL;
	GstElement *volume = NULL;
	GstElement *convert = NULL;
	BurnerTrack *track = NULL;
	GstElement *resample = NULL;
	BurnerTranscodePrivate *priv;

	priv = BURNER_TRANSCODE_PRIVATE (transcode);

	BURNER_JOB_LOG (transcode, "Creating new pipeline");

	priv->set_active_state = 0;

	/* free the possible current pipeline and create a new one */
	if (priv->pipeline) {
		gst_element_set_state (priv->pipeline, GST_STATE_NULL);
		gst_object_unref (G_OBJECT (priv->pipeline));
		priv->link = NULL;
		priv->sink = NULL;
		priv->source = NULL;
		priv->convert = NULL;
		priv->pipeline = NULL;
	}

	/* create three types of pipeline according to the needs: (possibly adding grvolume)
	 * - filesrc ! decodebin ! audioconvert ! fakesink (find size) and filesrc!mp3parse!fakesink for mp3s
	 * - filesrc ! decodebin ! audioresample ! audioconvert ! audio/x-raw,format=S16BE,rate=44100 ! filesink
	 * - filesrc ! decodebin ! audioresample ! audioconvert ! audio/x-raw,format=S16BE,rate=44100 ! fdsink
	 */
	pipeline = gst_pipeline_new (NULL);

	bus = gst_pipeline_get_bus (GST_PIPELINE (pipeline));
	gst_bus_add_watch (bus,
			   (GstBusFunc) burner_transcode_bus_messages,
			   transcode);
	gst_object_unref (bus);

	/* source */
	burner_job_get_current_track (BURNER_JOB (transcode), &track);
	uri = burner_track_stream_get_source (BURNER_TRACK_STREAM (track), TRUE);
	source = gst_element_make_from_uri (GST_URI_SRC, uri, NULL, NULL);
	g_free (uri);

	if (source == NULL) {
		g_set_error (error,
			     BURNER_BURN_ERROR,
			     BURNER_BURN_ERROR_GENERAL,
			     /* Translators: %s is the name of the object (as in
			      * GObject) from the Gstreamer library that could
			      * not be created */
			     _("%s element could not be created"),
			     "\"Source\"");
		goto error;
	}
	gst_bin_add (GST_BIN (pipeline), source);
	g_object_set (source,
		      "typefind", FALSE,
		      NULL);

	/* sink */
	burner_job_get_action (BURNER_JOB (transcode), &action);
	switch (action) {
	case BURNER_JOB_ACTION_SIZE:
		if (priv->mp3_size_pipeline)
			return burner_transcode_create_pipeline_size_mp3 (transcode, pipeline, source, error);

		sink = gst_element_factory_make ("fakesink", NULL);
		break;

	case BURNER_JOB_ACTION_IMAGE:
		volume = burner_transcode_create_volume (transcode, track);

		if (burner_job_get_fd_out (BURNER_JOB (transcode), NULL) != BURNER_BURN_OK) {
			gchar *output;

			burner_job_get_image_output (BURNER_JOB (transcode),
						      &output,
						      NULL);
			sink = gst_element_factory_make ("filesink", NULL);
			g_object_set (sink,
				      "location", output,
				      NULL);
			g_free (output);
		}
		else {
			int fd;

			burner_job_get_fd_out (BURNER_JOB (transcode), &fd);
			sink = gst_element_factory_make ("fdsink", NULL);
			g_object_set (sink,
				      "fd", fd,
				      NULL);
		}
		break;

	default:
		goto error;
	}

	if (!sink) {
		g_set_error (error,
			     BURNER_BURN_ERROR,
			     BURNER_BURN_ERROR_GENERAL,
			     _("%s element could not be created"),
			     "\"Sink\"");
		goto error;
	}

	gst_bin_add (GST_BIN (pipeline), sink);
	g_object_set (sink,
		      "sync", FALSE,
		      NULL);

	burner_job_tag_lookup (BURNER_JOB (transcode),
				BURNER_SESSION_STREAM_AUDIO_FORMAT,
				&value);
	if (value)
		keep_dts = (g_value_get_int (value) & BURNER_AUDIO_FORMAT_DTS) != 0;
	else
		keep_dts = FALSE;

	if (keep_dts
	&&  action == BURNER_JOB_ACTION_IMAGE
	&& (burner_track_stream_get_format (BURNER_TRACK_STREAM (track)) & BURNER_AUDIO_FORMAT_DTS) != 0) {
		GstElement *wavparse;
		GstPad *sinkpad;

		BURNER_JOB_LOG (transcode, "DTS wav pipeline");

		/* FIXME: volume normalization won't work here. We'd need to 
		 * reencode it afterwards otherwise. */
		/* This is a special case. This is DTS wav. So we only decode wav. */
		wavparse = gst_element_factory_make ("wavparse", NULL);
		if (wavparse == NULL) {
			g_set_error (error,
				     BURNER_BURN_ERROR,
				     BURNER_BURN_ERROR_GENERAL,
				     _("%s element could not be created"),
				     "\"Wavparse\"");
			goto error;
		}
		gst_bin_add (GST_BIN (pipeline), wavparse);

		if (!gst_element_link_many (source, wavparse, sink, NULL)) {
			g_set_error (error,
				     BURNER_BURN_ERROR,
				     BURNER_BURN_ERROR_GENERAL,
			             _("Impossible to link plugin pads"));
			goto error;
		}

		/* This is an ugly workaround for the lack of accuracy with
		 * gstreamer. Yet this is unfortunately a necessary evil. */
		/* FIXME: this does not look like it makes sense... (tpm) */
		priv->pos = 0;
		priv->size = 0;
		sinkpad = gst_element_get_static_pad (sink, "sink");
		priv->probe = gst_pad_add_probe (sinkpad, GST_PAD_PROBE_TYPE_BUFFER,
		                                 burner_transcode_buffer_handler,
		                                 transcode, NULL);
		gst_object_unref (sinkpad);


		priv->link = NULL;
		priv->sink = sink;
		priv->decode = NULL;
		priv->source = source;
		priv->convert = NULL;
		priv->pipeline = pipeline;

		gst_element_set_state (pipeline, GST_STATE_PLAYING);
		return TRUE;
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

	if (action == BURNER_JOB_ACTION_IMAGE) {
		BurnerStreamFormat session_format;
		BurnerTrackType *output_type;

		output_type = burner_track_type_new ();
		burner_job_get_output_type (BURNER_JOB (transcode), output_type);
		session_format = burner_track_type_get_stream_format (output_type);
		burner_track_type_free (output_type);

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

		/* filter */
		filter = gst_element_factory_make ("capsfilter", NULL);
		if (!filter) {
			g_set_error (error,
				     BURNER_BURN_ERROR,
				     BURNER_BURN_ERROR_GENERAL,
				     _("%s element could not be created"),
				     "\"Filter\"");
			goto error;
		}
		gst_bin_add (GST_BIN (pipeline), filter);
		filtercaps = gst_caps_new_full (gst_structure_new ("audio/x-raw",
								   /* NOTE: we use little endianness only for libburn which requires little */
								   "format", G_TYPE_STRING, (session_format & BURNER_AUDIO_FORMAT_RAW_LITTLE_ENDIAN) != 0 ? "S16LE" : "S16BE",
								   "channels", G_TYPE_INT, 2,
								   "rate", G_TYPE_INT, 44100,
								   NULL),
						NULL);
		g_object_set (GST_OBJECT (filter), "caps", filtercaps, NULL);
		gst_caps_unref (filtercaps);
	}

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

	if (action == BURNER_JOB_ACTION_IMAGE) {
		GstPad *sinkpad;
		gboolean res;

		if (!gst_element_link (source, decode)) {
			BURNER_JOB_LOG (transcode, "Impossible to link plugin pads");
			g_set_error (error,
				     BURNER_BURN_ERROR,
				     BURNER_BURN_ERROR_GENERAL,
			             _("Impossible to link plugin pads"));
			goto error;
		}

		priv->link = resample;
		g_signal_connect (G_OBJECT (decode),
				  "pad-added",
				  G_CALLBACK (burner_transcode_new_decoded_pad_cb),
				  transcode);

		if (volume) {
			gst_bin_add (GST_BIN (pipeline), volume);
			res = gst_element_link_many (resample,
			                             volume,
						     convert,
			                             filter,
			                             sink,
			                             NULL);
		}
		else
			res = gst_element_link_many (resample,
			                             convert,
			                             filter,
			                             sink,
			                             NULL);

		if (!res) {
			BURNER_JOB_LOG (transcode, "Impossible to link plugin pads");
			g_set_error (error,
				     BURNER_BURN_ERROR,
				     BURNER_BURN_ERROR_GENERAL,
				     _("Impossible to link plugin pads"));
			goto error;
		}

		/* This is an ugly workaround for the lack of accuracy with
		 * gstreamer. Yet this is unfortunately a necessary evil. */
		/* FIXME: this does not look like it makes sense... (tpm) */
		priv->pos = 0;
		priv->size = 0;
		sinkpad = gst_element_get_static_pad (sink, "sink");
		priv->probe = gst_pad_add_probe (sinkpad, GST_PAD_PROBE_TYPE_BUFFER,
		                                 burner_transcode_buffer_handler,
		                                 transcode, NULL);
		gst_object_unref (sinkpad);
	}
	else {
		if (!gst_element_link (source, decode)
		||  !gst_element_link (convert, sink)) {
			BURNER_JOB_LOG (transcode, "Impossible to link plugin pads");
			g_set_error (error,
				     BURNER_BURN_ERROR,
				     BURNER_BURN_ERROR_GENERAL,
				     _("Impossible to link plugin pads"));
			goto error;
		}

		priv->link = convert;
		g_signal_connect (G_OBJECT (decode),
				  "pad-added",
				  G_CALLBACK (burner_transcode_new_decoded_pad_cb),
				  transcode);
	}

	priv->sink = sink;
	priv->decode = decode;
	priv->source = source;
	priv->convert = convert;
	priv->pipeline = pipeline;

	gst_element_set_state (pipeline, GST_STATE_PLAYING);
	return TRUE;

error:

	if (error && (*error))
		BURNER_JOB_LOG (transcode,
				 "can't create object : %s \n",
				 (*error)->message);

	gst_object_unref (GST_OBJECT (pipeline));
	return FALSE;
}

static void
burner_transcode_set_track_size (BurnerTranscode *transcode,
				  gint64 duration)
{
	gchar *uri;
	BurnerTrack *track;

	burner_job_get_current_track (BURNER_JOB (transcode), &track);
	burner_track_stream_set_boundaries (BURNER_TRACK_STREAM (track), -1, duration, -1);
	duration += burner_track_stream_get_gap (BURNER_TRACK_STREAM (track));

	/* if transcoding on the fly we should add some length just to make
	 * sure we won't be too short (gstreamer duration discrepancy) */
	burner_job_set_output_size_for_current_track (BURNER_JOB (transcode),
						       BURNER_DURATION_TO_SECTORS (duration),
						       BURNER_DURATION_TO_BYTES (duration));

	uri = burner_track_stream_get_source (BURNER_TRACK_STREAM (track), FALSE);
	BURNER_JOB_LOG (transcode,
			 "Song %s"
			 "\nsectors %" G_GINT64_FORMAT
			 "\ntime %" G_GINT64_FORMAT, 
			 uri,
			 BURNER_DURATION_TO_SECTORS (duration),
			 duration);
	g_free (uri);
}

/**
 * These functions are to deal with siblings
 */

static BurnerBurnResult
burner_transcode_create_sibling_size (BurnerTranscode *transcode,
				        BurnerTrack *src,
				        GError **error)
{
	BurnerTrack *dest;
	guint64 duration;

	/* it means the same file uri is in the selection and was already
	 * checked. Simply get the values for the length and other information
	 * and copy them. */
	/* NOTE: no need to copy the length since if they are sibling that means
	 * that they have the same length */
	burner_track_stream_get_length (BURNER_TRACK_STREAM (src), &duration);
	burner_job_set_output_size_for_current_track (BURNER_JOB (transcode),
						       BURNER_DURATION_TO_SECTORS (duration),
						       BURNER_DURATION_TO_BYTES (duration));

	/* copy the info we are missing */
	burner_job_get_current_track (BURNER_JOB (transcode), &dest);
	burner_track_tag_copy_missing (dest, src);

	return BURNER_BURN_OK;
}

static BurnerBurnResult
burner_transcode_create_sibling_image (BurnerTranscode *transcode,
					BurnerTrack *src,
					GError **error)
{
	BurnerTrackStream *dest;
	BurnerTrack *track;
	guint64 length = 0;
	gchar *path_dest;
	gchar *path_src;

	/* it means the file is already in the selection. Simply create a 
	 * symlink pointing to first file in the selection with the same uri */
	path_src = burner_track_stream_get_source (BURNER_TRACK_STREAM (src), FALSE);
	burner_job_get_audio_output (BURNER_JOB (transcode), &path_dest);

	if (symlink (path_src, path_dest) == -1) {
                int errsv = errno;

		g_set_error (error,
			     BURNER_BURN_ERROR,
			     BURNER_BURN_ERROR_GENERAL,
			     /* Translators: the %s is the error message from errno */
			     _("An internal error occurred (%s)"),
			     g_strerror (errsv));

		goto error;
	}

	dest = burner_track_stream_new ();
	burner_track_stream_set_source (dest, path_dest);

	/* FIXME: what if input had metadata ?*/
	burner_track_stream_set_format (dest, BURNER_AUDIO_FORMAT_RAW);

	/* NOTE: there is no gap and start = 0 since these tracks are the result
	 * of the transformation of previous ones */
	burner_track_stream_get_length (BURNER_TRACK_STREAM (src), &length);
	burner_track_stream_set_boundaries (dest, 0, length, 0);

	/* copy all infos but from the current track */
	burner_job_get_current_track (BURNER_JOB (transcode), &track);
	burner_track_tag_copy_missing (BURNER_TRACK (dest), track);
	burner_job_add_track (BURNER_JOB (transcode), BURNER_TRACK (dest));

	/* It's good practice to unref the track afterwards as we don't need it
	 * anymore. BurnerTaskCtx refs it. */
	g_object_unref (dest);

	g_free (path_src);
	g_free (path_dest);

	return BURNER_BURN_NOT_RUNNING;

error:
	g_free (path_src);
	g_free (path_dest);

	return BURNER_BURN_ERR;
}

static BurnerTrack *
burner_transcode_search_for_sibling (BurnerTranscode *transcode)
{
	BurnerJobAction action;
	GSList *iter, *songs;
	BurnerTrack *track;
	gint64 start;
	gint64 end;
	gchar *uri;

	burner_job_get_action (BURNER_JOB (transcode), &action);

	burner_job_get_current_track (BURNER_JOB (transcode), &track);
	start = burner_track_stream_get_start (BURNER_TRACK_STREAM (track));
	end = burner_track_stream_get_end (BURNER_TRACK_STREAM (track));
	uri = burner_track_stream_get_source (BURNER_TRACK_STREAM (track), TRUE);

	burner_job_get_done_tracks (BURNER_JOB (transcode), &songs);

	for (iter = songs; iter; iter = iter->next) {
		gchar *iter_uri;
		gint64 iter_end;
		gint64 iter_start;
		BurnerTrack *iter_track;

		iter_track = iter->data;
		iter_uri = burner_track_stream_get_source (BURNER_TRACK_STREAM (iter_track), TRUE);

		if (strcmp (iter_uri, uri))
			continue;

		iter_end = burner_track_stream_get_end (BURNER_TRACK_STREAM (iter_track));
		if (!iter_end)
			continue;

		if (iter_end != end)
			continue;

		iter_start = burner_track_stream_get_start (BURNER_TRACK_STREAM (track));
		if (iter_start == start) {
			g_free (uri);
			return iter_track;
		}
	}

	g_free (uri);
	return NULL;
}

static BurnerBurnResult
burner_transcode_has_track_sibling (BurnerTranscode *transcode,
				     GError **error)
{
	BurnerJobAction action;
	BurnerTrack *sibling = NULL;
	BurnerBurnResult result = BURNER_BURN_OK;

	if (burner_job_get_fd_out (BURNER_JOB (transcode), NULL) == BURNER_BURN_OK)
		return BURNER_BURN_OK;

	sibling = burner_transcode_search_for_sibling (transcode);
	if (!sibling)
		return BURNER_BURN_OK;

	BURNER_JOB_LOG (transcode, "found sibling: skipping");
	burner_job_get_action (BURNER_JOB (transcode), &action);
	if (action == BURNER_JOB_ACTION_IMAGE)
		result = burner_transcode_create_sibling_image (transcode,
								 sibling,
								 error);
	else if (action == BURNER_JOB_ACTION_SIZE)
		result = burner_transcode_create_sibling_size (transcode,
								sibling,
								error);

	return result;
}

static BurnerBurnResult
burner_transcode_start (BurnerJob *job,
			 GError **error)
{
	BurnerTranscode *transcode;
	BurnerBurnResult result;
	BurnerJobAction action;

	transcode = BURNER_TRANSCODE (job);

	burner_job_get_action (job, &action);
	burner_job_set_use_average_rate (job, TRUE);

	if (action == BURNER_JOB_ACTION_SIZE) {
		BurnerTrack *track;

		/* see if the track size was already set since then no need to 
		 * carry on with a lengthy get size and the library will do it
		 * itself. */
		burner_job_get_current_track (job, &track);
		if (burner_track_stream_get_end (BURNER_TRACK_STREAM (track)) > 0)
			return BURNER_BURN_NOT_SUPPORTED;

		if (!burner_transcode_create_pipeline (transcode, error))
			return BURNER_BURN_ERR;

		burner_job_set_current_action (job,
						BURNER_BURN_ACTION_GETTING_SIZE,
						NULL,
						TRUE);

		burner_job_start_progress (job, FALSE);
		return BURNER_BURN_OK;
	}

	if (action == BURNER_JOB_ACTION_IMAGE) {
		/* Look for a sibling to avoid transcoding twice. In this case
		 * though start and end of this track must be inside start and
		 * end of the previous track. Of course if we are piping that
		 * operation is simply impossible. */
		if (burner_job_get_fd_out (job, NULL) != BURNER_BURN_OK) {
			result = burner_transcode_has_track_sibling (BURNER_TRANSCODE (job), error);
			if (result != BURNER_BURN_OK)
				return result;
		}

		burner_transcode_set_boundaries (transcode);
		if (!burner_transcode_create_pipeline (transcode, error))
			return BURNER_BURN_ERR;
	}
	else
		BURNER_JOB_NOT_SUPPORTED (transcode);

	return BURNER_BURN_OK;
}

static void
burner_transcode_stop_pipeline (BurnerTranscode *transcode)
{
	BurnerTranscodePrivate *priv;
	GstPad *sinkpad;

	priv = BURNER_TRANSCODE_PRIVATE (transcode);
	if (!priv->pipeline)
		return;

	sinkpad = gst_element_get_static_pad (priv->sink, "sink");
	if (priv->probe)
		gst_pad_remove_probe (sinkpad, priv->probe);

	gst_object_unref (sinkpad);

	gst_element_set_state (priv->pipeline, GST_STATE_NULL);
	gst_object_unref (GST_OBJECT (priv->pipeline));

	priv->link = NULL;
	priv->sink = NULL;
	priv->source = NULL;
	priv->convert = NULL;
	priv->pipeline = NULL;

	priv->set_active_state = 0;
}

static BurnerBurnResult
burner_transcode_stop (BurnerJob *job,
			GError **error)
{
	BurnerTranscodePrivate *priv;

	priv = BURNER_TRANSCODE_PRIVATE (job);

	priv->mp3_size_pipeline = 0;

	if (priv->pad_id) {
		g_source_remove (priv->pad_id);
		priv->pad_id = 0;
	}

	burner_transcode_stop_pipeline (BURNER_TRANSCODE (job));
	return BURNER_BURN_OK;
}

/* we must make sure that the track size is a multiple
 * of 2352 to be burnt by cdrecord with on the fly */

static gint64
burner_transcode_pad_real (BurnerTranscode *transcode,
			    int fd,
			    gint64 bytes2write,
			    GError **error)
{
	const int buffer_size = 512;
	char buffer [buffer_size];
	gint64 b_written;
	gint64 size;

	b_written = 0;
	bzero (buffer, sizeof (buffer));
	for (; bytes2write; bytes2write -= b_written) {
		size = bytes2write > buffer_size ? buffer_size : bytes2write;
		b_written = write (fd, buffer, (int) size);

		BURNER_JOB_LOG (transcode,
				 "written %" G_GINT64_FORMAT " bytes for padding",
				 b_written);

		/* we should not handle EINTR and EAGAIN as errors */
		if (b_written < 0) {
			if (errno == EINTR || errno == EAGAIN) {
				BURNER_JOB_LOG (transcode, "got EINTR / EAGAIN, retrying");
	
				/* we'll try later again */
				return bytes2write;
			}
		}

		if (size != b_written) {
                        int errsv = errno;

			g_set_error (error,
				     BURNER_BURN_ERROR,
				     BURNER_BURN_ERROR_GENERAL,
				     /* Translators: %s is the string error from errno */
				     _("Error while padding file (%s)"),
				     g_strerror (errsv));
			return -1;
		}
	}

	return 0;
}

static void
burner_transcode_push_track (BurnerTranscode *transcode)
{
	guint64 length = 0;
	gchar *output = NULL;
	BurnerTrack *src = NULL;
	BurnerTrackStream *track;

	burner_job_get_audio_output (BURNER_JOB (transcode), &output);
	burner_job_get_current_track (BURNER_JOB (transcode), &src);

	burner_track_stream_get_length (BURNER_TRACK_STREAM (src), &length);

	track = burner_track_stream_new ();
	burner_track_stream_set_source (track, output);
	g_free (output);

	/* FIXME: what if input had metadata ?*/
	burner_track_stream_set_format (track, BURNER_AUDIO_FORMAT_RAW);
	burner_track_stream_set_boundaries (track, 0, length, 0);
	burner_track_tag_copy_missing (BURNER_TRACK (track), src);

	burner_job_add_track (BURNER_JOB (transcode), BURNER_TRACK (track));
	
	/* It's good practice to unref the track afterwards as we don't need it
	 * anymore. BurnerTaskCtx refs it. */
	g_object_unref (track);

	burner_job_finished_track (BURNER_JOB (transcode));
}

static gboolean
burner_transcode_pad_idle (BurnerTranscode *transcode)
{
	gint64 bytes2write;
	GError *error = NULL;
	BurnerTranscodePrivate *priv;

	priv = BURNER_TRANSCODE_PRIVATE (transcode);
	bytes2write = burner_transcode_pad_real (transcode,
						  priv->pad_fd,
						  priv->pad_size,
						  &error);

	if (bytes2write == -1) {
		priv->pad_id = 0;
		burner_job_error (BURNER_JOB (transcode), error);
		return FALSE;
	}

	if (bytes2write) {
		priv->pad_size = bytes2write;
		return TRUE;
	}

	/* we are finished with padding */
	priv->pad_id = 0;
	close (priv->pad_fd);
	priv->pad_fd = -1;

	/* set the next song or finish */
	burner_transcode_push_track (transcode);
	return FALSE;
}

static gboolean
burner_transcode_pad (BurnerTranscode *transcode, int fd, GError **error)
{
	guint64 length = 0;
	gint64 bytes2write = 0;
	BurnerTrack *track = NULL;
	BurnerTranscodePrivate *priv;

	priv = BURNER_TRANSCODE_PRIVATE (transcode);
	if (priv->pos < 0)
		return TRUE;

	/* Padding is important for two reasons:
	 * - first if didn't output enough bytes compared to what we should have
	 * - second we must output a multiple of 2352 to respect sector
	 *   boundaries */
	burner_job_get_current_track (BURNER_JOB (transcode), &track);
	burner_track_stream_get_length (BURNER_TRACK_STREAM (track), &length);

	if (priv->pos < BURNER_DURATION_TO_BYTES (length)) {
		gint64 b_written = 0;

		/* Check bytes boundary for length */
		b_written = BURNER_DURATION_TO_BYTES (length);
		b_written += (b_written % 2352) ? 2352 - (b_written % 2352):0;
		bytes2write = b_written - priv->pos;

		BURNER_JOB_LOG (transcode,
				 "wrote %lli bytes (= %lli ns) out of %lli (= %lli ns)"
				 "\n=> padding %lli bytes",
				 priv->pos,
				 BURNER_BYTES_TO_DURATION (priv->pos),
				 BURNER_DURATION_TO_BYTES (length),
				 length,
				 bytes2write);
	}
	else {
		gint64 b_written = 0;

		/* wrote more or the exact amount of bytes. Check bytes boundary */
		b_written = priv->pos;
		bytes2write = (b_written % 2352) ? 2352 - (b_written % 2352):0;
		BURNER_JOB_LOG (transcode,
				 "wrote %lli bytes (= %lli ns)"
				 "\n=> padding %lli bytes",
				 b_written,
				 priv->pos,
				 bytes2write);
	}

	if (!bytes2write)
		return TRUE;

	bytes2write = burner_transcode_pad_real (transcode,
						  fd,
						  bytes2write,
						  error);
	if (bytes2write == -1)
		return TRUE;

	if (bytes2write) {
		BurnerTranscodePrivate *priv;

		priv = BURNER_TRANSCODE_PRIVATE (transcode);
		/* when writing to a pipe it can happen that its buffer is full
		 * because cdrecord is not fast enough. Therefore we couldn't
		 * write/pad it and we'll have to wait for the pipe to become
		 * available again */
		priv->pad_fd = fd;
		priv->pad_size = bytes2write;
		priv->pad_id = g_timeout_add (50,
					     (GSourceFunc) burner_transcode_pad_idle,
					      transcode);
		return FALSE;		
	}

	return TRUE;
}

static gboolean
burner_transcode_pad_pipe (BurnerTranscode *transcode, GError **error)
{
	int fd;
	gboolean result;

	burner_job_get_fd_out (BURNER_JOB (transcode), &fd);
	fd = dup (fd);

	result = burner_transcode_pad (transcode, fd, error);
	if (result)
		close (fd);

	return result;
}

static gboolean
burner_transcode_pad_file (BurnerTranscode *transcode, GError **error)
{
	int fd;
	gchar *output;
	gboolean result;

	output = NULL;
	burner_job_get_audio_output (BURNER_JOB (transcode), &output);
	fd = open (output, O_WRONLY | O_CREAT | O_APPEND, S_IRWXU | S_IRGRP | S_IROTH);
	g_free (output);

	if (fd == -1) {
                int errsv = errno;

		g_set_error (error,
			     BURNER_BURN_ERROR,
			     BURNER_BURN_ERROR_GENERAL,
			     /* Translators: %s is the string error from errno */
			     _("Error while padding file (%s)"),
			     g_strerror (errsv));
		return FALSE;
	}

	result = burner_transcode_pad (transcode, fd, error);
	if (result)
		close (fd);

	return result;
}

static gboolean
burner_transcode_is_mp3 (BurnerTranscode *transcode)
{
	BurnerTranscodePrivate *priv;
	GstElement *typefind;
	GstCaps *caps = NULL;
	const gchar *mime;

	priv = BURNER_TRANSCODE_PRIVATE (transcode);

	/* find the type of the file */
	typefind = gst_bin_get_by_name (GST_BIN (priv->decode), "typefind");
	g_object_get (typefind, "caps", &caps, NULL);
	if (!caps) {
		gst_object_unref (typefind);
		return TRUE;
	}

	if (caps && gst_caps_get_size (caps) > 0) {
		mime = gst_structure_get_name (gst_caps_get_structure (caps, 0));
		gst_object_unref (typefind);

		if (mime && !strcmp (mime, "application/x-id3"))
			return TRUE;

		if (!strcmp (mime, "audio/mpeg"))
			return TRUE;
	}
	else
		gst_object_unref (typefind);

	return FALSE;
}

static gint64
burner_transcode_get_duration (BurnerTranscode *transcode)
{
	gint64 duration = -1;
	BurnerTranscodePrivate *priv;

	priv = BURNER_TRANSCODE_PRIVATE (transcode);

	/* This part is specific to MP3s */
	if (priv->mp3_size_pipeline) {
		/* This is the most reliable way to get the duration for mp3
		 * read them till the end and get the position. */
		gst_element_query_position (priv->pipeline,
					    GST_FORMAT_TIME,
					    &duration);
	}

	/* This is for any sort of files */
	if (duration == -1 || duration == 0)
		gst_element_query_duration (priv->pipeline,
					    GST_FORMAT_TIME,
					    &duration);

	BURNER_JOB_LOG (transcode, "got duration %" GST_TIME_FORMAT, GST_TIME_ARGS (duration));

	if (duration == -1 || duration == 0)	
	    burner_job_error (BURNER_JOB (transcode),
			       g_error_new (BURNER_BURN_ERROR,
					    BURNER_BURN_ERROR_GENERAL, "%s",
					    _("Error while getting duration")));
	return duration;
}

static gboolean
burner_transcode_song_end_reached (BurnerTranscode *transcode)
{
	GError *error = NULL;
	BurnerJobAction action;

	burner_job_get_action (BURNER_JOB (transcode), &action);
	if (action == BURNER_JOB_ACTION_SIZE) {
		gint64 duration;

		/* this is when we need to write infs:
		 * - when asked to create infs
		 * - when decoding to a file */
		duration = burner_transcode_get_duration (transcode);
		if (duration == -1)
			return FALSE;

		burner_transcode_set_track_size (transcode, duration);
		burner_job_finished_track (BURNER_JOB (transcode));
		return TRUE;
	}

	if (action == BURNER_JOB_ACTION_IMAGE) {
		gboolean result;

		/* pad file so it is a multiple of 2352 (= 1 sector) */
		if (burner_job_get_fd_out (BURNER_JOB (transcode), NULL) == BURNER_BURN_OK)
			result = burner_transcode_pad_pipe (transcode, &error);
		else
			result = burner_transcode_pad_file (transcode, &error);
	
		if (error) {
			burner_job_error (BURNER_JOB (transcode), error);
			return FALSE;
		}

		if (!result) {
			burner_transcode_stop_pipeline (transcode);
			return FALSE;
		}
	}

	burner_transcode_push_track (transcode);
	return TRUE;
}

static void
foreach_tag (const GstTagList *list,
	     const gchar *tag,
	     BurnerTranscode *transcode)
{
	BurnerTrack *track;
	BurnerJobAction action;

	burner_job_get_action (BURNER_JOB (transcode), &action);
	burner_job_get_current_track (BURNER_JOB (transcode), &track);

	BURNER_JOB_LOG (transcode, "Retrieving tags");

	if (!strcmp (tag, GST_TAG_TITLE)) {
		if (!burner_track_tag_lookup_string (track, BURNER_TRACK_STREAM_TITLE_TAG)) {
			gchar *title = NULL;

			gst_tag_list_get_string (list, tag, &title);
			burner_track_tag_add_string (track,
						      BURNER_TRACK_STREAM_TITLE_TAG,
						      title);
			g_free (title);
		}
	}
	else if (!strcmp (tag, GST_TAG_ARTIST)) {
		if (!burner_track_tag_lookup_string (track, BURNER_TRACK_STREAM_ARTIST_TAG)) {
			gchar *artist = NULL;

			gst_tag_list_get_string (list, tag, &artist);
			burner_track_tag_add_string (track,
						      BURNER_TRACK_STREAM_ARTIST_TAG,
						      artist);
			g_free (artist);
		}
	}
	else if (!strcmp (tag, GST_TAG_ISRC)) {
		if (!burner_track_tag_lookup_string (track, BURNER_TRACK_STREAM_ISRC_TAG)) {
			gchar *isrc = NULL;

			gst_tag_list_get_string (list, tag, &isrc);
			burner_track_tag_add_string (track,
						      BURNER_TRACK_STREAM_ISRC_TAG,
						      isrc);
		}
	}
	else if (!strcmp (tag, GST_TAG_PERFORMER)) {
		if (!burner_track_tag_lookup_string (track, BURNER_TRACK_STREAM_ARTIST_TAG)) {
			gchar *artist = NULL;

			gst_tag_list_get_string (list, tag, &artist);
			burner_track_tag_add_string (track,
						      BURNER_TRACK_STREAM_ARTIST_TAG,
						      artist);
			g_free (artist);
		}
	}
	else if (action == BURNER_JOB_ACTION_SIZE
	     &&  !strcmp (tag, GST_TAG_DURATION)) {
		guint64 duration;

		/* this is only useful when we try to have the size */
		gst_tag_list_get_uint64 (list, tag, &duration);
		burner_track_stream_set_boundaries (BURNER_TRACK_STREAM (track), 0, duration, -1);
	}
}

/* NOTE: the return value is whether or not we should stop the bus callback */
static gboolean
burner_transcode_active_state (BurnerTranscode *transcode)
{
	BurnerTranscodePrivate *priv;
	gchar *name, *string, *uri;
	BurnerJobAction action;
	BurnerTrack *track;

	priv = BURNER_TRANSCODE_PRIVATE (transcode);

	if (priv->set_active_state)
		return TRUE;

	priv->set_active_state = TRUE;

	burner_job_get_current_track (BURNER_JOB (transcode), &track);
	uri = burner_track_stream_get_source (BURNER_TRACK_STREAM (track), FALSE);

	burner_job_get_action (BURNER_JOB (transcode), &action);
	if (action == BURNER_JOB_ACTION_SIZE) {
		BURNER_JOB_LOG (transcode,
				 "Analysing Track %s",
				 uri);

		if (priv->mp3_size_pipeline) {
			gchar *escaped_basename;

			/* Run the pipeline till the end */
			escaped_basename = g_path_get_basename (uri);
			name = g_uri_unescape_string (escaped_basename, NULL);
			g_free (escaped_basename);

			string = g_strdup_printf (_("Analysing \"%s\""), name);
			g_free (name);
		
			burner_job_set_current_action (BURNER_JOB (transcode),
							BURNER_BURN_ACTION_ANALYSING,
							string,
							TRUE);
			g_free (string);

			burner_job_start_progress (BURNER_JOB (transcode), FALSE);
			g_free (uri);
			return TRUE;
		}

		if (burner_transcode_is_mp3 (transcode)) {
			GError *error = NULL;

			/* Rebuild another pipeline which is specific to MP3s. */
			priv->mp3_size_pipeline = TRUE;
			burner_transcode_stop_pipeline (transcode);

			if (!burner_transcode_create_pipeline (transcode, &error))
				burner_job_error (BURNER_JOB (transcode), error);
		}
		else
			burner_transcode_song_end_reached (transcode);

		g_free (uri);
		return FALSE;
	}
	else {
		gchar *escaped_basename;

		escaped_basename = g_path_get_basename (uri);
		name = g_uri_unescape_string (escaped_basename, NULL);
		g_free (escaped_basename);

		string = g_strdup_printf (_("Transcoding \"%s\""), name);
		g_free (name);

		burner_job_set_current_action (BURNER_JOB (transcode),
						BURNER_BURN_ACTION_TRANSCODING,
						string,
						TRUE);
		g_free (string);
		burner_job_start_progress (BURNER_JOB (transcode), FALSE);

		if (burner_job_get_fd_out (BURNER_JOB (transcode), NULL) != BURNER_BURN_OK) {
			gchar *dest = NULL;

			burner_job_get_audio_output (BURNER_JOB (transcode), &dest);
			BURNER_JOB_LOG (transcode,
					 "start decoding %s to %s",
					 uri,
					 dest);
			g_free (dest);
		}
		else
			BURNER_JOB_LOG (transcode,
					 "start piping %s",
					 uri)
	}

	g_free (uri);
	return TRUE;
}

static gboolean
burner_transcode_bus_messages (GstBus *bus,
				GstMessage *msg,
				BurnerTranscode *transcode)
{
	BurnerTranscodePrivate *priv;
	GstTagList *tags = NULL;
	GError *error = NULL;
	GstState state;
	gchar *debug;

	priv = BURNER_TRANSCODE_PRIVATE (transcode);
	switch (GST_MESSAGE_TYPE (msg)) {
	case GST_MESSAGE_TAG:
		/* we use the information to write an .inf file 
		 * for the time being just store the information */
		gst_message_parse_tag (msg, &tags);
		gst_tag_list_foreach (tags, (GstTagForeachFunc) foreach_tag, transcode);
		gst_tag_list_free (tags);
		return TRUE;

	case GST_MESSAGE_ERROR:
		gst_message_parse_error (msg, &error, &debug);
		BURNER_JOB_LOG (transcode, debug);
		g_free (debug);

	        burner_job_error (BURNER_JOB (transcode), error);
		return FALSE;

	case GST_MESSAGE_EOS:
		burner_transcode_song_end_reached (transcode);
		return FALSE;

	case GST_MESSAGE_STATE_CHANGED: {
		GstStateChangeReturn result;

		result = gst_element_get_state (priv->pipeline,
						&state,
						NULL,
						1);

		if (result != GST_STATE_CHANGE_SUCCESS)
			return TRUE;

		if (state == GST_STATE_PLAYING)
			return burner_transcode_active_state (transcode);

		break;
	}

	default:
		return TRUE;
	}

	return TRUE;
}

static void
burner_transcode_new_decoded_pad_cb (GstElement *decode,
				      GstPad *pad,
				      BurnerTranscode *transcode)
{
	GstCaps *caps;
	GstStructure *structure;
	BurnerTranscodePrivate *priv;

	priv = BURNER_TRANSCODE_PRIVATE (transcode);

	BURNER_JOB_LOG (transcode, "New pad");

	/* make sure we only have audio */
	/* FIXME: get_current_caps() doesn't always seem to work yet here */
	caps = gst_pad_query_caps (pad, NULL);
	if (!caps)
		return;

	structure = gst_caps_get_structure (caps, 0);
	if (structure) {
		if (g_strrstr (gst_structure_get_name (structure), "audio")) {
			GstPad *sink;
			GstElement *queue;
			GstPadLinkReturn res;

			/* before linking pads (before any data reach grvolume), send tags */
			burner_transcode_send_volume_event (transcode);

			/* This is necessary in case there is a video stream
			 * (see burner-metadata.c). we need to queue to avoid
			 * a deadlock. */
			queue = gst_element_factory_make ("queue", NULL);
			gst_bin_add (GST_BIN (priv->pipeline), queue);
			if (!gst_element_link (queue, priv->link)) {
				burner_transcode_error_on_pad_linking (transcode, "Sent by burner_transcode_new_decoded_pad_cb");
				goto end;
			}

			sink = gst_element_get_static_pad (queue, "sink");
			if (GST_PAD_IS_LINKED (sink)) {
				burner_transcode_error_on_pad_linking (transcode, "Sent by burner_transcode_new_decoded_pad_cb");
				goto end;
			}

			res = gst_pad_link (pad, sink);
			if (res == GST_PAD_LINK_OK)
				gst_element_set_state (queue, GST_STATE_PLAYING);
			else
				burner_transcode_error_on_pad_linking (transcode, "Sent by burner_transcode_new_decoded_pad_cb");

			gst_object_unref (sink);
		}
		/* For video streams add a fakesink (see burner-metadata.c) */
		else if (g_strrstr (gst_structure_get_name (structure), "video")) {
			GstPad *sink;
			GstElement *fakesink;
			GstPadLinkReturn res;

			BURNER_JOB_LOG (transcode, "Adding a fakesink for video stream");

			fakesink = gst_element_factory_make ("fakesink", NULL);
			if (!fakesink) {
				burner_transcode_error_on_pad_linking (transcode, "Sent by burner_transcode_new_decoded_pad_cb");
				goto end;
			}

			sink = gst_element_get_static_pad (fakesink, "sink");
			if (!sink) {
				burner_transcode_error_on_pad_linking (transcode, "Sent by burner_transcode_new_decoded_pad_cb");
				gst_object_unref (fakesink);
				goto end;
			}

			gst_bin_add (GST_BIN (priv->pipeline), fakesink);
			res = gst_pad_link (pad, sink);

			if (res == GST_PAD_LINK_OK)
				gst_element_set_state (fakesink, GST_STATE_PLAYING);
			else
				burner_transcode_error_on_pad_linking (transcode, "Sent by burner_transcode_new_decoded_pad_cb");

			gst_object_unref (sink);
		}
	}

end:
	gst_caps_unref (caps);
}

static BurnerBurnResult
burner_transcode_clock_tick (BurnerJob *job)
{
	BurnerTranscodePrivate *priv;

	priv = BURNER_TRANSCODE_PRIVATE (job);

	if (!priv->pipeline)
		return BURNER_BURN_ERR;

	burner_job_set_written_track (job, priv->pos);
	return BURNER_BURN_OK;
}

static void
burner_transcode_class_init (BurnerTranscodeClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);
	BurnerJobClass *job_class = BURNER_JOB_CLASS (klass);

	g_type_class_add_private (klass, sizeof (BurnerTranscodePrivate));

	parent_class = g_type_class_peek_parent (klass);
	object_class->finalize = burner_transcode_finalize;

	job_class->start = burner_transcode_start;
	job_class->clock_tick = burner_transcode_clock_tick;
	job_class->stop = burner_transcode_stop;
}

static void
burner_transcode_init (BurnerTranscode *obj)
{ }

static void
burner_transcode_finalize (GObject *object)
{
	BurnerTranscodePrivate *priv;

	priv = BURNER_TRANSCODE_PRIVATE (object);

	if (priv->pad_id) {
		g_source_remove (priv->pad_id);
		priv->pad_id = 0;
	}

	burner_transcode_stop_pipeline (BURNER_TRANSCODE (object));

	G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
burner_transcode_export_caps (BurnerPlugin *plugin)
{
	GSList *input;
	GSList *output;

	burner_plugin_define (plugin,
			       "transcode",
	                       NULL,
			       _("Converts any song file into a format suitable for audio CDs"),
			       "Philippe Rouquier",
			       1);

	output = burner_caps_audio_new (BURNER_PLUGIN_IO_ACCEPT_FILE|
					 BURNER_PLUGIN_IO_ACCEPT_PIPE,
					 BURNER_AUDIO_FORMAT_RAW|
					 BURNER_AUDIO_FORMAT_RAW_LITTLE_ENDIAN|
					 BURNER_METADATA_INFO);

	input = burner_caps_audio_new (BURNER_PLUGIN_IO_ACCEPT_FILE,
					BURNER_AUDIO_FORMAT_UNDEFINED|
					BURNER_METADATA_INFO);

	burner_plugin_link_caps (plugin, output, input);
	g_slist_free (input);

	input = burner_caps_audio_new (BURNER_PLUGIN_IO_ACCEPT_FILE,
					BURNER_AUDIO_FORMAT_DTS|
					BURNER_METADATA_INFO);
	burner_plugin_link_caps (plugin, output, input);
	g_slist_free (output);
	g_slist_free (input);

	output = burner_caps_audio_new (BURNER_PLUGIN_IO_ACCEPT_FILE|
					 BURNER_PLUGIN_IO_ACCEPT_PIPE,
					 BURNER_AUDIO_FORMAT_RAW|
					 BURNER_AUDIO_FORMAT_RAW_LITTLE_ENDIAN);

	input = burner_caps_audio_new (BURNER_PLUGIN_IO_ACCEPT_FILE,
					BURNER_AUDIO_FORMAT_UNDEFINED);

	burner_plugin_link_caps (plugin, output, input);
	g_slist_free (input);

	input = burner_caps_audio_new (BURNER_PLUGIN_IO_ACCEPT_FILE,
					BURNER_AUDIO_FORMAT_DTS);

	burner_plugin_link_caps (plugin, output, input);
	g_slist_free (output);
	g_slist_free (input);
}
