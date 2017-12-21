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

#ifndef _BURN_PLUGIN_H_REGISTRATION_
#define _BURN_PLUGIN_H_REGISTRATION_

#include <glib.h>

#include "burner-medium.h"

#include "burner-enums.h"
#include "burner-track.h"
#include "burner-track-stream.h"
#include "burner-track-data.h"
#include "burner-plugin.h"

G_BEGIN_DECLS

#define BURNER_PLUGIN_BURN_FLAG_MASK	(BURNER_BURN_FLAG_DUMMY|		\
					 BURNER_BURN_FLAG_MULTI|		\
					 BURNER_BURN_FLAG_DAO|			\
					 BURNER_BURN_FLAG_RAW|			\
					 BURNER_BURN_FLAG_BURNPROOF|		\
					 BURNER_BURN_FLAG_OVERBURN|		\
					 BURNER_BURN_FLAG_NOGRACE|		\
					 BURNER_BURN_FLAG_APPEND|		\
					 BURNER_BURN_FLAG_MERGE)

#define BURNER_PLUGIN_IMAGE_FLAG_MASK	(BURNER_BURN_FLAG_APPEND|		\
					 BURNER_BURN_FLAG_MERGE)

#define BURNER_PLUGIN_BLANK_FLAG_MASK	(BURNER_BURN_FLAG_NOGRACE|		\
					 BURNER_BURN_FLAG_FAST_BLANK)

/**
 * These are the functions a plugin must implement
 */

GType burner_plugin_register_caps (BurnerPlugin *plugin, gchar **error);

void
burner_plugin_define (BurnerPlugin *plugin,
		       const gchar *name,
                       const gchar *display_name,
		       const gchar *description,
		       const gchar *author,
		       guint priority);
void
burner_plugin_set_compulsory (BurnerPlugin *self,
			       gboolean compulsory);

void
burner_plugin_register_group (BurnerPlugin *plugin,
			       const gchar *name);

typedef enum {
	BURNER_PLUGIN_IO_NONE			= 0,
	BURNER_PLUGIN_IO_ACCEPT_PIPE		= 1,
	BURNER_PLUGIN_IO_ACCEPT_FILE		= 1 << 1,
} BurnerPluginIOFlag;

GSList *
burner_caps_image_new (BurnerPluginIOFlag flags,
			BurnerImageFormat format);

GSList *
burner_caps_audio_new (BurnerPluginIOFlag flags,
			BurnerStreamFormat format);

GSList *
burner_caps_data_new (BurnerImageFS fs_type);

GSList *
burner_caps_disc_new (BurnerMedia media);

GSList *
burner_caps_checksum_new (BurnerChecksumType checksum);

void
burner_plugin_link_caps (BurnerPlugin *plugin,
			  GSList *outputs,
			  GSList *inputs);

void
burner_plugin_blank_caps (BurnerPlugin *plugin,
			   GSList *caps);

/**
 * This function is important since not only does it set the flags but it also 
 * tells burner which types of media are supported. So even if a plugin doesn't
 * support any flag, use it to tell burner which media are supported.
 * That's only needed if the plugin supports burn/blank operations.
 */
void
burner_plugin_set_flags (BurnerPlugin *plugin,
			  BurnerMedia media,
			  BurnerBurnFlag supported,
			  BurnerBurnFlag compulsory);
void
burner_plugin_set_blank_flags (BurnerPlugin *self,
				BurnerMedia media,
				BurnerBurnFlag supported,
				BurnerBurnFlag compulsory);

void
burner_plugin_process_caps (BurnerPlugin *plugin,
			     GSList *caps);

void
burner_plugin_set_process_flags (BurnerPlugin *plugin,
				  BurnerPluginProcessFlag flags);

void
burner_plugin_check_caps (BurnerPlugin *plugin,
			   BurnerChecksumType type,
			   GSList *caps);

/**
 * Plugin configure options
 */

BurnerPluginConfOption *
burner_plugin_conf_option_new (const gchar *key,
				const gchar *description,
				BurnerPluginConfOptionType type);

BurnerBurnResult
burner_plugin_add_conf_option (BurnerPlugin *plugin,
				BurnerPluginConfOption *option);

