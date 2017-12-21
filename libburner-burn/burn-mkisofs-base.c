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

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>


#include <glib.h>
#include <glib-object.h>
#include <glib/gi18n-lib.h>
#include <glib/gstdio.h>

#include "burn-basics.h"
#include "burn-debug.h"
#include "burner-track.h"
#include "burn-mkisofs-base.h"

struct _BurnerMkisofsBase {
	const gchar *emptydir;
	const gchar *videodir;

	gint grafts_fd;
	gint excluded_fd;

	GHashTable *grafts;

	guint found_video_ts:1;
	guint use_joliet:1;
};
typedef struct _BurnerMkisofsBase BurnerMkisofsBase;

struct _BurnerWriteGraftData {
	BurnerMkisofsBase *base;
	GError **error;
};
typedef struct _BurnerWriteGraftData BurnerWriteGraftData;

static void
burner_mkisofs_base_clean (BurnerMkisofsBase *base)
{
	/* now we clean base we have the most important :
	 * graft and excluded list, flags and that's what
	 * we're going to use when we'll start the image 
	 * creation */
	if (base->grafts_fd)
		close (base->grafts_fd);
	if (base->excluded_fd)
		close (base->excluded_fd);
	if (base->grafts) {
		g_hash_table_destroy (base->grafts);
		base->grafts = NULL;
	}
}

static BurnerBurnResult
_write_line (int fd, const gchar *filepath, GError **error)
{
	gint len;
	gint w_len;

	if (lseek (fd, 0, SEEK_CUR)
	&&  write (fd, "\n", 1) != 1) {
		g_set_error (error,
			     BURNER_BURN_ERROR,
			     BURNER_BURN_ERROR_GENERAL,
			     "%s",
			     g_strerror (errno));
		return BURNER_BURN_ERR;
	}

	len = strlen (filepath);
	w_len = write (fd, filepath, len);

	if (w_len != len) {
		g_set_error (error,
			     BURNER_BURN_ERROR,
			     BURNER_BURN_ERROR_GENERAL,
			     "%s",
			     g_strerror (errno));
		return BURNER_BURN_ERR;
	}

	return BURNER_BURN_OK;
}

static BurnerBurnResult
burner_mkisofs_base_write_excluded (BurnerMkisofsBase *base,
				     const gchar *uri,
				     GError **error)
{
	gint num;
	gint forbidden;
	gchar *character;
	gchar *localpath;
	BurnerBurnResult result = BURNER_BURN_OK;

	/* make sure uri is local: otherwise error out */
	/* FIXME: uri can be path or URI? problem with graft->uri */
	if (uri && uri [0] == '/') {
		localpath = g_strdup (uri);
	}
	else if (uri && g_str_has_prefix (uri, "file://")) {
		gchar *unescaped_uri;

		unescaped_uri = g_uri_unescape_string (uri, NULL);
		localpath = g_filename_from_uri (unescaped_uri, NULL, NULL);
		g_free (unescaped_uri);

		if (!localpath)
			localpath = g_filename_from_uri (uri, NULL, NULL);					  
	}
	else {
		BURNER_BURN_LOG ("File not stored locally %s", uri);
		g_set_error (error,
			     BURNER_BURN_ERROR,
			     BURNER_BURN_ERROR_FILE_NOT_LOCAL,
			     _("The file is not stored locally"));
		return BURNER_BURN_ERR;
	}

	if (!localpath) {
		BURNER_BURN_LOG ("Localpath is NULL");
		return BURNER_BURN_ERR;
	}

	/* we need to escape some characters like []\? since in this file we
	 * can use glob like expressions. */
	character = localpath;
	forbidden = 0;
	num = 0;

	while (character [0]) {
		if (character [0] == '['
		||  character [0] == ']'
		||  character [0] == '?'
		||  character [0] == '\\')
			forbidden++;

		num++;
		character++;
	}

	if (forbidden) {
		gchar *tmp;
		gint i;

		tmp = g_new0 (gchar, num + forbidden + 1);
		character = tmp;

		for (i = 0; i < num; i++) {
			if (localpath [i] == '['
			||  localpath [i] == ']'
			||  localpath [i] == '?'
			||  localpath [i] == '\\') {
				character [i] = '\\';
				character++;
			}
			character [i] = localpath [i];
		}

		BURNER_BURN_LOG ("Escaped path %s into %s", localpath, tmp);
		g_free (localpath);
		localpath = tmp;
	}

	/* we just ignore if localpath is NULL:
	 * - it could be a non local whose graft point couldn't be downloaded */
	if (localpath)
		result = _write_line (base->excluded_fd, localpath, error);

	g_free (localpath);
	return result;
}

