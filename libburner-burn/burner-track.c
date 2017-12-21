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

#include <glib.h>

#include "burner-track.h"


typedef struct _BurnerTrackPrivate BurnerTrackPrivate;
struct _BurnerTrackPrivate
{
	GHashTable *tags;

	gchar *checksum;
	BurnerChecksumType checksum_type;
};

#define BURNER_TRACK_PRIVATE(o)  (G_TYPE_INSTANCE_GET_PRIVATE ((o), BURNER_TYPE_TRACK, BurnerTrackPrivate))

enum
{
	CHANGED,

	LAST_SIGNAL
};


static guint track_signals[LAST_SIGNAL] = { 0 };

G_DEFINE_TYPE (BurnerTrack, burner_track, G_TYPE_OBJECT);

/**
 * burner_track_get_track_type:
 * @track: a #BurnerTrack
 * @type: a #BurnerTrackType or NULL
 *
 * Sets @type to reflect the type of data contained in @track
 *
 * Return value: the #BurnerBurnResult of the track
 **/

BurnerBurnResult
burner_track_get_track_type (BurnerTrack *track,
			      BurnerTrackType *type)
{
	BurnerTrackClass *klass;

	g_return_val_if_fail (BURNER_IS_TRACK (track), BURNER_BURN_ERR);
	g_return_val_if_fail (type != NULL, BURNER_BURN_ERR);

	klass = BURNER_TRACK_GET_CLASS (track);
	if (!klass->get_type)
		return BURNER_BURN_ERR;

	return klass->get_type (track, type);
}

/**
 * burner_track_get_size:
 * @track: a #BurnerTrack
 * @blocks: a #goffset or NULL
 * @bytes: a #goffset or NULL
 *
 * Returns the size of the data contained by @track in bytes or in sectors
 *
 * Return value: a #BurnerBurnResult.
 * BURNER_BURN_OK if it was successful
 * BURNER_BURN_NOT_READY if @track needs more time for processing the size
 * BURNER_BURN_ERR if something is wrong or if it is empty
 **/

BurnerBurnResult
burner_track_get_size (BurnerTrack *track,
			goffset *blocks,
			goffset *bytes)
{
	BurnerBurnResult res;
	BurnerTrackClass *klass;
	goffset blocks_local = 0;
	goffset block_size_local = 0;

	g_return_val_if_fail (BURNER_IS_TRACK (track), BURNER_BURN_ERR);

	klass = BURNER_TRACK_GET_CLASS (track);
	if (!klass->get_size)
		return BURNER_BURN_OK;

	res = klass->get_size (track, &blocks_local, &block_size_local);
	if (res != BURNER_BURN_OK)
		return res;

	if (blocks)
		*blocks = blocks_local;

	if (bytes)
		*bytes = blocks_local * block_size_local;

	return BURNER_BURN_OK;
}

/**
 * burner_track_get_status:
 * @track: a #BurnerTrack
 * @status: a #BurnerTrackStatus
 *
 * Sets @status to reflect whether @track is ready to be used
 *
 * Return value: a #BurnerBurnResult.
 * BURNER_BURN_OK if it was successful
 * BURNER_BURN_NOT_READY if @track needs more time for processing
 * BURNER_BURN_ERR if something is wrong or if it is empty
 **/

BurnerBurnResult
burner_track_get_status (BurnerTrack *track,
			  BurnerStatus *status)
{
	BurnerTrackClass *klass;

	g_return_val_if_fail (BURNER_IS_TRACK (track), BURNER_BURN_ERR);

	klass = BURNER_TRACK_GET_CLASS (track);

	/* If this is not implement we consider that it means it is not needed
	 * and that the track doesn't perform on the side (threaded?) checks or
	 * information retrieval and that it's therefore always OK:
	 * - for example BurnerTrackDisc. */
	if (!klass->get_status) {
		if (status)
			burner_status_set_completed (status);

		return BURNER_BURN_OK;
	}

	return klass->get_status (track, status);
}

/**
 * burner_track_set_checksum:
 * @track: a #BurnerTrack
 * @type: a #BurnerChecksumType
 * @checksum: a #gchar * holding the checksum
 *
 * Sets a checksum for the track
 *
 * Return value: a #BurnerBurnResult.
 * BURNER_BURN_OK if the checksum was previously empty or matches the new one
 * BURNER_BURN_ERR otherwise
 **/

BurnerBurnResult
burner_track_set_checksum (BurnerTrack *track,
			    BurnerChecksumType type,
			    const gchar *checksum)
{
	BurnerBurnResult result = BURNER_BURN_OK;
	BurnerTrackPrivate *priv;

	g_return_val_if_fail (BURNER_IS_TRACK (track), BURNER_CHECKSUM_NONE);
	priv = BURNER_TRACK_PRIVATE (track);

	if (type == priv->checksum_type
	&& (type == BURNER_CHECKSUM_MD5 || type == BURNER_CHECKSUM_SHA1 || type == BURNER_CHECKSUM_SHA256)
	&&  checksum && strcmp (checksum, priv->checksum))
		result = BURNER_BURN_ERR;

	if (priv->checksum)
		g_free (priv->checksum);

	priv->checksum_type = type;
	if (checksum)
		priv->checksum = g_strdup (checksum);
	else
		priv->checksum = NULL;

	return result;
}

