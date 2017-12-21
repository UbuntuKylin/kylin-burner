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

#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <time.h>

#include <glib.h>
#include <glib-object.h>
#include <glib/gi18n-lib.h>
#include <glib/gstdio.h>
#include <gmodule.h>

#include "burner-units.h"

#include "burn-debug.h"
#include "burn-job.h"
#include "burn-process.h"
#include "burner-plugin-registration.h"
#include "burn-cdrtools.h"
#include "burner-track-disc.h"
#include "burner-track-stream.h"

#define BURNER_TYPE_CDDA2WAV         (burner_cdda2wav_get_type ())
#define BURNER_CDDA2WAV(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), BURNER_TYPE_CDDA2WAV, BurnerCdda2wav))
#define BURNER_CDDA2WAV_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k), BURNER_TYPE_CDDA2WAV, BurnerCdda2wavClass))
#define BURNER_IS_CDDA2WAV(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), BURNER_TYPE_CDDA2WAV))
#define BURNER_IS_CDDA2WAV_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), BURNER_TYPE_CDDA2WAV))
#define BURNER_CDDA2WAV_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), BURNER_TYPE_CDDA2WAV, BurnerCdda2wavClass))

BURNER_PLUGIN_BOILERPLATE (BurnerCdda2wav, burner_cdda2wav, BURNER_TYPE_PROCESS, BurnerProcess);

struct _BurnerCdda2wavPrivate {
	gchar *file_pattern;

	guint track_num;
	guint track_nb;

	guint is_inf	:1;
};
typedef struct _BurnerCdda2wavPrivate BurnerCdda2wavPrivate;

#define BURNER_CDDA2WAV_PRIVATE(o)  (G_TYPE_INSTANCE_GET_PRIVATE ((o), BURNER_TYPE_CDDA2WAV, BurnerCdda2wavPrivate))
static GObjectClass *parent_class = NULL;


static BurnerBurnResult
burner_cdda2wav_post (BurnerJob *job)
{
	BurnerCdda2wavPrivate *priv;
	BurnerMedium *medium;
	BurnerJobAction action;
	BurnerDrive *drive;
	BurnerTrack *track;
	int track_num;
	int i;

	priv = BURNER_CDDA2WAV_PRIVATE (job);

	burner_job_get_action (job, &action);
	if (action == BURNER_JOB_ACTION_SIZE)
		return BURNER_BURN_OK;

	/* we add the tracks */
	track = NULL;
	burner_job_get_current_track (job, &track);

	drive = burner_track_disc_get_drive (BURNER_TRACK_DISC (track));
	medium = burner_drive_get_medium (drive);

	track_num = burner_medium_get_track_num (medium);
	for (i = 0; i < track_num; i ++) {
		BurnerTrackStream *track_stream;
		goffset block_num = 0;

		burner_medium_get_track_space (medium, i + 1, NULL, &block_num);
		track_stream = burner_track_stream_new ();

		burner_track_stream_set_format (track_stream,
		                                 BURNER_AUDIO_FORMAT_RAW|
		                                 BURNER_METADATA_INFO);

		BURNER_JOB_LOG (job, "Adding new audio track of size %" G_GOFFSET_FORMAT, BURNER_BYTES_TO_DURATION (block_num * 2352));

		/* either add .inf or .cdr files */
		if (!priv->is_inf) {
			gchar *uri;
			gchar *filename;

			if (track_num == 1)
				filename = g_strdup_printf ("%s.cdr", priv->file_pattern);
			else
				filename = g_strdup_printf ("%s_%02i.cdr", priv->file_pattern, i + 1);

			uri = g_filename_to_uri (filename, NULL, NULL);
			g_free (filename);

			burner_track_stream_set_source (track_stream, uri);
			g_free (uri);

			/* signal to cdrecord that we have an .inf file */
			if (i != 0)
				filename = g_strdup_printf ("%s_%02i.inf", priv->file_pattern, i);
			else
				filename = g_strdup_printf ("%s.inf", priv->file_pattern);

			burner_track_tag_add_string (BURNER_TRACK (track_stream),
			                              BURNER_CDRTOOLS_TRACK_INF_FILE,
			                              filename);
			g_free (filename);
		}

		/* Always set the boundaries after the source as
		 * burner_track_stream_set_source () resets the length */
		burner_track_stream_set_boundaries (track_stream,
		                                     0,
		                                     BURNER_BYTES_TO_DURATION (block_num * 2352),
		                                     0);
		burner_job_add_track (job, BURNER_TRACK (track_stream));
		g_object_unref (track_stream);
	}

	return burner_job_finished_session (job);
}

