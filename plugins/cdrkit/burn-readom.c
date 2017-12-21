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
#include <stdlib.h>

#include <glib.h>
#include <glib/gi18n-lib.h>
#include <glib/gstdio.h>
#include <gmodule.h>

#include "burn-cdrkit.h"
#include "burn-process.h"
#include "burn-job.h"
#include "burner-plugin-registration.h"
#include "burner-tags.h"
#include "burner-track-disc.h"

#include "burn-volume.h"
#include "burner-drive.h"


#define BURNER_TYPE_READOM         (burner_readom_get_type ())
#define BURNER_READOM(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), BURNER_TYPE_READOM, BurnerReadom))
#define BURNER_READOM_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k), BURNER_TYPE_READOM, BurnerReadomClass))
#define BURNER_IS_READOM(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), BURNER_TYPE_READOM))
#define BURNER_IS_READOM_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), BURNER_TYPE_READOM))
#define BURNER_READOM_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), BURNER_TYPE_READOM, BurnerReadomClass))

BURNER_PLUGIN_BOILERPLATE (BurnerReadom, burner_readom, BURNER_TYPE_PROCESS, BurnerProcess);

static GObjectClass *parent_class = NULL;

static BurnerBurnResult
burner_readom_read_stderr (BurnerProcess *process, const gchar *line)
{
	BurnerReadom *readom;
	gint dummy1;
	gint dummy2;
	gchar *pos;

	readom = BURNER_READOM (process);

	if ((pos = strstr (line, "addr:"))) {
		gint sector;
		gint64 written;
		BurnerTrackType *output = NULL;

		pos += strlen ("addr:");
		sector = strtoll (pos, NULL, 10);

		output = burner_track_type_new ();
		burner_job_get_output_type (BURNER_JOB (readom), output);

		if (burner_track_type_get_image_format (output) == BURNER_IMAGE_FORMAT_BIN)
			written = (gint64) ((gint64) sector * 2048ULL);
		else if (burner_track_type_get_image_format (output) == BURNER_IMAGE_FORMAT_CLONE)
			written = (gint64) ((gint64) sector * 2448ULL);
		else
			written = (gint64) ((gint64) sector * 2048ULL);

		burner_job_set_written_track (BURNER_JOB (readom), written);

		if (sector > 10)
			burner_job_start_progress (BURNER_JOB (readom), FALSE);

		burner_track_type_free (output);
	}
	else if ((pos = strstr (line, "Capacity:"))) {
		burner_job_set_current_action (BURNER_JOB (readom),
						BURNER_BURN_ACTION_DRIVE_COPY,
						NULL,
						FALSE);
	}
	else if (strstr (line, "Device not ready.")) {
		burner_job_error (BURNER_JOB (readom),
				   g_error_new (BURNER_BURN_ERROR,
						BURNER_BURN_ERROR_DRIVE_BUSY,
						_("The drive is busy")));
	}
	else if (strstr (line, "Cannot open SCSI driver.")) {
		burner_job_error (BURNER_JOB (readom),
				   g_error_new (BURNER_BURN_ERROR,
						BURNER_BURN_ERROR_PERMISSION,
						_("You do not have the required permissions to use this drive")));		
	}
	else if (strstr (line, "Cannot send SCSI cmd via ioctl")) {
		burner_job_error (BURNER_JOB (readom),
				   g_error_new (BURNER_BURN_ERROR,
						BURNER_BURN_ERROR_PERMISSION,
						_("You do not have the required permissions to use this drive")));
	}
	/* we scan for this error as in this case readcd returns success */
	else if (sscanf (line, "Input/output error. Error on sector %d not corrected. Total of %d error", &dummy1, &dummy2) == 2) {
		burner_job_error (BURNER_JOB (process),
				   g_error_new (BURNER_BURN_ERROR,
						BURNER_BURN_ERROR_GENERAL,
						_("An internal error occurred")));
	}
	else if (strstr (line, "No space left on device")) {
		/* This is necessary as readcd won't return an error code on exit */
		burner_job_error (BURNER_JOB (readom),
				   g_error_new (BURNER_BURN_ERROR,
						BURNER_BURN_ERROR_DISK_SPACE,
						_("The location you chose to store the image on does not have enough free space for the disc image")));
	}

	return BURNER_BURN_OK;
}

