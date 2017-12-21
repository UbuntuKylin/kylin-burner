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

#include <stdio.h>
#include <string.h>

#include <glib.h>
#include <glib-object.h>
#include <glib/gi18n-lib.h>

#include <gmodule.h>

#include "burn-basics.h"
#include "burner-plugin.h"
#include "burner-plugin-registration.h"
#include "burn-job.h"
#include "burn-process.h"
#include "burner-medium.h"
#include "burn-growisofs-common.h"


#define BURNER_TYPE_DVD_RW_FORMAT         (burner_dvd_rw_format_get_type ())
#define BURNER_DVD_RW_FORMAT(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), BURNER_TYPE_DVD_RW_FORMAT, BurnerDvdRwFormat))
#define BURNER_DVD_RW_FORMAT_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k), BURNER_TYPE_DVD_RW_FORMAT, BurnerDvdRwFormatClass))
#define BURNER_IS_DVD_RW_FORMAT(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), BURNER_TYPE_DVD_RW_FORMAT))
#define BURNER_IS_DVD_RW_FORMAT_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), BURNER_TYPE_DVD_RW_FORMAT))
#define BURNER_DVD_RW_FORMAT_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), BURNER_TYPE_DVD_RW_FORMAT, BurnerDvdRwFormatClass))

BURNER_PLUGIN_BOILERPLATE (BurnerDvdRwFormat, burner_dvd_rw_format, BURNER_TYPE_PROCESS, BurnerProcess);

static GObjectClass *parent_class = NULL;

static BurnerBurnResult
burner_dvd_rw_format_read_stderr (BurnerProcess *process, const gchar *line)
{
	int perc_1 = 0, perc_2 = 0;
	float percent;

	if (strstr (line, "unable to proceed with format")
	||  strstr (line, "media is not blank")
	||  strstr (line, "media is already formatted")
	||  strstr (line, "you have the option to re-run")) {
		burner_job_error (BURNER_JOB (process),
				   g_error_new (BURNER_BURN_ERROR,
						BURNER_BURN_ERROR_MEDIUM_INVALID,
						_("The disc is not supported")));
		return BURNER_BURN_OK;
	}
	else if (strstr (line, "unable to umount")) {
		burner_job_error (BURNER_JOB (process),
				   g_error_new (BURNER_BURN_ERROR,
						BURNER_BURN_ERROR_DRIVE_BUSY,
						_("The drive is busy")));
		return BURNER_BURN_OK;
	}

	if ((sscanf (line, "* blanking %d.%1d%%,", &perc_1, &perc_2) == 2)
	||  (sscanf (line, "* formatting %d.%1d%%,", &perc_1, &perc_2) == 2)
	||  (sscanf (line, "* relocating lead-out %d.%1d%%,", &perc_1, &perc_2) == 2))
		burner_job_set_dangerous (BURNER_JOB (process), TRUE);
	else 
		sscanf (line, "%d.%1d%%", &perc_1, &perc_2);

	percent = (float) perc_1 / 100.0 + (float) perc_2 / 1000.0;
	if (percent) {
		burner_job_start_progress (BURNER_JOB (process), FALSE);
		burner_job_set_progress (BURNER_JOB (process), percent);
	}

	return BURNER_BURN_OK;
}

