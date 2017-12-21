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

#include <math.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#include <glib.h>
#include <glib-object.h>
#include <glib/gi18n-lib.h>
#include <glib/gstdio.h>
#include <gmodule.h>

#include "burner-error.h"
#include "burner-plugin-registration.h"
#include "burn-job.h"
#include "burn-process.h"
#include "burner-track-disc.h"
#include "burner-track-image.h"
#include "burner-drive.h"
#include "burner-medium.h"

#define CDRDAO_DESCRIPTION		N_("cdrdao burning suite")

#define BURNER_TYPE_CDRDAO         (burner_cdrdao_get_type ())
#define BURNER_CDRDAO(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), BURNER_TYPE_CDRDAO, BurnerCdrdao))
#define BURNER_CDRDAO_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k), BURNER_TYPE_CDRDAO, BurnerCdrdaoClass))
#define BURNER_IS_CDRDAO(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), BURNER_TYPE_CDRDAO))
#define BURNER_IS_CDRDAO_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), BURNER_TYPE_CDRDAO))
#define BURNER_CDRDAO_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), BURNER_TYPE_CDRDAO, BurnerCdrdaoClass))

BURNER_PLUGIN_BOILERPLATE (BurnerCdrdao, burner_cdrdao, BURNER_TYPE_PROCESS, BurnerProcess);

struct _BurnerCdrdaoPrivate {
 	gchar *tmp_toc_path;
	guint use_raw:1;
};
typedef struct _BurnerCdrdaoPrivate BurnerCdrdaoPrivate;
#define BURNER_CDRDAO_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), BURNER_TYPE_CDRDAO, BurnerCdrdaoPrivate)) 

static GObjectClass *parent_class = NULL;

#define BURNER_SCHEMA_CONFIG		"org.gnome.burner.config"
#define BURNER_KEY_RAW_FLAG		"raw-flag"

static gboolean
burner_cdrdao_read_stderr_image (BurnerCdrdao *cdrdao, const gchar *line)
{
	int min, sec, sub, s1;

	if (sscanf (line, "%d:%d:%d", &min, &sec, &sub) == 3) {
		guint64 secs = min * 60 + sec;

		burner_job_set_written_track (BURNER_JOB (cdrdao), secs * 75 * 2352);
		if (secs > 2)
			burner_job_start_progress (BURNER_JOB (cdrdao), FALSE);
	}
	else if (sscanf (line, "Leadout %*s %*d %d:%d:%*d(%i)", &min, &sec, &s1) == 3) {
		BurnerJobAction action;

		burner_job_get_action (BURNER_JOB (cdrdao), &action);
		if (action == BURNER_JOB_ACTION_SIZE) {
			/* get the number of sectors. As we added -raw sector = 2352 bytes */
			burner_job_set_output_size_for_current_track (BURNER_JOB (cdrdao), s1, (gint64) s1 * 2352ULL);
			burner_job_finished_session (BURNER_JOB (cdrdao));
		}
	}
	else if (strstr (line, "Copying audio tracks")) {
		burner_job_set_current_action (BURNER_JOB (cdrdao),
						BURNER_BURN_ACTION_DRIVE_COPY,
						_("Copying audio track"),
						FALSE);
	}
	else if (strstr (line, "Copying data track")) {
		burner_job_set_current_action (BURNER_JOB (cdrdao),
						BURNER_BURN_ACTION_DRIVE_COPY,
						_("Copying data track"),
						FALSE);
	}
	else
		return FALSE;

	return TRUE;
}