BurnerBurnResult
burner_plugin_conf_option_bool_add_suboption (BurnerPluginConfOption *option,
					       BurnerPluginConfOption *suboption);

BurnerBurnResult
burner_plugin_conf_option_int_set_range (BurnerPluginConfOption *option,
					  gint min,
					  gint max);

BurnerBurnResult
burner_plugin_conf_option_choice_add (BurnerPluginConfOption *option,
				       const gchar *string,
				       gint value);

void
burner_plugin_add_error (BurnerPlugin *plugin,
                          BurnerPluginErrorType type,
                          const gchar *detail);

void
burner_plugin_test_gstreamer_plugin (BurnerPlugin *plugin,
                                      const gchar *name);

void
burner_plugin_test_app (BurnerPlugin *plugin,
                         const gchar *name,
                         const gchar *version_arg,
                         const gchar *version_format,
                         gint version [3]);

/**
 * Boiler plate for plugin definition to save the hassle of definition.
 * To be put at the beginning of the .c file.
 */
typedef GType	(* BurnerPluginRegisterType)	(BurnerPlugin *plugin);

G_MODULE_EXPORT void
burner_plugin_check_config (BurnerPlugin *plugin);

#define BURNER_PLUGIN_BOILERPLATE(PluginName, plugin_name, PARENT_NAME, ParentName) \
typedef struct {								\
	ParentName parent;							\
} PluginName;									\
										\
typedef struct {								\
	ParentName##Class parent_class;						\
} PluginName##Class;								\
										\
static GType plugin_name##_type = 0;						\
										\
static GType									\
plugin_name##_get_type (void)							\
{										\
	return plugin_name##_type;						\
}										\
										\
static void plugin_name##_class_init (PluginName##Class *klass);		\
static void plugin_name##_init (PluginName *sp);				\
static void plugin_name##_finalize (GObject *object);			\
static void plugin_name##_export_caps (BurnerPlugin *plugin);	\
G_MODULE_EXPORT GType								\
burner_plugin_register (BurnerPlugin *plugin);				\
G_MODULE_EXPORT GType								\
burner_plugin_register (BurnerPlugin *plugin)				\
{														\
	if (burner_plugin_get_gtype (plugin) == G_TYPE_NONE)	\
		plugin_name##_export_caps (plugin);					\
	static const GTypeInfo our_info = {					\
		sizeof (PluginName##Class),					\
		NULL,										\
		NULL,										\
		(GClassInitFunc)plugin_name##_class_init,			\
		NULL,										\
		NULL,										\
		sizeof (PluginName),							\
		0,											\
		(GInstanceInitFunc)plugin_name##_init,			\
	};												\
	plugin_name##_type = g_type_module_register_type (G_TYPE_MODULE (plugin),		\
							  PARENT_NAME,			\
							  G_STRINGIFY (PluginName),		\
							  &our_info,				\
							  0);						\
	return plugin_name##_type;						\
}

#define BURNER_PLUGIN_ADD_STANDARD_CDR_FLAGS(plugin_MACRO, unsupported_MACRO)	\
	/* Use DAO for first session since AUDIO need it to write CD-TEXT */	\
	burner_plugin_set_flags (plugin_MACRO,					\
				  BURNER_MEDIUM_CD|				\
				  BURNER_MEDIUM_WRITABLE|			\
				  BURNER_MEDIUM_BLANK,				\
				  (BURNER_BURN_FLAG_DAO|			\
				  BURNER_BURN_FLAG_MULTI|			\
				  BURNER_BURN_FLAG_BURNPROOF|			\
				  BURNER_BURN_FLAG_OVERBURN|			\
				  BURNER_BURN_FLAG_DUMMY|			\
				  BURNER_BURN_FLAG_NOGRACE) &			\
				  (~(unsupported_MACRO)),				\
				  BURNER_BURN_FLAG_NONE);			\
	/* This is a CDR with data data can be merged or at least appended */	\
	burner_plugin_set_flags (plugin_MACRO,					\
				  BURNER_MEDIUM_CD|				\
				  BURNER_MEDIUM_WRITABLE|			\
				  BURNER_MEDIUM_APPENDABLE|			\
				  BURNER_MEDIUM_HAS_AUDIO|			\
				  BURNER_MEDIUM_HAS_DATA,			\
				  (BURNER_BURN_FLAG_APPEND|			\
				  BURNER_BURN_FLAG_MERGE|			\
				  BURNER_BURN_FLAG_BURNPROOF|			\
				  BURNER_BURN_FLAG_OVERBURN|			\
				  BURNER_BURN_FLAG_MULTI|			\
				  BURNER_BURN_FLAG_DUMMY|			\
				  BURNER_BURN_FLAG_NOGRACE) &			\
				  (~(unsupported_MACRO)),				\
				  BURNER_BURN_FLAG_APPEND);

