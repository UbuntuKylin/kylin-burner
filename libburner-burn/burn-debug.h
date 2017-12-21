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
 
#ifndef _BURN_DEBUG_H
#define _BURN_DEBUG_H

#include <glib.h>
#include <gmodule.h>

#include "burner-medium.h"

#include "burner-track.h"
#include "burner-track-type-private.h"
#include "burner-plugin-registration.h"

G_BEGIN_DECLS

#define BURNER_BURN_LOG(format, ...)						\
		burner_burn_debug_message (G_STRLOC,				\
					    format,				\
					    ##__VA_ARGS__);

#define BURNER_BURN_LOGV(format, args_list)			\
		burner_burn_debug_messagev (G_STRLOC,		\
					     format,		\
					     args_list);

#define BURNER_BURN_LOG_DISC_TYPE(media_MACRO, format, ...)	\
		BURNER_BURN_LOG_WITH_FULL_TYPE (BURNER_TRACK_TYPE_DISC,	\
						 media_MACRO,	\
						 BURNER_PLUGIN_IO_NONE,	\
						 format,			\
						 ##__VA_ARGS__);

#define BURNER_BURN_LOG_FLAGS(flags_MACRO, format, ...)			\
		burner_burn_debug_flags_type_message (flags_MACRO,		\
						       G_STRLOC,		\
						       format,			\
						       ##__VA_ARGS__);

#define BURNER_BURN_LOG_TYPE(type_MACRO, format, ...)						\
		burner_burn_debug_track_type_struct_message (type_MACRO,			\
							      BURNER_PLUGIN_IO_NONE,		\
							      G_STRLOC,				\
							      format,				\
							      ##__VA_ARGS__);

#define BURNER_BURN_LOG_WITH_TYPE(type_MACRO, flags_MACRO, format, ...)	\
		BURNER_BURN_LOG_WITH_FULL_TYPE ((type_MACRO)->type,		\
						 (type_MACRO)->subtype.media,	\
						 flags_MACRO,			\
						 format,			\
						 ##__VA_ARGS__);

#define BURNER_BURN_LOG_WITH_FULL_TYPE(type_MACRO, subtype_MACRO, flags_MACRO, format, ...)	\
		burner_burn_debug_track_type_message ((type_MACRO),				\
						       (subtype_MACRO),				\
						       (flags_MACRO),				\
						       G_STRLOC,				\
						       format,					\
						       ##__VA_ARGS__);

void
burner_burn_library_set_debug (gboolean value);

void
burner_burn_debug_setup_module (GModule *handle);

void
burner_burn_debug_track_type_struct_message (BurnerTrackType *type,
					      BurnerPluginIOFlag flags,
					      const gchar *location,
					      const gchar *format,
					      ...);
void
burner_burn_debug_track_type_message (BurnerTrackDataType type,
				       guint subtype,
				       BurnerPluginIOFlag flags,
				       const gchar *location,
				       const gchar *format,
				       ...);
void
burner_burn_debug_flags_type_message (BurnerBurnFlag flags,
				       const gchar *location,
				       const gchar *format,
				       ...);
void
burner_burn_debug_message (const gchar *location,
			    const gchar *format,
			    ...);

void
burner_burn_debug_messagev (const gchar *location,
			     const gchar *format,
			     va_list args);

G_END_DECLS

#endif /* _BURN_DEBUG_H */

 
