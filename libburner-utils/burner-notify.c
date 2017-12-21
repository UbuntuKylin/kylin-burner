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

#include <gtk/gtk.h>

#include "burner-notify.h"
#include "burner-disc-message.h"


GtkWidget *
burner_notify_get_message_by_context_id (GtkWidget *self,
					  guint context_id)
{
	GtkWidget *retval = NULL;
	GList *children;
	GList *iter;

	GDK_THREADS_ENTER ();

	children = gtk_container_get_children (GTK_CONTAINER (self));
	for (iter = children; iter; iter = iter->next) {
		GtkWidget *widget;

		widget = iter->data;
		if (burner_disc_message_get_context (BURNER_DISC_MESSAGE (widget)) == context_id) {
			retval = widget;
			break;
		}
	}
	g_list_free (children);

	GDK_THREADS_LEAVE ();

	return retval;
}

void
burner_notify_message_remove (GtkWidget *self,
			       guint context_id)
{
	GList *children;
	GList *iter;

	GDK_THREADS_ENTER ();

	children = gtk_container_get_children (GTK_CONTAINER (self));
	for (iter = children; iter; iter = iter->next) {
		GtkWidget *widget;

		widget = iter->data;
		if (burner_disc_message_get_context (BURNER_DISC_MESSAGE (widget)) == context_id) {
			burner_disc_message_destroy (BURNER_DISC_MESSAGE (widget));
		}
	}
	g_list_free (children);

	GDK_THREADS_LEAVE ();
}

GtkWidget *
burner_notify_message_add (GtkWidget *self,
			    const gchar *primary,
			    const gchar *secondary,
			    gint timeout,
			    guint context_id)
{
	GtkWidget *message;

	GDK_THREADS_ENTER ();

	burner_notify_message_remove (self, context_id);
	
	message = burner_disc_message_new ();

	burner_disc_message_set_primary (BURNER_DISC_MESSAGE (message), primary);
	burner_disc_message_set_secondary (BURNER_DISC_MESSAGE (message), secondary);
	burner_disc_message_set_context (BURNER_DISC_MESSAGE (message), context_id);

	if (timeout > 0)
		burner_disc_message_set_timeout (BURNER_DISC_MESSAGE (message), timeout);

	gtk_container_add (GTK_CONTAINER (self), message);
	gtk_widget_show (message);
		
	GDK_THREADS_LEAVE ();

	return message;
}

GtkWidget *
burner_notify_new (void)
{
	return gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
}
