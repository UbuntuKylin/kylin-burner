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

#ifndef BURN_CAPS_H
#define BURN_CAPS_H

#include <glib.h>
#include <glib-object.h>

#include "burn-basics.h"
#include "burner-track-type.h"
#include "burner-track-type-private.h"
#include "burner-plugin.h"
#include "burner-plugin-information.h"
#include "burner-plugin-registration.h"

G_BEGIN_DECLS

#define BURNER_TYPE_BURNCAPS         (burner_burn_caps_get_type ())
#define BURNER_BURNCAPS(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), BURNER_TYPE_BURNCAPS, BurnerBurnCaps))
#define BURNER_BURNCAPS_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k), BURNER_TYPE_BURNCAPS, BurnerBurnCapsClass))
#define BURNER_IS_BURNCAPS(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), BURNER_TYPE_BURNCAPS))
#define BURNER_IS_BURNCAPS_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), BURNER_TYPE_BURNCAPS))
#define BURNER_BURNCAPS_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), BURNER_TYPE_BURNCAPS, BurnerBurnCapsClass))

struct _BurnerCaps {
	GSList *links;			/* BurnerCapsLink */
	GSList *modifiers;		/* BurnerPlugin */
	BurnerTrackType type;
	BurnerPluginIOFlag flags;
};
typedef struct _BurnerCaps BurnerCaps;

struct _BurnerCapsLink {
	GSList *plugins;
	BurnerCaps *caps;
};
typedef struct _BurnerCapsLink BurnerCapsLink;

struct _BurnerCapsTest {
	GSList *links;
	BurnerChecksumType type;
};
typedef struct _BurnerCapsTest BurnerCapsTest;

typedef struct BurnerBurnCapsPrivate BurnerBurnCapsPrivate;
struct BurnerBurnCapsPrivate {
	GSList *caps_list;		/* BurnerCaps */
	GSList *tests;			/* BurnerCapsTest */

	GHashTable *groups;

	gchar *group_str;
	guint group_id;
};

typedef struct {
	GObject parent;
	BurnerBurnCapsPrivate *priv;
} BurnerBurnCaps;

typedef struct {
	GObjectClass parent_class;
} BurnerBurnCapsClass;

GType burner_burn_caps_get_type (void);

BurnerBurnCaps *burner_burn_caps_get_default (void);

BurnerPlugin *
burner_caps_link_need_download (BurnerCapsLink *link);

gboolean
burner_caps_link_active (BurnerCapsLink *link,
                          gboolean ignore_plugin_errors);

gboolean
burner_burn_caps_is_input (BurnerBurnCaps *self,
			    BurnerCaps *input);

BurnerCaps *
burner_burn_caps_find_start_caps (BurnerBurnCaps *self,
				   BurnerTrackType *output);

gboolean
burner_caps_is_compatible_type (const BurnerCaps *caps,
				 const BurnerTrackType *type);

BurnerBurnResult
burner_caps_link_check_recorder_flags_for_input (BurnerCapsLink *link,
                                                  BurnerBurnFlag session_flags);

G_END_DECLS

#endif /* BURN_CAPS_H */
