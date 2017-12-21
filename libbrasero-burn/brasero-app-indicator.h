/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/*
 * Libbrasero-burn
 * Copyright (C) Canonical 2010
 *
 * Libbrasero-burn is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * The Libbrasero-burn authors hereby grant permission for non-GPL compatible
 * GStreamer plugins to be used and distributed together with GStreamer
 * and Libbrasero-burn. This permission is above and beyond the permissions granted
 * by the GPL license by which Libbrasero-burn is covered. If you modify this code
 * you may extend this exception to your version of the code, but you are not
 * obligated to do so. If you do not wish to do so, delete this exception
 * statement from your version.
 *
 * Libbrasero-burn is distributed in the hope that it will be useful,
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

#ifndef APP_INDICATOR_H
#define APP_INDICATOR_H

#include <glib.h>
#include <glib-object.h>

#include <gtk/gtk.h>

#include "burn-basics.h"

G_BEGIN_DECLS

#define BRASERO_TYPE_APPINDICATOR         (brasero_app_indicator_get_type ())
#define BRASERO_APPINDICATOR(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), BRASERO_TYPE_APPINDICATOR, BraseroAppIndicator))
#define BRASERO_APPINDICATOR_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k), BRASERO_TYPE_APPINDICATOR, BraseroAppIndicatorClass))
#define BRASERO_IS_APPINDICATOR(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), BRASERO_TYPE_APPINDICATOR))
#define BRASERO_IS_APPINDICATOR_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), BRASERO_TYPE_APPINDICATOR))
#define BRASERO_APPINDICATOR_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), BRASERO_TYPE_APPINDICATOR, BraseroAppIndicatorClass))

typedef struct BraseroAppIndicatorPrivate BraseroAppIndicatorPrivate;

typedef struct {
	GObject parent;
	BraseroAppIndicatorPrivate *priv;
} BraseroAppIndicator;

typedef struct {
	GObjectClass parent_class;

	void		(*show_dialog)		(BraseroAppIndicator *indicator, gboolean show);
	void		(*close_after)		(BraseroAppIndicator *indicator, gboolean close);
	void		(*cancel)		(BraseroAppIndicator *indicator);

} BraseroAppIndicatorClass;

GType brasero_app_indicator_get_type (void);
BraseroAppIndicator *brasero_app_indicator_new (void);

void
brasero_app_indicator_set_progress (BraseroAppIndicator *indicator,
				    gdouble fraction,
				    long remaining);

void
brasero_app_indicator_set_action (BraseroAppIndicator *indicator,
				  BraseroBurnAction action,
				  const gchar *string);

void
brasero_app_indicator_set_show_dialog (BraseroAppIndicator *indicator,
				       gboolean show);

void
brasero_app_indicator_hide (BraseroAppIndicator *indicator);

G_END_DECLS

#endif /* APP_INDICATOR_H */
