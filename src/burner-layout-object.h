/***************************************************************************
 *            burner-layout-object.h
 *
 *  dim oct 15 17:15:58 2006
 *  Copyright  2006  Philippe Rouquier
 *  bonfire-app@wanadoo.fr
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

#ifndef BURNER_LAYOUT_OBJECT_H
#define BURNER_LAYOUT_OBJECT_H

#include <glib.h>
#include <glib-object.h>

#include "burner-layout.h"

G_BEGIN_DECLS

#define BURNER_TYPE_LAYOUT_OBJECT         (burner_layout_object_get_type ())
#define BURNER_LAYOUT_OBJECT(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), BURNER_TYPE_LAYOUT_OBJECT, BurnerLayoutObject))
#define BURNER_IS_LAYOUT_OBJECT(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), BURNER_TYPE_LAYOUT_OBJECT))
#define BURNER_LAYOUT_OBJECT_GET_IFACE(o) (G_TYPE_INSTANCE_GET_INTERFACE ((o), BURNER_TYPE_LAYOUT_OBJECT, BurnerLayoutObjectIFace))

typedef struct _BurnerLayoutObject BurnerLayoutObject;
typedef struct _BurnerLayoutIFace BurnerLayoutObjectIFace;

struct _BurnerLayoutIFace {
	GTypeInterface g_iface;

	void	(*get_proportion)	(BurnerLayoutObject *self,
					 gint *header,
					 gint *center,
					 gint *footer);
	void	(*set_context)		(BurnerLayoutObject *self,
					 BurnerLayoutType type);
};

GType burner_layout_object_get_type (void);

void burner_layout_object_get_proportion (BurnerLayoutObject *self,
					   gint *header,
					   gint *center,
					   gint *footer);

void burner_layout_object_set_context (BurnerLayoutObject *self,
					BurnerLayoutType type);

G_END_DECLS

#endif /* BURNER_LAYOUT_OBJECT_H */
