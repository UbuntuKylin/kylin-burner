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
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>

#include <glib.h>
#include <glib-object.h>
#include <glib/gi18n-lib.h>
#include <glib/gstdio.h>
#include <gmodule.h>

#include "burner-units.h"

#include "burn-job.h"
#include "burn-process.h"
#include "burner-plugin-registration.h"
#include "burn-cdrtools.h"

#include "burner-tags.h"
#include "burner-track-image.h"
#include "burner-track-stream.h"


#define BURNER_TYPE_CD_RECORD         (burner_cdrecord_get_type ())
#define BURNER_CD_RECORD(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), BURNER_TYPE_CD_RECORD, BurnerCDRecord))
#define BURNER_CD_RECORD_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k), BURNER_TYPE_CD_RECORD, BurnerCDRecordClass))
#define BURNER_IS_CD_RECORD(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), BURNER_TYPE_CD_RECORD))
#define BURNER_IS_CD_RECORD_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), BURNER_TYPE_CD_RECORD))
#define BURNER_CD_RECORD_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), BURNER_TYPE_CD_RECORD, BurnerCDRecordClass))

BURNER_PLUGIN_BOILERPLATE (BurnerCDRecord, burner_cdrecord, BURNER_TYPE_PROCESS, BurnerProcess);

struct _BurnerCDRecordPrivate {
	goffset current_track_end_pos;
	goffset current_track_written;

	gint current_track_num;
	gint track_count;

	gint minbuf;

	GSList *infs;

	guint immediate:1;
};
typedef struct _BurnerCDRecordPrivate BurnerCDRecordPrivate;
#define BURNER_CD_RECORD_PRIVATE(o)  (G_TYPE_INSTANCE_GET_PRIVATE ((o), BURNER_TYPE_CD_RECORD, BurnerCDRecordPrivate))

static GObjectClass *parent_class = NULL;

#define BURNER_SCHEMA_CONFIG		"org.gnome.burner.config"
#define BURNER_KEY_IMMEDIATE_FLAG      "immed-flag"
#define BURNER_KEY_MINBUF_VALUE	"minbuf-value"

static BurnerBurnResult
burner_cdrecord_stderr_read (BurnerProcess *process, const gchar *line)
{
	BurnerBurnFlag flags;

	burner_job_get_flags (BURNER_JOB (process), &flags);

	if (strstr (line, "Cannot open SCSI driver.")
	||  strstr (line, "Operation not permitted. Cannot send SCSI cmd via ioctl")
	||  strstr (line, "Cannot open or use SCSI driver")) {
		burner_job_error (BURNER_JOB (process),
				   g_error_new (BURNER_BURN_ERROR,
						BURNER_BURN_ERROR_PERMISSION,
						_("You do not have the required permissions to use this drive")));
	}
	else if (!(flags & BURNER_BURN_FLAG_OVERBURN)
	     &&  strstr (line, "Data may not fit on current disk")) {
		/* we don't error out if overburn was chosen */
		burner_job_error (BURNER_JOB (process),
				   g_error_new (BURNER_BURN_ERROR,
						BURNER_BURN_ERROR_MEDIUM_SPACE,
						_("Not enough space available on the disc")));
	}
	else if (strstr (line ,"cdrecord: A write error occurred")
	     ||  strstr (line, "Could not write Lead-in")
	     ||  strstr (line, "Cannot fixate disk")) {
		burner_job_error (BURNER_JOB (process),
				   g_error_new (BURNER_BURN_ERROR,
						BURNER_BURN_ERROR_WRITE_MEDIUM,
						_("An error occurred while writing to disc")));
	}
	else if (strstr (line, "DMA speed too slow") != NULL) {
		burner_job_error (BURNER_JOB (process),
				   g_error_new (BURNER_BURN_ERROR,
						BURNER_BURN_ERROR_SLOW_DMA,
						_("The system is too slow to write the disc at this speed. Try a lower speed")));
	}
	else if (strstr (line, "Device or resource busy")) {
		if (!strstr (line, "retrying in")) {
			burner_job_error (BURNER_JOB (process),
					   g_error_new (BURNER_BURN_ERROR,
							BURNER_BURN_ERROR_DRIVE_BUSY,
							_("The drive is busy")));
		}
	}
	else if (strstr (line, "Illegal write mode for this drive")) {
		/* NOTE : when it happened I had to unlock the
		 * drive with cdrdao and eject it. Should we ? */
		burner_job_error (BURNER_JOB (process),
				   g_error_new (BURNER_BURN_ERROR,
						BURNER_BURN_ERROR_DRIVE_BUSY,
						_("The drive is busy")));
	}
	else if (strstr (line, "Probably trying to use ultra high speed+ medium on improper writer")) {
		/* Set a deferred error as this message tends to indicate a failure */
		burner_process_deferred_error (process,
						g_error_new (BURNER_BURN_ERROR,
							     BURNER_BURN_ERROR_MEDIUM_INVALID,
							     _("The disc is not supported")));
	}

	/* REMINDER: these should not be necessary as we checked that already */
	/**
	else if (strstr (line, "cannot write medium - incompatible format") != NULL) {
		burner_job_error (BURNER_JOB (process),
				   g_error_new (BURNER_BURN_ERROR,
						BURNER_BURN_ERROR_INPUT_INVALID,
						_("The image does not seem to be a proper iso9660 file system")));
	}
	else if (strstr (line, "This means that we are checking recorded media.") != NULL) {
	**/	/* NOTE: defer the consequence of this error as it is not always
		 * fatal. So send a warning but don't stop the process. */
	/**	burner_process_deferred_error (process,
						g_error_new (BURNER_BURN_ERROR,
							     BURNER_BURN_ERROR_MEDIUM_INVALID,
							     _("The disc is already burnt")));
	}
	else if (strstr (line, "Cannot blank disk, aborting.") != NULL) {
		burner_job_error (BURNER_JOB (process),
				   g_error_new (BURNER_BURN_ERROR,
						BURNER_BURN_ERROR_MEDIUM_INVALID,
						_("The disc could not be blanked")));
	}
	else if (strstr (line, "Bad audio track size")) {
		burner_job_error (BURNER_JOB (process),
				   g_error_new (BURNER_BURN_ERROR,
						BURNER_BURN_ERROR_GENERAL,
						_("The audio tracks are too short or not a multiple of 2352")));
	}
	else if (strstr (line, "cdrecord: No such file or directory. Cannot open")
	     ||  strstr (line, "No tracks specified. Need at least one.")) {
		burner_job_error (BURNER_JOB (process),
				   g_error_new (BURNER_BURN_ERROR,
						BURNER_BURN_ERROR_INPUT,
						_("The image file cannot be found")));
	}
	else if (strstr (line, "Inappropriate audio coding")) {
		burner_job_error (BURNER_JOB (process),
				   g_error_new (BURNER_BURN_ERROR,
						BURNER_BURN_ERROR_INPUT_INVALID,
						_("All audio files must be stereo, 16-bit digital audio with 44100Hz samples")));
	}
	else if (strstr (line, "No disk / Wrong disk!") != NULL) {
		burner_job_error (BURNER_JOB (process),
				   g_error_new (BURNER_BURN_ERROR,
						BURNER_BURN_ERROR_MEDIA_NONE,
						_("There seems to be no disc in the drive")));
	}

	**/

	/** For these we'd rather have a message saying "cdrecord failed"
	 *  as an internal error occurred says nothing/even less
	else if (strstr (line, "Bad file descriptor. read error on input file")
	     ||  strstr (line, "Input buffer error, aborting")) {
		burner_job_error (BURNER_JOB (process),
				   g_error_new (BURNER_BURN_ERROR,
						BURNER_BURN_ERROR_GENERAL,
						_("An internal error occurred")));
	}

	**/

	return BURNER_BURN_OK;
}

