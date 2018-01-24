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

#include "burn-job.h"
#include "burn-process.h"
#include "burner-plugin-registration.h"
#include "burn-cdrkit.h"
#include "burner-track-data.h"


#define BURNER_TYPE_GENISOIMAGE         (burner_genisoimage_get_type ())
#define BURNER_GENISOIMAGE(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), BURNER_TYPE_GENISOIMAGE, BurnerGenisoimage))
#define BURNER_GENISOIMAGE_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k), BURNER_TYPE_GENISOIMAGE, BurnerGenisoimageClass))
#define BURNER_IS_GENISOIMAGE(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), BURNER_TYPE_GENISOIMAGE))
#define BURNER_IS_GENISOIMAGE_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), BURNER_TYPE_GENISOIMAGE))
#define BURNER_GENISOIMAGE_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), BURNER_TYPE_GENISOIMAGE, BurnerGenisoimageClass))

BURNER_PLUGIN_BOILERPLATE (BurnerGenisoimage, burner_genisoimage, BURNER_TYPE_PROCESS, BurnerProcess);

struct _BurnerGenisoimagePrivate {
	guint use_utf8:1;
};
typedef struct _BurnerGenisoimagePrivate BurnerGenisoimagePrivate;

#define BURNER_GENISOIMAGE_PRIVATE(o)  (G_TYPE_INSTANCE_GET_PRIVATE ((o), BURNER_TYPE_GENISOIMAGE, BurnerGenisoimagePrivate))
static GObjectClass *parent_class = NULL;

static BurnerBurnResult
burner_genisoimage_read_isosize (BurnerProcess *process, const gchar *line)
{
	gint64 sectors;

	sectors = strtoll (line, NULL, 10);
	if (!sectors)
		return BURNER_BURN_OK;

	/* genisoimage reports blocks of 2048 bytes */
	burner_job_set_output_size_for_current_track (BURNER_JOB (process),
						       sectors,
						       (gint64) sectors * 2048ULL);
	return BURNER_BURN_OK;
}

static BurnerBurnResult
burner_genisoimage_read_stdout (BurnerProcess *process, const gchar *line)
{
	BurnerJobAction action;

	burner_job_get_action (BURNER_JOB (process), &action);
	if (action == BURNER_JOB_ACTION_SIZE)
		return burner_genisoimage_read_isosize (process, line);

	return BURNER_BURN_OK;
}

