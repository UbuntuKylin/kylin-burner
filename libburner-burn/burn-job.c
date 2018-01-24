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
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/resource.h>

#include <glib.h>
#include <glib-object.h>
#include <glib/gi18n-lib.h>

#include <gio/gio.h>

#include "burner-drive.h"
#include "burner-medium.h"

#include "burn-basics.h"
#include "burn-debug.h"
#include "burner-session.h"
#include "burner-session-helper.h"
#include "burner-plugin-information.h"
#include "burn-job.h"
#include "burn-task-ctx.h"
#include "burn-task-item.h"
#include "libburner-marshal.h"

#include "burner-track-type-private.h"

typedef struct _BurnerJobOutput {
	gchar *image;
	gchar *toc;
} BurnerJobOutput;

typedef struct _BurnerJobInput {
	int out;
	int in;
} BurnerJobInput;

static void burner_job_iface_init_task_item (BurnerTaskItemIFace *iface);
G_DEFINE_TYPE_WITH_CODE (BurnerJob, burner_job, G_TYPE_OBJECT,
			 G_IMPLEMENT_INTERFACE (BURNER_TYPE_TASK_ITEM,
						burner_job_iface_init_task_item));

typedef struct BurnerJobPrivate BurnerJobPrivate;
struct BurnerJobPrivate {
	BurnerJob *next;
	BurnerJob *previous;

	BurnerTaskCtx *ctx;

	/* used if job reads data from a pipe */
	BurnerJobInput *input;

	/* output type (sets at construct time) */
	BurnerTrackType type;

	/* used if job writes data to a pipe (link is then NULL) */
	BurnerJobOutput *output;
	BurnerJob *linked;
};

#define BURNER_JOB_DEBUG(job_MACRO)						\
{										\
	const gchar *class_name_MACRO = NULL;					\
	if (BURNER_IS_JOB (job_MACRO))						\
		class_name_MACRO = G_OBJECT_TYPE_NAME (job_MACRO);		\
	burner_job_log_message (job_MACRO, G_STRLOC,				\
				 "%s called %s", 				\
				 class_name_MACRO,				\
				 G_STRFUNC);					\
}

#define BURNER_JOB_PRIVATE(o)  (G_TYPE_INSTANCE_GET_PRIVATE ((o), BURNER_TYPE_JOB, BurnerJobPrivate))

enum {
	PROP_NONE,
	PROP_OUTPUT,
};

typedef enum {
	ERROR_SIGNAL,
	LAST_SIGNAL
} BurnerJobSignalType;

static guint burner_job_signals [LAST_SIGNAL] = { 0 };

static GObjectClass *parent_class = NULL;


/**
 * Task item virtual functions implementation
 */

static BurnerTaskItem *
burner_job_item_next (BurnerTaskItem *item)
{
	BurnerJob *self;
	BurnerJobPrivate *priv;

	self = BURNER_JOB (item);
	priv = BURNER_JOB_PRIVATE (self);

	if (!priv->next)
		return NULL;

	return BURNER_TASK_ITEM (priv->next);
}

static BurnerTaskItem *
burner_job_item_previous (BurnerTaskItem *item)
{
	BurnerJob *self;
	BurnerJobPrivate *priv;

	self = BURNER_JOB (item);
	priv = BURNER_JOB_PRIVATE (self);

	if (!priv->previous)
		return NULL;

	return BURNER_TASK_ITEM (priv->previous);
}

static BurnerBurnResult
burner_job_item_link (BurnerTaskItem *input,
		       BurnerTaskItem *output)
{
	BurnerJobPrivate *priv_input;
	BurnerJobPrivate *priv_output;

	priv_input = BURNER_JOB_PRIVATE (input);
	priv_output = BURNER_JOB_PRIVATE (output);

	priv_input->next = BURNER_JOB (output);
	priv_output->previous = BURNER_JOB (input);

	g_object_ref (input);
	return BURNER_BURN_OK;
}

static gboolean
burner_job_is_last_active (BurnerJob *self)
{
	BurnerJobPrivate *priv;
	BurnerJob *next;

	priv = BURNER_JOB_PRIVATE (self);
	if (!priv->ctx)
		return FALSE;

	next = priv->next;
	while (next) {
		priv = BURNER_JOB_PRIVATE (next);
		if (priv->ctx)
			return FALSE;
		next = priv->next;
	}

	return TRUE;
}

static gboolean
burner_job_is_first_active (BurnerJob *self)
{
	BurnerJobPrivate *priv;
	BurnerJob *prev;

	priv = BURNER_JOB_PRIVATE (self);
	if (!priv->ctx)
		return FALSE;

	prev = priv->previous;
	while (prev) {
		priv = BURNER_JOB_PRIVATE (prev);
		if (priv->ctx)
			return FALSE;
		prev = priv->previous;
	}

	return TRUE;
}

static void
burner_job_input_free (BurnerJobInput *input)
{
	if (!input)
		return;

	if (input->in > 0)
		close (input->in);

	if (input->out > 0)
		close (input->out);

	g_free (input);
}

static void
burner_job_output_free (BurnerJobOutput *output)
{
	if (!output)
		return;

	if (output->image) {
		g_free (output->image);
		output->image = NULL;
	}

	if (output->toc) {
		g_free (output->toc);
		output->toc = NULL;
	}

	g_free (output);
}

static void
burner_job_deactivate (BurnerJob *self)
{
	BurnerJobPrivate *priv;

	priv = BURNER_JOB_PRIVATE (self);

	BURNER_JOB_LOG (self, "deactivating");

	/* ::start hasn't been called yet */
	if (priv->ctx) {
		g_object_unref (priv->ctx);
		priv->ctx = NULL;
	}

	if (priv->input) {
		burner_job_input_free (priv->input);
		priv->input = NULL;
	}

	if (priv->output) {
		burner_job_output_free (priv->output);
		priv->output = NULL;
	}

	if (priv->linked)
		priv->linked = NULL;
}

static BurnerBurnResult
burner_job_allow_deactivation (BurnerJob *self,
				BurnerBurnSession *session,
				GError **error)
{
	BurnerJobPrivate *priv;
	BurnerTrackType input;

	priv = BURNER_JOB_PRIVATE (self);

	/* This job refused to work. This is allowed in three cases:
	 * - the job is the only one in the task (no other job linked) and the
	 *   track type as input is the same as the track type of the output
	 *   except if type is DISC as input or output
	 * - if the job hasn't got any job linked before the next linked job
	 *   accepts the track type of the session as input
	 * - the output of the previous job is the same as this job output type
	 */

	/* there can't be two recorders in a row so ... */
	if (priv->type.type == BURNER_TRACK_TYPE_DISC)
		goto error;

	if (priv->previous) {
		BurnerJobPrivate *previous;
		previous = BURNER_JOB_PRIVATE (priv->previous);
		memcpy (&input, &previous->type, sizeof (BurnerTrackType));
	}
	else
		burner_burn_session_get_input_type (session, &input);

	if (!burner_track_type_equal (&input, &priv->type))
		goto error;

	return BURNER_BURN_NOT_RUNNING;

error:

	g_set_error (error,
		     BURNER_BURN_ERR,
		     BURNER_BURN_ERROR_PLUGIN_MISBEHAVIOR,
		     /* Translators: %s is the plugin name */
		     _("\"%s\" did not behave properly"),
		     G_OBJECT_TYPE_NAME (self));
	return BURNER_BURN_ERR;
}