#define BURNER_PLUGIN_ADD_STANDARD_CDRW_FLAGS(plugin_MACRO, unsupported_MACRO)			\
	/* Use DAO for first session since AUDIO needs it to write CD-TEXT */	\
	burner_plugin_set_flags (plugin_MACRO,					\
				  BURNER_MEDIUM_CD|				\
				  BURNER_MEDIUM_REWRITABLE|			\
				  BURNER_MEDIUM_BLANK,				\
				  (BURNER_BURN_FLAG_DAO|			\
				  BURNER_BURN_FLAG_MULTI|			\
				  BURNER_BURN_FLAG_BURNPROOF|			\
				  BURNER_BURN_FLAG_OVERBURN|			\
				  BURNER_BURN_FLAG_DUMMY|			\
				  BURNER_BURN_FLAG_NOGRACE) &			\
				  (~(unsupported_MACRO)),				\
				  BURNER_BURN_FLAG_NONE);			\
	/* It is a CDRW we want the CD to be either blanked before or appended	\
	 * that's why we set MERGE as compulsory. That way if the CD is not 	\
	 * MERGED we force the blank before writing to avoid appending sessions	\
	 * endlessly until there is no free space. */				\
	burner_plugin_set_flags (plugin_MACRO,					\
				  BURNER_MEDIUM_CD|				\
				  BURNER_MEDIUM_REWRITABLE|			\
				  BURNER_MEDIUM_APPENDABLE|			\
				  BURNER_MEDIUM_HAS_AUDIO|			\
				  BURNER_MEDIUM_HAS_DATA,			\
				  (BURNER_BURN_FLAG_APPEND|			\
				  BURNER_BURN_FLAG_MERGE|			\
				  BURNER_BURN_FLAG_BURNPROOF|			\
				  BURNER_BURN_FLAG_OVERBURN|			\
				  BURNER_BURN_FLAG_MULTI|			\
				  BURNER_BURN_FLAG_DUMMY|			\
				  BURNER_BURN_FLAG_NOGRACE) &			\
				  (~(unsupported_MACRO)),				\
				  BURNER_BURN_FLAG_MERGE);

#define BURNER_PLUGIN_ADD_STANDARD_DVDR_FLAGS(plugin_MACRO, unsupported_MACRO)			\
	/* DAO and MULTI are exclusive */					\
	burner_plugin_set_flags (plugin_MACRO,					\
				  BURNER_MEDIUM_DVDR|				\
				  BURNER_MEDIUM_DUAL_L|			\
				  BURNER_MEDIUM_JUMP|				\
				  BURNER_MEDIUM_BLANK,				\
				  (BURNER_BURN_FLAG_DAO|			\
				  BURNER_BURN_FLAG_BURNPROOF|			\
				  BURNER_BURN_FLAG_DUMMY|			\
				  BURNER_BURN_FLAG_NOGRACE) &			\
				  (~(unsupported_MACRO)),				\
				  BURNER_BURN_FLAG_NONE);			\
	burner_plugin_set_flags (plugin_MACRO,					\
				  BURNER_MEDIUM_DVDR|				\
				  BURNER_MEDIUM_DUAL_L|			\
				  BURNER_MEDIUM_JUMP|				\
				  BURNER_MEDIUM_BLANK,				\
				  (BURNER_BURN_FLAG_BURNPROOF|			\
				  BURNER_BURN_FLAG_MULTI|			\
				  BURNER_BURN_FLAG_DUMMY|			\
				  BURNER_BURN_FLAG_NOGRACE) &			\
				  (~(unsupported_MACRO)),				\
				  BURNER_BURN_FLAG_NONE);			\
	/* This is a DVDR with data; data can be merged or at least appended */	\
	burner_plugin_set_flags (plugin_MACRO,					\
				  BURNER_MEDIUM_DVDR|				\
				  BURNER_MEDIUM_DUAL_L|			\
				  BURNER_MEDIUM_JUMP|				\
				  BURNER_MEDIUM_APPENDABLE|			\
				  BURNER_MEDIUM_HAS_DATA,			\
				  (BURNER_BURN_FLAG_APPEND|			\
				  BURNER_BURN_FLAG_MERGE|			\
				  BURNER_BURN_FLAG_BURNPROOF|			\
				  BURNER_BURN_FLAG_MULTI|			\
				  BURNER_BURN_FLAG_DUMMY|			\
				  BURNER_BURN_FLAG_NOGRACE) &			\
				  (~(unsupported_MACRO)),				\
				  BURNER_BURN_FLAG_APPEND);

