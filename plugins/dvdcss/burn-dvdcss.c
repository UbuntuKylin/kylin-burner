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
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>

#include <glib.h>
#include <glib-object.h>
#include <glib/gstdio.h>
#include <glib/gi18n-lib.h>
#include <gmodule.h>

#include "burner-units.h"

#include "burn-job.h"
#include "burner-plugin-registration.h"
#include "burn-dvdcss-private.h"
#include "burn-volume.h"
#include "burner-medium.h"
#include "burner-track-image.h"
#include "burner-track-disc.h"


#define BURNER_TYPE_DVDCSS         (burner_dvdcss_get_type ())
#define BURNER_DVDCSS(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), BURNER_TYPE_DVDCSS, BurnerDvdcss))
#define BURNER_DVDCSS_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k), BURNER_TYPE_DVDCSS, BurnerDvdcssClass))
#define BURNER_IS_DVDCSS(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), BURNER_TYPE_DVDCSS))
#define BURNER_IS_DVDCSS_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), BURNER_TYPE_DVDCSS))
#define BURNER_DVDCSS_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), BURNER_TYPE_DVDCSS, BurnerDvdcssClass))

BURNER_PLUGIN_BOILERPLATE (BurnerDvdcss, burner_dvdcss, BURNER_TYPE_JOB, BurnerJob);

struct _BurnerDvdcssPrivate {
	GError *error;
	GThread *thread;
	GMutex *mutex;
	GCond *cond;
	guint thread_id;

	guint cancel:1;
};
typedef struct _BurnerDvdcssPrivate BurnerDvdcssPrivate;

#define BURNER_DVDCSS_PRIVATE(o)  (G_TYPE_INSTANCE_GET_PRIVATE ((o), BURNER_TYPE_DVDCSS, BurnerDvdcssPrivate))

#define BURNER_DVDCSS_I_BLOCKS	16ULL

static GObjectClass *parent_class = NULL;

static gboolean
burner_dvdcss_library_init (BurnerPlugin *plugin)
{
	gpointer address;
	GModule *module;

	if (css_ready)
		return TRUE;

	/* load libdvdcss library and see the version (mine is 1.2.0) */
	module = g_module_open ("libdvdcss.so.2", G_MODULE_BIND_LOCAL);
	if (!module)
		goto error_doesnt_exist;

	if (!g_module_symbol (module, "dvdcss_open", &address))
		goto error_version;
	dvdcss_open = address;

	if (!g_module_symbol (module, "dvdcss_close", &address))
		goto error_version;
	dvdcss_close = address;

	if (!g_module_symbol (module, "dvdcss_read", &address))
		goto error_version;
	dvdcss_read = address;

	if (!g_module_symbol (module, "dvdcss_seek", &address))
		goto error_version;
	dvdcss_seek = address;

	if (!g_module_symbol (module, "dvdcss_error", &address))
		goto error_version;
	dvdcss_error = address;

	if (plugin) {
		g_module_close (module);
		return TRUE;
	}

	css_ready = TRUE;
	return TRUE;

error_doesnt_exist:
	burner_plugin_add_error (plugin,
	                          BURNER_PLUGIN_ERROR_MISSING_LIBRARY,
	                          "libdvdcss.so.2");
	return FALSE;

error_version:
	burner_plugin_add_error (plugin,
	                          BURNER_PLUGIN_ERROR_LIBRARY_VERSION,
	                          "libdvdcss.so.2");
	g_module_close (module);
	return FALSE;
}

static gboolean
burner_dvdcss_thread_finished (gpointer data)
{
	goffset blocks = 0;
	gchar *image = NULL;
	BurnerDvdcss *self = data;
	BurnerDvdcssPrivate *priv;
	BurnerTrackImage *track = NULL;

	priv = BURNER_DVDCSS_PRIVATE (self);
	priv->thread_id = 0;

	if (priv->error) {
		GError *error;

		error = priv->error;
		priv->error = NULL;
		burner_job_error (BURNER_JOB (self), error);
		return FALSE;
	}

	track = burner_track_image_new ();
	burner_job_get_image_output (BURNER_JOB (self),
				      &image,
				      NULL);
	burner_track_image_set_source (track,
					image,
					NULL,
					BURNER_IMAGE_FORMAT_BIN);

	burner_job_get_session_output_size (BURNER_JOB (self), &blocks, NULL);
	burner_track_image_set_block_num (track, blocks);

	burner_job_add_track (BURNER_JOB (self), BURNER_TRACK (track));

	/* It's good practice to unref the track afterwards as we don't need it
	 * anymore. BurnerTaskCtx refs it. */
	g_object_unref (track);

	burner_job_finished_track (BURNER_JOB (self));

	return FALSE;
}

