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

#ifndef _BURNER_TASK_ITEM_H
#define _BURNER_TASK_ITEM_H

#include <glib-object.h>

#include "burn-basics.h"
#include "burn-task-ctx.h"

G_BEGIN_DECLS

#define BURNER_TYPE_TASK_ITEM			(burner_task_item_get_type ())
#define BURNER_TASK_ITEM(obj)			(G_TYPE_CHECK_INSTANCE_CAST ((obj), BURNER_TYPE_TASK_ITEM, BurnerTaskItem))
#define BURNER_TASK_ITEM_CLASS(vtable)		(G_TYPE_CHECK_CLASS_CAST ((vtable), BURNER_TYPE_TASK_ITEM, BurnerTaskItemIFace))
#define BURNER_IS_TASK_ITEM(obj)		(G_TYPE_CHECK_INSTANCE_TYPE ((obj), BURNER_TYPE_TASK_ITEM))
#define BURNER_IS_TASK_ITEM_CLASS(vtable)	(G_TYPE_CHECK_CLASS_TYPE ((vtable), BURNER_TYPE_TASK_ITEM))
#define BURNER_TASK_ITEM_GET_CLASS(inst)	(G_TYPE_INSTANCE_GET_INTERFACE ((inst), BURNER_TYPE_TASK_ITEM, BurnerTaskItemIFace))

typedef struct _BurnerTaskItem BurnerTaskItem;
typedef struct _BurnerTaskItemIFace BurnerTaskItemIFace;

struct _BurnerTaskItemIFace {
	GTypeInterface parent;

	BurnerBurnResult	(*link)		(BurnerTaskItem *input,
						 BurnerTaskItem *output);
	BurnerTaskItem *	(*previous)	(BurnerTaskItem *item);
	BurnerTaskItem *	(*next)		(BurnerTaskItem *item);

	gboolean		(*is_active)	(BurnerTaskItem *item);
	BurnerBurnResult	(*activate)	(BurnerTaskItem *item,
						 BurnerTaskCtx *ctx,
						 GError **error);
	BurnerBurnResult	(*start)	(BurnerTaskItem *item,
						 GError **error);
	BurnerBurnResult	(*clock_tick)	(BurnerTaskItem *item,
						 BurnerTaskCtx *ctx,
						 GError **error);
	BurnerBurnResult	(*stop)		(BurnerTaskItem *item,
						 BurnerTaskCtx *ctx,
						 GError **error);
};

GType
burner_task_item_get_type (void);

BurnerBurnResult
burner_task_item_link (BurnerTaskItem *input,
			BurnerTaskItem *output);

BurnerTaskItem *
burner_task_item_previous (BurnerTaskItem *item);

BurnerTaskItem *
burner_task_item_next (BurnerTaskItem *item);

gboolean
burner_task_item_is_active (BurnerTaskItem *item);

BurnerBurnResult
burner_task_item_activate (BurnerTaskItem *item,
			    BurnerTaskCtx *ctx,
			    GError **error);

BurnerBurnResult
burner_task_item_start (BurnerTaskItem *item,
			 GError **error);

BurnerBurnResult
burner_task_item_clock_tick (BurnerTaskItem *item,
			      BurnerTaskCtx *ctx,
			      GError **error);

BurnerBurnResult
burner_task_item_stop (BurnerTaskItem *item,
			BurnerTaskCtx *ctx,
			GError **error);

G_END_DECLS

#endif
