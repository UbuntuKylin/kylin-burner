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

#ifndef BURN_H
#define BURN_H

#include <glib.h>
#include <glib-object.h>

#include <burner-error.h>
#include <burner-track.h>
#include <burner-session.h>

#include <burner-medium.h>

G_BEGIN_DECLS

#define BURNER_TYPE_BURN         (burner_burn_get_type ())
#define BURNER_BURN(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), BURNER_TYPE_BURN, BurnerBurn))
#define BURNER_BURN_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k), BURNER_TYPE_BURN, BurnerBurnClass))
#define BURNER_IS_BURN(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), BURNER_TYPE_BURN))
#define BURNER_IS_BURN_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), BURNER_TYPE_BURN))
#define BURNER_BURN_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), BURNER_TYPE_BURN, BurnerBurnClass))

typedef struct {
	GObject parent;
} BurnerBurn;

typedef struct {
	GObjectClass parent_class;

	/* signals */
	BurnerBurnResult		(*insert_media_request)		(BurnerBurn *obj,
									 BurnerDrive *drive,
									 BurnerBurnError error,
									 BurnerMedia required_media);

	BurnerBurnResult		(*eject_failure)		(BurnerBurn *obj,
							                 BurnerDrive *drive);

	BurnerBurnResult		(*blank_failure)		(BurnerBurn *obj);

	BurnerBurnResult		(*location_request)		(BurnerBurn *obj,
									 GError *error,
									 gboolean is_temporary);

	BurnerBurnResult		(*ask_disable_joliet)		(BurnerBurn *obj);

	BurnerBurnResult		(*warn_data_loss)		(BurnerBurn *obj);
	BurnerBurnResult		(*warn_previous_session_loss)	(BurnerBurn *obj);
	BurnerBurnResult		(*warn_audio_to_appendable)	(BurnerBurn *obj);
	BurnerBurnResult		(*warn_rewritable)		(BurnerBurn *obj);

	BurnerBurnResult		(*dummy_success)		(BurnerBurn *obj);

	void				(*progress_changed)		(BurnerBurn *obj,
									 gdouble overall_progress,
									 gdouble action_progress,
									 glong time_remaining);
	void				(*action_changed)		(BurnerBurn *obj,
									 BurnerBurnAction action);

	BurnerBurnResult		(*install_missing)		(BurnerBurn *obj,
									 BurnerPluginErrorType error,
									 const gchar *detail);
} BurnerBurnClass;

GType burner_burn_get_type (void);
BurnerBurn *burner_burn_new (void);

BurnerBurnResult 
burner_burn_record (BurnerBurn *burn,
		     BurnerBurnSession *session,
		     GError **error);

BurnerBurnResult
burner_burn_check (BurnerBurn *burn,
		    BurnerBurnSession *session,
		    GError **error);

BurnerBurnResult
burner_burn_blank (BurnerBurn *burn,
		    BurnerBurnSession *session,
		    GError **error);

BurnerBurnResult
burner_burn_cancel (BurnerBurn *burn,
		     gboolean protect);

BurnerBurnResult
burner_burn_status (BurnerBurn *burn,
		     BurnerMedia *media,
		     goffset *isosize,
		     goffset *written,
		     guint64 *rate);

void
burner_burn_get_action_string (BurnerBurn *burn,
				BurnerBurnAction action,
				gchar **string);

G_END_DECLS

#endif /* BURN_H */
