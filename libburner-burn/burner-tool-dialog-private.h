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

#ifndef BURNER_TOOL_DIALOG_PRIVATE_H
#define BURNER_TOOL_DIALOG_PRIVATE_H

#include "burner-tool-dialog.h"


G_BEGIN_DECLS

void
burner_tool_dialog_pack_options (BurnerToolDialog *dialog, ...);

void
burner_tool_dialog_set_button (BurnerToolDialog *dialog,
				const gchar *text,
				const gchar *image,
				const gchar *theme);
void
burner_tool_dialog_set_valid (BurnerToolDialog *dialog,
			       gboolean valid);
void
burner_tool_dialog_set_medium_type_shown (BurnerToolDialog *dialog,
					   BurnerMediaType media_type);
void
burner_tool_dialog_set_progress (BurnerToolDialog *dialog,
				  gdouble overall_progress,
				  gdouble task_progress,
				  glong remaining,
				  gint size_mb,
				  gint written_mb);
void
burner_tool_dialog_set_action (BurnerToolDialog *dialog,
				BurnerBurnAction action,
				const gchar *string);

BurnerBurn *
burner_tool_dialog_get_burn (BurnerToolDialog *dialog);

BurnerMedium *
burner_tool_dialog_get_medium (BurnerToolDialog *dialog);

G_END_DECLS

#endif
