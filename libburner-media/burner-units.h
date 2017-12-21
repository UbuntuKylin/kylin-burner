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
 
#ifndef _BURNER_UNITS_H_
#define _BURNER_UNITS_H_

#include <glib.h>

G_BEGIN_DECLS

/* Data Transfer Speeds: rates are in KiB/sec */
/* NOTE: rates for audio and data transfer speeds are different:
 * - Data : 150 KiB/sec
 * - Audio : 172.3 KiB/sec
 * Source Wikipedia.com =)
 * Apparently most drives return rates that should be used with Audio factor
 */

#define CD_RATE 176400		/* bytes by second */
#define DVD_RATE 1387500	/* bytes by second */
#define BD_RATE 4500000		/* bytes by second */

#define BURNER_SPEED_TO_RATE_CD(speed)						\
	(guint) ((speed) * CD_RATE)

#define BURNER_SPEED_TO_RATE_DVD(speed)					\
	(guint) ((speed) * DVD_RATE)

#define BURNER_SPEED_TO_RATE_BD(speed)						\
	(guint) ((speed) * BD_RATE)

#define BURNER_RATE_TO_SPEED_CD(rate)						\
	(gdouble) ((gdouble) (rate) / (gdouble) CD_RATE)

#define BURNER_RATE_TO_SPEED_DVD(rate)						\
	(gdouble) ((gdouble) (rate) / (gdouble) DVD_RATE)

#define BURNER_RATE_TO_SPEED_BD(rate)						\
	(gdouble) ((gdouble) (rate) / (gdouble) BD_RATE)


/**
 * Used to convert between known units
 **/

#define BURNER_DURATION_TO_BYTES(duration)					\
	((gint64) (duration) * 75LL * 2352LL / 1000000000LL +				\
	(((gint64) ((duration) * 75LL * 2352LL) % 1000000000LL) ? 1:0))

#define BURNER_DURATION_TO_SECTORS(duration)					\
	((gint64) (duration) * 75LL / 1000000000LL +				\
	(((gint64) ((duration) * 75LL) % 1000000000LL) ? 1:0))

#define BURNER_BYTES_TO_SECTORS(size, secsize)					\
	(((size) / (secsize)) + (((size) % (secsize)) ? 1:0))

#define BURNER_BYTES_TO_DURATION(bytes)					\
	(guint64) ((guint64) ((guint64) (bytes) * 1000000000LL) / (guint64) (2352LL * 75LL) + 				\
	(guint64) (((guint64) ((guint64) (bytes) * 1000000000LL) % (guint64) (2352LL * 75LL)) ? 1:0))

/* NOTE: 1 sec = 75 sectors */

/**
 * BURNER_SECTORS_TO_DURATION:
 * This macro is used to convert sector sizes in
 * in length (in nanoseconds).
 *
**/
#define BURNER_SECTORS_TO_DURATION(sectors)	\
	(guint64) ((sectors) * 1000000000LL / 75LL)

/**
 * Used to get string
 */

gchar *
burner_units_get_time_string (guint64 time,
			       gboolean with_unit,
			       gboolean round);

gchar *
burner_units_get_time_string_from_size (gint64 size,
					 gboolean with_unit,
					 gboolean round);

G_END_DECLS

#endif /* _BURNER_UNITS_H_ */

 
