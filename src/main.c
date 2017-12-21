/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */

/*
 * Burner is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * Burner is distributed in the hope that it will be useful,
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
/***************************************************************************
 *            main.c
 *
 *  Sat Jun 11 12:00:29 2005
 *  Copyright  2005  Philippe Rouquier	
 *  <burner-app@wanadoo.fr>
 ****************************************************************************/

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include<stdio.h>
#include <string.h>
#include <locale.h>

#include <glib.h>
#include <glib/gi18n-lib.h>

#include <gtk/gtk.h>

#include <gst/gst.h>


#include "burner-burn-lib.h"
#include "burner-misc.h"

#include "burner-multi-dnd.h"
#include "burner-app.h"
#include "burner-cli.h"

static BurnerApp *current_app = NULL;

/**
 * This is actually declared in burner-app.h
 */

BurnerApp *
burner_app_get_default (void)
{
	return current_app;
}

int
main (int argc, char **argv)
{
	GApplication *gapp = NULL;
	GOptionContext *context;

	GdkScreen *screen = NULL;
	GtkCssProvider *provider = NULL;
	GFile *file = NULL;
	GError *css_error = NULL;

#ifdef ENABLE_NLS
	bindtextdomain (GETTEXT_PACKAGE, PACKAGE_LOCALE_DIR);
	bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
	textdomain (GETTEXT_PACKAGE);
#endif

	g_thread_init (NULL);
	g_type_init ();

	/* Though we use gtk_get_option_group we nevertheless want gtk+ to be
	 * in a usable state to display our error messages while burner
	 * specific options are parsed. Otherwise on error that crashes. */
	gtk_init (&argc, &argv);

	memset (&cmd_line_options, 0, sizeof (cmd_line_options));

	context = g_option_context_new (_("[URI] [URI] …"));
	g_option_context_add_main_entries (context,
					   prog_options,
					   GETTEXT_PACKAGE);
	g_option_context_set_translation_domain (context, GETTEXT_PACKAGE);

	g_option_context_add_group (context, gtk_get_option_group (TRUE));
	g_option_context_add_group (context, burner_media_get_option_group ());
	g_option_context_add_group (context, burner_burn_library_get_option_group ());
	g_option_context_add_group (context, burner_utils_get_option_group ());
	g_option_context_add_group (context, gst_init_get_option_group ());
	if (g_option_context_parse (context, &argc, &argv, NULL) == FALSE) {
		g_print (_("Please type \"%s --help\" to see all available options\n"), argv [0]);
		g_option_context_free (context);
		return FALSE;
	}
	g_option_context_free (context);

        /* gtk3.0+ use css */
        screen = gdk_screen_get_default();
        file = g_file_new_for_path("/usr/share/burner/style.css");
        if (file != NULL)
        {
            if (provider == NULL)
            {
                provider = gtk_css_provider_new();
            }
            gtk_css_provider_load_from_file(provider, file, NULL);
            gtk_style_context_add_provider_for_screen(screen, GTK_STYLE_PROVIDER(provider), GTK_STYLE_PROVIDER_PRIORITY_USER);
            gtk_style_context_reset_widgets(screen);
            gsize bytes_written, bytes_read;
            gtk_css_provider_load_from_path(provider, g_filename_to_utf8("/usr/share/burner/style.css", strlen("/usr/share/burner/style.css"), &bytes_read, &bytes_written, &css_error), NULL);
        }
        else
        {
            if (NULL != provider)
            {
                gtk_style_context_remove_provider_for_screen(screen, GTK_STYLE_PROVIDER(provider));
                g_object_unref(provider);
                provider = NULL;
            }
            gtk_style_context_reset_widgets(screen);
        }
        if (NULL != css_error)
        {
            g_clear_error(&css_error);
        }
        if (NULL != file)
        {
            g_object_unref(file);
            file = NULL;
        }

	if (cmd_line_options.not_unique == FALSE) {
		GError *error = NULL;
		/* Create GApplication and check if there is a process running already */
		gapp = g_application_new ("org.gnome.Burner", G_APPLICATION_FLAGS_NONE);

		if (!g_application_register (gapp, NULL, &error)) {
			g_warning ("Burner registered");
			g_error_free (error);
			return 1;
		}

		if (g_application_get_is_remote (gapp)) {
			g_warning ("An instance of Burner is already running, exiting");
			return 0;
		}
	}

	burner_burn_library_start (&argc, &argv);
	burner_enable_multi_DND ();

	current_app = burner_app_new (gapp);
	if (current_app == NULL)
		return 1;

	burner_cli_apply_options (current_app);

	g_object_unref (current_app);
	current_app = NULL;

	burner_burn_library_stop ();

	gst_deinit ();

	return 0;
}
