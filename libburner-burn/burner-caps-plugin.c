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

#include <string.h>

#include <glib.h>

#include "burner-media.h"
#include "burner-media-private.h"

#include "burn-caps.h"
#include "burn-debug.h"

#include "burner-plugin-private.h"
#include "burner-plugin-information.h"

#define SUBSTRACT(a, b)		((a) &= ~((b)&(a)))

/**
 * the following functions are used to register new caps
 */

static BurnerCapsLink *
burner_caps_find_link_for_input (BurnerCaps *caps,
				  BurnerCaps *input)
{
	GSList *links;

	for (links = caps->links; links; links = links->next) {
		BurnerCapsLink *link;

		link = links->data;
		if (link->caps == input)
			return link;
	}

	return NULL;
}

static gint
burner_burn_caps_sort (gconstpointer a, gconstpointer b)
{
	const BurnerCaps *caps_a = a;
	const BurnerCaps *caps_b = b;
	gint result;

	/* First put DISC (the most used caps) then AUDIO then IMAGE type; these
	 * two types are the ones that most often searched. At the end of the
	 * list we put DATA.
	 * Another (sub)rule is that for DATA, DISC, AUDIO we put a caps that is
	 * encompassed by another before.
	 */

	result = caps_b->type.type - caps_a->type.type;
	if (result)
		return result;

	switch (caps_a->type.type) {
	case BURNER_TRACK_TYPE_DISC:
		if (BURNER_MEDIUM_TYPE (caps_a->type.subtype.media) !=
		    BURNER_MEDIUM_TYPE (caps_b->type.subtype.media))
			return ((gint32) BURNER_MEDIUM_TYPE (caps_a->type.subtype.media) -
			        (gint32) BURNER_MEDIUM_TYPE (caps_b->type.subtype.media));

		if ((caps_a->type.subtype.media & BURNER_MEDIUM_DVD)
		&&  BURNER_MEDIUM_SUBTYPE (caps_a->type.subtype.media) !=
		    BURNER_MEDIUM_SUBTYPE (caps_b->type.subtype.media))			
			return ((gint32) BURNER_MEDIUM_SUBTYPE (caps_a->type.subtype.media) -
			        (gint32) BURNER_MEDIUM_SUBTYPE (caps_b->type.subtype.media));

		if (BURNER_MEDIUM_ATTR (caps_a->type.subtype.media) !=
		    BURNER_MEDIUM_ATTR (caps_b->type.subtype.media))
			return BURNER_MEDIUM_ATTR (caps_a->type.subtype.media) -
			       BURNER_MEDIUM_ATTR (caps_b->type.subtype.media);

		if (BURNER_MEDIUM_STATUS (caps_a->type.subtype.media) !=
		    BURNER_MEDIUM_STATUS (caps_b->type.subtype.media))
			return BURNER_MEDIUM_STATUS (caps_a->type.subtype.media) -
			       BURNER_MEDIUM_STATUS (caps_b->type.subtype.media);

		return (BURNER_MEDIUM_INFO (caps_a->type.subtype.media) -
			BURNER_MEDIUM_INFO (caps_b->type.subtype.media));

	case BURNER_TRACK_TYPE_IMAGE:
		/* This way BIN subtype is always sorted at the end */
		return caps_a->type.subtype.img_format - caps_b->type.subtype.img_format;

	case BURNER_TRACK_TYPE_STREAM:
		if (caps_a->type.subtype.stream_format != caps_b->type.subtype.stream_format) {
			result = (caps_a->type.subtype.stream_format & caps_b->type.subtype.stream_format);
			if (result == caps_a->type.subtype.stream_format)
				return -1;
			else if (result == caps_b->type.subtype.stream_format)
				return 1;

			return  (gint32) caps_a->type.subtype.stream_format -
				(gint32) caps_b->type.subtype.stream_format;
		}
		break;

	case BURNER_TRACK_TYPE_DATA:
		result = (caps_a->type.subtype.fs_type & caps_b->type.subtype.fs_type);
		if (result == caps_a->type.subtype.fs_type)
			return -1;
		else if (result == caps_b->type.subtype.fs_type)
			return 1;

		return (caps_a->type.subtype.fs_type - caps_b->type.subtype.fs_type);

	default:
		break;
	}

	return 0;
}

