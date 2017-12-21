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

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <glib.h>
#include <glib-object.h>

#include "burner-search-engine.h"

static void burner_search_engine_base_init (gpointer g_class);

typedef enum {
	SEARCH_FINISHED,
	SEARCH_ERROR,
	HIT_REMOVED,
	HIT_ADDED,
	LAST_SIGNAL
} BurnerSearchEngineSignalType;

static guint burner_search_engine_signals [LAST_SIGNAL] = { 0 };

gboolean
burner_search_engine_is_available (BurnerSearchEngine *search)
{
	BurnerSearchEngineIface *iface;

	g_return_val_if_fail (BURNER_IS_SEARCH_ENGINE (search), FALSE);

	iface = BURNER_SEARCH_ENGINE_GET_IFACE (search);
	if (!iface->is_available)
		return FALSE;

	return (* iface->is_available) (search);
}

gboolean
burner_search_engine_start_query (BurnerSearchEngine *search)
{
	BurnerSearchEngineIface *iface;

	g_return_val_if_fail (BURNER_IS_SEARCH_ENGINE (search), FALSE);

	iface = BURNER_SEARCH_ENGINE_GET_IFACE (search);
	if (!iface->query_start)
		return FALSE;

	return (* iface->query_start) (search);
}

gboolean
burner_search_engine_new_query (BurnerSearchEngine *search,
                                 const gchar *keywords)
{
	BurnerSearchEngineIface *iface;

	g_return_val_if_fail (BURNER_IS_SEARCH_ENGINE (search), FALSE);

	iface = BURNER_SEARCH_ENGINE_GET_IFACE (search);
	if (!iface->query_new)
		return FALSE;

	return (* iface->query_new) (search, keywords);
}

gboolean
burner_search_engine_set_query_scope (BurnerSearchEngine *search,
                                       BurnerSearchScope scope)
{
	BurnerSearchEngineIface *iface;

	g_return_val_if_fail (BURNER_IS_SEARCH_ENGINE (search), FALSE);

	iface = BURNER_SEARCH_ENGINE_GET_IFACE (search);
	if (!iface->query_set_scope)
		return FALSE;

	return (* iface->query_set_scope) (search, scope);
}

gboolean
burner_search_engine_set_query_mime (BurnerSearchEngine *search,
                                      const gchar **mimes)
{
	BurnerSearchEngineIface *iface;

	g_return_val_if_fail (BURNER_IS_SEARCH_ENGINE (search), FALSE);

	iface = BURNER_SEARCH_ENGINE_GET_IFACE (search);
	if (!iface->query_set_mime)
		return FALSE;

	return (* iface->query_set_mime) (search, mimes);
}

gboolean
burner_search_engine_add_hits (BurnerSearchEngine *search,
                                GtkTreeModel *model,
                                gint range_start,
                                gint range_end)
{
	BurnerSearchEngineIface *iface;

	g_return_val_if_fail (BURNER_IS_SEARCH_ENGINE (search), FALSE);

	iface = BURNER_SEARCH_ENGINE_GET_IFACE (search);
	if (!iface->add_hits)
		return FALSE;

	return iface->add_hits (search, model, range_start, range_end);
}

const gchar *
burner_search_engine_uri_from_hit (BurnerSearchEngine *search,
                                     gpointer hit)
{
	BurnerSearchEngineIface *iface;

	g_return_val_if_fail (BURNER_IS_SEARCH_ENGINE (search), NULL);

	if (!hit)
		return NULL;

	iface = BURNER_SEARCH_ENGINE_GET_IFACE (search);
	if (!iface->uri_from_hit)
		return FALSE;

	return (* iface->uri_from_hit) (search, hit);
}

const gchar *
burner_search_engine_mime_from_hit (BurnerSearchEngine *search,
                                     gpointer hit)
{
	BurnerSearchEngineIface *iface;

	g_return_val_if_fail (BURNER_IS_SEARCH_ENGINE (search), NULL);

	if (!hit)
		return NULL;

	iface = BURNER_SEARCH_ENGINE_GET_IFACE (search);
	if (!iface->mime_from_hit)
		return FALSE;

	return (* iface->mime_from_hit) (search, hit);
}

gint
burner_search_engine_score_from_hit (BurnerSearchEngine *search,
                                      gpointer hit)
{
	BurnerSearchEngineIface *iface;

	g_return_val_if_fail (BURNER_IS_SEARCH_ENGINE (search), 0);

	if (!hit)
		return 0;

	iface = BURNER_SEARCH_ENGINE_GET_IFACE (search);
	if (!iface->score_from_hit)
		return FALSE;

	return (* iface->score_from_hit) (search, hit);
}

