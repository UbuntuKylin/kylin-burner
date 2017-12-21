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
#include <glib/gi18n-lib.h>
#include <glib/gstdio.h>
#include <gmodule.h>

#include <libxml/xmlerror.h>
#include <libxml/xmlwriter.h>
#include <libxml/parser.h>
#include <libxml/xmlstring.h>
#include <libxml/uri.h>

#include "burner-plugin-registration.h"
#include "burn-job.h"
#include "burn-process.h"
#include "burner-track-data.h"
#include "burner-track-stream.h"


#define BURNER_TYPE_DVD_AUTHOR             (burner_dvd_author_get_type ())
#define BURNER_DVD_AUTHOR(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), BURNER_TYPE_DVD_AUTHOR, BurnerDvdAuthor))
#define BURNER_DVD_AUTHOR_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), BURNER_TYPE_DVD_AUTHOR, BurnerDvdAuthorClass))
#define BURNER_IS_DVD_AUTHOR(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BURNER_TYPE_DVD_AUTHOR))
#define BURNER_IS_DVD_AUTHOR_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), BURNER_TYPE_DVD_AUTHOR))
#define BURNER_DVD_AUTHOR_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), BURNER_TYPE_DVD_AUTHOR, BurnerDvdAuthorClass))

BURNER_PLUGIN_BOILERPLATE (BurnerDvdAuthor, burner_dvd_author, BURNER_TYPE_PROCESS, BurnerProcess);

typedef struct _BurnerDvdAuthorPrivate BurnerDvdAuthorPrivate;
struct _BurnerDvdAuthorPrivate
{
	gchar *output;
};

#define BURNER_DVD_AUTHOR_PRIVATE(o)  (G_TYPE_INSTANCE_GET_PRIVATE ((o), BURNER_TYPE_DVD_AUTHOR, BurnerDvdAuthorPrivate))

static BurnerProcessClass *parent_class = NULL;

static BurnerBurnResult
burner_dvd_author_add_track (BurnerJob *job)
{
	gchar *path;
	GSList *grafts = NULL;
	BurnerGraftPt *graft;
	BurnerTrackData *track;
	BurnerDvdAuthorPrivate *priv;

	priv = BURNER_DVD_AUTHOR_PRIVATE (job);

	/* create the track */
	track = burner_track_data_new ();

	/* audio */
	graft = g_new (BurnerGraftPt, 1);
	path = g_build_path (G_DIR_SEPARATOR_S,
			     priv->output,
			     "AUDIO_TS",
			     NULL);
	graft->uri = g_filename_to_uri (path, NULL, NULL);
	g_free (path);

	graft->path = g_strdup ("/AUDIO_TS");
	grafts = g_slist_prepend (grafts, graft);

	BURNER_JOB_LOG (job, "Adding graft point for %s", graft->uri);

	/* video */
	graft = g_new (BurnerGraftPt, 1);
	path = g_build_path (G_DIR_SEPARATOR_S,
			     priv->output,
			     "VIDEO_TS",
			     NULL);
	graft->uri = g_filename_to_uri (path, NULL, NULL);
	g_free (path);

	graft->path = g_strdup ("/VIDEO_TS");
	grafts = g_slist_prepend (grafts, graft);

	BURNER_JOB_LOG (job, "Adding graft point for %s", graft->uri);

	burner_track_data_add_fs (track,
				   BURNER_IMAGE_FS_ISO|
				   BURNER_IMAGE_FS_UDF|
				   BURNER_IMAGE_FS_VIDEO);
	burner_track_data_set_source (track,
				       grafts,
				       NULL);
	burner_job_add_track (job, BURNER_TRACK (track));
	g_object_unref (track);

	return BURNER_BURN_OK;
}

static BurnerBurnResult
burner_dvd_author_read_stdout (BurnerProcess *process,
				const gchar *line)
{
	return BURNER_BURN_OK;
}

static BurnerBurnResult
burner_dvd_author_read_stderr (BurnerProcess *process,
				const gchar *line)
{
	gint percent = 0;

	if (sscanf (line, "STAT: fixing VOBU at %*s (%*d/%*d, %d%%)", &percent) == 1) {
		burner_job_start_progress (BURNER_JOB (process), FALSE);
		burner_job_set_progress (BURNER_JOB (process),
					  (gdouble) ((gdouble) percent) / 100.0);
	}

	return BURNER_BURN_OK;
}