static void
burner_cdrecord_compute (BurnerCDRecord *cdrecord,
			  goffset mb_written,
			  goffset mb_total,
			  goffset track_num)
{
	gboolean track_num_changed = FALSE;
	BurnerCDRecordPrivate *priv;
	gchar *action_string;
	gchar *track_num_str;
	goffset this_remain;
	goffset bytes;
	goffset total;

	priv = BURNER_CD_RECORD_PRIVATE (cdrecord);
	if (mb_total <= 0)
		return;

	total = mb_total * (goffset) 1048576LL;

	if (track_num > priv->current_track_num) {
		track_num_changed = TRUE;
		priv->current_track_num = track_num;
		priv->current_track_end_pos += mb_total * (goffset) 1048576LL;
	}

	this_remain = (mb_total - mb_written) * (goffset) 1048576LL;
	bytes = (total - priv->current_track_end_pos) + this_remain;
	burner_job_set_written_session (BURNER_JOB (cdrecord), total - bytes);

	track_num_str = g_strdup_printf ("%02" G_GOFFSET_FORMAT, track_num);

	/* Translators: %s is the number of the track */
	action_string = g_strdup_printf (_("Writing track %s"), track_num_str);
	g_free (track_num_str);

	burner_job_set_current_action (BURNER_JOB (cdrecord),
					BURNER_BURN_ACTION_RECORDING,
					action_string,
					track_num_changed);
	g_free (action_string);
}

static void
burner_cdrecord_set_rate (BurnerProcess *process,
                           int speed_1,
                           int speed_2)
{
	gdouble current_rate = -1.0;
	BurnerMedia media;

	if (burner_job_get_media (BURNER_JOB (process), &media) != BURNER_BURN_OK)
		return;

	if (BURNER_MEDIUM_IS (media, BURNER_MEDIUM_CD))
		current_rate = (gdouble) ((gdouble) speed_1 +
			       (gdouble) speed_2 / 10.0) *
			       (gdouble) CD_RATE;
	else if (BURNER_MEDIUM_IS (media, BURNER_MEDIUM_DVD))
		current_rate = (gdouble) ((gdouble) speed_1 +
			       (gdouble) speed_2 / 10.0) *
			       (gdouble) DVD_RATE;
	else if (BURNER_MEDIUM_IS (media, BURNER_MEDIUM_BD))
		current_rate = (gdouble) ((gdouble) speed_1 +
			       (gdouble) speed_2 / 10.0) *
			       (gdouble) BD_RATE;

	burner_job_set_rate (BURNER_JOB (process), current_rate);
}

static BurnerBurnResult
burner_cdrecord_stdout_read (BurnerProcess *process, const gchar *line)
{
	guint track;
	guint speed_1, speed_2;
	BurnerCDRecord *cdrecord;
	BurnerCDRecordPrivate *priv;
	int mb_written = 0, mb_total = 0, fifo = 0, buf = 0;

	cdrecord = BURNER_CD_RECORD (process);
	priv = BURNER_CD_RECORD_PRIVATE (cdrecord);

	if (sscanf (line, "Track %2u: %d of %d MB written (fifo %d%%) [buf %d%%] %d.%dx.",
		    &track, &mb_written, &mb_total, &fifo, &buf, &speed_1, &speed_2) == 7 ||
	    /* This is for DVD+R */
	    sscanf (line, "Track %2u:    %d of %d MB written (fifo  %d%%) [buf  %d%%] |%*s  %*s|   %d.%dx.",
	            &track, &mb_written, &mb_total, &fifo, &buf, &speed_1, &speed_2) == 7) {

		burner_cdrecord_set_rate (process, speed_1, speed_2);
		priv->current_track_written = (goffset) mb_written * (goffset) 1048576LL;
		burner_cdrecord_compute (cdrecord,
					  mb_written,
					  mb_total,
					  track);

		burner_job_start_progress (BURNER_JOB (cdrecord), FALSE);
	} 
	else if (sscanf (line, "Track %2u:    %d MB written (fifo %d%%) [buf  %d%%]  %d.%dx.",
			 &track, &mb_written, &fifo, &buf, &speed_1, &speed_2) == 6 ||
	         sscanf (line, "Track %2u:    %d MB written (fifo %d%%) [buf  %d%%] |%*s  %*s|   %d.%dx.",
			 &track, &mb_written, &fifo, &buf, &speed_1, &speed_2) == 6) {

				 burner_cdrecord_set_rate (process, speed_1, speed_2);
		priv->current_track_written = (goffset) mb_written * (goffset) 1048576LL;
		if (burner_job_get_fd_in (BURNER_JOB (cdrecord), NULL) == BURNER_BURN_OK) {
			goffset bytes = 0;

			/* we must ask the imager what is the total size */
			burner_job_get_session_output_size (BURNER_JOB (cdrecord),
							     NULL,
							     &bytes);
			mb_total = bytes / (goffset) 1048576LL;
			burner_cdrecord_compute (cdrecord,
						  mb_written,
						  mb_total,
						  track);
		}

		burner_job_start_progress (BURNER_JOB (cdrecord), FALSE);
	}
	else if (sscanf (line, "Track %*d: %*s %d MB ", &mb_total) == 1) {
/*		if (mb_total > 0)
			priv->tracks_total_bytes += mb_total * 1048576;
*/	}
	else if (strstr (line, "Formatting media")) {
		burner_job_set_current_action (BURNER_JOB (process),
						BURNER_BURN_ACTION_BLANKING,
						_("Formatting disc"),
						FALSE);
	}
	else if (strstr (line, "Sending CUE sheet")) {
		BurnerTrackType *type = NULL;

		/* See if we are in an audio case which would mean we're writing
		 * CD-TEXT */
		type = burner_track_type_new ();
		burner_job_get_input_type (BURNER_JOB (cdrecord), type);
		burner_job_set_current_action (BURNER_JOB (process),
						BURNER_BURN_ACTION_RECORDING_CD_TEXT,
						burner_track_type_get_has_stream (type) ? NULL:_("Writing cue sheet"),
						FALSE);
		burner_track_type_free (type);
	}
	else if (g_str_has_prefix (line, "Re-load disk and hit <CR>")
	     ||  g_str_has_prefix (line, "send SIGUSR1 to continue")) {
		BurnerBurnAction action = BURNER_BURN_ACTION_NONE;

		burner_job_get_current_action (BURNER_JOB (process), &action);

		/* NOTE: There seems to be a BUG somewhere when writing raw images
		 * with clone mode. After disc has been written and fixated cdrecord
		 * asks the media to be reloaded. So we simply ignore this message
		 * and returns that everything went well. Which is indeed the case */
		if (action == BURNER_BURN_ACTION_FIXATING) {
			burner_job_finished_session (BURNER_JOB (process));
			return BURNER_BURN_OK;
		}

		burner_job_error (BURNER_JOB (process),
				   g_error_new (BURNER_BURN_ERROR,
						BURNER_BURN_ERROR_MEDIUM_NEED_RELOADING,
						_("The disc needs to be reloaded before being recorded")));
	}
	else if (g_str_has_prefix (line, "Fixating...")
	     ||  g_str_has_prefix (line, "Writing Leadout...")) {
		BurnerJobAction action;

		/* Do this to avoid strange things to appear when erasing */
		burner_job_get_action (BURNER_JOB (process), &action);
		if (action == BURNER_JOB_ACTION_RECORD)
			burner_job_set_current_action (BURNER_JOB (process),
							BURNER_BURN_ACTION_FIXATING,
							NULL,
							FALSE);
	}
	else if (g_str_has_prefix (line, "Last chance to quit, ")) {
		burner_job_set_dangerous (BURNER_JOB (process), TRUE);
	}
/*	else if (g_str_has_prefix (line, "Blanking PMA, TOC, pregap")
	     ||  strstr (line, "Blanking entire disk")) {

	}
*/
	else if (strstr (line, "Disk sub type: Ultra High speed+")) {
		/* Set a deferred error as this message tends to indicate a failure */
		burner_process_deferred_error (process,
						g_error_new (BURNER_BURN_ERROR,
							     BURNER_BURN_ERROR_MEDIUM_INVALID,
							     _("The disc is not supported")));
	}
	/* This should not happen */
	/* else if (strstr (line, "Use tsize= option in SAO mode to specify track size")) */

	return BURNER_BURN_OK;
}

