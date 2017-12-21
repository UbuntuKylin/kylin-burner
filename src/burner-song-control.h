/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * burner
 * Copyright (C) Philippe Rouquier 2010 <bonfire-app@wanadoo.fr>
 * 
 * burner is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * burner is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef _BURNER_SONG_CONTROL_H_
#define _BURNER_SONG_CONTROL_H_

#include <glib-object.h>
#include <gtk/gtk.h>

G_BEGIN_DECLS

#define BURNER_TYPE_SONG_CONTROL             (burner_song_control_get_type ())
#define BURNER_SONG_CONTROL(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), BURNER_TYPE_SONG_CONTROL, BurnerSongControl))
#define BURNER_SONG_CONTROL_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), BURNER_TYPE_SONG_CONTROL, BurnerSongControlClass))
#define BURNER_IS_SONG_CONTROL(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BURNER_TYPE_SONG_CONTROL))
#define BURNER_IS_SONG_CONTROL_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), BURNER_TYPE_SONG_CONTROL))
#define BURNER_SONG_CONTROL_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), BURNER_TYPE_SONG_CONTROL, BurnerSongControlClass))

typedef struct _BurnerSongControlClass BurnerSongControlClass;
typedef struct _BurnerSongControl BurnerSongControl;

struct _BurnerSongControlClass
{
	GtkAlignmentClass parent_class;
};

struct _BurnerSongControl
{
	GtkAlignment parent_instance;
};

GType burner_song_control_get_type (void) G_GNUC_CONST;

GtkWidget *
burner_song_control_new (void);

void
burner_song_control_set_uri (BurnerSongControl *player,
                              const gchar *uri);

void
burner_song_control_set_info (BurnerSongControl *player,
                               const gchar *title,
                               const gchar *artist);

void
burner_song_control_set_boundaries (BurnerSongControl *player, 
                                     gsize start,
                                     gsize end);

gint64
burner_song_control_get_pos (BurnerSongControl *control);

gint64
burner_song_control_get_length (BurnerSongControl *control);

const gchar *
burner_song_control_get_uri (BurnerSongControl *control);

G_END_DECLS

#endif /* _BURNER_SONG_CONTROL_H_ */
