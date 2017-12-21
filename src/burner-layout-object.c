/***************************************************************************
 *            burner-layout-object.c
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

#include "burner-layout-object.h"
 
static void burner_layout_object_base_init (gpointer g_class);

GType
burner_layout_object_get_type()
{
	static GType type = 0;

	if(type == 0) {
		static const GTypeInfo our_info = {
			sizeof (BurnerLayoutObjectIFace),
			burner_layout_object_base_init,
			NULL,
			NULL,
			NULL,
			NULL,
			0,
			0,
			NULL,
		};

		type = g_type_register_static (G_TYPE_INTERFACE,
					       "BurnerLayoutObject",
					       &our_info,
					       0);

		g_type_interface_add_prerequisite (type, G_TYPE_OBJECT);
	}

	return type;
}

static void
burner_layout_object_base_init (gpointer g_class)
{
	static gboolean initialized = FALSE;

	if (initialized)
		return;

	initialized = TRUE;
}

void
burner_layout_object_get_proportion (BurnerLayoutObject *self,
				      gint *header,
				      gint *center,
				      gint *footer)
{
	BurnerLayoutObjectIFace *iface;

	g_return_if_fail (BURNER_IS_LAYOUT_OBJECT (self));
	g_return_if_fail (header != NULL && center != NULL && footer != NULL);
	
	iface = BURNER_LAYOUT_OBJECT_GET_IFACE (self);
	if (iface->get_proportion)
		(* iface->get_proportion) (self,
					   header,
					   center,
					   footer);
}

void
burner_layout_object_set_context (BurnerLayoutObject *self,
				   BurnerLayoutType type)
{
	BurnerLayoutObjectIFace *iface;

	g_return_if_fail (BURNER_IS_LAYOUT_OBJECT (self));
	
	iface = BURNER_LAYOUT_OBJECT_GET_IFACE (self);
	if (iface->set_context)
		(* iface->set_context) (self, type);
}
