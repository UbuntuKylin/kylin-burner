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

#ifndef _BURNER_JACKET_VIEW_H_
#define _BURNER_JACKET_VIEW_H_

#include <glib-object.h>

#include <gtk/gtk.h>

#include "burner-jacket-background.h"

G_BEGIN_DECLS

typedef enum {
	BURNER_JACKET_FRONT		= 0,
	BURNER_JACKET_BACK		= 1,
} BurnerJacketSide;

#define COVER_HEIGHT_FRONT_MM		120.0
#define COVER_WIDTH_FRONT_MM		120.0
#define COVER_WIDTH_FRONT_INCH		4.724
#define COVER_HEIGHT_FRONT_INCH		4.724

#define COVER_HEIGHT_BACK_MM		118.0
#define COVER_WIDTH_BACK_MM		150.0
#define COVER_HEIGHT_BACK_INCH		4.646
#define COVER_WIDTH_BACK_INCH		5.906

#define COVER_HEIGHT_SIDE_MM		COVER_HEIGHT_BACK_MM
#define COVER_WIDTH_SIDE_MM		6.0
#define COVER_HEIGHT_SIDE_INCH		COVER_HEIGHT_BACK_INCH
#define COVER_WIDTH_SIDE_INCH		0.236

#define COVER_TEXT_MARGIN		/*1.*/0.03 //0.079

#define BURNER_TYPE_JACKET_VIEW             (burner_jacket_view_get_type ())
#define BURNER_JACKET_VIEW(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), BURNER_TYPE_JACKET_VIEW, BurnerJacketView))
#define BURNER_JACKET_VIEW_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), BURNER_TYPE_JACKET_VIEW, BurnerJacketViewClass))
#define BURNER_IS_JACKET_VIEW(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BURNER_TYPE_JACKET_VIEW))
#define BURNER_IS_JACKET_VIEW_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), BURNER_TYPE_JACKET_VIEW))
#define BURNER_JACKET_VIEW_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), BURNER_TYPE_JACKET_VIEW, BurnerJacketViewClass))

typedef struct _BurnerJacketViewClass BurnerJacketViewClass;
typedef struct _BurnerJacketView BurnerJacketView;

struct _BurnerJacketViewClass
{
	GtkContainerClass parent_class;
};

struct _BurnerJacketView
{
	GtkContainer parent_instance;
};

GType burner_jacket_view_get_type (void) G_GNUC_CONST;

GtkWidget *
burner_jacket_view_new (void);

void
burner_jacket_view_add_default_tag (BurnerJacketView *self,
				     GtkTextTag *tag);

void
burner_jacket_view_set_side (BurnerJacketView *view,
			      BurnerJacketSide side);

void
burner_jacket_view_set_color (BurnerJacketView *view,
			       BurnerJacketColorStyle style,
			       GdkColor *color,
			       GdkColor *color2);

const gchar *
burner_jacket_view_get_image (BurnerJacketView *self);

const gchar *
burner_jacket_view_set_image (BurnerJacketView *view,
			       BurnerJacketImageStyle style,
			       const gchar *path);

void
burner_jacket_view_configure_background (BurnerJacketView *view);

guint
burner_jacket_view_print (BurnerJacketView *view,
			   GtkPrintContext *context,
			   gdouble x,
			   gdouble y);

GtkTextBuffer *
burner_jacket_view_get_active_buffer (BurnerJacketView *view);

GtkTextBuffer *
burner_jacket_view_get_body_buffer (BurnerJacketView *view);

GtkTextBuffer *
burner_jacket_view_get_side_buffer (BurnerJacketView *view);

GtkTextAttributes *
burner_jacket_view_get_attributes (BurnerJacketView *view,
				    GtkTextIter *iter);

G_END_DECLS

#endif /* _BURNER_JACKET_VIEW_H_ */
