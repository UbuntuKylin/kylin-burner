/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/*
 * burner
 * Copyright (C) Philippe Rouquier 2005-2008 <bonfire-app@wanadoo.fr>
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

#ifndef _BURNER_FILE_FILTERED_H_
#define _BURNER_FILE_FILTERED_H_

#include <glib-object.h>
#include <gtk/gtk.h>

#include "burner-track-data-cfg.h"

G_BEGIN_DECLS

#define BURNER_TYPE_FILE_FILTERED             (burner_file_filtered_get_type ())
#define BURNER_FILE_FILTERED(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), BURNER_TYPE_FILE_FILTERED, BurnerFileFiltered))
#define BURNER_FILE_FILTERED_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), BURNER_TYPE_FILE_FILTERED, BurnerFileFilteredClass))
#define BURNER_IS_FILE_FILTERED(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BURNER_TYPE_FILE_FILTERED))
#define BURNER_IS_FILE_FILTERED_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), BURNER_TYPE_FILE_FILTERED))
#define BURNER_FILE_FILTERED_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), BURNER_TYPE_FILE_FILTERED, BurnerFileFilteredClass))

typedef struct _BurnerFileFilteredClass BurnerFileFilteredClass;
typedef struct _BurnerFileFiltered BurnerFileFiltered;

struct _BurnerFileFilteredClass
{
	GtkExpanderClass parent_class;
};

struct _BurnerFileFiltered
{
	GtkExpander parent_instance;
};

GType burner_file_filtered_get_type (void) G_GNUC_CONST;

GtkWidget*
burner_file_filtered_new (BurnerTrackDataCfg *track);

void
burner_file_filtered_set_right_button_group (BurnerFileFiltered *self,
					      GtkSizeGroup *group);

G_END_DECLS

#endif /* _BURNER_FILE_FILTERED_H_ */
