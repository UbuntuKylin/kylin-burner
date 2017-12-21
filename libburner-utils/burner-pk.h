/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/*
 * Libburner-burn
 * Copyright (C) Luis Medinas 2008 <lmedinas@gmail.com>
 * Copyright (C) Philippe Rouquier 2008 <bonfire-app@wanadoo.fr>
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

#ifndef _BURNER_PK_H_
#define _BURNER_PK_H_

#include <glib-object.h>

G_BEGIN_DECLS

#define BURNER_TYPE_PK             (burner_pk_get_type ())
#define BURNER_PK(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), BURNER_TYPE_PK, BurnerPK))
#define BURNER_PK_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), BURNER_TYPE_PK, BurnerPKClass))
#define BURNER_IS_PK(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BURNER_TYPE_PK))
#define BURNER_IS_PK_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), BURNER_TYPE_PK))
#define BURNER_PK_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), BURNER_TYPE_PK, BurnerPKClass))

typedef struct _BurnerPKClass BurnerPKClass;
typedef struct _BurnerPK BurnerPK;

struct _BurnerPKClass
{
	GObjectClass parent_class;
};

struct _BurnerPK
{
	GObject parent_instance;
};

GType burner_pk_get_type (void) G_GNUC_CONST;

BurnerPK *burner_pk_new (void);

gboolean
burner_pk_install_gstreamer_plugin (BurnerPK *package,
                                     const gchar *element_name,
                                     int xid,
                                     GCancellable *cancel);
gboolean
burner_pk_install_missing_app (BurnerPK *package,
                                const gchar *file_name,
                                int xid,
                                GCancellable *cancel);
gboolean
burner_pk_install_missing_library (BurnerPK *package,
                                    const gchar *library_name,
                                    int xid,
                                    GCancellable *cancel);

G_END_DECLS

#endif /* _BURNER_PK_H_ */