static BurnerBurnResult
burner_dvdcss_write_sector_to_fd (BurnerDvdcss *self,
				   gpointer buffer,
				   gint bytes_remaining)
{
	int fd;
	gint bytes_written = 0;
	BurnerDvdcssPrivate *priv;

	priv = BURNER_DVDCSS_PRIVATE (self);

	burner_job_get_fd_out (BURNER_JOB (self), &fd);
	while (bytes_remaining) {
		gint written;

		written = write (fd,
				 ((gchar *) buffer)  + bytes_written,
				 bytes_remaining);

		if (priv->cancel)
			break;

		if (written != bytes_remaining) {
			if (errno != EINTR && errno != EAGAIN) {
                                int errsv = errno;

				/* unrecoverable error */
				priv->error = g_error_new (BURNER_BURN_ERROR,
							   BURNER_BURN_ERROR_GENERAL,
							   _("Data could not be written (%s)"),
							   g_strerror (errsv));
				return BURNER_BURN_ERR;
			}

			g_thread_yield ();
		}

		if (written > 0) {
			bytes_remaining -= written;
			bytes_written += written;
		}
	}

	return BURNER_BURN_OK;
}

struct _BurnerScrambledSectorRange {
	gint start;
	gint end;
};
typedef struct _BurnerScrambledSectorRange BurnerScrambledSectorRange;

static gboolean
burner_dvdcss_create_scrambled_sectors_map (BurnerDvdcss *self,
					     BurnerDrive *drive,
                                             GQueue *map,
					     dvdcss_handle *handle,
					     BurnerVolFile *parent,
					     GError **error)
{
	GList *iter;

	/* this allows one to cache keys for encrypted files */
	for (iter = parent->specific.dir.children; iter; iter = iter->next) {
		BurnerVolFile *file;

		file = iter->data;
		if (!file->isdir) {
			if (!strncmp (file->name + strlen (file->name) - 6, ".VOB", 4)) {
				BurnerScrambledSectorRange *range;
				gsize current_extent;
				GSList *extents;

				BURNER_JOB_LOG (self, "Retrieving keys for %s", file->name);

				/* take the first address for each extent of the file */
				if (!file->specific.file.extents) {
					BURNER_JOB_LOG (self, "Problem: file has no extents");
					return FALSE;
				}

				range = g_new0 (BurnerScrambledSectorRange, 1);
				for (extents = file->specific.file.extents; extents; extents = extents->next) {
					BurnerVolFileExtent *extent;

					extent = extents->data;

					range->start = extent->block;
					range->end = extent->block + BURNER_BYTES_TO_SECTORS (extent->size, DVDCSS_BLOCK_SIZE);

					BURNER_JOB_LOG (self, "From 0x%llx to 0x%llx", range->start, range->end);
					g_queue_push_head (map, range);

					if (extent->size == 0) {
						BURNER_JOB_LOG (self, "0 size extent");
						continue;
					}

					current_extent = dvdcss_seek (handle, range->start, DVDCSS_SEEK_KEY);
					if (current_extent != range->start) {
						BURNER_JOB_LOG (self, "Problem: could not retrieve key");
						g_set_error (error,
							     BURNER_BURN_ERROR,
							     BURNER_BURN_ERROR_GENERAL,
							     /* Translators: %s is the path to a drive. "regionset %s"
							      * should be left as is just like "DVDCSS_METHOD=title
							      * burner --no-existing-session" */
							     _("Error while retrieving a key used for encryption. You may solve such a problem with one of the following methods: in a terminal either set the proper DVD region code for your CD/DVD player with the \"regionset %s\" command or run the \"DVDCSS_METHOD=title burner --no-existing-session\" command"),
							     burner_drive_get_device (drive));
						return FALSE;
					}
				}
			}
		}
		else if (!burner_dvdcss_create_scrambled_sectors_map (self, drive, map, handle, file, error))
			return FALSE;
	}

	return TRUE;
}

