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
#include <errno.h>
#include <ctype.h>
#include <sys/param.h>
#include <unistd.h>
#include <fcntl.h>

#include <glib.h>
#include <glib-object.h>
#include <glib/gi18n-lib.h>
#include <glib/gstdio.h>

#include <gmodule.h>

#include "burner-plugin-registration.h"
#include "burn-job.h"
#include "burn-volume.h"
#include "burner-drive.h"
#include "burner-track-disc.h"
#include "burner-track-image.h"
#include "burner-tags.h"


#define BURNER_TYPE_CHECKSUM_IMAGE		(burner_checksum_image_get_type ())
#define BURNER_CHECKSUM_IMAGE(o)		(G_TYPE_CHECK_INSTANCE_CAST ((o), BURNER_TYPE_CHECKSUM_IMAGE, BurnerChecksumImage))
#define BURNER_CHECKSUM_CLASS(k)		(G_TYPE_CHECK_CLASS_CAST((k), BURNER_TYPE_CHECKSUM_IMAGE, BurnerChecksumImageClass))
#define BURNER_IS_CHECKSUM_IMAGE(o)		(G_TYPE_CHECK_INSTANCE_TYPE ((o), BURNER_TYPE_CHECKSUM_IMAGE))
#define BURNER_IS_CHECKSUM_IMAGE_CLASS(k)	(G_TYPE_CHECK_CLASS_TYPE ((k), BURNER_TYPE_CHECKSUM_IMAGE))
#define BURNER_CHECKSUM_GET_CLASS(o)		(G_TYPE_INSTANCE_GET_CLASS ((o), BURNER_TYPE_CHECKSUM_IMAGE, BurnerChecksumImageClass))

BURNER_PLUGIN_BOILERPLATE (BurnerChecksumImage, burner_checksum_image, BURNER_TYPE_JOB, BurnerJob);

struct _BurnerChecksumImagePrivate {
	GChecksum *checksum;
	BurnerChecksumType checksum_type;

	/* That's for progress reporting */
	goffset total;
	goffset bytes;

	/* this is for the thread and the end of it */
	GThread *thread;
	GMutex *mutex;
	GCond *cond;
	gint end_id;

	guint cancel;
};
typedef struct _BurnerChecksumImagePrivate BurnerChecksumImagePrivate;

#define BURNER_CHECKSUM_IMAGE_PRIVATE(o)  (G_TYPE_INSTANCE_GET_PRIVATE ((o), BURNER_TYPE_CHECKSUM_IMAGE, BurnerChecksumImagePrivate))

#define BURNER_SCHEMA_CONFIG		"org.gnome.burner.config"
#define BURNER_PROPS_CHECKSUM_IMAGE	"checksum-image"

static BurnerJobClass *parent_class = NULL;

static gint
burner_checksum_image_read (BurnerChecksumImage *self,
			     int fd,
			     guchar *buffer,
			     gint bytes,
			     GError **error)
{
	gint total = 0;
	gint read_bytes;
	BurnerChecksumImagePrivate *priv;

	priv = BURNER_CHECKSUM_IMAGE_PRIVATE (self);

	while (1) {
		read_bytes = read (fd, buffer + total, (bytes - total));

		/* maybe that's the end of the stream ... */
		if (!read_bytes)
			return total;

		if (priv->cancel)
			return -2;

		/* ... or an error =( */
		if (read_bytes == -1) {
			if (errno != EAGAIN && errno != EINTR) {
                                int errsv = errno;

				g_set_error (error,
					     BURNER_BURN_ERROR,
					     BURNER_BURN_ERROR_GENERAL,
					     _("Data could not be read (%s)"),
					     g_strerror (errsv));
				return -1;
			}
		}
		else {
			total += read_bytes;

			if (total == bytes)
				return total;
		}

		g_usleep (500);
	}

	return total;
}

static BurnerBurnResult
burner_checksum_image_write (BurnerChecksumImage *self,
			      int fd,
			      guchar *buffer,
			      gint bytes,
			      GError **error)
{
	gint bytes_remaining;
	gint bytes_written = 0;
	BurnerChecksumImagePrivate *priv;

	priv = BURNER_CHECKSUM_IMAGE_PRIVATE (self);

	bytes_remaining = bytes;
	while (bytes_remaining) {
		gint written;

		written = write (fd,
				 buffer + bytes_written,
				 bytes_remaining);

		if (priv->cancel)
			return BURNER_BURN_CANCEL;

		if (written != bytes_remaining) {
			if (errno != EINTR && errno != EAGAIN) {
                                int errsv = errno;

				/* unrecoverable error */
				g_set_error (error,
					     BURNER_BURN_ERROR,
					     BURNER_BURN_ERROR_GENERAL,
					     _("Data could not be written (%s)"),
					     g_strerror (errsv));
				return BURNER_BURN_ERR;
			}
		}

		g_usleep (500);

		if (written > 0) {
			bytes_remaining -= written;
			bytes_written += written;
		}
	}

	return BURNER_BURN_OK;
}

