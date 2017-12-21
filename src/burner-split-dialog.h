/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/*
 * burner
 * Copyright (C) Philippe Rouquier 2007-2008 <bonfire-app@wanadoo.fr>
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

#ifndef _BURNER_SPLIT_DIALOG_H_
#define _BURNER_SPLIT_DIALOG_H_

#include <glib-object.h>

#include <gtk/gtk.h>

G_BEGIN_DECLS

struct _BurnerAudioSlice {
	gint64 start;
	gint64 end;
};
typedef struct _BurnerAudioSlice BurnerAudioSlice;

#define BURNER_TYPE_SPLIT_DIALOG             (burner_split_dialog_get_type ())
#define BURNER_SPLIT_DIALOG(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), BURNER_TYPE_SPLIT_DIALOG, BurnerSplitDialog))
#define BURNER_SPLIT_DIALOG_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), BURNER_TYPE_SPLIT_DIALOG, BurnerSplitDialogClass))
#define BURNER_IS_SPLIT_DIALOG(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BURNER_TYPE_SPLIT_DIALOG))
#define BURNER_IS_SPLIT_DIALOG_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), BURNER_TYPE_SPLIT_DIALOG))
#define BURNER_SPLIT_DIALOG_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), BURNER_TYPE_SPLIT_DIALOG, BurnerSplitDialogClass))

typedef struct _BurnerSplitDialogClass BurnerSplitDialogClass;
typedef struct _BurnerSplitDialog BurnerSplitDialog;

struct _BurnerSplitDialogClass
{
	GtkDialogClass parent_class;
};

struct _BurnerSplitDialog
{
	GtkDialog parent_instance;
};

GType burner_split_dialog_get_type (void) G_GNUC_CONST;

GtkWidget *
burner_split_dialog_new (void);

void
burner_split_dialog_set_uri (BurnerSplitDialog *dialog,
			      const gchar *uri,
                              const gchar *title,
                              const gchar *artist);
void
burner_split_dialog_set_boundaries (BurnerSplitDialog *dialog,
				     gint64 start,
				     gint64 end);

GSList *
burner_split_dialog_get_slices (BurnerSplitDialog *self);

G_END_DECLS

#endif /* _BURNER_SPLIT_DIALOG_H_ */
