/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/*
 * Libburner-burn
 * Copyright (C) Philippe Rouquier 2005-2009 <bonfire-app@wanadoo.fr>
 *
 * Libburner-burn is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * The Libburner-burn authors hereby grant permission for non-GPL compatible
 * GStreamer plugins to be used and distributed together with GStreamer
 * and Libburner-burn. This permission is above and beyond the permissions granted
 * by the GPL license by which Libburner-burn is covered. If you modify this code
 * you may extend this exception to your version of the code, but you are not
 * obligated to do so. If you do not wish to do so, delete this exception
 * statement from your version.
 * 
 * Libburner-burn is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Library General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to:
 * 	The Free Software Foundation, Inc.,
 * 	51 Franklin Street, Fifth Floor
 * 	Boston, MA  02110-1301, USA.
 */

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#ifndef _BURNER_DATA_PROJECT_H_
#define _BURNER_DATA_PROJECT_H_

#include <glib-object.h>
#include <gio/gio.h>

#include <gtk/gtk.h>

#include "burner-file-node.h"
#include "burner-session.h"

#include "burner-track-data.h"

#ifdef BUILD_INOTIFY
#include "burner-file-monitor.h"
#endif

G_BEGIN_DECLS


#define BURNER_TYPE_DATA_PROJECT             (burner_data_project_get_type ())
#define BURNER_DATA_PROJECT(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), BURNER_TYPE_DATA_PROJECT, BurnerDataProject))
#define BURNER_DATA_PROJECT_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), BURNER_TYPE_DATA_PROJECT, BurnerDataProjectClass))
#define BURNER_IS_DATA_PROJECT(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BURNER_TYPE_DATA_PROJECT))
#define BURNER_IS_DATA_PROJECT_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), BURNER_TYPE_DATA_PROJECT))
#define BURNER_DATA_PROJECT_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), BURNER_TYPE_DATA_PROJECT, BurnerDataProjectClass))

typedef struct _BurnerDataProjectClass BurnerDataProjectClass;
typedef struct _BurnerDataProject BurnerDataProject;

struct _BurnerDataProjectClass
{
#ifdef BUILD_INOTIFY
	BurnerFileMonitorClass parent_class;
#else
	GObjectClass parent_class;
#endif

	/* virtual functions */

	/**
	 * num_nodes is the number of nodes that were at the root of the 
	 * project.
	 */
	void		(*reset)		(BurnerDataProject *project,
						 guint num_nodes);

	/* NOTE: node_added is also called when there is a moved node;
	 * in this case a node_removed is first called and then the
	 * following function is called (mostly to match GtkTreeModel
	 * API). To detect such a case look at uri which will then be
	 * set to NULL.
	 * NULL uri can also happen when it's a created directory.
	 * if return value is FALSE, node was invalidated during call */
	gboolean	(*node_added)		(BurnerDataProject *project,
						 BurnerFileNode *node,
						 const gchar *uri);

	/* This is more an unparent signal. It shouldn't be assumed that the
	 * node was destroyed or not destroyed. Like the above function, it is
	 * also called when a node is moved. */
	void		(*node_removed)		(BurnerDataProject *project,
						 BurnerFileNode *former_parent,
						 guint former_position,
						 BurnerFileNode *node);

	void		(*node_changed)		(BurnerDataProject *project,
						 BurnerFileNode *node);
	void		(*node_reordered)	(BurnerDataProject *project,
						 BurnerFileNode *parent,
						 gint *new_order);

	void		(*uri_removed)		(BurnerDataProject *project,
						 const gchar *uri);
};

struct _BurnerDataProject
{
#ifdef BUILD_INOTIFY
	BurnerFileMonitor parent_instance;
#else
	GObject parent_instance;
#endif
};

GType burner_data_project_get_type (void) G_GNUC_CONST;

void
burner_data_project_reset (BurnerDataProject *project);

goffset
burner_data_project_get_sectors (BurnerDataProject *project);

goffset
burner_data_project_improve_image_size_accuracy (goffset blocks,
						  guint64 dir_num,
						  BurnerImageFS fs_type);
goffset
burner_data_project_get_folder_sectors (BurnerDataProject *project,
					 BurnerFileNode *node);

