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

#ifndef _BURNER_STATUS_H_
#define _BURNER_STATUS_H_

#include <glib.h>
#include <glib-object.h>

#include <burner-enums.h>

G_BEGIN_DECLS

#define BURNER_TYPE_STATUS             (burner_status_get_type ())
#define BURNER_STATUS(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), BURNER_TYPE_STATUS, BurnerStatus))
#define BURNER_STATUS_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), BURNER_TYPE_STATUS, BurnerStatusClass))
#define BURNER_IS_STATUS(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BURNER_TYPE_STATUS))
#define BURNER_IS_STATUS_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), BURNER_TYPE_STATUS))
#define BURNER_STATUS_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), BURNER_TYPE_STATUS, BurnerStatusClass))

typedef struct _BurnerStatusClass BurnerStatusClass;
typedef struct _BurnerStatus BurnerStatus;

struct _BurnerStatusClass
{
	GObjectClass parent_class;
};

struct _BurnerStatus
{
	GObject parent_instance;
};

GType burner_status_get_type (void) G_GNUC_CONST;

BurnerStatus *
burner_status_new (void);

typedef enum {
	BURNER_STATUS_OK			= 0,
	BURNER_STATUS_ERROR,
	BURNER_STATUS_QUESTION,
	BURNER_STATUS_INFORMATION
} BurnerStatusType;

BurnerBurnResult
burner_status_get_result (BurnerStatus *status);

gdouble
burner_status_get_progress (BurnerStatus *status);

GError *
burner_status_get_error (BurnerStatus *status);

gchar *
burner_status_get_current_action (BurnerStatus *status);

void
burner_status_set_completed (BurnerStatus *status);

void
burner_status_set_not_ready (BurnerStatus *status,
			      gdouble progress,
			      const gchar *current_action);

void
burner_status_set_running (BurnerStatus *status,
			    gdouble progress,
			    const gchar *current_action);

void
burner_status_set_error (BurnerStatus *status,
			  GError *error);

G_END_DECLS

#endif
