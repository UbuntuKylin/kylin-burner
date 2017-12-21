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
  
#include <glib.h>
#include <glib-object.h>
#include <glib/gi18n-lib.h>
  
#include <gtk/gtk.h>
  
#include "burner-drive-selection.h"
#include "burner-medium-monitor.h"
#include "burner-drive.h"
#include "burner-units.h"
  
typedef struct _BurnerDriveSelectionPrivate BurnerDriveSelectionPrivate;
struct _BurnerDriveSelectionPrivate
{
	BurnerDrive *active;

	BurnerDriveType type;
	gulong added_sig;
	gulong removed_sig;
};
  
#define BURNER_DRIVE_SELECTION_PRIVATE(o)  (G_TYPE_INSTANCE_GET_PRIVATE ((o), BURNER_TYPE_DRIVE_SELECTION, BurnerDriveSelectionPrivate))

typedef enum {
	CHANGED_SIGNAL,
	LAST_SIGNAL
} BurnerDriveSelectionSignalType;

/* GtkBuildable */
static GtkBuildableIface *parent_buildable_iface;

static guint burner_drive_selection_signals [LAST_SIGNAL] = { 0 };

enum {
	PROP_0,
	PROP_DRIVE,
	PROP_DRIVE_TYPE
};
  
enum {
	DRIVE_COL,
	NAME_COL,
	ICON_COL,
	NUM_COL
};

  
static void
burner_drive_selection_buildable_init (GtkBuildableIface *iface)
{
	parent_buildable_iface = g_type_interface_peek_parent (iface);
}  

G_DEFINE_TYPE_WITH_CODE (BurnerDriveSelection, burner_drive_selection, GTK_TYPE_COMBO_BOX, G_IMPLEMENT_INTERFACE (GTK_TYPE_BUILDABLE, burner_drive_selection_buildable_init));

static void
burner_drive_selection_set_current_drive (BurnerDriveSelection *self,
					   GtkTreeIter *iter)
{
	BurnerDriveSelectionPrivate *priv;
	BurnerDrive *drive;
	GtkTreeModel *model;

	priv = BURNER_DRIVE_SELECTION_PRIVATE (self);

	model = gtk_combo_box_get_model (GTK_COMBO_BOX (self));
	gtk_tree_model_get (model, iter,
			    DRIVE_COL, &drive,
			    -1);

	if (drive)
		gtk_widget_set_sensitive (GTK_WIDGET (self), TRUE);
	else
		gtk_widget_set_sensitive (GTK_WIDGET (self), FALSE);

	if (priv->active == drive)
		return;
  
	if (priv->active)
		g_object_unref (priv->active);
  
	priv->active = drive;
  
	if (priv->active)
		g_object_ref (priv->active);
  
	g_signal_emit (self,
		       burner_drive_selection_signals [CHANGED_SIGNAL],
		       0,
		       priv->active);
}
  
static void
burner_drive_selection_changed (GtkComboBox *combo)
{
	GtkTreeIter iter;
  
	if (!gtk_combo_box_get_active_iter (combo, &iter))
		return;
  
	burner_drive_selection_set_current_drive (BURNER_DRIVE_SELECTION (combo), &iter);
}
  
/**
 * burner_drive_selection_set_active:
 * @selector: a #BurnerDriveSelection
 * @drive: a #BurnerDrive to set as the active one in the selector
 *
 * Sets the active drive. Emits the ::drive-changed signal.
 *
 * Return value: a #gboolean. TRUE if it succeeded, FALSE otherwise.
 **/
gboolean
burner_drive_selection_set_active (BurnerDriveSelection *selector,
				     BurnerDrive *drive)
{
	BurnerDriveSelectionPrivate *priv;
	gboolean result = FALSE;
	GtkTreeModel *model;
	GtkTreeIter iter;

	g_return_val_if_fail (selector != NULL, FALSE);
	g_return_val_if_fail (BURNER_IS_DRIVE_SELECTION (selector), FALSE);

	priv = BURNER_DRIVE_SELECTION_PRIVATE (selector);
  
	if (priv->active == drive)
		return TRUE;
  
	model = gtk_combo_box_get_model (GTK_COMBO_BOX (selector));
	if (!gtk_tree_model_get_iter_first (model, &iter))
		return FALSE;
  
	do {
		BurnerDrive *iter_drive;
  
		gtk_tree_model_get (model, &iter,
				    DRIVE_COL, &iter_drive,
				    -1);
  
		if (drive == iter_drive) {
			if (iter_drive)
				g_object_unref (iter_drive);

			gtk_combo_box_set_active_iter (GTK_COMBO_BOX (selector), &iter);
			burner_drive_selection_set_current_drive (selector, &iter);
			result = TRUE;
			break;
		}

		g_object_unref (iter_drive);
	} while (gtk_tree_model_iter_next (model, &iter));

	return result;
}
  
