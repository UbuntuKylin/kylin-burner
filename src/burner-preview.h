/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/*
 * burner
 * Copyright (C) Philippe Rouquier 2007-2008 <bonfire-app@wanadoo.fr>
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

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#ifdef BUILD_PREVIEW

#ifndef _BURNER_PREVIEW_H_
#define _BURNER_PREVIEW_H_

#include <glib-object.h>
#include <gtk/gtk.h>

#include "burner-uri-container.h"

G_BEGIN_DECLS

#define BURNER_TYPE_PREVIEW             (burner_preview_get_type ())
#define BURNER_PREVIEW(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), BURNER_TYPE_PREVIEW, BurnerPreview))
#define BURNER_PREVIEW_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), BURNER_TYPE_PREVIEW, BurnerPreviewClass))
#define BURNER_IS_PREVIEW(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BURNER_TYPE_PREVIEW))
#define BURNER_IS_PREVIEW_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), BURNER_TYPE_PREVIEW))
#define BURNER_PREVIEW_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), BURNER_TYPE_PREVIEW, BurnerPreviewClass))

typedef struct _BurnerPreviewClass BurnerPreviewClass;
typedef struct _BurnerPreview BurnerPreview;

struct _BurnerPreviewClass
{
	GtkAlignmentClass parent_class;
};

struct _BurnerPreview
{
	GtkAlignment parent_instance;
};

GType burner_preview_get_type (void) G_GNUC_CONST;
GtkWidget *burner_preview_new (void);

void
burner_preview_add_source (BurnerPreview *preview,
			    BurnerURIContainer *source);

void
burner_preview_hide (BurnerPreview *preview);

void
burner_preview_set_enabled (BurnerPreview *self,
			     gboolean preview);

G_END_DECLS

#endif /* _BURNER_PREVIEW_H_ */

#endif /* BUILD_PREVIEW */
