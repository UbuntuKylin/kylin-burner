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

#include <glib.h>
#include <glib/gi18n-lib.h>

#include "burner-media-private.h"

#include "scsi-device.h"

#include "burner-drive.h"
#include "burner-medium.h"
#include "burner-medium-monitor.h"
#include "burn-volume.h"

#include "burner-burn-lib.h"

#include "burner-data-session.h"
#include "burner-data-project.h"
#include "burner-file-node.h"
#include "burner-io.h"

#include "libburner-marshal.h"

typedef struct _BurnerDataSessionPrivate BurnerDataSessionPrivate;
struct _BurnerDataSessionPrivate
{
	BurnerIOJobBase *load_dir;

	/* Multisession drives that are inserted */
	GSList *media;

	/* Drive whose session is loaded */
	BurnerMedium *loaded;

	/* Nodes from the loaded session in the tree */
	GSList *nodes;
};

#define BURNER_DATA_SESSION_PRIVATE(o)  (G_TYPE_INSTANCE_GET_PRIVATE ((o), BURNER_TYPE_DATA_SESSION, BurnerDataSessionPrivate))

G_DEFINE_TYPE (BurnerDataSession, burner_data_session, BURNER_TYPE_DATA_PROJECT);

enum {
	AVAILABLE_SIGNAL,
	LOADED_SIGNAL,
	LAST_SIGNAL
};

static gulong burner_data_session_signals [LAST_SIGNAL] = { 0 };

/**
 * to evaluate the contents of a medium or image async
 */
struct _BurnerIOImageContentsData {
	BurnerIOJob job;
	gchar *dev_image;

	gint64 session_block;
	gint64 block;
};
typedef struct _BurnerIOImageContentsData BurnerIOImageContentsData;

static void
burner_io_image_directory_contents_destroy (BurnerAsyncTaskManager *manager,
					     gboolean cancelled,
					     gpointer callback_data)
{
	BurnerIOImageContentsData *data = callback_data;

	g_free (data->dev_image);
	burner_io_job_free (cancelled, BURNER_IO_JOB (data));
}

static BurnerAsyncTaskResult
burner_io_image_directory_contents_thread (BurnerAsyncTaskManager *manager,
					    GCancellable *cancel,
					    gpointer callback_data)
{
	BurnerIOImageContentsData *data = callback_data;
	BurnerDeviceHandle *handle;
	GList *children, *iter;
	GError *error = NULL;
	BurnerVolSrc *vol;

	handle = burner_device_handle_open (data->job.uri, FALSE, NULL);
	if (!handle) {
		GError *error;

		error = g_error_new (BURNER_BURN_ERROR,
		                     BURNER_BURN_ERROR_GENERAL,
		                     _("The drive is busy"));

		burner_io_return_result (data->job.base,
					  data->job.uri,
					  NULL,
					  error,
					  data->job.callback_data);
		return BURNER_ASYNC_TASK_FINISHED;
	}

	vol = burner_volume_source_open_device_handle (handle, &error);
	if (!vol) {
		burner_device_handle_close (handle);
		burner_io_return_result (data->job.base,
					  data->job.uri,
					  NULL,
					  error,
					  data->job.callback_data);
		return BURNER_ASYNC_TASK_FINISHED;
	}

	children = burner_volume_load_directory_contents (vol,
							   data->session_block,
							   data->block,
							   &error);
	burner_volume_source_close (vol);
	burner_device_handle_close (handle);

	for (iter = children; iter; iter = iter->next) {
		BurnerVolFile *file;
		GFileInfo *info;

		file = iter->data;

		info = g_file_info_new ();
		g_file_info_set_file_type (info, file->isdir? G_FILE_TYPE_DIRECTORY:G_FILE_TYPE_REGULAR);
		g_file_info_set_name (info, BURNER_VOLUME_FILE_NAME (file));

		if (file->isdir)
			g_file_info_set_attribute_int64 (info,
							 BURNER_IO_DIR_CONTENTS_ADDR,
							 file->specific.dir.address);
		else
			g_file_info_set_size (info, BURNER_VOLUME_FILE_SIZE (file));

		burner_io_return_result (data->job.base,
					  data->job.uri,
					  info,
					  NULL,
					  data->job.callback_data);
	}

	g_list_foreach (children, (GFunc) burner_volume_file_free, NULL);
	g_list_free (children);

	return BURNER_ASYNC_TASK_FINISHED;
}

static const BurnerAsyncTaskType image_contents_type = {
	burner_io_image_directory_contents_thread,
	burner_io_image_directory_contents_destroy
};