static BurnerBurnResult
burner_genisoimage_read_stderr (BurnerProcess *process, const gchar *line)
{
	gchar fraction_str [7] = { 0, };
	BurnerGenisoimage *genisoimage;

	genisoimage = BURNER_GENISOIMAGE (process);

	if (strstr (line, "estimate finish")
	&&  sscanf (line, "%6c%% done, estimate finish", fraction_str) == 1) {
		gdouble fraction;
	
		fraction = g_strtod (fraction_str, NULL) / (gdouble) 100.0;
		burner_job_set_progress (BURNER_JOB (genisoimage), fraction);
		burner_job_start_progress (BURNER_JOB (process), FALSE);
	}
	else if (strstr (line, "Input/output error. Read error on old image")) {
		burner_job_error (BURNER_JOB (process), 
				   g_error_new_literal (BURNER_BURN_ERROR,
							BURNER_BURN_ERROR_IMAGE_LAST_SESSION,
							_("Last session import failed")));
	}
	else if (strstr (line, "Unable to sort directory")) {
		burner_job_error (BURNER_JOB (process), 
				   g_error_new_literal (BURNER_BURN_ERROR,
							BURNER_BURN_ERROR_WRITE_IMAGE,
							_("An image could not be created")));
	}
	else if (strstr (line, "have the same joliet name")
	     ||  strstr (line, "Joliet tree sort failed.")) {
		burner_job_error (BURNER_JOB (process), 
				   g_error_new_literal (BURNER_BURN_ERROR,
							BURNER_BURN_ERROR_IMAGE_JOLIET,
							_("An image could not be created")));
	}
	else if (strstr (line, "Use genisoimage -help")) {
		burner_job_error (BURNER_JOB (process), 
				   g_error_new_literal (BURNER_BURN_ERROR,
							BURNER_BURN_ERROR_GENERAL,
							_("This version of genisoimage is not supported")));
	}
/*	else if ((pos =  strstr (line,"genisoimage: Permission denied. "))) {
		int res = FALSE;
		gboolean isdir = FALSE;
		char *path = NULL;

		pos += strlen ("genisoimage: Permission denied. ");
		if (!strncmp (pos, "Unable to open directory ", 24)) {
			isdir = TRUE;

			pos += strlen ("Unable to open directory ");
			path = g_strdup (pos);
			path[strlen (path) - 1] = 0;
		}
		else if (!strncmp (pos, "File ", 5)) {
			char *end;

			isdir = FALSE;
			pos += strlen ("File ");
			end = strstr (pos, " is not readable - ignoring");
			if (end)
				path = g_strndup (pos, end - pos);
		}
		else
			return TRUE;

		res = burner_genisoimage_base_ask_unreadable_file (BURNER_GENISOIMAGE_BASE (process),
								path,
								isdir);
		if (!res) {
			g_free (path);

			burner_job_progress_changed (BURNER_JOB (process), 1.0, -1);
			burner_job_cancel (BURNER_JOB (process), FALSE);
			return FALSE;
		}
	}*/
	else if (strstr (line, "Incorrectly encoded string")) {
		burner_job_error (BURNER_JOB (process),
				   g_error_new_literal (BURNER_BURN_ERROR,
							BURNER_BURN_ERROR_INPUT_INVALID,
							_("Some files have invalid filenames")));
	}
	else if (strstr (line, "Unknown charset")) {
		burner_job_error (BURNER_JOB (process),
				   g_error_new_literal (BURNER_BURN_ERROR,
							BURNER_BURN_ERROR_INPUT_INVALID,
							_("Unknown character encoding")));
	}
	else if (strstr (line, "No space left on device")) {
		burner_job_error (BURNER_JOB (process),
				   g_error_new_literal (BURNER_BURN_ERROR,
							BURNER_BURN_ERROR_DISK_SPACE,
							_("There is no space left on the device")));

	}
	else if (strstr (line, "Unable to open disc image file")) {
		burner_job_error (BURNER_JOB (process),
				   g_error_new_literal (BURNER_BURN_ERROR,
							BURNER_BURN_ERROR_PERMISSION,
							_("You do not have the required permission to write at this location")));

	}
	else if (strstr (line, "Value too large for defined data type")) {
		burner_job_error (BURNER_JOB (process),
				   g_error_new_literal (BURNER_BURN_ERROR,
							BURNER_BURN_ERROR_MEDIUM_SPACE,
							_("Not enough space available on the disc")));
	}

	/** REMINDER: these should not be necessary

	else if (strstr (line, "Resource temporarily unavailable")) {
		burner_job_error (BURNER_JOB (process),
				   g_error_new_literal (BURNER_BURN_ERROR,
							BURNER_BURN_ERROR_INPUT,
							_("Data could not be written")));
	}
	else if (strstr (line, "Bad file descriptor.")) {
		burner_job_error (BURNER_JOB (process),
				   g_error_new_literal (BURNER_BURN_ERROR,
							BURNER_BURN_ERROR_INPUT,
							_("Internal error: bad file descriptor")));
	}

	**/

	return BURNER_BURN_OK;
}

