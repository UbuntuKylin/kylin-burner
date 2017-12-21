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

#ifndef _BURNER_DATA_VFS_H_
#define _BURNER_DATA_VFS_H_

#include <glib-object.h>
#include <gtk/gtk.h>

#include "burner-data-session.h"
#include "burner-filtered-uri.h"

G_BEGIN_DECLS

#define BURNER_SCHEMA_FILTER			"org.gnome.burner.filter"
#define BURNER_PROPS_FILTER_HIDDEN	        "hidden"
#define BURNER_PROPS_FILTER_BROKEN	        "broken-sym"
#define BURNER_PROPS_FILTER_REPLACE_SYMLINK    "replace-sym"

#define BURNER_TYPE_DATA_VFS             (burner_data_vfs_get_type ())
#define BURNER_DATA_VFS(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), BURNER_TYPE_DATA_VFS, BurnerDataVFS))
#define BURNER_DATA_VFS_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), BURNER_TYPE_DATA_VFS, BurnerDataVFSClass))
#define BURNER_IS_DATA_VFS(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BURNER_TYPE_DATA_VFS))
#define BURNER_IS_DATA_VFS_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), BURNER_TYPE_DATA_VFS))
#define BURNER_DATA_VFS_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), BURNER_TYPE_DATA_VFS, BurnerDataVFSClass))

typedef struct _BurnerDataVFSClass BurnerDataVFSClass;
typedef struct _BurnerDataVFS BurnerDataVFS;

struct _BurnerDataVFSClass
{
	BurnerDataSessionClass parent_class;

	void	(*activity_changed)	(BurnerDataVFS *vfs,
					 gboolean active);
};

struct _BurnerDataVFS
{
	BurnerDataSession parent_instance;
};

GType burner_data_vfs_get_type (void) G_GNUC_CONST;

gboolean
burner_data_vfs_is_active (BurnerDataVFS *vfs);

gboolean
burner_data_vfs_is_loading_uri (BurnerDataVFS *vfs);

gboolean
burner_data_vfs_load_mime (BurnerDataVFS *vfs,
			    BurnerFileNode *node);

gboolean
burner_data_vfs_require_node_load (BurnerDataVFS *vfs,
				    BurnerFileNode *node);

gboolean
burner_data_vfs_require_directory_contents (BurnerDataVFS *vfs,
					     BurnerFileNode *node);

BurnerFilteredUri *
burner_data_vfs_get_filtered_model (BurnerDataVFS *vfs);

G_END_DECLS

#endif /* _BURNER_DATA_VFS_H_ */