static gchar *
_escape_path (const gchar *str)
{
	gchar *escaped, *d;
	const gchar *s;
	gint len;

	s = str;
	len = 1;
	while (*s != 0) {
		if (*s == '\\' || *s == '=') {
			len++;
		}

		len++;
		s++;
	}

	escaped = g_malloc (len);

	s = str;
	d = escaped;
	while (*s != 0) {
		if (*s == '\\' || *s == '=') {
			*d++ = '\\';
		}

		*d++ = *s++;
	}
	*d = 0;

	return escaped;
}

static gchar *
_build_graft_point (const gchar *uri, const gchar *discpath)
{
	gchar *escaped_discpath;
	gchar *graft_point;
	gchar *path;

	if (uri == NULL || discpath == NULL)
		return NULL;

	/* make up the graft point */
	if (*uri != '/')
		path = g_filename_from_uri (uri, NULL, NULL);
	else
		path = g_strdup (uri);

	if (discpath) {
		gchar *escaped_path;

		/* There is a graft because either it's not at the root of the 
		 * disc or because its name has changed. */
		escaped_path = _escape_path (path);
		g_free (path);

		escaped_discpath = _escape_path (discpath);
		graft_point = g_strconcat (escaped_discpath,
					   "=",
					   escaped_path,
					   NULL);
		g_free (escaped_path);
		g_free (escaped_discpath);
	}
	else
		graft_point = path;

	return graft_point;
}

static BurnerBurnResult
burner_mkisofs_base_write_graft (BurnerMkisofsBase *base,
				  const gchar *uri,
				  const gchar *disc_path,
				  GError **error)
{
	gchar *graft_point;
	BurnerBurnResult result;

	/* build up graft and write it */
	graft_point = _build_graft_point (uri, disc_path);
	if (!graft_point) {
		g_set_error (error,
			     BURNER_BURN_ERROR,
			     BURNER_BURN_ERROR_GENERAL,
			     /* Translators: Error message saying no graft point
			      * is specified. A graft point is the path (on the
			      * disc) where a file from any source will be added
			      * ("grafted") */
			     _("An internal error occurred"));
		return BURNER_BURN_ERR;
	}

	result = _write_line (base->grafts_fd, graft_point, error);
	g_free (graft_point);
	if (result != BURNER_BURN_OK)
		return result;

	return BURNER_BURN_OK;
}

static gboolean
_foreach_write_grafts (const gchar *uri,
		       GSList *grafts,
		       BurnerWriteGraftData *data)
{
	BurnerBurnResult result;
	BurnerGraftPt *graft;

	for (; grafts; grafts = grafts->next) {
		graft = grafts->data;

		if (!graft->path) {
			result = burner_mkisofs_base_write_graft (data->base,
								   graft->uri,
								   NULL,
								   data->error);
			if (result != BURNER_BURN_OK)
				return TRUE;

			continue;
		}

		result = burner_mkisofs_base_write_graft (data->base,
							   graft->uri,
							   graft->path,
							   data->error);
		if (result != BURNER_BURN_OK)
			return TRUE;
	}

	return FALSE;
}

static BurnerBurnResult
burner_mkisofs_base_write_grafts (BurnerMkisofsBase *base,
				   GError **error)
{
	BurnerWriteGraftData callback_data;
	gpointer result;

	callback_data.error = error;
	callback_data.base = base;

	result = g_hash_table_find (base->grafts,
				    (GHRFunc) _foreach_write_grafts,
				    &callback_data);

	if (result)
		return BURNER_BURN_ERR;

	return BURNER_BURN_OK;
}

static BurnerBurnResult
burner_mkisofs_base_create_video_empty (BurnerMkisofsBase *base,
					 const gchar *disc_path)
{
	gchar *path;

	BURNER_BURN_LOG ("Creating an empty Video or Audio directory");

	if (base->found_video_ts)
		return BURNER_BURN_OK;

	base->found_video_ts = TRUE;

	path = g_build_path (G_DIR_SEPARATOR_S,
			     base->videodir,
			     disc_path,
			     NULL);

	g_mkdir_with_parents (path, S_IRWXU);
	g_free (path);

	return BURNER_BURN_OK;
}