static gboolean
burner_cdrdao_read_stderr_record (BurnerCdrdao *cdrdao, const gchar *line)
{
	int fifo, track, min, sec;
	guint written, total;

	if (sscanf (line, "Wrote %u of %u (Buffers %d%%  %*s", &written, &total, &fifo) >= 2) {
		burner_job_set_dangerous (BURNER_JOB (cdrdao), TRUE);

		burner_job_set_written_session (BURNER_JOB (cdrdao), written * 1048576);
		burner_job_set_current_action (BURNER_JOB (cdrdao),
						BURNER_BURN_ACTION_RECORDING,
						NULL,
						FALSE);

		burner_job_start_progress (BURNER_JOB (cdrdao), FALSE);
	}
	else if (sscanf (line, "Wrote %*s blocks. Buffer fill min") == 1) {
		/* this is for fixating phase */
		burner_job_set_current_action (BURNER_JOB (cdrdao),
						BURNER_BURN_ACTION_FIXATING,
						NULL,
						FALSE);
	}
	else if (sscanf (line, "Analyzing track %d %*s start %d:%d:%*d, length %*d:%*d:%*d", &track, &min, &sec) == 3) {
		gchar *string;

		string = g_strdup_printf (_("Analysing track %02i"), track);
		burner_job_set_current_action (BURNER_JOB (cdrdao),
						BURNER_BURN_ACTION_ANALYSING,
						string,
						TRUE);
		g_free (string);
	}
	else if (sscanf (line, "%d:%d:%*d", &min, &sec) == 2) {
		gint64 written;
		guint64 secs = min * 60 + sec;

		if (secs > 2)
			burner_job_start_progress (BURNER_JOB (cdrdao), FALSE);

		written = secs * 75 * 2352;
		burner_job_set_written_session (BURNER_JOB (cdrdao), written);
	}
	else if (strstr (line, "Writing track")) {
		burner_job_set_dangerous (BURNER_JOB (cdrdao), TRUE);
	}
	else if (strstr (line, "Writing finished successfully")
	     ||  strstr (line, "On-the-fly CD copying finished successfully")) {
		burner_job_set_dangerous (BURNER_JOB (cdrdao), FALSE);
	}
	else if (strstr (line, "Blanking disk...")) {
		burner_job_set_current_action (BURNER_JOB (cdrdao),
						BURNER_BURN_ACTION_BLANKING,
						NULL,
						FALSE);
		burner_job_start_progress (BURNER_JOB (cdrdao), FALSE);
		burner_job_set_dangerous (BURNER_JOB (cdrdao), TRUE);
	}
	else {
		gchar *name = NULL;
		gchar *cuepath = NULL;
		BurnerTrack *track = NULL;
		BurnerJobAction action;

		/* Try to catch error could not find cue file */

		/* Track could be NULL here if we're simply blanking a medium */
		burner_job_get_action (BURNER_JOB (cdrdao), &action);
		if (action == BURNER_JOB_ACTION_ERASE)
			return TRUE;

		burner_job_get_current_track (BURNER_JOB (cdrdao), &track);
		if (!track)
			return FALSE;

		cuepath = burner_track_image_get_toc_source (BURNER_TRACK_IMAGE (track), FALSE);

		if (!cuepath)
			return FALSE;

		if (!strstr (line, "ERROR: Could not find input file")) {
			g_free (cuepath);
			return FALSE;
		}

		name = g_path_get_basename (cuepath);
		g_free (cuepath);

		burner_job_error (BURNER_JOB (cdrdao),
				   g_error_new (BURNER_BURN_ERROR,
						BURNER_BURN_ERROR_FILE_NOT_FOUND,
						/* Translators: %s is a filename */
						_("\"%s\" could not be found"),
						name));
		g_free (name);
	}

	return FALSE;
}

static BurnerBurnResult
burner_cdrdao_read_stderr (BurnerProcess *process, const gchar *line)
{
	BurnerCdrdao *cdrdao;
	gboolean result = FALSE;
	BurnerJobAction action;

	cdrdao = BURNER_CDRDAO (process);

	burner_job_get_action (BURNER_JOB (cdrdao), &action);
	if (action == BURNER_JOB_ACTION_RECORD
	||  action == BURNER_JOB_ACTION_ERASE)
		result = burner_cdrdao_read_stderr_record (cdrdao, line);
	else if (action == BURNER_JOB_ACTION_IMAGE
	     ||  action == BURNER_JOB_ACTION_SIZE)
		result = burner_cdrdao_read_stderr_image (cdrdao, line);

	if (result)
		return BURNER_BURN_OK;

	if (strstr (line, "Cannot setup device")) {
		burner_job_error (BURNER_JOB (cdrdao),
				   g_error_new (BURNER_BURN_ERROR,
						BURNER_BURN_ERROR_DRIVE_BUSY,
						_("The drive is busy")));
	}
	else if (strstr (line, "Operation not permitted. Cannot send SCSI")) {
		burner_job_error (BURNER_JOB (cdrdao),
				   g_error_new (BURNER_BURN_ERROR,
						BURNER_BURN_ERROR_PERMISSION,
						_("You do not have the required permissions to use this drive")));
	}

	return BURNER_BURN_OK;
}

