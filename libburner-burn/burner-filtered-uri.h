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

#ifndef _BURNER_FILTERED_URI_H_
#define _BURNER_FILTERED_URI_H_

#include <glib-object.h>

#include <gtk/gtk.h>

G_BEGIN_DECLS

typedef enum {
	BURNER_FILTER_NONE			= 0,
	BURNER_FILTER_HIDDEN			= 1,
	BURNER_FILTER_UNREADABLE,
	BURNER_FILTER_BROKEN_SYM,
	BURNER_FILTER_RECURSIVE_SYM,
	BURNER_FILTER_UNKNOWN,
} BurnerFilterStatus;

#define BURNER_TYPE_FILTERED_URI             (burner_filtered_uri_get_type ())
#define BURNER_FILTERED_URI(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), BURNER_TYPE_FILTERED_URI, BurnerFilteredUri))
#define BURNER_FILTERED_URI_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), BURNER_TYPE_FILTERED_URI, BurnerFilteredUriClass))
#define BURNER_IS_FILTERED_URI(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BURNER_TYPE_FILTERED_URI))
#define BURNER_IS_FILTERED_URI_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), BURNER_TYPE_FILTERED_URI))
#define BURNER_FILTERED_URI_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), BURNER_TYPE_FILTERED_URI, BurnerFilteredUriClass))

typedef struct _BurnerFilteredUriClass BurnerFilteredUriClass;
typedef struct _BurnerFilteredUri BurnerFilteredUri;

struct _BurnerFilteredUriClass
{
	GtkListStoreClass parent_class;
};

struct _BurnerFilteredUri
{
	GtkListStore parent_instance;
};

GType burner_filtered_uri_get_type (void) G_GNUC_CONST;

BurnerFilteredUri *
burner_filtered_uri_new (void);

gchar *
burner_filtered_uri_restore (BurnerFilteredUri *filtered,
			      GtkTreePath *treepath);

BurnerFilterStatus
burner_filtered_uri_lookup_restored (BurnerFilteredUri *filtered,
				      const gchar *uri);

GSList *
burner_filtered_uri_get_restored_list (BurnerFilteredUri *filtered);

void
burner_filtered_uri_filter (BurnerFilteredUri *filtered,
			     const gchar *uri,
			     BurnerFilterStatus status);
void
burner_filtered_uri_dont_filter (BurnerFilteredUri *filtered,
				  const gchar *uri);

void
burner_filtered_uri_clear (BurnerFilteredUri *filtered);

void
burner_filtered_uri_remove_with_children (BurnerFilteredUri *filtered,
					   const gchar *uri);

G_END_DECLS

#endif /* _BURNER_FILTERED_URI_H_ */
