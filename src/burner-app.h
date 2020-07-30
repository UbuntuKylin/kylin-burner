/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/*
 * Burner
 * Copyright (C) Philippe Rouquier 2005-2009 <bonfire-app@wanadoo.fr>
 * Copyright (C) 2017,Tianjin KYLIN Information Technology Co., Ltd.
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

#ifndef _BURNER_APP_H_
#define _BURNER_APP_H_

#include <glib-object.h>
#include <gtk/gtk.h>
#include <stdio.h>

#include "burner-session-cfg.h"

G_BEGIN_DECLS

#define BURNER_TYPE_APP             (burner_app_get_type ())
#define BURNER_APP(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), BURNER_TYPE_APP, BurnerApp))
#define BURNER_APP_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), BURNER_TYPE_APP, BurnerAppClass))
#define BURNER_IS_APP(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BURNER_TYPE_APP))
#define BURNER_IS_APP_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), BURNER_TYPE_APP))
#define BURNER_APP_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), BURNER_TYPE_APP, BurnerAppClass))

typedef struct _BurnerAppClass BurnerAppClass;
typedef struct _BurnerApp BurnerApp;
extern int nb_items;
extern GtkWidget *widget[10];
extern GtkWidget *statusbarbox;
extern GtkWidget *maximize_bt;
extern GtkWidget *unmaximize_bt;

struct _BurnerAppClass
{
	GtkWindowClass parent_class;
};

struct _BurnerApp
{
	GtkWindow parent_instance;
};

GType burner_app_get_type (void) G_GNUC_CONST;

BurnerApp *
burner_app_new (GApplication *gapp);

BurnerApp *
burner_app_get_default (void);

void
burner_app_set_parent (BurnerApp *app,
			guint xid);

void
burner_app_set_toplevel (BurnerApp *app, GtkWindow *window);

void
burner_app_create_mainwin (BurnerApp *app);

gboolean
burner_app_run_mainwin (BurnerApp *app);

gboolean
burner_app_is_running (BurnerApp *app);

GtkWidget *
burner_app_dialog (BurnerApp *app,
		    const gchar *primary_message,
		    GtkButtonsType button_type,
		    GtkMessageType msg_type);

void
burner_app_alert (BurnerApp *app,
		   const gchar *primary_message,
		   const gchar *secondary_message,
		   GtkMessageType type);

gboolean
burner_app_burn (BurnerApp *app,
		  BurnerBurnSession *session,
		  gboolean multi);

void
burner_app_burn_uri (BurnerApp *app,
		      BurnerDrive *burner,
		      gboolean burn);

void
burner_app_data (BurnerApp *app,
		  BurnerDrive *burner,
		  gchar * const *uris,
		  gboolean burn);

void
burner_app_stream (BurnerApp *app,
		    BurnerDrive *burner,
		    gchar * const *uris,
		    gboolean is_video,
		    gboolean burn);

void
burner_app_image (BurnerApp *app,
		   BurnerDrive *burner,
		   const gchar *uri,
		   gboolean burn);

void
burner_app_copy_disc (BurnerApp *app,
		       BurnerDrive *burner,
		       const gchar *device,
		       const gchar *cover,
		       gboolean burn);

void
burner_app_blank (BurnerApp *app,
		   BurnerDrive *burner,
		   gboolean burn);

void
burner_app_check (BurnerApp *app,
		   BurnerDrive *burner,
		   gboolean burn);

gboolean
burner_app_open_project (BurnerApp *app,
			  BurnerDrive *burner,
                          const gchar *uri,
                          gboolean is_playlist,
                          gboolean warn_user,
                          gboolean burn);

gboolean
burner_app_open_uri (BurnerApp *app,
                      const gchar *uri_arg,
                      gboolean warn_user);

gboolean
burner_app_open_uri_drive_detection (BurnerApp *app,
                                      BurnerDrive *burner,
                                      const gchar *uri,
                                      const gchar *cover_project,
                                      gboolean burn);
GtkWidget *
burner_app_get_statusbar1 (BurnerApp *app);

GtkWidget *
burner_app_get_statusbar2 (BurnerApp *app);

GtkWidget *
burner_app_get_project_manager (BurnerApp *app);

/**
 * Session management
 */

#define BURNER_SESSION_TMP_PROJECT_PATH	"burner-tmp-project"

const gchar *
burner_app_get_saved_contents (BurnerApp *app);

gboolean
burner_app_save_contents (BurnerApp *app,
			   gboolean cancellable);

G_END_DECLS

#endif /* _BURNER_APP_H_ */