static BurnerBurnResult
burner_readom_argv_set_iso_boundary (BurnerReadom *readom,
				      GPtrArray *argv,
				      GError **error)
{
	goffset nb_blocks;
	BurnerTrack *track;
	GValue *value = NULL;
	BurnerTrackType *output = NULL;

	burner_job_get_current_track (BURNER_JOB (readom), &track);

	output = burner_track_type_new ();
	burner_job_get_output_type (BURNER_JOB (readom), output);

	burner_track_tag_lookup (track,
				  BURNER_TRACK_MEDIUM_ADDRESS_START_TAG,
				  &value);
	if (value) {
		guint64 start, end;

		/* we were given an address to start */
		start = g_value_get_uint64 (value);

		/* get the length now */
		value = NULL;
		burner_track_tag_lookup (track,
					  BURNER_TRACK_MEDIUM_ADDRESS_END_TAG,
					  &value);

		end = g_value_get_uint64 (value);

		BURNER_JOB_LOG (readom,
				 "reading from sector %lli to %lli",
				 start,
				 end);
		g_ptr_array_add (argv, g_strdup_printf ("-sectors=%"G_GINT64_FORMAT"-%"G_GINT64_FORMAT,
							start,
							end));
	}
	/* 0 means all disc, -1 problem */
	else if (burner_track_disc_get_track_num (BURNER_TRACK_DISC (track)) > 0) {
		goffset start;
		BurnerDrive *drive;
		BurnerMedium *medium;

		drive = burner_track_disc_get_drive (BURNER_TRACK_DISC (track));
		medium = burner_drive_get_medium (drive);
		burner_medium_get_track_space (medium,
						burner_track_disc_get_track_num (BURNER_TRACK_DISC (track)),
						NULL,
						&nb_blocks);
		burner_medium_get_track_address (medium,
						  burner_track_disc_get_track_num (BURNER_TRACK_DISC (track)),
						  NULL,
						  &start);

		BURNER_JOB_LOG (readom,
				 "reading %i from sector %lli to %lli",
				 burner_track_disc_get_track_num (BURNER_TRACK_DISC (track)),
				 start,
				 start + nb_blocks);
		g_ptr_array_add (argv, g_strdup_printf ("-sectors=%"G_GINT64_FORMAT"-%"G_GINT64_FORMAT,
							start,
							start + nb_blocks));
	}
	/* if it's BIN output just read the last track */
	else if (burner_track_type_get_image_format (output) == BURNER_IMAGE_FORMAT_BIN) {
		goffset start;
		BurnerDrive *drive;
		BurnerMedium *medium;

		drive = burner_track_disc_get_drive (BURNER_TRACK_DISC (track));
		medium = burner_drive_get_medium (drive);
		burner_medium_get_last_data_track_space (medium,
							  NULL,
							  &nb_blocks);
		burner_medium_get_last_data_track_address (medium,
							    NULL,
							    &start);
		BURNER_JOB_LOG (readom,
				 "reading last track from sector %"G_GINT64_FORMAT" to %"G_GINT64_FORMAT,
				 start,
				 start + nb_blocks);
		g_ptr_array_add (argv, g_strdup_printf ("-sectors=%"G_GINT64_FORMAT"-%"G_GINT64_FORMAT,
							start,
							start + nb_blocks));
	}
	else {
		burner_track_get_size (track, &nb_blocks, NULL);
		g_ptr_array_add (argv, g_strdup_printf ("-sectors=0-%"G_GINT64_FORMAT, nb_blocks));
	}

	burner_track_type_free (output);

	return BURNER_BURN_OK;
}