static void
burner_io_load_image_directory (const gchar *dev_image,
				 gint64 session_block,
				 gint64 block,
				 const BurnerIOJobBase *base,
				 BurnerIOFlags options,
				 gpointer user_data)
{
	BurnerIOImageContentsData *data;
	BurnerIOResultCallbackData *callback_data = NULL;

	if (user_data) {
		callback_data = g_new0 (BurnerIOResultCallbackData, 1);
		callback_data->callback_data = user_data;
	}

	data = g_new0 (BurnerIOImageContentsData, 1);
	data->block = block;
	data->session_block = session_block;

	burner_io_set_job (BURNER_IO_JOB (data),
			    base,
			    dev_image,
			    options,
			    callback_data);

	burner_io_push_job (BURNER_IO_JOB (data),
			     &image_contents_type);

}

void
burner_data_session_remove_last (BurnerDataSession *self)
{
	BurnerDataSessionPrivate *priv;
	GSList *iter;

	priv = BURNER_DATA_SESSION_PRIVATE (self);

	if (!priv->nodes)
		return;

	/* go through the top nodes and remove all the imported nodes */
	for (iter = priv->nodes; iter; iter = iter->next) {
		BurnerFileNode *node;

		node = iter->data;
		burner_data_project_destroy_node (BURNER_DATA_PROJECT (self), node);
	}

	g_slist_free (priv->nodes);
	priv->nodes = NULL;

	g_signal_emit (self,
		       burner_data_session_signals [LOADED_SIGNAL],
		       0,
		       priv->loaded,
		       FALSE);

	if (priv->loaded) {
		g_object_unref (priv->loaded);
		priv->loaded = NULL;
	}
}

static void
burner_data_session_load_dir_destroy (GObject *object,
				       gboolean cancelled,
				       gpointer data)
{
	gint reference;
	BurnerFileNode *parent;

	/* reference */
	reference = GPOINTER_TO_INT (data);
	if (reference <= 0)
		return;

	parent = burner_data_project_reference_get (BURNER_DATA_PROJECT (object), reference);
	if (parent)
		burner_data_project_directory_node_loaded (BURNER_DATA_PROJECT (object), parent);

	burner_data_project_reference_free (BURNER_DATA_PROJECT (object), reference);
}

static void
burner_data_session_load_dir_result (GObject *owner,
				      GError *error,
				      const gchar *dev_image,
				      GFileInfo *info,
				      gpointer data)
{
	BurnerDataSessionPrivate *priv;
	BurnerFileNode *parent;
	BurnerFileNode *node;
	gint reference;

	priv = BURNER_DATA_SESSION_PRIVATE (owner);

	if (!info) {
		g_signal_emit (owner,
			       burner_data_session_signals [LOADED_SIGNAL],
			       0,
			       priv->loaded,
			       FALSE);

		/* FIXME: tell the user the error message */
		return;
	}

	reference = GPOINTER_TO_INT (data);
	if (reference > 0)
		parent = burner_data_project_reference_get (BURNER_DATA_PROJECT (owner),
							     reference);
	else
		parent = NULL;

	/* add all the files/folders at the root of the session */
	node = burner_data_project_add_imported_session_file (BURNER_DATA_PROJECT (owner),
							       info,
							       parent);
	if (!node) {
		/* This is not a problem, it could be simply that the user did 
		 * not want to overwrite, so do not do the following (reminder):
		g_signal_emit (owner,
			       burner_data_session_signals [LOADED_SIGNAL],
			       0,
			       priv->loaded,
			       (priv->nodes != NULL));
		*/
		return;
 	}

	/* Only if we're exploring root directory */
	if (!parent) {
		priv->nodes = g_slist_prepend (priv->nodes, node);

		if (g_slist_length (priv->nodes) == 1) {
			/* Only tell when the first top node is successfully loaded */
			g_signal_emit (owner,
				       burner_data_session_signals [LOADED_SIGNAL],
				       0,
				       priv->loaded,
				       TRUE);
		}
	}
}