static BurnerBurnResult
burner_checksum_image_checksum (BurnerChecksumImage *self,
				 GChecksumType checksum_type,
				 int fd_in,
				 int fd_out,
				 GError **error)
{
	gint read_bytes;
	guchar buffer [2048];
	BurnerBurnResult result;
	BurnerChecksumImagePrivate *priv;

	priv = BURNER_CHECKSUM_IMAGE_PRIVATE (self);

	priv->checksum = g_checksum_new (checksum_type);
	result = BURNER_BURN_OK;
	while (1) {
		read_bytes = burner_checksum_image_read (self,
							  fd_in,
							  buffer,
							  sizeof (buffer),
							  error);
		if (read_bytes == -2)
			return BURNER_BURN_CANCEL;

		if (read_bytes == -1)
			return BURNER_BURN_ERR;

		if (!read_bytes)
			break;

		/* it can happen when we're just asked to generate a checksum
		 * that we don't need to output the received data */
		if (fd_out > 0) {
			result = burner_checksum_image_write (self,
							       fd_out,
							       buffer,
							       read_bytes, error);
			if (result != BURNER_BURN_OK)
				break;
		}

		g_checksum_update (priv->checksum,
				   buffer,
				   read_bytes);

		priv->bytes += read_bytes;
	}

	return result;
}

static BurnerBurnResult
burner_checksum_image_checksum_fd_input (BurnerChecksumImage *self,
					  GChecksumType checksum_type,
					  GError **error)
{
	int fd_in = -1;
	int fd_out = -1;
	BurnerBurnResult result;
	BurnerChecksumImagePrivate *priv;

	priv = BURNER_CHECKSUM_IMAGE_PRIVATE (self);

	BURNER_JOB_LOG (self, "Starting checksum generation live (size = %lli)", priv->total);
	result = burner_job_set_nonblocking (BURNER_JOB (self), error);
	if (result != BURNER_BURN_OK)
		return result;

	burner_job_get_fd_in (BURNER_JOB (self), &fd_in);
	burner_job_get_fd_out (BURNER_JOB (self), &fd_out);

	return burner_checksum_image_checksum (self, checksum_type, fd_in, fd_out, error);
}

static BurnerBurnResult
burner_checksum_image_checksum_file_input (BurnerChecksumImage *self,
					    GChecksumType checksum_type,
					    GError **error)
{
	BurnerChecksumImagePrivate *priv;
	BurnerBurnResult result;
	BurnerTrack *track;
	int fd_out = -1;
	int fd_in = -1;
	gchar *path;

	priv = BURNER_CHECKSUM_IMAGE_PRIVATE (self);

	/* get all information */
	burner_job_get_current_track (BURNER_JOB (self), &track);
	path = burner_track_image_get_source (BURNER_TRACK_IMAGE (track), FALSE);
	if (!path) {
		g_set_error (error,
			     BURNER_BURN_ERROR,
			     BURNER_BURN_ERROR_FILE_NOT_LOCAL,
			     _("The file is not stored locally"));
		return BURNER_BURN_ERR;
	}

	BURNER_JOB_LOG (self,
			 "Starting checksuming file %s (size = %"G_GOFFSET_FORMAT")",
			 path,
			 priv->total);

	fd_in = open (path, O_RDONLY);
	if (!fd_in) {
                int errsv;
		gchar *name = NULL;

		if (errno == ENOENT)
			return BURNER_BURN_RETRY;

		name = g_path_get_basename (path);

                errsv = errno;

		g_set_error (error,
			     BURNER_BURN_ERROR,
			     BURNER_BURN_ERROR_GENERAL,
			     /* Translators: first %s is the filename, second %s
			      * is the error generated from errno */
			     _("\"%s\" could not be opened (%s)"),
			     name,
			     g_strerror (errsv));
		g_free (name);
		g_free (path);

		return BURNER_BURN_ERR;
	}

	/* and here we go */
	burner_job_get_fd_out (BURNER_JOB (self), &fd_out);
	result = burner_checksum_image_checksum (self, checksum_type, fd_in, fd_out, error);
	g_free (path);
	close (fd_in);

	return result;
}

