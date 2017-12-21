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

#include <sys/param.h>

#include <glib.h>
#include <glib/gstdio.h>
#include <glib/gi18n-lib.h>

#include "burner-track-data.h"
#include "burn-mkisofs-base.h"
#include "burn-debug.h"

typedef struct _BurnerTrackDataPrivate BurnerTrackDataPrivate;
struct _BurnerTrackDataPrivate
{
	BurnerImageFS fs_type;
	GSList *grafts;
	GSList *excluded;

	guint file_num;
	guint64 data_blocks;
};

#define BURNER_TRACK_DATA_PRIVATE(o)  (G_TYPE_INSTANCE_GET_PRIVATE ((o), BURNER_TYPE_TRACK_DATA, BurnerTrackDataPrivate))

G_DEFINE_TYPE (BurnerTrackData, burner_track_data, BURNER_TYPE_TRACK);

/**
 * burner_graft_point_free:
 * @graft: a #BurnerGraftPt
 *
 * Frees @graft. Do not use @grafts afterwards.
 *
 **/

void
burner_graft_point_free (BurnerGraftPt *graft)
{
	if (graft->uri)
		g_free (graft->uri);

	g_free (graft->path);
	g_free (graft);
}

/**
 * burner_graft_point_copy:
 * @graft: a #BurnerGraftPt
 *
 * Copies @graft.
 *
 * Return value: a #BurnerGraftPt.
 **/

BurnerGraftPt *
burner_graft_point_copy (BurnerGraftPt *graft)
{
	BurnerGraftPt *newgraft;

	g_return_val_if_fail (graft != NULL, NULL);

	newgraft = g_new0 (BurnerGraftPt, 1);
	newgraft->path = g_strdup (graft->path);
	if (graft->uri)
		newgraft->uri = g_strdup (graft->uri);

	return newgraft;
}

static BurnerBurnResult
burner_track_data_set_source_real (BurnerTrackData *track,
				    GSList *grafts,
				    GSList *unreadable)
{
	BurnerTrackDataPrivate *priv;

	g_return_val_if_fail (BURNER_IS_TRACK_DATA (track), BURNER_BURN_NOT_SUPPORTED);

	priv = BURNER_TRACK_DATA_PRIVATE (track);

	if (priv->grafts) {
		g_slist_foreach (priv->grafts, (GFunc) burner_graft_point_free, NULL);
		g_slist_free (priv->grafts);
	}

	if (priv->excluded) {
		g_slist_foreach (priv->excluded, (GFunc) g_free, NULL);
		g_slist_free (priv->excluded);
	}

	priv->grafts = grafts;
	priv->excluded = unreadable;
	burner_track_changed (BURNER_TRACK (track));

	return BURNER_BURN_OK;
}

/**
 * burner_track_data_set_source:
 * @track: a #BurnerTrackData.
 * @grafts: (element-type BurnerBurn.GraftPt) (in) (transfer full): a #GSList of #BurnerGraftPt.
 * @unreadable: (element-type utf8) (allow-none) (in) (transfer full): a #GSList of URIS as strings or %NULL.
 *
 * Sets the lists of grafts points (@grafts) and excluded
 * URIs (@unreadable) to be used to create an image.
 *
 * Be careful @track takes ownership of @grafts and
 * @unreadable which must not be freed afterwards.
 *
 * Return value: a #BurnerBurnResult.
 * BURNER_BURN_OK if it was successful,
 * BURNER_BURN_ERR otherwise.
 **/

BurnerBurnResult
burner_track_data_set_source (BurnerTrackData *track,
			       GSList *grafts,
			       GSList *unreadable)
{
	BurnerTrackDataClass *klass;

	g_return_val_if_fail (BURNER_IS_TRACK_DATA (track), BURNER_BURN_ERR);

	klass = BURNER_TRACK_DATA_GET_CLASS (track);
	return klass->set_source (track, grafts, unreadable);
}

static BurnerBurnResult
burner_track_data_add_fs_real (BurnerTrackData *track,
				BurnerImageFS fstype)
{
	BurnerTrackDataPrivate *priv;

	priv = BURNER_TRACK_DATA_PRIVATE (track);
	priv->fs_type |= fstype;
	return BURNER_BURN_OK;
}