static BurnerBurnResult
burner_job_item_activate (BurnerTaskItem *item,
			   BurnerTaskCtx *ctx,
			   GError **error)
{
	BurnerJob *self;
	BurnerJobClass *klass;
	BurnerJobPrivate *priv;
	BurnerBurnSession *session;
	BurnerBurnResult result = BURNER_BURN_OK;

	self = BURNER_JOB (item);
	priv = BURNER_JOB_PRIVATE (self);
	session = burner_task_ctx_get_session (ctx);

	g_object_ref (ctx);
	priv->ctx = ctx;

	klass = BURNER_JOB_GET_CLASS (self);

	/* see if this job needs to be deactivated (if no function then OK) */
	if (klass->activate)
		result = klass->activate (self, error);
	else
		BURNER_BURN_LOG ("no ::activate method %s",
				  G_OBJECT_TYPE_NAME (item));

	if (result != BURNER_BURN_OK) {
		g_object_unref (ctx);
		priv->ctx = NULL;

		if (result == BURNER_BURN_NOT_RUNNING)
			result = burner_job_allow_deactivation (self, session, error);

		return result;
	}

	return BURNER_BURN_OK;
}

static gboolean
burner_job_item_is_active (BurnerTaskItem *item)
{
	BurnerJob *self;
	BurnerJobPrivate *priv;

	self = BURNER_JOB (item);
	priv = BURNER_JOB_PRIVATE (self);

	return (priv->ctx != NULL);
}

static BurnerBurnResult
burner_job_check_output_disc_space (BurnerJob *self,
				     GError **error)
{
	BurnerBurnSession *session;
	goffset output_blocks = 0;
	goffset media_blocks = 0;
	BurnerJobPrivate *priv;
	BurnerBurnFlag flags;
	BurnerMedium *medium;
	BurnerDrive *drive;

	priv = BURNER_JOB_PRIVATE (self);

	burner_task_ctx_get_session_output_size (priv->ctx,
						  &output_blocks,
						  NULL);

	session = burner_task_ctx_get_session (priv->ctx);
	drive = burner_burn_session_get_burner (session);
	medium = burner_drive_get_medium (drive);
	flags = burner_burn_session_get_flags (session);

	/* FIXME: if we can't recover the size of the medium 
	 * what should we do ? do as if we could ? */

	/* see if we are appending or not */
	if (flags & (BURNER_BURN_FLAG_APPEND|BURNER_BURN_FLAG_MERGE))
		burner_medium_get_free_space (medium, NULL, &media_blocks);
	else
		burner_medium_get_capacity (medium, NULL, &media_blocks);

	/* This is not really an error, we'll probably ask the 
	 * user to load a new disc */
	if (output_blocks > media_blocks) {
		gchar *media_blocks_str;
		gchar *output_blocks_str;

		BURNER_BURN_LOG ("Insufficient space on disc %"G_GINT64_FORMAT"/%"G_GINT64_FORMAT,
				  media_blocks,
				  output_blocks);

		media_blocks_str = g_strdup_printf ("%"G_GINT64_FORMAT, media_blocks);
		output_blocks_str = g_strdup_printf ("%"G_GINT64_FORMAT, output_blocks);

		g_set_error (error,
			     BURNER_BURN_ERROR,
			     BURNER_BURN_ERROR_MEDIUM_SPACE,
			     /* Translators: the first %s is the size of the free space on the medium
			      * and the second %s is the size of the space required by the data to be
			      * burnt. */
			     _("Not enough space available on the disc (%s available for %s)"),
			     media_blocks_str,
			     output_blocks_str);

		g_free (media_blocks_str);
		g_free (output_blocks_str);
		return BURNER_BURN_NEED_RELOAD;
	}

	return BURNER_BURN_OK;
}

static BurnerBurnResult
burner_job_check_output_volume_space (BurnerJob *self,
				       GError **error)
{
	GFileInfo *info;
	gchar *directory;
	GFile *file = NULL;
	struct rlimit limit;
	guint64 vol_size = 0;
	gint64 output_size = 0;
	BurnerJobPrivate *priv;
	const gchar *filesystem;

	/* now that the job has a known output we must check that the volume the
	 * job is writing to has enough space for all output */

	priv = BURNER_JOB_PRIVATE (self);

	/* Get the size and filesystem type for the volume first.
	 * NOTE: since any plugin must output anything LOCALLY, we can then use
	 * all libc API. */
	if (!priv->output)
		return BURNER_BURN_ERR;

	directory = g_path_get_dirname (priv->output->image);
	file = g_file_new_for_path (directory);
	g_free (directory);

	if (file == NULL)
		goto error;

	info = g_file_query_info (file,
				  G_FILE_ATTRIBUTE_ACCESS_CAN_WRITE,
				  G_FILE_QUERY_INFO_NONE,
				  NULL,
				  error);
	if (!info)
		goto error;

	/* Check permissions first */
	if (!g_file_info_get_attribute_boolean (info, G_FILE_ATTRIBUTE_ACCESS_CAN_WRITE)) {
		BURNER_JOB_LOG (self, "No permissions");

		g_object_unref (info);
		g_object_unref (file);
		g_set_error (error,
			     BURNER_BURN_ERROR,
			     BURNER_BURN_ERROR_PERMISSION,
			     _("You do not have the required permission to write at this location"));
		return BURNER_BURN_ERR;
	}
	g_object_unref (info);

	/* Now size left */
	info = g_file_query_filesystem_info (file,
					     G_FILE_ATTRIBUTE_FILESYSTEM_FREE ","
					     G_FILE_ATTRIBUTE_FILESYSTEM_TYPE,
					     NULL,
					     error);
	if (!info)
		goto error;

	g_object_unref (file);
	file = NULL;

	/* Now check the filesystem type: the problem here is that some
	 * filesystems have a maximum file size limit of 4 GiB and more than
	 * often we need a temporary file size of 4 GiB or more. */
	filesystem = g_file_info_get_attribute_string (info, G_FILE_ATTRIBUTE_FILESYSTEM_TYPE);
	BURNER_BURN_LOG ("%s filesystem detected", filesystem);

	burner_job_get_session_output_size (self, NULL, &output_size);

	if (output_size >= 2147483648ULL
	&&  filesystem
	&& !strcmp (filesystem, "msdos")) {
		g_object_unref (info);
		g_set_error (error,
			     BURNER_BURN_ERROR,
			     BURNER_BURN_ERROR_DISK_SPACE,
			     _("The filesystem you chose to store the temporary image on cannot hold files with a size over 2 GiB"));
		return BURNER_BURN_ERR;
	}

	vol_size = g_file_info_get_attribute_uint64 (info, G_FILE_ATTRIBUTE_FILESYSTEM_FREE);
	g_object_unref (info);

	/* get the size of the output this job is supposed to create */
	BURNER_BURN_LOG ("Volume size %lli, output size %lli", vol_size, output_size);

	/* it's fine here to check size in bytes */
	if (output_size > vol_size) {
		g_set_error (error,
			     BURNER_BURN_ERROR,
			     BURNER_BURN_ERROR_DISK_SPACE,
			     _("The location you chose to store the temporary image on does not have enough free space for the disc image (%ld MiB needed)"),
			     (unsigned long) output_size / 1048576);
		return BURNER_BURN_ERR;
	}

	/* Last but not least, use getrlimit () to check that we are allowed to
	 * write a file of such length and that quotas won't get in our way */
	if (getrlimit (RLIMIT_FSIZE, &limit)) {
		g_set_error (error,
			     BURNER_BURN_ERROR,
			     BURNER_BURN_ERROR_DISK_SPACE,
			     "%s",
			     g_strerror (errno));
		return BURNER_BURN_ERR;
	}

	if (limit.rlim_cur < output_size) {
		BURNER_BURN_LOG ("User not allowed to write such a large file");

		g_set_error (error,
			     BURNER_BURN_ERROR,
			     BURNER_BURN_ERROR_DISK_SPACE,
			     _("The location you chose to store the temporary image on does not have enough free space for the disc image (%ld MiB needed)"),
			     (unsigned long) output_size / 1048576);
		return BURNER_BURN_ERR;
	}

	return BURNER_BURN_OK;

error:

	if (error && *error == NULL)
		g_set_error (error,
			     BURNER_BURN_ERROR,
			     BURNER_BURN_ERROR_GENERAL,
			     _("The size of the volume could not be retrieved"));

	if (file)
		g_object_unref (file);

	return BURNER_BURN_ERR;
}

