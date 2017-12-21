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

#ifndef _BURNER_TIME_BUTTON_H_
#define _BURNER_TIME_BUTTON_H_

#include <glib-object.h>

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define BURNER_TYPE_TIME_BUTTON             (burner_time_button_get_type ())
#define BURNER_TIME_BUTTON(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), BURNER_TYPE_TIME_BUTTON, BurnerTimeButton))
#define BURNER_TIME_BUTTON_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), BURNER_TYPE_TIME_BUTTON, BurnerTimeButtonClass))
#define BURNER_IS_TIME_BUTTON(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BURNER_TYPE_TIME_BUTTON))
#define BURNER_IS_TIME_BUTTON_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), BURNER_TYPE_TIME_BUTTON))
#define BURNER_TIME_BUTTON_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), BURNER_TYPE_TIME_BUTTON, BurnerTimeButtonClass))

typedef struct _BurnerTimeButtonClass BurnerTimeButtonClass;
typedef struct _BurnerTimeButton BurnerTimeButton;

struct _BurnerTimeButtonClass
{
	GtkBoxClass parent_class;

	void		(*value_changed)	(BurnerTimeButton *self);
};

struct _BurnerTimeButton
{
	GtkBox parent_instance;
};

GType burner_time_button_get_type (void) G_GNUC_CONST;

GtkWidget *
burner_time_button_new (void);

gint64
burner_time_button_get_value (BurnerTimeButton *time);

void
burner_time_button_set_value (BurnerTimeButton *time,
			       gint64 value);
void
burner_time_button_set_max (BurnerTimeButton *time,
			     gint64 max);

void
burner_time_button_set_show_frames (BurnerTimeButton *time,
				     gboolean show);

G_END_DECLS

#endif /* _BURNER_TIME_BUTTON_H_ */
