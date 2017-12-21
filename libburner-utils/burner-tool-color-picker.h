/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/*
 * Libburner-misc
 * Copyright (C) Philippe Rouquier 2005-2009 <bonfire-app@wanadoo.fr>
 *
 * Libburner-misc is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * The Libburner-misc authors hereby grant permission for non-GPL compatible
 * GStreamer plugins to be used and distributed together with GStreamer
 * and Libburner-misc. This permission is above and beyond the permissions granted
 * by the GPL license by which Libburner-burn is covered. If you modify this code
 * you may extend this exception to your version of the code, but you are not
 * obligated to do so. If you do not wish to do so, delete this exception
 * statement from your version.
 * 
 * Libburner-misc is distributed in the hope that it will be useful,
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

#ifndef _BURNER_TOOL_COLOR_PICKER_H_
#define _BURNER_TOOL_COLOR_PICKER_H_

#include <glib-object.h>

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define BURNER_TYPE_TOOL_COLOR_PICKER             (burner_tool_color_picker_get_type ())
#define BURNER_TOOL_COLOR_PICKER(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), BURNER_TYPE_TOOL_COLOR_PICKER, BurnerToolColorPicker))
#define BURNER_TOOL_COLOR_PICKER_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), BURNER_TYPE_TOOL_COLOR_PICKER, BurnerToolColorPickerClass))
#define BURNER_IS_TOOL_COLOR_PICKER(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BURNER_TYPE_TOOL_COLOR_PICKER))
#define BURNER_IS_TOOL_COLOR_PICKER_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), BURNER_TYPE_TOOL_COLOR_PICKER))
#define BURNER_TOOL_COLOR_PICKER_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), BURNER_TYPE_TOOL_COLOR_PICKER, BurnerToolColorPickerClass))

typedef struct _BurnerToolColorPickerClass BurnerToolColorPickerClass;
typedef struct _BurnerToolColorPicker BurnerToolColorPicker;

struct _BurnerToolColorPickerClass
{
	GtkToolButtonClass parent_class;
};

struct _BurnerToolColorPicker
{
	GtkToolButton parent_instance;
};

GType burner_tool_color_picker_get_type (void) G_GNUC_CONST;

GtkWidget *
burner_tool_color_picker_new (void);

void
burner_tool_color_picker_set_text (BurnerToolColorPicker *picker,
				    const gchar *text);
void
burner_tool_color_picker_set_color (BurnerToolColorPicker *picker,
				     GdkColor *color);
void
burner_tool_color_picker_get_color (BurnerToolColorPicker *picker,
				     GdkColor *color);

G_END_DECLS

#endif /* _BURNER_TOOL_COLOR_PICKER_H_ */