static gboolean
burner_cdda2wav_get_output_filename_pattern (BurnerCdda2wav *cdda2wav,
                                              GError **error)
{
	gchar *path;
	gchar *file_pattern;
	BurnerCdda2wavPrivate *priv;

	priv = BURNER_CDDA2WAV_PRIVATE (cdda2wav);

	if (priv->file_pattern) {
		g_free (priv->file_pattern);
		priv->file_pattern = NULL;
	}

	/* Create a tmp directory so cdda2wav can
	 * put all its stuff in there */
	path = NULL;
	burner_job_get_tmp_dir (BURNER_JOB (cdda2wav), &path, error);
	if (!path)
		return FALSE;

	file_pattern = g_strdup_printf ("%s/cd_file", path);
	g_free (path);

	/* NOTE: this file pattern is used to
	 * name all wav and inf files. It is followed
	 * by underscore/number of the track/extension */

	priv->file_pattern = file_pattern;
	return TRUE;
}

static BurnerBurnResult
burner_cdda2wav_read_stderr (BurnerProcess *process, const gchar *line)
{
	int num;
	BurnerCdda2wav *cdda2wav;
	BurnerCdda2wavPrivate *priv;

	cdda2wav = BURNER_CDDA2WAV (process);
	priv = BURNER_CDDA2WAV_PRIVATE (process);

	if (sscanf (line, "100%%  track %d '%*s' recorded successfully", &num) == 1) {
		gchar *string;

		priv->track_nb = num;
		string = g_strdup_printf (_("Copying audio track %02d"), priv->track_nb + 1);
		burner_job_set_current_action (BURNER_JOB (process),
		                                BURNER_BURN_ACTION_DRIVE_COPY,
		                                string,
		                                TRUE);
		g_free (string);
	}
	else if (strstr (line, "percent_done:")) {
		gchar *string;

		string = g_strdup_printf (_("Copying audio track %02d"), 1);
		burner_job_set_current_action (BURNER_JOB (process),
		                                BURNER_BURN_ACTION_DRIVE_COPY,
		                                string,
		                                TRUE);
		g_free (string);
	}
	/* we have to do this otherwise with sscanf it will 
	 * match every time it begins with a number */
	else if (strchr (line, '%') && sscanf (line, " %d%%", &num) == 1) {
		gdouble fraction;

		fraction = (gdouble) num / (gdouble) 100.0;
		fraction = ((gdouble) priv->track_nb + fraction) / (gdouble) priv->track_num;
		burner_job_set_progress (BURNER_JOB (cdda2wav), fraction);
		burner_job_start_progress (BURNER_JOB (process), FALSE);
	}

	return BURNER_BURN_OK;
}

static BurnerBurnResult
burner_cdda2wav_set_argv_image (BurnerCdda2wav *cdda2wav,
				GPtrArray *argv,
				GError **error)
{
	BurnerCdda2wavPrivate *priv;
	int fd_out;

	priv = BURNER_CDDA2WAV_PRIVATE (cdda2wav);

	/* We want raw output */
	g_ptr_array_add (argv, g_strdup ("output-format=cdr"));

	/* we want all tracks */
	g_ptr_array_add (argv, g_strdup ("-B"));

	priv->is_inf = FALSE;

	if (burner_job_get_fd_out (BURNER_JOB (cdda2wav), &fd_out) == BURNER_BURN_OK) {
		/* On the fly copying */
		g_ptr_array_add (argv, g_strdup ("-"));
	}
	else {
		if (!burner_cdda2wav_get_output_filename_pattern (cdda2wav, error))
			return BURNER_BURN_ERR;

		g_ptr_array_add (argv, g_strdup (priv->file_pattern));

		burner_job_set_current_action (BURNER_JOB (cdda2wav),
		                                BURNER_BURN_ACTION_DRIVE_COPY,
		                                _("Preparing to copy audio disc"),
		                                FALSE);
	}

	return BURNER_BURN_OK;
}

static BurnerBurnResult
burner_cdda2wav_set_argv_size (BurnerCdda2wav *cdda2wav,
                                GPtrArray *argv,
                                GError **error)
{
	BurnerCdda2wavPrivate *priv;
	BurnerMedium *medium;
	BurnerTrack *track;
	BurnerDrive *drive;
	goffset medium_len;
	int i;

	priv = BURNER_CDDA2WAV_PRIVATE (cdda2wav);

	/* we add the tracks */
	medium_len = 0;
	track = NULL;
	burner_job_get_current_track (BURNER_JOB (cdda2wav), &track);

	drive = burner_track_disc_get_drive (BURNER_TRACK_DISC (track));
	medium = burner_drive_get_medium (drive);

	priv->track_num = burner_medium_get_track_num (medium);
	for (i = 0; i < priv->track_num; i ++) {
		goffset len = 0;

		burner_medium_get_track_space (medium, i, NULL, &len);
		medium_len += len;
	}
	burner_job_set_output_size_for_current_track (BURNER_JOB (cdda2wav), medium_len, medium_len * 2352);

	/* if there isn't any output file then that
	 * means we have to generate all the
	 * .inf files for cdrecord. */
	if (burner_job_get_audio_output (BURNER_JOB (cdda2wav), NULL) != BURNER_BURN_OK)
		return BURNER_BURN_NOT_RUNNING;

	/* we want all tracks */
	g_ptr_array_add (argv, g_strdup ("-B"));

	/* since we're running for an on-the-fly burning
	 * we only want the .inf files */
	g_ptr_array_add (argv, g_strdup ("-J"));

	if (!burner_cdda2wav_get_output_filename_pattern (cdda2wav, error))
		return BURNER_BURN_ERR;

	g_ptr_array_add (argv, g_strdup (priv->file_pattern));

	priv->is_inf = TRUE;

	return BURNER_BURN_OK;
}