static BurnerCapsLink *
burner_caps_link_copy (BurnerCapsLink *link)
{
	BurnerCapsLink *retval;

	retval = g_new0 (BurnerCapsLink, 1);
	retval->plugins = g_slist_copy (link->plugins);
	retval->caps = link->caps;

	return retval;
}

static void
burner_caps_link_list_duplicate (BurnerCaps *dest, BurnerCaps *src)
{
	GSList *iter;

	for (iter = src->links; iter; iter = iter->next) {
		BurnerCapsLink *link;

		link = iter->data;
		dest->links = g_slist_prepend (dest->links, burner_caps_link_copy (link));
	}
}

static BurnerCaps *
burner_caps_duplicate (BurnerCaps *caps)
{
	BurnerCaps *retval;

	retval = g_new0 (BurnerCaps, 1);
	retval->flags = caps->flags;
	memcpy (&retval->type, &caps->type, sizeof (BurnerTrackType));
	retval->modifiers = g_slist_copy (caps->modifiers);

	return retval;
}

static void
burner_caps_replicate_modifiers (BurnerCaps *dest, BurnerCaps *src)
{
	GSList *iter;

	for (iter = src->modifiers; iter; iter = iter->next) {
		BurnerPlugin *plugin;

		plugin = iter->data;

		if (g_slist_find (dest->modifiers, plugin))
			continue;

		dest->modifiers = g_slist_prepend (dest->modifiers, plugin);
	}
}

static void
burner_caps_replicate_links (BurnerBurnCaps *self,
			      BurnerCaps *dest,
			      BurnerCaps *src)
{
	GSList *iter;

	burner_caps_link_list_duplicate (dest, src);

	for (iter = self->priv->caps_list; iter; iter = iter->next) {
		BurnerCaps *iter_caps;
		GSList *links;

		iter_caps = iter->data;
		if (iter_caps == src)
			continue;

		for (links = iter_caps->links; links; links = links->next) {
			BurnerCapsLink *link;

			link = links->data;
			if (link->caps == src) {
				BurnerCapsLink *copy;

				copy = burner_caps_link_copy (link);
				copy->caps = dest;
				iter_caps->links = g_slist_prepend (iter_caps->links, copy);
			}
		}
	}
}

static void
burner_caps_replicate_tests (BurnerBurnCaps *self,
			      BurnerCaps *dest,
			      BurnerCaps *src)
{
	GSList *iter;

	for (iter = self->priv->tests; iter; iter = iter->next) {
		BurnerCapsTest *test;
		GSList *links;

		test = iter->data;
		for (links = test->links; links; links = links->next) {
			BurnerCapsLink *link;

			link = links->data;
			if (link->caps == src) {
				BurnerCapsLink *copy;

				copy = burner_caps_link_copy (link);
				copy->caps = dest;
				test->links = g_slist_prepend (test->links, copy);
			}
		}
	}
}

static void
burner_caps_copy_deep (BurnerBurnCaps *self,
			BurnerCaps *dest,
			BurnerCaps *src)
{
	burner_caps_replicate_links (self, dest, src);
	burner_caps_replicate_tests (self, dest, src);
	burner_caps_replicate_modifiers (dest,src);
}

static BurnerCaps *
burner_caps_duplicate_deep (BurnerBurnCaps *self,
			     BurnerCaps *caps)
{
	BurnerCaps *retval;

	retval = burner_caps_duplicate (caps);
	burner_caps_copy_deep (self, retval, caps);
	return retval;
}

