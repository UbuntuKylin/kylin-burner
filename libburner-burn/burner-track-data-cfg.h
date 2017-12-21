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

#ifndef _BURNER_TRACK_DATA_CFG_H_
#define _BURNER_TRACK_DATA_CFG_H_

#include <glib-object.h>
#include <gtk/gtk.h>

#include <burner-track-data.h>

G_BEGIN_DECLS

/**
 * GtkTreeModel Part
 */

/* This DND target when moving nodes inside ourselves */
#define BURNER_DND_TARGET_DATA_TRACK_REFERENCE_LIST	"GTK_TREE_MODEL_ROW"

typedef enum {
	BURNER_DATA_TREE_MODEL_NAME		= 0,
	BURNER_DATA_TREE_MODEL_URI,
	BURNER_DATA_TREE_MODEL_MIME_DESC,
	BURNER_DATA_TREE_MODEL_MIME_ICON,
	BURNER_DATA_TREE_MODEL_SIZE,
	BURNER_DATA_TREE_MODEL_SHOW_PERCENT,
	BURNER_DATA_TREE_MODEL_PERCENT,
	BURNER_DATA_TREE_MODEL_STYLE,
	BURNER_DATA_TREE_MODEL_COLOR,
	BURNER_DATA_TREE_MODEL_EDITABLE,
	BURNER_DATA_TREE_MODEL_IS_FILE,
	BURNER_DATA_TREE_MODEL_IS_LOADING,
	BURNER_DATA_TREE_MODEL_IS_IMPORTED,
	BURNER_DATA_TREE_MODEL_COL_NUM
} BurnerTrackDataCfgColumn;


#define BURNER_TYPE_TRACK_DATA_CFG             (burner_track_data_cfg_get_type ())
#define BURNER_TRACK_DATA_CFG(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), BURNER_TYPE_TRACK_DATA_CFG, BurnerTrackDataCfg))
#define BURNER_TRACK_DATA_CFG_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), BURNER_TYPE_TRACK_DATA_CFG, BurnerTrackDataCfgClass))
#define BURNER_IS_TRACK_DATA_CFG(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BURNER_TYPE_TRACK_DATA_CFG))
#define BURNER_IS_TRACK_DATA_CFG_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), BURNER_TYPE_TRACK_DATA_CFG))
#define BURNER_TRACK_DATA_CFG_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), BURNER_TYPE_TRACK_DATA_CFG, BurnerTrackDataCfgClass))

typedef struct _BurnerTrackDataCfgClass BurnerTrackDataCfgClass;
typedef struct _BurnerTrackDataCfg BurnerTrackDataCfg;

struct _BurnerTrackDataCfgClass
{
	BurnerTrackDataClass parent_class;
};

struct _BurnerTrackDataCfg
{
	BurnerTrackData parent_instance;
};

GType burner_track_data_cfg_get_type (void) G_GNUC_CONST;

BurnerTrackDataCfg *
burner_track_data_cfg_new (void);

gboolean
burner_track_data_cfg_add (BurnerTrackDataCfg *track,
			    const gchar *uri,
			    GtkTreePath *parent);
GtkTreePath *
burner_track_data_cfg_add_empty_directory (BurnerTrackDataCfg *track,
					    const gchar *name,
					    GtkTreePath *parent);

gboolean
burner_track_data_cfg_remove (BurnerTrackDataCfg *track,
			       GtkTreePath *treepath);
gboolean
burner_track_data_cfg_rename (BurnerTrackDataCfg *track,
			       const gchar *newname,
			       GtkTreePath *treepath);

gboolean
burner_track_data_cfg_reset (BurnerTrackDataCfg *track);

gboolean
burner_track_data_cfg_load_medium (BurnerTrackDataCfg *track,
				    BurnerMedium *medium,
				    GError **error);
void
burner_track_data_cfg_unload_current_medium (BurnerTrackDataCfg *track);

BurnerMedium *
burner_track_data_cfg_get_current_medium (BurnerTrackDataCfg *track);

GSList *
burner_track_data_cfg_get_available_media (BurnerTrackDataCfg *track);

/**
 * For filtered URIs tree model
 */

void
burner_track_data_cfg_dont_filter_uri (BurnerTrackDataCfg *track,
					const gchar *uri);

GSList *
burner_track_data_cfg_get_restored_list (BurnerTrackDataCfg *track);

enum  {
	BURNER_FILTERED_STOCK_ID_COL,
	BURNER_FILTERED_URI_COL,
	BURNER_FILTERED_STATUS_COL,
	BURNER_FILTERED_FATAL_ERROR_COL,
	BURNER_FILTERED_NB_COL,
};


void
burner_track_data_cfg_restore (BurnerTrackDataCfg *track,
				GtkTreePath *treepath);

GtkTreeModel *
burner_track_data_cfg_get_filtered_model (BurnerTrackDataCfg *track);


/**
 * Track Spanning
 */

BurnerBurnResult
burner_track_data_cfg_span (BurnerTrackDataCfg *track,
			     goffset sectors,
			     BurnerTrackData *new_track);
BurnerBurnResult
burner_track_data_cfg_span_again (BurnerTrackDataCfg *track);

BurnerBurnResult
burner_track_data_cfg_span_possible (BurnerTrackDataCfg *track,
				      goffset sectors);

goffset
burner_track_data_cfg_span_max_space (BurnerTrackDataCfg *track);

void
burner_track_data_cfg_span_stop (BurnerTrackDataCfg *track);

/**
 * Icon
 */

GIcon *
burner_track_data_cfg_get_icon (BurnerTrackDataCfg *track);

gchar *
burner_track_data_cfg_get_icon_path (BurnerTrackDataCfg *track);

gboolean
burner_track_data_cfg_set_icon (BurnerTrackDataCfg *track,
				 const gchar *icon_path,
				 GError **error);

G_END_DECLS

#endif /* _BURNER_TRACK_DATA_CFG_H_ */
