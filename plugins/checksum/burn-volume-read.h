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
 
#ifndef _BURNER_MEDIUM_HANDLE_H
#define BURNER_MEDIUM_HANDLE_H

#include <glib.h>

#include "burn-basics.h"
#include "burn-volume-source.h"

G_BEGIN_DECLS

typedef struct _BurnerVolFileHandle BurnerVolFileHandle;


BurnerVolFileHandle *
burner_volume_file_open (BurnerVolSrc *src,
			  BurnerVolFile *file);

void
burner_volume_file_close (BurnerVolFileHandle *handle);

gboolean
burner_volume_file_rewind (BurnerVolFileHandle *handle);

gint
burner_volume_file_read (BurnerVolFileHandle *handle,
			  gchar *buffer,
			  guint len);

BurnerBurnResult
burner_volume_file_read_line (BurnerVolFileHandle *handle,
			       gchar *buffer,
			       guint len);

BurnerVolFileHandle *
burner_volume_file_open_direct (BurnerVolSrc *src,
				 BurnerVolFile *file);

gint64
burner_volume_file_read_direct (BurnerVolFileHandle *handle,
				 guchar *buffer,
				 guint blocks);

G_END_DECLS

#endif /* BURNER_MEDIUM_HANDLE_H */

 
