/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/*
 * Burner
 * Copyright (C) Philippe Rouquier 2005-2009 <bonfire-app@wanadoo.fr>
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

#ifndef _BURNER_CUSTOMIZE_TITLE_H_
#define _BURNER_CUSTOMIZE_TITLE_H_

#include <glib-object.h>
#include <gtk/gtk.h>
#include <stdio.h>

G_BEGIN_DECLS


void
burner_message_title (GtkWidget *dialog);

void
burner_dialog_title (GtkWidget *dialog, gchar *title_label);

void
burner_dialog_button_image (GtkWidget *properties);


G_END_DECLS

#endif /* _BURNER_CUSTOMIZE_TITLE_H_ */


