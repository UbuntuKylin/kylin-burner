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

#include "burner-drive.h"
#include "burner-medium.h"

#include "burn-debug.h"
#include "burner-session-helper.h"
#include "burner-track.h"
#include "burner-track-data.h"
#include "burner-track-data-cfg.h"
#include "burner-session-span.h"

typedef struct _BurnerSessionSpanPrivate BurnerSessionSpanPrivate;
struct _BurnerSessionSpanPrivate
{
	GSList * track_list;
	BurnerTrack * last_track;
};

#define BURNER_SESSION_SPAN_PRIVATE(o)  (G_TYPE_INSTANCE_GET_PRIVATE ((o), BURNER_TYPE_SESSION_SPAN, BurnerSessionSpanPrivate))

G_DEFINE_TYPE (BurnerSessionSpan, burner_session_span, BURNER_TYPE_BURN_SESSION);

/**
 * burner_session_span_get_max_space:
 * @session: a #BurnerSessionSpan
 *
 * Returns the maximum required space (in sectors) 
 * among all the possible spanned batches.
 * This means that when burningto a media
 * it will also be the minimum required
 * space to burn all the contents in several
 * batches.
 *
 * Return value: a #goffset.
 **/

goffset
burner_session_span_get_max_space (BurnerSessionSpan *session)
{
	GSList *tracks;
	goffset max_sectors = 0;
	BurnerSessionSpanPrivate *priv;

	g_return_val_if_fail (BURNER_IS_SESSION_SPAN (session), 0);

	priv = BURNER_SESSION_SPAN_PRIVATE (session);

	if (priv->last_track) {
		tracks = g_slist_find (priv->track_list, priv->last_track);

		if (!tracks->next)
			return 0;

		tracks = tracks->next;
	}
	else if (priv->track_list)
		tracks = priv->track_list;
	else
		tracks = burner_burn_session_get_tracks (BURNER_BURN_SESSION (session));

	for (; tracks; tracks = tracks->next) {
		BurnerTrack *track;
		goffset track_blocks = 0;

		track = tracks->data;

		if (BURNER_IS_TRACK_DATA_CFG (track))
			return burner_track_data_cfg_span_max_space (BURNER_TRACK_DATA_CFG (track));

		/* This is the common case */
		burner_track_get_size (BURNER_TRACK (track),
					&track_blocks,
					NULL);

		max_sectors = MAX (max_sectors, track_blocks);
	}

	return max_sectors;
}

/**
 * burner_session_span_again:
 * @session: a #BurnerSessionSpan
 *
 * Checks whether some data were not included during calls to burner_session_span_next ().
 *
 * Return value: a #BurnerBurnResult. BURNER_BURN_OK if there is not anymore data.
 * BURNER_BURN_RETRY if the operation was successful and a new #BurnerTrackDataCfg was created.
 * BURNER_BURN_ERR otherwise.
 **/

BurnerBurnResult
burner_session_span_again (BurnerSessionSpan *session)
{
	GSList *tracks;
	BurnerTrack *track;
	BurnerSessionSpanPrivate *priv;

	g_return_val_if_fail (BURNER_IS_SESSION_SPAN (session), BURNER_BURN_ERR);

	priv = BURNER_SESSION_SPAN_PRIVATE (session);

	/* This is not an error */
	if (!priv->track_list)
		return BURNER_BURN_OK;

	if (priv->last_track) {
		tracks = g_slist_find (priv->track_list, priv->last_track);
		if (!tracks->next) {
			priv->track_list = NULL;
			return BURNER_BURN_OK;
		}

		return BURNER_BURN_RETRY;
	}

	tracks = priv->track_list;
	track = tracks->data;

	if (BURNER_IS_TRACK_DATA_CFG (track))
		return burner_track_data_cfg_span_again (BURNER_TRACK_DATA_CFG (track));

	return (tracks != NULL)? BURNER_BURN_RETRY:BURNER_BURN_OK;
}