static GSList *
burner_caps_list_check_io (BurnerBurnCaps *self,
			    GSList *list,
			    BurnerPluginIOFlag flags)
{
	GSList *iter;

	/* in this function we create the caps with the missing IO. All in the
	 * list have something in common with flags. */
	for (iter = list; iter; iter = iter->next) {
		BurnerCaps *caps;
		BurnerPluginIOFlag common;

		caps = iter->data;
		common = caps->flags & flags;
		if (common != caps->flags) {
			BurnerCaps *new_caps;

			/* (common == flags) && common != caps->flags
			 * caps->flags encompasses flags: Split the caps in two
			 * and only keep the interesting part */
			caps->flags &= ~common;

			/* this caps has changed and needs to be sorted again */
			self->priv->caps_list = g_slist_sort (self->priv->caps_list,
							      burner_burn_caps_sort);

			new_caps = burner_caps_duplicate_deep (self, caps);
			new_caps->flags = common;

			self->priv->caps_list = g_slist_insert_sorted (self->priv->caps_list,
								       new_caps,
								       burner_burn_caps_sort);

			list = g_slist_prepend (list, new_caps);
		}
		else if (common != flags) {
			GSList *node, *next;
			BurnerPluginIOFlag complement = flags;

			complement &= ~common;
			for (node = list; node; node = next) {
				BurnerCaps *tmp;

				tmp = node->data;
				next = node->next;

				if (node == iter)
					continue;

				if (caps->type.type != tmp->type.type
				||  caps->type.subtype.media != tmp->type.subtype.media)
					continue;

				/* substract the flags and relocate them at the
				 * head of the list since we don't need to look
				 * them up again */
				complement &= ~(tmp->flags);
				list = g_slist_remove (list, tmp);
				list = g_slist_prepend (list, tmp);
			}

			if (complement != BURNER_PLUGIN_IO_NONE) {
				BurnerCaps *new_caps;

				/* common == caps->flags && common != flags.
				 * Flags encompasses caps->flags. So we need to
				 * create a new caps for this type with the
				 * substraction of flags if the other part isn't
				 * in the list */
				new_caps = burner_caps_duplicate (caps);
				new_caps->flags = flags & (~common);
				self->priv->caps_list = g_slist_insert_sorted (self->priv->caps_list,
									       new_caps,
									       burner_burn_caps_sort);

				list = g_slist_prepend (list, new_caps);
			}
		}
	}

	return list;
}

GSList *
burner_caps_image_new (BurnerPluginIOFlag flags,
			BurnerImageFormat format)
{
	BurnerImageFormat remaining_format;
	BurnerBurnCaps *self;
	GSList *retval = NULL;
	GSList *iter;

	BURNER_BURN_LOG_WITH_FULL_TYPE (BURNER_TRACK_TYPE_IMAGE,
					 format,
					 flags,
					 "New caps required");

	self = burner_burn_caps_get_default ();

	remaining_format = format;

	/* We have to search all caps with or related to the format */
	for (iter = self->priv->caps_list; iter; iter = iter->next) {
		BurnerCaps *caps;
		BurnerImageFormat common;
		BurnerPluginIOFlag common_io;

		caps = iter->data;
		if (caps->type.type != BURNER_TRACK_TYPE_IMAGE)
			continue;

		common_io = caps->flags & flags;
		if (common_io == BURNER_PLUGIN_IO_NONE)
			continue;

		common = (caps->type.subtype.img_format & format);
		if (common == BURNER_IMAGE_FORMAT_NONE)
			continue;

		if (common != caps->type.subtype.img_format) {
			/* img_format encompasses format. Split it in two and
			 * keep caps with common format */
			SUBSTRACT (caps->type.subtype.img_format, common);
			self->priv->caps_list = g_slist_sort (self->priv->caps_list,
							      burner_burn_caps_sort);

			caps = burner_caps_duplicate_deep (self, caps);
			caps->type.subtype.img_format = common;

			self->priv->caps_list = g_slist_insert_sorted (self->priv->caps_list,
								       caps,
								       burner_burn_caps_sort);
		}

		retval = g_slist_prepend (retval, caps);
		remaining_format &= ~common;
	}

	/* Now we make sure that all these new or already 
	 * existing caps have the proper IO Flags */
	retval = burner_caps_list_check_io (self, retval, flags);

	if (remaining_format != BURNER_IMAGE_FORMAT_NONE) {
		BurnerCaps *caps;

		caps = g_new0 (BurnerCaps, 1);
		caps->flags = flags;
		caps->type.subtype.img_format = remaining_format;
		caps->type.type = BURNER_TRACK_TYPE_IMAGE;

		self->priv->caps_list = g_slist_insert_sorted (self->priv->caps_list,
							       caps,
							       burner_burn_caps_sort);
		retval = g_slist_prepend (retval, caps);

		BURNER_BURN_LOG_TYPE (&caps->type, "Created new caps");
	}

	g_object_unref (self);
	return retval;
}