static BurnerBurnResult
burner_genisoimage_set_argv_image (BurnerGenisoimage *genisoimage,
				    GPtrArray *argv,
				    GError **error)
{
	gchar *label = NULL;
	BurnerTrack *track;
	BurnerBurnFlag flags;
	gchar *emptydir = NULL;
	gchar *videodir = NULL;
	BurnerImageFS image_fs;
	BurnerBurnResult result;
	BurnerJobAction action;
	gchar *grafts_path = NULL;
	gchar *excluded_path = NULL;

	/* set argv */
	g_ptr_array_add (argv, g_strdup ("-r"));

	result = burner_job_get_current_track (BURNER_JOB (genisoimage), &track);
	if (result != BURNER_BURN_OK)
		BURNER_JOB_NOT_READY (genisoimage);

	image_fs = burner_track_data_get_fs (BURNER_TRACK_DATA (track));
	if (image_fs & BURNER_IMAGE_FS_JOLIET)
		g_ptr_array_add (argv, g_strdup ("-J"));

	if ((image_fs & BURNER_IMAGE_FS_ISO)
	&&  (image_fs & BURNER_IMAGE_ISO_FS_LEVEL_3)) {
		g_ptr_array_add (argv, g_strdup ("-iso-level"));
		g_ptr_array_add (argv, g_strdup ("3"));

		/* NOTE: the following is specific to genisoimage
		 * It allows one to burn files over 4 GiB */
		g_ptr_array_add (argv, g_strdup ("-allow-limited-size"));
	}

	if (image_fs & BURNER_IMAGE_FS_UDF)
		g_ptr_array_add (argv, g_strdup ("-udf"));

	if (image_fs & BURNER_IMAGE_FS_VIDEO) {
		g_ptr_array_add (argv, g_strdup ("-dvd-video"));

		result = burner_job_get_tmp_dir (BURNER_JOB (genisoimage),
						  &videodir,
						  error);
		if (result != BURNER_BURN_OK)
			return result;
	}

	g_ptr_array_add (argv, g_strdup ("-graft-points"));

	if (image_fs & BURNER_IMAGE_ISO_FS_DEEP_DIRECTORY)
		g_ptr_array_add (argv, g_strdup ("-D"));	// This is dangerous the manual says but apparently it works well

	result = burner_job_get_tmp_file (BURNER_JOB (genisoimage),
					   NULL,
					   &grafts_path,
					   error);
	if (result != BURNER_BURN_OK) {
		g_free (videodir);
		return result;
	}

	result = burner_job_get_tmp_file (BURNER_JOB (genisoimage),
					   NULL,
					   &excluded_path,
					   error);
	if (result != BURNER_BURN_OK) {
		g_free (videodir);
		g_free (grafts_path);
		return result;
	}

	result = burner_job_get_tmp_dir (BURNER_JOB (genisoimage),
					  &emptydir,
					  error);
	if (result != BURNER_BURN_OK) {
		g_free (videodir);
		g_free (grafts_path);
		g_free (excluded_path);
		return result;
	}

	result = burner_track_data_write_to_paths (BURNER_TRACK_DATA (track),
	                                            grafts_path,
	                                            excluded_path,
	                                            emptydir,
	                                            videodir,
	                                            error);
	g_free (emptydir);

	if (result != BURNER_BURN_OK) {
		g_free (videodir);
		g_free (grafts_path);
		g_free (excluded_path);
		return result;
	}

	g_ptr_array_add (argv, g_strdup ("-path-list"));
	g_ptr_array_add (argv, grafts_path);

	g_ptr_array_add (argv, g_strdup ("-exclude-list"));
	g_ptr_array_add (argv, excluded_path);

	burner_job_get_data_label (BURNER_JOB (genisoimage), &label);
	if (label) {
		g_ptr_array_add (argv, g_strdup ("-V"));
		g_ptr_array_add (argv, label);
	}

	g_ptr_array_add (argv, g_strdup ("-A"));
	g_ptr_array_add (argv, g_strdup_printf ("Burner-%i.%i.%i",
						BURNER_MAJOR_VERSION,
						BURNER_MINOR_VERSION,
						BURNER_SUB));
	
	g_ptr_array_add (argv, g_strdup ("-sysid"));
	g_ptr_array_add (argv, g_strdup ("LINUX"));
	
	/* FIXME! -sort is an interesting option allowing to decide where the 
	* files are written on the disc and therefore to optimize later reading */
	/* FIXME: -hidden --hidden-list -hide-jolie -hide-joliet-list will allow one to hide
	* some files when we will display the contents of a disc we will want to merge */
	/* FIXME: support preparer publisher options */

	burner_job_get_flags (BURNER_JOB (genisoimage), &flags);
	if (flags & (BURNER_BURN_FLAG_APPEND|BURNER_BURN_FLAG_MERGE)) {
		goffset last_session = 0, next_wr_add = 0;
		gchar *startpoint = NULL;

		burner_job_get_last_session_address (BURNER_JOB (genisoimage), &last_session);
		burner_job_get_next_writable_address (BURNER_JOB (genisoimage), &next_wr_add);
		if (last_session == -1 || next_wr_add == -1) {
			g_free (videodir);
			BURNER_JOB_LOG (genisoimage, "Failed to get the start point of the track. Make sure the media allow one to add files (it is not closed)");
			g_set_error (error,
				     BURNER_BURN_ERROR,
				     BURNER_BURN_ERROR_GENERAL,
				     _("An internal error occurred"));
			return BURNER_BURN_ERR;
		}

		startpoint = g_strdup_printf ("%"G_GINT64_FORMAT",%"G_GINT64_FORMAT,
					      last_session,
					      next_wr_add);

		g_ptr_array_add (argv, g_strdup ("-C"));
		g_ptr_array_add (argv, startpoint);

		if (flags & BURNER_BURN_FLAG_MERGE) {
		        gchar *device = NULL;

			g_ptr_array_add (argv, g_strdup ("-M"));

			burner_job_get_device (BURNER_JOB (genisoimage), &device);
			g_ptr_array_add (argv, device);
		}
	}

	burner_job_get_action (BURNER_JOB (genisoimage), &action);
	if (action == BURNER_JOB_ACTION_SIZE) {
		g_ptr_array_add (argv, g_strdup ("-quiet"));
		g_ptr_array_add (argv, g_strdup ("-print-size"));

		burner_job_set_current_action (BURNER_JOB (genisoimage),
						BURNER_BURN_ACTION_GETTING_SIZE,
						NULL,
						FALSE);
		burner_job_start_progress (BURNER_JOB (genisoimage), FALSE);

		if (videodir) {
			g_ptr_array_add (argv, g_strdup ("-f"));
			g_ptr_array_add (argv, videodir);
		}

		return BURNER_BURN_OK;
	}

	if (burner_job_get_fd_out (BURNER_JOB (genisoimage), NULL) != BURNER_BURN_OK) {
		gchar *output = NULL;

		result = burner_job_get_image_output (BURNER_JOB (genisoimage),
						      &output,
						       NULL);
		if (result != BURNER_BURN_OK) {
			g_free (videodir);
			return result;
		}

		g_ptr_array_add (argv, g_strdup ("-o"));
		g_ptr_array_add (argv, output);
	}

	if (videodir) {
		g_ptr_array_add (argv, g_strdup ("-f"));
		g_ptr_array_add (argv, videodir);
	}

	burner_job_set_current_action (BURNER_JOB (genisoimage),
					BURNER_BURN_ACTION_CREATING_IMAGE,
					NULL,
					FALSE);

	return BURNER_BURN_OK;
}