/**
 * burner_track_data_add_fs:
 * @track: a #BurnerTrackData
 * @fstype: a #BurnerImageFS
 *
 * Adds one or more parameters determining the file system type
 * and various other options to create an image.
 *
 * Return value: a #BurnerBurnResult.
 * BURNER_BURN_OK if it was successful,
 * BURNER_BURN_ERR otherwise.
 **/

BurnerBurnResult
burner_track_data_add_fs (BurnerTrackData *track,
			   BurnerImageFS fstype)
{
	BurnerTrackDataClass *klass;
	BurnerImageFS fs_before;
	BurnerBurnResult result;

	g_return_val_if_fail (BURNER_IS_TRACK_DATA (track), BURNER_BURN_NOT_SUPPORTED);

	fs_before = burner_track_data_get_fs (track);
	klass = BURNER_TRACK_DATA_GET_CLASS (track);
	if (!klass->add_fs)
		return BURNER_BURN_NOT_SUPPORTED;

	result = klass->add_fs (track, fstype);
	if (result != BURNER_BURN_OK)
		return result;

	if (fs_before != burner_track_data_get_fs (track))
		burner_track_changed (BURNER_TRACK (track));

	return BURNER_BURN_OK;
}

static BurnerBurnResult
burner_track_data_rm_fs_real (BurnerTrackData *track,
			       BurnerImageFS fstype)
{
	BurnerTrackDataPrivate *priv;

	priv = BURNER_TRACK_DATA_PRIVATE (track);
	priv->fs_type &= ~(fstype);
	return BURNER_BURN_OK;
}

/**
 * burner_track_data_rm_fs:
 * @track: a #BurnerTrackData
 * @fstype: a #BurnerImageFS
 *
 * Removes one or more parameters determining the file system type
 * and various other options to create an image.
 *
 * Return value: a #BurnerBurnResult.
 * BURNER_BURN_OK if it was successful,
 * BURNER_BURN_ERR otherwise.
 **/

BurnerBurnResult
burner_track_data_rm_fs (BurnerTrackData *track,
			  BurnerImageFS fstype)
{
	BurnerTrackDataClass *klass;
	BurnerImageFS fs_before;
	BurnerBurnResult result;

	g_return_val_if_fail (BURNER_IS_TRACK_DATA (track), BURNER_BURN_NOT_SUPPORTED);

	fs_before = burner_track_data_get_fs (track);
	klass = BURNER_TRACK_DATA_GET_CLASS (track);
	if (!klass->rm_fs);
		return BURNER_BURN_NOT_SUPPORTED;

	result = klass->rm_fs (track, fstype);
	if (result != BURNER_BURN_OK)
		return result;

	if (fs_before != burner_track_data_get_fs (track))
		burner_track_changed (BURNER_TRACK (track));

	return BURNER_BURN_OK;
}

/**
 * burner_track_data_set_data_blocks:
 * @track: a #BurnerTrackData
 * @blocks: a #goffset
 *
 * Sets the size of the image to be created (in sectors of 2048 bytes).
 *
 * Return value: a #BurnerBurnResult.
 * BURNER_BURN_OK if it was successful,
 * BURNER_BURN_ERR otherwise.
 **/

BurnerBurnResult
burner_track_data_set_data_blocks (BurnerTrackData *track,
				    goffset blocks)
{
	BurnerTrackDataPrivate *priv;

	g_return_val_if_fail (BURNER_IS_TRACK_DATA (track), BURNER_BURN_NOT_SUPPORTED);

	priv = BURNER_TRACK_DATA_PRIVATE (track);
	priv->data_blocks = blocks;

	return BURNER_BURN_OK;
}

/**
 * burner_track_data_set_file_num:
 * @track: a #BurnerTrackData
 * @number: a #guint64
 *
 * Sets the number of files (not directories) in @track.
 *
 * Return value: a #BurnerBurnResult.
 * BURNER_BURN_OK if it was successful,
 * BURNER_BURN_ERR otherwise.
 **/

BurnerBurnResult
burner_track_data_set_file_num (BurnerTrackData *track,
				 guint64 number)
{
	BurnerTrackDataPrivate *priv;

	g_return_val_if_fail (BURNER_IS_TRACK_DATA (track), BURNER_BURN_NOT_SUPPORTED);

	priv = BURNER_TRACK_DATA_PRIVATE (track);

	priv->file_num = number;
	return BURNER_BURN_OK;
}