static BurnerBurnResult
burner_checksum_image_create_checksum (BurnerChecksumImage *self,
					GError **error)
{
	BurnerBurnResult result;
	BurnerTrack *track = NULL;
	GChecksumType checksum_type;
	BurnerChecksumImagePrivate *priv;

	priv = BURNER_CHECKSUM_IMAGE_PRIVATE (self);

	/* get the checksum type */
	switch (priv->checksum_type) {
		case BURNER_CHECKSUM_MD5:
			checksum_type = G_CHECKSUM_MD5;
			break;
		case BURNER_CHECKSUM_SHA1:
			checksum_type = G_CHECKSUM_SHA1;
			break;
		case BURNER_CHECKSUM_SHA256:
			checksum_type = G_CHECKSUM_SHA256;
			break;
		default:
			return BURNER_BURN_ERR;
	}

	burner_job_set_current_action (BURNER_JOB (self),
					BURNER_BURN_ACTION_CHECKSUM,
					_("Creating image checksum"),
					FALSE);
	burner_job_start_progress (BURNER_JOB (self), FALSE);
	burner_job_get_current_track (BURNER_JOB (self), &track);

	/* see if another plugin is sending us data to checksum
	 * or if we do it ourself (and then that must be from an
	 * image file only). */
	if (burner_job_get_fd_in (BURNER_JOB (self), NULL) == BURNER_BURN_OK) {
		BurnerMedium *medium;
		GValue *value = NULL;
		BurnerDrive *drive;
		guint64 start, end;
		goffset sectors;
		goffset bytes;

		burner_track_tag_lookup (track,
					  BURNER_TRACK_MEDIUM_ADDRESS_START_TAG,
					  &value);

		/* we were given an address to start */
		start = g_value_get_uint64 (value);

		/* get the length now */
		value = NULL;
		burner_track_tag_lookup (track,
					  BURNER_TRACK_MEDIUM_ADDRESS_END_TAG,
					  &value);

		end = g_value_get_uint64 (value);

		priv->total = end - start;

		/* we're only able to checksum ISO format at the moment so that
		 * means we can only handle last session */
		drive = burner_track_disc_get_drive (BURNER_TRACK_DISC (track));
		medium = burner_drive_get_medium (drive);
		burner_medium_get_last_data_track_space (medium,
							  &bytes,
							  &sectors);

		/* That's the only way to get the sector size */
		priv->total *= bytes / sectors;

		return burner_checksum_image_checksum_fd_input (self, checksum_type, error);
	}
	else {
		result = burner_track_get_size (track,
						 NULL,
						 &priv->total);
		if (result != BURNER_BURN_OK)
			return result;

		return burner_checksum_image_checksum_file_input (self, checksum_type, error);
	}

	return BURNER_BURN_OK;
}

static BurnerChecksumType
burner_checksum_get_checksum_type (void)
{
	GSettings *settings;
	GChecksumType checksum_type;

	settings = g_settings_new (BURNER_SCHEMA_CONFIG);
	checksum_type = g_settings_get_int (settings, BURNER_PROPS_CHECKSUM_IMAGE);
	g_object_unref (settings);

	return checksum_type;
}

static BurnerBurnResult
burner_checksum_image_image_and_checksum (BurnerChecksumImage *self,
					   GError **error)
{
	BurnerBurnResult result;
	GChecksumType checksum_type;
	BurnerChecksumImagePrivate *priv;

	priv = BURNER_CHECKSUM_IMAGE_PRIVATE (self);

	priv->checksum_type = burner_checksum_get_checksum_type ();

	if (priv->checksum_type & BURNER_CHECKSUM_MD5)
		checksum_type = G_CHECKSUM_MD5;
	else if (priv->checksum_type & BURNER_CHECKSUM_SHA1)
		checksum_type = G_CHECKSUM_SHA1;
	else if (priv->checksum_type & BURNER_CHECKSUM_SHA256)
		checksum_type = G_CHECKSUM_SHA256;
	else {
		checksum_type = G_CHECKSUM_MD5;
		priv->checksum_type = BURNER_CHECKSUM_MD5;
	}

	burner_job_set_current_action (BURNER_JOB (self),
					BURNER_BURN_ACTION_CHECKSUM,
					_("Creating image checksum"),
					FALSE);
	burner_job_start_progress (BURNER_JOB (self), FALSE);

	if (burner_job_get_fd_in (BURNER_JOB (self), NULL) != BURNER_BURN_OK) {
		BurnerTrack *track;

		burner_job_get_current_track (BURNER_JOB (self), &track);
		result = burner_track_get_size (track,
						 NULL,
						 &priv->total);
		if (result != BURNER_BURN_OK)
			return result;

		result = burner_checksum_image_checksum_file_input (self,
								     checksum_type,
								     error);
	}
	else
		result = burner_checksum_image_checksum_fd_input (self,
								   checksum_type,
								   error);

	return result;
}

