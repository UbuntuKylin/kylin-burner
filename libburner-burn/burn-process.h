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

#ifndef PROCESS_H
#define PROCESS_H

#include <glib.h>
#include <glib-object.h>

#include "burn-job.h"

G_BEGIN_DECLS

#define BURNER_TYPE_PROCESS         (burner_process_get_type ())
#define BURNER_PROCESS(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), BURNER_TYPE_PROCESS, BurnerProcess))
#define BURNER_PROCESS_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k), BURNER_TYPE_PROCESS, BurnerProcessClass))
#define BURNER_IS_PROCESS(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), BURNER_TYPE_PROCESS))
#define BURNER_IS_PROCESS_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), BURNER_TYPE_PROCESS))
#define BURNER_PROCESS_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), BURNER_TYPE_PROCESS, BurnerProcessClass))

typedef struct {
	BurnerJob parent;
} BurnerProcess;

typedef struct {
	BurnerJobClass parent_class;

	/* virtual functions */
	BurnerBurnResult	(*stdout_func)	(BurnerProcess *process,
						 const gchar *line);
	BurnerBurnResult	(*stderr_func)	(BurnerProcess *process,
						 const gchar *line);
	BurnerBurnResult	(*set_argv)	(BurnerProcess *process,
						 GPtrArray *argv,
						 GError **error);

	/* since burn-process.c doesn't know if it should call finished_session
	 * of finished track this allows one to override the default call which is
	 * burner_job_finished_track */
	BurnerBurnResult      	(*post)       	(BurnerJob *job);
} BurnerProcessClass;

GType burner_process_get_type (void);

/**
 * This function allows one to set an error that is used if the process doesn't 
 * return 0.
 */
void
burner_process_deferred_error (BurnerProcess *process,
				GError *error);

void
burner_process_set_working_directory (BurnerProcess *process,
				       const gchar *directory);

G_END_DECLS

#endif /* PROCESS_H */
