/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/*
 * Libburner-misc
 * Copyright (C) Philippe Rouquier 2005-2009 <bonfire-app@wanadoo.fr>
 *
 * Libburner-misc is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * The Libburner-misc authors hereby grant permission for non-GPL compatible
 * GStreamer plugins to be used and distributed together with GStreamer
 * and Libburner-misc. This permission is above and beyond the permissions granted
 * by the GPL license by which Libburner-burn is covered. If you modify this code
 * you may extend this exception to your version of the code, but you are not
 * obligated to do so. If you do not wish to do so, delete this exception
 * statement from your version.
 * 
 * Libburner-misc is distributed in the hope that it will be useful,
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

#ifndef ASYNC_TASK_MANAGER_H
#define ASYNC_TASK_MANAGER_H

#include <glib.h>
#include <glib-object.h>

#include <gio/gio.h>

G_BEGIN_DECLS

#define BURNER_TYPE_ASYNC_TASK_MANAGER         (burner_async_task_manager_get_type ())
#define BURNER_ASYNC_TASK_MANAGER(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), BURNER_TYPE_ASYNC_TASK_MANAGER, BurnerAsyncTaskManager))
#define BURNER_ASYNC_TASK_MANAGER_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k), BURNER_TYPE_ASYNC_TASK_MANAGER, BurnerAsyncTaskManagerClass))
#define BURNER_IS_ASYNC_TASK_MANAGER(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), BURNER_TYPE_ASYNC_TASK_MANAGER))
#define BURNER_IS_ASYNC_TASK_MANAGER_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), BURNER_TYPE_ASYNC_TASK_MANAGER))
#define BURNER_ASYNC_TASK_MANAGER_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), BURNER_TYPE_ASYNC_TASK_MANAGER, BurnerAsyncTaskManagerClass))

typedef struct BurnerAsyncTaskManagerPrivate BurnerAsyncTaskManagerPrivate;
typedef struct _BurnerAsyncTaskManagerClass BurnerAsyncTaskManagerClass;
typedef struct _BurnerAsyncTaskManager BurnerAsyncTaskManager;

struct _BurnerAsyncTaskManager {
	GObject parent;
	BurnerAsyncTaskManagerPrivate *priv;
};

struct _BurnerAsyncTaskManagerClass {
	GObjectClass parent_class;
};

GType burner_async_task_manager_get_type (void);

typedef enum {
	BURNER_ASYNC_TASK_FINISHED		= 0,
	BURNER_ASYNC_TASK_RESCHEDULE		= 1
} BurnerAsyncTaskResult;

typedef BurnerAsyncTaskResult	(*BurnerAsyncThread)		(BurnerAsyncTaskManager *manager,
								 GCancellable *cancel,
								 gpointer user_data);
typedef void			(*BurnerAsyncDestroy)		(BurnerAsyncTaskManager *manager,
								 gboolean cancelled,
								 gpointer user_data);
typedef gboolean		(*BurnerAsyncFindTask)		(BurnerAsyncTaskManager *manager,
								 gpointer task,
								 gpointer user_data);

struct _BurnerAsyncTaskType {
	BurnerAsyncThread thread;
	BurnerAsyncDestroy destroy;
};
typedef struct _BurnerAsyncTaskType BurnerAsyncTaskType;

typedef enum {
	/* used internally when reschedule */
	BURNER_ASYNC_RESCHEDULE	= 1,

	BURNER_ASYNC_IDLE		= 1 << 1,
	BURNER_ASYNC_NORMAL		= 1 << 2,
	BURNER_ASYNC_URGENT		= 1 << 3
} BurnerAsyncPriority;

gboolean
burner_async_task_manager_queue (BurnerAsyncTaskManager *manager,
				  BurnerAsyncPriority priority,
				  const BurnerAsyncTaskType *type,
				  gpointer data);

gboolean
burner_async_task_manager_foreach_active (BurnerAsyncTaskManager *manager,
					   BurnerAsyncFindTask func,
					   gpointer user_data);
gboolean
burner_async_task_manager_foreach_active_remove (BurnerAsyncTaskManager *manager,
						  BurnerAsyncFindTask func,
						  gpointer user_data);
gboolean
burner_async_task_manager_foreach_unprocessed_remove (BurnerAsyncTaskManager *self,
						       BurnerAsyncFindTask func,
						       gpointer user_data);

gboolean
burner_async_task_manager_find_urgent_task (BurnerAsyncTaskManager *manager,
					     BurnerAsyncFindTask func,
					     gpointer user_data);

G_END_DECLS

#endif /* ASYNC_JOB_MANAGER_H */
