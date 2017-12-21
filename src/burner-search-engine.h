/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/*
 * Burner
 * Copyright (C) Philippe Rouquier 2005-2009 <bonfire-app@wanadoo.fr>
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

#ifndef _SEARCH_ENGINE_H
#define _SEARCH_ENGINE_H

#include <glib.h>
#include <glib-object.h>

#include <gtk/gtk.h>

G_BEGIN_DECLS

enum {
	BURNER_SEARCH_TREE_HIT_COL		= 0,
	BURNER_SEARCH_TREE_NB_COL
};

typedef enum {
	BURNER_SEARCH_SCOPE_ANY		= 0,
	BURNER_SEARCH_SCOPE_VIDEO		= 1,
	BURNER_SEARCH_SCOPE_MUSIC		= 1 << 1,
	BURNER_SEARCH_SCOPE_PICTURES	= 1 << 2,
	BURNER_SEARCH_SCOPE_DOCUMENTS	= 1 << 3,
} BurnerSearchScope;

#define BURNER_TYPE_SEARCH_ENGINE         (burner_search_engine_get_type ())
#define BURNER_SEARCH_ENGINE(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), BURNER_TYPE_SEARCH_ENGINE, BurnerSearchEngine))
#define BURNER_IS_SEARCH_ENGINE(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), BURNER_TYPE_SEARCH_ENGINE))
#define BURNER_SEARCH_ENGINE_GET_IFACE(o) (G_TYPE_INSTANCE_GET_INTERFACE ((o), BURNER_TYPE_SEARCH_ENGINE, BurnerSearchEngineIface))

typedef struct _BurnerSearchEngine        BurnerSearchEngine;
typedef struct _BurnerSearchEngineIface   BurnerSearchEngineIface;

struct _BurnerSearchEngineIface {
	GTypeInterface g_iface;

	/* <Signals> */
	void	(*search_error)				(BurnerSearchEngine *search,
							 GError *error);
	void	(*search_finished)			(BurnerSearchEngine *search);
	void	(*hit_removed)				(BurnerSearchEngine *search,
					                 gpointer hit);
	void	(*hit_added)				(BurnerSearchEngine *search,
						         gpointer hit);

	/* <Virtual functions> */
	gboolean	(*is_available)			(BurnerSearchEngine *search);
	gboolean	(*query_new)			(BurnerSearchEngine *search,
					                 const gchar *keywords);
	gboolean	(*query_set_scope)		(BurnerSearchEngine *search,
					                 BurnerSearchScope scope);
	gboolean	(*query_set_mime)		(BurnerSearchEngine *search,
					                 const gchar **mimes);
	gboolean	(*query_start)			(BurnerSearchEngine *search);

	gboolean	(*add_hits)			(BurnerSearchEngine *search,
					                 GtkTreeModel *model,
					                 gint range_start,
					                 gint range_end);

	gint		(*num_hits)			(BurnerSearchEngine *engine);

	const gchar*	(*uri_from_hit)			(BurnerSearchEngine *engine,
				                         gpointer hit);
	const gchar *	(*mime_from_hit)		(BurnerSearchEngine *engine,
				                	 gpointer hit);
	gint		(*score_from_hit)		(BurnerSearchEngine *engine,
							 gpointer hit);
};

GType burner_search_engine_get_type (void);

BurnerSearchEngine *
burner_search_engine_new_default (void);

gboolean
burner_search_engine_is_available (BurnerSearchEngine *search);

gint
burner_search_engine_num_hits (BurnerSearchEngine *search);

gboolean
burner_search_engine_new_query (BurnerSearchEngine *search,
                                 const gchar *keywords);

gboolean
burner_search_engine_set_query_scope (BurnerSearchEngine *search,
                                       BurnerSearchScope scope);

gboolean
burner_search_engine_set_query_mime (BurnerSearchEngine *search,
                                      const gchar **mimes);

gboolean
burner_search_engine_start_query (BurnerSearchEngine *search);

void
burner_search_engine_query_finished (BurnerSearchEngine *search);

void
burner_search_engine_query_error (BurnerSearchEngine *search,
                                   GError *error);

void
burner_search_engine_hit_removed (BurnerSearchEngine *search,
                                   gpointer hit);
void
burner_search_engine_hit_added (BurnerSearchEngine *search,
                                 gpointer hit);

const gchar *
burner_search_engine_uri_from_hit (BurnerSearchEngine *search,
                                     gpointer hit);
const gchar *
burner_search_engine_mime_from_hit (BurnerSearchEngine *search,
                                     gpointer hit);
gint
burner_search_engine_score_from_hit (BurnerSearchEngine *search,
                                      gpointer hit);

gint
burner_search_engine_add_hits (BurnerSearchEngine *search,
                                GtkTreeModel *model,
                                gint range_start,
                                gint range_end);

G_END_DECLS

#endif /* _SEARCH_ENGINE_H */