static gint
burner_dvdcss_sort_ranges (gconstpointer a, gconstpointer b, gpointer user_data)
{
	const BurnerScrambledSectorRange *range_a = a;
	const BurnerScrambledSectorRange *range_b = b;

	return range_a->start - range_b->start;
}

static gpointer
burner_dvdcss_write_image_thread (gpointer data)
{
	guchar buf [DVDCSS_BLOCK_SIZE * BURNER_DVDCSS_I_BLOCKS];
	BurnerScrambledSectorRange *range = NULL;
	BurnerMedium *medium = NULL;
	BurnerVolFile *files = NULL;
	dvdcss_handle *handle = NULL;
	BurnerDrive *drive = NULL;
	BurnerDvdcssPrivate *priv;
	gint64 written_sectors = 0;
	BurnerDvdcss *self = data;
	BurnerTrack *track = NULL;
	guint64 remaining_sectors;
	FILE *output_fd = NULL;
	BurnerVolSrc *vol;
	gint64 volume_size;
	GQueue *map = NULL;

	burner_job_set_use_average_rate (BURNER_JOB (self), TRUE);
	burner_job_set_current_action (BURNER_JOB (self),
					BURNER_BURN_ACTION_ANALYSING,
					_("Retrieving DVD keys"),
					FALSE);
	burner_job_start_progress (BURNER_JOB (self), FALSE);

	priv = BURNER_DVDCSS_PRIVATE (self);

	/* get the contents of the DVD */
	burner_job_get_current_track (BURNER_JOB (self), &track);
	drive = burner_track_disc_get_drive (BURNER_TRACK_DISC (track));

	vol = burner_volume_source_open_file (burner_drive_get_device (drive), &priv->error);
	files = burner_volume_get_files (vol,
					  0,
					  NULL,
					  NULL,
					  NULL,
					  &priv->error);
	burner_volume_source_close (vol);
	if (!files)
		goto end;

	medium = burner_drive_get_medium (drive);
	burner_medium_get_data_size (medium, NULL, &volume_size);
	if (volume_size == -1) {
		priv->error = g_error_new (BURNER_BURN_ERROR,
					   BURNER_BURN_ERROR_GENERAL,
					   _("The size of the volume could not be retrieved"));
		goto end;
	}

	/* create a handle/open DVD */
	handle = dvdcss_open (burner_drive_get_device (drive));
	if (!handle) {
		priv->error = g_error_new (BURNER_BURN_ERROR,
					   BURNER_BURN_ERROR_GENERAL,
					   _("Video DVD could not be opened"));
		goto end;
	}

	/* look through the files to get the ranges of encrypted sectors
	 * and cache the CSS keys while at it. */
	map = g_queue_new ();
	if (!burner_dvdcss_create_scrambled_sectors_map (self, drive, map, handle, files, &priv->error))
		goto end;

	BURNER_JOB_LOG (self, "DVD map created (keys retrieved)");

	g_queue_sort (map, burner_dvdcss_sort_ranges, NULL);

	burner_volume_file_free (files);
	files = NULL;

	if (dvdcss_seek (handle, 0, DVDCSS_NOFLAGS) < 0) {
		BURNER_JOB_LOG (self, "Error initial seeking");
		priv->error = g_error_new (BURNER_BURN_ERROR,
					   BURNER_BURN_ERROR_GENERAL,
					   _("Error while reading video DVD (%s)"),
					   dvdcss_error (handle));
		goto end;
	}

	burner_job_set_current_action (BURNER_JOB (self),
					BURNER_BURN_ACTION_DRIVE_COPY,
					_("Copying video DVD"),
					FALSE);

	burner_job_start_progress (BURNER_JOB (self), TRUE);

	remaining_sectors = volume_size;
	range = g_queue_pop_head (map);

	if (burner_job_get_fd_out (BURNER_JOB (self), NULL) != BURNER_BURN_OK) {
		gchar *output = NULL;

		burner_job_get_image_output (BURNER_JOB (self), &output, NULL);
		output_fd = fopen (output, "w");
		if (!output_fd) {
			priv->error = g_error_new_literal (BURNER_BURN_ERROR,
							   BURNER_BURN_ERROR_GENERAL,
							   g_strerror (errno));
			g_free (output);
			goto end;
		}
		g_free (output);
	}

	while (remaining_sectors) {
		gint flag;
		guint64 num_blocks, data_size;

		if (priv->cancel)
			break;

		num_blocks = BURNER_DVDCSS_I_BLOCKS;

		/* see if we are approaching the end of the dvd */
		if (num_blocks > remaining_sectors)
			num_blocks = remaining_sectors;

		/* see if we need to update the key */
		if (!range || written_sectors < range->start) {
			/* this is in a non scrambled sectors range */
			flag = DVDCSS_NOFLAGS;
	
			/* we don't want to mix scrambled and non scrambled sectors */
			if (range && written_sectors + num_blocks > range->start)
				num_blocks = range->start - written_sectors;
		}
		else {
			/* this is in a scrambled sectors range */
			flag = DVDCSS_READ_DECRYPT;

			/* see if we need to update the key */
			if (written_sectors == range->start) {
				int pos;

				pos = dvdcss_seek (handle, written_sectors, DVDCSS_SEEK_KEY);
				if (pos < 0) {
					BURNER_JOB_LOG (self, "Error seeking");
					priv->error = g_error_new (BURNER_BURN_ERROR,
								   BURNER_BURN_ERROR_GENERAL,
								   _("Error while reading video DVD (%s)"),
								   dvdcss_error (handle));
					break;
				}
			}

			/* we don't want to mix scrambled and non scrambled sectors
			 * NOTE: range->end address is the next non scrambled sector */
			if (written_sectors + num_blocks > range->end)
				num_blocks = range->end - written_sectors;

			if (written_sectors + num_blocks == range->end) {
				/* update to get the next range of scrambled sectors */
				g_free (range);
				range = g_queue_pop_head (map);
			}
		}

		num_blocks = dvdcss_read (handle, buf, num_blocks, flag);
		if (num_blocks < 0) {
			BURNER_JOB_LOG (self, "Error reading");
			priv->error = g_error_new (BURNER_BURN_ERROR,
						   BURNER_BURN_ERROR_GENERAL,
						   _("Error while reading video DVD (%s)"),
						   dvdcss_error (handle));
			break;
		}

		data_size = num_blocks * DVDCSS_BLOCK_SIZE;
		if (output_fd) {
			if (fwrite (buf, 1, data_size, output_fd) != data_size) {
                                int errsv = errno;

				priv->error = g_error_new (BURNER_BURN_ERROR,
							   BURNER_BURN_ERROR_GENERAL,
							   _("Data could not be written (%s)"),
							   g_strerror (errsv));
				break;
			}
		}
		else {
			BurnerBurnResult result;

			result = burner_dvdcss_write_sector_to_fd (self,
								    buf,
								    data_size);
			if (result != BURNER_BURN_OK)
				break;
		}

		written_sectors += num_blocks;
		remaining_sectors -= num_blocks;
		burner_job_set_written_track (BURNER_JOB (self), written_sectors * DVDCSS_BLOCK_SIZE);
	}

end:

	if (range)
		g_free (range);

	if (handle)
		dvdcss_close (handle);

	if (files)
		burner_volume_file_free (files);

	if (output_fd)
		fclose (output_fd);

	if (map) {
		g_queue_foreach (map, (GFunc) g_free, NULL);
		g_queue_free (map);
	}

	if (!priv->cancel)
		priv->thread_id = g_idle_add (burner_dvdcss_thread_finished, self);

	/* End thread */
	g_mutex_lock (priv->mutex);
	priv->thread = NULL;
	g_cond_signal (priv->cond);
	g_mutex_unlock (priv->mutex);

	g_thread_exit (NULL);

	return NULL;
}

