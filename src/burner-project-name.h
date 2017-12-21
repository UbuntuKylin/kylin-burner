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

#ifndef _BURNER_PROJECT_NAME_H_
#define _BURNER_PROJECT_NAME_H_

#include <glib-object.h>

#include <gtk/gtk.h>

#include "burner-session.h"

#include "burner-project-type-chooser.h"

G_BEGIN_DECLS

#define BURNER_TYPE_PROJECT_NAME             (burner_project_name_get_type ())
#define BURNER_PROJECT_NAME(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), BURNER_TYPE_PROJECT_NAME, BurnerProjectName))
#define BURNER_PROJECT_NAME_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), BURNER_TYPE_PROJECT_NAME, BurnerProjectNameClass))
#define BURNER_IS_PROJECT_NAME(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BURNER_TYPE_PROJECT_NAME))
#define BURNER_IS_PROJECT_NAME_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), BURNER_TYPE_PROJECT_NAME))
#define BURNER_PROJECT_NAME_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), BURNER_TYPE_PROJECT_NAME, BurnerProjectNameClass))

typedef struct _BurnerProjectNameClass BurnerProjectNameClass;
typedef struct _BurnerProjectName BurnerProjectName;

struct _BurnerProjectNameClass
{
	GtkEntryClass parent_class;
};

struct _BurnerProjectName
{
	GtkEntry parent_instance;
};

GType burner_project_name_get_type (void) G_GNUC_CONST;

GtkWidget *
burner_project_name_new (BurnerBurnSession *session);

void
burner_project_name_set_session (BurnerProjectName *project,
				  BurnerBurnSession *session);

G_END_DECLS

#endif /* _BURNER_PROJECT_NAME_H_ */