struct _BurnerChecksumImageThreadCtx {
	BurnerChecksumImage *sum;
	BurnerBurnResult result;
	GError *error;
};
typedef struct _BurnerChecksumImageThreadCtx BurnerChecksumImageThreadCtx;

static gboolean
burner_checksum_image_end (gpointer data)
{
	BurnerChecksumImage *self;
	BurnerTrack *track;
	const gchar *checksum;
	BurnerBurnResult result;
	BurnerChecksumImagePrivate *priv;
	BurnerChecksumImageThreadCtx *ctx;

	ctx = data;
	self = ctx->sum;
	priv = BURNER_CHECKSUM_IMAGE_PRIVATE (self);

	/* NOTE ctx/data is destroyed in its own callback */
	priv->end_id = 0;

	if (ctx->result != BURNER_BURN_OK) {
		GError *error;

		error = ctx->error;
		ctx->error = NULL;

		g_checksum_free (priv->checksum);
		priv->checksum = NULL;

		burner_job_error (BURNER_JOB (self), error);
		return FALSE;
	}

	/* we were asked to check the sum of the track so get the type
	 * of the checksum first to see what to do */
	track = NULL;
	burner_job_get_current_track (BURNER_JOB (self), &track);

	/* Set the checksum for the track and at the same time compare it to a
	 * potential previous one. */
	checksum = g_checksum_get_string (priv->checksum);
	BURNER_JOB_LOG (self,
			 "Setting new checksum (type = %i) %s (%s before)",
			 priv->checksum_type,
			 checksum,
			 burner_track_get_checksum (track));
	result = burner_track_set_checksum (track,
					     priv->checksum_type,
					     checksum);
	g_checksum_free (priv->checksum);
	priv->checksum = NULL;

	if (result != BURNER_BURN_OK)
		goto error;

	burner_job_finished_track (BURNER_JOB (self));
	return FALSE;

error:
{
	GError *error = NULL;

	error = g_error_new (BURNER_BURN_ERROR,
			     BURNER_BURN_ERROR_BAD_CHECKSUM,
			     _("Some files may be corrupted on the disc"));
	burner_job_error (BURNER_JOB (self), error);
	return FALSE;
}
}

static void
burner_checksum_image_destroy (gpointer data)
{
	BurnerChecksumImageThreadCtx *ctx;

	ctx = data;
	if (ctx->error) {
		g_error_free (ctx->error);
		ctx->error = NULL;
	}

	g_free (ctx);
}

static gpointer
burner_checksum_image_thread (gpointer data)
{
	GError *error = NULL;
	BurnerJobAction action;
	BurnerTrack *track = NULL;
	BurnerChecksumImage *self;
	BurnerChecksumImagePrivate *priv;
	BurnerChecksumImageThreadCtx *ctx;
	BurnerBurnResult result = BURNER_BURN_NOT_SUPPORTED;

	self = BURNER_CHECKSUM_IMAGE (data);
	priv = BURNER_CHECKSUM_IMAGE_PRIVATE (self);

	/* check DISC types and add checksums for DATA and IMAGE-bin types */
	burner_job_get_action (BURNER_JOB (self), &action);
	burner_job_get_current_track (BURNER_JOB (self), &track);

	if (action == BURNER_JOB_ACTION_CHECKSUM) {
		priv->checksum_type = burner_track_get_checksum_type (track);
		if (priv->checksum_type & (BURNER_CHECKSUM_MD5|BURNER_CHECKSUM_SHA1|BURNER_CHECKSUM_SHA256))
			result = burner_checksum_image_create_checksum (self, &error);
		else
			result = BURNER_BURN_ERR;
	}
	else if (action == BURNER_JOB_ACTION_IMAGE) {
		BurnerTrackType *input;

		input = burner_track_type_new ();
		burner_job_get_input_type (BURNER_JOB (self), input);

		if (burner_track_type_get_has_image (input))
			result = burner_checksum_image_image_and_checksum (self, &error);
		else
			result = BURNER_BURN_ERR;

		burner_track_type_free (input);
	}

	if (result != BURNER_BURN_CANCEL) {
		ctx = g_new0 (BurnerChecksumImageThreadCtx, 1);
		ctx->sum = self;
		ctx->error = error;
		ctx->result = result;
		priv->end_id = g_idle_add_full (G_PRIORITY_HIGH_IDLE,
						burner_checksum_image_end,
						ctx,
						burner_checksum_image_destroy);
	}

	/* End thread */
	g_mutex_lock (priv->mutex);
	priv->thread = NULL;
	g_cond_signal (priv->cond);
	g_mutex_unlock (priv->mutex);

	g_thread_exit (NULL);
	return NULL;
}