#define BURNER_PLUGIN_ADD_STANDARD_DVDR_PLUS_FLAGS(plugin_MACRO, unsupported_MACRO)		\
	/* DVD+R don't have a DUMMY mode */					\
	burner_plugin_set_flags (plugin_MACRO,					\
				  BURNER_MEDIUM_DVDR_PLUS|			\
				  BURNER_MEDIUM_DUAL_L|			\
				  BURNER_MEDIUM_BLANK,				\
				  (BURNER_BURN_FLAG_DAO|			\
				  BURNER_BURN_FLAG_BURNPROOF|			\
				  BURNER_BURN_FLAG_NOGRACE) &			\
				  (~(unsupported_MACRO)),				\
				  BURNER_BURN_FLAG_NONE);			\
	burner_plugin_set_flags (plugin_MACRO,					\
				  BURNER_MEDIUM_DVDR_PLUS|			\
				  BURNER_MEDIUM_DUAL_L|			\
				  BURNER_MEDIUM_BLANK,				\
				  (BURNER_BURN_FLAG_BURNPROOF|			\
				  BURNER_BURN_FLAG_MULTI|			\
				  BURNER_BURN_FLAG_NOGRACE) &			\
				  (~(unsupported_MACRO)),				\
				  BURNER_BURN_FLAG_NONE);			\
	/* DVD+R with data: data can be merged or at least appended */		\
	burner_plugin_set_flags (plugin_MACRO,					\
				  BURNER_MEDIUM_DVDR_PLUS|			\
				  BURNER_MEDIUM_DUAL_L|			\
				  BURNER_MEDIUM_APPENDABLE|			\
				  BURNER_MEDIUM_HAS_DATA,			\
				  (BURNER_BURN_FLAG_MERGE|			\
				  BURNER_BURN_FLAG_APPEND|			\
				  BURNER_BURN_FLAG_BURNPROOF|			\
				  BURNER_BURN_FLAG_MULTI|			\
				  BURNER_BURN_FLAG_NOGRACE) &			\
				  (~(unsupported_MACRO)),				\
				  BURNER_BURN_FLAG_APPEND);

#define BURNER_PLUGIN_ADD_STANDARD_DVDRW_FLAGS(plugin_MACRO, unsupported_MACRO)			\
	burner_plugin_set_flags (plugin_MACRO,					\
				  BURNER_MEDIUM_DVDRW|				\
				  BURNER_MEDIUM_UNFORMATTED|			\
				  BURNER_MEDIUM_BLANK,				\
				  (BURNER_BURN_FLAG_DAO|			\
				  BURNER_BURN_FLAG_BURNPROOF|			\
				  BURNER_BURN_FLAG_DUMMY|			\
				  BURNER_BURN_FLAG_NOGRACE) &			\
				  (~(unsupported_MACRO)),				\
				  BURNER_BURN_FLAG_NONE);			\
	burner_plugin_set_flags (plugin_MACRO,					\
				  BURNER_MEDIUM_DVDRW|				\
				  BURNER_MEDIUM_BLANK,				\
				  (BURNER_BURN_FLAG_BURNPROOF|			\
				  BURNER_BURN_FLAG_MULTI|			\
				  BURNER_BURN_FLAG_DUMMY|			\
				  BURNER_BURN_FLAG_NOGRACE) &			\
				  (~(unsupported_MACRO)),				\
				  BURNER_BURN_FLAG_NONE);			\
	/* This is a DVDRW we want the DVD to be either blanked before or	\
	 * appended that's why we set MERGE as compulsory. That way if the DVD	\
	 * is not MERGED we force the blank before writing to avoid appending	\
	 * sessions endlessly until there is no free space. */			\
	burner_plugin_set_flags (plugin_MACRO,					\
				  BURNER_MEDIUM_DVDRW|				\
				  BURNER_MEDIUM_APPENDABLE|			\
				  BURNER_MEDIUM_HAS_DATA,			\
				  (BURNER_BURN_FLAG_MERGE|			\
				  BURNER_BURN_FLAG_APPEND|			\
				  BURNER_BURN_FLAG_BURNPROOF|			\
				  BURNER_BURN_FLAG_MULTI|			\
				  BURNER_BURN_FLAG_DUMMY|			\
				  BURNER_BURN_FLAG_NOGRACE) &			\
				  (~(unsupported_MACRO)),				\
				  BURNER_BURN_FLAG_MERGE);

