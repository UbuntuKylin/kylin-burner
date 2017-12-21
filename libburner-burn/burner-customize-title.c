/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/*
 * Burner
 * Copyright (C) Philippe Rouquier 2005-2009 <bonfire-app@wanadoo.fr>
 * Copyright (C) 2017,Tianjin KYLIN Information Technology Co., Ltd.
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

#include <glib.h>
#include <glib/gstdio.h>
#include <glib/gi18n.h>

#include <gtk/gtk.h>
#include <gdk/gdkx.h>
#include "burner-customize-title.h"


static void
on_exit_cb (GtkWidget *button, GtkWidget *dialog)
{
    if (dialog)
        gtk_dialog_response (dialog, GTK_RESPONSE_CANCEL);
}

void
burner_message_title (GtkWidget *dialog)
{
    GtkWidget *action_area;
    GtkWidget *box;
    GtkWidget *vbox;
    GtkWidget *title;
    GtkWidget *close_bt;
    GtkWidget *label;
    GtkWidget *alignment;
    GtkWidget *image;

    gtk_window_set_decorated(GTK_WINDOW(dialog),FALSE);
    box = gtk_dialog_get_content_area (GTK_DIALOG (dialog));
//    gtk_widget_set_halign(box,GTK_ALIGN_CENTER);
//    gtk_container_set_border_width (GTK_CONTAINER ((gtk_container_get_children(GTK_CONTAINER(box)))->data), 20);
    gtk_style_context_add_class ( gtk_widget_get_style_context (box), "message_dialog");

    action_area = gtk_dialog_get_action_area(dialog);
    gtk_widget_set_hexpand(GTK_WIDGET(action_area), FALSE);
    gtk_widget_set_hexpand_set(GTK_WIDGET(action_area), TRUE);

    vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
    gtk_widget_show (vbox);
    alignment = gtk_alignment_new (0, 0, 1.0, 0.9);
    gtk_widget_show (alignment);
    gtk_container_add (GTK_CONTAINER (box), alignment);
    gtk_container_add (GTK_CONTAINER (alignment), vbox);
    gtk_box_reorder_child(GTK_BOX(box), GTK_WIDGET(alignment), 0);

    title = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_box_pack_start (GTK_BOX (vbox),title , FALSE, TRUE, 4);
    gtk_widget_show (title);
//    gtk_widget_add_events(priv->mainwin, GDK_BUTTON_PRESS_MASK);
//    g_signal_connect(G_OBJECT(priv->mainwin), "button-press-event", G_CALLBACK (drag_handle), NULL);

    label = gtk_label_new (_("Prompt information"));
    gtk_widget_show (label);
    gtk_misc_set_alignment (GTK_MISC (label), 0.5, 0.5);
    gtk_box_pack_start (GTK_BOX (title), label, FALSE, FALSE, 15);

    close_bt = gtk_button_new ();
    gtk_button_set_alignment(GTK_BUTTON(close_bt),0.5,0.5);
    gtk_widget_set_size_request(GTK_WIDGET(close_bt),45,30);
    gtk_widget_show (close_bt);
    gtk_style_context_add_class ( gtk_widget_get_style_context (close_bt), "close_bt");
    gtk_box_pack_end (GTK_BOX (title), close_bt, FALSE, TRUE, 5);

    image = gtk_image_new_from_icon_name ("dialog-message", GTK_ICON_SIZE_DIALOG);
    gtk_widget_show (image);
    gtk_message_dialog_set_image(dialog,image);

    g_signal_connect (close_bt,
              "clicked",
              G_CALLBACK (on_exit_cb),
              dialog);

    gtk_container_set_border_width(GTK_CONTAINER(box), 5);
    burner_dialog_button_image(gtk_dialog_get_action_area(GTK_DIALOG (dialog)));
    return;
}

void
burner_dialog_title (GtkWidget *dialog, gchar *title_label)
{
    GtkWidget *action_area;
    GtkWidget *box;
    GtkWidget *vbox;
    GtkWidget *title;
    GtkWidget *close_bt;
    GtkWidget *label;
    GtkWidget *alignment;

    gtk_window_set_decorated(GTK_WINDOW(dialog),FALSE);
    box = gtk_dialog_get_content_area (GTK_DIALOG (dialog));
//    gtk_widget_set_halign(box,GTK_ALIGN_CENTER);
//    gtk_container_set_border_width (GTK_CONTAINER ((gtk_container_get_children(GTK_CONTAINER(box)))->data), 20);
    gtk_style_context_add_class ( gtk_widget_get_style_context (box), "message_dialog");
    gtk_widget_set_size_request(GTK_WIDGET(dialog),400,400);

    action_area = gtk_dialog_get_action_area(dialog);
    gtk_widget_set_hexpand(GTK_WIDGET(action_area), FALSE);
    gtk_widget_set_hexpand_set(GTK_WIDGET(action_area), TRUE);

    vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
    gtk_widget_show (vbox);
    alignment = gtk_alignment_new (0, 0, 1.0, 0.9);
    gtk_widget_show (alignment);
    gtk_container_add (GTK_CONTAINER (box), alignment);
    gtk_container_add (GTK_CONTAINER (alignment), vbox);
    gtk_box_reorder_child(GTK_BOX(box), GTK_WIDGET(alignment), 0);

    title = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_box_pack_start (GTK_BOX (vbox),title , FALSE, TRUE, 4);
    gtk_widget_show (title);
//    gtk_widget_add_events(priv->mainwin, GDK_BUTTON_PRESS_MASK);
//    g_signal_connect(G_OBJECT(priv->mainwin), "button-press-event", G_CALLBACK (drag_handle), NULL);

    label = gtk_label_new (_(title_label));
    gtk_widget_show (label);
    gtk_misc_set_alignment (GTK_MISC (label), 0.5, 0.5);
    gtk_box_pack_start (GTK_BOX (title), label, FALSE, FALSE, 15);

    close_bt = gtk_button_new ();
    gtk_button_set_alignment(GTK_BUTTON(close_bt),0.5,0.5);
    gtk_widget_set_size_request(GTK_WIDGET(close_bt),45,30);
    gtk_widget_show (close_bt);
    gtk_style_context_add_class ( gtk_widget_get_style_context (close_bt), "close_bt");
    gtk_box_pack_end (GTK_BOX (title), close_bt, FALSE, TRUE, 5);

    gtk_container_set_border_width(GTK_CONTAINER(box), 5);

    g_signal_connect (close_bt,
              "clicked",
              G_CALLBACK (on_exit_cb),
              dialog);
    return;
}


void
burner_dialog_button_image (GtkWidget *properties)
{
    GList *children;
    GList *iter;
    children = gtk_container_get_children (GTK_CONTAINER (properties));
    for (iter = children; iter; iter = iter->next) {
        GtkWidget *widget;
        widget = iter->data;
        if (GTK_IS_BUTTON (widget)) {
            gtk_button_set_always_show_image(GTK_BUTTON(widget), FALSE);
            gtk_widget_hide(gtk_button_get_image(GTK_BUTTON(widget)));
//            gtk_widget_destroy(GTK_WIDGET(gtk_button_get_image(GTK_BUTTON(widget))));
        }
    }

    g_list_free (children);
    return;
}