static BurnerBurnResult
burner_dvd_author_generate_xml_file (BurnerProcess *process,
				      const gchar *path,
				      GError **error)
{
	BurnerDvdAuthorPrivate *priv;
	BurnerBurnResult result;
	GSList *tracks = NULL;
	xmlTextWriter *xml;
	gint success;
	GSList *iter;

	BURNER_JOB_LOG (process, "Creating DVD layout xml file(%s)", path);

	xml = xmlNewTextWriterFilename (path, 0);
	if (!xml)
		return BURNER_BURN_ERR;

	priv = BURNER_DVD_AUTHOR_PRIVATE (process);

	xmlTextWriterSetIndent (xml, 1);
	xmlTextWriterSetIndentString (xml, (xmlChar *) "\t");

	success = xmlTextWriterStartDocument (xml,
					      NULL,
					      "UTF8",
					      NULL);
	if (success < 0)
		goto error;

	result = burner_job_get_tmp_dir (BURNER_JOB (process),
					  &priv->output,
					  error);
	if (result != BURNER_BURN_OK)
		return result;

	/* let's start */
	success = xmlTextWriterStartElement (xml, (xmlChar *) "dvdauthor");
	if (success < 0)
		goto error;

	success = xmlTextWriterWriteAttribute (xml,
					       (xmlChar *) "dest",
					       (xmlChar *) priv->output);
	if (success < 0)
		goto error;

	/* This is needed to finalize */
	success = xmlTextWriterWriteElement (xml, (xmlChar *) "vmgm", (xmlChar *) "");
	if (success < 0)
		goto error;

	/* the tracks */
	success = xmlTextWriterStartElement (xml, (xmlChar *) "titleset");
	if (success < 0)
		goto error;

	success = xmlTextWriterStartElement (xml, (xmlChar *) "titles");
	if (success < 0)
		goto error;

	/* get all tracks */
	burner_job_get_tracks (BURNER_JOB (process), &tracks);
	for (iter = tracks; iter; iter = iter->next) {
		BurnerTrack *track;
		gchar *video;

		track = iter->data;
		success = xmlTextWriterStartElement (xml, (xmlChar *) "pgc");
		if (success < 0)
			goto error;

		success = xmlTextWriterStartElement (xml, (xmlChar *) "vob");
		if (success < 0)
			goto error;

		video = burner_track_stream_get_source (BURNER_TRACK_STREAM (track), FALSE);
		success = xmlTextWriterWriteAttribute (xml,
						       (xmlChar *) "file",
						       (xmlChar *) video);
		g_free (video);

		if (success < 0)
			goto error;

		/* vob */
		success = xmlTextWriterEndElement (xml);
		if (success < 0)
			goto error;

		/* pgc */
		success = xmlTextWriterEndElement (xml);
		if (success < 0)
			goto error;
	}

	/* titles */
	success = xmlTextWriterEndElement (xml);
	if (success < 0)
		goto error;

	/* titleset */
	success = xmlTextWriterEndElement (xml);
	if (success < 0)
		goto error;

	/* close dvdauthor */
	success = xmlTextWriterEndElement (xml);
	if (success < 0)
		goto error;

	xmlTextWriterEndDocument (xml);
	xmlFreeTextWriter (xml);

	return BURNER_BURN_OK;

error:

	BURNER_JOB_LOG (process, "Error");

	/* close everything */
	xmlTextWriterEndDocument (xml);
	xmlFreeTextWriter (xml);

	/* FIXME: get the error */

	return BURNER_BURN_ERR;
}

static BurnerBurnResult
burner_dvd_author_set_argv (BurnerProcess *process,
			     GPtrArray *argv,
			     GError **error)
{
	BurnerBurnResult result;
	BurnerJobAction action;
	gchar *output;

	burner_job_get_action (BURNER_JOB (process), &action);
	if (action != BURNER_JOB_ACTION_IMAGE)
		BURNER_JOB_NOT_SUPPORTED (process);

	g_ptr_array_add (argv, g_strdup ("dvdauthor"));
	
	/* get all arguments to write XML file */
	result = burner_job_get_tmp_file (BURNER_JOB (process),
					   NULL,
					   &output,
					   error);
	if (result != BURNER_BURN_OK)
		return result;

	g_ptr_array_add (argv, g_strdup ("-x"));
	g_ptr_array_add (argv, output);

	result = burner_dvd_author_generate_xml_file (process, output, error);
	if (result != BURNER_BURN_OK)
		return result;

	burner_job_set_current_action (BURNER_JOB (process),
					BURNER_BURN_ACTION_CREATING_IMAGE,
					_("Creating file layout"),
					FALSE);
	return BURNER_BURN_OK;
}

