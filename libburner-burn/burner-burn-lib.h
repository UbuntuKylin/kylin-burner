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

#ifndef _BURNER_BURN_LIB_
#define _BURNER_BURN_LIB_

#include <glib.h>

#include <burner-enums.h>
#include <burner-error.h>
#include <burner-track-type.h>

G_BEGIN_DECLS

/**
 * Some needed information about the library
 */

#define LIBBURNER_BURN_VERSION_MAJOR						\
	3
#define LIBBURNER_BURN_VERSION_MINOR						\
	12
#define LIBBURNER_BURN_VERSION_MICRO						\
	1
#define LIBBURNER_BURN_AGE							\
	362


/**
 * Function to start and stop the library
 */

gboolean
burner_burn_library_start (int *argc,
                            char **argv []);

void
burner_burn_library_stop (void);

/**
 * GOptionGroup for apps
 */

GOptionGroup *
burner_burn_library_get_option_group (void);

/**
 * Allows to get some information about capabilities
 */

GSList *
burner_burn_library_get_plugins_list (void);

gboolean
burner_burn_library_can_checksum (void);

BurnerBurnResult
burner_burn_library_input_supported (BurnerTrackType *type);

BurnerMedia
burner_burn_library_get_media_capabilities (BurnerMedia media);

G_END_DECLS

#endif /* _BURNER_BURN_LIB_ */

