/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/*
 * burner
 * Copyright (C) Philippe Rouquier 2007 <bonfire-app@wanadoo.fr>
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

#ifndef _BURNER_VIDEO_TREE_MODEL_H_
#define _BURNER_VIDEO_TREE_MODEL_H_

#include <glib-object.h>

#include "burner-track.h"
#include "burner-session-cfg.h"

G_BEGIN_DECLS

/* This DND target when moving nodes inside ourselves */
#define BURNER_DND_TARGET_SELF_FILE_NODES	"GTK_TREE_MODEL_ROW"

struct _BurnerDNDVideoContext {
	GtkTreeModel *model;
	GList *references;
};
typedef struct _BurnerDNDVideoContext BurnerDNDVideoContext;

typedef enum {
	BURNER_VIDEO_TREE_MODEL_NAME		= 0,		/* Markup */
	BURNER_VIDEO_TREE_MODEL_ARTIST		= 1,
	BURNER_VIDEO_TREE_MODEL_THUMBNAIL,
	BURNER_VIDEO_TREE_MODEL_ICON_NAME,
	BURNER_VIDEO_TREE_MODEL_SIZE,
	BURNER_VIDEO_TREE_MODEL_EDITABLE,
	BURNER_VIDEO_TREE_MODEL_SELECTABLE,
	BURNER_VIDEO_TREE_MODEL_INDEX,
	BURNER_VIDEO_TREE_MODEL_INDEX_NUM,
	BURNER_VIDEO_TREE_MODEL_IS_GAP,
	BURNER_VIDEO_TREE_MODEL_WEIGHT,
	BURNER_VIDEO_TREE_MODEL_STYLE,
	BURNER_VIDEO_TREE_MODEL_COL_NUM
} BurnerVideoProjectColumn;

#define BURNER_TYPE_VIDEO_TREE_MODEL             (burner_video_tree_model_get_type ())
#define BURNER_VIDEO_TREE_MODEL(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), BURNER_TYPE_VIDEO_TREE_MODEL, BurnerVideoTreeModel))
#define BURNER_VIDEO_TREE_MODEL_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), BURNER_TYPE_VIDEO_TREE_MODEL, BurnerVideoTreeModelClass))
#define BURNER_IS_VIDEO_TREE_MODEL(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BURNER_TYPE_VIDEO_TREE_MODEL))
#define BURNER_IS_VIDEO_TREE_MODEL_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), BURNER_TYPE_VIDEO_TREE_MODEL))
#define BURNER_VIDEO_TREE_MODEL_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), BURNER_TYPE_VIDEO_TREE_MODEL, BurnerVideoTreeModelClass))

typedef struct _BurnerVideoTreeModelClass BurnerVideoTreeModelClass;
typedef struct _BurnerVideoTreeModel BurnerVideoTreeModel;

struct _BurnerVideoTreeModelClass
{
	GObjectClass parent_class;
};

struct _BurnerVideoTreeModel
{
	GObject parent_instance;
};

GType burner_video_tree_model_get_type (void) G_GNUC_CONST;

BurnerVideoTreeModel *
burner_video_tree_model_new (void);

void
burner_video_tree_model_set_session (BurnerVideoTreeModel *model,
				      BurnerSessionCfg *session);
BurnerSessionCfg *
burner_video_tree_model_get_session (BurnerVideoTreeModel *model);

BurnerTrack *
burner_video_tree_model_path_to_track (BurnerVideoTreeModel *self,
					GtkTreePath *path);

GtkTreePath *
burner_video_tree_model_track_to_path (BurnerVideoTreeModel *self,
				        BurnerTrack *track);

void
burner_video_tree_model_move_before (BurnerVideoTreeModel *self,
				      GtkTreeIter *iter,
				      GtkTreePath *dest_before);

G_END_DECLS

#endif /* _BURNER_VIDEO_TREE_MODEL_H_ */