static BurnerBurnResult
burner_readom_get_size (BurnerReadom *self,
			 GError **error)
{
	goffset blocks;
	GValue *value = NULL;
	BurnerTrack *track = NULL;
	BurnerTrackType *output = NULL;

	output = burner_track_type_new ();
	burner_job_get_output_type (BURNER_JOB (self), output);

	if (!burner_track_type_get_has_image (output)) {
		burner_track_type_free (output);
		return BURNER_BURN_ERR;
	}

	burner_job_get_current_track (BURNER_JOB (self), &track);
	burner_track_tag_lookup (track,
				  BURNER_TRACK_MEDIUM_ADDRESS_START_TAG,
				  &value);
	if (value) {
		guint64 start, end;

		/* we were given an address to start */
		start = g_value_get_uint64 (value);

		/* get the length now */
		value = NULL;
		burner_track_tag_lookup (track,
					  BURNER_TRACK_MEDIUM_ADDRESS_END_TAG,
					  &value);

		end = g_value_get_uint64 (value);
		blocks = end - start;
	}
	else if (burner_track_disc_get_track_num (BURNER_TRACK_DISC (track)) > 0) {
		BurnerDrive *drive;
		BurnerMedium *medium;

		drive = burner_track_disc_get_drive (BURNER_TRACK_DISC (track));
		medium = burner_drive_get_medium (drive);
		burner_medium_get_track_space (medium,
						burner_track_disc_get_track_num (BURNER_TRACK_DISC (track)),
						NULL,
						&blocks);
	}
	else if (burner_track_type_get_image_format (output) == BURNER_IMAGE_FORMAT_BIN) {
		BurnerDrive *drive;
		BurnerMedium *medium;

		drive = burner_track_disc_get_drive (BURNER_TRACK_DISC (track));
		medium = burner_drive_get_medium (drive);
		burner_medium_get_last_data_track_space (medium,
							  NULL,
							  &blocks);
	}
	else
		burner_track_get_size (track, &blocks, NULL);

	if (burner_track_type_get_image_format (output) == BURNER_IMAGE_FORMAT_BIN) {
		burner_job_set_output_size_for_current_track (BURNER_JOB (self),
							       blocks,
							       blocks * 2048ULL);
	}
	else if (burner_track_type_get_image_format (output) == BURNER_IMAGE_FORMAT_CLONE) {
		burner_job_set_output_size_for_current_track (BURNER_JOB (self),
							       blocks,
							       blocks * 2448ULL);
	}
	else {
		burner_track_type_free (output);
		return BURNER_BURN_NOT_SUPPORTED;
	}

	burner_track_type_free (output);

	/* no need to go any further */
	return BURNER_BURN_NOT_RUNNING;
}

static BurnerBurnResult
burner_readom_set_argv (BurnerProcess *process,
			 GPtrArray *argv,
			 GError **error)
{
	BurnerBurnResult result = FALSE;
	BurnerTrackType *output = NULL;
	BurnerImageFormat format;
	BurnerJobAction action;
	BurnerReadom *readom;
	BurnerMedium *medium;
	BurnerDrive *drive;
	BurnerTrack *track;
	BurnerMedia media;
	gchar *outfile_arg;
	gchar *dev_str;

	readom = BURNER_READOM (process);

	/* This is a kind of shortcut */
	burner_job_get_action (BURNER_JOB (process), &action);
	if (action == BURNER_JOB_ACTION_SIZE)
		return burner_readom_get_size (readom, error);

	g_ptr_array_add (argv, g_strdup ("readom"));

	burner_job_get_current_track (BURNER_JOB (readom), &track);
	drive = burner_track_disc_get_drive (BURNER_TRACK_DISC (track));
	if (!burner_drive_get_device (drive))
		return BURNER_BURN_ERR;

	dev_str = g_strdup_printf ("dev=%s", burner_drive_get_device (drive));
	g_ptr_array_add (argv, dev_str);

	g_ptr_array_add (argv, g_strdup ("-nocorr"));

	medium = burner_drive_get_medium (drive);
	media = burner_medium_get_status (medium);

	output = burner_track_type_new ();
	burner_job_get_output_type (BURNER_JOB (readom), output);
	format = burner_track_type_get_image_format (output);
	burner_track_type_free (output);

	if ((media & BURNER_MEDIUM_DVD)
	&&   format != BURNER_IMAGE_FORMAT_BIN) {
		g_set_error (error,
			     BURNER_BURN_ERROR,
			     BURNER_BURN_ERROR_GENERAL,
			     _("An internal error occurred"));
		return BURNER_BURN_ERR;
	}

	if (format == BURNER_IMAGE_FORMAT_CLONE) {
		/* NOTE: with this option the sector size is 2448 
		 * because it is raw96 (2352+96) otherwise it is 2048  */
		g_ptr_array_add (argv, g_strdup ("-clone"));
	}
	else if (format == BURNER_IMAGE_FORMAT_BIN) {
		g_ptr_array_add (argv, g_strdup ("-noerror"));

		/* don't do it for clone since we need the entire disc */
		result = burner_readom_argv_set_iso_boundary (readom, argv, error);
		if (result != BURNER_BURN_OK)
			return result;
	}
	else
		BURNER_JOB_NOT_SUPPORTED (readom);

	if (burner_job_get_fd_out (BURNER_JOB (readom), NULL) != BURNER_BURN_OK) {
		gchar *image;

		if (format != BURNER_IMAGE_FORMAT_CLONE
		&&  format != BURNER_IMAGE_FORMAT_BIN)
			BURNER_JOB_NOT_SUPPORTED (readom);

		result = burner_job_get_image_output (BURNER_JOB (readom),
						       &image,
						       NULL);
		if (result != BURNER_BURN_OK)
			return result;

		outfile_arg = g_strdup_printf ("-f=%s", image);
		g_ptr_array_add (argv, outfile_arg);
		g_free (image);
	}
	else if (format == BURNER_IMAGE_FORMAT_BIN) {
		outfile_arg = g_strdup ("-f=-");
		g_ptr_array_add (argv, outfile_arg);
	}
	else 	/* unfortunately raw images can't be piped out */
		BURNER_JOB_NOT_SUPPORTED (readom);

	burner_job_set_use_average_rate (BURNER_JOB (process), TRUE);
	return BURNER_BURN_OK;
}