/**
 * burner_track_get_checksum:
 * @track: a #BurnerTrack
 *
 * Get the current checksum (as a string) for the track
 *
 * Return value: a #gchar * (not to be freed) or NULL
 **/

const gchar *
burner_track_get_checksum (BurnerTrack *track)
{
	BurnerTrackPrivate *priv;

	g_return_val_if_fail (BURNER_IS_TRACK (track), NULL);
	priv = BURNER_TRACK_PRIVATE (track);

	return priv->checksum ? priv->checksum : NULL;
}

/**
 * burner_track_get_checksum_type:
 * @track: a #BurnerTrack
 *
 * Get the current checksum type for the track if any.
 *
 * Return value: a #BurnerChecksumType
 **/

BurnerChecksumType
burner_track_get_checksum_type (BurnerTrack *track)
{
	BurnerTrackPrivate *priv;

	g_return_val_if_fail (BURNER_IS_TRACK (track), BURNER_CHECKSUM_NONE);
	priv = BURNER_TRACK_PRIVATE (track);

	return priv->checksum_type;
}

/**
 * Can be used to set arbitrary data
 */

static void
burner_track_tag_value_free (gpointer user_data)
{
	GValue *value = user_data;

	g_value_reset (value);
	g_free (value);
}

/**
 * burner_track_tag_add:
 * @track: a #BurnerTrack
 * @tag: a #gchar *
 * @value: a #GValue
 *
 * Associates a new @tag with a track. This can be used
 * to pass arbitrary information for plugins, like parameters
 * for video discs, ...
 * See burner-tags.h for a list of knowns tags.
 *
 * Return value: a #BurnerBurnResult.
 * BURNER_BURN_OK if it was successful,
 * BURNER_BURN_ERR otherwise.
 **/

BurnerBurnResult
burner_track_tag_add (BurnerTrack *track,
		       const gchar *tag,
		       GValue *value)
{
	BurnerTrackPrivate *priv;

	g_return_val_if_fail (BURNER_IS_TRACK (track), BURNER_BURN_ERR);

	priv = BURNER_TRACK_PRIVATE (track);

	if (!priv->tags)
		priv->tags = g_hash_table_new_full (g_str_hash,
						    g_str_equal,
						    g_free,
						    burner_track_tag_value_free);
	g_hash_table_insert (priv->tags,
			     g_strdup (tag),
			     value);

	return BURNER_BURN_OK;
}

/**
 * burner_track_tag_add_int:
 * @track: a #BurnerTrack
 * @tag: a #gchar *
 * @value: a #int
 *
 * A wrapper around burner_track_tag_add () to associate
 * a int value with @track
 * See also burner_track_tag_add ()
 *
 * Return value: a #BurnerBurnResult.
 * BURNER_BURN_OK if it was successful,
 * BURNER_BURN_ERR otherwise.
 **/

BurnerBurnResult
burner_track_tag_add_int (BurnerTrack *track,
			   const gchar *tag,
			   int value_int)
{
	GValue *value;

	value = g_new0 (GValue, 1);
	g_value_init (value, G_TYPE_INT);
	g_value_set_int (value, value_int);

	return burner_track_tag_add (track, tag, value);
}

/**
 * burner_track_tag_add_string:
 * @track: a #BurnerTrack
 * @tag: a #gchar *
 * @string: a #gchar *
 *
 * A wrapper around burner_track_tag_add () to associate
 * a string with @track
 * See also burner_track_tag_add ()
 *
 * Return value: a #BurnerBurnResult.
 * BURNER_BURN_OK if it was successful,
 * BURNER_BURN_ERR otherwise.
 **/

BurnerBurnResult
burner_track_tag_add_string (BurnerTrack *track,
			      const gchar *tag,
			      const gchar *string)
{
	GValue *value;

	value = g_new0 (GValue, 1);
	g_value_init (value, G_TYPE_STRING);
	g_value_set_string (value, string);

	return burner_track_tag_add (track, tag, value);
}

/**
 * burner_track_tag_lookup:
 * @track: a #BurnerTrack
 * @tag: a #gchar *
 * @value: a #GValue **
 *
 * Retrieves a value associated with @track through
 * burner_track_tag_add () and stores it in @value. Do
 * not destroy @value afterwards as it is not a copy
 *
 * Return value: a #BurnerBurnResult.
 * BURNER_BURN_OK if the retrieval was successful
 * BURNER_BURN_ERR otherwise
 **/

