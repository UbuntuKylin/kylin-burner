/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/*
 * Libburner-media
 * Copyright (C) Philippe Rouquier 2005-2009 <bonfire-app@wanadoo.fr>
 *
 * Libburner-media is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * The Libburner-media authors hereby grant permission for non-GPL compatible
 * GStreamer plugins to be used and distributed together with GStreamer
 * and Libburner-media. This permission is above and beyond the permissions granted
 * by the GPL license by which Libburner-media is covered. If you modify this code
 * you may extend this exception to your version of the code, but you are not
 * obligated to do so. If you do not wish to do so, delete this exception
 * statement from your version.
 * 
 * Libburner-media is distributed in the hope that it will be useful,
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

#include <string.h>

#include <glib.h>

#include "burn-iso-field.h"
#include "burn-susp.h"

struct _BurnerSusp {
	gchar signature		[2];
	guchar len;
	gchar version;
	gchar data		[0];
};
typedef struct _BurnerSusp BurnerSusp;

struct _BurnerSuspSP {
	BurnerSusp susp	[1];
	gchar magic		[2];
	gchar skip;
};
typedef struct _BurnerSuspSP BurnerSuspSP;

struct _BurnerSuspER {
	BurnerSusp susp	[1];
	gchar id_len;
	gchar desc_len;
	gchar src_len;
	gchar extension_version;
	gchar id [0];
};
typedef struct _BurnerSuspER BurnerSuspER;

struct _BurnerSuspCE {
	BurnerSusp susp	[1];
	guchar block		[8];
	guchar offset		[8];
	guchar len		[8];
};
typedef struct _BurnerSuspCE BurnerSuspCE;

struct _BurnerRockNM {
	BurnerSusp susp	[1];
	gchar flags;
	gchar name		[0];
};
typedef struct _BurnerRockNM BurnerRockNM;

struct _BurnerRockCL {
	BurnerSusp susp	[1];
	guchar location		[8];
};
typedef struct _BurnerRockCL BurnerRockCL;

struct _BurnerRockPL {
	BurnerSusp susp	[1];
	guchar location		[8];
};
typedef struct _BurnerRockPL BurnerRockPL;

typedef enum {
	BURNER_NM_CONTINUE	= 1,
	BURNER_NM_CURRENT	= 1 << 1,
	BURNER_NM_PARENT	= 1 << 2,
	BURNER_NM_NETWORK	= 1 << 5
} BurnerNMFlag;

void
burner_susp_ctx_clean (BurnerSuspCtx *ctx)
{
	if (ctx->rr_name)
		g_free (ctx->rr_name);
}

static gboolean
burner_susp_SP (BurnerSusp *susp,
		 BurnerSuspCtx *ctx)
{
	gchar magic [2] = { 0xBE, 0xEF };
	BurnerSuspSP *sp;

	sp = (BurnerSuspSP *) susp;

	if (memcmp (sp->magic, magic, 2))
		return FALSE;

	ctx->skip = sp->skip;
	ctx->has_SP = TRUE;
	return TRUE;
}

static gboolean
burner_susp_CE (BurnerSusp *susp,
		 BurnerSuspCtx *ctx)
{
	BurnerSuspCE *ce;

	ce = (BurnerSuspCE *) susp;

	ctx->CE_address = burner_iso9660_get_733_val (ce->block);
	ctx->CE_offset = burner_iso9660_get_733_val (ce->offset);
	ctx->CE_len = burner_iso9660_get_733_val (ce->len);

	return TRUE;
}

static gboolean
burner_susp_ER (BurnerSusp *susp,
		 BurnerSuspCtx *ctx)
{
	BurnerSuspER *er;

	er = (BurnerSuspER *) susp;

	/* Make sure the extention is Rock Ridge */
	if (susp->version != 1)
		return FALSE;

	if (er->id_len == 9 && !strncmp (er->id, "IEEE_1282", 9)) {
		ctx->has_RockRidge = TRUE;
		return TRUE;
	}

	if (er->id_len != 10)
		return TRUE;

	if (!strncmp (er->id, "IEEE_P1282", 10))
		ctx->has_RockRidge = TRUE;
	else if (!strncmp (er->id, "RRIP_1991A", 10))
		ctx->has_RockRidge = TRUE;

	return TRUE;
}

