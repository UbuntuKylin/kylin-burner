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

#ifndef PROGRESS_H
#define PROGRESS_H

#include <glib.h>
#include <glib-object.h>

#include <gtk/gtk.h>

#include "burn-basics.h"
#include "burner-media.h"

G_BEGIN_DECLS

#define BURNER_TYPE_BURN_PROGRESS         (burner_burn_progress_get_type ())
#define BURNER_BURN_PROGRESS(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), BURNER_TYPE_BURN_PROGRESS, BurnerBurnProgress))
#define BURNER_BURN_PROGRESS_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k), BURNER_TYPE_BURN_PROGRESS, BurnerBurnProgressClass))
#define BURNER_IS_BURN_PROGRESS(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), BURNER_TYPE_BURN_PROGRESS))
#define BURNER_IS_BURN_PROGRESS_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), BURNER_TYPE_BURN_PROGRESS))
#define BURNER_BURN_PROGRESS_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), BURNER_TYPE_BURN_PROGRESS, BurnerBurnProgressClass))

typedef struct BurnerBurnProgressPrivate BurnerBurnProgressPrivate;

typedef struct {
	GtkBox parent;
	BurnerBurnProgressPrivate *priv;
} BurnerBurnProgress;

typedef struct {
	GtkBoxClass parent_class;
} BurnerBurnProgressClass;

GType burner_burn_progress_get_type (void);

GtkWidget *burner_burn_progress_new (void);

void
burner_burn_progress_reset (BurnerBurnProgress *progress);

void
burner_burn_progress_set_status (BurnerBurnProgress *progress,
				  BurnerMedia media,
				  gdouble overall_progress,
				  gdouble action_progress,
				  glong remaining,
				  gint mb_isosize,
				  gint mb_written,
				  gint64 rate);
void
burner_burn_progress_display_session_info (BurnerBurnProgress *progress,
					    glong time,
					    gint64 rate,
					    BurnerMedia media,
					    gint mb_written);

void
burner_burn_progress_set_action (BurnerBurnProgress *progress,
				  BurnerBurnAction action,
				  const gchar *string);

G_END_DECLS

#endif /* PROGRESS_H */