/**
 * burner_track_data_get_fs:
 * @track: a #BurnerTrackData
 *
 * Returns the parameters determining the file system type
 * and various other options to create an image.
 *
 * Return value: a #BurnerImageFS.
 **/

BurnerImageFS
burner_track_data_get_fs (BurnerTrackData *track)
{
	BurnerTrackDataClass *klass;

	g_return_val_if_fail (BURNER_IS_TRACK_DATA (track), BURNER_IMAGE_FS_NONE);

	klass = BURNER_TRACK_DATA_GET_CLASS (track);
	return klass->get_fs (track);
}

static BurnerImageFS
burner_track_data_get_fs_real (BurnerTrackData *track)
{
	BurnerTrackDataPrivate *priv;

	priv = BURNER_TRACK_DATA_PRIVATE (track);
	return priv->fs_type;
}

static GHashTable *
burner_track_data_mangle_joliet_name (GHashTable *joliet,
				       const gchar *path,
				       gchar *buffer)
{
	gboolean has_slash = FALSE;
	gint dot_pos = -1;
	gint dot_len = -1;
	gchar *name;
	gint width;
	gint start;
	gint num;
	gint end;

	/* NOTE: this wouldn't work on windows (not a big deal) */
	end = strlen (path);
	if (!end) {
		buffer [0] = '\0';
		return joliet;
	}

	memcpy (buffer, path, MIN (end, MAXPATHLEN));
	buffer [MIN (end, MAXPATHLEN)] = '\0';

	/* move back until we find a character different from G_DIR_SEPARATOR */
	end --;
	while (end >= 0 && G_IS_DIR_SEPARATOR (path [end])) {
		end --;
		has_slash = TRUE;
	}

	/* There are only slashes */
	if (end == -1)
		return joliet;

	start = end - 1;
	while (start >= 0 && !G_IS_DIR_SEPARATOR (path [start])) {
		/* Find the extension while at it */
		if (dot_pos <= 0 && path [start] == '.')
			dot_pos = start;

		start --;
	}

	if (end - start <= 64)
		return joliet;

	name = buffer + start + 1;
	if (dot_pos > 0)
		dot_len = end - dot_pos + 1;

	if (dot_len > 1 && dot_len < 5)
		memcpy (name + 64 - dot_len,
			path + dot_pos,
			dot_len);

	name [64] = '\0';

	if (!joliet) {
		joliet = g_hash_table_new_full (g_str_hash,
						g_str_equal,
						g_free,
						NULL);

		g_hash_table_insert (joliet, g_strdup (buffer), GINT_TO_POINTER (1));
		if (has_slash)
			strcat (buffer, G_DIR_SEPARATOR_S);

		BURNER_BURN_LOG ("Mangled name to %s (truncated)", buffer);
		return joliet;
	}

	/* see if this path was already used */
	num = GPOINTER_TO_INT (g_hash_table_lookup (joliet, buffer));
	if (!num) {
		g_hash_table_insert (joliet, g_strdup (buffer), GINT_TO_POINTER (1));

		if (has_slash)
			strcat (buffer, G_DIR_SEPARATOR_S);

		BURNER_BURN_LOG ("Mangled name to %s (truncated)", buffer);
		return joliet;
	}

	/* NOTE: g_hash_table_insert frees key_path */
	num ++;
	g_hash_table_insert (joliet, g_strdup (buffer), GINT_TO_POINTER (num));

	width = 1;
	while (num / (width * 10)) width ++;

	/* try to keep the extension */
	if (dot_len < 5 && dot_len > 1 )
		sprintf (name + (64 - width - dot_len),
			 "%i%s",
			 num,
			 path + dot_pos);
	else
		sprintf (name + (64 - width),
			 "%i",
			 num);

	if (has_slash)
		strcat (buffer, G_DIR_SEPARATOR_S);

	BURNER_BURN_LOG ("Mangled name to %s", buffer);
	return joliet;
}

/**
 * burner_track_data_get_grafts:
 * @track: a #BurnerTrackData
 *
 * Returns a list of #BurnerGraftPt.
 *
 * Do not free after usage as @track retains ownership.
 *
 * Return value: (transfer none) (element-type BurnerBurn.GraftPt) (allow-none): a #GSList of #BurnerGraftPt or %NULL if empty.
 **/