static BurnerBurnResult
burner_cdda2wav_set_argv (BurnerProcess *process,
			  GPtrArray *argv,
			  GError **error)
{
	BurnerDrive *drive;
	const gchar *device;
	BurnerTrack *track;
	BurnerJobAction action;
	BurnerBurnResult result;
	BurnerCdda2wav *cdda2wav;

	cdda2wav = BURNER_CDDA2WAV (process);

	g_ptr_array_add (argv, g_strdup ("cdda2wav"));

	/* Add the device path */
	track = NULL;
	result = burner_job_get_current_track (BURNER_JOB (process), &track);
	if (!track)
		return result;

	drive = burner_track_disc_get_drive (BURNER_TRACK_DISC (track));
	device = burner_drive_get_device (drive);
	g_ptr_array_add (argv, g_strdup_printf ("dev=%s", device));

	/* Have it talking */
	g_ptr_array_add (argv, g_strdup ("-v255"));

	burner_job_get_action (BURNER_JOB (cdda2wav), &action);
	if (action == BURNER_JOB_ACTION_SIZE)
		result = burner_cdda2wav_set_argv_size (cdda2wav, argv, error);
	else if (action == BURNER_JOB_ACTION_IMAGE)
		result = burner_cdda2wav_set_argv_image (cdda2wav, argv, error);
	else
		BURNER_JOB_NOT_SUPPORTED (cdda2wav);

	return result;
}

static void
burner_cdda2wav_class_init (BurnerCdda2wavClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);
	BurnerProcessClass *process_class = BURNER_PROCESS_CLASS (klass);

	g_type_class_add_private (klass, sizeof (BurnerCdda2wavPrivate));

	parent_class = g_type_class_peek_parent(klass);
	object_class->finalize = burner_cdda2wav_finalize;

	process_class->stderr_func = burner_cdda2wav_read_stderr;
	process_class->set_argv = burner_cdda2wav_set_argv;
	process_class->post = burner_cdda2wav_post;
}

static void
burner_cdda2wav_init (BurnerCdda2wav *obj)
{ }

static void
burner_cdda2wav_finalize (GObject *object)
{
	BurnerCdda2wavPrivate *priv;

	priv = BURNER_CDDA2WAV_PRIVATE (object);

	if (priv->file_pattern) {
		g_free (priv->file_pattern);
		priv->file_pattern = NULL;
	}
	G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
burner_cdda2wav_export_caps (BurnerPlugin *plugin)
{
	GSList *output;
	GSList *input;

	burner_plugin_define (plugin,
			       "cdda2wav",
	                       NULL,
			       _("Copy tracks from an audio CD with all associated information"),
			       "Philippe Rouquier",
			       1);

	output = burner_caps_audio_new (BURNER_PLUGIN_IO_ACCEPT_FILE /*|BURNER_PLUGIN_IO_ACCEPT_PIPE*/, /* Keep on the fly on hold until it gets proper testing */
					 BURNER_AUDIO_FORMAT_RAW|
	                                 BURNER_METADATA_INFO);

	input = burner_caps_disc_new (BURNER_MEDIUM_CDR|
	                               BURNER_MEDIUM_CDRW|
	                               BURNER_MEDIUM_CDROM|
	                               BURNER_MEDIUM_CLOSED|
	                               BURNER_MEDIUM_APPENDABLE|
	                               BURNER_MEDIUM_HAS_AUDIO);

	burner_plugin_link_caps (plugin, output, input);
	g_slist_free (output);
	g_slist_free (input);

	burner_plugin_register_group (plugin, _(CDRTOOLS_DESCRIPTION));
}

G_MODULE_EXPORT void
burner_plugin_check_config (BurnerPlugin *plugin)
{
	gchar *prog_path;

	/* Just check that the program is in the path and executable. */
	prog_path = g_find_program_in_path ("cdda2wav");
	if (!prog_path) {
		burner_plugin_add_error (plugin,
		                          BURNER_PLUGIN_ERROR_MISSING_APP,
		                          "cdda2wav");
		return;
	}

	if (!g_file_test (prog_path, G_FILE_TEST_IS_EXECUTABLE)) {
		g_free (prog_path);
		burner_plugin_add_error (plugin,
		                          BURNER_PLUGIN_ERROR_MISSING_APP,
		                          "cdda2wav");
		return;
	}
	g_free (prog_path);

	/* This is what it should be. Now, as I did not write and icedax plugin
	 * the above is enough so that cdda2wav can use a symlink to icedax.
	 * As for the version checking, it becomes impossible given that with
	 * icedax the string would not start with cdda2wav. So ... */
	/*
	gint version [3] = { 2, 0, -1};
	burner_plugin_test_app (plugin,
	                         "cdda2wav",
	                         "--version",
	                         "cdda2wav %d.%d",
	                         version);
	*/
}
