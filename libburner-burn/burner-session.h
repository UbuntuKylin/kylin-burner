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

#ifndef BURN_SESSION_H
#define BURN_SESSION_H

#include <glib.h>
#include <glib-object.h>

#include <burner-drive.h>

#include <burner-error.h>
#include <burner-status.h>
#include <burner-track.h>

G_BEGIN_DECLS

#define BURNER_TYPE_BURN_SESSION         (burner_burn_session_get_type ())
#define BURNER_BURN_SESSION(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), BURNER_TYPE_BURN_SESSION, BurnerBurnSession))
#define BURNER_BURN_SESSION_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k), BURNER_TYPE_BURN_SESSION, BurnerBurnSessionClass))
#define BURNER_IS_BURN_SESSION(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), BURNER_TYPE_BURN_SESSION))
#define BURNER_IS_BURN_SESSION_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), BURNER_TYPE_BURN_SESSION))
#define BURNER_BURN_SESSION_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), BURNER_TYPE_BURN_SESSION, BurnerBurnSessionClass))

typedef struct _BurnerBurnSession BurnerBurnSession;
typedef struct _BurnerBurnSessionClass BurnerBurnSessionClass;

struct _BurnerBurnSession {
	GObject parent;
};

struct _BurnerBurnSessionClass {
	GObjectClass parent_class;

	/** Virtual functions **/
	BurnerBurnResult	(*set_output_image)	(BurnerBurnSession *session,
							 BurnerImageFormat format,
							 const gchar *image,
							 const gchar *toc);
	BurnerBurnResult	(*get_output_path)	(BurnerBurnSession *session,
							 gchar **image,
							 gchar **toc);
	BurnerImageFormat	(*get_output_format)	(BurnerBurnSession *session);

	/** Signals **/
	void			(*tag_changed)		(BurnerBurnSession *session,
					                 const gchar *tag);
	void			(*track_added)		(BurnerBurnSession *session,
							 BurnerTrack *track);
	void			(*track_removed)	(BurnerBurnSession *session,
							 BurnerTrack *track,
							 guint former_position);
	void			(*track_changed)	(BurnerBurnSession *session,
							 BurnerTrack *track);
	void			(*output_changed)	(BurnerBurnSession *session,
							 BurnerMedium *former_medium);
};

GType burner_burn_session_get_type (void);

BurnerBurnSession *burner_burn_session_new (void);


/**
 * Used to manage tracks for input
 */

BurnerBurnResult
burner_burn_session_add_track (BurnerBurnSession *session,
				BurnerTrack *new_track,
				BurnerTrack *sibling);

BurnerBurnResult
burner_burn_session_move_track (BurnerBurnSession *session,
				 BurnerTrack *track,
				 BurnerTrack *sibling);

BurnerBurnResult
burner_burn_session_remove_track (BurnerBurnSession *session,
				   BurnerTrack *track);

GSList *
burner_burn_session_get_tracks (BurnerBurnSession *session);

/**
 * Get some information about the session
 */

BurnerBurnResult
burner_burn_session_get_status (BurnerBurnSession *session,
				 BurnerStatus *status);

BurnerBurnResult
burner_burn_session_get_size (BurnerBurnSession *session,
			       goffset *blocks,
			       goffset *bytes);

BurnerBurnResult
burner_burn_session_get_input_type (BurnerBurnSession *session,
				     BurnerTrackType *type);

/**
 * This is to set additional arbitrary information
 */

BurnerBurnResult
burner_burn_session_tag_lookup (BurnerBurnSession *session,
				 const gchar *tag,
				 GValue **value);

BurnerBurnResult
burner_burn_session_tag_add (BurnerBurnSession *session,
			      const gchar *tag,
			      GValue *value);

BurnerBurnResult
burner_burn_session_tag_remove (BurnerBurnSession *session,
				 const gchar *tag);

BurnerBurnResult
burner_burn_session_tag_add_int (BurnerBurnSession *self,
                                  const gchar *tag,
                                  gint value);
