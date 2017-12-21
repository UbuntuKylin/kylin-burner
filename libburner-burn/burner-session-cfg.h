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

#ifndef _BURNER_SESSION_CFG_H_
#define _BURNER_SESSION_CFG_H_

#include <glib-object.h>

#include <burner-session.h>
#include <burner-session-span.h>

G_BEGIN_DECLS

#define BURNER_TYPE_SESSION_CFG             (burner_session_cfg_get_type ())
#define BURNER_SESSION_CFG(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), BURNER_TYPE_SESSION_CFG, BurnerSessionCfg))
#define BURNER_SESSION_CFG_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), BURNER_TYPE_SESSION_CFG, BurnerSessionCfgClass))
#define BURNER_IS_SESSION_CFG(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BURNER_TYPE_SESSION_CFG))
#define BURNER_IS_SESSION_CFG_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), BURNER_TYPE_SESSION_CFG))
#define BURNER_SESSION_CFG_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), BURNER_TYPE_SESSION_CFG, BurnerSessionCfgClass))

typedef struct _BurnerSessionCfgClass BurnerSessionCfgClass;
typedef struct _BurnerSessionCfg BurnerSessionCfg;

struct _BurnerSessionCfgClass
{
	BurnerSessionSpanClass parent_class;
};

struct _BurnerSessionCfg
{
	BurnerSessionSpan parent_instance;
};

GType burner_session_cfg_get_type (void) G_GNUC_CONST;

/**
 * This is for the signal sent to tell whether or not session is valid
 */

typedef enum {
	BURNER_SESSION_VALID				= 0,
	BURNER_SESSION_NO_CD_TEXT			= 1,
	BURNER_SESSION_NOT_READY,
	BURNER_SESSION_EMPTY,
	BURNER_SESSION_NO_INPUT_IMAGE,
	BURNER_SESSION_UNKNOWN_IMAGE,
	BURNER_SESSION_NO_INPUT_MEDIUM,
	BURNER_SESSION_NO_OUTPUT,
	BURNER_SESSION_INSUFFICIENT_SPACE,
	BURNER_SESSION_OVERBURN_NECESSARY,
	BURNER_SESSION_NOT_SUPPORTED,
	BURNER_SESSION_DISC_PROTECTED
} BurnerSessionError;

#define BURNER_SESSION_IS_VALID(result_MACRO)					\
	((result_MACRO) == BURNER_SESSION_VALID ||				\
	 (result_MACRO) == BURNER_SESSION_NO_CD_TEXT)

BurnerSessionCfg *
burner_session_cfg_new (void);

BurnerSessionError
burner_session_cfg_get_error (BurnerSessionCfg *cfg);

void
burner_session_cfg_add_flags (BurnerSessionCfg *cfg,
			       BurnerBurnFlag flags);
void
burner_session_cfg_remove_flags (BurnerSessionCfg *cfg,
				  BurnerBurnFlag flags);
gboolean
burner_session_cfg_is_supported (BurnerSessionCfg *cfg,
				  BurnerBurnFlag flag);
gboolean
burner_session_cfg_is_compulsory (BurnerSessionCfg *cfg,
				   BurnerBurnFlag flag);

gboolean
burner_session_cfg_has_default_output_path (BurnerSessionCfg *cfg);

void
burner_session_cfg_enable (BurnerSessionCfg *cfg);

void
burner_session_cfg_disable (BurnerSessionCfg *cfg);

G_END_DECLS

#endif /* _BURNER_SESSION_CFG_H_ */