static void
burner_cdrdao_set_argv_device (BurnerCdrdao *cdrdao,
				GPtrArray *argv)
{
	gchar *device = NULL;

	g_ptr_array_add (argv, g_strdup ("--device"));

	/* NOTE: that function returns either bus_target_lun or the device path
	 * according to OSes. Basically it returns bus/target/lun only for FreeBSD
	 * which is the only OS in need for that. For all others it returns the device
	 * path. */
	burner_job_get_bus_target_lun (BURNER_JOB (cdrdao), &device);
	g_ptr_array_add (argv, device);
}

static void
burner_cdrdao_set_argv_common_rec (BurnerCdrdao *cdrdao,
				    GPtrArray *argv)
{
	BurnerBurnFlag flags;
	gchar *speed_str;
	guint speed;

	burner_job_get_flags (BURNER_JOB (cdrdao), &flags);
	if (flags & BURNER_BURN_FLAG_DUMMY)
		g_ptr_array_add (argv, g_strdup ("--simulate"));

	g_ptr_array_add (argv, g_strdup ("--speed"));

	burner_job_get_speed (BURNER_JOB (cdrdao), &speed);
	speed_str = g_strdup_printf ("%d", speed);
	g_ptr_array_add (argv, speed_str);

	if (flags & BURNER_BURN_FLAG_OVERBURN)
		g_ptr_array_add (argv, g_strdup ("--overburn"));
	if (flags & BURNER_BURN_FLAG_MULTI)
		g_ptr_array_add (argv, g_strdup ("--multi"));
}

static void
burner_cdrdao_set_argv_common (BurnerCdrdao *cdrdao,
				GPtrArray *argv)
{
	BurnerBurnFlag flags;

	burner_job_get_flags (BURNER_JOB (cdrdao), &flags);

	/* cdrdao manual says it is a similar option to gracetime */
	if (flags & BURNER_BURN_FLAG_NOGRACE)
		g_ptr_array_add (argv, g_strdup ("-n"));

	g_ptr_array_add (argv, g_strdup ("-v"));
	g_ptr_array_add (argv, g_strdup ("2"));
}

