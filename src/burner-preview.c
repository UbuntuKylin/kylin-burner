/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/*
 * burner
 * Copyright (C) Philippe Rouquier 2007-2008 <bonfire-app@wanadoo.fr>
 * 
 *  Burner is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 * 
 * burner is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with burner.  If not, write to:
 * 	The Free Software Foundation, Inc.,
 * 	51 Franklin Street, Fifth Floor
 * 	Boston, MA  02110-1301, USA.
 */

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#ifdef BUILD_PREVIEW

#include <string.h>

#include <glib.h>
#include <glib/gi18n-lib.h>

#include <gtk/gtk.h>

#include "burner-player.h"
#include "burner-preview.h"

typedef struct _BurnerPreviewPrivate BurnerPreviewPrivate;
struct _BurnerPreviewPrivate
{
	GtkWidget *player;

	guint set_uri_id;

	gchar *uri;
	gint64 start;
	gint64 end;

	guint is_enabled:1;
};

#define BURNER_PREVIEW_PRIVATE(o)  (G_TYPE_INSTANCE_GET_PRIVATE ((o), BURNER_TYPE_PREVIEW, BurnerPreviewPrivate))

G_DEFINE_TYPE (BurnerPreview, burner_preview, GTK_TYPE_ALIGNMENT);

static gboolean
burner_preview_set_uri_delayed_cb (gpointer data)
{
	BurnerPreview *self = BURNER_PREVIEW (data);
	BurnerPreviewPrivate *priv;

	priv = BURNER_PREVIEW_PRIVATE (self);

	burner_player_set_uri (BURNER_PLAYER (priv->player), priv->uri);
	if (priv->end >= 0 && priv->start >= 0)
		burner_player_set_boundaries (BURNER_PLAYER (priv->player),
					       priv->start,
					       priv->end);

	priv->set_uri_id = 0;
	return FALSE;
}

static void
burner_preview_source_selection_changed_cb (BurnerURIContainer *source,
					     BurnerPreview *self)
{
	BurnerPreviewPrivate *priv;
	gchar *uri;

	priv = BURNER_PREVIEW_PRIVATE (self);

	/* make sure that we're supposed to activate preview */
	if (!priv->is_enabled)
		return;

	/* Should we always hide ? */
	uri = burner_uri_container_get_selected_uri (source);
	if (!uri)
		gtk_widget_hide (priv->player);

	/* clean the potentially previous uri information */
	priv->end = -1;
	priv->start = -1;
	if (priv->uri)
		g_free (priv->uri);

	/* set the new one */
	priv->uri = uri;
	burner_uri_container_get_boundaries (source, &priv->start, &priv->end);

	/* This delay is used in case the user searches the file he wants to display
	 * and goes through very quickly lots of other files before with arrows */
	if (!priv->set_uri_id)
		priv->set_uri_id = g_timeout_add (400,
						  burner_preview_set_uri_delayed_cb,
						  self);
}

void
burner_preview_add_source (BurnerPreview *self, BurnerURIContainer *source)
{
	g_signal_connect (source,
			  "uri-selected",
			  G_CALLBACK (burner_preview_source_selection_changed_cb),
			  self);
}

/**
 * Hides preview until another uri is set and recognised
 */
void
burner_preview_hide (BurnerPreview *self)
{
	BurnerPreviewPrivate *priv;

	priv = BURNER_PREVIEW_PRIVATE (self);
	gtk_widget_hide (priv->player);
}

void
burner_preview_set_enabled (BurnerPreview *self,
			     gboolean preview)
{
	BurnerPreviewPrivate *priv;

	priv = BURNER_PREVIEW_PRIVATE (self);
	priv->is_enabled = preview;
}

static void
burner_preview_player_error_cb (BurnerPlayer *player,
				 BurnerPreview *self)
{
	BurnerPreviewPrivate *priv;

	priv = BURNER_PREVIEW_PRIVATE (self);
	gtk_widget_hide (priv->player);
}

static void
burner_preview_player_ready_cb (BurnerPlayer *player,
				 BurnerPreview *self)
{
	BurnerPreviewPrivate *priv;

	priv = BURNER_PREVIEW_PRIVATE (self);
	gtk_widget_show (priv->player);
}

static void
burner_preview_init (BurnerPreview *object)
{
	BurnerPreviewPrivate *priv;

	priv = BURNER_PREVIEW_PRIVATE (object);

	priv->player = burner_player_new ();
	gtk_container_set_border_width (GTK_CONTAINER (priv->player), 4);

	gtk_container_add (GTK_CONTAINER (object), priv->player);
	g_signal_connect (priv->player,
			  "error",
			  G_CALLBACK (burner_preview_player_error_cb),
			  object);
	g_signal_connect (priv->player,
			  "ready",
			  G_CALLBACK (burner_preview_player_ready_cb),
			  object);
}

static void
burner_preview_finalize (GObject *object)
{
	BurnerPreviewPrivate *priv;

	priv = BURNER_PREVIEW_PRIVATE (object);

	if (priv->set_uri_id) {
		g_source_remove (priv->set_uri_id);
		priv->set_uri_id = 0;
	}

	if (priv->uri) {
		g_free (priv->uri);
		priv->uri = NULL;
	}

	G_OBJECT_CLASS (burner_preview_parent_class)->finalize (object);
}

static void
burner_preview_class_init (BurnerPreviewClass *klass)
{
	GObjectClass* object_class = G_OBJECT_CLASS (klass);

	g_type_class_add_private (klass, sizeof (BurnerPreviewPrivate));

	object_class->finalize = burner_preview_finalize;
}

GtkWidget *
burner_preview_new (void)
{
	return g_object_new (BURNER_TYPE_PREVIEW, NULL);
}

#endif /* BUILD_PREVIEW */
