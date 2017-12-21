/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/*
 * Burner
 * Copyright (C) Philippe Rouquier 2005-2009 <bonfire-app@wanadoo.fr>
 * 
 *  Burner is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 * 
 * burner is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with burner.  If not, write to:
 * 	The Free Software Foundation, Inc.,
 * 	51 Franklin Street, Fifth Floor
 * 	Boston, MA  02110-1301, USA.
 */

#ifndef _BURNER_RENAME_H_
#define _BURNER_RENAME_H_

#include <glib-object.h>

#include <gtk/gtk.h>

G_BEGIN_DECLS

typedef gboolean (*BurnerRenameCallback)	(GtkTreeModel *model,
						 GtkTreeIter *iter,
						 GtkTreePath *treepath,
						 const gchar *old_name,
						 const gchar *new_name);

#define BURNER_TYPE_RENAME             (burner_rename_get_type ())
#define BURNER_RENAME(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), BURNER_TYPE_RENAME, BurnerRename))
#define BURNER_RENAME_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), BURNER_TYPE_RENAME, BurnerRenameClass))
#define BURNER_IS_RENAME(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BURNER_TYPE_RENAME))
#define BURNER_IS_RENAME_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), BURNER_TYPE_RENAME))
#define BURNER_RENAME_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), BURNER_TYPE_RENAME, BurnerRenameClass))

typedef struct _BurnerRenameClass BurnerRenameClass;
typedef struct _BurnerRename BurnerRename;

struct _BurnerRenameClass
{
	GtkBoxClass parent_class;
};

struct _BurnerRename
{
	GtkBox parent_instance;
};

GType burner_rename_get_type (void) G_GNUC_CONST;

GtkWidget *
burner_rename_new (void);

gboolean
burner_rename_do (BurnerRename *self,
		   GtkTreeSelection *selection,
		   guint column_num,
		   BurnerRenameCallback callback);

void
burner_rename_set_show_keep_default (BurnerRename *self,
				      gboolean show);

G_END_DECLS

#endif /* _BURNER_RENAME_H_ */
