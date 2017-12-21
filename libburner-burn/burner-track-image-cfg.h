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

#ifndef _BURN_TRACK_IMAGE_CFG_H_
#define _BURN_TRACK_IMAGE_CFG_H_

#include <glib-object.h>

#include <burner-track.h>
#include <burner-track-image.h>

G_BEGIN_DECLS

#define BURNER_TYPE_TRACK_IMAGE_CFG             (burner_track_image_cfg_get_type ())
#define BURNER_TRACK_IMAGE_CFG(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), BURNER_TYPE_TRACK_IMAGE_CFG, BurnerTrackImageCfg))
#define BURNER_TRACK_IMAGE_CFG_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), BURNER_TYPE_TRACK_IMAGE_CFG, BurnerTrackImageCfgClass))
#define BURNER_IS_TRACK_IMAGE_CFG(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BURNER_TYPE_TRACK_IMAGE_CFG))
#define BURNER_IS_TRACK_IMAGE_CFG_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), BURNER_TYPE_TRACK_IMAGE_CFG))
#define BURNER_TRACK_IMAGE_CFG_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), BURNER_TYPE_TRACK_IMAGE_CFG, BurnerTrackImageCfgClass))

typedef struct _BurnerTrackImageCfgClass BurnerTrackImageCfgClass;
typedef struct _BurnerTrackImageCfg BurnerTrackImageCfg;

struct _BurnerTrackImageCfgClass
{
	BurnerTrackImageClass parent_class;
};

struct _BurnerTrackImageCfg
{
	BurnerTrackImage parent_instance;
};

GType burner_track_image_cfg_get_type (void) G_GNUC_CONST;

BurnerTrackImageCfg *
burner_track_image_cfg_new (void);

BurnerBurnResult
burner_track_image_cfg_set_source (BurnerTrackImageCfg *track,
				    const gchar *uri);

BurnerBurnResult
burner_track_image_cfg_force_format (BurnerTrackImageCfg *track,
				      BurnerImageFormat format);

BurnerImageFormat
burner_track_image_cfg_get_forced_format (BurnerTrackImageCfg *track);

G_END_DECLS

#endif /* _BURN_TRACK_IMAGE_CFG_H_ */