static BurnerBurnResult
burner_genisoimage_set_argv (BurnerProcess *process,
			      GPtrArray *argv,
			      GError **error)
{
	BurnerGenisoimagePrivate *priv;
	BurnerGenisoimage *genisoimage;
	BurnerBurnResult result;
	BurnerJobAction action;
	gchar *prog_name;

	genisoimage = BURNER_GENISOIMAGE (process);
	priv = BURNER_GENISOIMAGE_PRIVATE (process);

	prog_name = g_find_program_in_path ("genisoimage");
	if (prog_name && g_file_test (prog_name, G_FILE_TEST_IS_EXECUTABLE))
		g_ptr_array_add (argv, prog_name);
	else
		g_ptr_array_add (argv, g_strdup ("genisoimage"));

	if (priv->use_utf8) {
		g_ptr_array_add (argv, g_strdup ("-input-charset"));
		g_ptr_array_add (argv, g_strdup ("utf8"));
	}

	burner_job_get_action (BURNER_JOB (genisoimage), &action);
	if (action == BURNER_JOB_ACTION_SIZE)
		result = burner_genisoimage_set_argv_image (genisoimage, argv, error);
	else if (action == BURNER_JOB_ACTION_IMAGE)
		result = burner_genisoimage_set_argv_image (genisoimage, argv, error);
	else
		BURNER_JOB_NOT_SUPPORTED (genisoimage);

	return result;
}