GSList *
burner_track_data_get_grafts (BurnerTrackData *track)
{
	BurnerTrackDataClass *klass;
	GHashTable *mangle = NULL;
	BurnerImageFS image_fs;
	GSList *grafts;
	GSList *iter;

	g_return_val_if_fail (BURNER_IS_TRACK_DATA (track), NULL);

	klass = BURNER_TRACK_DATA_GET_CLASS (track);
	grafts = klass->get_grafts (track);

	image_fs = burner_track_data_get_fs (track);
	if ((image_fs & BURNER_IMAGE_FS_JOLIET) == 0)
		return grafts;

	for (iter = grafts; iter; iter = iter->next) {
		BurnerGraftPt *graft;
		gchar newpath [MAXPATHLEN];

		graft = iter->data;
		mangle = burner_track_data_mangle_joliet_name (mangle,
								graft->path,
								newpath);

		g_free (graft->path);
		graft->path = g_strdup (newpath);
	}

	if (mangle)
		g_hash_table_destroy (mangle);

	return grafts;
}

static GSList *
burner_track_data_get_grafts_real (BurnerTrackData *track)
{
	BurnerTrackDataPrivate *priv;

	priv = BURNER_TRACK_DATA_PRIVATE (track);
	return priv->grafts;
}

/**
 * burner_track_data_get_excluded_list:
 * @track: a #BurnerTrackData.
 *
 * Returns a list of URIs which must not be included in
 * the image to be created.
 * Do not free the list or any of the URIs after
 * usage as @track retains ownership.
 *
 * Return value: (transfer none) (element-type utf8) (allow-none): a #GSList of #gchar * or %NULL if no
 * URI should be excluded.
 **/

GSList *
burner_track_data_get_excluded_list (BurnerTrackData *track)
{
	BurnerTrackDataClass *klass;

	g_return_val_if_fail (BURNER_IS_TRACK_DATA (track), NULL);

	klass = BURNER_TRACK_DATA_GET_CLASS (track);
	return klass->get_excluded (track);
}

static GSList *
burner_track_data_get_excluded_real (BurnerTrackData *track)
{
	BurnerTrackDataPrivate *priv;

	priv = BURNER_TRACK_DATA_PRIVATE (track);
	return priv->excluded;
}

/**
 * burner_track_data_write_to_paths:
 * @track: a #BurnerTrackData.
 * @grafts_path: a #gchar.
 * @excluded_path: a #gchar.
 * @emptydir: a #gchar.
 * @videodir: (allow-none): a #gchar or %NULL.
 * @error: a #GError.
 *
 * Write to @grafts_path (a path to a file) the graft points,
 * and to @excluded_path (a path to a file) the list of paths to
 * be excluded; @emptydir is (path) is an empty
 * directory to be used for created directories;
 * @videodir (a path) is a directory to be used to build the
 * the video image.
 *
 * This is mostly for internal use by mkisofs and similar.
 *
 * This function takes care of file name mangling.
 *
 * Return value: a #BurnerBurnResult.
 **/

BurnerBurnResult
burner_track_data_write_to_paths (BurnerTrackData *track,
                                   const gchar *grafts_path,
                                   const gchar *excluded_path,
                                   const gchar *emptydir,
                                   const gchar *videodir,
                                   GError **error)
{
	GSList *grafts;
	GSList *excluded;
	BurnerBurnResult result;
	BurnerTrackDataClass *klass;

	g_return_val_if_fail (BURNER_IS_TRACK_DATA (track), BURNER_BURN_NOT_SUPPORTED);

	klass = BURNER_TRACK_DATA_GET_CLASS (track);
	grafts = klass->get_grafts (track);
	excluded = klass->get_excluded (track);

	result = burner_mkisofs_base_write_to_files (grafts,
						      excluded,
						      burner_track_data_get_fs (track),
						      emptydir,
						      videodir,
						      grafts_path,
						      excluded_path,
						      error);
	return result;
}

/**
 * burner_track_data_get_file_num:
 * @track: a #BurnerTrackData.
 * @file_num: (allow-none) (out): a #guint64 or %NULL.
 *
 * Sets the number of files (not directories) in @file_num.
 *
 * Return value: a #BurnerBurnResult. %TRUE if @file_num
 * was set, %FALSE otherwise.
 **/