static BurnerBurnResult
burner_cdrdao_set_argv_record (BurnerCdrdao *cdrdao,
				GPtrArray *argv)
{
	BurnerTrackType *type = NULL;
	BurnerCdrdaoPrivate *priv;

	priv = BURNER_CDRDAO_PRIVATE (cdrdao); 

	g_ptr_array_add (argv, g_strdup ("cdrdao"));

	type = burner_track_type_new ();
	burner_job_get_input_type (BURNER_JOB (cdrdao), type);

        if (burner_track_type_get_has_medium (type)) {
		BurnerDrive *drive;
		BurnerTrack *track;
		BurnerBurnFlag flags;

		g_ptr_array_add (argv, g_strdup ("copy"));
		burner_cdrdao_set_argv_device (cdrdao, argv);
		burner_cdrdao_set_argv_common (cdrdao, argv);
		burner_cdrdao_set_argv_common_rec (cdrdao, argv);

		burner_job_get_flags (BURNER_JOB (cdrdao), &flags);
		if (flags & BURNER_BURN_FLAG_NO_TMP_FILES)
			g_ptr_array_add (argv, g_strdup ("--on-the-fly"));

		if (priv->use_raw)
		  	g_ptr_array_add (argv, g_strdup ("--driver generic-mmc-raw")); 

		g_ptr_array_add (argv, g_strdup ("--source-device"));

		burner_job_get_current_track (BURNER_JOB (cdrdao), &track);
		drive = burner_track_disc_get_drive (BURNER_TRACK_DISC (track));

		/* NOTE: that function returns either bus_target_lun or the device path
		 * according to OSes. Basically it returns bus/target/lun only for FreeBSD
		 * which is the only OS in need for that. For all others it returns the device
		 * path. */
		g_ptr_array_add (argv, burner_drive_get_bus_target_lun_string (drive));
	}
	else if (burner_track_type_get_has_image (type)) {
		gchar *cuepath;
		BurnerTrack *track;

		g_ptr_array_add (argv, g_strdup ("write"));
		
		burner_job_get_current_track (BURNER_JOB (cdrdao), &track);

		if (burner_track_type_get_image_format (type) == BURNER_IMAGE_FORMAT_CUE) {
			gchar *parent;

			cuepath = burner_track_image_get_toc_source (BURNER_TRACK_IMAGE (track), FALSE);
			parent = g_path_get_dirname (cuepath);
			burner_process_set_working_directory (BURNER_PROCESS (cdrdao), parent);
			g_free (parent);

			/* This does not work as toc2cue will use BINARY even if
			 * if endianness is big endian */
			/* we need to check endianness */
			/* if (burner_track_image_need_byte_swap (BURNER_TRACK_IMAGE (track)))
				g_ptr_array_add (argv, g_strdup ("--swap")); */
		}
		else if (burner_track_type_get_image_format (type) == BURNER_IMAGE_FORMAT_CDRDAO) {
			/* CDRDAO files are always BIG ENDIAN */
			cuepath = burner_track_image_get_toc_source (BURNER_TRACK_IMAGE (track), FALSE);
		}
		else {
			burner_track_type_free (type);
			BURNER_JOB_NOT_SUPPORTED (cdrdao);
		}

		if (!cuepath) {
			burner_track_type_free (type);
			BURNER_JOB_NOT_READY (cdrdao);
		}

		burner_cdrdao_set_argv_device (cdrdao, argv);
		burner_cdrdao_set_argv_common (cdrdao, argv);
		burner_cdrdao_set_argv_common_rec (cdrdao, argv);

		g_ptr_array_add (argv, cuepath);
	}
	else {
		burner_track_type_free (type);
		BURNER_JOB_NOT_SUPPORTED (cdrdao);
	}

	burner_track_type_free (type);
	burner_job_set_use_average_rate (BURNER_JOB (cdrdao), TRUE);
	burner_job_set_current_action (BURNER_JOB (cdrdao),
					BURNER_BURN_ACTION_START_RECORDING,
					NULL,
					FALSE);
	return BURNER_BURN_OK;
}

static BurnerBurnResult
burner_cdrdao_set_argv_blank (BurnerCdrdao *cdrdao,
			       GPtrArray *argv)
{
	BurnerBurnFlag flags;

	g_ptr_array_add (argv, g_strdup ("cdrdao"));
	g_ptr_array_add (argv, g_strdup ("blank"));

	burner_cdrdao_set_argv_device (cdrdao, argv);
	burner_cdrdao_set_argv_common (cdrdao, argv);

	g_ptr_array_add (argv, g_strdup ("--blank-mode"));
	burner_job_get_flags (BURNER_JOB (cdrdao), &flags);
	if (!(flags & BURNER_BURN_FLAG_FAST_BLANK))
		g_ptr_array_add (argv, g_strdup ("full"));
	else
		g_ptr_array_add (argv, g_strdup ("minimal"));

	burner_job_set_current_action (BURNER_JOB (cdrdao),
					BURNER_BURN_ACTION_BLANKING,
					NULL,
					FALSE);

	return BURNER_BURN_OK;
}

static BurnerBurnResult
burner_cdrdao_post (BurnerJob *job)
{
	BurnerCdrdaoPrivate *priv;

	priv = BURNER_CDRDAO_PRIVATE (job);
	if (!priv->tmp_toc_path) {
		burner_job_finished_session (job);
		return BURNER_BURN_OK;
	}

	/* we have to run toc2cue now to convert the toc file into a cue file */
	return BURNER_BURN_RETRY;
}

