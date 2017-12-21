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

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <string.h>
#include <stdio.h>

#include <glib.h>
#include <glib/gi18n-lib.h>

#include "burner-media.h"
#include "burner-media-private.h"

static gboolean debug = 0;

#define BURNER_MEDIUM_TRUE_RANDOM_WRITABLE(media)				\
	(BURNER_MEDIUM_IS (media, BURNER_MEDIUM_DVDRW_RESTRICTED) ||		\
	 BURNER_MEDIUM_IS (media, BURNER_MEDIUM_DVDRW_PLUS) ||		\
	 BURNER_MEDIUM_IS (media, BURNER_MEDIUM_DVD_RAM) || 			\
	 BURNER_MEDIUM_IS (media, BURNER_MEDIUM_BDRE))

static const GOptionEntry options [] = {
	{ "burner-media-debug", 0, 0, G_OPTION_ARG_NONE, &debug,
	  N_("Display debug statements on stdout for Burner media library"),
	  NULL },
	{ NULL }
};

void
burner_media_library_set_debug (gboolean value)
{
	debug = value;
}

static GSList *
burner_media_add_to_list (GSList *retval,
			   BurnerMedia media)
{
	retval = g_slist_prepend (retval, GINT_TO_POINTER (media));
	return retval;
}

static GSList *
burner_media_new_status (GSList *retval,
			  BurnerMedia media,
			  BurnerMedia type)
{
	if ((type & BURNER_MEDIUM_BLANK)
	&& !(media & BURNER_MEDIUM_ROM)) {
		/* If media is blank there is no other possible property. */
		retval = burner_media_add_to_list (retval,
						    media|
						    BURNER_MEDIUM_BLANK);

		/* NOTE about BR-R they can be "formatted" but they are never
		 * unformatted since by default they'll be used as sequential.
		 * We don't consider DVD-RW as unformatted as in this case
		 * they are treated as DVD-RW in sequential mode and therefore
		 * don't require any formatting. */
		if (!(media & BURNER_MEDIUM_RAM)
		&&   (BURNER_MEDIUM_IS (media, BURNER_MEDIUM_DVDRW_PLUS)
		||    BURNER_MEDIUM_IS (media, BURNER_MEDIUM_BD|BURNER_MEDIUM_REWRITABLE))) {
			if (type & BURNER_MEDIUM_UNFORMATTED)
				retval = burner_media_add_to_list (retval,
								    media|
								    BURNER_MEDIUM_BLANK|
								    BURNER_MEDIUM_UNFORMATTED);
		}
	}

	if (type & BURNER_MEDIUM_CLOSED) {
		if (media & (BURNER_MEDIUM_DVD|BURNER_MEDIUM_BD))
			retval = burner_media_add_to_list (retval,
							    media|
							    BURNER_MEDIUM_CLOSED|
							    (type & BURNER_MEDIUM_HAS_DATA)|
							    (type & BURNER_MEDIUM_PROTECTED));
		else {
			if (type & BURNER_MEDIUM_HAS_AUDIO)
				retval = burner_media_add_to_list (retval,
								    media|
								    BURNER_MEDIUM_CLOSED|
								    BURNER_MEDIUM_HAS_AUDIO);
			if (type & BURNER_MEDIUM_HAS_DATA)
				retval = burner_media_add_to_list (retval,
								    media|
								    BURNER_MEDIUM_CLOSED|
								    BURNER_MEDIUM_HAS_DATA);
			if (BURNER_MEDIUM_IS (type, BURNER_MEDIUM_HAS_AUDIO|BURNER_MEDIUM_HAS_DATA))
				retval = burner_media_add_to_list (retval,
								    media|
								    BURNER_MEDIUM_CLOSED|
								    BURNER_MEDIUM_HAS_DATA|
								    BURNER_MEDIUM_HAS_AUDIO);
		}
	}

	if ((type & BURNER_MEDIUM_APPENDABLE)
	&& !(media & BURNER_MEDIUM_ROM)
	&& !BURNER_MEDIUM_TRUE_RANDOM_WRITABLE (media)) {
		if (media & (BURNER_MEDIUM_BD|BURNER_MEDIUM_DVD))
			retval = burner_media_add_to_list (retval,
							    media|
							    BURNER_MEDIUM_APPENDABLE|
							    BURNER_MEDIUM_HAS_DATA);
		else {
			if (type & BURNER_MEDIUM_HAS_AUDIO)
				retval = burner_media_add_to_list (retval,
								    media|
								    BURNER_MEDIUM_APPENDABLE|
								    BURNER_MEDIUM_HAS_AUDIO);
			if (type & BURNER_MEDIUM_HAS_DATA)
				retval = burner_media_add_to_list (retval,
								    media|
								    BURNER_MEDIUM_APPENDABLE|
								    BURNER_MEDIUM_HAS_DATA);
			if (BURNER_MEDIUM_IS (type, BURNER_MEDIUM_HAS_AUDIO|BURNER_MEDIUM_HAS_DATA))
				retval = burner_media_add_to_list (retval,
								    media|
								    BURNER_MEDIUM_HAS_DATA|
								    BURNER_MEDIUM_APPENDABLE|
								    BURNER_MEDIUM_HAS_AUDIO);
		}
	}

	return retval;
}