/**
 * burner_drive_selection_get_active:
 * @selector: a #BurnerDriveSelection
 *
 * Gets the active drive.
 *
 * Return value: a #BurnerDrive or NULL. Unref when it is not needed anymore.
 **/
BurnerDrive *
burner_drive_selection_get_active (BurnerDriveSelection *selector)
{
	BurnerDriveSelectionPrivate *priv;

	g_return_val_if_fail (selector != NULL, NULL);
	g_return_val_if_fail (BURNER_IS_DRIVE_SELECTION (selector), NULL);

	priv = BURNER_DRIVE_SELECTION_PRIVATE (selector);
	if (!priv->active)
		return NULL;

	return g_object_ref (priv->active);
}

static void
burner_drive_selection_update_no_disc_entry (BurnerDriveSelection *self,
					      GtkTreeModel *model,
					      GtkTreeIter *iter)
{
	GIcon *icon;

	icon = g_themed_icon_new_with_default_fallbacks ("drive-optical");

	/* FIXME: that needs a string */
	gtk_list_store_set (GTK_LIST_STORE (model), iter,
			    NAME_COL, NULL,
			    ICON_COL, NULL,
			    -1);

	g_object_unref (icon);

	gtk_combo_box_set_active_iter (GTK_COMBO_BOX (self), iter);
	burner_drive_selection_set_current_drive (self, iter);
}
  
static void
burner_drive_selection_add_no_disc_entry (BurnerDriveSelection *self)
{
	GtkTreeIter iter;
	GtkTreeModel *model;

	/* Nothing's available. Say it. Two cases here, either we're
	 * still probing drives or there isn't actually any available
	 * drive. */
	model = gtk_combo_box_get_model (GTK_COMBO_BOX (self));
	gtk_list_store_append (GTK_LIST_STORE (model), &iter);
	burner_drive_selection_update_no_disc_entry (self, model, &iter);
}
  
/**
 * burner_drive_selection_show_type:
 * @selector: a #BurnerDriveSelection
 * @type: a #BurnerDriveType
 *
 * Filters and displays drive corresponding to @type.
 *
 **/
