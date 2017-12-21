/***************************************************************************
 *            burner-file-chooser.h
 *
 *  lun mai 29 08:53:18 2006
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

#ifndef BURNER_FILE_CHOOSER_H
#define BURNER_FILE_CHOOSER_H

#include <glib.h>
#include <glib-object.h>

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define BURNER_TYPE_FILE_CHOOSER         (burner_file_chooser_get_type ())
#define BURNER_FILE_CHOOSER(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), BURNER_TYPE_FILE_CHOOSER, BurnerFileChooser))
#define BURNER_FILE_CHOOSER_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k), BURNER_TYPE_FILE_CHOOSER, BurnerFileChooserClass))
#define BURNER_IS_FILE_CHOOSER(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), BURNER_TYPE_FILE_CHOOSER))
#define BURNER_IS_FILE_CHOOSER_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), BURNER_TYPE_FILE_CHOOSER))
#define BURNER_FILE_CHOOSER_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), BURNER_TYPE_FILE_CHOOSER, BurnerFileChooserClass))

typedef struct BurnerFileChooserPrivate BurnerFileChooserPrivate;

typedef struct {
	GtkAlignment parent;
	BurnerFileChooserPrivate *priv;
} BurnerFileChooser;

typedef struct {
	GtkAlignmentClass parent_class;
} BurnerFileChooserClass;

GType burner_file_chooser_get_type (void);
GtkWidget *burner_file_chooser_new (void);

void
burner_file_chooser_customize (GtkWidget *widget,
				gpointer null_data);

G_END_DECLS

#endif /* BURNER_FILE_CHOOSER_H */
