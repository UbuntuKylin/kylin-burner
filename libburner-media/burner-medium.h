/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/*
 * Libburner-media
 * Copyright (C) Philippe Rouquier 2005-2009 <bonfire-app@wanadoo.fr>
 *
 * Libburner-media is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * The Libburner-media authors hereby grant permission for non-GPL compatible
 * GStreamer plugins to be used and distributed together with GStreamer
 * and Libburner-media. This permission is above and beyond the permissions granted
 * by the GPL license by which Libburner-media is covered. If you modify this code
 * you may extend this exception to your version of the code, but you are not
 * obligated to do so. If you do not wish to do so, delete this exception
 * statement from your version.
 * 
 * Libburner-media is distributed in the hope that it will be useful,
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

#include <glib-object.h>

#include <burner-media.h>

#ifndef _BURN_MEDIUM_H_
#define _BURN_MEDIUM_H_

G_BEGIN_DECLS

#define BURNER_TYPE_MEDIUM             (burner_medium_get_type ())
#define BURNER_MEDIUM(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), BURNER_TYPE_MEDIUM, BurnerMedium))
#define BURNER_MEDIUM_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), BURNER_TYPE_MEDIUM, BurnerMediumClass))
#define BURNER_IS_MEDIUM(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BURNER_TYPE_MEDIUM))
#define BURNER_IS_MEDIUM_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), BURNER_TYPE_MEDIUM))
#define BURNER_MEDIUM_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), BURNER_TYPE_MEDIUM, BurnerMediumClass))

typedef struct _BurnerMediumClass BurnerMediumClass;

/**
 * BurnerMedium:
 *
 * Represents a physical medium currently inserted in a #BurnerDrive.
 **/
typedef struct _BurnerMedium BurnerMedium;

/**
 * BurnerDrive:
 *
 * Represents a physical drive currently connected.
 **/
typedef struct _BurnerDrive BurnerDrive;

struct _BurnerMediumClass
{
	GObjectClass parent_class;
};

struct _BurnerMedium
{
	GObject parent_instance;
};

GType burner_medium_get_type (void) G_GNUC_CONST;


BurnerMedia
burner_medium_get_status (BurnerMedium *medium);

guint64
burner_medium_get_max_write_speed (BurnerMedium *medium);

guint64 *
burner_medium_get_write_speeds (BurnerMedium *medium);

void
burner_medium_get_free_space (BurnerMedium *medium,
			       goffset *bytes,
			       goffset *blocks);

void
burner_medium_get_capacity (BurnerMedium *medium,
			     goffset *bytes,
			     goffset *blocks);

void
burner_medium_get_data_size (BurnerMedium *medium,
			      goffset *bytes,
			      goffset *blocks);

gint64
burner_medium_get_next_writable_address (BurnerMedium *medium);

gboolean
burner_medium_can_be_rewritten (BurnerMedium *medium);

gboolean
burner_medium_can_be_written (BurnerMedium *medium);

const gchar *
burner_medium_get_CD_TEXT_title (BurnerMedium *medium);

const gchar *
burner_medium_get_type_string (BurnerMedium *medium);

gchar *
burner_medium_get_tooltip (BurnerMedium *medium);

BurnerDrive *
burner_medium_get_drive (BurnerMedium *medium);

guint
burner_medium_get_track_num (BurnerMedium *medium);

gboolean
burner_medium_get_last_data_track_space (BurnerMedium *medium,
					  goffset *bytes,
					  goffset *sectors);

gboolean
burner_medium_get_last_data_track_address (BurnerMedium *medium,
					    goffset *bytes,
					    goffset *sectors);

gboolean
burner_medium_get_track_space (BurnerMedium *medium,
				guint num,
				goffset *bytes,
				goffset *sectors);

gboolean
burner_medium_get_track_address (BurnerMedium *medium,
				  guint num,
				  goffset *bytes,
				  goffset *sectors);

gboolean
burner_medium_can_use_dummy_for_sao (BurnerMedium *medium);

gboolean
burner_medium_can_use_dummy_for_tao (BurnerMedium *medium);

gboolean
burner_medium_can_use_burnfree (BurnerMedium *medium);

gboolean
burner_medium_can_use_sao (BurnerMedium *medium);

gboolean
burner_medium_can_use_tao (BurnerMedium *medium);

G_END_DECLS

#endif /* _BURN_MEDIUM_H_ */