GSList *
burner_caps_audio_new (BurnerPluginIOFlag flags,
			BurnerStreamFormat format)
{
	GSList *iter;
	GSList *retval = NULL;
	BurnerBurnCaps *self;
	GSList *encompassing = NULL;
	gboolean have_the_one = FALSE;

	BURNER_BURN_LOG_WITH_FULL_TYPE (BURNER_TRACK_TYPE_STREAM,
					 format,
					 flags,
					 "New caps required");

	self = burner_burn_caps_get_default ();

	for (iter = self->priv->caps_list; iter; iter = iter->next) {
		BurnerCaps *caps;
		BurnerStreamFormat common;
		BurnerPluginIOFlag common_io;
		BurnerStreamFormat common_audio;
		BurnerStreamFormat common_video;

		caps = iter->data;

		if (caps->type.type != BURNER_TRACK_TYPE_STREAM)
			continue;

		common_io = (flags & caps->flags);
		if (common_io == BURNER_PLUGIN_IO_NONE)
			continue;

		if (caps->type.subtype.stream_format == format) {
			/* that's the perfect fit */
			have_the_one = TRUE;
			retval = g_slist_prepend (retval, caps);
			continue;
		}

		/* Search caps strictly encompassed or encompassing our format
		 * NOTE: make sure that if there is a VIDEO stream in one of
		 * them, the other does have a VIDEO stream too. */
		common_audio = BURNER_STREAM_FORMAT_AUDIO (caps->type.subtype.stream_format) & 
			       BURNER_STREAM_FORMAT_AUDIO (format);
		if (common_audio == BURNER_AUDIO_FORMAT_NONE
		&& (BURNER_STREAM_FORMAT_AUDIO (caps->type.subtype.stream_format)
		||  BURNER_STREAM_FORMAT_AUDIO (format)))
			continue;

		common_video = BURNER_STREAM_FORMAT_VIDEO (caps->type.subtype.stream_format) & 
			       BURNER_STREAM_FORMAT_VIDEO (format);

		if (common_video == BURNER_AUDIO_FORMAT_NONE
		&& (BURNER_STREAM_FORMAT_VIDEO (caps->type.subtype.stream_format)
		||  BURNER_STREAM_FORMAT_VIDEO (format)))
			continue;

		/* Likewise... that must be common */
		if ((caps->type.subtype.stream_format & BURNER_METADATA_INFO) != (format & BURNER_METADATA_INFO))
			continue;

		common = common_audio|common_video|(format & BURNER_METADATA_INFO);

		/* encompassed caps just add it to retval */
		if (caps->type.subtype.stream_format == common)
			retval = g_slist_prepend (retval, caps);

		/* encompassing caps keep it if we need to create perfect fit */
		if (format == common)
			encompassing = g_slist_prepend (encompassing, caps);
	}

	/* Now we make sure that all these new or already 
	 * existing caps have the proper IO Flags */
	retval = burner_caps_list_check_io (self, retval, flags);

	if (!have_the_one) {
		BurnerCaps *caps;

		caps = g_new0 (BurnerCaps, 1);
		caps->flags = flags;
		caps->type.subtype.stream_format = format;
		caps->type.type = BURNER_TRACK_TYPE_STREAM;

		if (encompassing) {
			for (iter = encompassing; iter; iter = iter->next) {
				BurnerCaps *iter_caps;

				iter_caps = iter->data;
				burner_caps_copy_deep (self, caps, iter_caps);
			}
		}

		self->priv->caps_list = g_slist_insert_sorted (self->priv->caps_list,
							       caps,
							       burner_burn_caps_sort);
		retval = g_slist_prepend (retval, caps);

		BURNER_BURN_LOG_TYPE (&caps->type, "Created new caps");
	}

	g_slist_free (encompassing);

	g_object_unref (self);

	return retval;
}

