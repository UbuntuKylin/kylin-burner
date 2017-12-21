/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/*
 * Burner
 * Copyright (C) Philippe Rouquier 2005-2010 <bonfire-app@wanadoo.fr>
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

#include "burner-drive-settings.h"
#include "burner-session.h"
#include "burner-session-helper.h"
#include "burner-drive-properties.h"

typedef struct _BurnerDriveSettingsPrivate BurnerDriveSettingsPrivate;
struct _BurnerDriveSettingsPrivate
{
	BurnerMedia dest_media;
	BurnerDrive *dest_drive;
	BurnerTrackType *src_type;

	GSettings *settings;
	GSettings *config_settings;
	BurnerBurnSession *session;
};

#define BURNER_DRIVE_SETTINGS_PRIVATE(o)  (G_TYPE_INSTANCE_GET_PRIVATE ((o), BURNER_TYPE_DRIVE_SETTINGS, BurnerDriveSettingsPrivate))

#define BURNER_SCHEMA_DRIVES			"org.gnome.burner.drives"
#define BURNER_DRIVE_PROPERTIES_PATH		"/org/gnome/burner/drives/"
#define BURNER_PROPS_FLAGS			"flags"
#define BURNER_PROPS_SPEED			"speed"

#define BURNER_SCHEMA_CONFIG			"org.gnome.burner.config"
#define BURNER_PROPS_TMP_DIR			"tmpdir"

#define BURNER_DEST_SAVED_FLAGS		(BURNER_DRIVE_PROPERTIES_FLAGS|BURNER_BURN_FLAG_MULTI)

G_DEFINE_TYPE (BurnerDriveSettings, burner_drive_settings, G_TYPE_OBJECT);

static GVariant *
burner_drive_settings_set_mapping_speed (const GValue *value,
                                          const GVariantType *variant_type,
                                          gpointer user_data)
{
	return g_variant_new_int32 (g_value_get_int64 (value) / 1000);
}

static gboolean
burner_drive_settings_get_mapping_speed (GValue *value,
                                          GVariant *variant,
                                          gpointer user_data)
{
	if (!g_variant_get_int32 (variant)) {
		BurnerDriveSettingsPrivate *priv;
		BurnerMedium *medium;
		BurnerDrive *drive;

		priv = BURNER_DRIVE_SETTINGS_PRIVATE (user_data);
		drive = burner_burn_session_get_burner (priv->session);
		medium = burner_drive_get_medium (drive);

		/* Must not be NULL since we only bind when a medium is available */
		g_assert (medium != NULL);

		g_value_set_int64 (value, burner_medium_get_max_write_speed (medium));
	}
	else
		g_value_set_int64 (value, g_variant_get_int32 (variant) * 1000);

	return TRUE;
}

static GVariant *
burner_drive_settings_set_mapping_flags (const GValue *value,
                                          const GVariantType *variant_type,
                                          gpointer user_data)
{
	return g_variant_new_int32 (g_value_get_int (value) & BURNER_DEST_SAVED_FLAGS);
}

static gboolean
burner_drive_settings_get_mapping_flags (GValue *value,
                                          GVariant *variant,
                                          gpointer user_data)
{
	BurnerBurnFlag flags;
	BurnerBurnFlag current_flags;
	BurnerDriveSettingsPrivate *priv;

	priv = BURNER_DRIVE_SETTINGS_PRIVATE (user_data);

	flags = g_variant_get_int32 (variant);
	if (burner_burn_session_same_src_dest_drive (priv->session)) {
		/* Special case */
		if (flags == 1) {
			flags = BURNER_BURN_FLAG_EJECT|
				BURNER_BURN_FLAG_BURNPROOF;
		}
		else
			flags &= BURNER_DEST_SAVED_FLAGS;

		flags |= BURNER_BURN_FLAG_BLANK_BEFORE_WRITE|
			 BURNER_BURN_FLAG_FAST_BLANK;
	}
	/* This is for the default value when the user has never used it */
	else if (flags == 1) {
		BurnerTrackType *source;

		flags = BURNER_BURN_FLAG_EJECT|
			BURNER_BURN_FLAG_BURNPROOF;

		source = burner_track_type_new ();
		burner_burn_session_get_input_type (BURNER_BURN_SESSION (priv->session), source);

		if (!burner_track_type_get_has_medium (source))
			flags |= BURNER_BURN_FLAG_NO_TMP_FILES;

		burner_track_type_free (source);
	}
	else
		flags &= BURNER_DEST_SAVED_FLAGS;

	current_flags = burner_burn_session_get_flags (BURNER_BURN_SESSION (priv->session));
	current_flags &= (~BURNER_DEST_SAVED_FLAGS);

	g_value_set_int (value, flags|current_flags);
	return TRUE;
}