static BurnerBurnResult
burner_cdrdao_start_toc2cue (BurnerCdrdao *cdrdao,
			      GPtrArray *argv,
			      GError **error)
{
	gchar *cue_output;
	BurnerBurnResult result;
	BurnerCdrdaoPrivate *priv;

	priv = BURNER_CDRDAO_PRIVATE (cdrdao);

	g_ptr_array_add (argv, g_strdup ("toc2cue"));

	g_ptr_array_add (argv, priv->tmp_toc_path);
	priv->tmp_toc_path = NULL;

	result = burner_job_get_image_output (BURNER_JOB (cdrdao),
					       NULL,
					       &cue_output);
	if (result != BURNER_BURN_OK)
		return result;

	g_ptr_array_add (argv, cue_output);

	/* if there is a file toc2cue will fail */
	g_remove (cue_output);

	burner_job_set_current_action (BURNER_JOB (cdrdao),
					BURNER_BURN_ACTION_CREATING_IMAGE,
					_("Converting toc file"),
					TRUE);

	return BURNER_BURN_OK;
}

static BurnerBurnResult
burner_cdrdao_set_argv_image (BurnerCdrdao *cdrdao,
			       GPtrArray *argv,
			       GError **error)
{
	gchar *image = NULL, *toc = NULL;
	BurnerTrackType *output = NULL;
	BurnerCdrdaoPrivate *priv;
	BurnerBurnResult result;
	BurnerJobAction action;
	BurnerDrive *drive;
	BurnerTrack *track;

	priv = BURNER_CDRDAO_PRIVATE (cdrdao);
	if (priv->tmp_toc_path)
		return burner_cdrdao_start_toc2cue (cdrdao, argv, error);

	g_ptr_array_add (argv, g_strdup ("cdrdao"));
	g_ptr_array_add (argv, g_strdup ("read-cd"));
	g_ptr_array_add (argv, g_strdup ("--device"));

	burner_job_get_current_track (BURNER_JOB (cdrdao), &track);
	drive = burner_track_disc_get_drive (BURNER_TRACK_DISC (track));

	/* NOTE: that function returns either bus_target_lun or the device path
	 * according to OSes. Basically it returns bus/target/lun only for FreeBSD
	 * which is the only OS in need for that. For all others it returns the device
	 * path. */
	g_ptr_array_add (argv, burner_drive_get_bus_target_lun_string (drive));
	g_ptr_array_add (argv, g_strdup ("--read-raw"));

	/* This is done so that if a cue file is required we first generate
	 * a temporary toc file that will be later converted to a cue file.
	 * The datafile is written where it should be from the start. */
	output = burner_track_type_new ();
	burner_job_get_output_type (BURNER_JOB (cdrdao), output);

	if (burner_track_type_get_image_format (output) == BURNER_IMAGE_FORMAT_CDRDAO) {
		result = burner_job_get_image_output (BURNER_JOB (cdrdao),
						       &image,
						       &toc);
		if (result != BURNER_BURN_OK) {
			burner_track_type_free (output);
			return result;
		}
	}
	else if (burner_track_type_get_image_format (output) == BURNER_IMAGE_FORMAT_CUE) {
		/* NOTE: we don't generate the .cue file right away; we'll call
		 * toc2cue right after we finish */
		result = burner_job_get_image_output (BURNER_JOB (cdrdao),
						       &image,
						       NULL);
		if (result != BURNER_BURN_OK) {
			burner_track_type_free (output);
			return result;
		}

		result = burner_job_get_tmp_file (BURNER_JOB (cdrdao),
						   NULL,
						   &toc,
						   error);
		if (result != BURNER_BURN_OK) {
			g_free (image);
			burner_track_type_free (output);
			return result;
		}

		/* save the temporary toc path to resuse it later. */
		priv->tmp_toc_path = g_strdup (toc);
	}

	burner_track_type_free (output);

	/* it's safe to remove them: session/task make sure they don't exist 
	 * when there is the proper flag whether it be tmp or real output. */ 
	if (toc)
		g_remove (toc);
	if (image)
		g_remove (image);

	burner_job_get_action (BURNER_JOB (cdrdao), &action);
	if (action == BURNER_JOB_ACTION_SIZE) {
		burner_job_set_current_action (BURNER_JOB (cdrdao),
						BURNER_BURN_ACTION_GETTING_SIZE,
						NULL,
						FALSE);
		burner_job_start_progress (BURNER_JOB (cdrdao), FALSE);
	}

	g_ptr_array_add (argv, g_strdup ("--datafile"));
	g_ptr_array_add (argv, image);

	g_ptr_array_add (argv, g_strdup ("-v"));
	g_ptr_array_add (argv, g_strdup ("2"));

	g_ptr_array_add (argv, toc);

	burner_job_set_use_average_rate (BURNER_JOB (cdrdao), TRUE);

	return BURNER_BURN_OK;
}