static GSList *
burner_media_new_attribute (GSList *retval,
			     BurnerMedia media,
			     BurnerMedia type)
{
	/* NOTE: never reached by BDs, ROMs (any) or Restricted Overwrite
	 * and DVD- dual layer */

	/* NOTE: there is no dual layer DVD-RW */
	if (type & BURNER_MEDIUM_REWRITABLE)
		retval = burner_media_new_status (retval,
						   media|BURNER_MEDIUM_REWRITABLE,
						   type);

	if (type & BURNER_MEDIUM_WRITABLE)
		retval = burner_media_new_status (retval,
						   media|BURNER_MEDIUM_WRITABLE,
						   type);

	return retval;
}

static GSList *
burner_media_new_subtype (GSList *retval,
			   BurnerMedia media,
			   BurnerMedia type)
{
	if (media & BURNER_MEDIUM_BD) {
		/* There seems to be Dual layers BDs as well */

		if (type & BURNER_MEDIUM_ROM) {
			retval = burner_media_new_status (retval,
							   media|
							   BURNER_MEDIUM_ROM,
							   type);
			if (type & BURNER_MEDIUM_DUAL_L)
				retval = burner_media_new_status (retval,
								   media|
								   BURNER_MEDIUM_ROM|
								   BURNER_MEDIUM_DUAL_L,
								   type);
		}

		if (type & BURNER_MEDIUM_RANDOM) {
			retval = burner_media_new_status (retval,
							   media|
							   BURNER_MEDIUM_RANDOM|
							   BURNER_MEDIUM_WRITABLE,
							   type);
			if (type & BURNER_MEDIUM_DUAL_L)
				retval = burner_media_new_status (retval,
								   media|
								   BURNER_MEDIUM_RANDOM|
								   BURNER_MEDIUM_WRITABLE|
								   BURNER_MEDIUM_DUAL_L,
								   type);
		}

		if (type & BURNER_MEDIUM_SRM) {
			retval = burner_media_new_status (retval,
							   media|
							   BURNER_MEDIUM_SRM|
							   BURNER_MEDIUM_WRITABLE,
							   type);
			if (type & BURNER_MEDIUM_DUAL_L)
				retval = burner_media_new_status (retval,
								   media|
								   BURNER_MEDIUM_SRM|
								   BURNER_MEDIUM_WRITABLE|
								   BURNER_MEDIUM_DUAL_L,
								   type);
		}

		if (type & BURNER_MEDIUM_POW) {
			retval = burner_media_new_status (retval,
							   media|
							   BURNER_MEDIUM_POW|
							   BURNER_MEDIUM_WRITABLE,
							   type);
			if (type & BURNER_MEDIUM_DUAL_L)
				retval = burner_media_new_status (retval,
								   media|
								   BURNER_MEDIUM_POW|
								   BURNER_MEDIUM_WRITABLE|
								   BURNER_MEDIUM_DUAL_L,
								   type);
		}

		/* BD-RE */
		if (type & BURNER_MEDIUM_REWRITABLE) {
			retval = burner_media_new_status (retval,
							   media|
							   BURNER_MEDIUM_REWRITABLE,
							   type);
			if (type & BURNER_MEDIUM_DUAL_L)
				retval = burner_media_new_status (retval,
								   media|
								   BURNER_MEDIUM_REWRITABLE|
								   BURNER_MEDIUM_DUAL_L,
								   type);
		}
	}

	if (media & BURNER_MEDIUM_DVD) {
		/* There is no such thing as DVD-RW DL nor DVD-RAM DL*/

		/* The following is always a DVD-R dual layer */
		if (type & BURNER_MEDIUM_JUMP)
			retval = burner_media_new_status (retval,
							   media|
							   BURNER_MEDIUM_JUMP|
							   BURNER_MEDIUM_DUAL_L|
							   BURNER_MEDIUM_WRITABLE,
							   type);

		if (type & BURNER_MEDIUM_SEQUENTIAL) {
			retval = burner_media_new_attribute (retval,
							      media|
							      BURNER_MEDIUM_SEQUENTIAL,
							      type);

			/* This one has to be writable only, no RW */
			if (type & BURNER_MEDIUM_DUAL_L)
				retval = burner_media_new_status (retval,
								   media|
								   BURNER_MEDIUM_SEQUENTIAL|
								   BURNER_MEDIUM_WRITABLE|
								   BURNER_MEDIUM_DUAL_L,
								   type);
		}

		/* Restricted Overwrite media are always rewritable */
		if (type & BURNER_MEDIUM_RESTRICTED)
			retval = burner_media_new_status (retval,
							   media|
							   BURNER_MEDIUM_RESTRICTED|
							   BURNER_MEDIUM_REWRITABLE,
							   type);

		if (type & BURNER_MEDIUM_PLUS) {
			retval = burner_media_new_attribute (retval,
							      media|
							      BURNER_MEDIUM_PLUS,
							      type);

			if (type & BURNER_MEDIUM_DUAL_L)
				retval = burner_media_new_attribute (retval,
								      media|
								      BURNER_MEDIUM_PLUS|
								      BURNER_MEDIUM_DUAL_L,
								      type);

		}

		if (type & BURNER_MEDIUM_ROM) {
			retval = burner_media_new_status (retval,
							   media|
							   BURNER_MEDIUM_ROM,
							   type);

			if (type & BURNER_MEDIUM_DUAL_L)
				retval = burner_media_new_status (retval,
								   media|
								   BURNER_MEDIUM_ROM|
								   BURNER_MEDIUM_DUAL_L,
								   type);
		}

		/* RAM media are always rewritable */
		if (type & BURNER_MEDIUM_RAM)
			retval = burner_media_new_status (retval,
							   media|
							   BURNER_MEDIUM_RAM|
							   BURNER_MEDIUM_REWRITABLE,
							   type);
	}

	return retval;
}