static gboolean
burner_data_session_load_directory_contents_real (BurnerDataSession *self,
						   BurnerFileNode *node,
						   GError **error)
{
	BurnerDataSessionPrivate *priv;
	goffset session_block;
	const gchar *device;
	gint reference = -1;

	if (node && !node->is_fake)
		return TRUE;

	priv = BURNER_DATA_SESSION_PRIVATE (self);
	device = burner_drive_get_device (burner_medium_get_drive (priv->loaded));
	burner_medium_get_last_data_track_address (priv->loaded,
						    NULL,
						    &session_block);

	if (!priv->load_dir)
		priv->load_dir = burner_io_register (G_OBJECT (self),
						      burner_data_session_load_dir_result,
						      burner_data_session_load_dir_destroy,
						      NULL);

	/* If there aren't any node then that's root */
	if (node) {
		reference = burner_data_project_reference_new (BURNER_DATA_PROJECT (self), node);
		node->is_exploring = TRUE;
	}

	burner_io_load_image_directory (device,
					 session_block,
					 BURNER_FILE_NODE_IMPORTED_ADDRESS (node),
					 priv->load_dir,
					 BURNER_IO_INFO_URGENT,
					 GINT_TO_POINTER (reference));

	if (node)
		node->is_fake = FALSE;

	return TRUE;
}

gboolean
burner_data_session_load_directory_contents (BurnerDataSession *self,
					      BurnerFileNode *node,
					      GError **error)
{
	if (node == NULL)
		return FALSE;

	return burner_data_session_load_directory_contents_real (self, node, error);
}

gboolean
burner_data_session_add_last (BurnerDataSession *self,
			       BurnerMedium *medium,
			       GError **error)
{
	BurnerDataSessionPrivate *priv;

	priv = BURNER_DATA_SESSION_PRIVATE (self);

	if (priv->nodes)
		return FALSE;

	priv->loaded = medium;
	g_object_ref (medium);

	return burner_data_session_load_directory_contents_real (self, NULL, error);
}

gboolean
burner_data_session_has_available_media (BurnerDataSession *self)
{
	BurnerDataSessionPrivate *priv;

	priv = BURNER_DATA_SESSION_PRIVATE (self);

	return priv->media != NULL;
}

GSList *
burner_data_session_get_available_media (BurnerDataSession *self)
{
	GSList *retval;
	BurnerDataSessionPrivate *priv;

	priv = BURNER_DATA_SESSION_PRIVATE (self);

	retval = g_slist_copy (priv->media);
	g_slist_foreach (retval, (GFunc) g_object_ref, NULL);

	return retval;
}

BurnerMedium *
burner_data_session_get_loaded_medium (BurnerDataSession *self)
{
	BurnerDataSessionPrivate *priv;

	priv = BURNER_DATA_SESSION_PRIVATE (self);
	if (!priv->media || !priv->nodes)
		return NULL;

	return priv->loaded;
}

static gboolean
burner_data_session_is_valid_multi (BurnerMedium *medium)
{
	BurnerMedia media;
	BurnerMedia media_status;

	media = burner_medium_get_status (medium);
	media_status = burner_burn_library_get_media_capabilities (media);

	return (media_status & BURNER_MEDIUM_WRITABLE) &&
	       (media & BURNER_MEDIUM_HAS_DATA) &&
	       (burner_medium_get_last_data_track_address (medium, NULL, NULL) != -1);
}

static void
burner_data_session_disc_added_cb (BurnerMediumMonitor *monitor,
				    BurnerMedium *medium,
				    BurnerDataSession *self)
{
	BurnerDataSessionPrivate *priv;

	priv = BURNER_DATA_SESSION_PRIVATE (self);

	if (!burner_data_session_is_valid_multi (medium))
		return;

	g_object_ref (medium);
	priv->media = g_slist_prepend (priv->media, medium);

	g_signal_emit (self,
		       burner_data_session_signals [AVAILABLE_SIGNAL],
		       0,
		       medium,
		       TRUE);
}

static void
burner_data_session_disc_removed_cb (BurnerMediumMonitor *monitor,
				      BurnerMedium *medium,
				      BurnerDataSession *self)
{
	GSList *iter;
	GSList *next;
	BurnerDataSessionPrivate *priv;

	priv = BURNER_DATA_SESSION_PRIVATE (self);

	/* see if that's the current loaded one */
	if (priv->loaded && priv->loaded == medium)
		burner_data_session_remove_last (self);

	/* remove it from our list */
	for (iter = priv->media; iter; iter = next) {
		BurnerMedium *iter_medium;

		iter_medium = iter->data;
		next = iter->next;

		if (medium == iter_medium) {
			g_signal_emit (self,
				       burner_data_session_signals [AVAILABLE_SIGNAL],
				       0,
				       medium,
				       FALSE);

			priv->media = g_slist_remove (priv->media, iter_medium);
			g_object_unref (iter_medium);
		}
	}
}