static void
burner_drive_settings_bind_session (BurnerDriveSettings *self)
{
	BurnerDriveSettingsPrivate *priv;
	gchar *display_name;
	gchar *path;
	gchar *tmp;

	priv = BURNER_DRIVE_SETTINGS_PRIVATE (self);

	/* Get the drive name: it's done this way to avoid escaping */
	tmp = burner_drive_get_display_name (priv->dest_drive);
	display_name = g_strdup_printf ("drive-%u", g_str_hash (tmp));
	g_free (tmp);

	if (burner_track_type_get_has_medium (priv->src_type))
		path = g_strdup_printf ("%s%s/disc-%i/",
		                        BURNER_DRIVE_PROPERTIES_PATH,
		                        display_name,
		                        priv->dest_media);
	else if (burner_track_type_get_has_data (priv->src_type))
		path = g_strdup_printf ("%s%s/data-%i/",
		                        BURNER_DRIVE_PROPERTIES_PATH,
		                        display_name,
		                        priv->dest_media);
	else if (burner_track_type_get_has_image (priv->src_type))
		path = g_strdup_printf ("%s%s/image-%i/",
		                        BURNER_DRIVE_PROPERTIES_PATH,
		                        display_name,
		                        priv->dest_media);
	else if (burner_track_type_get_has_stream (priv->src_type))
		path = g_strdup_printf ("%s%s/audio-%i/",
		                        BURNER_DRIVE_PROPERTIES_PATH,
		                        display_name,
		                        priv->dest_media);
	else {
		g_free (display_name);
		return;
	}
	g_free (display_name);

	priv->settings = g_settings_new_with_path (BURNER_SCHEMA_DRIVES, path);
	g_free (path);

	g_settings_bind_with_mapping (priv->settings, BURNER_PROPS_SPEED,
	                              priv->session, "speed", G_SETTINGS_BIND_DEFAULT,
	                              burner_drive_settings_get_mapping_speed,
	                              burner_drive_settings_set_mapping_speed,
	                              self,
	                              NULL);

	g_settings_bind_with_mapping (priv->settings, BURNER_PROPS_FLAGS,
	                              priv->session, "flags", G_SETTINGS_BIND_DEFAULT,
	                              burner_drive_settings_get_mapping_flags,
	                              burner_drive_settings_set_mapping_flags,
	                              self,
	                              NULL);
}

static void
burner_drive_settings_unbind_session (BurnerDriveSettings *self)
{
	BurnerDriveSettingsPrivate *priv;

	priv = BURNER_DRIVE_SETTINGS_PRIVATE (self);

	if (priv->settings) {
		burner_track_type_free (priv->src_type);
		priv->src_type = NULL;

		g_object_unref (priv->dest_drive);
		priv->dest_drive = NULL;

		priv->dest_media = BURNER_MEDIUM_NONE;

		g_settings_unbind (priv->settings, "speed");
		g_settings_unbind (priv->settings, "flags");

		g_object_unref (priv->settings);
		priv->settings = NULL;
	}
}

