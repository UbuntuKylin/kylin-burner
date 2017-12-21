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

#include <glib-object.h>

#include "burn-basics.h"
#include "burn-task-ctx.h"
#include "burn-task-item.h"

GType
burner_task_item_get_type (void)
{
	static GType type = 0;
	
	if (type == 0) {
		static const GTypeInfo info = {
			sizeof (BurnerTaskItemIFace),
			NULL,   /* base_init */
			NULL,   /* base_finalize */
			NULL,   /* class_init */
			NULL,   /* class_finalize */
			NULL,   /* class_data */
			0,
			0,      /* n_preallocs */
			NULL    /* instance_init */
		};
		type = g_type_register_static (G_TYPE_INTERFACE,
					       "BurnerTaskItem",
					       &info,
					       0);
	}
	return type;
}

BurnerBurnResult
burner_task_item_link (BurnerTaskItem *input, BurnerTaskItem *output)
{
	BurnerTaskItemIFace *klass;

	g_return_val_if_fail (BURNER_IS_TASK_ITEM (input), BURNER_BURN_ERR);
	g_return_val_if_fail (BURNER_IS_TASK_ITEM (output), BURNER_BURN_ERR);

	klass = BURNER_TASK_ITEM_GET_CLASS (input);
	if (klass->link)
		return klass->link (input, output);

	klass = BURNER_TASK_ITEM_GET_CLASS (output);
	if (klass->link)
		return klass->link (input, output);

	return BURNER_BURN_NOT_SUPPORTED;
}

BurnerTaskItem *
burner_task_item_previous (BurnerTaskItem *item)
{
	BurnerTaskItemIFace *klass;

	g_return_val_if_fail (BURNER_IS_TASK_ITEM (item), NULL);

	klass = BURNER_TASK_ITEM_GET_CLASS (item);
	if (klass->previous)
		return klass->previous (item);

	return NULL;
}

BurnerTaskItem *
burner_task_item_next (BurnerTaskItem *item)
{
	BurnerTaskItemIFace *klass;

	g_return_val_if_fail (BURNER_IS_TASK_ITEM (item), NULL);

	klass = BURNER_TASK_ITEM_GET_CLASS (item);
	if (klass->next)
		return klass->next (item);

	return NULL;
}

gboolean
burner_task_item_is_active (BurnerTaskItem *item)
{
	BurnerTaskItemIFace *klass;

	g_return_val_if_fail (BURNER_IS_TASK_ITEM (item), FALSE);

	klass = BURNER_TASK_ITEM_GET_CLASS (item);
	if (klass->is_active)
		return klass->is_active (item);

	return FALSE;
}

BurnerBurnResult
burner_task_item_activate (BurnerTaskItem *item,
			    BurnerTaskCtx *ctx,
			    GError **error)
{
	BurnerTaskItemIFace *klass;

	g_return_val_if_fail (BURNER_IS_TASK_ITEM (item), BURNER_BURN_ERR);

	klass = BURNER_TASK_ITEM_GET_CLASS (item);
	if (klass->activate)
		return klass->activate (item, ctx, error);

	return BURNER_BURN_NOT_SUPPORTED;
}

BurnerBurnResult
burner_task_item_start (BurnerTaskItem *item,
			 GError **error)
{
	BurnerTaskItemIFace *klass;

	g_return_val_if_fail (BURNER_IS_TASK_ITEM (item), BURNER_BURN_ERR);

	klass = BURNER_TASK_ITEM_GET_CLASS (item);
	if (klass->start)
		return klass->start (item, error);

	return BURNER_BURN_NOT_SUPPORTED;
}

BurnerBurnResult
burner_task_item_clock_tick (BurnerTaskItem *item,
			      BurnerTaskCtx *ctx,
			      GError **error)
{
	BurnerTaskItemIFace *klass;

	g_return_val_if_fail (BURNER_IS_TASK_ITEM (item), BURNER_BURN_ERR);

	klass = BURNER_TASK_ITEM_GET_CLASS (item);
	if (klass->clock_tick)
		return klass->clock_tick (item, ctx, error);

	return BURNER_BURN_NOT_SUPPORTED;
}

BurnerBurnResult
burner_task_item_stop (BurnerTaskItem *item,
			BurnerTaskCtx *ctx,
			GError **error)
{
	BurnerTaskItemIFace *klass;

	g_return_val_if_fail (BURNER_IS_TASK_ITEM (item), BURNER_BURN_ERR);

	klass = BURNER_TASK_ITEM_GET_CLASS (item);
	if (klass->stop)
		return klass->stop (item, ctx, error);

	return BURNER_BURN_NOT_SUPPORTED;
}
