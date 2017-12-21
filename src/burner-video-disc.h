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

#ifndef _BURNER_VIDEO_DISC_H_
#define _BURNER_VIDEO_DISC_H_

#include <glib-object.h>

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define BURNER_TYPE_VIDEO_DISC             (burner_video_disc_get_type ())
#define BURNER_VIDEO_DISC(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), BURNER_TYPE_VIDEO_DISC, BurnerVideoDisc))
#define BURNER_VIDEO_DISC_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), BURNER_TYPE_VIDEO_DISC, BurnerVideoDiscClass))
#define BURNER_IS_VIDEO_DISC(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BURNER_TYPE_VIDEO_DISC))
#define BURNER_IS_VIDEO_DISC_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), BURNER_TYPE_VIDEO_DISC))
#define BURNER_VIDEO_DISC_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), BURNER_TYPE_VIDEO_DISC, BurnerVideoDiscClass))

typedef struct _BurnerVideoDiscClass BurnerVideoDiscClass;
typedef struct _BurnerVideoDisc BurnerVideoDisc;

struct _BurnerVideoDiscClass
{
	GtkBoxClass parent_class;
};

struct _BurnerVideoDisc
{
	GtkBox parent_instance;
};

GType burner_video_disc_get_type (void) G_GNUC_CONST;

GtkWidget *
burner_video_disc_new (void);

G_END_DECLS

#endif /* _BURNER_VIDEO_DISC_H_ */