gboolean
burner_data_project_get_contents (BurnerDataProject *project,
				   GSList **grafts,
				   GSList **unreadable,
				   gboolean hidden_nodes,
				   gboolean joliet_compat,
				   gboolean append_slash);

gboolean
burner_data_project_is_empty (BurnerDataProject *project);

gboolean
burner_data_project_has_symlinks (BurnerDataProject *project);

gboolean
burner_data_project_is_video_project (BurnerDataProject *project);

gboolean
burner_data_project_is_joliet_compliant (BurnerDataProject *project);

guint
burner_data_project_load_contents (BurnerDataProject *project,
				    GSList *grafts,
				    GSList *excluded);

BurnerFileNode *
burner_data_project_add_hidden_node (BurnerDataProject *project,
				      const gchar *uri,
				      const gchar *name,
				      BurnerFileNode *parent);

BurnerFileNode *
burner_data_project_add_loading_node (BurnerDataProject *project,
				       const gchar *uri,
				       BurnerFileNode *parent);
BurnerFileNode *
burner_data_project_add_node_from_info (BurnerDataProject *project,
					 const gchar *uri,
					 GFileInfo *info,
					 BurnerFileNode *parent);
BurnerFileNode *
burner_data_project_add_empty_directory (BurnerDataProject *project,
					  const gchar *name,
					  BurnerFileNode *parent);
BurnerFileNode *
burner_data_project_add_imported_session_file (BurnerDataProject *project,
						GFileInfo *info,
						BurnerFileNode *parent);

void
burner_data_project_remove_node (BurnerDataProject *project,
				  BurnerFileNode *node);
void
burner_data_project_destroy_node (BurnerDataProject *self,
				   BurnerFileNode *node);

gboolean
burner_data_project_node_loaded (BurnerDataProject *project,
				  BurnerFileNode *node,
				  const gchar *uri,
				  GFileInfo *info);
void
burner_data_project_node_reloaded (BurnerDataProject *project,
				    BurnerFileNode *node,
				    const gchar *uri,
				    GFileInfo *info);
void
burner_data_project_directory_node_loaded (BurnerDataProject *project,
					    BurnerFileNode *parent);

gboolean
burner_data_project_rename_node (BurnerDataProject *project,
				  BurnerFileNode *node,
				  const gchar *name);

gboolean
burner_data_project_move_node (BurnerDataProject *project,
				BurnerFileNode *node,
				BurnerFileNode *parent);

void
burner_data_project_restore_uri (BurnerDataProject *project,
				  const gchar *uri);
void
burner_data_project_exclude_uri (BurnerDataProject *project,
				  const gchar *uri);

guint
burner_data_project_reference_new (BurnerDataProject *project,
				    BurnerFileNode *node);
void
burner_data_project_reference_free (BurnerDataProject *project,
				     guint reference);
BurnerFileNode *
burner_data_project_reference_get (BurnerDataProject *project,
				    guint reference);

BurnerFileNode *
burner_data_project_get_root (BurnerDataProject *project);

gchar *
burner_data_project_node_to_uri (BurnerDataProject *project,
				  BurnerFileNode *node);
gchar *
burner_data_project_node_to_path (BurnerDataProject *self,
				   BurnerFileNode *node);

void
burner_data_project_set_sort_function (BurnerDataProject *project,
					GtkSortType sort_type,
					GCompareFunc sort_func);

BurnerFileNode *
burner_data_project_watch_path (BurnerDataProject *project,
				 const gchar *path);

BurnerBurnResult
burner_data_project_span (BurnerDataProject *project,
			   goffset max_sectors,
			   gboolean append_slash,
			   gboolean joliet,
			   BurnerTrackData *track);
BurnerBurnResult
burner_data_project_span_again (BurnerDataProject *project);

BurnerBurnResult
burner_data_project_span_possible (BurnerDataProject *project,
				    goffset max_sectors);
goffset
burner_data_project_get_max_space (BurnerDataProject *self);

void
burner_data_project_span_stop (BurnerDataProject *project);

G_END_DECLS

#endif /* _BURNER_DATA_PROJECT_H_ */