static BurnerBurnResult
burner_job_set_output_file (BurnerJob *self,
			     GError **error)
{
	BurnerTrackType *session_output;
	BurnerBurnSession *session;
	BurnerBurnResult result;
	BurnerJobPrivate *priv;
	goffset output_size = 0;
	gchar *image = NULL;
	gchar *toc = NULL;
	gboolean is_last;

	priv = BURNER_JOB_PRIVATE (self);

	/* no next job so we need a file pad */
	session = burner_task_ctx_get_session (priv->ctx);

	/* If the plugin is not supposed to output anything, then don't test */
	burner_job_get_session_output_size (BURNER_JOB (self), NULL, &output_size);

	/* This should be re-enabled when we make sure all plugins (like vcd)
	 * don't advertize an output of 0 whereas it's not true. Maybe we could
	 * use -1 for plugins that don't output. */
	/* if (!output_size)
		return BURNER_BURN_OK; */

	/* check if that's the last task */
	session_output = burner_track_type_new ();
	burner_burn_session_get_output_type (session, session_output);
	is_last = burner_track_type_equal (session_output, &priv->type);
	burner_track_type_free (session_output);

	if (priv->type.type == BURNER_TRACK_TYPE_IMAGE) {
		if (is_last) {
			BurnerTrackType input = { 0, };

			result = burner_burn_session_get_output (session,
								  &image,
								  &toc);

			/* check paths are set */
			if (!image
			|| (priv->type.subtype.img_format != BURNER_IMAGE_FORMAT_BIN && !toc)) {
				g_set_error (error,
					     BURNER_BURN_ERROR,
					     BURNER_BURN_ERROR_OUTPUT_NONE,
					     _("No path was specified for the image output"));
				return BURNER_BURN_ERR;
			}

			burner_burn_session_get_input_type (session,
							     &input);

			/* if input is the same as output, then that's a
			 * processing task and there's no need to check if the
			 * output already exists since it will (but that's OK) */
			if (input.type == BURNER_TRACK_TYPE_IMAGE
			&&  input.subtype.img_format == priv->type.subtype.img_format) {
				BURNER_BURN_LOG ("Processing task, skipping check size");
				priv->output = g_new0 (BurnerJobOutput, 1);
				priv->output->image = image;
				priv->output->toc = toc;
				return BURNER_BURN_OK;
			}
		}
		else {
			/* NOTE: no need to check for the existence here */
			result = burner_burn_session_get_tmp_image (session,
								     priv->type.subtype.img_format,
								     &image,
								     &toc,
								     error);
			if (result != BURNER_BURN_OK)
				return result;
		}

		BURNER_JOB_LOG (self, "output set (IMAGE) image = %s toc = %s",
				 image,
				 toc ? toc : "none");
	}
	else if (priv->type.type == BURNER_TRACK_TYPE_STREAM) {
		/* NOTE: this one can only a temporary file */
		result = burner_burn_session_get_tmp_file (session,
							    ".cdr",
							    &image,
							    error);
		BURNER_JOB_LOG (self, "Output set (AUDIO) image = %s", image);
	}
	else /* other types don't need an output */
		return BURNER_BURN_OK;

	if (result != BURNER_BURN_OK)
		return result;

	priv->output = g_new0 (BurnerJobOutput, 1);
	priv->output->image = image;
	priv->output->toc = toc;

	if (burner_burn_session_get_flags (session) & BURNER_BURN_FLAG_CHECK_SIZE)
		return burner_job_check_output_volume_space (self, error);

	return result;
}

static BurnerJob *
burner_job_get_next_active (BurnerJob *self)
{
	BurnerJobPrivate *priv;
	BurnerJob *next;

	/* since some jobs can return NOT_RUNNING after ::init, skip them */
	priv = BURNER_JOB_PRIVATE (self);
	if (!priv->next)
		return NULL;

	next = priv->next;
	while (next) {
		priv = BURNER_JOB_PRIVATE (next);

		if (priv->ctx)
			return next;

		next = priv->next;
	}

	return NULL;
}

static BurnerBurnResult
burner_job_item_start (BurnerTaskItem *item,
		        GError **error)
{
	BurnerJob *self;
	BurnerJobClass *klass;
	BurnerJobAction action;
	BurnerJobPrivate *priv;
	BurnerBurnResult result;

	/* This function is compulsory */
	self = BURNER_JOB (item);
	priv = BURNER_JOB_PRIVATE (self);

	/* skip jobs that are not active */
	if (!priv->ctx)
		return BURNER_BURN_OK;

	/* set the output if need be */
	burner_job_get_action (self, &action);
	priv->linked = burner_job_get_next_active (self);

	if (!priv->linked) {
		/* that's the last job so is action is image it needs a file */
		if (action == BURNER_JOB_ACTION_IMAGE) {
			result = burner_job_set_output_file (self, error);
			if (result != BURNER_BURN_OK)
				return result;
		}
		else if (action == BURNER_JOB_ACTION_RECORD) {
			BurnerBurnFlag flags;
			BurnerBurnSession *session;

			session = burner_task_ctx_get_session (priv->ctx);
			flags = burner_burn_session_get_flags (session);
			if (flags & BURNER_BURN_FLAG_CHECK_SIZE
			&& !(flags & BURNER_BURN_FLAG_OVERBURN)) {
				result = burner_job_check_output_disc_space (self, error);
				if (result != BURNER_BURN_OK)
					return result;
			}
		}
	}
	else
		BURNER_JOB_LOG (self, "linked to %s", G_OBJECT_TYPE_NAME (priv->linked));

	if (!burner_job_is_first_active (self)) {
		int fd [2];

		BURNER_JOB_LOG (self, "creating input");

		/* setup a pipe */
		if (pipe (fd)) {
                        int errsv = errno;

			BURNER_BURN_LOG ("A pipe couldn't be created");
			g_set_error (error,
				     BURNER_BURN_ERROR,
				     BURNER_BURN_ERROR_GENERAL,
				     _("An internal error occurred (%s)"),
				     g_strerror (errsv));

			return BURNER_BURN_ERR;
		}

		/* NOTE: don't set O_NONBLOCK automatically as some plugins 
		 * don't like that (genisoimage, mkisofs) */
		priv->input = g_new0 (BurnerJobInput, 1);
		priv->input->in = fd [0];
		priv->input->out = fd [1];
	}

	klass = BURNER_JOB_GET_CLASS (self);
	if (!klass->start) {
		BURNER_JOB_LOG (self, "no ::start method");
		BURNER_JOB_NOT_SUPPORTED (self);
	}

	result = klass->start (self, error);
	if (result == BURNER_BURN_NOT_RUNNING) {
		/* this means that the task is already completed. This 
		 * must be returned by the last active job of the task
		 * (and usually the only one?) */

		if (priv->linked) {
			g_set_error (error,
				     BURNER_BURN_ERROR,
				     BURNER_BURN_ERROR_PLUGIN_MISBEHAVIOR,
				     _("\"%s\" did not behave properly"),
				     G_OBJECT_TYPE_NAME (self));
			return BURNER_BURN_ERR;
		}
	}

	if (result == BURNER_BURN_NOT_SUPPORTED) {
		/* only forgive this error when that's the last job and we're
		 * searching for a job to set the current track size */
		if (action != BURNER_JOB_ACTION_SIZE) {
			g_set_error (error,
				     BURNER_BURN_ERROR,
				     BURNER_BURN_ERROR_PLUGIN_MISBEHAVIOR,
				     _("\"%s\" did not behave properly"),
				     G_OBJECT_TYPE_NAME (self));
			return BURNER_BURN_ERR;
		}

		/* deactivate it */
		burner_job_deactivate (self);
	}

	return result;
}