/**
 * burner_session_span_possible:
 * @session: a #BurnerSessionSpan
 *
 * Checks if a new #BurnerTrackData can be created from the files remaining in the tree 
 * after calls to burner_session_span_next (). The maximum size of the data will be the one
 * of the medium inserted in the #BurnerDrive set for @session (see burner_burn_session_set_burner ()).
 *
 * Return value: a #BurnerBurnResult. BURNER_BURN_OK if there is not anymore data.
 * BURNER_BURN_RETRY if the operation was successful and a new #BurnerTrackDataCfg was created.
 * BURNER_BURN_ERR otherwise.
 **/

BurnerBurnResult
burner_session_span_possible (BurnerSessionSpan *session)
{
	GSList *tracks;
	BurnerTrack *track;
	goffset max_sectors = 0;
	goffset track_blocks = 0;
	BurnerSessionSpanPrivate *priv;

	g_return_val_if_fail (BURNER_IS_SESSION_SPAN (session), BURNER_BURN_ERR);

	priv = BURNER_SESSION_SPAN_PRIVATE (session);

	max_sectors = burner_burn_session_get_available_medium_space (BURNER_BURN_SESSION (session));
	if (max_sectors <= 0)
		return BURNER_BURN_ERR;

	if (!priv->track_list)
		tracks = burner_burn_session_get_tracks (BURNER_BURN_SESSION (session));
	else if (priv->last_track) {
		tracks = g_slist_find (priv->track_list, priv->last_track);
		if (!tracks->next) {
			priv->track_list = NULL;
			return BURNER_BURN_OK;
		}
		tracks = tracks->next;
	}
	else
		tracks = priv->track_list;

	if (!tracks)
		return BURNER_BURN_ERR;

	track = tracks->data;

	if (BURNER_IS_TRACK_DATA_CFG (track))
		return burner_track_data_cfg_span_possible (BURNER_TRACK_DATA_CFG (track), max_sectors);

	/* This is the common case */
	burner_track_get_size (BURNER_TRACK (track),
				&track_blocks,
				NULL);

	if (track_blocks >= max_sectors)
		return BURNER_BURN_ERR;

	return BURNER_BURN_RETRY;
}

/**
 * burner_session_span_start:
 * @session: a #BurnerSessionSpan
 *
 * Get the object ready for spanning a #BurnerBurnSession object. This function
 * must be called before burner_session_span_next ().
 *
 * Return value: a #BurnerBurnResult. BURNER_BURN_OK if successful.
 **/

BurnerBurnResult
burner_session_span_start (BurnerSessionSpan *session)
{
	BurnerSessionSpanPrivate *priv;

	g_return_val_if_fail (BURNER_IS_SESSION_SPAN (session), BURNER_BURN_ERR);

	priv = BURNER_SESSION_SPAN_PRIVATE (session);

	priv->track_list = burner_burn_session_get_tracks (BURNER_BURN_SESSION (session));
	if (priv->last_track) {
		g_object_unref (priv->last_track);
		priv->last_track = NULL;
	}

	return BURNER_BURN_OK;
}

/**
 * burner_session_span_next:
 * @session: a #BurnerSessionSpan
 *
 * Sets the next batch of data to be burnt onto the medium inserted in the #BurnerDrive
 * set for @session (see burner_burn_session_set_burner ()). Its free space or it capacity
 * will be used as the maximum amount of data to be burnt.
 *
 * Return value: a #BurnerBurnResult. BURNER_BURN_OK if successful.
 **/

