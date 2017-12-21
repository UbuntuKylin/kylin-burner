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

#ifndef BURN_LIBBURN_COMMON_H
#define BURN_LIBBURN_COMMON_H

#include <glib.h>
#include <glib-object.h>

#include "burn-job.h"

#include <libburn/libburn.h>

G_BEGIN_DECLS

struct _BurnerLibburnCtx {
	struct burn_drive_info *drive_info;
	struct burn_drive *drive;
	struct burn_disc *disc;

	enum burn_drive_status status;

	/* used detect track hops */
	gint track_num;

	/* used to report current written sector */
	gint64 sectors;
	gint64 cur_sector;
	gint64 track_sectors;

	GTimer *op_start;

	guint is_burning:1;
	guint has_leadin:1;
};
typedef struct _BurnerLibburnCtx BurnerLibburnCtx;

BurnerLibburnCtx *
burner_libburn_common_ctx_new (BurnerJob *job,
                                gboolean is_burning,
				GError **error);

void
burner_libburn_common_ctx_free (BurnerLibburnCtx *ctx);

BurnerBurnResult
burner_libburn_common_status (BurnerJob *job,
			       BurnerLibburnCtx *ctx);

G_END_DECLS

#endif /* BURN_LIBBURN_COMMON_H */
