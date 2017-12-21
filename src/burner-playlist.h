/*
 *  Burner is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  Burner is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Library General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to:
 * 	The Free Software Foundation, Inc.,
 * 	51 Franklin Street, Fifth Floor
 * 	Boston, MA  02110-1301, USA.
 */
/***************************************************************************
*            play-list.h
*
*  mer mai 25 22:22:53 2005
*  Copyright  2005  Philippe Rouquier
*  burner-app@wanadoo.fr
****************************************************************************/

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#ifdef BUILD_PLAYLIST

#ifndef PLAY_LIST_H
#define PLAY_LIST_H

#include <glib.h>
#include <glib-object.h>
#include <gtk/gtk.h>

G_BEGIN_DECLS

#define BURNER_TYPE_PLAYLIST         (burner_playlist_get_type ())
#define BURNER_PLAYLIST(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), BURNER_TYPE_PLAYLIST, BurnerPlaylist))
#define BURNER_PLAYLIST_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k), BURNER_TYPE_PLAYLIST, BurnerPlaylistClass))
#define BURNER_IS_PLAY_LIST(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), BURNER_TYPE_PLAYLIST))
#define BURNER_IS_PLAY_LIST_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), BURNER_TYPE_PLAYLIST))
#define BURNER_PLAYLIST_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), BURNER_TYPE_PLAYLIST, BurnerPlaylistClass))
typedef struct BurnerPlaylistPrivate BurnerPlaylistPrivate;

typedef struct {
	GtkBox parent;
	BurnerPlaylistPrivate *priv;
} BurnerPlaylist;

typedef struct {
	GtkBoxClass parent_class;
} BurnerPlaylistClass;

GType burner_playlist_get_type (void);
GtkWidget *burner_playlist_new (void);

G_END_DECLS

#endif				/* PLAY_LIST_H */

#endif