static BurnerBurnResult
burner_cdrdao_set_argv (BurnerProcess *process,
			 GPtrArray *argv,
			 GError **error)
{
	BurnerCdrdao *cdrdao;
	BurnerJobAction action;

	cdrdao = BURNER_CDRDAO (process);

	/* sets the first argv */
	burner_job_get_action (BURNER_JOB (cdrdao), &action);
	if (action == BURNER_JOB_ACTION_RECORD)
		return burner_cdrdao_set_argv_record (cdrdao, argv);
	else if (action == BURNER_JOB_ACTION_ERASE)
		return burner_cdrdao_set_argv_blank (cdrdao, argv);
	else if (action == BURNER_JOB_ACTION_IMAGE)
		return burner_cdrdao_set_argv_image (cdrdao, argv, error);
	else if (action == BURNER_JOB_ACTION_SIZE) {
		BurnerTrack *track;

		burner_job_get_current_track (BURNER_JOB (cdrdao), &track);
		if (BURNER_IS_TRACK_DISC (track)) {
			goffset sectors = 0;

			burner_track_get_size (track, &sectors, NULL);

			/* cdrdao won't get a track size under 300 sectors */
			if (sectors < 300)
				sectors = 300;

			burner_job_set_output_size_for_current_track (BURNER_JOB (cdrdao),
								       sectors,
								       sectors * 2352ULL);
		}
		else
			return BURNER_BURN_NOT_SUPPORTED;

		return BURNER_BURN_NOT_RUNNING;
	}

	BURNER_JOB_NOT_SUPPORTED (cdrdao);
}

static void
burner_cdrdao_class_init (BurnerCdrdaoClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);
	BurnerProcessClass *process_class = BURNER_PROCESS_CLASS (klass);

	g_type_class_add_private (klass, sizeof (BurnerCdrdaoPrivate));

	parent_class = g_type_class_peek_parent(klass);
	object_class->finalize = burner_cdrdao_finalize;

	process_class->stderr_func = burner_cdrdao_read_stderr;
	process_class->set_argv = burner_cdrdao_set_argv;
	process_class->post = burner_cdrdao_post;
}

static void
burner_cdrdao_init (BurnerCdrdao *obj)
{  
	GSettings *settings;
 	BurnerCdrdaoPrivate *priv;
 	
	/* load our "configuration" */
 	priv = BURNER_CDRDAO_PRIVATE (obj);

	settings = g_settings_new (BURNER_SCHEMA_CONFIG);
	priv->use_raw = g_settings_get_boolean (settings, BURNER_KEY_RAW_FLAG);
	g_object_unref (settings);
}