static void
burner_drive_settings_rebind_session (BurnerDriveSettings *self)
{
	BurnerDriveSettingsPrivate *priv;
	BurnerTrackType *type;
	BurnerMedia new_media;
	BurnerMedium *medium;
	BurnerDrive *drive;

	priv = BURNER_DRIVE_SETTINGS_PRIVATE (self);

	/* See if we really need to do that:
	 * - check the source type has changed 
	 * - check the output type has changed */
	drive = burner_burn_session_get_burner (priv->session);
	medium = burner_drive_get_medium (drive);
	type = burner_track_type_new ();

	if (!drive
	||  !medium
	||   burner_drive_is_fake (drive)
	||  !BURNER_MEDIUM_VALID (burner_medium_get_status (medium))
	||   burner_burn_session_get_input_type (BURNER_BURN_SESSION (priv->session), type) != BURNER_BURN_OK) {
		burner_drive_settings_unbind_session (self);
		return;
	}

	new_media = BURNER_MEDIUM_TYPE (burner_medium_get_status (medium));

	if (priv->dest_drive == drive
	&&  priv->dest_media == new_media
	&&  priv->src_type && burner_track_type_equal (priv->src_type, type)) {
		burner_track_type_free (type);
		return;
	}

	burner_track_type_free (priv->src_type);
	priv->src_type = type;

	if (priv->dest_drive)
		g_object_unref (priv->dest_drive);

	priv->dest_drive = g_object_ref (drive);

	priv->dest_media = new_media;

	burner_drive_settings_bind_session (self);
}

static void
burner_drive_settings_output_changed_cb (BurnerBurnSession *session,
                                          BurnerMedium *former_medium,
                                          BurnerDriveSettings *self)
{
	burner_drive_settings_rebind_session (self);
}

static void
burner_drive_settings_track_added_cb (BurnerBurnSession *session,
                                       BurnerTrack *track,
                                       BurnerDriveSettings *self)
{
	burner_drive_settings_rebind_session (self);
}

static void
burner_drive_settings_track_removed_cb (BurnerBurnSession *session,
                                         BurnerTrack *track,
                                         guint former_position,
                                         BurnerDriveSettings *self)
{
	burner_drive_settings_rebind_session (self);
}

static void
burner_drive_settings_unset_session (BurnerDriveSettings *self)
{
	BurnerDriveSettingsPrivate *priv;

	priv = BURNER_DRIVE_SETTINGS_PRIVATE (self);

	burner_drive_settings_unbind_session (self);

	if (priv->session) {
		g_signal_handlers_disconnect_by_func (priv->session,
		                                      burner_drive_settings_track_added_cb,
		                                      self);
		g_signal_handlers_disconnect_by_func (priv->session,
		                                      burner_drive_settings_track_removed_cb,
		                                      self);
		g_signal_handlers_disconnect_by_func (priv->session,
		                                      burner_drive_settings_output_changed_cb,
		                                      self);

		g_settings_unbind (priv->config_settings, "tmpdir");
		g_object_unref (priv->config_settings);

		g_object_unref (priv->session);
		priv->session = NULL;
	}
}

void
burner_drive_settings_set_session (BurnerDriveSettings *self,
                                    BurnerBurnSession *session)
{
	BurnerDriveSettingsPrivate *priv;

	priv = BURNER_DRIVE_SETTINGS_PRIVATE (self);

	burner_drive_settings_unset_session (self);

	priv->session = g_object_ref (session);
	g_signal_connect (session,
	                  "track-added",
	                  G_CALLBACK (burner_drive_settings_track_added_cb),
	                  self);
	g_signal_connect (session,
	                  "track-removed",
	                  G_CALLBACK (burner_drive_settings_track_removed_cb),
	                  self);
	g_signal_connect (session,
	                  "output-changed",
	                  G_CALLBACK (burner_drive_settings_output_changed_cb),
	                  self);
	burner_drive_settings_rebind_session (self);

	priv->config_settings = g_settings_new (BURNER_SCHEMA_CONFIG);
	g_settings_bind (priv->config_settings,
	                 BURNER_PROPS_TMP_DIR, session,
	                 "tmpdir", G_SETTINGS_BIND_DEFAULT);
}

static void
burner_drive_settings_init (BurnerDriveSettings *object)
{ }

static void
burner_drive_settings_finalize (GObject *object)
{
	burner_drive_settings_unset_session (BURNER_DRIVE_SETTINGS (object));
	G_OBJECT_CLASS (burner_drive_settings_parent_class)->finalize (object);
}

static void
burner_drive_settings_class_init (BurnerDriveSettingsClass *klass)
{
	GObjectClass* object_class = G_OBJECT_CLASS (klass);

	g_type_class_add_private (klass, sizeof (BurnerDriveSettingsPrivate));

	object_class->finalize = burner_drive_settings_finalize;
}

BurnerDriveSettings *
burner_drive_settings_new (void)
{
	return g_object_new (BURNER_TYPE_DRIVE_SETTINGS, NULL);
}