void
burner_drive_selection_show_type (BurnerDriveSelection *selector,
				   BurnerDriveType type)
{
	BurnerDriveSelectionPrivate *priv;
	BurnerMediumMonitor *monitor;
	GtkTreeModel *model;
	GtkTreeIter iter;
	GSList *list;
	GSList *item;

	g_return_if_fail (selector != NULL);
	g_return_if_fail (BURNER_IS_DRIVE_SELECTION (selector));

	priv = BURNER_DRIVE_SELECTION_PRIVATE (selector);

	priv->type = type;

	monitor = burner_medium_monitor_get_default ();
	list = burner_medium_monitor_get_drives (monitor, type);
	g_object_unref (monitor);
  
	model = gtk_combo_box_get_model (GTK_COMBO_BOX (selector));
	if (gtk_tree_model_get_iter_first (model, &iter)) {
		/* First filter */
		do {
			GSList *node;
			BurnerDrive *drive;
  
			gtk_tree_model_get (model, &iter,
					    DRIVE_COL, &drive,
					    -1);
  
			if (!drive) {
				/* That's the dummy line saying there isn't any
				 * available drive for whatever action it is */
				gtk_list_store_remove (GTK_LIST_STORE (model), &iter);
				break;
			}
  
			node = g_slist_find (list, drive);
			g_object_unref (drive);
  
			if (!node) {
				if (gtk_list_store_remove (GTK_LIST_STORE (model), &iter))
					continue;
  
				/* no more iter in the tree get out */
				break;
			}
  
			g_object_unref (node->data);
			list = g_slist_delete_link (list, node);
		} while (gtk_tree_model_iter_next (model, &iter));
	}
  
	if (list) {
		/* add remaining drive */
		for (item = list; item; item = item->next) {
			gchar *drive_name;
			BurnerDrive *drive;
  			GIcon *drive_icon = NULL;

			drive = item->data;

			drive_name = burner_drive_get_display_name (drive);

			if (!burner_drive_is_fake (drive)) {
				GDrive *gdrive;

				gdrive = burner_drive_get_gdrive (drive);
				if (gdrive) {
					drive_icon = g_drive_get_icon (gdrive);
					g_object_unref (gdrive);
				}
				else
					drive_icon = g_themed_icon_new_with_default_fallbacks ("drive-optical");
			}
			else
				drive_icon = g_themed_icon_new_with_default_fallbacks ("iso-image-new");
  
			gtk_list_store_append (GTK_LIST_STORE (model), &iter);
			gtk_list_store_set (GTK_LIST_STORE (model), &iter,
					    DRIVE_COL, drive,
					    NAME_COL, drive_name?drive_name:_("Unnamed CD/DVD Drive"),
					    ICON_COL, drive_icon,
					    -1);
			g_free (drive_name);
			g_object_unref (drive_icon);
		}
		g_slist_foreach (list, (GFunc) g_object_unref, NULL);
		g_slist_free (list);
	}
  
	if (!gtk_tree_model_get_iter_first (model, &iter)) {
		burner_drive_selection_add_no_disc_entry (selector);
		return;
	}

	gtk_widget_set_sensitive (GTK_WIDGET (selector), TRUE);
	if (gtk_combo_box_get_active (GTK_COMBO_BOX (selector)) == -1) {
		gtk_combo_box_set_active_iter (GTK_COMBO_BOX (selector), &iter);
		burner_drive_selection_set_current_drive (selector, &iter);
	}
}

static void
burner_drive_selection_drive_added_cb (BurnerMediumMonitor *monitor,
					BurnerDrive *drive,
					BurnerDriveSelection *self)
{
	BurnerDriveSelectionPrivate *priv;
	gchar *drive_name = NULL;
	gboolean add = FALSE;
	GtkTreeModel *model;
	GIcon *drive_icon;
	GtkTreeIter iter;

	priv = BURNER_DRIVE_SELECTION_PRIVATE (self);

	if ((priv->type & BURNER_DRIVE_TYPE_WRITER)
	&&  (burner_drive_can_write (drive)))
		add = TRUE;
	else if (priv->type & BURNER_DRIVE_TYPE_READER)
		add = TRUE;

	model = gtk_combo_box_get_model (GTK_COMBO_BOX (self));

	if (!add) {
		/* Try to get the first iter (it shouldn't fail) */
		if (!gtk_tree_model_get_iter_first (model, &iter)) {
			burner_drive_selection_add_no_disc_entry (self);
			return;
		}
  
		/* See if that's a real drive or not; if so, return. */
		drive = NULL;
		gtk_tree_model_get (model, &iter,
				    DRIVE_COL, &drive,
				    -1);
		if (drive)
			return;
  
		burner_drive_selection_update_no_disc_entry (self, model, &iter);
		return;
	}
  
	/* remove warning message */
	if (gtk_tree_model_get_iter_first (model, &iter)) {
		BurnerDrive *tmp;
  
		gtk_tree_model_get (model, &iter,
				    DRIVE_COL, &tmp,
				    -1);
		if (!tmp)
			gtk_list_store_remove (GTK_LIST_STORE (model), &iter);
		else
			g_object_unref (tmp);
	}

	if (!burner_drive_is_fake (drive)) {
		GDrive *gdrive;

		gdrive = burner_drive_get_gdrive (drive);
		if (gdrive) {
			drive_icon = g_drive_get_icon (gdrive);
			g_object_unref (gdrive);
		}
		else
			drive_icon = g_themed_icon_new_with_default_fallbacks ("drive-optical");
	}
	else
		drive_icon = g_themed_icon_new_with_default_fallbacks ("iso-image-new");

	drive_name = burner_drive_get_display_name (drive);

	gtk_list_store_append (GTK_LIST_STORE (model), &iter);
	gtk_list_store_set (GTK_LIST_STORE (model), &iter,
			    DRIVE_COL, drive,
			    NAME_COL, drive_name?drive_name:_("Unnamed CD/DVD Drive"),
			    ICON_COL, drive_icon,
			    -1);
	g_free (drive_name);
	g_object_unref (drive_icon);

	gtk_widget_set_sensitive (GTK_WIDGET (self), TRUE);
	if (gtk_combo_box_get_active (GTK_COMBO_BOX (self)) == -1) {
		gtk_combo_box_set_active_iter (GTK_COMBO_BOX (self), &iter);
		burner_drive_selection_set_current_drive (self, &iter);
	}
}
  