static BurnerBurnResult
burner_checksum_image_start (BurnerJob *job,
			      GError **error)
{
	BurnerChecksumImagePrivate *priv;
	GError *thread_error = NULL;
	BurnerJobAction action;

	burner_job_get_action (job, &action);
	if (action == BURNER_JOB_ACTION_SIZE) {
		/* say we won't write to disc if we're just checksuming "live" */
		if (burner_job_get_fd_in (job, NULL) == BURNER_BURN_OK)
			return BURNER_BURN_NOT_SUPPORTED;

		/* otherwise return an output of 0 since we're not actually 
		 * writing anything to the disc. That will prevent a disc space
		 * failure. */
		burner_job_set_output_size_for_current_track (job, 0, 0);
		return BURNER_BURN_NOT_RUNNING;
	}

	/* we start a thread for the exploration of the graft points */
	priv = BURNER_CHECKSUM_IMAGE_PRIVATE (job);
	g_mutex_lock (priv->mutex);
	priv->thread = g_thread_create (burner_checksum_image_thread,
					BURNER_CHECKSUM_IMAGE (job),
					FALSE,
					&thread_error);
	g_mutex_unlock (priv->mutex);

	/* Reminder: this is not necessarily an error as the thread may have finished */
	//if (!priv->thread)
	//	return BURNER_BURN_ERR;
	if (thread_error) {
		g_propagate_error (error, thread_error);
		return BURNER_BURN_ERR;
	}

	return BURNER_BURN_OK;
}

static BurnerBurnResult
burner_checksum_image_activate (BurnerJob *job,
				 GError **error)
{
	BurnerBurnFlag flags = BURNER_BURN_FLAG_NONE;
	BurnerTrack *track = NULL;
	BurnerJobAction action;

	burner_job_get_current_track (job, &track);
	burner_job_get_action (job, &action);

	if (action == BURNER_JOB_ACTION_IMAGE
	&&  burner_track_get_checksum_type (track) != BURNER_CHECKSUM_NONE
	&&  burner_track_get_checksum_type (track) == burner_checksum_get_checksum_type ()) {
		BURNER_JOB_LOG (job,
				 "There is a checksum already %d",
				 burner_track_get_checksum_type (track));
		/* if there is a checksum already, if so no need to redo one */
		return BURNER_BURN_NOT_RUNNING;
	}

	flags = BURNER_BURN_FLAG_NONE;
	burner_job_get_flags (job, &flags);
	if (flags & BURNER_BURN_FLAG_DUMMY) {
		BURNER_JOB_LOG (job, "Dummy operation, skipping");
		return BURNER_BURN_NOT_RUNNING;
	}

	return BURNER_BURN_OK;
}

static BurnerBurnResult
burner_checksum_image_clock_tick (BurnerJob *job)
{
	BurnerChecksumImagePrivate *priv;

	priv = BURNER_CHECKSUM_IMAGE_PRIVATE (job);

	if (!priv->checksum)
		return BURNER_BURN_OK;

	if (!priv->total)
		return BURNER_BURN_OK;

	burner_job_start_progress (job, FALSE);
	burner_job_set_progress (job,
				  (gdouble) priv->bytes /
				  (gdouble) priv->total);

	return BURNER_BURN_OK;
}