gint
burner_burn_session_tag_lookup_int (BurnerBurnSession *self,
                                     const gchar *tag);

/**
 * Destination 
 */
BurnerBurnResult
burner_burn_session_get_output_type (BurnerBurnSession *self,
                                      BurnerTrackType *output);

BurnerDrive *
burner_burn_session_get_burner (BurnerBurnSession *session);

void
burner_burn_session_set_burner (BurnerBurnSession *session,
				 BurnerDrive *drive);

BurnerBurnResult
burner_burn_session_set_image_output_full (BurnerBurnSession *session,
					    BurnerImageFormat format,
					    const gchar *image,
					    const gchar *toc);

BurnerBurnResult
burner_burn_session_get_output (BurnerBurnSession *session,
				 gchar **image,
				 gchar **toc);

BurnerBurnResult
burner_burn_session_set_image_output_format (BurnerBurnSession *self,
					    BurnerImageFormat format);

BurnerImageFormat
burner_burn_session_get_output_format (BurnerBurnSession *session);

const gchar *
burner_burn_session_get_label (BurnerBurnSession *session);

void
burner_burn_session_set_label (BurnerBurnSession *session,
				const gchar *label);

BurnerBurnResult
burner_burn_session_set_rate (BurnerBurnSession *session,
			       guint64 rate);

guint64
burner_burn_session_get_rate (BurnerBurnSession *session);

/**
 * Session flags
 */

void
burner_burn_session_set_flags (BurnerBurnSession *session,
			        BurnerBurnFlag flags);

void
burner_burn_session_add_flag (BurnerBurnSession *session,
			       BurnerBurnFlag flags);

void
burner_burn_session_remove_flag (BurnerBurnSession *session,
				  BurnerBurnFlag flags);

BurnerBurnFlag
burner_burn_session_get_flags (BurnerBurnSession *session);


/**
 * Used to deal with the temporary files (mostly used by plugins)
 */

BurnerBurnResult
burner_burn_session_set_tmpdir (BurnerBurnSession *session,
				 const gchar *path);
const gchar *
burner_burn_session_get_tmpdir (BurnerBurnSession *session);

/**
 * Test the supported or compulsory flags for a given session
 */

BurnerBurnResult
burner_burn_session_get_burn_flags (BurnerBurnSession *session,
				     BurnerBurnFlag *supported,
				     BurnerBurnFlag *compulsory);

BurnerBurnResult
burner_burn_session_get_blank_flags (BurnerBurnSession *session,
				      BurnerBurnFlag *supported,
				      BurnerBurnFlag *compulsory);

/**
 * Used to test the possibilities offered for a given session
 */

void
burner_burn_session_set_strict_support (BurnerBurnSession *session,
                                         gboolean strict_check);

gboolean
burner_burn_session_get_strict_support (BurnerBurnSession *session);

BurnerBurnResult
burner_burn_session_can_blank (BurnerBurnSession *session);

BurnerBurnResult
burner_burn_session_can_burn (BurnerBurnSession *session,
                               gboolean check_flags);

typedef BurnerBurnResult	(* BurnerForeachPluginErrorCb)	(BurnerPluginErrorType type,
		                                                 const gchar *detail,
		                                                 gpointer user_data);

BurnerBurnResult
burner_session_foreach_plugin_error (BurnerBurnSession *session,
                                      BurnerForeachPluginErrorCb callback,
                                      gpointer user_data);

BurnerBurnResult
burner_burn_session_input_supported (BurnerBurnSession *session,
				      BurnerTrackType *input,
                                      gboolean check_flags);

BurnerBurnResult
burner_burn_session_output_supported (BurnerBurnSession *session,
				       BurnerTrackType *output);

BurnerMedia
burner_burn_session_get_required_media_type (BurnerBurnSession *session);

guint
burner_burn_session_get_possible_output_formats (BurnerBurnSession *session,
						  BurnerImageFormat *formats);

BurnerImageFormat
burner_burn_session_get_default_output_format (BurnerBurnSession *session);


G_END_DECLS

#endif /* BURN_SESSION_H */
