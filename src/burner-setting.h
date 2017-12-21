/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/*
 * burner
 * Copyright (C) Philippe Rouquier 2009 <bonfire-app@wanadoo.fr>
 * 
 * burner is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * burner is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef _BURNER_SETTING_H_
#define _BURNER_SETTING_H_

#include <glib-object.h>

G_BEGIN_DECLS

typedef enum {
	BURNER_SETTING_VALUE_NONE,

	/** gint value **/
	BURNER_SETTING_WIN_WIDTH,
	BURNER_SETTING_WIN_HEIGHT,
	BURNER_SETTING_STOCK_FILE_CHOOSER_PERCENT,
	BURNER_SETTING_BURNER_FILE_CHOOSER_PERCENT,
	BURNER_SETTING_PLAYER_VOLUME,
	BURNER_SETTING_DISPLAY_PROPORTION,
	BURNER_SETTING_DISPLAY_LAYOUT,
	BURNER_SETTING_DATA_DISC_COLUMN,
	BURNER_SETTING_DATA_DISC_COLUMN_ORDER,
	BURNER_SETTING_IMAGE_SIZE_WIDTH,
	BURNER_SETTING_IMAGE_SIZE_HEIGHT,
	BURNER_SETTING_VIDEO_SIZE_HEIGHT,
	BURNER_SETTING_VIDEO_SIZE_WIDTH,

	/** gboolean **/
	BURNER_SETTING_WIN_MAXIMIZED,
	BURNER_SETTING_SHOW_SIDEPANE,
	BURNER_SETTING_SHOW_PREVIEW,

	/** gchar * **/
	BURNER_SETTING_DISPLAY_LAYOUT_AUDIO,
	BURNER_SETTING_DISPLAY_LAYOUT_DATA,
	BURNER_SETTING_DISPLAY_LAYOUT_VIDEO,

	/** gchar ** **/
	BURNER_SETTING_SEARCH_ENTRY_HISTORY,

} BurnerSettingValue;

#define BURNER_TYPE_SETTING             (burner_setting_get_type ())
#define BURNER_SETTING(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), BURNER_TYPE_SETTING, BurnerSetting))
#define BURNER_SETTING_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), BURNER_TYPE_SETTING, BurnerSettingClass))
#define BURNER_IS_SETTING(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BURNER_TYPE_SETTING))
#define BURNER_IS_SETTING_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), BURNER_TYPE_SETTING))
#define BURNER_SETTING_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), BURNER_TYPE_SETTING, BurnerSettingClass))

typedef struct _BurnerSettingClass BurnerSettingClass;
typedef struct _BurnerSetting BurnerSetting;

struct _BurnerSettingClass
{
	GObjectClass parent_class;

	/* Signals */
	void(* value_changed) (BurnerSetting *self, gint value);
};

struct _BurnerSetting
{
	GObject parent_instance;
};

GType burner_setting_get_type (void) G_GNUC_CONST;

BurnerSetting *
burner_setting_get_default (void);

gboolean
burner_setting_get_value (BurnerSetting *setting,
                           BurnerSettingValue setting_value,
                           gpointer *value);

gboolean
burner_setting_set_value (BurnerSetting *setting,
                           BurnerSettingValue setting_value,
                           gconstpointer value);

gboolean
burner_setting_load (BurnerSetting *setting);

gboolean
burner_setting_save (BurnerSetting *setting);

G_END_DECLS

#endif /* _BURNER_SETTING_H_ */
