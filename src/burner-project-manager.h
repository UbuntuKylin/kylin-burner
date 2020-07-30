/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */

/***************************************************************************
 *            burner-project-manager.h
 *
 *  mer mai 24 14:22:56 2006
 *  Copyright  2006  Rouquier Philippe
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

#ifndef BURNER_PROJECT_MANAGER_H
#define BURNER_PROJECT_MANAGER_H

#include <glib.h>
#include <glib-object.h>

#include <gtk/gtk.h>

#include "burner-io.h"
#include "burner-medium.h"
#include "burner-project-parse.h"
#include "burner-project-type-chooser.h"
#include "burner-session-cfg.h"

G_BEGIN_DECLS

#define BURNER_TYPE_PROJECT_MANAGER         (burner_project_manager_get_type ())
#define BURNER_PROJECT_MANAGER(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), BURNER_TYPE_PROJECT_MANAGER, BurnerProjectManager))
#define BURNER_PROJECT_MANAGER_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k), BURNER_TYPE_PROJECT_MANAGER, BurnerProjectManagerClass))
#define BURNER_IS_PROJECT_MANAGER(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), BURNER_TYPE_PROJECT_MANAGER))
#define BURNER_IS_PROJECT_MANAGER_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), BURNER_TYPE_PROJECT_MANAGER))
#define BURNER_PROJECT_MANAGER_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), BURNER_TYPE_PROJECT_MANAGER, BurnerProjectManagerClass))

typedef struct BurnerProjectManagerPrivate BurnerProjectManagerPrivate;

typedef struct {
	GtkNotebook parent;
	BurnerProjectManagerPrivate *priv;
} BurnerProjectManager;

struct BurnerProjectManagerPrivate {
	BurnerProjectType type;
	BurnerIOJobBase *size_preview;

	GtkWidget *project;
	GtkWidget *layout;

	gchar **selected;
	guint preview_id;

	guint status_ctx;

	GtkActionGroup *action_group;
};

typedef struct {
	GtkNotebookClass parent_class;	
} BurnerProjectManagerClass;

extern BurnerProjectManager *manager_chooser;
extern GtkWidget *type;

GType burner_project_manager_get_type (void);
GtkWidget *burner_project_manager_new (void);

gboolean
burner_project_manager_open_session (BurnerProjectManager *manager,
                                      BurnerSessionCfg *session);

void
burner_project_manager_empty (BurnerProjectManager *manager);

/**
 * returns the path of the project that was saved. NULL otherwise.
 */

gboolean
burner_project_manager_save_session (BurnerProjectManager *manager,
				      const gchar *path,
				      gchar **saved_uri,
				      gboolean cancellable);

void
burner_project_manager_register_ui (BurnerProjectManager *manager,
				     GtkUIManager *ui_manager);

void
burner_project_manager_switch (BurnerProjectManager *manager,
				BurnerProjectType type,
				gboolean reset);

G_END_DECLS

#endif /* BURNER_PROJECT_MANAGER_H */