static BurnerBurnResult
burner_job_item_clock_tick (BurnerTaskItem *item,
			     BurnerTaskCtx *ctx,
			     GError **error)
{
	BurnerJob *self;
	BurnerJobClass *klass;
	BurnerJobPrivate *priv;
	BurnerBurnResult result = BURNER_BURN_OK;

	self = BURNER_JOB (item);
	priv = BURNER_JOB_PRIVATE (self);
	if (!priv->ctx)
		return BURNER_BURN_OK;

	klass = BURNER_JOB_GET_CLASS (self);
	if (klass->clock_tick)
		result = klass->clock_tick (self);

	return result;
}

static BurnerBurnResult
burner_job_disconnect (BurnerJob *self,
			GError **error)
{
	BurnerJobPrivate *priv;
	BurnerBurnResult result = BURNER_BURN_OK;

	priv = BURNER_JOB_PRIVATE (self);

	/* NOTE: this function is only called when there are no more track to 
	 * process */

	if (priv->linked) {
		BurnerJobPrivate *priv_link;

		BURNER_JOB_LOG (self,
				 "disconnecting %s from %s",
				 G_OBJECT_TYPE_NAME (self),
				 G_OBJECT_TYPE_NAME (priv->linked));

		priv_link = BURNER_JOB_PRIVATE (priv->linked);

		/* only close the input to tell the other end that we're
		 * finished with writing to the pipe */
		if (priv_link->input->out > 0) {
			close (priv_link->input->out);
			priv_link->input->out = 0;
		}
	}
	else if (priv->output) {
		burner_job_output_free (priv->output);
		priv->output = NULL;
	}

	if (priv->input) {
		BURNER_JOB_LOG (self,
				 "closing connection for %s",
				 G_OBJECT_TYPE_NAME (self));

		burner_job_input_free (priv->input);
		priv->input = NULL;
	}

	return result;
}

static BurnerBurnResult
burner_job_item_stop (BurnerTaskItem *item,
		       BurnerTaskCtx *ctx,
		       GError **error)
{
	BurnerJob *self;
	BurnerJobClass *klass;
	BurnerJobPrivate *priv;
	BurnerBurnResult result = BURNER_BURN_OK;
	
	self = BURNER_JOB (item);
	priv = BURNER_JOB_PRIVATE (self);

	if (!priv->ctx)
		return BURNER_BURN_OK;

	BURNER_JOB_LOG (self, "stopping");

	/* the order is important here */
	klass = BURNER_JOB_GET_CLASS (self);
	if (klass->stop)
		result = klass->stop (self, error);

	burner_job_disconnect (self, error);

	if (priv->ctx) {
		g_object_unref (priv->ctx);
		priv->ctx = NULL;
	}

	return result;
}

static void
burner_job_iface_init_task_item (BurnerTaskItemIFace *iface)
{
	iface->next = burner_job_item_next;
	iface->previous = burner_job_item_previous;
	iface->link = burner_job_item_link;
	iface->activate = burner_job_item_activate;
	iface->is_active = burner_job_item_is_active;
	iface->start = burner_job_item_start;
	iface->stop = burner_job_item_stop;
	iface->clock_tick = burner_job_item_clock_tick;
}

BurnerBurnResult
burner_job_tag_lookup (BurnerJob *self,
			const gchar *tag,
			GValue **value)
{
	BurnerJobPrivate *priv;
	BurnerBurnSession *session;

	BURNER_JOB_DEBUG (self);

	priv = BURNER_JOB_PRIVATE (self);

	session = burner_task_ctx_get_session (priv->ctx);
	return burner_burn_session_tag_lookup (session,
						tag,
						value);
}

BurnerBurnResult
burner_job_tag_add (BurnerJob *self,
		     const gchar *tag,
		     GValue *value)
{
	BurnerJobPrivate *priv;
	BurnerBurnSession *session;

	BURNER_JOB_DEBUG (self);

	priv = BURNER_JOB_PRIVATE (self);

	if (!burner_job_is_last_active (self))
		return BURNER_BURN_ERR;

	session = burner_task_ctx_get_session (priv->ctx);
	burner_burn_session_tag_add (session,
				      tag,
				      value);

	return BURNER_BURN_OK;
}

/**
 * Means a job successfully completed its task.
 * track can be NULL, depending on whether or not the job created a track.
 */

BurnerBurnResult
burner_job_add_track (BurnerJob *self,
		       BurnerTrack *track)
{
	BurnerJobPrivate *priv;
	BurnerJobAction action;

	BURNER_JOB_DEBUG (self);

	priv = BURNER_JOB_PRIVATE (self);

	/* to add a track to the session, a job :
	 * - must be the last running in the chain
	 * - the action for the job must be IMAGE */

	action = BURNER_JOB_ACTION_NONE;
	burner_job_get_action (self, &action);
	if (action != BURNER_JOB_ACTION_IMAGE)
		return BURNER_BURN_ERR;

	if (!burner_job_is_last_active (self))
		return BURNER_BURN_ERR;

	return burner_task_ctx_add_track (priv->ctx, track);
}

BurnerBurnResult
burner_job_finished_session (BurnerJob *self)
{
	GError *error = NULL;
	BurnerJobClass *klass;
	BurnerJobPrivate *priv;
	BurnerBurnResult result;

	priv = BURNER_JOB_PRIVATE (self);

	BURNER_JOB_LOG (self, "Finished successfully session");

	if (burner_job_is_last_active (self))
		return burner_task_ctx_finished (priv->ctx);

	if (!burner_job_is_first_active (self)) {
		/* This job is apparently a go between job.
		 * It should only call for a stop on an error. */
		BURNER_JOB_LOG (self, "is not a leader");
		error = g_error_new (BURNER_BURN_ERROR,
				     BURNER_BURN_ERROR_PLUGIN_MISBEHAVIOR,
				     _("\"%s\" did not behave properly"),
				     G_OBJECT_TYPE_NAME (self));
		return burner_task_ctx_error (priv->ctx,
					       BURNER_BURN_ERR,
					       error);
	}

	/* call the stop method of the job since it's finished */ 
	klass = BURNER_JOB_GET_CLASS (self);
	if (klass->stop) {
		result = klass->stop (self, &error);
		if (result != BURNER_BURN_OK)
			return burner_task_ctx_error (priv->ctx,
						       result,
						       error);
	}

	/* this job is finished but it's not the leader so
	 * the task is not finished. Close the pipe on
	 * one side to let the next job know that there
	 * isn't any more data to be expected */
	result = burner_job_disconnect (self, &error);
	g_object_unref (priv->ctx);
	priv->ctx = NULL;

	if (result != BURNER_BURN_OK)
		return burner_task_ctx_error (priv->ctx,
					       result,
					       error);

	return BURNER_BURN_OK;
}

