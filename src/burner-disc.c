/***************************************************************************
 *            disc.c
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

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <glib.h>
#include <glib-object.h>
#include <glib/gi18n-lib.h>

#include <gtk/gtk.h>

#include "burner-disc.h"
#include "burner-session.h"
 
static void burner_disc_base_init (gpointer g_class);

typedef enum {
	SELECTION_CHANGED_SIGNAL,
	LAST_SIGNAL
} BurnerDiscSignalType;

static guint burner_disc_signals [LAST_SIGNAL] = { 0 };

GType
burner_disc_get_type()
{
	static GType type = 0;

	if(type == 0) {
		static const GTypeInfo our_info = {
			sizeof (BurnerDiscIface),
			burner_disc_base_init,
			NULL,
			NULL,
			NULL,
			NULL,
			0,
			0,
			NULL
		};

		type = g_type_register_static (G_TYPE_INTERFACE, 
					       "BurnerDisc",
					       &our_info,
					       0);

		g_type_interface_add_prerequisite (type, G_TYPE_OBJECT);
	}

	return type;
}

static void
burner_disc_base_init (gpointer g_class)
{
	static gboolean initialized = FALSE;

	if (initialized)
		return;

	burner_disc_signals [SELECTION_CHANGED_SIGNAL] =
	    g_signal_new ("selection_changed",
			  BURNER_TYPE_DISC,
			  G_SIGNAL_RUN_LAST|G_SIGNAL_ACTION|G_SIGNAL_NO_RECURSE,
			  G_STRUCT_OFFSET (BurnerDiscIface, selection_changed),
			  NULL,
			  NULL,
			  g_cclosure_marshal_VOID__VOID,
			  G_TYPE_NONE,
			  0);
	initialized = TRUE;
}

BurnerDiscResult
burner_disc_add_uri (BurnerDisc *disc,
		      const gchar *uri)
{
	BurnerDiscIface *iface;

	g_return_val_if_fail (BURNER_IS_DISC (disc), BURNER_DISC_ERROR_UNKNOWN);
	g_return_val_if_fail (uri != NULL, BURNER_DISC_ERROR_UNKNOWN);
	
	iface = BURNER_DISC_GET_IFACE (disc);
	if (iface->add_uri)
		return (* iface->add_uri) (disc, uri);

	return BURNER_DISC_ERROR_UNKNOWN;
}

void
burner_disc_delete_selected (BurnerDisc *disc)
{
	BurnerDiscIface *iface;

	g_return_if_fail (BURNER_IS_DISC (disc));
	
	iface = BURNER_DISC_GET_IFACE (disc);
	if (iface->delete_selected)
		(* iface->delete_selected) (disc);
}

gboolean
burner_disc_clear (BurnerDisc *disc)
{
	BurnerDiscIface *iface;

	g_return_val_if_fail (BURNER_IS_DISC (disc), FALSE);
	
	iface = BURNER_DISC_GET_IFACE (disc);
	if (!iface->clear)
		return FALSE;

	(* iface->clear) (disc);
	return TRUE;
}

BurnerDiscResult
burner_disc_set_session_contents (BurnerDisc *disc,
				   BurnerBurnSession *session)
{
	BurnerDiscIface *iface;

	g_return_val_if_fail (BURNER_IS_DISC (disc), BURNER_DISC_ERROR_UNKNOWN);
	
	iface = BURNER_DISC_GET_IFACE (disc);
	if (iface->set_session_contents)
		return (* iface->set_session_contents) (disc, session);

	return BURNER_DISC_ERROR_UNKNOWN;
}

gboolean
burner_disc_get_selected_uri (BurnerDisc *disc, gchar **uri)
{
	BurnerDiscIface *iface;

	g_return_val_if_fail (BURNER_IS_DISC (disc), FALSE);
	iface = BURNER_DISC_GET_IFACE (disc);
	if (iface->get_selected_uri)
		return (* iface->get_selected_uri) (disc, uri);

	return FALSE;
}

gboolean
burner_disc_get_boundaries (BurnerDisc *disc,
			     gint64 *start,
			     gint64 *end)
{
	BurnerDiscIface *iface;

	g_return_val_if_fail (BURNER_IS_DISC (disc), FALSE);
	iface = BURNER_DISC_GET_IFACE (disc);
	if (iface->get_boundaries)
		return (* iface->get_boundaries) (disc,
						  start,
						  end);

	return FALSE;
}

guint
burner_disc_add_ui (BurnerDisc *disc, GtkUIManager *manager, GtkWidget *message)
{
	BurnerDiscIface *iface;

	if (!disc)
		return 0;

	g_return_val_if_fail (BURNER_IS_DISC (disc), 0);
	g_return_val_if_fail (GTK_IS_UI_MANAGER (manager), 0);

	iface = BURNER_DISC_GET_IFACE (disc);
	if (iface->add_ui)
		return (* iface->add_ui) (disc, manager, message);

	return 0;
}

gboolean
burner_disc_is_empty (BurnerDisc *disc)
{
	BurnerDiscIface *iface;

	if (!disc)
		return 0;

	g_return_val_if_fail (BURNER_IS_DISC (disc), 0);

	iface = BURNER_DISC_GET_IFACE (disc);
	if (iface->is_empty)
		return (* iface->is_empty) (disc);

	return FALSE;

}

void
burner_disc_selection_changed (BurnerDisc *disc)
{
	g_return_if_fail (BURNER_IS_DISC (disc));
	g_signal_emit (disc,
		       burner_disc_signals [SELECTION_CHANGED_SIGNAL],
		       0);
}
