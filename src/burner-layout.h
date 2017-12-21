/***************************************************************************
 *            burner-layout.h
 *
 *  mer mai 24 15:14:42 2006
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

#ifndef BURNER_LAYOUT_H
#define BURNER_LAYOUT_H

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <glib.h>
#include <glib-object.h>

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define BURNER_TYPE_LAYOUT         (burner_layout_get_type ())
#define BURNER_LAYOUT(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), BURNER_TYPE_LAYOUT, BurnerLayout))
#define BURNER_LAYOUT_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k), BURNER_TYPE_LAYOUT, BurnerLayoutClass))
#define BURNER_IS_LAYOUT(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), BURNER_TYPE_LAYOUT))
#define BURNER_IS_LAYOUT_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), BURNER_TYPE_LAYOUT))
#define BURNER_LAYOUT_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), BURNER_TYPE_LAYOUT, BurnerLayoutClass))

typedef struct BurnerLayoutPrivate BurnerLayoutPrivate;

typedef enum {
	BURNER_LAYOUT_NONE		= 0,
	BURNER_LAYOUT_AUDIO		= 1,
	BURNER_LAYOUT_DATA		= 1 << 1,
	BURNER_LAYOUT_VIDEO		= 1 << 2
} BurnerLayoutType;

typedef struct {
	GtkPaned parent;
	BurnerLayoutPrivate *priv;
} BurnerLayout;

typedef struct {
	GtkPanedClass parent_class;
} BurnerLayoutClass;

GType burner_layout_get_type (void);
GtkWidget *burner_layout_new (void);

void
burner_layout_add_project (BurnerLayout *layout,
			    GtkWidget *project);
void
burner_layout_add_preview (BurnerLayout*layout,
			    GtkWidget *preview);

void
burner_layout_add_source (BurnerLayout *layout,
			   GtkWidget *child,
			   const gchar *id,
			   const gchar *subtitle,
			   const gchar *icon_name,
			   BurnerLayoutType types);
void
burner_layout_load (BurnerLayout *layout,
		     BurnerLayoutType type);

void
burner_layout_register_ui (BurnerLayout *layout,
			    GtkUIManager *manager);

G_END_DECLS

#endif /* BURNER_LAYOUT_H */