static gboolean
burner_cdrecord_write_inf (BurnerCDRecord *cdrecord,
			    GPtrArray *argv,
			    BurnerTrack *track,
			    const gchar *tmpdir,
			    const gchar *album,
			    gint index,
			    gint start,
			    gboolean last_track,
			    GError **error)
{
	gint fd;
        int errsv;
	gint size;
	gchar *path;
	guint64 length;
	gchar *string;
	gint b_written;
	gint64 sectors;
	const gchar *info;
	gchar buffer [128];
	BurnerCDRecordPrivate *priv;

	priv = BURNER_CD_RECORD_PRIVATE (cdrecord);

	/* NOTE: about the .inf files: they should have the exact same path
	 * but the ending suffix file is replaced by inf:
	 * example : /path/to/file.mp3 => /path/to/file.inf */
	if (burner_job_get_fd_in (BURNER_JOB (cdrecord), NULL) != BURNER_BURN_OK) {
		gchar *dot, *separator;

		path = burner_track_stream_get_source (BURNER_TRACK_STREAM (track), FALSE);

		dot = strrchr (path, '.');
		separator = strrchr (path, G_DIR_SEPARATOR);

		if (dot && dot > separator)
			path = g_strdup_printf ("%.*s.inf",
						(int) (dot - path),
						path);
		else
			path = g_strdup_printf ("%s.inf",
						path);

		/* since this file was not returned by burner_job_get_tmp_file
		 * it won't be erased when session is unrefed so we have to do 
		 * it ourselves */
		priv->infs = g_slist_prepend (priv->infs, g_strdup (path));
	}
	else {
		BurnerBurnResult result;

		/* in this case don't care about the name since stdin is used */
		result = burner_job_get_tmp_file (BURNER_JOB (cdrecord),
						   ".inf",
						   &path,
						   error);
		if (result != BURNER_BURN_OK)
			return result;
	}

	fd = open (path, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
	if (fd < 0)
		goto error;

	BURNER_JOB_LOG (cdrecord, "writing inf (%s)", path);

	/* The problem here is that when writing CD-TEXT from .inf files, wodim
	 * uses only one charset (and don't let us specify which one) which is
	 * ISO-8859-1. (NOTE: don't believe the doc claiming it requires ASCII
	 * and see cdrecord/cdtext.c line 309).
	 * So we have to convert our UTF-8 input into such a charset.
	 * NOTE: according to docs ASCII should be used for text packs other
	 * than disc/track title.
	 * It might be good in the end to write and pack CD-TEXT pack data 
	 * ourselves so we can set a different charset from English like 
	 * Chinese for example. */
	strcpy (buffer, "# created by burner\n");
	size = strlen (buffer);
	b_written = write (fd, buffer, size);
	if (b_written != size)
		goto error;

	strcpy (buffer, "MCN=\t\n");
	size = strlen (buffer);
	b_written = write (fd, buffer, size);
	if (b_written != size)
		goto error;

	/* ISRC */
	info = burner_track_tag_lookup_string (BURNER_TRACK (track), BURNER_TRACK_STREAM_ISRC_TAG);
	if (info)
		string = g_strdup_printf ("ISRC=\t%s\n", info);
	else
		string = g_strdup ("ISRC=\t\n");
	size = strlen (string);
	b_written = write (fd, string, size);
	g_free (string);
	if (b_written != size)
		goto error;

	strcpy (buffer, "Albumperformer=\t\n");
	size = strlen (buffer);
	b_written = write (fd, buffer, size);
	if (b_written != size)
		goto error;

	if (album) {
		gchar *encoded;

		encoded = g_convert_with_fallback (album,
						   -1,
						   "ISO-8859-1",
						   "UTF-8",
						   "_",	/* Fallback for non convertible characters */
						   NULL,
						   NULL,
						   NULL);
		string = g_strdup_printf ("Albumtitle=\t%s\n", encoded);
		g_free (encoded);
	}
	else
		string = strdup ("Albumtitle=\t\n");
	size = strlen (string);
	b_written = write (fd, string, size);
	g_free (string);
	if (b_written != size)
		goto error;

	/* ARTIST */
	info = burner_track_tag_lookup_string (BURNER_TRACK (track),
						BURNER_TRACK_STREAM_ARTIST_TAG);
	if (info) {
		gchar *encoded;

		encoded = g_convert_with_fallback (info,
						   -1,
						   "ISO-8859-1",
						   "UTF-8",
						   "_",	/* Fallback for non convertible characters */
						   NULL,
						   NULL,
						   NULL);
		string = g_strdup_printf ("Performer=\t%s\n", encoded);
		g_free (encoded);
	}
	else
		string = strdup ("Performer=\t\n");
	size = strlen (string);
	b_written = write (fd, string, size);
	g_free (string);
	if (b_written != size)
		goto error;

	/* COMPOSER */
	info = burner_track_tag_lookup_string (BURNER_TRACK (track),
						BURNER_TRACK_STREAM_COMPOSER_TAG);
	if (info) {
		gchar *encoded;

		encoded = g_convert_with_fallback (info,
						   -1,
						   "ISO-8859-1",
						   "UTF-8",
						   "_",	/* Fallback for non convertible characters */
						   NULL,
						   NULL,
						   NULL);
		string = g_strdup_printf ("Composer=\t%s\n", encoded);
		g_free (encoded);
	}
	else
		string = strdup ("Composer=\t\n");
	size = strlen (string);
	b_written = write (fd, string, size);
	g_free (string);
	if (b_written != size)
		goto error;

	/* TITLE */
	info = burner_track_tag_lookup_string (BURNER_TRACK (track),
						BURNER_TRACK_STREAM_TITLE_TAG);
	if (info) {
		gchar *encoded;

		encoded = g_convert_with_fallback (info,
						   -1,
						   "ISO-8859-1",
						   "UTF-8",
						   "_",	/* Fallback for non convertible characters */
						   NULL,
						   NULL,
						   NULL);
		string = g_strdup_printf ("Tracktitle=\t%s\n", encoded);
		g_free (encoded);
	}
	else
		string = strdup ("Tracktitle=\t\n");
	size = strlen (string);
	b_written = write (fd, string, size);
	g_free (string);
	if (b_written != size)
		goto error;

	string = g_strdup_printf ("Tracknumber=\t%i\n", index);
	size = strlen (string);
	b_written = write (fd, string, size);
	g_free (string);
	if (b_written != size)
		goto error;

	string = g_strdup_printf ("Trackstart=\t%i\n", start);
	size = strlen (string);
	b_written = write (fd, string, size);
	g_free (string);
	if (b_written != size)
		goto error;

	length = 0;
	burner_track_stream_get_length (BURNER_TRACK_STREAM (track), &length);
	sectors = BURNER_DURATION_TO_SECTORS (length);

	BURNER_JOB_LOG (cdrecord, "got track length %lli", length);
	string = g_strdup_printf ("Tracklength=\t%"G_GINT64_FORMAT", 0\n", sectors);
	size = strlen (string);
	b_written = write (fd, string, size);
	g_free (string);
	if (b_written != size)
		goto error;

	strcpy (buffer, "Pre-emphasis=\tno\n");
	size = strlen (buffer);
	b_written = write (fd, buffer, size);
	if (b_written != size)
		goto error;

	strcpy (buffer, "Channels=\t2\n");
	size = strlen (buffer);
	b_written = write (fd, buffer, size);
	if (b_written != size)
		goto error;

	strcpy (buffer, "Copy_permitted=\tyes\n");
	size = strlen (buffer);
	b_written = write (fd, buffer, size);
	if (b_written != size)
		goto error;

	strcpy (buffer, "Endianess=\tlittle\n");
	size = strlen (buffer);
	b_written = write (fd, buffer, size);
	if (b_written != size)
		goto error;

	strcpy (buffer, "Index=\t\t0\n");
	size = strlen (buffer);
	b_written = write (fd, buffer, size);
	if (b_written != size)
		goto error;

	/* NOTE: -1 here means no pregap */
	if (!last_track) {
		/* K3b does this (possibly to remove silence) */
		string = g_strdup_printf ("Index0=\t\t%"G_GINT64_FORMAT"\n",
					  sectors - 150);
	}
	else
		string = g_strdup_printf ("Index0=\t\t-1\n");

	size = strlen (string);
	b_written = write (fd, string, size);
	g_free (string);
	if (b_written != size)
		goto error;

	close (fd);

	if (argv)
		g_ptr_array_add (argv, path);
	else
		g_free (path);

	return BURNER_BURN_OK;


error:
        errsv = errno;

	g_remove (path);
	g_free (path);

	g_set_error (error,
		     BURNER_BURN_ERROR,
		     BURNER_BURN_ERROR_GENERAL,
		     _("An internal error occurred (%s)"), 
		     g_strerror (errsv));

	return BURNER_BURN_ERR;
}

static BurnerBurnResult
burner_cdrecord_write_infs (BurnerCDRecord *cdrecord,
			     GPtrArray *argv,
			     GError **error)
{
	BurnerBurnResult result;
	gchar *tmpdir = NULL;
	GSList *tracks;
	goffset start;
	GSList *iter;
	gchar *album;
	gint index;

	burner_job_get_audio_title (BURNER_JOB (cdrecord), &album);
	burner_job_get_tracks (BURNER_JOB (cdrecord), &tracks);
	index = 1;
	start = 0;

	for (iter = tracks; iter; iter = iter->next) {
		goffset sectors;
		BurnerTrack *track;
		const gchar *track_inf;

		track = iter->data;

		track_inf = burner_track_tag_lookup_string (track, BURNER_CDRTOOLS_TRACK_INF_FILE);
		if (track_inf) {
			BURNER_JOB_LOG (cdrecord, "Already existing .inf file");

			/* There is already an .inf file associated with this track */
			if (argv)
				g_ptr_array_add (argv, g_strdup (track_inf));
		}
		else {
			result = burner_cdrecord_write_inf (cdrecord,
							     argv,
							     track,
							     tmpdir,
							     album,
							     index,
							     start,
							     (iter->next == NULL),
							     error);
			if (result != BURNER_BURN_OK)
				return result;
		}

		index ++;
		sectors = 0;

		burner_track_get_size (track, &sectors, NULL);
		start += sectors;
	}

	g_free (album);
	g_free (tmpdir);

	return BURNER_BURN_OK;
}

static BurnerBurnResult
burner_cdrecord_set_argv_record (BurnerCDRecord *cdrecord,
				  GPtrArray *argv, 
				  GError **error)
{
	guint speed;
	BurnerBurnFlag flags;
	BurnerCDRecordPrivate *priv;
	BurnerTrackType *type = NULL;

	priv = BURNER_CD_RECORD_PRIVATE (cdrecord);

	if (priv->immediate) {
		g_ptr_array_add (argv, g_strdup ("-immed"));
		g_ptr_array_add (argv, g_strdup_printf ("minbuf=%i", priv->minbuf));
	}

	if (burner_job_get_speed (BURNER_JOB (cdrecord), &speed) == BURNER_BURN_OK) {
		gchar *speed_str;

		speed_str = g_strdup_printf ("speed=%d", speed);
		g_ptr_array_add (argv, speed_str);
	}

	burner_job_get_flags (BURNER_JOB (cdrecord), &flags);
	if (flags & BURNER_BURN_FLAG_OVERBURN)
		g_ptr_array_add (argv, g_strdup ("-overburn"));
	if (flags & BURNER_BURN_FLAG_BURNPROOF)
		g_ptr_array_add (argv, g_strdup ("driveropts=burnfree"));
	if (flags & BURNER_BURN_FLAG_MULTI)
		g_ptr_array_add (argv, g_strdup ("-multi"));

	/* NOTE: This write mode is necessary for all CLONE images burning */
	if (flags & BURNER_BURN_FLAG_RAW)
		g_ptr_array_add (argv, g_strdup ("-raw96r"));

	/* NOTE1: DAO can't be used if we're appending to a disc */
	/* NOTE2: CD-text cannot be written in tao mode (which is the default)
	 * NOTE3: when we don't want wodim to use stdin then we give the audio
	 * file on the command line. Otherwise we use the .inf */
	if (flags & BURNER_BURN_FLAG_DAO)
		g_ptr_array_add (argv, g_strdup ("-dao"));

	type = burner_track_type_new ();
	burner_job_get_input_type (BURNER_JOB (cdrecord), type);

	if (burner_job_get_fd_in (BURNER_JOB (cdrecord), NULL) == BURNER_BURN_OK) {
		BurnerBurnResult result;
		int buffer_size;
		goffset sectors;
		
		/* we need to know what is the type of the track (audio / data) */
		result = burner_job_get_input_type (BURNER_JOB (cdrecord), type);
		if (result != BURNER_BURN_OK) {
			burner_track_type_free (type);

			BURNER_JOB_LOG (cdrecord, "Imager doesn't seem to be ready")
			g_set_error (error,
				     BURNER_BURN_ERROR,
				     BURNER_BURN_ERROR_GENERAL,
				     _("An internal error occurred"));
			return BURNER_BURN_ERR;
		}
		
		/* ask the size */
		result = burner_job_get_session_output_size (BURNER_JOB (cdrecord),
							      &sectors,
							      NULL);
		if (result != BURNER_BURN_OK) {
			burner_track_type_free (type);

			BURNER_JOB_LOG (cdrecord, "The size of the session cannot be retrieved")
			g_set_error (error,
				     BURNER_BURN_ERROR,
				     BURNER_BURN_ERROR_GENERAL,
				     _("An internal error occurred"));
			return BURNER_BURN_ERR;
		}

		/* we create a buffer depending on the size 
		 * buffer 4m> < 64m and is 1/25th of size otherwise */
		buffer_size = sectors * 2352 / 1024 / 1024 / 25;
		if (buffer_size > 32)
			buffer_size = 32;
		else if (buffer_size < 4)
			buffer_size = 4;

		g_ptr_array_add (argv, g_strdup_printf ("fs=%im", buffer_size));
		if (burner_track_type_get_has_image (type)) {
			BurnerImageFormat format;

			format = burner_track_type_get_image_format (type);
			if (format == BURNER_IMAGE_FORMAT_BIN) {
				g_ptr_array_add (argv, g_strdup_printf ("tsize=%"G_GINT64_FORMAT"s", sectors));
				g_ptr_array_add (argv, g_strdup ("-data"));
				g_ptr_array_add (argv, g_strdup ("-nopad"));
				g_ptr_array_add (argv, g_strdup ("-"));
			}
			else {
				burner_track_type_free (type);
				BURNER_JOB_NOT_SUPPORTED (cdrecord);
			}
		}
		else if (burner_track_type_get_has_stream (type)) {
			g_ptr_array_add (argv, g_strdup ("-audio"));
			g_ptr_array_add (argv, g_strdup ("-useinfo"));
			g_ptr_array_add (argv, g_strdup ("-text"));

			result = burner_cdrecord_write_infs (cdrecord,
							      argv,
							      error);
			if (result != BURNER_BURN_OK) {
				burner_track_type_free (type);
				return result;
			}
		}
		else {
			burner_track_type_free (type);
			BURNER_JOB_NOT_SUPPORTED (cdrecord);
		}
	}
	else if (burner_track_type_get_has_stream (type)) {
		BurnerBurnResult result;
		GSList *tracks;

		g_ptr_array_add (argv, g_strdup ("fs=16m"));
		g_ptr_array_add (argv, g_strdup ("-audio"));
		g_ptr_array_add (argv, g_strdup ("-pad"));
	
		g_ptr_array_add (argv, g_strdup ("-useinfo"));
		g_ptr_array_add (argv, g_strdup ("-text"));

		result = burner_cdrecord_write_infs (cdrecord,
						      NULL,
						      error);
		if (result != BURNER_BURN_OK) {
			burner_track_type_free (type);
			return result;
		}

		tracks = NULL;
		burner_job_get_tracks (BURNER_JOB (cdrecord), &tracks);
		for (; tracks; tracks = tracks->next) {
			BurnerTrack *track;
			gchar *path;

			track = tracks->data;
			path = burner_track_stream_get_source (BURNER_TRACK_STREAM (track), FALSE);
			g_ptr_array_add (argv, path);
		}
	}
	else if (burner_track_type_get_has_image (type)) {
		BurnerTrack *track = NULL;
		BurnerImageFormat format;

		burner_job_get_current_track (BURNER_JOB (cdrecord), &track);
		if (!track) {
			burner_track_type_free (type);
			BURNER_JOB_NOT_READY (cdrecord);
		}

		format = burner_track_type_get_image_format (type);
		if (format == BURNER_IMAGE_FORMAT_NONE) {
			gchar *image_path;

			image_path = burner_track_image_get_source (BURNER_TRACK_IMAGE (track), FALSE);
			if (!image_path) {
				burner_track_type_free (type);
				BURNER_JOB_NOT_READY (cdrecord);
			}

			g_ptr_array_add (argv, g_strdup ("fs=16m"));
			g_ptr_array_add (argv, g_strdup ("-data"));
			g_ptr_array_add (argv, g_strdup ("-nopad"));
			g_ptr_array_add (argv, image_path);
		}
		else if (format == BURNER_IMAGE_FORMAT_BIN) {
			gchar *isopath;

			isopath = burner_track_image_get_source (BURNER_TRACK_IMAGE (track), FALSE);
			if (!isopath) {
				burner_track_type_free (type);
				BURNER_JOB_NOT_READY (cdrecord);
			}

			g_ptr_array_add (argv, g_strdup ("fs=16m"));
			g_ptr_array_add (argv, g_strdup ("-data"));
			g_ptr_array_add (argv, g_strdup ("-nopad"));
			g_ptr_array_add (argv, isopath);
		}
		else if (format == BURNER_IMAGE_FORMAT_CLONE) {
			gchar *rawpath;

			rawpath = burner_track_image_get_source (BURNER_TRACK_IMAGE (track), FALSE);
			if (!rawpath) {
				burner_track_type_free (type);
				BURNER_JOB_NOT_READY (cdrecord);
			}

			g_ptr_array_add (argv, g_strdup ("fs=16m"));
			g_ptr_array_add (argv, g_strdup ("-clone"));
			g_ptr_array_add (argv, rawpath);
		}
		else if (format == BURNER_IMAGE_FORMAT_CUE) {
			gchar *cue_str;
			gchar *cuepath;
			gchar *parent;

			cuepath = burner_track_image_get_toc_source (BURNER_TRACK_IMAGE (track), FALSE);
			if (!cuepath) {
				burner_track_type_free (type);
				BURNER_JOB_NOT_READY (cdrecord);
			}

			parent = g_path_get_dirname (cuepath);
			burner_process_set_working_directory (BURNER_PROCESS (cdrecord), parent);
			g_free (parent);

			/* we need to check endianness */
			if (burner_track_image_need_byte_swap (BURNER_TRACK_IMAGE (track)))
				g_ptr_array_add (argv, g_strdup ("-swab"));

			g_ptr_array_add (argv, g_strdup ("fs=16m"));

			/* This is to make sure the CD-TEXT stuff gets written */
			g_ptr_array_add (argv, g_strdup ("-text"));

			cue_str = g_strdup_printf ("cuefile=%s", cuepath);
			g_ptr_array_add (argv, cue_str);
			g_free (cuepath);
		}
		else {
			burner_track_type_free (type);
			BURNER_JOB_NOT_SUPPORTED (cdrecord);
		}
	}
	else {
		burner_track_type_free (type);
		BURNER_JOB_NOT_SUPPORTED (cdrecord);
	}

	burner_track_type_free (type);

	burner_job_set_current_action (BURNER_JOB (cdrecord),
					BURNER_BURN_ACTION_START_RECORDING,
					NULL,
					FALSE);
	return BURNER_BURN_OK;
}

static BurnerBurnResult
burner_cdrecord_set_argv_blank (BurnerCDRecord *cdrecord, GPtrArray *argv)
{
	gchar *blank_str;
	BurnerBurnFlag flags;

	burner_job_get_flags (BURNER_JOB (cdrecord), &flags);
	blank_str = g_strdup_printf ("blank=%s",
				    (flags & BURNER_BURN_FLAG_FAST_BLANK) ? "fast" : "all");
	g_ptr_array_add (argv, blank_str);

	burner_job_set_current_action (BURNER_JOB (cdrecord),
					BURNER_BURN_ACTION_BLANKING,
					NULL,
					FALSE);
	return BURNER_BURN_OK;
}

static BurnerBurnResult
burner_cdrecord_set_argv (BurnerProcess *process,
			   GPtrArray *argv,
			   GError **error)
{
	BurnerCDRecord *cdrecord;
	BurnerBurnResult result;
	BurnerJobAction action;
	BurnerBurnFlag flags;
	gchar *device = NULL;
	gchar *dev_str;

	cdrecord = BURNER_CD_RECORD (process);

	burner_job_get_action (BURNER_JOB (cdrecord), &action);
	if (action == BURNER_JOB_ACTION_SIZE)
		return BURNER_BURN_NOT_SUPPORTED;

	g_ptr_array_add (argv, g_strdup ("cdrecord"));
	g_ptr_array_add (argv, g_strdup ("-v"));

	/* NOTE: that function returns either bus_target_lun or the device path
	 * according to OSes. Basically it returns bus/target/lun only for FreeBSD
	 * which is the only OS in need for that. For all others it returns the device
	 * path. */
	burner_job_get_bus_target_lun (BURNER_JOB (cdrecord), &device);
	dev_str = g_strdup_printf ("dev=%s", device);
	g_ptr_array_add (argv, dev_str);
	g_free (device);

	burner_job_get_flags (BURNER_JOB (cdrecord), &flags);
        if (flags & BURNER_BURN_FLAG_DUMMY)
		g_ptr_array_add (argv, g_strdup ("-dummy"));

	if (flags & BURNER_BURN_FLAG_NOGRACE)
		g_ptr_array_add (argv, g_strdup ("gracetime=0"));

	if (action == BURNER_JOB_ACTION_RECORD)
		result = burner_cdrecord_set_argv_record (cdrecord, argv, error);
	else if (action == BURNER_JOB_ACTION_ERASE)
		result = burner_cdrecord_set_argv_blank (cdrecord, argv);
	else
		BURNER_JOB_NOT_SUPPORTED (cdrecord);

	return result;	
}

static BurnerBurnResult
burner_cdrecord_post (BurnerJob *job)
{
	BurnerCDRecordPrivate *priv;
	GSList *iter;

	priv = BURNER_CD_RECORD_PRIVATE (job);
	for (iter = priv->infs; iter; iter = iter->next) {
		gchar *path;

		path = iter->data;
		g_remove (path);
		g_free (path);
	}

	g_slist_free (priv->infs);
	priv->infs = NULL;

	return burner_job_finished_session (job);
}

static void
burner_cdrecord_class_init (BurnerCDRecordClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);
	BurnerProcessClass *process_class = BURNER_PROCESS_CLASS (klass);

	g_type_class_add_private (klass, sizeof (BurnerCDRecordPrivate));

	parent_class = g_type_class_peek_parent (klass);
	object_class->finalize = burner_cdrecord_finalize;

	process_class->stderr_func = burner_cdrecord_stderr_read;
	process_class->stdout_func = burner_cdrecord_stdout_read;
	process_class->set_argv = burner_cdrecord_set_argv;
	process_class->post = burner_cdrecord_post;
}

