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

#ifndef _BURN_TASK_CTX_H_
#define _BURN_TASK_CTX_H_

#include <glib-object.h>

#include "burn-basics.h"
#include "burner-session.h"

G_BEGIN_DECLS

#define BURNER_TYPE_TASK_CTX             (burner_task_ctx_get_type ())
#define BURNER_TASK_CTX(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), BURNER_TYPE_TASK_CTX, BurnerTaskCtx))
#define BURNER_TASK_CTX_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), BURNER_TYPE_TASK_CTX, BurnerTaskCtxClass))
#define BURNER_IS_TASK_CTX(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BURNER_TYPE_TASK_CTX))
#define BURNER_IS_TASK_CTX_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), BURNER_TYPE_TASK_CTX))
#define BURNER_TASK_CTX_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), BURNER_TYPE_TASK_CTX, BurnerTaskCtxClass))

typedef enum {
	BURNER_TASK_ACTION_NONE		= 0,
	BURNER_TASK_ACTION_ERASE,
	BURNER_TASK_ACTION_NORMAL,
	BURNER_TASK_ACTION_CHECKSUM,
} BurnerTaskAction;

typedef struct _BurnerTaskCtxClass BurnerTaskCtxClass;
typedef struct _BurnerTaskCtx BurnerTaskCtx;

struct _BurnerTaskCtxClass
{
	GObjectClass parent_class;

	void			(* finished)		(BurnerTaskCtx *ctx,
							 BurnerBurnResult retval,
							 GError *error);

	/* signals */
	void			(*progress_changed)	(BurnerTaskCtx *task,
							 gdouble fraction,
							 glong remaining_time);
	void			(*action_changed)	(BurnerTaskCtx *task,
							 BurnerBurnAction action);
};

struct _BurnerTaskCtx
{
	GObject parent_instance;
};

GType burner_task_ctx_get_type (void) G_GNUC_CONST;

void
burner_task_ctx_reset (BurnerTaskCtx *ctx);

void
burner_task_ctx_set_fake (BurnerTaskCtx *ctx,
			   gboolean fake);

void
burner_task_ctx_set_dangerous (BurnerTaskCtx *ctx,
				gboolean value);

guint
burner_task_ctx_get_dangerous (BurnerTaskCtx *ctx);

/**
 * Used to get the session it is associated with
 */

BurnerBurnSession *
burner_task_ctx_get_session (BurnerTaskCtx *ctx);

BurnerTaskAction
burner_task_ctx_get_action (BurnerTaskCtx *ctx);

BurnerBurnResult
burner_task_ctx_get_stored_tracks (BurnerTaskCtx *ctx,
				    GSList **tracks);

BurnerBurnResult
burner_task_ctx_get_current_track (BurnerTaskCtx *ctx,
				    BurnerTrack **track);

/**
 * Used to give job results and tell when a job has finished
 */

BurnerBurnResult
burner_task_ctx_add_track (BurnerTaskCtx *ctx,
			    BurnerTrack *track);

BurnerBurnResult
burner_task_ctx_next_track (BurnerTaskCtx *ctx);

BurnerBurnResult
burner_task_ctx_finished (BurnerTaskCtx *ctx);

BurnerBurnResult
burner_task_ctx_error (BurnerTaskCtx *ctx,
			BurnerBurnResult retval,
			GError *error);

/**
 * Used to start progress reporting and starts an internal timer to keep track
 * of remaining time among other things
 */

BurnerBurnResult
burner_task_ctx_start_progress (BurnerTaskCtx *ctx,
				 gboolean force);

void
burner_task_ctx_report_progress (BurnerTaskCtx *ctx);

void
burner_task_ctx_stop_progress (BurnerTaskCtx *ctx);

/**
 * task progress report for jobs
 */

BurnerBurnResult
burner_task_ctx_set_rate (BurnerTaskCtx *ctx,
			   gint64 rate);

BurnerBurnResult
burner_task_ctx_set_written_session (BurnerTaskCtx *ctx,
				      gint64 written);
BurnerBurnResult
burner_task_ctx_set_written_track (BurnerTaskCtx *ctx,
				    gint64 written);
BurnerBurnResult
burner_task_ctx_reset_progress (BurnerTaskCtx *ctx);
BurnerBurnResult
burner_task_ctx_set_progress (BurnerTaskCtx *ctx,
			       gdouble progress);
BurnerBurnResult
burner_task_ctx_set_current_action (BurnerTaskCtx *ctx,
				     BurnerBurnAction action,
				     const gchar *string,
				     gboolean force);
BurnerBurnResult
burner_task_ctx_set_use_average (BurnerTaskCtx *ctx,
				  gboolean use_average);
BurnerBurnResult
burner_task_ctx_set_output_size_for_current_track (BurnerTaskCtx *ctx,
						    goffset sectors,
						    goffset bytes);

/**
 * task progress for library
 */

BurnerBurnResult
burner_task_ctx_get_rate (BurnerTaskCtx *ctx,
			   guint64 *rate);
BurnerBurnResult
burner_task_ctx_get_remaining_time (BurnerTaskCtx *ctx,
				     long *remaining);
BurnerBurnResult
burner_task_ctx_get_session_output_size (BurnerTaskCtx *ctx,
					  goffset *blocks,
					  goffset *bytes);
BurnerBurnResult
burner_task_ctx_get_written (BurnerTaskCtx *ctx,
			      goffset *written);
BurnerBurnResult
burner_task_ctx_get_current_action_string (BurnerTaskCtx *ctx,
					    BurnerBurnAction action,
					    gchar **string);
BurnerBurnResult
burner_task_ctx_get_progress (BurnerTaskCtx *ctx, 
			       gdouble *progress);
BurnerBurnResult
burner_task_ctx_get_current_action (BurnerTaskCtx *ctx,
				     BurnerBurnAction *action);

G_END_DECLS

#endif /* _BURN_TASK_CTX_H_ */