static void
burner_genisoimage_class_init (BurnerGenisoimageClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);
	BurnerProcessClass *process_class = BURNER_PROCESS_CLASS (klass);

	g_type_class_add_private (klass, sizeof (BurnerGenisoimagePrivate));

	parent_class = g_type_class_peek_parent(klass);
	object_class->finalize = burner_genisoimage_finalize;

	process_class->stdout_func = burner_genisoimage_read_stdout;
	process_class->stderr_func = burner_genisoimage_read_stderr;
	process_class->set_argv = burner_genisoimage_set_argv;
}

static void
burner_genisoimage_init (BurnerGenisoimage *obj)
{
	BurnerGenisoimagePrivate *priv;
	gchar *standard_error;
	gboolean res;

	priv = BURNER_GENISOIMAGE_PRIVATE (obj);

	/* this code used to be ncb_genisoimage_supports_utf8 */
	res = g_spawn_command_line_sync ("genisoimage -input-charset utf8",
					 NULL,
					 &standard_error,
					 NULL,
					 NULL);

	if (res && !g_strrstr (standard_error, "Unknown charset"))
		priv->use_utf8 = TRUE;
	else
		priv->use_utf8 = FALSE;

	g_free (standard_error);
}

static void
burner_genisoimage_finalize (GObject *object)
{
	G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
burner_genisoimage_export_caps (BurnerPlugin *plugin)
{
	GSList *output;
	GSList *input;

	burner_plugin_define (plugin,
			       "genisoimage",
	                       NULL,
			       _("Creates disc images from a file selection"),
			       "Philippe Rouquier",
			       1);

	burner_plugin_set_flags (plugin,
				  BURNER_MEDIUM_CDR|
				  BURNER_MEDIUM_CDRW|
				  BURNER_MEDIUM_DVDR|
				  BURNER_MEDIUM_DVDRW|
				  BURNER_MEDIUM_DUAL_L|
				  BURNER_MEDIUM_DVDR_PLUS|
				  BURNER_MEDIUM_APPENDABLE|
				  BURNER_MEDIUM_HAS_AUDIO|
				  BURNER_MEDIUM_HAS_DATA,
				  BURNER_BURN_FLAG_APPEND|
				  BURNER_BURN_FLAG_MERGE,
				  BURNER_BURN_FLAG_NONE);

	burner_plugin_set_flags (plugin,
				  BURNER_MEDIUM_DUAL_L|
				  BURNER_MEDIUM_DVDRW_PLUS|
				  BURNER_MEDIUM_RESTRICTED|
				  BURNER_MEDIUM_APPENDABLE|
				  BURNER_MEDIUM_CLOSED|
				  BURNER_MEDIUM_HAS_DATA,
				  BURNER_BURN_FLAG_APPEND|
				  BURNER_BURN_FLAG_MERGE,
				  BURNER_BURN_FLAG_NONE);

	/* Caps */
	output = burner_caps_image_new (BURNER_PLUGIN_IO_ACCEPT_FILE|
					 BURNER_PLUGIN_IO_ACCEPT_PIPE,
					 BURNER_IMAGE_FORMAT_BIN);

	input = burner_caps_data_new (BURNER_IMAGE_FS_ISO|
				       BURNER_IMAGE_FS_UDF|
				       BURNER_IMAGE_ISO_FS_LEVEL_3|
				       BURNER_IMAGE_ISO_FS_DEEP_DIRECTORY|
				       BURNER_IMAGE_FS_JOLIET|
				       BURNER_IMAGE_FS_VIDEO);
	burner_plugin_link_caps (plugin, output, input);
	g_slist_free (input);

	input = burner_caps_data_new (BURNER_IMAGE_FS_ISO|
				       BURNER_IMAGE_ISO_FS_LEVEL_3|
				       BURNER_IMAGE_ISO_FS_DEEP_DIRECTORY|
				       BURNER_IMAGE_FS_SYMLINK);
	burner_plugin_link_caps (plugin, output, input);
	g_slist_free (input);

	g_slist_free (output);

	burner_plugin_register_group (plugin, _(CDRKIT_DESCRIPTION));
}

G_MODULE_EXPORT void
burner_plugin_check_config (BurnerPlugin *plugin)
{
	gint version [3] = { 1, 1, 0};
	burner_plugin_test_app (plugin,
	                         "genisoimage",
	                         "--version",
	                         "genisoimage %d.%d.%d (Linux)",
	                         version);
}