BurnerBurnResult
burner_session_span_next (BurnerSessionSpan *session)
{
	GSList *tracks;
	gboolean pushed = FALSE;
	goffset max_sectors = 0;
	goffset total_sectors = 0;
	BurnerSessionSpanPrivate *priv;

	g_return_val_if_fail (BURNER_IS_SESSION_SPAN (session), BURNER_BURN_ERR);

	priv = BURNER_SESSION_SPAN_PRIVATE (session);

	g_return_val_if_fail (priv->track_list != NULL, BURNER_BURN_ERR);

	max_sectors = burner_burn_session_get_available_medium_space (BURNER_BURN_SESSION (session));
	if (max_sectors <= 0)
		return BURNER_BURN_ERR;

	/* NOTE: should we pop here? */
	if (priv->last_track) {
		tracks = g_slist_find (priv->track_list, priv->last_track);
		g_object_unref (priv->last_track);
		priv->last_track = NULL;

		if (!tracks->next) {
			priv->track_list = NULL;
			return BURNER_BURN_OK;
		}
		tracks = tracks->next;
	}
	else
		tracks = priv->track_list;

	for (; tracks; tracks = tracks->next) {
		BurnerTrack *track;
		goffset track_blocks = 0;

		track = tracks->data;

		if (BURNER_IS_TRACK_DATA_CFG (track)) {
			BurnerTrackData *new_track;
			BurnerBurnResult result;

			/* NOTE: the case where track_blocks < max_blocks will
			 * be handled by burner_track_data_cfg_span () */

			/* This track type is the only one to be able to span itself */
			new_track = burner_track_data_new ();
			result = burner_track_data_cfg_span (BURNER_TRACK_DATA_CFG (track),
							      max_sectors,
							      new_track);
			if (result != BURNER_BURN_RETRY) {
				g_object_unref (new_track);
				return result;
			}

			pushed = TRUE;
			burner_burn_session_push_tracks (BURNER_BURN_SESSION (session));
			burner_burn_session_add_track (BURNER_BURN_SESSION (session),
							BURNER_TRACK (new_track),
							NULL);
			break;
		}

		/* This is the common case */
		burner_track_get_size (BURNER_TRACK (track),
					&track_blocks,
					NULL);

		/* NOTE: keep the order of tracks */
		if (track_blocks + total_sectors >= max_sectors) {
			BURNER_BURN_LOG ("Reached end of spanned size");
			break;
		}

		total_sectors += track_blocks;

		if (!pushed) {
			BURNER_BURN_LOG ("Pushing tracks for media spanning");
			burner_burn_session_push_tracks (BURNER_BURN_SESSION (session));
			pushed = TRUE;
		}

		BURNER_BURN_LOG ("Adding tracks");
		burner_burn_session_add_track (BURNER_BURN_SESSION (session), track, NULL);

		if (priv->last_track)
			g_object_unref (priv->last_track);

		priv->last_track = g_object_ref (track);
	}

	/* If we pushed anything it means we succeeded */
	return (pushed? BURNER_BURN_RETRY:BURNER_BURN_ERR);
}

/**
 * burner_session_span_stop:
 * @session: a #BurnerSessionSpan
 *
 * Ends and cleans a spanning operation started with burner_session_span_start ().
 *
 **/

void
burner_session_span_stop (BurnerSessionSpan *session)
{
	BurnerSessionSpanPrivate *priv;

	g_return_if_fail (BURNER_IS_SESSION_SPAN (session));

	priv = BURNER_SESSION_SPAN_PRIVATE (session);

	if (priv->last_track) {
		g_object_unref (priv->last_track);
		priv->last_track = NULL;
	}
	else if (priv->track_list) {
		BurnerTrack *track;

		track = priv->track_list->data;
		if (BURNER_IS_TRACK_DATA_CFG (track))
			burner_track_data_cfg_span_stop (BURNER_TRACK_DATA_CFG (track));
	}

	priv->track_list = NULL;
}

static void
burner_session_span_init (BurnerSessionSpan *object)
{ }

static void
burner_session_span_finalize (GObject *object)
{
	burner_session_span_stop (BURNER_SESSION_SPAN (object));
	G_OBJECT_CLASS (burner_session_span_parent_class)->finalize (object);
}

static void
burner_session_span_class_init (BurnerSessionSpanClass *klass)
{
	GObjectClass* object_class = G_OBJECT_CLASS (klass);

	g_type_class_add_private (klass, sizeof (BurnerSessionSpanPrivate));

	object_class->finalize = burner_session_span_finalize;
}

/**
 * burner_session_span_new:
 *
 * Creates a new #BurnerSessionSpan object.
 *
 * Return value: a #BurnerSessionSpan object
 **/

BurnerSessionSpan *
burner_session_span_new (void)
{
	return g_object_new (BURNER_TYPE_SESSION_SPAN, NULL);
}