static gboolean
burner_susp_NM (BurnerSusp *susp,
		 BurnerSuspCtx *ctx)
{
	gint len;
	gchar *rr_name;
	BurnerRockNM *nm;

	nm = (BurnerRockNM *) susp;
	if (nm->flags & (BURNER_NM_CURRENT|BURNER_NM_PARENT|BURNER_NM_NETWORK))
		return TRUE;

	len = susp->len - sizeof (BurnerRockNM);

	if (!len)
		return TRUE;

	/* should we concatenate ? */
	if (ctx->rr_name
	&&  ctx->rr_name_continue)
		rr_name = g_strdup_printf ("%s%.*s",
					   ctx->rr_name,
					   len,
					   nm->name);
	else
		rr_name = g_strndup (nm->name, len);

	if (ctx->rr_name)
		g_free (ctx->rr_name);

	ctx->rr_name = rr_name;
	ctx->rr_name_continue = (nm->flags & BURNER_NM_CONTINUE);

	return TRUE;
}

/**
 * All these entries are defined in rrip standards.
 * They are mainly used for directory/file relocation.
 */
static gboolean
burner_susp_CL (BurnerSusp *susp,
		 BurnerSuspCtx *ctx)
{
	BurnerRockCL *cl;

	/* Child Link */
	cl = (BurnerRockCL *) susp;
	ctx->CL_address = burner_iso9660_get_733_val (cl->location);

	return TRUE;
}

static gboolean
burner_susp_RE (BurnerSusp *susp,
		 BurnerSuspCtx *ctx)
{
	/* Nothing to see here really this is just an indication.
	 * Check consistency though BP3 = "4" and BP4 = "1" */
	if (susp->len != 4 || susp->version != 1)
		return FALSE;

	ctx->has_RE = TRUE;
	return TRUE;
}

static gboolean
burner_susp_PL (BurnerSusp *susp,
		 BurnerSuspCtx *ctx)
{
	BurnerRockPL *pl;

	if (ctx->rr_parent)
		return FALSE;

	/* That's to store original parent address of this node before it got
	 * relocated. */
	pl = (BurnerRockPL *) susp;
	ctx->rr_parent = burner_iso9660_get_733_val (pl->location);
	return TRUE;
}

gboolean
burner_susp_read (BurnerSuspCtx *ctx, gchar *buffer, guint max)
{
	BurnerSusp *susp;
	gint offset;

	if (max <= 0)
		return TRUE;

	if (!buffer)
		return FALSE;

	susp = (BurnerSusp *) buffer;
	if (susp->len > max)
		goto error;

	offset = 0;

	/* we are only interested in some of the Rock Ridge entries:
	 * name, relocation for directories */
	while (susp->len) {
		gboolean result;

		result = TRUE;
		if (!memcmp (susp->signature, "SP", 2))
			result = burner_susp_SP (susp, ctx);
		/* Continuation area */
		else if (!memcmp (susp->signature, "CE", 2))
			result = burner_susp_CE (susp, ctx);
		/* This is to indicate that we're using Rock Ridge */
		else if (!memcmp (susp->signature, "ER", 2))
			result = burner_susp_ER (susp, ctx);
		else if (!memcmp (susp->signature, "NM", 2))
			result = burner_susp_NM (susp, ctx);
		else if (!memcmp (susp->signature, "CL", 2))
			result = burner_susp_CL (susp, ctx);
		else if (!memcmp (susp->signature, "PL", 2))
			result = burner_susp_PL (susp, ctx);

		/* This is to indicate that the entry has been relocated */
		else if (!memcmp (susp->signature, "RE", 2))
			result = burner_susp_RE (susp, ctx);

		if (!result)
			goto error;

		offset += susp->len;
		/* there may be one byte for padding to an even number */
		if (offset == max || offset + 1 == max)
			break;

		if (offset > max)
			goto error;

		susp = (BurnerSusp *) (buffer + offset);

		/* some coherency checks */
		if (offset + susp->len > max)
			goto error;
	}

	return TRUE;

error:

	/* clean up context */
	burner_susp_ctx_clean (ctx);
	return FALSE;
}