GSList *
burner_media_get_all_list (BurnerMedia type)
{
	GSList *retval = NULL;

	if (type & BURNER_MEDIUM_FILE)
		retval = burner_media_add_to_list (retval, BURNER_MEDIUM_FILE);					       

	if (type & BURNER_MEDIUM_CD) {
		if (type & BURNER_MEDIUM_ROM)
			retval = burner_media_new_status (retval,
							   BURNER_MEDIUM_CD|
							   BURNER_MEDIUM_ROM,
							   type);

		retval = burner_media_new_attribute (retval,
						      BURNER_MEDIUM_CD,
						      type);
	}

	if (type & BURNER_MEDIUM_DVD)
		retval = burner_media_new_subtype (retval,
						    BURNER_MEDIUM_DVD,
						    type);


	if (type & BURNER_MEDIUM_BD)
		retval = burner_media_new_subtype (retval,
						    BURNER_MEDIUM_BD,
						    type);

	return retval;
}

GQuark
burner_media_quark (void)
{
	static GQuark quark = 0;

	if (!quark)
		quark = g_quark_from_static_string ("BurnerMediaError");

	return quark;
}

void
burner_media_to_string (BurnerMedia media,
			 gchar *buffer)
{
	if (media & BURNER_MEDIUM_FILE)
		strcat (buffer, "file ");

	if (media & BURNER_MEDIUM_CD)
		strcat (buffer, "CD ");

	if (media & BURNER_MEDIUM_DVD)
		strcat (buffer, "DVD ");

	if (media & BURNER_MEDIUM_RAM)
		strcat (buffer, "RAM ");

	if (media & BURNER_MEDIUM_BD)
		strcat (buffer, "BD ");

	if (media & BURNER_MEDIUM_DUAL_L)
		strcat (buffer, "DL ");

	/* DVD subtypes */
	if (media & BURNER_MEDIUM_PLUS)
		strcat (buffer, "+ ");

	if (media & BURNER_MEDIUM_SEQUENTIAL)
		strcat (buffer, "- (sequential) ");

	if (media & BURNER_MEDIUM_RESTRICTED)
		strcat (buffer, "- (restricted) ");

	if (media & BURNER_MEDIUM_JUMP)
		strcat (buffer, "- (jump) ");

	/* BD subtypes */
	if (media & BURNER_MEDIUM_SRM)
		strcat (buffer, "SRM ");

	if (media & BURNER_MEDIUM_POW)
		strcat (buffer, "POW ");

	if (media & BURNER_MEDIUM_RANDOM)
		strcat (buffer, "RANDOM ");

	/* discs attributes */
	if (media & BURNER_MEDIUM_REWRITABLE)
		strcat (buffer, "RW ");

	if (media & BURNER_MEDIUM_WRITABLE)
		strcat (buffer, "W ");

	if (media & BURNER_MEDIUM_ROM)
		strcat (buffer, "ROM ");

	/* status of the disc */
	if (media & BURNER_MEDIUM_CLOSED)
		strcat (buffer, "closed ");

	if (media & BURNER_MEDIUM_BLANK)
		strcat (buffer, "blank ");

	if (media & BURNER_MEDIUM_APPENDABLE)
		strcat (buffer, "appendable ");

	if (media & BURNER_MEDIUM_PROTECTED)
		strcat (buffer, "protected ");

	if (media & BURNER_MEDIUM_HAS_DATA)
		strcat (buffer, "with data ");

	if (media & BURNER_MEDIUM_HAS_AUDIO)
		strcat (buffer, "with audio ");

	if (media & BURNER_MEDIUM_UNFORMATTED)
		strcat (buffer, "Unformatted ");
}