static BurnerBurnResult
burner_mkisofs_base_empty_directory (BurnerMkisofsBase *base,
				      const gchar *disc_path,
				      GError **error)
{
	BurnerBurnResult result;
	gchar *graft_point;

	/* This is a special case when the URI is NULL which can happen mainly
	 * when we have to deal with burn:// uri. */
	if (base->videodir) {
		/* try with "VIDEO_TS", "VIDEO_TS/", "VIDEO_TS/" and "/VIDEO_TS/"
		 * to make sure we don't miss one */
		if (!strcmp (disc_path, "VIDEO_TS")
		||  !strcmp (disc_path, "/VIDEO_TS")
		||  !strcmp (disc_path, "VIDEO_TS/")
		||  !strcmp (disc_path, "/VIDEO_TS/")) {
			burner_mkisofs_base_create_video_empty (base, disc_path);

			/* NOTE: joliet cannot be used in this case so that's
			 * perfectly fine to forget about it. */
			return BURNER_BURN_OK;
		}
	}

	/* Special case for uri = NULL; that is treated as if it were a directory */
	graft_point = _build_graft_point (base->emptydir, disc_path);
	result = _write_line (base->grafts_fd, graft_point, error);
	g_free (graft_point);

	return result;
}

static BurnerBurnResult
burner_mkisofs_base_process_video_graft (BurnerMkisofsBase *base,
					  BurnerGraftPt *graft,
					  GError **error)
{
	gchar *link_path;
	gchar *path;
	int res;

	/* Make sure it's a path and not a URI */
	if (!strncmp (graft->uri, "file:", 5))
		path = g_filename_from_uri (graft->uri, NULL, NULL);
	else
		path = g_strdup (graft->uri);

	if (g_str_has_suffix (path, G_DIR_SEPARATOR_S)) {
		gchar *tmp;

		tmp = g_strndup (path, strlen (path) - strlen (G_DIR_SEPARATOR_S));
		g_free (path);
		path = tmp;
	}

	link_path = g_build_path (G_DIR_SEPARATOR_S,
				  base->videodir,
				  graft->path,
				  NULL);

	if (g_str_has_suffix (link_path, G_DIR_SEPARATOR_S)) {
		gchar *tmp;

		tmp = g_strndup (link_path, strlen (link_path) - strlen (G_DIR_SEPARATOR_S));
		g_free (link_path);
		link_path = tmp;
	}

	BURNER_BURN_LOG ("Linking %s to %s", link_path, path);
	res = symlink (path, link_path);

	g_free (path);
	g_free (link_path);

	if (res) {
		g_set_error (error,
			     BURNER_BURN_ERROR,
			     BURNER_BURN_ERROR_GENERAL,
			     "%s",
			     g_strerror (errno));
		return BURNER_BURN_ERR;
	}

	return BURNER_BURN_OK;
}

static BurnerBurnResult
burner_mkisofs_base_add_graft (BurnerMkisofsBase *base,
				BurnerGraftPt *graft,
				GError **error)
{
	GSList *list;

	/* check the file is local */
	if (graft->uri
	&&  graft->uri [0] != '/'
	&& !g_str_has_prefix (graft->uri, "file://")) {
		/* Error out, files must be local */
		BURNER_BURN_LOG ("File not stored locally %s", graft->uri);
		g_set_error (error,
			     BURNER_BURN_ERROR,
			     BURNER_BURN_ERROR_FILE_NOT_LOCAL,
			     _("The file is not stored locally"));
		return BURNER_BURN_ERR;
	}

	/* This is a special case for VIDEO images. Given the tests I performed,
	 * the option --dvd-video requires the parent directory of VIDEO_TS and
	 * AUDIO_TS to be passed. If each of these two directories are passed
	 * as an option it will fail.
	 * One workaround is to create a fake directory for VIDEO_TS and
	 * AUDIO_TS and add hardlinks inside pointing to these two directories.
	 * As this parent directory is a temporary directory it must have been
	 * passed by the calling plugins. */
	if (base->videodir) {
		BurnerBurnResult res;
		gchar *parent;

		/* try with "VIDEO_TS", "VIDEO_TS/", "VIDEO_TS/" and "/VIDEO_TS/"
		 * to make sure we don't miss one */
		if (!strcmp (graft->path, "VIDEO_TS")
		||  !strcmp (graft->path, "/VIDEO_TS")
		||  !strcmp (graft->path, "VIDEO_TS/")
		||  !strcmp (graft->path, "/VIDEO_TS/")) {
			res = burner_mkisofs_base_process_video_graft (base, graft, error);
			if (res != BURNER_BURN_OK)
				return res;

			base->found_video_ts = TRUE;
			return BURNER_BURN_OK;
		}

		if (!strcmp (graft->path, "AUDIO_TS")
		||  !strcmp (graft->path, "/AUDIO_TS")
		||  !strcmp (graft->path, "AUDIO_TS/")
		||  !strcmp (graft->path, "/AUDIO_TS/"))
			return burner_mkisofs_base_process_video_graft (base, graft, error);

		/* it could also be a direct child of the VIDEO_TS directory */
		parent = g_path_get_dirname (graft->path);
		if (!strcmp (parent, "VIDEO_TS")
		||  !strcmp (parent, "/VIDEO_TS")
		||  !strcmp (parent, "VIDEO_TS/")
		||  !strcmp (parent, "/VIDEO_TS/")) {
			if (!base->found_video_ts)
				burner_mkisofs_base_create_video_empty (base, parent);

			g_free (parent);
			return burner_mkisofs_base_process_video_graft (base, graft, error);
		}
		g_free (parent);
	}

	/* add the graft point */
	list = g_hash_table_lookup (base->grafts, graft->uri);
	if (list)
		g_hash_table_steal (base->grafts, graft->uri);

	list = g_slist_prepend (list, graft);
	g_hash_table_insert (base->grafts, graft->uri, list);

	return BURNER_BURN_OK;
}

