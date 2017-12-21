/***************************************************************************
 *            disc.h
 *
 *  dim nov 27 14:58:13 2005
 *  Copyright  2005  Rouquier Philippe
 * Copyright (C) 2017,Tianjin KYLIN Information Technology Co., Ltd.
 *  burner-app@wanadoo.fr
 ***************************************************************************/

/*
 *  Burner is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  Burner is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Library General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to:
 * 	The Free Software Foundation, Inc.,
 * 	51 Franklin Street, Fifth Floor
 * 	Boston, MA  02110-1301, USA.
 */

#ifndef _BURNER_PROJECT_PARSE_H_
#define _BURNER_PROJECT_PARSE_H_

#include <glib.h>

#include "burner-track.h"
#include "burner-session.h"

G_BEGIN_DECLS

typedef enum {
	BURNER_PROJECT_SAVE_XML			= 0,
	BURNER_PROJECT_SAVE_PLAIN			= 1,
	BURNER_PROJECT_SAVE_PLAYLIST_PLS		= 2,
	BURNER_PROJECT_SAVE_PLAYLIST_M3U		= 3,
	BURNER_PROJECT_SAVE_PLAYLIST_XSPF		= 4,
	BURNER_PROJECT_SAVE_PLAYLIST_IRIVER_PLA	= 5
} BurnerProjectSave;

typedef enum {
	BURNER_PROJECT_TYPE_INVALID,
	BURNER_PROJECT_TYPE_COPY,
	BURNER_PROJECT_TYPE_ISO,
	BURNER_PROJECT_TYPE_AUDIO,
	BURNER_PROJECT_TYPE_DATA,
	BURNER_PROJECT_TYPE_VIDEO,
	BURNER_PROJECT_TYPE_WELC
} BurnerProjectType;

gboolean
burner_project_open_project_xml (const gchar *uri,
				  BurnerBurnSession *session,
				  gboolean warn_user);

gboolean
burner_project_open_audio_playlist_project (const gchar *uri,
					     BurnerBurnSession *session,
					     gboolean warn_user);

gboolean 
burner_project_save_project_xml (BurnerBurnSession *session,
				  const gchar *uri);

gboolean
burner_project_save_audio_project_plain_text (BurnerBurnSession *session,
					       const gchar *uri);

gboolean
burner_project_save_audio_project_playlist (BurnerBurnSession *session,
					     const gchar *uri,
					     BurnerProjectSave type);

G_END_DECLS

#endif
