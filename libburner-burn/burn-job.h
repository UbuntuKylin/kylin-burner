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

#ifndef JOB_H
#define JOB_H

#include <glib.h>
#include <glib-object.h>

#include "burner-track.h"

G_BEGIN_DECLS

#define BURNER_TYPE_JOB         (burner_job_get_type ())
#define BURNER_JOB(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), BURNER_TYPE_JOB, BurnerJob))
#define BURNER_JOB_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k), BURNER_TYPE_JOB, BurnerJobClass))
#define BURNER_IS_JOB(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), BURNER_TYPE_JOB))
#define BURNER_IS_JOB_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), BURNER_TYPE_JOB))
#define BURNER_JOB_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), BURNER_TYPE_JOB, BurnerJobClass))

typedef enum {
	BURNER_JOB_ACTION_NONE		= 0,
	BURNER_JOB_ACTION_SIZE,
	BURNER_JOB_ACTION_IMAGE,
	BURNER_JOB_ACTION_RECORD,
	BURNER_JOB_ACTION_ERASE,
	BURNER_JOB_ACTION_CHECKSUM
} BurnerJobAction;

typedef struct {
	GObject parent;
} BurnerJob;

typedef struct {
	GObjectClass parent_class;

	/**
	 * Virtual functions to implement in each object deriving from
	 * BurnerJob.
	 */

	/**
	 * This function is not compulsory. If not implemented then the library
	 * will act as if OK had been returned.
	 * returns 	BURNER_BURN_OK on successful initialization
	 *		The job can return BURNER_BURN_NOT_RUNNING if it should
	 *		not be started.
	 * 		BURNER_BURN_ERR otherwise
	 */
	BurnerBurnResult	(*activate)		(BurnerJob *job,
							 GError **error);

	/**
	 * This function is compulsory.
	 * returns 	BURNER_BURN_OK if a loop should be run afterward
	 *		The job can return BURNER_BURN_NOT_RUNNING if it already
	 *		completed successfully the task or don't need to be run. In this
	 *		case, it's the whole task that will be skipped.
	 *		NOT_SUPPORTED if it can't do the action required. When running
	 *		in fake mode (to get size mostly), the job will be "forgiven" and
	 *		deactivated.
	 *		RETRY if the job is not able to start at the moment but should
	 *		be given another chance later.
	 * 		ERR otherwise
	 */
	BurnerBurnResult	(*start)		(BurnerJob *job,
							 GError **error);

	BurnerBurnResult	(*clock_tick)		(BurnerJob *job);

	BurnerBurnResult	(*stop)			(BurnerJob *job,
							 GError **error);

	/**
	 * you should not connect to this signal. It's used internally to 
	 * "autoconfigure" the backend when an error occurs
	 */
	BurnerBurnResult	(*error)		(BurnerJob *job,
							 BurnerBurnError error);
} BurnerJobClass;

GType burner_job_get_type (void);

/**
 * These functions are to be used to get information for running jobs.
 * They are only available when a job is running.
 */

BurnerBurnResult
burner_job_set_nonblocking (BurnerJob *self,
			     GError **error);

BurnerBurnResult
burner_job_get_action (BurnerJob *job, BurnerJobAction *action);

BurnerBurnResult
burner_job_get_flags (BurnerJob *job, BurnerBurnFlag *flags);

BurnerBurnResult
burner_job_get_fd_in (BurnerJob *job, int *fd_in);

BurnerBurnResult
burner_job_get_tracks (BurnerJob *job, GSList **tracks);

BurnerBurnResult
burner_job_get_done_tracks (BurnerJob *job, GSList **tracks);

BurnerBurnResult
burner_job_get_current_track (BurnerJob *job, BurnerTrack **track);

BurnerBurnResult
burner_job_get_input_type (BurnerJob *job, BurnerTrackType *type);

BurnerBurnResult
burner_job_get_audio_title (BurnerJob *job, gchar **album);

BurnerBurnResult
burner_job_get_data_label (BurnerJob *job, gchar **label);

BurnerBurnResult
burner_job_get_session_output_size (BurnerJob *job,
				     goffset *blocks,
				     goffset *bytes);

/**
 * Used to get information of the destination media
 */

BurnerBurnResult
burner_job_get_medium (BurnerJob *job, BurnerMedium **medium);

BurnerBurnResult
burner_job_get_bus_target_lun (BurnerJob *job, gchar **BTL);

BurnerBurnResult
burner_job_get_device (BurnerJob *job, gchar **device);

BurnerBurnResult
burner_job_get_media (BurnerJob *job, BurnerMedia *media);

BurnerBurnResult
burner_job_get_last_session_address (BurnerJob *job, goffset *address);

BurnerBurnResult
burner_job_get_next_writable_address (BurnerJob *job, goffset *address);