BurnerBurnResult
burner_track_tag_lookup (BurnerTrack *track,
			  const gchar *tag,
			  GValue **value)
{
	gpointer data;
	BurnerTrackPrivate *priv;

	g_return_val_if_fail (BURNER_IS_TRACK (track), BURNER_BURN_ERR);

	priv = BURNER_TRACK_PRIVATE (track);

	if (!priv->tags)
		return BURNER_BURN_ERR;

	data = g_hash_table_lookup (priv->tags, tag);
	if (!data)
		return BURNER_BURN_ERR;

	if (value)
		*value = data;

	return BURNER_BURN_OK;
}

/**
 * burner_track_tag_lookup_int:
 * @track: a #BurnerTrack
 * @tag: a #gchar *
 *
 * Retrieves a int value associated with @track. This
 * is a wrapper around burner_track_tag_lookup ().
 *
 * Return value: a #int; the value or 0 otherwise
 **/

int
burner_track_tag_lookup_int (BurnerTrack *track,
			      const gchar *tag)
{
	GValue *value = NULL;
	BurnerBurnResult res;

	res = burner_track_tag_lookup (track, tag, &value);
	if (res != BURNER_BURN_OK)
		return 0;

	if (!value)
		return 0;

	if (!G_VALUE_HOLDS_INT (value))
		return 0;

	return g_value_get_int (value);
}

/**
 * burner_track_tag_lookup_string:
 * @track: a #BurnerTrack
 * @tag: a #gchar *
 *
 * Retrieves a string value associated with @track. This
 * is a wrapper around burner_track_tag_lookup ().
 *
 * Return value: a #gchar *. The value or NULL otherwise.
 * Do not free the string as it is not a copy.
 **/

const gchar *
burner_track_tag_lookup_string (BurnerTrack *track,
				 const gchar *tag)
{
	GValue *value = NULL;
	BurnerBurnResult res;

	res = burner_track_tag_lookup (track, tag, &value);
	if (res != BURNER_BURN_OK)
		return NULL;

	if (!value)
		return NULL;

	if (!G_VALUE_HOLDS_STRING (value))
		return NULL;

	return g_value_get_string (value);
}

/**
 * burner_track_tag_copy_missing:
 * @dest: a #BurnerTrack
 * @src: a #BurnerTrack
 *
 * Adds all tags of @dest to @src provided they do not
 * already exists.
 *
 **/

void
burner_track_tag_copy_missing (BurnerTrack *dest,
				BurnerTrack *src)
{
	BurnerTrackPrivate *priv;
	GHashTableIter iter;
	gpointer new_value;
	gpointer new_key;
	gpointer value;
	gpointer key;

	g_return_if_fail (BURNER_IS_TRACK (dest));
	g_return_if_fail (BURNER_IS_TRACK (src));

	priv = BURNER_TRACK_PRIVATE (src);

	if (!priv->tags)
		return;

	g_hash_table_iter_init (&iter, priv->tags);

	priv = BURNER_TRACK_PRIVATE (dest);
	if (!priv->tags)
		priv->tags = g_hash_table_new_full (g_str_hash,
						    g_str_equal,
						    g_free,
						    burner_track_tag_value_free);

	while (g_hash_table_iter_next (&iter, &key, &value)) {
		if (g_hash_table_lookup (priv->tags, key))
			continue;

		new_value = g_new0 (GValue, 1);

		g_value_init (new_value, G_VALUE_TYPE (value));
		g_value_copy (value, new_value);

		new_key = g_strdup (key);

		g_hash_table_insert (priv->tags, new_key, new_value);
	}
}

/**
 * burner_track_changed:
 * @track: a #BurnerTrack
 * 
 * Used internally in #BurnerTrack implementations to 
 * signal a #BurnerTrack object has changed.
 *
 **/

void
burner_track_changed (BurnerTrack *track)
{
	g_signal_emit (track,
		       track_signals [CHANGED],
		       0);
}


/**
 * GObject part
 */

static void
burner_track_init (BurnerTrack *object)
{ }

static void
burner_track_finalize (GObject *object)
{
	BurnerTrackPrivate *priv;

	priv = BURNER_TRACK_PRIVATE (object);

	if (priv->tags) {
		g_hash_table_destroy (priv->tags);
		priv->tags = NULL;
	}

	if (priv->checksum) {
		g_free (priv->checksum);
		priv->checksum = NULL;
	}

	G_OBJECT_CLASS (burner_track_parent_class)->finalize (object);
}

static void
burner_track_class_init (BurnerTrackClass *klass)
{
	GObjectClass* object_class = G_OBJECT_CLASS (klass);

	g_type_class_add_private (klass, sizeof (BurnerTrackPrivate));

	object_class->finalize = burner_track_finalize;

	track_signals[CHANGED] =
		g_signal_new ("changed",
		              G_OBJECT_CLASS_TYPE (klass),
		              G_SIGNAL_RUN_FIRST | G_SIGNAL_NO_RECURSE,
		              G_STRUCT_OFFSET (BurnerTrackClass, changed),
		              NULL, NULL,
		              g_cclosure_marshal_VOID__VOID,
		              G_TYPE_NONE, 0,
		              G_TYPE_NONE);
}
