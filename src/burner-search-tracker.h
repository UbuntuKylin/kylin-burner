/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/*
 * burner
 * Copyright (C) Rouquier Philippe 2009 <bonfire-app@wanadoo.fr>
 * 
 * burner is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * burner is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef _BURNER_SEARCH_TRACKER_H_
#define _BURNER_SEARCH_TRACKER_H_

#include <glib-object.h>

G_BEGIN_DECLS

#define BURNER_TYPE_SEARCH_TRACKER             (burner_search_tracker_get_type ())
#define BURNER_SEARCH_TRACKER(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), BURNER_TYPE_SEARCH_TRACKER, BurnerSearchTracker))
#define BURNER_SEARCH_TRACKER_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), BURNER_TYPE_SEARCH_TRACKER, BurnerSearchTrackerClass))
#define BURNER_IS_SEARCH_TRACKER(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BURNER_TYPE_SEARCH_TRACKER))
#define BURNER_IS_SEARCH_TRACKER_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), BURNER_TYPE_SEARCH_TRACKER))
#define BURNER_SEARCH_TRACKER_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), BURNER_TYPE_SEARCH_TRACKER, BurnerSearchTrackerClass))

typedef struct _BurnerSearchTrackerClass BurnerSearchTrackerClass;
typedef struct _BurnerSearchTracker BurnerSearchTracker;

struct _BurnerSearchTrackerClass
{
	GObjectClass parent_class;
};

struct _BurnerSearchTracker
{
	GObject parent_instance;
};

GType burner_search_tracker_get_type (void) G_GNUC_CONST;

G_END_DECLS

#endif /* _BURNER_SEARCH_TRACKER_H_ */