static void
burner_cdrdao_finalize (GObject *object)
{
	BurnerCdrdaoPrivate *priv;

	priv = BURNER_CDRDAO_PRIVATE (object);
	if (priv->tmp_toc_path) {
		g_free (priv->tmp_toc_path);
		priv->tmp_toc_path = NULL;
	}

	G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
burner_cdrdao_export_caps (BurnerPlugin *plugin)
{
	GSList *input;
	GSList *output;
	BurnerPluginConfOption *use_raw; 
	const BurnerMedia media_w = BURNER_MEDIUM_CD|
				     BURNER_MEDIUM_WRITABLE|
				     BURNER_MEDIUM_REWRITABLE|
				     BURNER_MEDIUM_BLANK;
	const BurnerMedia media_rw = BURNER_MEDIUM_CD|
				      BURNER_MEDIUM_REWRITABLE|
				      BURNER_MEDIUM_APPENDABLE|
				      BURNER_MEDIUM_CLOSED|
				      BURNER_MEDIUM_HAS_DATA|
				      BURNER_MEDIUM_HAS_AUDIO|
				      BURNER_MEDIUM_BLANK;

	burner_plugin_define (plugin,
			       "cdrdao",
	                       NULL,
			       _("Copies, burns and blanks CDs"),
			       "Philippe Rouquier",
			       0);

	/* that's for cdrdao images: CDs only as input */
	input = burner_caps_disc_new (BURNER_MEDIUM_CD|
				       BURNER_MEDIUM_ROM|
				       BURNER_MEDIUM_WRITABLE|
				       BURNER_MEDIUM_REWRITABLE|
				       BURNER_MEDIUM_APPENDABLE|
				       BURNER_MEDIUM_CLOSED|
				       BURNER_MEDIUM_HAS_AUDIO|
				       BURNER_MEDIUM_HAS_DATA);

	/* an image can be created ... */
	output = burner_caps_image_new (BURNER_PLUGIN_IO_ACCEPT_FILE,
					 BURNER_IMAGE_FORMAT_CDRDAO);
	burner_plugin_link_caps (plugin, output, input);
	g_slist_free (output);

	output = burner_caps_image_new (BURNER_PLUGIN_IO_ACCEPT_FILE,
					 BURNER_IMAGE_FORMAT_CUE);
	burner_plugin_link_caps (plugin, output, input);
	g_slist_free (output);

	/* ... or a disc */
	output = burner_caps_disc_new (media_w);
	burner_plugin_link_caps (plugin, output, input);
	g_slist_free (input);

	/* cdrdao can also record these types of images to a disc */
	input = burner_caps_image_new (BURNER_PLUGIN_IO_ACCEPT_FILE,
					BURNER_IMAGE_FORMAT_CDRDAO|
					BURNER_IMAGE_FORMAT_CUE);
	
	burner_plugin_link_caps (plugin, output, input);
	g_slist_free (output);
	g_slist_free (input);

	/* cdrdao is used to burn images so it can't APPEND and the disc must
	 * have been blanked before (it can't overwrite)
	 * NOTE: BURNER_MEDIUM_FILE is needed here because of restriction API
	 * when we output an image. */
	burner_plugin_set_flags (plugin,
				  media_w|
				  BURNER_MEDIUM_FILE,
				  BURNER_BURN_FLAG_DAO|
				  BURNER_BURN_FLAG_BURNPROOF|
				  BURNER_BURN_FLAG_OVERBURN|
				  BURNER_BURN_FLAG_DUMMY|
				  BURNER_BURN_FLAG_NOGRACE,
				  BURNER_BURN_FLAG_NONE);

	/* cdrdao can also blank */
	output = burner_caps_disc_new (media_rw);
	burner_plugin_blank_caps (plugin, output);
	g_slist_free (output);

	burner_plugin_set_blank_flags (plugin,
					media_rw,
					BURNER_BURN_FLAG_NOGRACE|
					BURNER_BURN_FLAG_FAST_BLANK,
					BURNER_BURN_FLAG_NONE);

	use_raw = burner_plugin_conf_option_new (BURNER_KEY_RAW_FLAG,
						  _("Enable the \"--driver generic-mmc-raw\" flag (see cdrdao manual)"),
						  BURNER_PLUGIN_OPTION_BOOL);

	burner_plugin_add_conf_option (plugin, use_raw);

	burner_plugin_register_group (plugin, _(CDRDAO_DESCRIPTION));
}

G_MODULE_EXPORT void
burner_plugin_check_config (BurnerPlugin *plugin)
{
	gint version [3] = { 1, 2, 0};
	burner_plugin_test_app (plugin,
	                         "cdrdao",
	                         "version",
	                         "Cdrdao version %d.%d.%d - (C) Andreas Mueller <andreas@daneb.de>",
	                         version);

	burner_plugin_test_app (plugin,
	                         "toc2cue",
	                         "-V",
	                         "%d.%d.%d",
	                         version);
}
