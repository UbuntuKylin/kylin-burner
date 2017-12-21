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

#ifndef _BURNER_JACKET_BUFFER_H_
#define _BURNER_JACKET_BUFFER_H_

#include <glib-object.h>

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define BURNER_TYPE_JACKET_BUFFER             (burner_jacket_buffer_get_type ())
#define BURNER_JACKET_BUFFER(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), BURNER_TYPE_JACKET_BUFFER, BurnerJacketBuffer))
#define BURNER_JACKET_BUFFER_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), BURNER_TYPE_JACKET_BUFFER, BurnerJacketBufferClass))
#define BURNER_IS_JACKET_BUFFER(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BURNER_TYPE_JACKET_BUFFER))
#define BURNER_IS_JACKET_BUFFER_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), BURNER_TYPE_JACKET_BUFFER))
#define BURNER_JACKET_BUFFER_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), BURNER_TYPE_JACKET_BUFFER, BurnerJacketBufferClass))

typedef struct _BurnerJacketBufferClass BurnerJacketBufferClass;
typedef struct _BurnerJacketBuffer BurnerJacketBuffer;

struct _BurnerJacketBufferClass
{
	GtkTextBufferClass parent_class;
};

struct _BurnerJacketBuffer
{
	GtkTextBuffer parent_instance;
};

GType burner_jacket_buffer_get_type (void) G_GNUC_CONST;

BurnerJacketBuffer *
burner_jacket_buffer_new (void);

void
burner_jacket_buffer_add_default_tag (BurnerJacketBuffer *self,
				       GtkTextTag *tag);

void
burner_jacket_buffer_get_attributes (BurnerJacketBuffer *self,
				      GtkTextAttributes *attributes);

void
burner_jacket_buffer_set_default_text (BurnerJacketBuffer *self,
					const gchar *default_text);

void
burner_jacket_buffer_show_default_text (BurnerJacketBuffer *self,
					 gboolean show);

gchar *
burner_jacket_buffer_get_text (BurnerJacketBuffer *self,
				GtkTextIter *start,
				GtkTextIter *end,
				gboolean invisible_chars,
				gboolean get_default_text);

G_END_DECLS

#endif /* _BURNER_JACKET_BUFFER_H_ */