GSList *
burner_caps_data_new (BurnerImageFS fs_type)
{
	GSList *iter;
	GSList *retval = NULL;
	BurnerBurnCaps *self;
	GSList *encompassing = NULL;
	gboolean have_the_one = FALSE;

	BURNER_BURN_LOG_WITH_FULL_TYPE (BURNER_TRACK_TYPE_DATA,
					 fs_type,
					 BURNER_PLUGIN_IO_NONE,
					 "New caps required");

	self = burner_burn_caps_get_default ();

	for (iter = self->priv->caps_list; iter; iter = iter->next) {
		BurnerCaps *caps;
		BurnerImageFS common;

		caps = iter->data;

		if (caps->type.type != BURNER_TRACK_TYPE_DATA)
			continue;

		if (caps->type.subtype.fs_type == fs_type) {
			/* that's the perfect fit */
			have_the_one = TRUE;
			retval = g_slist_prepend (retval, caps);
			continue;
		}

		/* search caps strictly encompassing our format ... */
		common = caps->type.subtype.fs_type & fs_type;
		if (common == BURNER_IMAGE_FS_NONE)
			continue;

		/* encompassed caps just add it to retval */
		if (caps->type.subtype.fs_type == common)
			retval = g_slist_prepend (retval, caps);

		/* encompassing caps keep it if we need to create perfect fit */
		if (fs_type == common)
			encompassing = g_slist_prepend (encompassing, caps);
	}

	if (!have_the_one) {
		BurnerCaps *caps;

		caps = g_new0 (BurnerCaps, 1);
		caps->flags = BURNER_PLUGIN_IO_ACCEPT_FILE;
		caps->type.type = BURNER_TRACK_TYPE_DATA;
		caps->type.subtype.fs_type = fs_type;

		if (encompassing) {
			for (iter = encompassing; iter; iter = iter->next) {
				BurnerCaps *iter_caps;

				iter_caps = iter->data;
				burner_caps_copy_deep (self, caps, iter_caps);
			}
		}

		self->priv->caps_list = g_slist_insert_sorted (self->priv->caps_list,
							       caps,
							       burner_burn_caps_sort);
		retval = g_slist_prepend (retval, caps);
	}

	g_slist_free (encompassing);

	g_object_unref (self);

	return retval;
}

static GSList *
burner_caps_disc_lookup_or_create (BurnerBurnCaps *self,
				    GSList *retval,
				    BurnerMedia media)
{
	GSList *iter;
	BurnerCaps *caps;

	for (iter = self->priv->caps_list; iter; iter = iter->next) {
		caps = iter->data;

		if (caps->type.type != BURNER_TRACK_TYPE_DISC)
			continue;

		if (caps->type.subtype.media == media) {
			BURNER_BURN_LOG_WITH_TYPE (&caps->type,
						    caps->flags,
						    "Retrieved");
			return g_slist_prepend (retval, caps);
		}
	}

	caps = g_new0 (BurnerCaps, 1);
	caps->flags = BURNER_PLUGIN_IO_ACCEPT_FILE;
	caps->type.type = BURNER_TRACK_TYPE_DISC;
	caps->type.subtype.media = media;

	BURNER_BURN_LOG_WITH_TYPE (&caps->type,
				    caps->flags,
				    "Created");

	self->priv->caps_list = g_slist_prepend (self->priv->caps_list, caps);

	return g_slist_prepend (retval, caps);
}

GSList *
burner_caps_disc_new (BurnerMedia type)
{
	BurnerBurnCaps *self;
	GSList *retval = NULL;
	GSList *list;
	GSList *iter;

	self = burner_burn_caps_get_default ();

	list = burner_media_get_all_list (type);
	for (iter = list; iter; iter = iter->next) {
		BurnerMedia medium;

		medium = GPOINTER_TO_INT (iter->data);
		retval = burner_caps_disc_lookup_or_create (self, retval, medium);
	}
	g_slist_free (list);

	g_object_unref (self);
	return retval;
}

/**
 * these functions are to create links
 */