static void
burner_cdrecord_init (BurnerCDRecord *obj)
{
	GSettings *settings;
	BurnerCDRecordPrivate *priv;

	/* load our "configuration" */
	priv = BURNER_CD_RECORD_PRIVATE (obj);

	settings = g_settings_new (BURNER_SCHEMA_CONFIG);

	priv->immediate = g_settings_get_boolean (settings, BURNER_KEY_IMMEDIATE_FLAG);
	priv->minbuf = g_settings_get_int (settings, BURNER_KEY_MINBUF_VALUE);
	if (priv->minbuf > 95 || priv->minbuf < 25)
		priv->minbuf = 30;

	g_object_unref (settings);
}

static void
burner_cdrecord_finalize (GObject *object)
{
	BurnerCDRecordPrivate *priv;
	GSList *iter;

	priv = BURNER_CD_RECORD_PRIVATE (object);

	for (iter = priv->infs; iter; iter = iter->next) {
		gchar *path;

		path = iter->data;
		g_remove (path);
		g_free (path);
	}

	g_slist_free (priv->infs);
	priv->infs = NULL;

	G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
burner_cdrecord_export_caps (BurnerPlugin *plugin)
{
	BurnerPluginConfOption *immed, *minbuf;
	const BurnerMedia media = BURNER_MEDIUM_CD|
				   BURNER_MEDIUM_WRITABLE|
				   BURNER_MEDIUM_REWRITABLE|
				   BURNER_MEDIUM_BLANK|
				   BURNER_MEDIUM_APPENDABLE|
				   BURNER_MEDIUM_HAS_AUDIO|
				   BURNER_MEDIUM_HAS_DATA;
	const BurnerMedia dvd_media = BURNER_MEDIUM_DVD|
				       BURNER_MEDIUM_PLUS|
				       BURNER_MEDIUM_SEQUENTIAL|
				       BURNER_MEDIUM_WRITABLE|
				       BURNER_MEDIUM_REWRITABLE|
				       BURNER_MEDIUM_BLANK|
				       BURNER_MEDIUM_UNFORMATTED|
				       BURNER_MEDIUM_APPENDABLE|
				       BURNER_MEDIUM_HAS_DATA;
	const BurnerMedia media_rw = BURNER_MEDIUM_CD|
				      BURNER_MEDIUM_REWRITABLE|
				      BURNER_MEDIUM_APPENDABLE|
				      BURNER_MEDIUM_CLOSED|
				      BURNER_MEDIUM_HAS_AUDIO|
				      BURNER_MEDIUM_HAS_DATA|
				      BURNER_MEDIUM_BLANK;
	GSList *output;
	GSList *input;

	/* NOTE: it seems that cdrecord can burn cue files on the fly */
	burner_plugin_define (plugin,
			       "cdrecord",
	                       NULL,
			       _("Burns, blanks and formats CDs, DVDs and BDs"),
			       "Philippe Rouquier",
			       1);

	/* for recording */
	input = burner_caps_image_new (BURNER_PLUGIN_IO_ACCEPT_PIPE|
					BURNER_PLUGIN_IO_ACCEPT_FILE,
					BURNER_IMAGE_FORMAT_BIN);

	/* cdrecord can burn all DVDs (except restricted) when it's ISOs */
	output = burner_caps_disc_new (dvd_media);
	burner_plugin_link_caps (plugin, output, input);
	g_slist_free (output);

	/* All CD-R(W) */
	output = burner_caps_disc_new (media);
	burner_plugin_link_caps (plugin, output, input);
	g_slist_free (input);

	input = burner_caps_audio_new (BURNER_PLUGIN_IO_ACCEPT_PIPE|
					BURNER_PLUGIN_IO_ACCEPT_FILE,
					BURNER_AUDIO_FORMAT_RAW|
					BURNER_METADATA_INFO);

	burner_plugin_link_caps (plugin, output, input);
	g_slist_free (input);

	input = burner_caps_audio_new (BURNER_PLUGIN_IO_ACCEPT_PIPE|
					BURNER_PLUGIN_IO_ACCEPT_FILE,
					BURNER_AUDIO_FORMAT_RAW);

	burner_plugin_link_caps (plugin, output, input);
	g_slist_free (output);
	g_slist_free (input);

	/* for CLONE and CUE type images, we only want blank CD-R(W) */
	output = burner_caps_disc_new (BURNER_MEDIUM_CD|
					BURNER_MEDIUM_WRITABLE|
					BURNER_MEDIUM_REWRITABLE|
					BURNER_MEDIUM_BLANK);

	input = burner_caps_image_new (BURNER_PLUGIN_IO_ACCEPT_FILE,
					BURNER_IMAGE_FORMAT_CUE|
					BURNER_IMAGE_FORMAT_CLONE);

	burner_plugin_link_caps (plugin, output, input);
	g_slist_free (output);
	g_slist_free (input);

	/* Blank CD(R)W : don't use standard flags cdrecord fails consistently
	 * to write a first track of a multisession disc with DAO mode. */
	burner_plugin_set_flags (plugin,
				  BURNER_MEDIUM_CD|
				  BURNER_MEDIUM_WRITABLE|
				  BURNER_MEDIUM_REWRITABLE|
				  BURNER_MEDIUM_BLANK,
				  BURNER_BURN_FLAG_DAO|
				  BURNER_BURN_FLAG_BURNPROOF|
				  BURNER_BURN_FLAG_OVERBURN|
				  BURNER_BURN_FLAG_DUMMY|
				  BURNER_BURN_FLAG_NOGRACE,
				  BURNER_BURN_FLAG_NONE);

	burner_plugin_set_flags (plugin,
				  BURNER_MEDIUM_CD|
				  BURNER_MEDIUM_WRITABLE|
				  BURNER_MEDIUM_REWRITABLE|
				  BURNER_MEDIUM_BLANK,
				  BURNER_BURN_FLAG_MULTI|
				  BURNER_BURN_FLAG_BURNPROOF|
				  BURNER_BURN_FLAG_OVERBURN|
				  BURNER_BURN_FLAG_DUMMY|
				  BURNER_BURN_FLAG_NOGRACE,
				  BURNER_BURN_FLAG_NONE);

	/* Apart from DAO it also supports RAW mode to burn CLONE images. This
	 * is a special mode for which there isn't any DUMMY burn possible */
	burner_plugin_set_flags (plugin,
				  BURNER_MEDIUM_CD|
				  BURNER_MEDIUM_WRITABLE|
				  BURNER_MEDIUM_REWRITABLE|
				  BURNER_MEDIUM_BLANK,
				  BURNER_BURN_FLAG_RAW|
				  BURNER_BURN_FLAG_BURNPROOF|
				  BURNER_BURN_FLAG_OVERBURN|
				  BURNER_BURN_FLAG_NOGRACE,
				  BURNER_BURN_FLAG_NONE);

	/* This is a CDR with data; data can be merged or at least appended */
	burner_plugin_set_flags (plugin,
				  BURNER_MEDIUM_CD|
				  BURNER_MEDIUM_WRITABLE|
				  BURNER_MEDIUM_APPENDABLE|
				  BURNER_MEDIUM_HAS_AUDIO|
				  BURNER_MEDIUM_HAS_DATA,
				  BURNER_BURN_FLAG_APPEND|
				  BURNER_BURN_FLAG_MERGE|
				  BURNER_BURN_FLAG_BURNPROOF|
				  BURNER_BURN_FLAG_OVERBURN|
				  BURNER_BURN_FLAG_MULTI|
				  BURNER_BURN_FLAG_DUMMY|
				  BURNER_BURN_FLAG_NOGRACE,
				  BURNER_BURN_FLAG_APPEND);

	/* It is a CDRW we want the CD to be either blanked before or appended
	 * that's why we set MERGE as compulsory. That way if the CD is not
	 * MERGED we force the blank before writing to avoid appending sessions
	 * endlessly until there is no free space. */
	burner_plugin_set_flags (plugin,
				  BURNER_MEDIUM_CD|
				  BURNER_MEDIUM_REWRITABLE|
				  BURNER_MEDIUM_APPENDABLE|
				  BURNER_MEDIUM_HAS_AUDIO|
				  BURNER_MEDIUM_HAS_DATA,
				  BURNER_BURN_FLAG_APPEND|
				  BURNER_BURN_FLAG_MERGE|
				  BURNER_BURN_FLAG_BURNPROOF|
				  BURNER_BURN_FLAG_OVERBURN|
				  BURNER_BURN_FLAG_MULTI|
				  BURNER_BURN_FLAG_DUMMY|
				  BURNER_BURN_FLAG_NOGRACE,
				  BURNER_BURN_FLAG_MERGE);

	/* DVD-RW cdrecord capabilites are limited to blank media.
	 * It should not start a multisession disc. */
	burner_plugin_set_flags (plugin,
				  BURNER_MEDIUM_DVD|
				  BURNER_MEDIUM_SEQUENTIAL|
				  BURNER_MEDIUM_WRITABLE|
				  BURNER_MEDIUM_REWRITABLE|
				  BURNER_MEDIUM_BLANK,
				  BURNER_BURN_FLAG_DAO|
				  BURNER_BURN_FLAG_BURNPROOF|
				  BURNER_BURN_FLAG_DUMMY|
				  BURNER_BURN_FLAG_NOGRACE,
				  BURNER_BURN_FLAG_NONE);

	/* DVD+W cdrecord capabilities are limited to blank media */
	burner_plugin_set_flags (plugin,
				  BURNER_MEDIUM_DVDR_PLUS|
				  BURNER_MEDIUM_BLANK,
				  BURNER_BURN_FLAG_DAO|
				  BURNER_BURN_FLAG_BURNPROOF|
				  BURNER_BURN_FLAG_NOGRACE,
				  BURNER_BURN_FLAG_NONE);

	/* for DVD+RW cdrecord capabilities are limited no MERGE */
	burner_plugin_set_flags (plugin,
				  BURNER_MEDIUM_DVDRW_PLUS|
				  BURNER_MEDIUM_UNFORMATTED|
				  BURNER_MEDIUM_BLANK,
				  BURNER_BURN_FLAG_NOGRACE,
				  BURNER_BURN_FLAG_NONE);

	burner_plugin_set_flags (plugin,
				  BURNER_MEDIUM_DVDRW_PLUS|
				  BURNER_MEDIUM_APPENDABLE|
				  BURNER_MEDIUM_CLOSED|
				  BURNER_MEDIUM_HAS_DATA,
				  BURNER_BURN_FLAG_NOGRACE,
				  BURNER_BURN_FLAG_NONE);

	/* blanking/formatting caps and flags for +/sequential RW
	 * NOTE: restricted overwrite DVD-RW can't be formatted.
	 * moreover DVD+RW are formatted while DVD-RW sequential are blanked.
	 */
	output = burner_caps_disc_new (BURNER_MEDIUM_DVD|
					BURNER_MEDIUM_PLUS|
					BURNER_MEDIUM_REWRITABLE|
					BURNER_MEDIUM_APPENDABLE|
	    				BURNER_MEDIUM_SEQUENTIAL|
					BURNER_MEDIUM_CLOSED|
					BURNER_MEDIUM_HAS_DATA|
					BURNER_MEDIUM_UNFORMATTED|
					BURNER_MEDIUM_BLANK);
	burner_plugin_blank_caps (plugin, output);
	g_slist_free (output);

	burner_plugin_set_blank_flags (plugin,
	    				BURNER_MEDIUM_DVDRW |
	    				BURNER_MEDIUM_BLANK|
	    				BURNER_MEDIUM_CLOSED |
	    				BURNER_MEDIUM_APPENDABLE|
	    				BURNER_MEDIUM_HAS_DATA|
	    				BURNER_MEDIUM_UNFORMATTED,
					BURNER_BURN_FLAG_NOGRACE|
					BURNER_BURN_FLAG_FAST_BLANK,
					BURNER_BURN_FLAG_NONE);

	/* again DVD+RW don't support dummy */
	burner_plugin_set_blank_flags (plugin,
					BURNER_MEDIUM_DVDRW_PLUS|
					BURNER_MEDIUM_APPENDABLE|
					BURNER_MEDIUM_HAS_DATA|
					BURNER_MEDIUM_UNFORMATTED|
					BURNER_MEDIUM_BLANK|
					BURNER_MEDIUM_CLOSED,
					BURNER_BURN_FLAG_NOGRACE,
					BURNER_BURN_FLAG_NONE);

	/* for blanking (CDRWs) */
	output = burner_caps_disc_new (media_rw);
	burner_plugin_blank_caps (plugin, output);
	g_slist_free (output);

	burner_plugin_set_blank_flags (plugin,
					media_rw,
					BURNER_BURN_FLAG_NOGRACE|
					BURNER_BURN_FLAG_FAST_BLANK,
					BURNER_BURN_FLAG_NONE);

	/* add some configure options */
	immed = burner_plugin_conf_option_new (BURNER_KEY_IMMEDIATE_FLAG,
						_("Enable the \"-immed\" flag (see cdrecord manual)"),
						BURNER_PLUGIN_OPTION_BOOL);
	minbuf = burner_plugin_conf_option_new (BURNER_KEY_MINBUF_VALUE,
						 _("Minimum drive buffer fill ratio (in %%) (see cdrecord manual):"),
						 BURNER_PLUGIN_OPTION_INT);
	burner_plugin_conf_option_int_set_range (minbuf, 25, 95);

	burner_plugin_conf_option_bool_add_suboption (immed, minbuf);
	burner_plugin_add_conf_option (plugin, immed);

	burner_plugin_register_group (plugin, _(CDRTOOLS_DESCRIPTION));
}

G_MODULE_EXPORT void
burner_plugin_check_config (BurnerPlugin *plugin)
{
	gint version [3] = { 2, 0, -1};
	burner_plugin_test_app (plugin,
	                         "cdrecord",
	                         "--version",
	                         "Cdrecord-ProDVD-ProBD-Clone %d.%d",
	                         version);
}
