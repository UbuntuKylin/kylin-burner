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

#ifndef _BURNER_DISC_MESSAGE_H_
#define _BURNER_DISC_MESSAGE_H_

#include <glib-object.h>

#include <gtk/gtk.h>
G_BEGIN_DECLS

#define BURNER_TYPE_DISC_MESSAGE             (burner_disc_message_get_type ())
#define BURNER_DISC_MESSAGE(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), BURNER_TYPE_DISC_MESSAGE, BurnerDiscMessage))
#define BURNER_DISC_MESSAGE_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), BURNER_TYPE_DISC_MESSAGE, BurnerDiscMessageClass))
#define BURNER_IS_DISC_MESSAGE(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BURNER_TYPE_DISC_MESSAGE))
#define BURNER_IS_DISC_MESSAGE_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), BURNER_TYPE_DISC_MESSAGE))
#define BURNER_DISC_MESSAGE_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), BURNER_TYPE_DISC_MESSAGE, BurnerDiscMessageClass))

typedef struct _BurnerDiscMessageClass BurnerDiscMessageClass;
typedef struct _BurnerDiscMessage BurnerDiscMessage;

struct _BurnerDiscMessageClass
{
	GtkInfoBarClass parent_class;
};

struct _BurnerDiscMessage
{
	GtkInfoBar parent_instance;
};

GType burner_disc_message_get_type (void) G_GNUC_CONST;

GtkWidget *
burner_disc_message_new (void);

void
burner_disc_message_destroy (BurnerDiscMessage *message);

void
burner_disc_message_set_primary (BurnerDiscMessage *message,
				  const gchar *text);
void
burner_disc_message_set_secondary (BurnerDiscMessage *message,
				    const gchar *text);

void
burner_disc_message_set_progress_active (BurnerDiscMessage *message,
					  gboolean active);
void
burner_disc_message_set_progress (BurnerDiscMessage *self,
				   gdouble progress);

void
burner_disc_message_remove_buttons (BurnerDiscMessage *message);

void
burner_disc_message_add_errors (BurnerDiscMessage *message,
				 GSList *errors);
void
burner_disc_message_remove_errors (BurnerDiscMessage *message);

void
burner_disc_message_set_timeout (BurnerDiscMessage *message,
				  guint mseconds);

void
burner_disc_message_set_context (BurnerDiscMessage *message,
				  guint context_id);

guint
burner_disc_message_get_context (BurnerDiscMessage *message);

G_END_DECLS

#endif /* _BURNER_DISC_MESSAGE_H_ */
