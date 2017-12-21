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

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <stdio.h>

#include <glib.h>

#include "burn-volume.h"
#include "burn-volume-source.h"

#ifndef _BURN_ISO9660_H
#define _BURN_ISO9660_H

G_BEGIN_DECLS

#define ISO9660_BLOCK_SIZE 2048

typedef enum {
	BURNER_ISO_FLAG_NONE	= 0,
	BURNER_ISO_FLAG_RR	= 1
} BurnerIsoFlag;

gboolean
burner_iso9660_is_primary_descriptor (const gchar *buffer,
				       GError **error);

gboolean
burner_iso9660_get_size (const gchar *block,
			  gint64 *nb_blocks,
			  GError **error);

gboolean
burner_iso9660_get_label (const gchar *block,
			   gchar **label,
			   GError **error);

BurnerVolFile *
burner_iso9660_get_contents (BurnerVolSrc *src,
			      const gchar *block,
			      gint64 *nb_blocks,
			      GError **error);

/**
 * Address to -1 for root
 */
GList *
burner_iso9660_get_directory_contents (BurnerVolSrc *vol,
					const gchar *vol_desc,
					gint address,
					GError **error);

BurnerVolFile *
burner_iso9660_get_file (BurnerVolSrc *src,
			  const gchar *path,
			  const gchar *block,
			  GError **error);

G_END_DECLS

#endif /* _BURN_ISO9660_H */

 