gint
burner_search_engine_num_hits (BurnerSearchEngine *search)
{
	BurnerSearchEngineIface *iface;

	g_return_val_if_fail (BURNER_IS_SEARCH_ENGINE (search), 0);

	iface = BURNER_SEARCH_ENGINE_GET_IFACE (search);
	if (!iface->num_hits)
		return FALSE;

	return (* iface->num_hits) (search);
}

void
burner_search_engine_query_finished (BurnerSearchEngine *search)
{
	g_signal_emit (search,
	               burner_search_engine_signals [SEARCH_FINISHED],
	               0);
}

void
burner_search_engine_hit_removed (BurnerSearchEngine *search,
                                   gpointer hit)
{
	g_signal_emit (search,
	               burner_search_engine_signals [HIT_REMOVED],
	               0,
	               hit);
}

void
burner_search_engine_hit_added (BurnerSearchEngine *search,
                                 gpointer hit)
{
	g_signal_emit (search,
	               burner_search_engine_signals [HIT_ADDED],
	               0,
	               hit);
}

void
burner_search_engine_query_error (BurnerSearchEngine *search,
                                   GError *error)
{
	g_signal_emit (search,
	               burner_search_engine_signals [SEARCH_ERROR],
	               0,
	               error);
}

GType
burner_search_engine_get_type()
{
	static GType type = 0;

	if(type == 0) {
		static const GTypeInfo our_info = {
			sizeof (BurnerSearchEngineIface),
			burner_search_engine_base_init,
			NULL,
			NULL,
			NULL,
			NULL,
			0,
			0,
			NULL
		};

		type = g_type_register_static (G_TYPE_INTERFACE, 
					       "BurnerSearchEngine",
					       &our_info,
					       0);

		g_type_interface_add_prerequisite (type, G_TYPE_OBJECT);
	}

	return type;
}

static void
burner_search_engine_base_init (gpointer g_class)
{
	static gboolean initialized = FALSE;

	if (initialized)
		return;

	burner_search_engine_signals [SEARCH_ERROR] =
	    g_signal_new ("search_error",
			  BURNER_TYPE_SEARCH_ENGINE,
			  G_SIGNAL_RUN_LAST|G_SIGNAL_ACTION|G_SIGNAL_NO_RECURSE,
			  G_STRUCT_OFFSET (BurnerSearchEngineIface, search_error),
			  NULL,
			  NULL,
			  g_cclosure_marshal_VOID__POINTER,
			  G_TYPE_NONE,
			  1,
			  G_TYPE_POINTER);
	burner_search_engine_signals [SEARCH_FINISHED] =
	    g_signal_new ("search_finished",
			  BURNER_TYPE_SEARCH_ENGINE,
			  G_SIGNAL_RUN_LAST|G_SIGNAL_ACTION|G_SIGNAL_NO_RECURSE,
			  G_STRUCT_OFFSET (BurnerSearchEngineIface, search_finished),
			  NULL,
			  NULL,
			  g_cclosure_marshal_VOID__VOID,
			  G_TYPE_NONE,
			  0);
	burner_search_engine_signals [HIT_REMOVED] =
	    g_signal_new ("hit_removed",
			  BURNER_TYPE_SEARCH_ENGINE,
			  G_SIGNAL_RUN_LAST|G_SIGNAL_ACTION|G_SIGNAL_NO_RECURSE,
			  G_STRUCT_OFFSET (BurnerSearchEngineIface, hit_removed),
			  NULL,
			  NULL,
			  g_cclosure_marshal_VOID__POINTER,
			  G_TYPE_NONE,
			  1,
	                  G_TYPE_POINTER);
	burner_search_engine_signals [HIT_ADDED] =
	    g_signal_new ("hit_added",
			  BURNER_TYPE_SEARCH_ENGINE,
			  G_SIGNAL_RUN_LAST|G_SIGNAL_ACTION|G_SIGNAL_NO_RECURSE,
			  G_STRUCT_OFFSET (BurnerSearchEngineIface, hit_added),
			  NULL,
			  NULL,
			  g_cclosure_marshal_VOID__POINTER,
			  G_TYPE_NONE,
			  1,
	                  G_TYPE_POINTER);
	initialized = TRUE;
}

#ifdef BUILD_SEARCH

#ifdef BUILD_TRACKER

#include "burner-search-tracker.h"

BurnerSearchEngine *
burner_search_engine_new_default (void)
{
	return g_object_new (BURNER_TYPE_SEARCH_TRACKER, NULL);
}

#endif

#else

BurnerSearchEngine *
burner_search_engine_new_default (void)
{
	return NULL;
}

#endif
