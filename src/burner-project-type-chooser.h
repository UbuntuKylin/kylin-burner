/***************************************************************************
*            cd-type-chooser.h
*
*  ven mai 27 17:33:12 2005
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

#ifndef CD_TYPE_CHOOSER_H
#define CD_TYPE_CHOOSER_H

#include <glib.h>
#include <glib-object.h>

#include <gtk/gtk.h>

#include "burner-project-parse.h"

G_BEGIN_DECLS

#define BURNER_TYPE_PROJECT_TYPE_CHOOSER         (burner_project_type_chooser_get_type ())
#define BURNER_PROJECT_TYPE_CHOOSER(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), BURNER_TYPE_PROJECT_TYPE_CHOOSER, BurnerProjectTypeChooser))
#define BURNER_PROJECT_TYPE_CHOOSER_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k),BURNER_TYPE_PROJECT_TYPE_CHOOSER, BurnerProjectTypeChooserClass))
#define BURNER_IS_PROJECT_TYPE_CHOOSER(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), BURNER_TYPE_PROJECT_TYPE_CHOOSER))
#define BURNER_IS_PROJECT_TYPE_CHOOSER_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), BURNER_TYPE_PROJECT_TYPE_CHOOSER))
#define BURNER_PROJECT_TYPE_CHOOSER_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), BURNER_TYPE_PROJECT_TYPE_CHOOSER, BurnerProjectTypeChooserClass))

typedef struct BurnerProjectTypeChooserPrivate BurnerProjectTypeChooserPrivate;

typedef struct {
	GtkBox parent;
	BurnerProjectTypeChooserPrivate *priv;
} BurnerProjectTypeChooser;

typedef struct {
	GtkBoxClass parent_class;

	void	(*last_saved_clicked)	(BurnerProjectTypeChooser *chooser,
					 const gchar *path);
	void	(*recent_clicked)	(BurnerProjectTypeChooser *chooser,
					 const gchar *uri);
	void	(*chosen)		(BurnerProjectTypeChooser *chooser,
					 BurnerProjectType type);
} BurnerProjectTypeChooserClass;

struct _ItemDescription {
	gchar *text;
	gchar *description;
	gchar *tooltip;
	gchar *image;
	BurnerProjectType type;
};
typedef struct _ItemDescription ItemDescription;

GType burner_project_type_chooser_get_type (void);
GtkWidget *burner_project_type_chooser_new (void);
GtkWidget *burner_project_type_chooser_new_item (BurnerProjectTypeChooser *chooser, ItemDescription *description);

G_END_DECLS

G_END_DECLS

#endif				/* CD_TYPE_CHOOSER_H */