BurnerBurnResult
burner_mkisofs_base_write_to_files (GSList *grafts,
				     GSList *excluded,
				     gboolean use_joliet,
				     const gchar *emptydir,
				     const gchar *videodir,
				     const gchar *grafts_path,
				     const gchar *excluded_path,
				     GError **error)
{
	gchar *uri;
	BurnerMkisofsBase base;
	BurnerBurnResult result;

	if (!grafts) {
		BURNER_BURN_LOG ("No graft passed");
		return BURNER_BURN_ERR;
	}

	/* initialize base */
	bzero (&base, sizeof (base));

	base.grafts_fd = open (grafts_path, O_WRONLY|O_TRUNC|O_EXCL);
	if (base.grafts_fd == -1) {
		g_set_error (error,
			     BURNER_BURN_ERROR,
			     BURNER_BURN_ERROR_GENERAL,
			     "%s",
			     g_strerror (errno));
		return BURNER_BURN_ERR;
	}

	base.excluded_fd = open (excluded_path, O_WRONLY|O_TRUNC|O_EXCL);
	if (base.excluded_fd == -1) {
		close (base.excluded_fd);
		g_set_error (error,
			     BURNER_BURN_ERROR,
			     BURNER_BURN_ERROR_GENERAL,
			     "%s",
			     g_strerror (errno));
		return BURNER_BURN_ERR;
	}

	base.use_joliet = use_joliet;
	base.emptydir = emptydir;
	base.videodir = videodir;

	base.grafts = g_hash_table_new_full (g_str_hash,
					     g_str_equal,
					     NULL,
					    (GDestroyNotify) g_slist_free);

	/* we analyse the graft points:
	 * first add graft points and excluded. At the same time create a hash 
	 * table in which key = uri and value = graft points, a list of all
	 * the uris that have been excluded.
	 * Once finished, for each excluded use the hash to see if there are not
	 * other paths at which the excluded uri must appear. If so, create an
	 * explicit graft point. */
	for (; grafts; grafts = grafts->next) {
		BurnerGraftPt *graft;

		graft = grafts->data;

		BURNER_BURN_LOG ("New graft %s %s", graft->uri, graft->path);

		if (!graft->uri) {
			result = burner_mkisofs_base_empty_directory (&base,
								       graft->path,
								       error);
			if (result != BURNER_BURN_OK)
				goto cleanup;

			continue;
		}

		result = burner_mkisofs_base_add_graft (&base,
							 graft,
							 error);
		if (result != BURNER_BURN_OK)
			goto cleanup;
	}

	/* simple check */
	if (base.videodir && !base.found_video_ts) {
		g_set_error (error,
			     BURNER_BURN_ERROR,
			     BURNER_BURN_ERROR_GENERAL,
			     _("VIDEO_TS directory is missing or invalid"));
		return BURNER_BURN_ERR;
	}

	/* write the grafts list */
	result = burner_mkisofs_base_write_grafts (&base, error);
	if (result != BURNER_BURN_OK)
		goto cleanup;

	/* write the global excluded files list */
	for (; excluded; excluded = excluded->next) {
		uri = excluded->data;
		if (!uri) {
			BURNER_BURN_LOG ("NULL URI");
			continue;
		}

		result = burner_mkisofs_base_write_excluded (&base,
							      uri,
							      error);
		if (result != BURNER_BURN_OK)
			goto cleanup;
	}

	burner_mkisofs_base_clean (&base);
	return BURNER_BURN_OK;


cleanup:

	burner_mkisofs_base_clean (&base);
	return result;
}
