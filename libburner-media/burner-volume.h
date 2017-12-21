/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/*
 * Burner
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

#ifndef _BURNER_VOLUME_H_
#define _BURNER_VOLUME_H_

#include <glib-object.h>
#include <gio/gio.h>
#include <gtk/gtk.h>
#include <burner-drive.h>

G_BEGIN_DECLS

#define BURNER_TYPE_VOLUME             (burner_volume_get_type ())
#define BURNER_VOLUME(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), BURNER_TYPE_VOLUME, BurnerVolume))
#define BURNER_VOLUME_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), BURNER_TYPE_VOLUME, BurnerVolumeClass))
#define BURNER_IS_VOLUME(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BURNER_TYPE_VOLUME))
#define BURNER_IS_VOLUME_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), BURNER_TYPE_VOLUME))
#define BURNER_VOLUME_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), BURNER_TYPE_VOLUME, BurnerVolumeClass))

typedef struct _BurnerVolumeClass BurnerVolumeClass;
typedef struct _BurnerVolume BurnerVolume;

struct _BurnerVolumeClass
{
	BurnerMediumClass parent_class;
};

struct _BurnerVolume
{
	BurnerMedium parent_instance;
};

GType burner_volume_get_type (void) G_GNUC_CONST;

gchar *
burner_volume_get_name (BurnerVolume *volume);

GIcon *
burner_volume_get_icon (BurnerVolume *volume);

GVolume *
burner_volume_get_gvolume (BurnerVolume *volume);

gboolean
burner_volume_is_mounted (BurnerVolume *volume);

gchar *
burner_volume_get_mount_point (BurnerVolume *volume,
				GError **error);

gboolean
burner_volume_umount (BurnerVolume *volume,
		       gboolean wait,
		       GError **error);

gboolean
burner_volume_mount (BurnerVolume *volume,
		      GtkWindow *parent_window,
		      gboolean wait,
		      GError **error);

void
burner_volume_cancel_current_operation (BurnerVolume *volume);

G_END_DECLS

#endif /* _BURNER_VOLUME_H_ */
