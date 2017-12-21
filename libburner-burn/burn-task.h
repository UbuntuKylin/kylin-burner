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

#ifndef BURN_TASK_H
#define BURN_TASK_H

#include <glib.h>
#include <glib-object.h>

#include "burn-basics.h"
#include "burn-job.h"
#include "burn-task-ctx.h"
#include "burn-task-item.h"

G_BEGIN_DECLS

#define BURNER_TYPE_TASK         (burner_task_get_type ())
#define BURNER_TASK(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), BURNER_TYPE_TASK, BurnerTask))
#define BURNER_TASK_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k), BURNER_TYPE_TASK, BurnerTaskClass))
#define BURNER_IS_TASK(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), BURNER_TYPE_TASK))
#define BURNER_IS_TASK_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), BURNER_TYPE_TASK))
#define BURNER_TASK_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), BURNER_TYPE_TASK, BurnerTaskClass))

typedef struct _BurnerTask BurnerTask;
typedef struct _BurnerTaskClass BurnerTaskClass;

struct _BurnerTask {
	BurnerTaskCtx parent;
};

struct _BurnerTaskClass {
	BurnerTaskCtxClass parent_class;
};

GType burner_task_get_type (void);

BurnerTask *burner_task_new (void);

void
burner_task_add_item (BurnerTask *task, BurnerTaskItem *item);

void
burner_task_reset (BurnerTask *task);

BurnerBurnResult
burner_task_run (BurnerTask *task,
		  GError **error);

BurnerBurnResult
burner_task_check (BurnerTask *task,
		    GError **error);

BurnerBurnResult
burner_task_cancel (BurnerTask *task,
		     gboolean protect);

gboolean
burner_task_is_running (BurnerTask *task);

BurnerBurnResult
burner_task_get_output_type (BurnerTask *task, BurnerTrackType *output);

G_END_DECLS

#endif /* BURN_TASK_H */