BurnerBurnResult
burner_job_finished_track (BurnerJob *self)
{
	GError *error = NULL;
	BurnerJobPrivate *priv;
	BurnerBurnResult result;

	priv = BURNER_JOB_PRIVATE (self);

	BURNER_JOB_LOG (self, "Finished track successfully");

	/* we first check if it's the first job */
	if (burner_job_is_first_active (self)) {
		BurnerJobClass *klass;

		/* call ::stop for the job since it's finished */ 
		klass = BURNER_JOB_GET_CLASS (self);
		if (klass->stop) {
			result = klass->stop (self, &error);

			if (result != BURNER_BURN_OK)
				return burner_task_ctx_error (priv->ctx,
							       result,
							       error);
		}

		/* see if there is another track to process */
		result = burner_task_ctx_next_track (priv->ctx);
		if (result == BURNER_BURN_RETRY) {
			/* there is another track to process: don't close the
			 * input of the next connected job. Leave it active */
			return BURNER_BURN_OK;
		}

		if (!burner_job_is_last_active (self)) {
			/* this job is finished but it's not the leader so the
			 * task is not finished. Close the pipe on one side to
			 * let the next job know that there isn't any more data
			 * to be expected */
			result = burner_job_disconnect (self, &error);

			burner_job_deactivate (self);

			if (result != BURNER_BURN_OK)
				return burner_task_ctx_error (priv->ctx,
							       result,
							       error);

			return BURNER_BURN_OK;
		}
	}
	else if (!burner_job_is_last_active (self)) {
		/* This job is apparently a go between job. It should only call
		 * for a stop on an error. */
		BURNER_JOB_LOG (self, "is not a leader");
		error = g_error_new (BURNER_BURN_ERROR,
				     BURNER_BURN_ERROR_PLUGIN_MISBEHAVIOR,
				     _("\"%s\" did not behave properly"),
				     G_OBJECT_TYPE_NAME (self));
		return burner_task_ctx_error (priv->ctx, BURNER_BURN_ERR, error);
	}

	return burner_task_ctx_finished (priv->ctx);
}

/**
 * means a job didn't successfully completed its task
 */

BurnerBurnResult
burner_job_error (BurnerJob *self, GError *error)
{
	GValue instance_and_params [2];
	BurnerJobPrivate *priv;
	GValue return_value;

	BURNER_JOB_DEBUG (self);

	BURNER_JOB_LOG (self, "finished with an error");

	priv = BURNER_JOB_PRIVATE (self);

	instance_and_params [0].g_type = 0;
	g_value_init (instance_and_params, G_TYPE_FROM_INSTANCE (self));
	g_value_set_instance (instance_and_params, self);

	instance_and_params [1].g_type = 0;
	g_value_init (instance_and_params + 1, G_TYPE_INT);

	if (error)
		g_value_set_int (instance_and_params + 1, error->code);
	else
		g_value_set_int (instance_and_params + 1, BURNER_BURN_ERROR_GENERAL);

	return_value.g_type = 0;
	g_value_init (&return_value, G_TYPE_INT);
	g_value_set_int (&return_value, BURNER_BURN_ERR);

	/* There was an error: signal it. That's mainly done
	 * for BurnerBurnCaps to override the result value */
	g_signal_emitv (instance_and_params,
			burner_job_signals [ERROR_SIGNAL],
			0,
			&return_value);

	g_value_unset (instance_and_params);

	BURNER_JOB_LOG (self,
			 "asked to stop because of an error\n"
			 "\terror\t\t= %i\n"
			 "\tmessage\t= \"%s\"",
			 error ? error->code:0,
			 error ? error->message:"no message");

	return burner_task_ctx_error (priv->ctx, g_value_get_int (&return_value), error);
}

/**
 * Used to retrieve config for a job
 * If the parameter is missing for the next 4 functions
 * it allows one to test if they could be used
 */

BurnerBurnResult
burner_job_get_fd_in (BurnerJob *self, int *fd_in)
{
	BurnerJobPrivate *priv;

	BURNER_JOB_DEBUG (self);

	priv = BURNER_JOB_PRIVATE (self);

	if (!priv->input)
		return BURNER_BURN_ERR;

	if (!fd_in)
		return BURNER_BURN_OK;

	*fd_in = priv->input->in;
	return BURNER_BURN_OK;
}

static BurnerBurnResult
burner_job_set_nonblocking_fd (int fd, GError **error)
{
	long flags = 0;

	if (fcntl (fd, F_GETFL, &flags) != -1) {
		/* Unfortunately some plugin (mkisofs/genisofs don't like 
		 * O_NONBLOCK (which is a shame) so we don't set them
		 * automatically but still offer that possibility. */
		flags |= O_NONBLOCK;
		if (fcntl (fd, F_SETFL, flags) == -1) {
			BURNER_BURN_LOG ("couldn't set non blocking mode");
			g_set_error (error,
				     BURNER_BURN_ERROR,
				     BURNER_BURN_ERROR_GENERAL,
				     _("An internal error occurred"));
			return BURNER_BURN_ERR;
		}
	}
	else {
		BURNER_BURN_LOG ("couldn't get pipe flags");
		g_set_error (error,
			     BURNER_BURN_ERROR,
			     BURNER_BURN_ERROR_GENERAL,
			     _("An internal error occurred"));
		return BURNER_BURN_ERR;
	}

	return BURNER_BURN_OK;
}

BurnerBurnResult
burner_job_set_nonblocking (BurnerJob *self,
			     GError **error)
{
	BurnerBurnResult result;
	int fd;

	BURNER_JOB_DEBUG (self);

	fd = -1;
	if (burner_job_get_fd_in (self, &fd) == BURNER_BURN_OK) {
		result = burner_job_set_nonblocking_fd (fd, error);
		if (result != BURNER_BURN_OK)
			return result;
	}

	fd = -1;
	if (burner_job_get_fd_out (self, &fd) == BURNER_BURN_OK) {
		result = burner_job_set_nonblocking_fd (fd, error);
		if (result != BURNER_BURN_OK)
			return result;
	}

	return BURNER_BURN_OK;
}

BurnerBurnResult
burner_job_get_current_track (BurnerJob *self,
			       BurnerTrack **track)
{
	BurnerJobPrivate *priv;

	BURNER_JOB_DEBUG (self);

	priv = BURNER_JOB_PRIVATE (self);
	if (!track)
		return BURNER_BURN_OK;

	return burner_task_ctx_get_current_track (priv->ctx, track);
}

BurnerBurnResult
burner_job_get_done_tracks (BurnerJob *self, GSList **tracks)
{
	BurnerJobPrivate *priv;

	BURNER_JOB_DEBUG (self);

	/* tracks already done are those that are in session */
	priv = BURNER_JOB_PRIVATE (self);
	return burner_task_ctx_get_stored_tracks (priv->ctx, tracks);
}

BurnerBurnResult
burner_job_get_tracks (BurnerJob *self, GSList **tracks)
{
	BurnerJobPrivate *priv;
	BurnerBurnSession *session;

	BURNER_JOB_DEBUG (self);

	g_return_val_if_fail (tracks != NULL, BURNER_BURN_ERR);

	priv = BURNER_JOB_PRIVATE (self);
	session = burner_task_ctx_get_session (priv->ctx);
	*tracks = burner_burn_session_get_tracks (session);
	return BURNER_BURN_OK;
}

BurnerBurnResult
burner_job_get_fd_out (BurnerJob *self, int *fd_out)
{
	BurnerJobPrivate *priv;

	BURNER_JOB_DEBUG (self);

	priv = BURNER_JOB_PRIVATE (self);

	if (!priv->linked)
		return BURNER_BURN_ERR;

	if (!fd_out)
		return BURNER_BURN_OK;

	priv = BURNER_JOB_PRIVATE (priv->linked);
	if (!priv->input)
		return BURNER_BURN_ERR;

	*fd_out = priv->input->out;
	return BURNER_BURN_OK;
}

