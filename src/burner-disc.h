/***************************************************************************
 *            burner-disc.h
 *
 *  dim nov 27 14:58:13 2005
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

#ifndef DISC_H
#define DISC_H

#include <glib.h>
#include <glib-object.h>

#include <gtk/gtk.h>

#include "burner-project-parse.h"
#include "burner-session.h"

G_BEGIN_DECLS

#define BURNER_TYPE_DISC         (burner_disc_get_type ())
#define BURNER_DISC(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), BURNER_TYPE_DISC, BurnerDisc))
#define BURNER_IS_DISC(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), BURNER_TYPE_DISC))
#define BURNER_DISC_GET_IFACE(o) (G_TYPE_INSTANCE_GET_INTERFACE ((o), BURNER_TYPE_DISC, BurnerDiscIface))

#define BURNER_DISC_ACTION "DiscAction"


typedef enum {
	BURNER_DISC_OK = 0,
	BURNER_DISC_NOT_IN_TREE,
	BURNER_DISC_NOT_READY,
	BURNER_DISC_LOADING,
	BURNER_DISC_BROKEN_SYMLINK,
	BURNER_DISC_CANCELLED,
	BURNER_DISC_ERROR_SIZE,
	BURNER_DISC_ERROR_EMPTY_SELECTION,
	BURNER_DISC_ERROR_FILE_NOT_FOUND,
	BURNER_DISC_ERROR_UNREADABLE,
	BURNER_DISC_ERROR_ALREADY_IN_TREE,
	BURNER_DISC_ERROR_JOLIET,
	BURNER_DISC_ERROR_FILE_TYPE,
	BURNER_DISC_ERROR_THREAD,
	BURNER_DISC_ERROR_UNKNOWN
} BurnerDiscResult;

typedef struct _BurnerDisc        BurnerDisc;
typedef struct _BurnerDiscIface   BurnerDiscIface;

struct _BurnerDiscIface {
	GTypeInterface g_iface;

	/* signals */
	void	(*selection_changed)			(BurnerDisc *disc);

	/* virtual functions */
	BurnerDiscResult	(*set_session_contents)	(BurnerDisc *disc,
							 BurnerBurnSession *session);

	BurnerDiscResult	(*add_uri)		(BurnerDisc *disc,
							 const gchar *uri);

	gboolean		(*is_empty)		(BurnerDisc *disc);
	gboolean		(*get_selected_uri)	(BurnerDisc *disc,
							 gchar **uri);
	gboolean		(*get_boundaries)	(BurnerDisc *disc,
							 gint64 *start,
							 gint64 *end);

	void			(*delete_selected)	(BurnerDisc *disc);
	void			(*clear)		(BurnerDisc *disc);

	guint			(*add_ui)		(BurnerDisc *disc,
							 GtkUIManager *manager,
							 GtkWidget *message);
};

GType burner_disc_get_type (void);

guint
burner_disc_add_ui (BurnerDisc *disc,
		     GtkUIManager *manager,
		     GtkWidget *message);

BurnerDiscResult
burner_disc_add_uri (BurnerDisc *disc, const gchar *escaped_uri);

gboolean
burner_disc_get_selected_uri (BurnerDisc *disc, gchar **uri);

gboolean
burner_disc_get_boundaries (BurnerDisc *disc,
			     gint64 *start,
			     gint64 *end);

void
burner_disc_delete_selected (BurnerDisc *disc);

gboolean
burner_disc_clear (BurnerDisc *disc);

BurnerDiscResult
burner_disc_get_status (BurnerDisc *disc,
			 gint *remaining,
			 gchar **current_task);

BurnerDiscResult
burner_disc_set_session_contents (BurnerDisc *disc,
				   BurnerBurnSession *session);

gboolean
burner_disc_is_empty (BurnerDisc *disc);

void
burner_disc_selection_changed (BurnerDisc *disc);

G_END_DECLS

#endif /* DISC_H */
