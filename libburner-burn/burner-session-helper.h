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

#ifndef _BURN_SESSION_HELPER_H_
#define _BURN_SESSION_HELPER_H_

#include <glib.h>

#include "burner-media.h"
#include "burner-drive.h"

#include "burner-session.h"

G_BEGIN_DECLS


/**
 * Some convenience functions used internally
 */

BurnerBurnResult
burner_caps_session_get_image_flags (BurnerTrackType *input,
                                     BurnerTrackType *output,
                                     BurnerBurnFlag *supported,
                                     BurnerBurnFlag *compulsory);

goffset
burner_burn_session_get_available_medium_space (BurnerBurnSession *session);

BurnerMedia
burner_burn_session_get_dest_media (BurnerBurnSession *session);

BurnerDrive *
burner_burn_session_get_src_drive (BurnerBurnSession *session);

BurnerMedium *
burner_burn_session_get_src_medium (BurnerBurnSession *session);

gboolean
burner_burn_session_is_dest_file (BurnerBurnSession *session);

gboolean
burner_burn_session_same_src_dest_drive (BurnerBurnSession *session);

#define BURNER_BURN_SESSION_EJECT(session)					\
(burner_burn_session_get_flags ((session)) & BURNER_BURN_FLAG_EJECT)

#define BURNER_BURN_SESSION_CHECK_SIZE(session)				\
(burner_burn_session_get_flags ((session)) & BURNER_BURN_FLAG_CHECK_SIZE)

#define BURNER_BURN_SESSION_NO_TMP_FILE(session)				\
(burner_burn_session_get_flags ((session)) & BURNER_BURN_FLAG_NO_TMP_FILES)

#define BURNER_BURN_SESSION_OVERBURN(session)					\
(burner_burn_session_get_flags ((session)) & BURNER_BURN_FLAG_OVERBURN)

#define BURNER_BURN_SESSION_APPEND(session)					\
(burner_burn_session_get_flags ((session)) & (BURNER_BURN_FLAG_APPEND|BURNER_BURN_FLAG_MERGE))

BurnerBurnResult
burner_burn_session_get_tmp_image (BurnerBurnSession *session,
				    BurnerImageFormat format,
				    gchar **image,
				    gchar **toc,
				    GError **error);

BurnerBurnResult
burner_burn_session_get_tmp_file (BurnerBurnSession *session,
				   const gchar *suffix,
				   gchar **path,
				   GError **error);

BurnerBurnResult
burner_burn_session_get_tmp_dir (BurnerBurnSession *session,
				  gchar **path,
				  GError **error);

BurnerBurnResult
burner_burn_session_get_tmp_image_type_same_src_dest (BurnerBurnSession *session,
                                                       BurnerTrackType *image_type);

/**
 * This is to log a session
 * (used internally)
 */

const gchar *
burner_burn_session_get_log_path (BurnerBurnSession *session);

gboolean
burner_burn_session_start (BurnerBurnSession *session);

void
burner_burn_session_stop (BurnerBurnSession *session);

void
burner_burn_session_logv (BurnerBurnSession *session,
			   const gchar *format,
			   va_list arg_list);
void
burner_burn_session_log (BurnerBurnSession *session,
			  const gchar *format,
			  ...);

/**
 * Allow one to save a whole session settings/source and restore it later.
 * (used internally)
 */

void
burner_burn_session_push_settings (BurnerBurnSession *session);
void
burner_burn_session_pop_settings (BurnerBurnSession *session);

void
burner_burn_session_push_tracks (BurnerBurnSession *session);
BurnerBurnResult
burner_burn_session_pop_tracks (BurnerBurnSession *session);


G_END_DECLS

#endif