static void
burner_caps_create_links (BurnerCaps *output,
	 		   GSList *inputs,
	 		   BurnerPlugin *plugin)
{
	for (; inputs; inputs = inputs->next) {
		BurnerCaps *input;
		BurnerCapsLink *link;

		input = inputs->data;

		if (output == input) {
			BURNER_BURN_LOG ("Same input and output for link. Dropping");
			continue;
		}

		if (input->flags == output->flags
		&&  input->type.type == output->type.type  
		&&  input->type.subtype.media == output->type.subtype.media)
			BURNER_BURN_LOG ("Recursive link");

		link = burner_caps_find_link_for_input (output, input);

#if 0

		/* Mainly for extra debugging */
		BURNER_BURN_LOG_TYPE (&output->type, "Linking");
		BURNER_BURN_LOG_TYPE (&input->type, "to");
		BURNER_BURN_LOG ("with %s", burner_plugin_get_name (plugin));

#endif

		if (!link) {
			link = g_new0 (BurnerCapsLink, 1);
			link->caps = input;
			link->plugins = g_slist_prepend (NULL, plugin);

			output->links = g_slist_prepend (output->links, link);
		}
		else
			link->plugins = g_slist_prepend (link->plugins, plugin);
	}
}

void
burner_plugin_link_caps (BurnerPlugin *plugin,
			  GSList *outputs,
			  GSList *inputs)
{
	/* we make sure the caps exists and if not we create them */
	for (; outputs; outputs = outputs->next) {
		BurnerCaps *output;

		output = outputs->data;
		burner_caps_create_links (output, inputs, plugin);
	}
}

void
burner_plugin_blank_caps (BurnerPlugin *plugin,
			   GSList *caps_list)
{
	for (; caps_list; caps_list = caps_list->next) {
		BurnerCaps *caps;
		BurnerCapsLink *link;

		caps = caps_list->data;

		if (caps->type.type != BURNER_TRACK_TYPE_DISC)
			continue;
	
		BURNER_BURN_LOG_WITH_TYPE (&caps->type,
					    caps->flags,
					    "Adding blank caps for");

		/* we need to find the link whose caps is NULL */
		link = burner_caps_find_link_for_input (caps, NULL);
		if (!link) {
			link = g_new0 (BurnerCapsLink, 1);
			link->caps = NULL;
			link->plugins = g_slist_prepend (NULL, plugin);

			caps->links = g_slist_prepend (caps->links, link);
		}
		else
			link->plugins = g_slist_prepend (link->plugins, plugin);
	}
}

void
burner_plugin_process_caps (BurnerPlugin *plugin,
			     GSList *caps_list)
{
	for (; caps_list; caps_list = caps_list->next) {
		BurnerCaps *caps;

		caps = caps_list->data;
		caps->modifiers = g_slist_prepend (caps->modifiers, plugin);
	}
}

void
burner_plugin_check_caps (BurnerPlugin *plugin,
			   BurnerChecksumType type,
			   GSList *caps_list)
{
	BurnerCapsTest *test = NULL;
	BurnerBurnCaps *self;
	GSList *iter;

	/* Find the the BurnerCapsTest for this type; if none create it */
	self = burner_burn_caps_get_default ();

	for (iter = self->priv->tests; iter; iter = iter->next) {
		BurnerCapsTest *tmp;

		tmp = iter->data;
		if (tmp->type == type) {
			test = tmp;
			break;
		}
	}

	if (!test) {
		test = g_new0 (BurnerCapsTest, 1);
		test->type = type;
		self->priv->tests = g_slist_prepend (self->priv->tests, test);
	}

	g_object_unref (self);

	for (; caps_list; caps_list = caps_list->next) {
		GSList *links;
		BurnerCaps *caps;
		BurnerCapsLink *link;

		caps = caps_list->data;

		/* try to find a link for the above caps, if none create one */
		link = NULL;
		for (links = test->links; links; links = links->next) {
			BurnerCapsLink *tmp;

			tmp = links->data;
			if (tmp->caps == caps) {
				link = tmp;
				break;
			}
		}

		if (!link) {
			link = g_new0 (BurnerCapsLink, 1);
			link->caps = caps;
			test->links = g_slist_prepend (test->links, link);
		}

		link->plugins = g_slist_prepend (link->plugins, plugin);
	}
}