BurnerBurnResult
burner_job_get_image_output (BurnerJob *self,
			      gchar **image,
			      gchar **toc)
{
	BurnerJobPrivate *priv;

	BURNER_JOB_DEBUG (self);

	priv = BURNER_JOB_PRIVATE (self);

	if (!priv->output)
		return BURNER_BURN_ERR;

	if (image)
		*image = g_strdup (priv->output->image);

	if (toc)
		*toc = g_strdup (priv->output->toc);

	return BURNER_BURN_OK;
}

BurnerBurnResult
burner_job_get_audio_output (BurnerJob *self,
			      gchar **path)
{
	BurnerJobPrivate *priv;

	BURNER_JOB_DEBUG (self);

	priv = BURNER_JOB_PRIVATE (self);
	if (!priv->output)
		return BURNER_BURN_ERR;

	if (path)
		*path = g_strdup (priv->output->image);

	return BURNER_BURN_OK;
}

BurnerBurnResult
burner_job_get_flags (BurnerJob *self, BurnerBurnFlag *flags)
{
	BurnerBurnSession *session;
	BurnerJobPrivate *priv;

	BURNER_JOB_DEBUG (self);

	g_return_val_if_fail (flags != NULL, BURNER_BURN_ERR);

	priv = BURNER_JOB_PRIVATE (self);
	session = burner_task_ctx_get_session (priv->ctx);
	*flags = burner_burn_session_get_flags (session);

	return BURNER_BURN_OK;
}

BurnerBurnResult
burner_job_get_input_type (BurnerJob *self, BurnerTrackType *type)
{
	BurnerBurnResult result = BURNER_BURN_OK;
	BurnerJobPrivate *priv;

	BURNER_JOB_DEBUG (self);

	priv = BURNER_JOB_PRIVATE (self);
	if (!priv->previous) {
		BurnerBurnSession *session;

		session = burner_task_ctx_get_session (priv->ctx);
		result = burner_burn_session_get_input_type (session, type);
	}
	else {
		BurnerJobPrivate *prev_priv;

		prev_priv = BURNER_JOB_PRIVATE (priv->previous);
		memcpy (type, &prev_priv->type, sizeof (BurnerTrackType));
		result = BURNER_BURN_OK;
	}

	return result;
}

BurnerBurnResult
burner_job_get_output_type (BurnerJob *self, BurnerTrackType *type)
{
	BurnerJobPrivate *priv;

	BURNER_JOB_DEBUG (self);

	priv = BURNER_JOB_PRIVATE (self);

	memcpy (type, &priv->type, sizeof (BurnerTrackType));
	return BURNER_BURN_OK;
}

BurnerBurnResult
burner_job_get_action (BurnerJob *self, BurnerJobAction *action)
{
	BurnerJobPrivate *priv;
	BurnerTaskAction task_action;

	BURNER_JOB_DEBUG (self);

	g_return_val_if_fail (action != NULL, BURNER_BURN_ERR);

	priv = BURNER_JOB_PRIVATE (self);

	if (!burner_job_is_last_active (self)) {
		*action = BURNER_JOB_ACTION_IMAGE;
		return BURNER_BURN_OK;
	}

	task_action = burner_task_ctx_get_action (priv->ctx);
	switch (task_action) {
	case BURNER_TASK_ACTION_NONE:
		*action = BURNER_JOB_ACTION_SIZE;
		break;

	case BURNER_TASK_ACTION_ERASE:
		*action = BURNER_JOB_ACTION_ERASE;
		break;

	case BURNER_TASK_ACTION_NORMAL:
		if (priv->type.type == BURNER_TRACK_TYPE_DISC)
			*action = BURNER_JOB_ACTION_RECORD;
		else
			*action = BURNER_JOB_ACTION_IMAGE;
		break;

	case BURNER_TASK_ACTION_CHECKSUM:
		*action = BURNER_JOB_ACTION_CHECKSUM;
		break;

	default:
		*action = BURNER_JOB_ACTION_NONE;
		break;
	}

	return BURNER_BURN_OK;
}

BurnerBurnResult
burner_job_get_bus_target_lun (BurnerJob *self, gchar **BTL)
{
	BurnerBurnSession *session;
	BurnerJobPrivate *priv;
	BurnerDrive *drive;

	BURNER_JOB_DEBUG (self);

	g_return_val_if_fail (BTL != NULL, BURNER_BURN_ERR);

	priv = BURNER_JOB_PRIVATE (self);
	session = burner_task_ctx_get_session (priv->ctx);

	drive = burner_burn_session_get_burner (session);
	*BTL = burner_drive_get_bus_target_lun_string (drive);

	return BURNER_BURN_OK;
}

BurnerBurnResult
burner_job_get_device (BurnerJob *self, gchar **device)
{
	BurnerBurnSession *session;
	BurnerJobPrivate *priv;
	BurnerDrive *drive;
	const gchar *path;

	BURNER_JOB_DEBUG (self);

	g_return_val_if_fail (device != NULL, BURNER_BURN_ERR);

	priv = BURNER_JOB_PRIVATE (self);
	session = burner_task_ctx_get_session (priv->ctx);

	drive = burner_burn_session_get_burner (session);
	path = burner_drive_get_device (drive);
	*device = g_strdup (path);

	return BURNER_BURN_OK;
}

BurnerBurnResult
burner_job_get_media (BurnerJob *self, BurnerMedia *media)
{
	BurnerBurnSession *session;
	BurnerJobPrivate *priv;

	BURNER_JOB_DEBUG (self);

	g_return_val_if_fail (media != NULL, BURNER_BURN_ERR);

	priv = BURNER_JOB_PRIVATE (self);
	session = burner_task_ctx_get_session (priv->ctx);
	*media = burner_burn_session_get_dest_media (session);

	return BURNER_BURN_OK;
}

BurnerBurnResult
burner_job_get_medium (BurnerJob *job, BurnerMedium **medium)
{
	BurnerBurnSession *session;
	BurnerJobPrivate *priv;
	BurnerDrive *drive;

	BURNER_JOB_DEBUG (job);

	g_return_val_if_fail (medium != NULL, BURNER_BURN_ERR);

	priv = BURNER_JOB_PRIVATE (job);
	session = burner_task_ctx_get_session (priv->ctx);
	drive = burner_burn_session_get_burner (session);
	*medium = burner_drive_get_medium (drive);

	if (!(*medium))
		return BURNER_BURN_ERR;

	g_object_ref (*medium);
	return BURNER_BURN_OK;
}

BurnerBurnResult
burner_job_get_last_session_address (BurnerJob *self, goffset *address)
{
	BurnerBurnSession *session;
	BurnerJobPrivate *priv;
	BurnerMedium *medium;
	BurnerDrive *drive;

	BURNER_JOB_DEBUG (self);

	g_return_val_if_fail (address != NULL, BURNER_BURN_ERR);

	priv = BURNER_JOB_PRIVATE (self);
	session = burner_task_ctx_get_session (priv->ctx);
	drive = burner_burn_session_get_burner (session);
	medium = burner_drive_get_medium (drive);
	if (burner_medium_get_last_data_track_address (medium, NULL, address))
		return BURNER_BURN_OK;

	return BURNER_BURN_ERR;
}

BurnerBurnResult
burner_job_get_next_writable_address (BurnerJob *self, goffset *address)
{
	BurnerBurnSession *session;
	BurnerJobPrivate *priv;
	BurnerMedium *medium;
	BurnerDrive *drive;

	BURNER_JOB_DEBUG (self);

	g_return_val_if_fail (address != NULL, BURNER_BURN_ERR);

	priv = BURNER_JOB_PRIVATE (self);
	session = burner_task_ctx_get_session (priv->ctx);
	drive = burner_burn_session_get_burner (session);
	medium = burner_drive_get_medium (drive);
	*address = burner_medium_get_next_writable_address (medium);

	return BURNER_BURN_OK;
}