static BurnerBurnResult
burner_dvdcss_start (BurnerJob *job,
		      GError **error)
{
	BurnerDvdcss *self;
	BurnerJobAction action;
	BurnerDvdcssPrivate *priv;
	GError *thread_error = NULL;

	self = BURNER_DVDCSS (job);
	priv = BURNER_DVDCSS_PRIVATE (self);

	burner_job_get_action (job, &action);
	if (action == BURNER_JOB_ACTION_SIZE) {
		goffset blocks = 0;
		BurnerTrack *track;

		burner_job_get_current_track (job, &track);
		burner_track_get_size (track, &blocks, NULL);
		burner_job_set_output_size_for_current_track (job,
							       blocks,
							       blocks * DVDCSS_BLOCK_SIZE);
		return BURNER_BURN_NOT_RUNNING;
	}

	if (action != BURNER_JOB_ACTION_IMAGE)
		return BURNER_BURN_NOT_SUPPORTED;

	if (priv->thread)
		return BURNER_BURN_RUNNING;

	if (!burner_dvdcss_library_init (NULL))
		return BURNER_BURN_ERR;

	g_mutex_lock (priv->mutex);
	priv->thread = g_thread_create (burner_dvdcss_write_image_thread,
					self,
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

static void
burner_dvdcss_stop_real (BurnerDvdcss *self)
{
	BurnerDvdcssPrivate *priv;

	priv = BURNER_DVDCSS_PRIVATE (self);

	g_mutex_lock (priv->mutex);
	if (priv->thread) {
		priv->cancel = 1;
		g_cond_wait (priv->cond, priv->mutex);
		priv->cancel = 0;
	}
	g_mutex_unlock (priv->mutex);

	if (priv->thread_id) {
		g_source_remove (priv->thread_id);
		priv->thread_id = 0;
	}

	if (priv->error) {
		g_error_free (priv->error);
		priv->error = NULL;
	}
}

static BurnerBurnResult
burner_dvdcss_stop (BurnerJob *job,
		     GError **error)
{
	BurnerDvdcss *self;

	self = BURNER_DVDCSS (job);

	burner_dvdcss_stop_real (self);
	return BURNER_BURN_OK;
}

static void
burner_dvdcss_class_init (BurnerDvdcssClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);
	BurnerJobClass *job_class = BURNER_JOB_CLASS (klass);

	g_type_class_add_private (klass, sizeof (BurnerDvdcssPrivate));

	parent_class = g_type_class_peek_parent (klass);
	object_class->finalize = burner_dvdcss_finalize;

	job_class->start = burner_dvdcss_start;
	job_class->stop = burner_dvdcss_stop;
}

static void
burner_dvdcss_init (BurnerDvdcss *obj)
{
	BurnerDvdcssPrivate *priv;

	priv = BURNER_DVDCSS_PRIVATE (obj);

	priv->mutex = g_mutex_new ();
	priv->cond = g_cond_new ();
}

static void
burner_dvdcss_finalize (GObject *object)
{
	BurnerDvdcssPrivate *priv;

	priv = BURNER_DVDCSS_PRIVATE (object);

	burner_dvdcss_stop_real (BURNER_DVDCSS (object));

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
burner_dvdcss_export_caps (BurnerPlugin *plugin)
{
	GSList *output;
	GSList *input;

	burner_plugin_define (plugin,
			       "dvdcss",
	                       NULL,
			       _("Copies CSS encrypted video DVDs to a disc image"),
			       "Philippe Rouquier",
			       0);

	/* to my knowledge, css can only be applied to pressed discs so no need
	 * to specify anything else but ROM */
	output = burner_caps_image_new (BURNER_PLUGIN_IO_ACCEPT_FILE|
					 BURNER_PLUGIN_IO_ACCEPT_PIPE,
					 BURNER_IMAGE_FORMAT_BIN);
	input = burner_caps_disc_new (BURNER_MEDIUM_DVD|
				       BURNER_MEDIUM_DUAL_L|
				       BURNER_MEDIUM_ROM|
				       BURNER_MEDIUM_CLOSED|
				       BURNER_MEDIUM_HAS_DATA|
				       BURNER_MEDIUM_PROTECTED);

	burner_plugin_link_caps (plugin, output, input);

	g_slist_free (input);
	g_slist_free (output);
}

G_MODULE_EXPORT void
burner_plugin_check_config (BurnerPlugin *plugin)
{
	burner_dvdcss_library_init (plugin);
}