void
burner_plugin_register_group (BurnerPlugin *plugin,
			       const gchar *name)
{
	guint retval;
	BurnerBurnCaps *self;

	if (!name) {
		burner_plugin_set_group (plugin, 0);
		return;
	}

	self = burner_burn_caps_get_default ();

	if (!self->priv->groups)
		self->priv->groups = g_hash_table_new_full (g_str_hash,
							    g_str_equal,
							    g_free,
							    NULL);

	retval = GPOINTER_TO_INT (g_hash_table_lookup (self->priv->groups, name));
	if (retval) {
		burner_plugin_set_group (plugin, retval);
		g_object_unref (self);
		return;
	}

	g_hash_table_insert (self->priv->groups,
			     g_strdup (name),
			     GINT_TO_POINTER (g_hash_table_size (self->priv->groups) + 1));

	/* see if we have a group id now */
	if (!self->priv->group_id
	&&   self->priv->group_str
	&&  !strcmp (name, self->priv->group_str))
		self->priv->group_id = g_hash_table_size (self->priv->groups) + 1;

	burner_plugin_set_group (plugin, g_hash_table_size (self->priv->groups) + 1);

	g_object_unref (self);
}

/** 
 * This is to find out what are the capacities of a plugin
 * Declared in burner-plugin-private.h
 */

BurnerBurnResult
burner_plugin_can_burn (BurnerPlugin *plugin)
{
	GSList *iter;
	BurnerBurnCaps *self;

	self = burner_burn_caps_get_default ();

	for (iter = self->priv->caps_list; iter; iter = iter->next) {
		BurnerCaps *caps;
		GSList *links;

		caps = iter->data;
		if (caps->type.type != BURNER_TRACK_TYPE_DISC)
			continue;

		for (links = caps->links; links; links = links->next) {
			BurnerCapsLink *link;
			GSList *plugins;

			link = links->data;

			/* see if the plugin is in the link by going through the list */
			for (plugins = link->plugins; plugins; plugins = plugins->next) {
				BurnerPlugin *tmp;

				tmp = plugins->data;
				if (tmp == plugin) {
					g_object_unref (self);
					return BURNER_BURN_OK;
				}
			}
		}
	}

	g_object_unref (self);
	return BURNER_BURN_NOT_SUPPORTED;
}

BurnerBurnResult
burner_plugin_can_image (BurnerPlugin *plugin)
{
	GSList *iter;
	BurnerBurnCaps *self;

	self = burner_burn_caps_get_default ();
	for (iter = self->priv->caps_list; iter; iter = iter->next) {
		BurnerTrackDataType destination;
		BurnerCaps *caps;
		GSList *links;

		caps = iter->data;
		if (caps->type.type != BURNER_TRACK_TYPE_IMAGE
		&&  caps->type.type != BURNER_TRACK_TYPE_STREAM
		&&  caps->type.type != BURNER_TRACK_TYPE_DATA)
			continue;

		destination = caps->type.type;
		for (links = caps->links; links; links = links->next) {
			BurnerCapsLink *link;
			GSList *plugins;

			link = links->data;
			if (!link->caps
			||   link->caps->type.type == destination)
				continue;

			/* see if the plugin is in the link by going through the list */
			for (plugins = link->plugins; plugins; plugins = plugins->next) {
				BurnerPlugin *tmp;

				tmp = plugins->data;
				if (tmp == plugin) {
					g_object_unref (self);
					return BURNER_BURN_OK;
				}
			}
		}
	}

	g_object_unref (self);
	return BURNER_BURN_NOT_SUPPORTED;
}

BurnerBurnResult
burner_plugin_can_convert (BurnerPlugin *plugin)
{
	GSList *iter;
	BurnerBurnCaps *self;

	self = burner_burn_caps_get_default ();

	for (iter = self->priv->caps_list; iter; iter = iter->next) {
		BurnerTrackDataType destination;
		BurnerCaps *caps;
		GSList *links;

		caps = iter->data;
		if (caps->type.type != BURNER_TRACK_TYPE_IMAGE
		&&  caps->type.type != BURNER_TRACK_TYPE_STREAM)
			continue;

		destination = caps->type.type;
		for (links = caps->links; links; links = links->next) {
			BurnerCapsLink *link;
			GSList *plugins;

			link = links->data;
			if (!link->caps
			||   link->caps->type.type != destination)
				continue;

			/* see if the plugin is in the link by going through the list */
			for (plugins = link->plugins; plugins; plugins = plugins->next) {
				BurnerPlugin *tmp;

				tmp = plugins->data;
				if (tmp == plugin) {
					g_object_unref (self);
					return BURNER_BURN_OK;
				}
			}
		}
	}

	g_object_unref (self);

	return BURNER_BURN_NOT_SUPPORTED;
}