BurnerBurnResult
burner_job_get_rate (BurnerJob *self, guint64 *rate)
{
	BurnerBurnSession *session;
	BurnerJobPrivate *priv;

	g_return_val_if_fail (rate != NULL, BURNER_BURN_ERR);

	priv = BURNER_JOB_PRIVATE (self);
	session = burner_task_ctx_get_session (priv->ctx);
	*rate = burner_burn_session_get_rate (session);

	return BURNER_BURN_OK;
}

static int
_round_speed (float number)
{
	int retval = (gint) number;

	/* NOTE: number must always be positive */
	number -= (float) retval;
	if (number >= 0.5)
		return retval + 1;

	return retval;
}

BurnerBurnResult
burner_job_get_speed (BurnerJob *self, guint *speed)
{
	BurnerBurnSession *session;
	BurnerJobPrivate *priv;
	BurnerMedia media;
	guint64 rate;

	BURNER_JOB_DEBUG (self);

	g_return_val_if_fail (speed != NULL, BURNER_BURN_ERR);

	priv = BURNER_JOB_PRIVATE (self);
	session = burner_task_ctx_get_session (priv->ctx);
	rate = burner_burn_session_get_rate (session);

	media = burner_burn_session_get_dest_media (session);
	if (media & BURNER_MEDIUM_DVD)
		*speed = _round_speed (BURNER_RATE_TO_SPEED_DVD (rate));
	else if (media & BURNER_MEDIUM_BD)
		*speed = _round_speed (BURNER_RATE_TO_SPEED_BD (rate));
	else
		*speed = _round_speed (BURNER_RATE_TO_SPEED_CD (rate));

	return BURNER_BURN_OK;
}

BurnerBurnResult
burner_job_get_max_rate (BurnerJob *self, guint64 *rate)
{
	BurnerBurnSession *session;
	BurnerJobPrivate *priv;
	BurnerMedium *medium;
	BurnerDrive *drive;

	BURNER_JOB_DEBUG (self);

	g_return_val_if_fail (rate != NULL, BURNER_BURN_ERR);

	priv = BURNER_JOB_PRIVATE (self);
	session = burner_task_ctx_get_session (priv->ctx);

	drive = burner_burn_session_get_burner (session);
	medium = burner_drive_get_medium (drive);

	if (!medium)
		return BURNER_BURN_NOT_READY;

	*rate = burner_medium_get_max_write_speed (medium);

	return BURNER_BURN_OK;
}

BurnerBurnResult
burner_job_get_max_speed (BurnerJob *self, guint *speed)
{
	BurnerBurnSession *session;
	BurnerJobPrivate *priv;
	BurnerMedium *medium;
	BurnerDrive *drive;
	BurnerMedia media;
	guint64 rate;

	BURNER_JOB_DEBUG (self);

	g_return_val_if_fail (speed != NULL, BURNER_BURN_ERR);

	priv = BURNER_JOB_PRIVATE (self);
	session = burner_task_ctx_get_session (priv->ctx);

	drive = burner_burn_session_get_burner (session);
	medium = burner_drive_get_medium (drive);
	if (!medium)
		return BURNER_BURN_NOT_READY;

	rate = burner_medium_get_max_write_speed (medium);
	media = burner_medium_get_status (medium);
	if (media & BURNER_MEDIUM_DVD)
		*speed = _round_speed (BURNER_RATE_TO_SPEED_DVD (rate));
	else if (media & BURNER_MEDIUM_BD)
		*speed = _round_speed (BURNER_RATE_TO_SPEED_BD (rate));
	else
		*speed = _round_speed (BURNER_RATE_TO_SPEED_CD (rate));

	return BURNER_BURN_OK;
}

BurnerBurnResult
burner_job_get_tmp_file (BurnerJob *self,
			  const gchar *suffix,
			  gchar **output,
			  GError **error)
{
	BurnerBurnSession *session;
	BurnerJobPrivate *priv;

	priv = BURNER_JOB_PRIVATE (self);
	session = burner_task_ctx_get_session (priv->ctx);
	return burner_burn_session_get_tmp_file (session,
						  suffix,
						  output,
						  error);
}

BurnerBurnResult
burner_job_get_tmp_dir (BurnerJob *self,
			 gchar **output,
			 GError **error)
{
	BurnerBurnSession *session;
	BurnerJobPrivate *priv;

	BURNER_JOB_DEBUG (self);

	priv = BURNER_JOB_PRIVATE (self);
	session = burner_task_ctx_get_session (priv->ctx);
	burner_burn_session_get_tmp_dir (session,
					  output,
					  error);

	return BURNER_BURN_OK;
}

BurnerBurnResult
burner_job_get_audio_title (BurnerJob *self, gchar **album)
{
	BurnerBurnSession *session;
	BurnerJobPrivate *priv;

	BURNER_JOB_DEBUG (self);

	g_return_val_if_fail (album != NULL, BURNER_BURN_ERR);

	priv = BURNER_JOB_PRIVATE (self);
	session = burner_task_ctx_get_session (priv->ctx);

	*album = g_strdup (burner_burn_session_get_label (session));
	return BURNER_BURN_OK;
}

BurnerBurnResult
burner_job_get_data_label (BurnerJob *self, gchar **label)
{
	BurnerBurnSession *session;
	BurnerJobPrivate *priv;

	BURNER_JOB_DEBUG (self);

	g_return_val_if_fail (label != NULL, BURNER_BURN_ERR);

	priv = BURNER_JOB_PRIVATE (self);
	session = burner_task_ctx_get_session (priv->ctx);

	*label = g_strdup (burner_burn_session_get_label (session));
	return BURNER_BURN_OK;
}

BurnerBurnResult
burner_job_get_session_output_size (BurnerJob *self,
				     goffset *blocks,
				     goffset *bytes)
{
	BurnerJobPrivate *priv;

	BURNER_JOB_DEBUG (self);

	priv = BURNER_JOB_PRIVATE (self);
	return burner_task_ctx_get_session_output_size (priv->ctx, blocks, bytes);
}

/**
 * Starts task internal timer 
 */

BurnerBurnResult
burner_job_start_progress (BurnerJob *self,
			    gboolean force)
{
	BurnerJobPrivate *priv;

	/* Turn this off as otherwise it floods bug reports */
	// BURNER_JOB_DEBUG (self);

	priv = BURNER_JOB_PRIVATE (self);
	if (priv->next)
		return BURNER_BURN_NOT_RUNNING;

	return burner_task_ctx_start_progress (priv->ctx, force);
}

BurnerBurnResult
burner_job_reset_progress (BurnerJob *self)
{
	BurnerJobPrivate *priv;

	priv = BURNER_JOB_PRIVATE (self);
	if (priv->next)
		return BURNER_BURN_ERR;

	return burner_task_ctx_reset_progress (priv->ctx);
}

/**
 * these should be used to set the different values of the task by the jobs
 */

BurnerBurnResult
burner_job_set_progress (BurnerJob *self,
			  gdouble progress)
{
	BurnerJobPrivate *priv;

	/* Turn this off as it floods bug reports */
	//BURNER_JOB_LOG (self, "Called burner_job_set_progress (%lf)", progress);

	priv = BURNER_JOB_PRIVATE (self);
	if (priv->next)
		return BURNER_BURN_ERR;

	if (progress < 0.0 || progress > 1.0) {
		BURNER_JOB_LOG (self, "Tried to set an insane progress value (%lf)", progress);
		return BURNER_BURN_ERR;
	}

	return burner_task_ctx_set_progress (priv->ctx, progress);
}