/**
 * These kind of media don't support:
 * - BURNPROOF
 * - DAO
 * - APPEND
 * since they don't behave and are not written in the same way.
 * They also can't be closed so MULTI is compulsory.
 */
#define BURNER_PLUGIN_ADD_STANDARD_DVDRW_PLUS_FLAGS(plugin_MACRO, unsupported_MACRO)		\
	burner_plugin_set_flags (plugin_MACRO,					\
				  BURNER_MEDIUM_DVDRW_PLUS|			\
				  BURNER_MEDIUM_DUAL_L|			\
				  BURNER_MEDIUM_UNFORMATTED|			\
				  BURNER_MEDIUM_BLANK,				\
				  (BURNER_BURN_FLAG_MULTI|			\
				  BURNER_BURN_FLAG_NOGRACE) &			\
				  (~(unsupported_MACRO)),				\
				  BURNER_BURN_FLAG_MULTI);			\
	burner_plugin_set_flags (plugin_MACRO,					\
				  BURNER_MEDIUM_DVDRW_PLUS|			\
				  BURNER_MEDIUM_DUAL_L|			\
				  BURNER_MEDIUM_APPENDABLE|			\
				  BURNER_MEDIUM_CLOSED|			\
				  BURNER_MEDIUM_HAS_DATA,			\
				  (BURNER_BURN_FLAG_MULTI|			\
				  BURNER_BURN_FLAG_NOGRACE|			\
				  BURNER_BURN_FLAG_MERGE) &			\
				  (~(unsupported_MACRO)),				\
				  BURNER_BURN_FLAG_MULTI);

/**
 * The above statement apply to these as well. There is no longer dummy mode
 * NOTE: there is no such thing as a DVD-RW DL.
 */
#define BURNER_PLUGIN_ADD_STANDARD_DVDRW_RESTRICTED_FLAGS(plugin_MACRO, unsupported_MACRO)	\
	burner_plugin_set_flags (plugin_MACRO,					\
				  BURNER_MEDIUM_DVD|				\
				  BURNER_MEDIUM_RESTRICTED|			\
				  BURNER_MEDIUM_REWRITABLE|			\
				  BURNER_MEDIUM_UNFORMATTED|			\
				  BURNER_MEDIUM_BLANK,				\
				  (BURNER_BURN_FLAG_MULTI|			\
				  BURNER_BURN_FLAG_NOGRACE) &			\
				  (~(unsupported_MACRO)),				\
				  BURNER_BURN_FLAG_MULTI);			\
	burner_plugin_set_flags (plugin_MACRO,					\
				  BURNER_MEDIUM_DVD|				\
				  BURNER_MEDIUM_RESTRICTED|			\
				  BURNER_MEDIUM_REWRITABLE|			\
				  BURNER_MEDIUM_APPENDABLE|			\
				  BURNER_MEDIUM_CLOSED|			\
				  BURNER_MEDIUM_HAS_DATA,			\
				  (BURNER_BURN_FLAG_MULTI|			\
				  BURNER_BURN_FLAG_NOGRACE|			\
				  BURNER_BURN_FLAG_MERGE) &			\
				  (~(unsupported_MACRO)),				\
				  BURNER_BURN_FLAG_MULTI);