static void
burner_data_session_init (BurnerDataSession *object)
{
	GSList *iter, *list;
	BurnerMediumMonitor *monitor;
	BurnerDataSessionPrivate *priv;

	priv = BURNER_DATA_SESSION_PRIVATE (object);

	monitor = burner_medium_monitor_get_default ();
	g_signal_connect (monitor,
			  "medium-added",
			  G_CALLBACK (burner_data_session_disc_added_cb),
			  object);
	g_signal_connect (monitor,
			  "medium-removed",
			  G_CALLBACK (burner_data_session_disc_removed_cb),
			  object);

	list = burner_medium_monitor_get_media (monitor,
						 BURNER_MEDIA_TYPE_WRITABLE|
						 BURNER_MEDIA_TYPE_REWRITABLE);
	g_object_unref (monitor);

	/* check for a multisession medium already in */
	for (iter = list; iter; iter = iter->next) {
		BurnerMedium *medium;

		medium = iter->data;
		if (burner_data_session_is_valid_multi (medium)) {
			g_object_ref (medium);
			priv->media = g_slist_prepend (priv->media, medium);
		}
	}
	g_slist_foreach (list, (GFunc) g_object_unref, NULL);
	g_slist_free (list);
}

static void
burner_data_session_stop_io (BurnerDataSession *self)
{
	BurnerDataSessionPrivate *priv;

	priv = BURNER_DATA_SESSION_PRIVATE (self);

	if (priv->load_dir) {
		burner_io_cancel_by_base (priv->load_dir);
		burner_io_job_base_free (priv->load_dir);
		priv->load_dir = NULL;
	}
}

static void
burner_data_session_reset (BurnerDataProject *project,
			    guint num_nodes)
{
	burner_data_session_stop_io (BURNER_DATA_SESSION (project));

	/* chain up this function except if we invalidated the node */
	if (BURNER_DATA_PROJECT_CLASS (burner_data_session_parent_class)->reset)
		BURNER_DATA_PROJECT_CLASS (burner_data_session_parent_class)->reset (project, num_nodes);
}

static void
burner_data_session_finalize (GObject *object)
{
	BurnerDataSessionPrivate *priv;
	BurnerMediumMonitor *monitor;

	priv = BURNER_DATA_SESSION_PRIVATE (object);

	monitor = burner_medium_monitor_get_default ();
	g_signal_handlers_disconnect_by_func (monitor,
	                                      burner_data_session_disc_added_cb,
	                                      object);
	g_signal_handlers_disconnect_by_func (monitor,
	                                      burner_data_session_disc_removed_cb,
	                                      object);
	g_object_unref (monitor);

	if (priv->loaded) {
		g_object_unref (priv->loaded);
		priv->loaded = NULL;
	}

	if (priv->media) {
		g_slist_foreach (priv->media, (GFunc) g_object_unref, NULL);
		g_slist_free (priv->media);
		priv->media = NULL;
	}

	if (priv->nodes) {
		g_slist_free (priv->nodes);
		priv->nodes = NULL;
	}

	/* NOTE no need to clean up size_changed_sig since it's connected to 
	 * ourselves. It disappears with use. */

	burner_data_session_stop_io (BURNER_DATA_SESSION (object));

	/* don't care about the nodes since they will be automatically
	 * destroyed */

	G_OBJECT_CLASS (burner_data_session_parent_class)->finalize (object);
}


static void
burner_data_session_class_init (BurnerDataSessionClass *klass)
{
	GObjectClass* object_class = G_OBJECT_CLASS (klass);
	BurnerDataProjectClass *project_class = BURNER_DATA_PROJECT_CLASS (klass);

	g_type_class_add_private (klass, sizeof (BurnerDataSessionPrivate));

	object_class->finalize = burner_data_session_finalize;

	project_class->reset = burner_data_session_reset;

	burner_data_session_signals [AVAILABLE_SIGNAL] = 
	    g_signal_new ("session_available",
			  G_TYPE_FROM_CLASS (klass),
			  G_SIGNAL_RUN_LAST,
			  0,
			  NULL, NULL,
			  burner_marshal_VOID__OBJECT_BOOLEAN,
			  G_TYPE_NONE,
			  2,
			  G_TYPE_OBJECT,
			  G_TYPE_BOOLEAN);
	burner_data_session_signals [LOADED_SIGNAL] = 
	    g_signal_new ("session_loaded",
			  G_TYPE_FROM_CLASS (klass),
			  G_SIGNAL_RUN_LAST,
			  0,
			  NULL, NULL,
			  burner_marshal_VOID__OBJECT_BOOLEAN,
			  G_TYPE_NONE,
			  2,
			  G_TYPE_OBJECT,
			  G_TYPE_BOOLEAN);
}
