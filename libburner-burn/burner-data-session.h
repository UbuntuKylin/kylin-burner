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

#ifndef _BURNER_DATA_SESSION_H_
#define _BURNER_DATA_SESSION_H_

#include <glib-object.h>

#include "burner-medium.h"
#include "burner-data-project.h"

G_BEGIN_DECLS

#define BURNER_TYPE_DATA_SESSION             (burner_data_session_get_type ())
#define BURNER_DATA_SESSION(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), BURNER_TYPE_DATA_SESSION, BurnerDataSession))
#define BURNER_DATA_SESSION_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), BURNER_TYPE_DATA_SESSION, BurnerDataSessionClass))
#define BURNER_IS_DATA_SESSION(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BURNER_TYPE_DATA_SESSION))
#define BURNER_IS_DATA_SESSION_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), BURNER_TYPE_DATA_SESSION))
#define BURNER_DATA_SESSION_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), BURNER_TYPE_DATA_SESSION, BurnerDataSessionClass))

typedef struct _BurnerDataSessionClass BurnerDataSessionClass;
typedef struct _BurnerDataSession BurnerDataSession;

struct _BurnerDataSessionClass
{
	BurnerDataProjectClass parent_class;
};

struct _BurnerDataSession
{
	BurnerDataProject parent_instance;
};

GType burner_data_session_get_type (void) G_GNUC_CONST;

gboolean
burner_data_session_add_last (BurnerDataSession *session,
			       BurnerMedium *medium,
			       GError **error);
void
burner_data_session_remove_last (BurnerDataSession *session);

BurnerMedium *
burner_data_session_get_loaded_medium (BurnerDataSession *session);

gboolean
burner_data_session_load_directory_contents (BurnerDataSession *session,
					      BurnerFileNode *node,
					      GError **error);

GSList *
burner_data_session_get_available_media (BurnerDataSession *session);

gboolean
burner_data_session_has_available_media (BurnerDataSession *session);

G_END_DECLS

#endif /* _BURNER_DATA_SESSION_H_ */
