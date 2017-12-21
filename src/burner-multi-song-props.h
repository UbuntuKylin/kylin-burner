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

#ifndef _BURNER_MULTI_SONG_PROPS_H_
#define _BURNER_MULTI_SONG_PROPS_H_

#include <glib.h>
#include <glib-object.h>

#include <gtk/gtk.h>

#include "burner-rename.h"

G_BEGIN_DECLS

#define BURNER_TYPE_MULTI_SONG_PROPS             (burner_multi_song_props_get_type ())
#define BURNER_MULTI_SONG_PROPS(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), BURNER_TYPE_MULTI_SONG_PROPS, BurnerMultiSongProps))
#define BURNER_MULTI_SONG_PROPS_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), BURNER_TYPE_MULTI_SONG_PROPS, BurnerMultiSongPropsClass))
#define BURNER_IS_MULTI_SONG_PROPS(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BURNER_TYPE_MULTI_SONG_PROPS))
#define BURNER_IS_MULTI_SONG_PROPS_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), BURNER_TYPE_MULTI_SONG_PROPS))
#define BURNER_MULTI_SONG_PROPS_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), BURNER_TYPE_MULTI_SONG_PROPS, BurnerMultiSongPropsClass))

typedef struct _BurnerMultiSongPropsClass BurnerMultiSongPropsClass;
typedef struct _BurnerMultiSongProps BurnerMultiSongProps;

struct _BurnerMultiSongPropsClass
{
	GtkDialogClass parent_class;
};

struct _BurnerMultiSongProps
{
	GtkDialog parent_instance;
};

GType burner_multi_song_props_get_type (void) G_GNUC_CONST;

GtkWidget *
burner_multi_song_props_new (void);

void
burner_multi_song_props_set_show_gap (BurnerMultiSongProps *props,
				       gboolean show);

void
burner_multi_song_props_set_rename_callback (BurnerMultiSongProps *props,
					      GtkTreeSelection *selection,
					      gint column_num,
					      BurnerRenameCallback callback);
void
burner_multi_song_props_get_properties (BurnerMultiSongProps *props,
					 gchar **artist,
					 gchar **composer,
					 gchar **isrc,
					 gint64 *gap);

G_END_DECLS

#endif /* _BURNER_MULTI_SONG_PROPS_H_ */
