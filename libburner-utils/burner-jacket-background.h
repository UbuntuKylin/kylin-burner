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

#ifndef _BURNER_JACKET_BACKGROUND_H_
#define _BURNER_JACKET_BACKGROUND_H_

#include <glib-object.h>

#include <gtk/gtk.h>

G_BEGIN_DECLS

typedef enum {
	BURNER_JACKET_IMAGE_NONE	= 0,
	BURNER_JACKET_IMAGE_CENTER	= 1,
	BURNER_JACKET_IMAGE_TILE,
	BURNER_JACKET_IMAGE_STRETCH
} BurnerJacketImageStyle;

typedef enum {
	BURNER_JACKET_COLOR_NONE	= 0,
	BURNER_JACKET_COLOR_SOLID	= 1,
	BURNER_JACKET_COLOR_HGRADIENT,
	BURNER_JACKET_COLOR_VGRADIENT
} BurnerJacketColorStyle;

#define BURNER_TYPE_JACKET_BACKGROUND             (burner_jacket_background_get_type ())
#define BURNER_JACKET_BACKGROUND(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), BURNER_TYPE_JACKET_BACKGROUND, BurnerJacketBackground))
#define BURNER_JACKET_BACKGROUND_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), BURNER_TYPE_JACKET_BACKGROUND, BurnerJacketBackgroundClass))
#define BURNER_IS_JACKET_BACKGROUND(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BURNER_TYPE_JACKET_BACKGROUND))
#define BURNER_IS_JACKET_BACKGROUND_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), BURNER_TYPE_JACKET_BACKGROUND))
#define BURNER_JACKET_BACKGROUND_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), BURNER_TYPE_JACKET_BACKGROUND, BurnerJacketBackgroundClass))

typedef struct _BurnerJacketBackgroundClass BurnerJacketBackgroundClass;
typedef struct _BurnerJacketBackground BurnerJacketBackground;

struct _BurnerJacketBackgroundClass
{
	GtkDialogClass parent_class;
};

struct _BurnerJacketBackground
{
	GtkDialog parent_instance;
};

GType burner_jacket_background_get_type (void) G_GNUC_CONST;

GtkWidget *
burner_jacket_background_new (void);

BurnerJacketColorStyle
burner_jacket_background_get_color_style (BurnerJacketBackground *back);

BurnerJacketImageStyle
burner_jacket_background_get_image_style (BurnerJacketBackground *back);

gchar *
burner_jacket_background_get_image_path (BurnerJacketBackground *back);

void
burner_jacket_background_get_color (BurnerJacketBackground *back,
				     GdkColor *color,
				     GdkColor *color2);

void
burner_jacket_background_set_color_style (BurnerJacketBackground *back,
					   BurnerJacketColorStyle style);

void
burner_jacket_background_set_image_style (BurnerJacketBackground *back,
					   BurnerJacketImageStyle style);

void
burner_jacket_background_set_image_path (BurnerJacketBackground *back,
					  const gchar *path);

void
burner_jacket_background_set_color (BurnerJacketBackground *back,
				     GdkColor *color,
				     GdkColor *color2);
G_END_DECLS

#endif /* _BURNER_JACKET_BACKGROUND_H_ */
