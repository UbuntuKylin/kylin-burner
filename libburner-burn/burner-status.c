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


#include "burner-status.h"


typedef struct _BurnerStatusPrivate BurnerStatusPrivate;
struct _BurnerStatusPrivate
{
	BurnerBurnResult res;
	GError * error;
	gdouble progress;
	gchar * current_action;
};

#define BURNER_STATUS_PRIVATE(o)  (G_TYPE_INSTANCE_GET_PRIVATE ((o), BURNER_TYPE_STATUS, BurnerStatusPrivate))

G_DEFINE_TYPE (BurnerStatus, burner_status, G_TYPE_OBJECT);


/**
 * burner_status_new:
 *
 * Creates a new #BurnerStatus object.
 *
 * Return value: a #BurnerStatus.
 **/

BurnerStatus *
burner_status_new (void)
{
	return g_object_new (BURNER_TYPE_STATUS, NULL);
}

/**
 * burner_status_get_result:
 * @status: a #BurnerStatus.
 *
 * After an object (see burner_burn_track_get_status ()) has
 * been requested its status, this function returns that status.
 *
 * Return value: a #BurnerBurnResult.
 * BURNER_BURN_OK if the object is ready.
 * BURNER_BURN_NOT_READY if some time should be given to the object before it is ready.
 * BURNER_BURN_ERR if there is an error.
 **/

BurnerBurnResult
burner_status_get_result (BurnerStatus *status)
{
	BurnerStatusPrivate *priv;

	g_return_val_if_fail (status != NULL, BURNER_BURN_ERR);
	g_return_val_if_fail (BURNER_IS_STATUS (status), BURNER_BURN_ERR);

	priv = BURNER_STATUS_PRIVATE (status);
	return priv->res;
}

/**
 * burner_status_get_progress:
 * @status: a #BurnerStatus.
 *
 * If burner_status_get_result () returned BURNER_BURN_NOT_READY,
 * this function returns the progress regarding the operation completion.
 *
 * Return value: a #gdouble
 **/

gdouble
burner_status_get_progress (BurnerStatus *status)
{
	BurnerStatusPrivate *priv;

	g_return_val_if_fail (status != NULL, -1.0);
	g_return_val_if_fail (BURNER_IS_STATUS (status), -1.0);

	priv = BURNER_STATUS_PRIVATE (status);
	if (priv->res == BURNER_BURN_OK)
		return 1.0;

	if (priv->res != BURNER_BURN_NOT_READY)
		return -1.0;

	return priv->progress;
}

/**
 * burner_status_get_error:
 * @status: a #BurnerStatus.
 *
 * If burner_status_get_result () returned BURNER_BURN_ERR,
 * this function returns the error.
 *
 * Return value: a #GError
 **/

GError *
burner_status_get_error (BurnerStatus *status)
{
	BurnerStatusPrivate *priv;

	g_return_val_if_fail (status != NULL, NULL);
	g_return_val_if_fail (BURNER_IS_STATUS (status), NULL);

	priv = BURNER_STATUS_PRIVATE (status);
	if (priv->res != BURNER_BURN_ERR)
		return NULL;

	return g_error_copy (priv->error);
}

/**
 * burner_status_get_current_action:
 * @status: a #BurnerStatus.
 *
 * If burner_status_get_result () returned BURNER_BURN_NOT_READY,
 * this function returns a string describing the operation currently performed.
 * Free the string when it is not needed anymore.
 *
 * Return value: a #gchar.
 **/

gchar *
burner_status_get_current_action (BurnerStatus *status)
{
	gchar *string;
	BurnerStatusPrivate *priv;

	g_return_val_if_fail (status != NULL, NULL);
	g_return_val_if_fail (BURNER_IS_STATUS (status), NULL);

	priv = BURNER_STATUS_PRIVATE (status);

	if (priv->res != BURNER_BURN_NOT_READY)
		return NULL;

	string = g_strdup (priv->current_action);
	return string;

}

/**
 * burner_status_set_completed:
 * @status: a #BurnerStatus.
 *
 * Sets the status for a request to BURNER_BURN_OK.
 *
 **/

void
burner_status_set_completed (BurnerStatus *status)
{
	BurnerStatusPrivate *priv;

	g_return_if_fail (status != NULL);
	g_return_if_fail (BURNER_IS_STATUS (status));

	priv = BURNER_STATUS_PRIVATE (status);

	priv->res = BURNER_BURN_OK;
	priv->progress = 1.0;
}

/**
 * burner_status_set_not_ready:
 * @status: a #BurnerStatus.
 * @progress: a #gdouble or -1.0.
 * @current_action: a #gchar or NULL.
 *
 * Sets the status for a request to BURNER_BURN_NOT_READY.
 * Allows to set a string describing the operation currently performed
 * as well as the progress regarding the operation completion.
 *
 **/

void
burner_status_set_not_ready (BurnerStatus *status,
			      gdouble progress,
			      const gchar *current_action)
{
	BurnerStatusPrivate *priv;

	g_return_if_fail (status != NULL);
	g_return_if_fail (BURNER_IS_STATUS (status));

	priv = BURNER_STATUS_PRIVATE (status);

	priv->res = BURNER_BURN_NOT_READY;
	priv->progress = progress;

	if (priv->current_action)
		g_free (priv->current_action);
	priv->current_action = g_strdup (current_action);
}

/**
 * burner_status_set_running:
 * @status: a #BurnerStatus.
 * @progress: a #gdouble or -1.0.
 * @current_action: a #gchar or NULL.
 *
 * Sets the status for a request to BURNER_BURN_RUNNING.
 * Allows to set a string describing the operation currently performed
 * as well as the progress regarding the operation completion.
 *
 **/

void
burner_status_set_running (BurnerStatus *status,
			    gdouble progress,
			    const gchar *current_action)
{
	BurnerStatusPrivate *priv;

	g_return_if_fail (status != NULL);
	g_return_if_fail (BURNER_IS_STATUS (status));

	priv = BURNER_STATUS_PRIVATE (status);

	priv->res = BURNER_BURN_RUNNING;
	priv->progress = progress;

	if (priv->current_action)
		g_free (priv->current_action);
	priv->current_action = g_strdup (current_action);
}

/**
 * burner_status_set_error:
 * @status: a #BurnerStatus.
 * @error: a #GError or NULL.
 *
 * Sets the status for a request to BURNER_BURN_ERR.
 *
 **/

void
burner_status_set_error (BurnerStatus *status,
			  GError *error)
{
	BurnerStatusPrivate *priv;

	g_return_if_fail (status != NULL);
	g_return_if_fail (BURNER_IS_STATUS (status));

	priv = BURNER_STATUS_PRIVATE (status);

	priv->res = BURNER_BURN_ERR;
	priv->progress = -1.0;

	if (priv->error)
		g_error_free (priv->error);
	priv->error = error;
}

static void
burner_status_init (BurnerStatus *object)
{}

static void
burner_status_finalize (GObject *object)
{
	BurnerStatusPrivate *priv;

	priv = BURNER_STATUS_PRIVATE (object);
	if (priv->error)
		g_error_free (priv->error);

	if (priv->current_action)
		g_free (priv->current_action);

	G_OBJECT_CLASS (burner_status_parent_class)->finalize (object);
}

static void
burner_status_class_init (BurnerStatusClass *klass)
{
	GObjectClass* object_class = G_OBJECT_CLASS (klass);

	g_type_class_add_private (klass, sizeof (BurnerStatusPrivate));

	object_class->finalize = burner_status_finalize;
}