static void
burner_drive_selection_drive_removed_cb (BurnerMediumMonitor *monitor,
					    BurnerDrive *drive,
					    BurnerDriveSelection *self)
{
	GtkTreeModel *model;
	GtkTreeIter iter;
  
	model = gtk_combo_box_get_model (GTK_COMBO_BOX (self));
	if (!gtk_tree_model_get_iter_first (model, &iter))
		return;
  
	do {
		BurnerDrive *iter_drive;
  
		gtk_tree_model_get (model, &iter,
				    DRIVE_COL, &iter_drive,
				    -1);
  
		if (drive == iter_drive) {
			g_object_unref (iter_drive);
			gtk_list_store_remove (GTK_LIST_STORE (model), &iter);
			break;
		}
  
		/* Could be NULL if a message "there is no drive ..." is on */
		if (iter_drive)
			g_object_unref (iter_drive);
  
	} while (gtk_tree_model_iter_next (model, &iter));

	if (!gtk_tree_model_get_iter_first (model, &iter)) {
		burner_drive_selection_add_no_disc_entry (self);
		return;
	}

	if (gtk_combo_box_get_active (GTK_COMBO_BOX (self)) == -1) {
		gtk_combo_box_set_active_iter (GTK_COMBO_BOX (self), &iter);
		burner_drive_selection_set_current_drive (self, &iter);
	}
}
  
static void
burner_drive_selection_init (BurnerDriveSelection *object)
{
	GtkListStore *model;
	GtkCellRenderer *renderer;
	BurnerMediumMonitor *monitor;
	BurnerDriveSelectionPrivate *priv;

	priv = BURNER_DRIVE_SELECTION_PRIVATE (object);

	monitor = burner_medium_monitor_get_default ();
	priv->added_sig = g_signal_connect (monitor,
					    "drive-added",
					    G_CALLBACK (burner_drive_selection_drive_added_cb),
					    object);
	priv->removed_sig = g_signal_connect (monitor,
					      "drive-removed",
					      G_CALLBACK (burner_drive_selection_drive_removed_cb),
					      object);

	g_object_unref (monitor);

	/* get the list and fill the model */
	model = gtk_list_store_new (NUM_COL,
				    G_TYPE_OBJECT,
				    G_TYPE_STRING,
				    G_TYPE_ICON);

	gtk_combo_box_set_model (GTK_COMBO_BOX (object), GTK_TREE_MODEL (model));
	g_object_unref (model);

	renderer = gtk_cell_renderer_pixbuf_new ();
	g_object_set (renderer, "follow-state", TRUE, NULL);
	gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (object), renderer, FALSE);
	gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (object), renderer,
					"gicon", ICON_COL,
					NULL);

	renderer = gtk_cell_renderer_text_new ();
	g_object_set (renderer, "xpad", 8, NULL);
	gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (object), renderer, TRUE);
	gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (object), renderer,
					"markup", NAME_COL,
					NULL);
}

static void
burner_drive_selection_constructed (GObject *object)
{
	G_OBJECT_CLASS (burner_drive_selection_parent_class)->constructed (object);

	burner_drive_selection_show_type (BURNER_DRIVE_SELECTION (object),
					   BURNER_DRIVE_TYPE_ALL_BUT_FILE);
}