static BurnerBurnResult
burner_dvd_author_post (BurnerJob *job)
{
	BurnerDvdAuthorPrivate *priv;

	priv = BURNER_DVD_AUTHOR_PRIVATE (job);

	burner_dvd_author_add_track (job);

	if (priv->output) {
		g_free (priv->output);
		priv->output = NULL;
	}

	return burner_job_finished_session (job);
}

static void
burner_dvd_author_init (BurnerDvdAuthor *object)
{}

static void
burner_dvd_author_finalize (GObject *object)
{
	G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
burner_dvd_author_class_init (BurnerDvdAuthorClass *klass)
{
	GObjectClass* object_class = G_OBJECT_CLASS (klass);
	BurnerProcessClass* process_class = BURNER_PROCESS_CLASS (klass);

	g_type_class_add_private (klass, sizeof (BurnerDvdAuthorPrivate));

	object_class->finalize = burner_dvd_author_finalize;

	process_class->stdout_func = burner_dvd_author_read_stdout;
	process_class->stderr_func = burner_dvd_author_read_stderr;
	process_class->set_argv = burner_dvd_author_set_argv;
	process_class->post = burner_dvd_author_post;
}

static void
burner_dvd_author_export_caps (BurnerPlugin *plugin)
{
	GSList *output;
	GSList *input;

	/* NOTE: it seems that cdrecord can burn cue files on the fly */
	burner_plugin_define (plugin,
			       "dvdauthor",
	                       NULL,
			       _("Creates disc images suitable for video DVDs"),
			       "Philippe Rouquier",
			       1);

	input = burner_caps_audio_new (BURNER_PLUGIN_IO_ACCEPT_FILE,
					BURNER_AUDIO_FORMAT_AC3|
					BURNER_AUDIO_FORMAT_MP2|
					BURNER_AUDIO_FORMAT_RAW|
					BURNER_METADATA_INFO|
					BURNER_VIDEO_FORMAT_VIDEO_DVD);

	output = burner_caps_data_new (BURNER_IMAGE_FS_ISO|
					BURNER_IMAGE_FS_UDF|
					BURNER_IMAGE_FS_VIDEO);

	burner_plugin_link_caps (plugin, output, input);
	g_slist_free (input);

	input = burner_caps_audio_new (BURNER_PLUGIN_IO_ACCEPT_FILE,
					BURNER_AUDIO_FORMAT_AC3|
					BURNER_AUDIO_FORMAT_MP2|
					BURNER_AUDIO_FORMAT_RAW|
					BURNER_VIDEO_FORMAT_VIDEO_DVD);

	burner_plugin_link_caps (plugin, output, input);
	g_slist_free (output);
	g_slist_free (input);

	/* we only support DVDs */
	burner_plugin_set_flags (plugin,
  				  BURNER_MEDIUM_FILE|
				  BURNER_MEDIUM_DVDR|
				  BURNER_MEDIUM_DVDR_PLUS|
				  BURNER_MEDIUM_DUAL_L|
				  BURNER_MEDIUM_BLANK|
				  BURNER_MEDIUM_APPENDABLE|
				  BURNER_MEDIUM_HAS_DATA,
				  BURNER_BURN_FLAG_NONE,
				  BURNER_BURN_FLAG_NONE);

	burner_plugin_set_flags (plugin,
				  BURNER_MEDIUM_DVDRW|
				  BURNER_MEDIUM_DVDRW_PLUS|
				  BURNER_MEDIUM_DVDRW_RESTRICTED|
				  BURNER_MEDIUM_DUAL_L|
				  BURNER_MEDIUM_BLANK|
				  BURNER_MEDIUM_CLOSED|
				  BURNER_MEDIUM_APPENDABLE|
				  BURNER_MEDIUM_HAS_DATA,
				  BURNER_BURN_FLAG_NONE,
				  BURNER_BURN_FLAG_NONE);
}

G_MODULE_EXPORT void
burner_plugin_check_config (BurnerPlugin *plugin)
{
	gint version [3] = { 0, 6, 0};
	burner_plugin_test_app (plugin,
	                         "dvdauthor",
	                         "-h",
	                         "DVDAuthor::dvdauthor, version %d.%d.%d.",
	                         version);
}
