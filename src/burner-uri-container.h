/***************************************************************************
 *            burner-uri-container.h
 *
 *  lun mai 22 08:54:18 2006
 *  Copyright  2006  Rouquier Philippe
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

#ifndef BURNER_URI_CONTAINER_H
#define BURNER_URI_CONTAINER_H

#include <glib.h>
#include <glib-object.h>

G_BEGIN_DECLS

#define BURNER_TYPE_URI_CONTAINER         (burner_uri_container_get_type ())
#define BURNER_URI_CONTAINER(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), BURNER_TYPE_URI_CONTAINER, BurnerURIContainer))
#define BURNER_IS_URI_CONTAINER(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), BURNER_TYPE_URI_CONTAINER))
#define BURNER_URI_CONTAINER_GET_IFACE(o) (G_TYPE_INSTANCE_GET_INTERFACE ((o), BURNER_TYPE_URI_CONTAINER, BurnerURIContainerIFace))


typedef struct _BurnerURIContainer BurnerURIContainer;

typedef struct {
	GTypeInterface g_iface;

	/* signals */
	void		(*uri_selected)		(BurnerURIContainer *container);
	void		(*uri_activated)	(BurnerURIContainer *container);

	/* virtual functions */
	gboolean	(*get_boundaries)	(BurnerURIContainer *container,
						 gint64 *start,
						 gint64 *end);
	gchar*		(*get_selected_uri)	(BurnerURIContainer *container);
	gchar**		(*get_selected_uris)	(BurnerURIContainer *container);

} BurnerURIContainerIFace;


GType burner_uri_container_get_type (void);

gboolean
burner_uri_container_get_boundaries (BurnerURIContainer *container,
				      gint64 *start,
				      gint64 *end);
gchar *
burner_uri_container_get_selected_uri (BurnerURIContainer *container);
gchar **
burner_uri_container_get_selected_uris (BurnerURIContainer *container);

void
burner_uri_container_uri_selected (BurnerURIContainer *container);
void
burner_uri_container_uri_activated (BurnerURIContainer *container);

G_END_DECLS

#endif /* BURNER_URI_CONTAINER_H */
