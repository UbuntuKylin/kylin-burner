/***************************************************************************
 *            player-bacon.h
 *
 *  ven déc 30 11:29:33 2005
 *  Copyright  2005  Rouquier Philippe
 *  burner-app@wanadoo.fr
 ***************************************************************************/

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

#ifndef PLAYER_BACON_H
#define PLAYER_BACON_H

#include <glib.h>
#include <glib-object.h>

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define BURNER_TYPE_PLAYER_BACON         (burner_player_bacon_get_type ())
#define BURNER_PLAYER_BACON(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), BURNER_TYPE_PLAYER_BACON, BurnerPlayerBacon))
#define BURNER_PLAYER_BACON_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k), BURNER_TYPE_PLAYER_BACON, BurnerPlayerBaconClass))
#define BURNER_IS_PLAYER_BACON(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), BURNER_TYPE_PLAYER_BACON))
#define BURNER_IS_PLAYER_BACON_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), BURNER_TYPE_PLAYER_BACON))
#define BURNER_PLAYER_BACON_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), BURNER_TYPE_PLAYER_BACON, BurnerPlayerBaconClass))

#define	PLAYER_BACON_WIDTH 120
#define	PLAYER_BACON_HEIGHT 90

typedef struct BurnerPlayerBaconPrivate BurnerPlayerBaconPrivate;

typedef enum {
	BACON_STATE_ERROR,
	BACON_STATE_READY,
	BACON_STATE_PAUSED,
	BACON_STATE_PLAYING
} BurnerPlayerBaconState;

typedef struct {
	GtkWidget parent;
	BurnerPlayerBaconPrivate *priv;
} BurnerPlayerBacon;

typedef struct {
	GtkWidgetClass parent_class;

	void	(*state_changed)	(BurnerPlayerBacon *bacon,
					 BurnerPlayerBaconState state);

	void	(*eof)			(BurnerPlayerBacon *bacon);

} BurnerPlayerBaconClass;

GType burner_player_bacon_get_type (void);
GtkWidget *burner_player_bacon_new (void);

void burner_player_bacon_set_uri (BurnerPlayerBacon *bacon, const gchar *uri);
void burner_player_bacon_set_volume (BurnerPlayerBacon *bacon, gdouble volume);
gboolean burner_player_bacon_set_boundaries (BurnerPlayerBacon *bacon, gint64 start, gint64 end);
gboolean burner_player_bacon_play (BurnerPlayerBacon *bacon);
gboolean burner_player_bacon_stop (BurnerPlayerBacon *bacon);
gboolean burner_player_bacon_set_pos (BurnerPlayerBacon *bacon, gdouble pos);
gboolean burner_player_bacon_get_pos (BurnerPlayerBacon *bacon, gint64 *pos);
gdouble  burner_player_bacon_get_volume (BurnerPlayerBacon *bacon);
gboolean burner_player_bacon_forward (BurnerPlayerBacon *bacon, gint64 value);
gboolean burner_player_bacon_backward (BurnerPlayerBacon *bacon, gint64 value);
G_END_DECLS

#endif /* PLAYER_BACON_H */
