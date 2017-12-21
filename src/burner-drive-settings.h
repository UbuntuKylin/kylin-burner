/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/*
 * Burner
 * Copyright (C) Philippe Rouquier 2005-2010 <bonfire-app@wanadoo.fr>
 * 
 *  Burner is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 * 
 * burner is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with burner.  If not, write to:
 * 	The Free Software Foundation, Inc.,
 * 	51 Franklin Street, Fifth Floor
 * 	Boston, MA  02110-1301, USA.
 */

#ifndef _BURNER_DRIVE_SETTINGS_H_
#define _BURNER_DRIVE_SETTINGS_H_

#include <glib-object.h>

#include "burner-session.h"

G_BEGIN_DECLS

#define BURNER_TYPE_DRIVE_SETTINGS             (burner_drive_settings_get_type ())
#define BURNER_DRIVE_SETTINGS(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), BURNER_TYPE_DRIVE_SETTINGS, BurnerDriveSettings))
#define BURNER_DRIVE_SETTINGS_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), BURNER_TYPE_DRIVE_SETTINGS, BurnerDriveSettingsClass))
#define BURNER_IS_DRIVE_SETTINGS(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BURNER_TYPE_DRIVE_SETTINGS))
#define BURNER_IS_DRIVE_SETTINGS_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), BURNER_TYPE_DRIVE_SETTINGS))
#define BURNER_DRIVE_SETTINGS_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), BURNER_TYPE_DRIVE_SETTINGS, BurnerDriveSettingsClass))

typedef struct _BurnerDriveSettingsClass BurnerDriveSettingsClass;
typedef struct _BurnerDriveSettings BurnerDriveSettings;

struct _BurnerDriveSettingsClass
{
	GObjectClass parent_class;
};

struct _BurnerDriveSettings
{
	GObject parent_instance;
};

GType burner_drive_settings_get_type (void) G_GNUC_CONST;

BurnerDriveSettings *
burner_drive_settings_new (void);

void
burner_drive_settings_set_session (BurnerDriveSettings *self,
                                    BurnerBurnSession *session);

G_END_DECLS

#endif /* _BURNER_DRIVE_SETTINGS_H_ */