static void
burner_drive_selection_finalize (GObject *object)
{
	BurnerDriveSelectionPrivate *priv;
	BurnerMediumMonitor *monitor;
  
	priv = BURNER_DRIVE_SELECTION_PRIVATE (object);
  
	monitor = burner_medium_monitor_get_default ();
  
	g_signal_handler_disconnect (monitor, priv->added_sig);
	g_signal_handler_disconnect (monitor, priv->removed_sig);
	priv->removed_sig = 0;
	priv->added_sig = 0;

	g_object_unref (monitor);

	G_OBJECT_CLASS (burner_drive_selection_parent_class)->finalize (object);
  }
  
static void
burner_drive_selection_set_property (GObject *object,
				       guint prop_id,
				       const GValue *value,
				       GParamSpec *pspec)
{
	g_return_if_fail (BURNER_IS_DRIVE_SELECTION (object));
  
	switch (prop_id)
	{
	case PROP_DRIVE_TYPE:
		burner_drive_selection_show_type (BURNER_DRIVE_SELECTION (object),
						   g_value_get_uint (value));
		break;
	case PROP_DRIVE:
		burner_drive_selection_set_active (BURNER_DRIVE_SELECTION (object),
						     BURNER_DRIVE (g_value_get_object (value)));
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
		break;
	}
  }
  
static void
burner_drive_selection_get_property (GObject *object,
				       guint prop_id,
				       GValue *value,
				       GParamSpec *pspec)
{
	BurnerDriveSelectionPrivate *priv;
  
	g_return_if_fail (BURNER_IS_DRIVE_SELECTION (object));
  
	priv = BURNER_DRIVE_SELECTION_PRIVATE (object);
  
	switch (prop_id)
	{
	case PROP_DRIVE_TYPE:
		g_value_set_uint (value, priv->type);
		break;
	case PROP_DRIVE:
		g_value_set_object (value, burner_drive_selection_get_active (BURNER_DRIVE_SELECTION (object)));
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
		break;
	}
}
  
static void
burner_drive_selection_class_init (BurnerDriveSelectionClass *klass)
{
	GObjectClass* object_class = G_OBJECT_CLASS (klass);
	GtkComboBoxClass *combo_class = GTK_COMBO_BOX_CLASS (klass);
  
	g_type_class_add_private (klass, sizeof (BurnerDriveSelectionPrivate));
  
	object_class->finalize = burner_drive_selection_finalize;
	object_class->set_property = burner_drive_selection_set_property;
	object_class->get_property = burner_drive_selection_get_property;
	object_class->constructed = burner_drive_selection_constructed;
  
	combo_class->changed = burner_drive_selection_changed;

	g_object_class_install_property (object_class, PROP_DRIVE,
					 g_param_spec_object ("Drive",
							      "Selected drive",
							      "The currently selected drive",
							      BURNER_TYPE_DRIVE, G_PARAM_READWRITE));

	g_object_class_install_property (object_class, PROP_DRIVE_TYPE,
					 g_param_spec_uint ("drive-type",
							    "The type of drives",
							    "The type of drives displayed",
							    0, BURNER_DRIVE_TYPE_ALL,
							    BURNER_DRIVE_TYPE_ALL_BUT_FILE,
							    G_PARAM_READWRITE));
	/**
 	* BurnerDriveSelection::drive-changed:
 	* @selector: the object which received the signal
  	* @drive: the drive which is now selected
	*
 	* This signal gets emitted when the selected medium has changed
 	*
 	*/
	burner_drive_selection_signals [CHANGED_SIGNAL] =
	    g_signal_new ("drive_changed",
			  BURNER_TYPE_DRIVE_SELECTION,
			  G_SIGNAL_RUN_FIRST|G_SIGNAL_ACTION|G_SIGNAL_NO_RECURSE,
			  G_STRUCT_OFFSET (BurnerDriveSelectionClass, drive_changed),
			  NULL,
			  NULL,
			  g_cclosure_marshal_VOID__OBJECT,
			  G_TYPE_NONE,
			  1,
			  BURNER_TYPE_DRIVE);
}

/**
 * burner_drive_selection_new:
 *
 * Creates a new #BurnerDriveSelection object
 *
 * Return value: a #GtkWidget. Unref when it is not needed anymore.
 **/

GtkWidget *
burner_drive_selection_new (void)
{
	return g_object_new (BURNER_TYPE_DRIVE_SELECTION, NULL);
}