static void
burner_readom_class_init (BurnerReadomClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS(klass);
	BurnerProcessClass *process_class = BURNER_PROCESS_CLASS (klass);

	parent_class = g_type_class_peek_parent (klass);
	object_class->finalize = burner_readom_finalize;

	process_class->stderr_func = burner_readom_read_stderr;
	process_class->set_argv = burner_readom_set_argv;
}

static void
burner_readom_init (BurnerReadom *obj)
{ }

static void
burner_readom_finalize (GObject *object)
{
	G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
burner_readom_export_caps (BurnerPlugin *plugin)
{
	GSList *output;
	GSList *input;

	burner_plugin_define (plugin,
			       "readom",
	                       NULL,
			       _("Copies any disc to a disc image"),
			       "Philippe Rouquier",
			       1);

	/* that's for clone mode only The only one to copy audio */
	output = burner_caps_image_new (BURNER_PLUGIN_IO_ACCEPT_FILE,
					 BURNER_IMAGE_FORMAT_CLONE);

	input = burner_caps_disc_new (BURNER_MEDIUM_CD|
				       BURNER_MEDIUM_ROM|
				       BURNER_MEDIUM_WRITABLE|
				       BURNER_MEDIUM_REWRITABLE|
				       BURNER_MEDIUM_APPENDABLE|
				       BURNER_MEDIUM_CLOSED|
				       BURNER_MEDIUM_HAS_AUDIO|
				       BURNER_MEDIUM_HAS_DATA);

	burner_plugin_link_caps (plugin, output, input);
	g_slist_free (output);
	g_slist_free (input);

	/* that's for regular mode: it accepts the previous type of discs 
	 * plus the DVDs types as well */
	output = burner_caps_image_new (BURNER_PLUGIN_IO_ACCEPT_FILE|
					 BURNER_PLUGIN_IO_ACCEPT_PIPE,
					 BURNER_IMAGE_FORMAT_BIN);

	input = burner_caps_disc_new (BURNER_MEDIUM_CD|
				       BURNER_MEDIUM_DVD|
				       BURNER_MEDIUM_BD|
				       BURNER_MEDIUM_DUAL_L|
				       BURNER_MEDIUM_PLUS|
				       BURNER_MEDIUM_SEQUENTIAL|
				       BURNER_MEDIUM_RESTRICTED|
				       BURNER_MEDIUM_ROM|
				       BURNER_MEDIUM_WRITABLE|
				       BURNER_MEDIUM_REWRITABLE|
				       BURNER_MEDIUM_CLOSED|
				       BURNER_MEDIUM_APPENDABLE|
				       BURNER_MEDIUM_HAS_DATA);

	burner_plugin_link_caps (plugin, output, input);
	g_slist_free (output);
	g_slist_free (input);

	burner_plugin_register_group (plugin, _(CDRKIT_DESCRIPTION));
}

G_MODULE_EXPORT void
burner_plugin_check_config (BurnerPlugin *plugin)
{
	gint version [3] = { 1, 1, 0};
	burner_plugin_test_app (plugin,
	                         "readom",
	                         "--version",
	                         "readcd %*s is not what you see here. This line is only a fake for too clever\nGUIs and other frontend applications. In fact, this program is:\nreadom %d.%d.%d",
	                         version);
}