BurnerBurnResult
burner_job_set_current_action (BurnerJob *self,
				BurnerBurnAction action,
				const gchar *string,
				gboolean force)
{
	BurnerJobPrivate *priv;

	BURNER_JOB_DEBUG (self);

	priv = BURNER_JOB_PRIVATE (self);
	if (!burner_job_is_last_active (self))
		return BURNER_BURN_NOT_RUNNING;

	return burner_task_ctx_set_current_action (priv->ctx,
						    action,
						    string,
						    force);
}

BurnerBurnResult
burner_job_get_current_action (BurnerJob *self,
				BurnerBurnAction *action)
{
	BurnerJobPrivate *priv;

	BURNER_JOB_DEBUG (self);

	g_return_val_if_fail (action != NULL, BURNER_BURN_ERR);

	priv = BURNER_JOB_PRIVATE (self);

	if (!priv->ctx) {
		BURNER_JOB_LOG (self,
				 "called %s whereas it wasn't running",
				 G_STRFUNC);
		return BURNER_BURN_NOT_RUNNING;
	}

	return burner_task_ctx_get_current_action (priv->ctx, action);
}

BurnerBurnResult
burner_job_set_rate (BurnerJob *self,
		      gint64 rate)
{
	BurnerJobPrivate *priv;

	/* Turn this off as otherwise it floods bug reports */
	// BURNER_JOB_DEBUG (self);

	priv = BURNER_JOB_PRIVATE (self);
	if (priv->next)
		return BURNER_BURN_NOT_RUNNING;

	return burner_task_ctx_set_rate (priv->ctx, rate);
}

BurnerBurnResult
burner_job_set_output_size_for_current_track (BurnerJob *self,
					       goffset sectors,
					       goffset bytes)
{
	BurnerJobPrivate *priv;

	/* this function can only be used by the last job which is not recording
	 * all other jobs trying to set this value will be ignored.
	 * It should be used mostly during a fake running. This value is stored
	 * by the task context as the amount of bytes/blocks produced by a task.
	 * That's why it's not possible to set the DATA type number of files.
	 * NOTE: the values passed on by this function to context may be added 
	 * to other when there are multiple tracks. */
	BURNER_JOB_DEBUG (self);

	priv = BURNER_JOB_PRIVATE (self);

	if (!burner_job_is_last_active (self))
		return BURNER_BURN_ERR;

	return burner_task_ctx_set_output_size_for_current_track (priv->ctx,
								   sectors,
								   bytes);
}

BurnerBurnResult
burner_job_set_written_track (BurnerJob *self,
			       goffset written)
{
	BurnerJobPrivate *priv;

	/* Turn this off as otherwise it floods bug reports */
	// BURNER_JOB_DEBUG (self);

	priv = BURNER_JOB_PRIVATE (self);
	if (priv->next)
		return BURNER_BURN_NOT_RUNNING;

	return burner_task_ctx_set_written_track (priv->ctx, written);
}

BurnerBurnResult
burner_job_set_written_session (BurnerJob *self,
				 goffset written)
{
	BurnerJobPrivate *priv;

	/* Turn this off as otherwise it floods bug reports */
	// BURNER_JOB_DEBUG (self);

	priv = BURNER_JOB_PRIVATE (self);
	if (priv->next)
		return BURNER_BURN_NOT_RUNNING;

	return burner_task_ctx_set_written_session (priv->ctx, written);
}

BurnerBurnResult
burner_job_set_use_average_rate (BurnerJob *self, gboolean value)
{
	BurnerJobPrivate *priv;

	BURNER_JOB_DEBUG (self);

	priv = BURNER_JOB_PRIVATE (self);
	if (priv->next)
		return BURNER_BURN_NOT_RUNNING;

	return burner_task_ctx_set_use_average (priv->ctx, value);
}

void
burner_job_set_dangerous (BurnerJob *self, gboolean value)
{
	BurnerJobPrivate *priv;

	BURNER_JOB_DEBUG (self);

	priv = BURNER_JOB_PRIVATE (self);
	if (priv->ctx)
		burner_task_ctx_set_dangerous (priv->ctx, value);
}

/**
 * used for debugging
 */

void
burner_job_log_message (BurnerJob *self,
			 const gchar *location,
			 const gchar *format,
			 ...)
{
	va_list arg_list;
	BurnerJobPrivate *priv;
	BurnerBurnSession *session;

	g_return_if_fail (BURNER_IS_JOB (self));
	g_return_if_fail (format != NULL);

	priv = BURNER_JOB_PRIVATE (self);
	session = burner_task_ctx_get_session (priv->ctx);

	va_start (arg_list, format);
	burner_burn_session_logv (session, format, arg_list);
	va_end (arg_list);

	va_start (arg_list, format);
	burner_burn_debug_messagev (location, format, arg_list);
	va_end (arg_list);
}

/**
 * Object creation stuff
 */

static void
burner_job_get_property (GObject *object,
			  guint prop_id,
			  GValue *value,
			  GParamSpec *pspec)
{
	BurnerJobPrivate *priv;
	BurnerTrackType *ptr;

	priv = BURNER_JOB_PRIVATE (object);

	switch (prop_id) {
	case PROP_OUTPUT:
		ptr = g_value_get_pointer (value);
		memcpy (ptr, &priv->type, sizeof (BurnerTrackType));
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
		break;
	}
}

static void
burner_job_set_property (GObject *object,
			  guint prop_id,
			  const GValue *value,
			  GParamSpec *pspec)
{
	BurnerJobPrivate *priv;
	BurnerTrackType *ptr;

	priv = BURNER_JOB_PRIVATE (object);

	switch (prop_id) {
	case PROP_OUTPUT:
		ptr = g_value_get_pointer (value);
		if (!ptr) {
			priv->type.type = BURNER_TRACK_TYPE_NONE;
			priv->type.subtype.media = BURNER_MEDIUM_NONE;
		}
		else
			memcpy (&priv->type, ptr, sizeof (BurnerTrackType));
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
		break;
	}
}

static void
burner_job_finalize (GObject *object)
{
	BurnerJobPrivate *priv;

	priv = BURNER_JOB_PRIVATE (object);

	if (priv->ctx) {
		g_object_unref (priv->ctx);
		priv->ctx = NULL;
	}

	if (priv->previous) {
		g_object_unref (priv->previous);
		priv->previous = NULL;
	}

	if (priv->input) {
		burner_job_input_free (priv->input);
		priv->input = NULL;
	}

	if (priv->linked)
		priv->linked = NULL;

	if (priv->output) {
		burner_job_output_free (priv->output);
		priv->output = NULL;
	}

	G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
burner_job_class_init (BurnerJobClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	g_type_class_add_private (klass, sizeof (BurnerJobPrivate));

	parent_class = g_type_class_peek_parent (klass);
	object_class->finalize = burner_job_finalize;
	object_class->set_property = burner_job_set_property;
	object_class->get_property = burner_job_get_property;

	burner_job_signals [ERROR_SIGNAL] =
	    g_signal_new ("error",
			  G_TYPE_FROM_CLASS (klass),
			  G_SIGNAL_RUN_LAST|G_SIGNAL_NO_RECURSE,
			  G_STRUCT_OFFSET (BurnerJobClass, error),
			  NULL, NULL,
			  burner_marshal_INT__INT,
			  G_TYPE_INT, 1, G_TYPE_INT);

	g_object_class_install_property (object_class,
					 PROP_OUTPUT,
					 g_param_spec_pointer ("output",
							       "The type the job must output",
							       "The type the job must output",
							       G_PARAM_READWRITE|G_PARAM_CONSTRUCT_ONLY));
}

static void
burner_job_init (BurnerJob *obj)
{ }