/**
 * burner_media_get_option_group:
 *
 * Returns a GOptionGroup for the commandline arguments recognized by libburner-media.
 * You should add this to your GOptionContext if your are using g_option_context_parse ()
 * to parse your commandline arguments.
 *
 * Return value: a #GOptionGroup *
 **/
GOptionGroup *
burner_media_get_option_group (void)
{
	GOptionGroup *group;

	group = g_option_group_new ("burner-media",
				    N_("Burner optical media library"),
				    N_("Display options for Burner media library"),
				    NULL,
				    NULL);
	g_option_group_add_entries (group, options);
	return group;
}

void
burner_media_message (const gchar *location,
		       const gchar *format,
		       ...)
{
	va_list arg_list;
	gchar *format_real;

	if (!debug)
		return;

	format_real = g_strdup_printf ("BurnerMedia: (at %s) %s\n",
				       location,
				       format);

	va_start (arg_list, format);
	vprintf (format_real, arg_list);
	va_end (arg_list);

	g_free (format_real);
}

#include <gtk/gtk.h>

#include "burner-medium-monitor.h"

static BurnerMediumMonitor *default_monitor = NULL;

/**
 * burner_media_library_start:
 *
 * Initialize the library.
 *
 * You should call this function before using any other from the library.
 *
 * Rename to: init
 **/
void
burner_media_library_start (void)
{
	if (default_monitor) {
		g_object_ref (default_monitor);
		return;
	}

	BURNER_MEDIA_LOG ("Initializing Burner-media %i.%i.%i",
	                    BURNER_MAJOR_VERSION,
	                    BURNER_MINOR_VERSION,
	                    BURNER_SUB);

#if defined(HAVE_STRUCT_USCSI_CMD)
	/* Work around: because on OpenSolaris burner posiblely be run
	 * as root for a user with 'Primary Administrator' profile,
	 * a root dbus session will be autospawned at that time.
	 * This fix is to work around
	 * http://bugzilla.gnome.org/show_bug.cgi?id=526454
	 */
	g_setenv ("DBUS_SESSION_BUS_ADDRESS", "autolaunch:", TRUE);
#endif

	/* Initialize external libraries (threads... */
	if (!g_thread_supported ())
		g_thread_init (NULL);

	/* Initialize i18n */
	bindtextdomain (GETTEXT_PACKAGE, PACKAGE_LOCALE_DIR);
	bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");

	/* Initialize icon-theme */
	gtk_icon_theme_append_search_path (gtk_icon_theme_get_default (),
					   BURNER_DATADIR "/icons");

	/* Take a reference for the monitoring library */
	default_monitor = burner_medium_monitor_get_default ();
}

/**
 * burner_media_library_stop:
 *
 * De-initialize the library once you do not need the library anymore.
 *
 * Rename to: deinit
 **/
void
burner_media_library_stop (void)
{
	g_object_unref (default_monitor);
	default_monitor = NULL;
}