static BurnerBurnResult
burner_checksum_image_stop (BurnerJob *job,
			     GError **error)
{
	BurnerChecksumImagePrivate *priv;

	priv = BURNER_CHECKSUM_IMAGE_PRIVATE (job);

	g_mutex_lock (priv->mutex);
	if (priv->thread) {
		priv->cancel = 1;
		g_cond_wait (priv->cond, priv->mutex);
		priv->cancel = 0;
		priv->thread = NULL;
	}
	g_mutex_unlock (priv->mutex);

	if (priv->end_id) {
		g_source_remove (priv->end_id);
		priv->end_id = 0;
	}

	if (priv->checksum) {
		g_checksum_free (priv->checksum);
		priv->checksum = NULL;
	}

	return BURNER_BURN_OK;
}

static void
burner_checksum_image_init (BurnerChecksumImage *obj)
{
	BurnerChecksumImagePrivate *priv;

	priv = BURNER_CHECKSUM_IMAGE_PRIVATE (obj);

	priv->mutex = g_mutex_new ();
	priv->cond = g_cond_new ();
}

static void
burner_checksum_image_finalize (GObject *object)
{
	BurnerChecksumImagePrivate *priv;
	
	priv = BURNER_CHECKSUM_IMAGE_PRIVATE (object);

	g_mutex_lock (priv->mutex);
	if (priv->thread) {
		priv->cancel = 1;
		g_cond_wait (priv->cond, priv->mutex);
		priv->cancel = 0;
		priv->thread = NULL;
	}
	g_mutex_unlock (priv->mutex);

	if (priv->end_id) {
		g_source_remove (priv->end_id);
		priv->end_id = 0;
	}

	if (priv->checksum) {
		g_checksum_free (priv->checksum);
		priv->checksum = NULL;
	}

	if (priv->mutex) {
		g_mutex_free (priv->mutex);
		priv->mutex = NULL;
	}

	if (priv->cond) {
		g_cond_free (priv->cond);
		priv->cond = NULL;
	}

	G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
burner_checksum_image_class_init (BurnerChecksumImageClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);
	BurnerJobClass *job_class = BURNER_JOB_CLASS (klass);

	g_type_class_add_private (klass, sizeof (BurnerChecksumImagePrivate));

	parent_class = g_type_class_peek_parent (klass);
	object_class->finalize = burner_checksum_image_finalize;

	job_class->activate = burner_checksum_image_activate;
	job_class->start = burner_checksum_image_start;
	job_class->stop = burner_checksum_image_stop;
	job_class->clock_tick = burner_checksum_image_clock_tick;
}

static void
burner_checksum_image_export_caps (BurnerPlugin *plugin)
{
	GSList *input;
	BurnerPluginConfOption *checksum_type;

	burner_plugin_define (plugin,
	                       "image-checksum",
			       /* Translators: this is the name of the plugin
				* which will be translated only when it needs
				* displaying. */
			       N_("Image Checksum"),
			       _("Checks disc integrity after it is burnt"),
			       "Philippe Rouquier",
			       0);

	/* For images we can process (thus generating a sum on the fly or simply
	 * test images). */
	input = burner_caps_image_new (BURNER_PLUGIN_IO_ACCEPT_FILE|
					BURNER_PLUGIN_IO_ACCEPT_PIPE,
					BURNER_IMAGE_FORMAT_BIN);
	burner_plugin_process_caps (plugin, input);

	burner_plugin_set_process_flags (plugin,
					  BURNER_PLUGIN_RUN_PREPROCESSING|
					  BURNER_PLUGIN_RUN_BEFORE_TARGET);

	burner_plugin_check_caps (plugin,
				   BURNER_CHECKSUM_MD5|
				   BURNER_CHECKSUM_SHA1|
				   BURNER_CHECKSUM_SHA256,
				   input);
	g_slist_free (input);

	/* add some configure options */
	checksum_type = burner_plugin_conf_option_new (BURNER_PROPS_CHECKSUM_IMAGE,
							_("Hashing algorithm to be used:"),
							BURNER_PLUGIN_OPTION_CHOICE);
	burner_plugin_conf_option_choice_add (checksum_type,
					       _("MD5"), BURNER_CHECKSUM_MD5);
	burner_plugin_conf_option_choice_add (checksum_type,
					       _("SHA1"), BURNER_CHECKSUM_SHA1);
	burner_plugin_conf_option_choice_add (checksum_type,
					       _("SHA256"), BURNER_CHECKSUM_SHA256);

	burner_plugin_add_conf_option (plugin, checksum_type);

	burner_plugin_set_compulsory (plugin, FALSE);
}
