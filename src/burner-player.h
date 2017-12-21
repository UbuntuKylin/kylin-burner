/***************************************************************************
*            player.h
*
*  lun mai 30 08:15:01 2005
*  Copyright  2005  Philippe Rouquier
*  burner-app@wanadoo.fr
****************************************************************************/

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

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#ifndef PLAYER_H
#define PLAYER_H

#include <glib.h>
#include <glib-object.h>
#include <gtk/gtk.h>

G_BEGIN_DECLS

#define BURNER_TYPE_PLAYER         (burner_player_get_type ())
#define BURNER_PLAYER(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), BURNER_TYPE_PLAYER, BurnerPlayer))
#define BURNER_PLAYER_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k), BURNER_TYPE_PLAYER, BurnerPlayerClass))
#define BURNER_IS_PLAYER(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), BURNER_TYPE_PLAYER))
#define BURNER_IS_PLAYER_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), BURNER_TYPE_PLAYER))
#define BURNER_PLAYER_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), BURNER_TYPE_PLAYER, BurnerPlayerClass))

typedef struct BurnerPlayerPrivate BurnerPlayerPrivate;

typedef struct {
	GtkAlignment parent;
	BurnerPlayerPrivate *priv;
} BurnerPlayer;

typedef struct {
	GtkAlignmentClass parent_class;

	void		(*error)	(BurnerPlayer *player);
	void		(*ready)	(BurnerPlayer *player);
} BurnerPlayerClass;

GType burner_player_get_type (void);
GtkWidget *burner_player_new (void);

void
burner_player_set_uri (BurnerPlayer *player,
			const gchar *uri);
void
burner_player_set_boundaries (BurnerPlayer *player, 
			       gint64 start,
			       gint64 end);

G_END_DECLS

#endif
