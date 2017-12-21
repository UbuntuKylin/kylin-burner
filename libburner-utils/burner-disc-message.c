/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/*
 * Libburner-misc
 * Copyright (C) Philippe Rouquier 2005-2009 <bonfire-app@wanadoo.fr>
 *
 * Libburner-misc is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * The Libburner-misc authors hereby grant permission for non-GPL compatible
 * GStreamer plugins to be used and distributed together with GStreamer
 * and Libburner-misc. This permission is above and beyond the permissions granted
 * by the GPL license by which Libburner-burn is covered. If you modify this code
 * you may extend this exception to your version of the code, but you are not
 * obligated to do so. If you do not wish to do so, delete this exception
 * statement from your version.
 * 
 * Libburner-misc is distributed in the hope that it will be useful,
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

#include <gdk/gdkkeysyms.h>

#include <gtk/gtk.h>

#include "burner-disc-message.h"


typedef struct _BurnerDiscMessagePrivate BurnerDiscMessagePrivate;
struct _BurnerDiscMessagePrivate
{
	GtkWidget *progress;

	GtkWidget *expander;

	GtkWidget *primary;
	GtkWidget *secondary;

	GtkWidget *text_box;

	guint context;

	guint id;
	guint timeout;

	guint prevent_destruction:1;
};

#define BURNER_DISC_MESSAGE_PRIVATE(o)  (G_TYPE_INSTANCE_GET_PRIVATE ((o), BURNER_TYPE_DISC_MESSAGE, BurnerDiscMessagePrivate))

G_DEFINE_TYPE (BurnerDiscMessage, burner_disc_message, GTK_TYPE_INFO_BAR);

enum {
	TEXT_COL,
	NUM_COL
};

static gboolean
burner_disc_message_timeout (gpointer data)
{
	BurnerDiscMessagePrivate *priv;

	priv = BURNER_DISC_MESSAGE_PRIVATE (data);
	priv->timeout = 0;

	priv->prevent_destruction = TRUE;
	gtk_info_bar_response (data, GTK_RESPONSE_DELETE_EVENT);
	priv->prevent_destruction = FALSE;

	gtk_widget_destroy (GTK_WIDGET (data));
	return FALSE;
}

void
burner_disc_message_set_timeout (BurnerDiscMessage *self,
				  guint mseconds)
{
	BurnerDiscMessagePrivate *priv;

	priv = BURNER_DISC_MESSAGE_PRIVATE (self);

	if (priv->timeout) {
		g_source_remove (priv->timeout);
		priv->timeout = 0;
	}

	if (mseconds > 0)
		priv->timeout = g_timeout_add (mseconds,
					       burner_disc_message_timeout,
					       self);
}

void
burner_disc_message_set_context (BurnerDiscMessage *self,
				  guint context_id)
{
	BurnerDiscMessagePrivate *priv;

	priv = BURNER_DISC_MESSAGE_PRIVATE (self);
	priv->context = context_id;
}

guint
burner_disc_message_get_context (BurnerDiscMessage *self)
{
	BurnerDiscMessagePrivate *priv;

	priv = BURNER_DISC_MESSAGE_PRIVATE (self);
	return priv->context;
}

static void
burner_disc_message_expander_activated_cb (GtkExpander *expander,
					    BurnerDiscMessage *self)
{
	if (!gtk_expander_get_expanded (expander))
		gtk_expander_set_label (expander, _("_Hide changes"));
	else
		gtk_expander_set_label (expander, _("_Show changes"));
}

void
burner_disc_message_add_errors (BurnerDiscMessage *self,
				 GSList *errors)
{
	BurnerDiscMessagePrivate *priv;
	GtkCellRenderer *renderer;
	GtkTreeViewColumn *column;
	GtkListStore *model;
	GtkWidget *scroll;
	GtkWidget *tree;

	priv = BURNER_DISC_MESSAGE_PRIVATE (self);

	if (priv->expander)
		gtk_widget_destroy (priv->expander);

	priv->expander = gtk_expander_new_with_mnemonic (_("_Show changes"));
	gtk_widget_show (priv->expander);
	gtk_box_pack_start (GTK_BOX (priv->text_box), priv->expander, FALSE, TRUE, 0);

	g_signal_connect (priv->expander,
			  "activate",
			  G_CALLBACK (burner_disc_message_expander_activated_cb),
			  self);

	model = gtk_list_store_new (NUM_COL, G_TYPE_STRING);

	tree = gtk_tree_view_new_with_model (GTK_TREE_MODEL (model));
	gtk_widget_show (tree);

	g_object_unref (G_OBJECT (model));

	gtk_tree_view_set_rules_hint (GTK_TREE_VIEW (tree), TRUE);
	gtk_tree_selection_set_mode (gtk_tree_view_get_selection (GTK_TREE_VIEW (tree)),
				     GTK_SELECTION_NONE);

	renderer = gtk_cell_renderer_text_new ();
	column = gtk_tree_view_column_new_with_attributes ("error",
							   renderer,
							   "text", TEXT_COL,
							   NULL);
	gtk_tree_view_append_column (GTK_TREE_VIEW (tree), column);
	gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (tree), FALSE);

	scroll = gtk_scrolled_window_new (NULL, NULL);
	gtk_widget_show (scroll);
	gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scroll),
					     GTK_SHADOW_IN);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scroll),
					GTK_POLICY_AUTOMATIC,
					GTK_POLICY_AUTOMATIC);
	gtk_container_add (GTK_CONTAINER (scroll), tree);
	gtk_container_add (GTK_CONTAINER (priv->expander), scroll);

	for (; errors; errors = errors->next) {
		GtkTreeIter iter;

		gtk_list_store_append (model, &iter);
		gtk_list_store_set (model, &iter,
				    TEXT_COL, errors->data,
				    -1);
	}
}

void
burner_disc_message_remove_errors (BurnerDiscMessage *self)
{
	BurnerDiscMessagePrivate *priv;

	priv = BURNER_DISC_MESSAGE_PRIVATE (self);

	gtk_widget_destroy (priv->expander);
	priv->expander = NULL;
}

void
burner_disc_message_destroy (BurnerDiscMessage *self)
{
	BurnerDiscMessagePrivate *priv;

	priv = BURNER_DISC_MESSAGE_PRIVATE (self);

	if (priv->prevent_destruction)
		return;

	gtk_widget_destroy (GTK_WIDGET (self));
}

void
burner_disc_message_remove_buttons (BurnerDiscMessage *self)
{
	gtk_container_foreach (GTK_CONTAINER (gtk_info_bar_get_action_area (GTK_INFO_BAR (self))),
			       (GtkCallback) gtk_widget_destroy,
			       NULL);
}

static gboolean
burner_disc_message_update_progress (gpointer data)
{
	BurnerDiscMessagePrivate *priv;

	priv = BURNER_DISC_MESSAGE_PRIVATE (data);
	gtk_progress_bar_pulse (GTK_PROGRESS_BAR (priv->progress));
	return TRUE;
}

void
burner_disc_message_set_progress_active (BurnerDiscMessage *self,
					  gboolean active)
{
	BurnerDiscMessagePrivate *priv;

	priv = BURNER_DISC_MESSAGE_PRIVATE (self);

	if (!priv->progress) {
		priv->progress = gtk_progress_bar_new ();
		gtk_box_pack_start (GTK_BOX (priv->text_box), priv->progress, FALSE, TRUE, 0);
	}

	if (active) {
		gtk_widget_show (priv->progress);

		if (!priv->id)
			priv->id = g_timeout_add (150,
						  (GSourceFunc) burner_disc_message_update_progress,
						  self);
	}
	else {
		gtk_widget_hide (priv->progress);
		if (priv->id) {
			g_source_remove (priv->id);
			priv->id = 0;
		}
	}
}

void
burner_disc_message_set_progress (BurnerDiscMessage *self,
				   gdouble progress)
{
	BurnerDiscMessagePrivate *priv;

	priv = BURNER_DISC_MESSAGE_PRIVATE (self);

	if (!priv->progress) {
		priv->progress = gtk_progress_bar_new ();
		gtk_box_pack_start (GTK_BOX (priv->text_box), priv->progress, FALSE, TRUE, 0);
	}

	gtk_widget_show (priv->progress);
	if (priv->id) {
		g_source_remove (priv->id);
		priv->id = 0;
	}

	gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (priv->progress), progress);
}

void
burner_disc_message_set_primary (BurnerDiscMessage *self,
				  const gchar *message)
{
	BurnerDiscMessagePrivate *priv;
	gchar *markup;

	priv = BURNER_DISC_MESSAGE_PRIVATE (self);

	markup = g_strdup_printf ("<b>%s</b>", message);
	gtk_label_set_markup (GTK_LABEL (priv->primary), markup);
	g_free (markup);
	gtk_widget_show (priv->primary);
}

void
burner_disc_message_set_secondary (BurnerDiscMessage *self,
				    const gchar *message)
{
	BurnerDiscMessagePrivate *priv;

	priv = BURNER_DISC_MESSAGE_PRIVATE (self);

	if (!message) {
		if (priv->secondary) {
			gtk_widget_destroy (priv->secondary);
			priv->secondary = NULL;
		}
		return;
	}

	if (!priv->secondary) {
		priv->secondary = gtk_label_new (NULL);
		gtk_label_set_line_wrap_mode (GTK_LABEL (priv->secondary), GTK_WRAP_WORD);
		gtk_label_set_line_wrap (GTK_LABEL (priv->secondary), TRUE);
		gtk_misc_set_alignment (GTK_MISC (priv->secondary), 0.0, 0.5);
		gtk_box_pack_start (GTK_BOX (priv->text_box), priv->secondary, FALSE, TRUE, 0);
	}

	gtk_label_set_markup (GTK_LABEL (priv->secondary), message);
	gtk_widget_show (priv->secondary);
}

static void
burner_disc_message_init (BurnerDiscMessage *object)
{
	BurnerDiscMessagePrivate *priv;
	GtkWidget *main_box;

	priv = BURNER_DISC_MESSAGE_PRIVATE (object);

	main_box = gtk_info_bar_get_content_area (GTK_INFO_BAR (object));

	priv->text_box = gtk_box_new (GTK_ORIENTATION_VERTICAL, 6);
	gtk_widget_show (priv->text_box);
	gtk_box_pack_start (GTK_BOX (main_box), priv->text_box, FALSE, FALSE, 0);

	priv->primary = gtk_label_new (NULL);
	gtk_widget_show (priv->primary);
	gtk_label_set_line_wrap_mode (GTK_LABEL (priv->primary), GTK_WRAP_WORD);
	gtk_label_set_line_wrap (GTK_LABEL (priv->primary), TRUE);
	gtk_misc_set_alignment (GTK_MISC (priv->primary), 0.0, 0.5);
	gtk_box_pack_start (GTK_BOX (priv->text_box), priv->primary, FALSE, FALSE, 0);
}

static void
burner_disc_message_finalize (GObject *object)
{
	BurnerDiscMessagePrivate *priv;

	priv = BURNER_DISC_MESSAGE_PRIVATE (object);	
	if (priv->id) {
		g_source_remove (priv->id);
		priv->id = 0;
	}

	if (priv->timeout) {
		g_source_remove (priv->timeout);
		priv->timeout = 0;
	}

	G_OBJECT_CLASS (burner_disc_message_parent_class)->finalize (object);
}

static void
burner_disc_message_class_init (BurnerDiscMessageClass *klass)
{
	GObjectClass* object_class = G_OBJECT_CLASS (klass);

	g_type_class_add_private (klass, sizeof (BurnerDiscMessagePrivate));

	object_class->finalize = burner_disc_message_finalize;
}

GtkWidget *
burner_disc_message_new (void)
{
	return g_object_new (BURNER_TYPE_DISC_MESSAGE, NULL);
}
