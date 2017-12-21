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

#include <glib.h>
#include <gio/gio.h>

#ifndef _BURN_XFER_H
#define _BURN_XFER_H

G_BEGIN_DECLS

typedef struct _BurnerXferCtx BurnerXferCtx;

BurnerXferCtx *
burner_xfer_new (void);

void
burner_xfer_free (BurnerXferCtx *ctx);

gboolean
burner_xfer_start (BurnerXferCtx *ctx,
		    GFile *src,
		    GFile *dest,
		    GCancellable *cancel,
		    GError **error);

gboolean
burner_xfer_wait (BurnerXferCtx *ctx,
		   const gchar *src,
		   const gchar *dest,
		   GCancellable *cancel,
		   GError **error);

gboolean
burner_xfer_get_progress (BurnerXferCtx *ctx,
			   goffset *written,
			   goffset *total);

G_END_DECLS

#endif /* _BURN_XFER_H */

 