static BurnerBurnResult
burner_dvd_rw_format_set_argv (BurnerProcess *process,
				GPtrArray *argv,
				GError **error)
{
	BurnerMedia media;
	BurnerBurnFlag flags;
	gchar *device;

	g_ptr_array_add (argv, g_strdup ("dvd+rw-format"));

	/* undocumented option to show progress */
	g_ptr_array_add (argv, g_strdup ("-gui"));

	burner_job_get_media (BURNER_JOB (process), &media);
	burner_job_get_flags (BURNER_JOB (process), &flags);
        if (!BURNER_MEDIUM_IS (media, BURNER_MEDIUM_BDRE)
	&&  !BURNER_MEDIUM_IS (media, BURNER_MEDIUM_DVDRW_PLUS)
	&&  !BURNER_MEDIUM_IS (media, BURNER_MEDIUM_DVDRW_RESTRICTED)
	&&  (flags & BURNER_BURN_FLAG_FAST_BLANK)) {
		gchar *blank_str;

		/* This creates a sequential DVD-RW */
		blank_str = g_strdup_printf ("-blank%s",
					     (flags & BURNER_BURN_FLAG_FAST_BLANK) ? "" : "=full");
		g_ptr_array_add (argv, blank_str);
	}
	else {
		gchar *format_str;

		/* This creates a restricted overwrite DVD-RW or reformat a + */
		format_str = g_strdup ("-force");
		g_ptr_array_add (argv, format_str);
	}

	burner_job_get_device (BURNER_JOB (process), &device);
	g_ptr_array_add (argv, device);

	burner_job_set_current_action (BURNER_JOB (process),
					BURNER_BURN_ACTION_BLANKING,
					NULL,
					FALSE);
	return BURNER_BURN_OK;
}

static void
burner_dvd_rw_format_class_init (BurnerDvdRwFormatClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);
	BurnerProcessClass *process_class = BURNER_PROCESS_CLASS (klass);

	parent_class = g_type_class_peek_parent(klass);
	object_class->finalize = burner_dvd_rw_format_finalize;

	process_class->set_argv = burner_dvd_rw_format_set_argv;
	process_class->stderr_func = burner_dvd_rw_format_read_stderr;
	process_class->post = burner_job_finished_session;
}

static void
burner_dvd_rw_format_init (BurnerDvdRwFormat *obj)
{ }

static void
burner_dvd_rw_format_finalize (GObject *object)
{
	G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
burner_dvd_rw_format_export_caps (BurnerPlugin *plugin)
{
	/* NOTE: sequential and restricted are added later on demand */
	const BurnerMedia media = BURNER_MEDIUM_DVD|
				   BURNER_MEDIUM_DUAL_L|
				   BURNER_MEDIUM_REWRITABLE|
				   BURNER_MEDIUM_APPENDABLE|
				   BURNER_MEDIUM_CLOSED|
				   BURNER_MEDIUM_HAS_DATA|
				   BURNER_MEDIUM_UNFORMATTED|
				   BURNER_MEDIUM_BLANK;
	GSList *output;

	burner_plugin_define (plugin,
			       "dvd-rw-format",
	                       NULL,
			       _("Blanks and formats rewritable DVDs and BDs"),
			       "Philippe Rouquier",
			       4);

	output = burner_caps_disc_new (media|
					BURNER_MEDIUM_BDRE|
					BURNER_MEDIUM_PLUS|
					BURNER_MEDIUM_RESTRICTED|
					BURNER_MEDIUM_SEQUENTIAL);
	burner_plugin_blank_caps (plugin, output);
	g_slist_free (output);

	burner_plugin_set_blank_flags (plugin,
					media|
					BURNER_MEDIUM_BDRE|
					BURNER_MEDIUM_PLUS|
					BURNER_MEDIUM_RESTRICTED,
					BURNER_BURN_FLAG_NOGRACE,
					BURNER_BURN_FLAG_NONE);
	burner_plugin_set_blank_flags (plugin,
					media|
					BURNER_MEDIUM_SEQUENTIAL,
					BURNER_BURN_FLAG_NOGRACE|
					BURNER_BURN_FLAG_FAST_BLANK,
					BURNER_BURN_FLAG_NONE);

	burner_plugin_register_group (plugin, _(GROWISOFS_DESCRIPTION));
}

G_MODULE_EXPORT void
burner_plugin_check_config (BurnerPlugin *plugin)
{
	gint version [3] = { 5, 0, -1};
	burner_plugin_test_app (plugin,
	                         "dvd+rw-format",
	                         "-v",
	                         "* BD/DVD±RW/-RAM format utility by <appro@fy.chalmers.se>, version %d.%d",
	                         version);
}
