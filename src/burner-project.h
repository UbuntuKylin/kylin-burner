/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */

/***************************************************************************
 *            project.h
 *
 *  mar nov 29 09:32:17 2005
 *  Copyright  2005  Rouquier Philippe
 * Copyright (C) 2017,Tianjin KYLIN Information Technology Co., Ltd.
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

#ifndef PROJECT_H
#define PROJECT_H

#include <glib.h>
#include <glib-object.h>

#include <gtk/gtk.h>

#include "burner-session-cfg.h"

#include "burner-disc.h"
#include "burner-uri-container.h"
#include "burner-project-type-chooser.h"
#include "burner-jacket-edit.h"

G_BEGIN_DECLS

#define BURNER_TYPE_PROJECT         (burner_project_get_type ())
#define BURNER_PROJECT(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), BURNER_TYPE_PROJECT, BurnerProject))
#define BURNER_PROJECT_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k), BURNER_TYPE_PROJECT, BurnerProjectClass))
#define BURNER_IS_PROJECT(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), BURNER_TYPE_PROJECT))
#define BURNER_IS_PROJECT_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), BURNER_TYPE_PROJECT))
#define BURNER_PROJECT_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), BURNER_TYPE_PROJECT, BurnerProjectClass))

typedef struct BurnerProjectPrivate BurnerProjectPrivate;
extern GtkWidget *burn_button;

typedef struct {
	GtkBox parent;
	BurnerProjectPrivate *priv;
} BurnerProject;

typedef struct {
	GtkBoxClass parent_class;

	void	(*add_pressed)	(BurnerProject *project);
} BurnerProjectClass;

GType burner_project_get_type (void);
GtkWidget *burner_project_new (void);

BurnerBurnResult
burner_project_confirm_switch (BurnerProject *project,
				gboolean keep_files);

void
burner_project_set_audio (BurnerProject *project);
void
burner_project_set_data (BurnerProject *project);
void
burner_project_set_video (BurnerProject *project);
void
burner_project_set_none (BurnerProject *project);

void
burner_project_set_source (BurnerProject *project,
			    BurnerURIContainer *source);

BurnerProjectType
burner_project_convert_to_data (BurnerProject *project);

BurnerProjectType
burner_project_convert_to_stream (BurnerProject *project,
				   gboolean is_video);

BurnerProjectType
burner_project_open_session (BurnerProject *project,
			      BurnerSessionCfg *session);

gboolean
burner_project_save_project (BurnerProject *project);
gboolean
burner_project_save_project_as (BurnerProject *project);

gboolean
burner_project_save_session (BurnerProject *project,
			      const gchar *uri,
			      gchar **saved_uri,
			      gboolean show_cancel);

void
burner_project_register_ui (BurnerProject *project,
			     GtkUIManager *manager);

void
burner_project_create_audio_cover (BurnerProject *project);

G_END_DECLS

#endif /* PROJECT_H */