BurnerBurnResult
burner_track_data_get_file_num (BurnerTrackData *track,
				 guint64 *file_num)
{
	BurnerTrackDataClass *klass;

	g_return_val_if_fail (BURNER_IS_TRACK_DATA (track), 0);

	klass = BURNER_TRACK_DATA_GET_CLASS (track);
	if (file_num)
		*file_num = klass->get_file_num (track);

	return BURNER_BURN_OK;
}

static guint64
burner_track_data_get_file_num_real (BurnerTrackData *track)
{
	BurnerTrackDataPrivate *priv;

	priv = BURNER_TRACK_DATA_PRIVATE (track);
	return priv->file_num;
}

static BurnerBurnResult
burner_track_data_get_size (BurnerTrack *track,
			     goffset *blocks,
			     goffset *block_size)
{
	BurnerTrackDataPrivate *priv;

	priv = BURNER_TRACK_DATA_PRIVATE (track);

	if (*block_size)
		*block_size = 2048;

	if (*blocks)
		*blocks = priv->data_blocks;

	return BURNER_BURN_OK;
}

static BurnerBurnResult
burner_track_data_get_track_type (BurnerTrack *track,
				   BurnerTrackType *type)
{
	BurnerTrackDataPrivate *priv;

	priv = BURNER_TRACK_DATA_PRIVATE (track);

	burner_track_type_set_has_data (type);
	burner_track_type_set_data_fs (type, priv->fs_type);

	return BURNER_BURN_OK;
}

static BurnerBurnResult
burner_track_data_get_status (BurnerTrack *track,
			       BurnerStatus *status)
{
	BurnerTrackDataPrivate *priv;

	priv = BURNER_TRACK_DATA_PRIVATE (track);

	if (!priv->grafts) {
		if (status)
			burner_status_set_error (status,
						  g_error_new (BURNER_BURN_ERROR,
							       BURNER_BURN_ERROR_EMPTY,
							       _("There are no files to write to disc")));
		return BURNER_BURN_ERR;
	}

	return BURNER_BURN_OK;
}

static void
burner_track_data_init (BurnerTrackData *object)
{ }

static void
burner_track_data_finalize (GObject *object)
{
	BurnerTrackDataPrivate *priv;

	priv = BURNER_TRACK_DATA_PRIVATE (object);
	if (priv->grafts) {
		g_slist_foreach (priv->grafts, (GFunc) burner_graft_point_free, NULL);
		g_slist_free (priv->grafts);
		priv->grafts = NULL;
	}

	if (priv->excluded) {
		g_slist_foreach (priv->excluded, (GFunc) g_free, NULL);
		g_slist_free (priv->excluded);
		priv->excluded = NULL;
	}

	G_OBJECT_CLASS (burner_track_data_parent_class)->finalize (object);
}

static void
burner_track_data_class_init (BurnerTrackDataClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);
	BurnerTrackClass *track_class = BURNER_TRACK_CLASS (klass);
	BurnerTrackDataClass *track_data_class = BURNER_TRACK_DATA_CLASS (klass);

	g_type_class_add_private (klass, sizeof (BurnerTrackDataPrivate));

	object_class->finalize = burner_track_data_finalize;

	track_class->get_type = burner_track_data_get_track_type;
	track_class->get_status = burner_track_data_get_status;
	track_class->get_size = burner_track_data_get_size;

	track_data_class->set_source = burner_track_data_set_source_real;
	track_data_class->add_fs = burner_track_data_add_fs_real;
	track_data_class->rm_fs = burner_track_data_rm_fs_real;

	track_data_class->get_fs = burner_track_data_get_fs_real;
	track_data_class->get_grafts = burner_track_data_get_grafts_real;
	track_data_class->get_excluded = burner_track_data_get_excluded_real;
	track_data_class->get_file_num = burner_track_data_get_file_num_real;
}

/**
 * burner_track_data_new:
 *
 * Creates a new #BurnerTrackData.
 * 
 *This type of tracks is used to create a disc image
 * from or burn a selection of files.
 *
 * Return value: a #BurnerTrackData
 **/

BurnerTrackData *
burner_track_data_new (void)
{
	return g_object_new (BURNER_TYPE_TRACK_DATA, NULL);
}