BurnerBurnResult
burner_job_get_rate (BurnerJob *job, guint64 *rate);

BurnerBurnResult
burner_job_get_speed (BurnerJob *self, guint *speed);

BurnerBurnResult
burner_job_get_max_rate (BurnerJob *job, guint64 *rate);

BurnerBurnResult
burner_job_get_max_speed (BurnerJob *job, guint *speed);

/**
 * necessary for objects imaging either to another or to a file
 */

BurnerBurnResult
burner_job_get_output_type (BurnerJob *job, BurnerTrackType *type);

BurnerBurnResult
burner_job_get_fd_out (BurnerJob *job, int *fd_out);

BurnerBurnResult
burner_job_get_image_output (BurnerJob *job,
			      gchar **image,
			      gchar **toc);
BurnerBurnResult
burner_job_get_audio_output (BurnerJob *job,
			      gchar **output);

/**
 * get a temporary file that will be deleted once the session is finished
 */
 
BurnerBurnResult
burner_job_get_tmp_file (BurnerJob *job,
			  const gchar *suffix,
			  gchar **output,
			  GError **error);

BurnerBurnResult
burner_job_get_tmp_dir (BurnerJob *job,
			 gchar **path,
			 GError **error);

/**
 * Each tag can be retrieved by any job
 */

BurnerBurnResult
burner_job_tag_lookup (BurnerJob *job,
			const gchar *tag,
			GValue **value);

BurnerBurnResult
burner_job_tag_add (BurnerJob *job,
		     const gchar *tag,
		     GValue *value);

/**
 * Used to give job results and tell when a job has finished
 */

BurnerBurnResult
burner_job_add_track (BurnerJob *job,
		       BurnerTrack *track);

BurnerBurnResult
burner_job_finished_track (BurnerJob *job);

BurnerBurnResult
burner_job_finished_session (BurnerJob *job);

BurnerBurnResult
burner_job_error (BurnerJob *job,
		   GError *error);

/**
 * Used to start progress reporting and starts an internal timer to keep track
 * of remaining time among other things
 */

BurnerBurnResult
burner_job_start_progress (BurnerJob *job,
			    gboolean force);

/**
 * task progress report: you can use only some of these functions
 */

BurnerBurnResult
burner_job_set_rate (BurnerJob *job,
		      gint64 rate);
BurnerBurnResult
burner_job_set_written_track (BurnerJob *job,
			       goffset written);
BurnerBurnResult
burner_job_set_written_session (BurnerJob *job,
				 goffset written);
BurnerBurnResult
burner_job_set_progress (BurnerJob *job,
			  gdouble progress);
BurnerBurnResult
burner_job_reset_progress (BurnerJob *job);
BurnerBurnResult
burner_job_set_current_action (BurnerJob *job,
				BurnerBurnAction action,
				const gchar *string,
				gboolean force);
BurnerBurnResult
burner_job_get_current_action (BurnerJob *job,
				BurnerBurnAction *action);
BurnerBurnResult
burner_job_set_output_size_for_current_track (BurnerJob *job,
					       goffset sectors,
					       goffset bytes);

/**
 * Used to tell it's (or not) dangerous to interrupt this job
 */

void
burner_job_set_dangerous (BurnerJob *job, gboolean value);

/**
 * This is for apps with a jerky current rate (like cdrdao)
 */

BurnerBurnResult
burner_job_set_use_average_rate (BurnerJob *job,
				  gboolean value);

/**
 * logging facilities
 */

void
burner_job_log_message (BurnerJob *job,
			 const gchar *location,
			 const gchar *format,
			 ...);

#define BURNER_JOB_LOG(job, message, ...) 			\
{								\
	gchar *format;						\
	format = g_strdup_printf ("%s %s",			\
				  G_OBJECT_TYPE_NAME (job),	\
				  message);			\
	burner_job_log_message (BURNER_JOB (job),		\
				 G_STRLOC,			\
				 format,		 	\
				 ##__VA_ARGS__);		\
	g_free (format);					\
}
#define BURNER_JOB_LOG_ARG(job, message, ...)			\
{								\
	gchar *format;						\
	format = g_strdup_printf ("\t%s",			\
				  (gchar*) message);		\
	burner_job_log_message (BURNER_JOB (job),		\
				 G_STRLOC,			\
				 format,			\
				 ##__VA_ARGS__);		\
	g_free (format);					\
}

#define BURNER_JOB_NOT_SUPPORTED(job) 					\
	{								\
		BURNER_JOB_LOG (job, "unsupported operation");		\
		return BURNER_BURN_NOT_SUPPORTED;			\
	}

#define BURNER_JOB_NOT_READY(job)						\
	{									\
		BURNER_JOB_LOG (job, "not ready to operate");	\
		return BURNER_BURN_NOT_READY;					\
	}


G_END_DECLS

#endif /* JOB_H */