#define BURNER_PLUGIN_ADD_STANDARD_BD_R_FLAGS(plugin_MACRO, unsupported_MACRO)			\
	/* DAO and MULTI are exclusive */					\
	burner_plugin_set_flags (plugin_MACRO,					\
				  BURNER_MEDIUM_BDR_RANDOM|			\
				  BURNER_MEDIUM_BDR_SRM|			\
				  BURNER_MEDIUM_BDR_SRM_POW|			\
				  BURNER_MEDIUM_DUAL_L|			\
				  BURNER_MEDIUM_BLANK,				\
				  (BURNER_BURN_FLAG_DAO|			\
				  BURNER_BURN_FLAG_DUMMY|			\
				  BURNER_BURN_FLAG_NOGRACE) &			\
				  (~(unsupported_MACRO)),				\
				  BURNER_BURN_FLAG_NONE);			\
	burner_plugin_set_flags (plugin_MACRO,					\
				  BURNER_MEDIUM_BDR_RANDOM|			\
				  BURNER_MEDIUM_BDR_SRM|			\
				  BURNER_MEDIUM_BDR_SRM_POW|			\
				  BURNER_MEDIUM_DUAL_L|			\
				  BURNER_MEDIUM_BLANK,				\
				  BURNER_BURN_FLAG_MULTI|			\
				  BURNER_BURN_FLAG_DUMMY|			\
				  BURNER_BURN_FLAG_NOGRACE) &			\
				  (~(unsupported_MACRO)),				\
				  BURNER_BURN_FLAG_NONE);			\
	/* This is a DVDR with data data can be merged or at least appended */	\
	burner_plugin_set_flags (plugin_MACRO,					\
				  BURNER_MEDIUM_BDR_RANDOM|			\
				  BURNER_MEDIUM_BDR_SRM|			\
				  BURNER_MEDIUM_BDR_SRM_POW|			\
				  BURNER_MEDIUM_DUAL_L|			\
				  BURNER_MEDIUM_APPENDABLE|			\
				  BURNER_MEDIUM_HAS_DATA,			\
				  (BURNER_BURN_FLAG_APPEND|			\
				  BURNER_BURN_FLAG_MERGE|			\
				  BURNER_BURN_FLAG_MULTI|			\
				  BURNER_BURN_FLAG_DUMMY|			\
				  BURNER_BURN_FLAG_NOGRACE) &			\
				  (~(unsupported_MACRO)),				\
				  BURNER_BURN_FLAG_APPEND);

/**
 * These kind of media don't support:
 * - BURNPROOF
 * - DAO
 * - APPEND
 * since they don't behave and are not written in the same way.
 * They also can't be closed so MULTI is compulsory.
 */
#define BURNER_PLUGIN_ADD_STANDARD_BD_RE_FLAGS(plugin_MACRO, unsupported_MACRO)			\
	burner_plugin_set_flags (plugin_MACRO,					\
				  BURNER_MEDIUM_BDRE|				\
				  BURNER_MEDIUM_DUAL_L|			\
				  BURNER_MEDIUM_UNFORMATTED|			\
				  BURNER_MEDIUM_BLANK,				\
				  (BURNER_BURN_FLAG_MULTI|			\
				  BURNER_BURN_FLAG_NOGRACE) &			\
				  (~(unsupported_MACRO)),				\
				  BURNER_BURN_FLAG_MULTI);			\
	burner_plugin_set_flags (plugin_MACRO,					\
				  BURNER_MEDIUM_BDRE|				\
				  BURNER_MEDIUM_DUAL_L|			\
				  BURNER_MEDIUM_APPENDABLE|			\
				  BURNER_MEDIUM_CLOSED|			\
				  BURNER_MEDIUM_HAS_DATA,			\
				  (BURNER_BURN_FLAG_MULTI|			\
				  BURNER_BURN_FLAG_NOGRACE|			\
				  BURNER_BURN_FLAG_MERGE) &			\
				  (~(unsupported_MACRO)),				\
				  BURNER_BURN_FLAG_MULTI);

G_END_DECLS

#endif
