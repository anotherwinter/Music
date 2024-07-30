#include "factory.h"
#include "contextmenu.h"
#include "handlers.h"

// Create "blank" widget for setting data on it then in bind_factory()
static void
list_factory_setup(GtkSignalListItemFactory* factory,
                   GtkListItem* list_item,
                   gpointer user_data)
{
  GtkWidget* box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
  GtkWidget* nameLabel = gtk_label_new(NULL);
  GtkWidget* descriptionLabel = gtk_label_new(NULL);
  gtk_label_set_justify(GTK_LABEL(descriptionLabel), GTK_JUSTIFY_CENTER);
  gtk_widget_set_valign(nameLabel, GTK_ALIGN_CENTER);
  gtk_widget_set_valign(descriptionLabel, GTK_ALIGN_CENTER);

  gtk_box_append(GTK_BOX(box), nameLabel);
  gtk_box_append(GTK_BOX(box), descriptionLabel);
  gtk_list_item_set_child(list_item, box);
  gtk_widget_add_css_class(nameLabel, "nameText");
  gtk_widget_add_css_class(descriptionLabel, "descriptionText");
}

// Bind data to widget after creating it
static void
list_factory_bind(GtkSignalListItemFactory* factory,
                  GtkListItem* list_item,
                  gpointer user_data)
{
  GtkWidget* box = gtk_list_item_get_child(GTK_LIST_ITEM(list_item));
  GtkWidget* nameLabel = gtk_widget_get_first_child(box);
  GtkWidget* descriptionLabel = gtk_widget_get_last_child(box);
  Playlist* playlist = APP_PLAYLIST(gtk_list_item_get_item(list_item));

  gtk_label_set_text(GTK_LABEL(nameLabel), playlist_get_name(playlist));
  gtk_label_set_text(GTK_LABEL(descriptionLabel),
                     playlist_get_description(playlist));

  GPtrArray* labels = g_ptr_array_new();
  g_ptr_array_add(labels, nameLabel);
  g_ptr_array_add(labels, descriptionLabel);
  g_object_set_data(G_OBJECT(list_item), "labels", labels);

  GPtrArray* handlers_ids = g_ptr_array_new();

  GtkGesture* gesture = gtk_gesture_click_new();
  gtk_gesture_single_set_button(GTK_GESTURE_SINGLE(gesture), 3);
  gtk_widget_add_controller(gtk_widget_get_parent(box), GTK_EVENT_CONTROLLER(gesture));
  g_object_set_data(G_OBJECT(list_item), "gesture", gesture);

  // Connect gesture and add it's handler id for further disconnecting
  g_ptr_array_add(
    handlers_ids,
    GINT_TO_POINTER(g_signal_connect(
      gesture, "pressed", G_CALLBACK(playlist_clicked), list_item)));

  // Same but connecting to playlist "info-changed" signal
  g_ptr_array_add(
    handlers_ids,
    GINT_TO_POINTER(g_signal_connect(
      playlist, "info-changed", G_CALLBACK(playlist_info_changed), list_item)));

  g_object_set_data(G_OBJECT(list_item), "handlers", handlers_ids);
}

static void
list_factory_unbind(GtkSignalListItemFactory* self,
                    GtkListItem* list_item,
                    gpointer user_data)
{
  GtkWidget* box = gtk_list_item_get_child(list_item);
  GtkWidget* menu = GTK_WIDGET(context_menu_get_menu(false));

  if (gtk_widget_get_parent(menu) == box) {
    gtk_widget_unparent(menu);
  }

  GPtrArray* labels = g_object_get_data(G_OBJECT(list_item), "labels");
  g_ptr_array_free(labels, FALSE);
  g_object_set_data(G_OBJECT(list_item), "labels", NULL);

  GPtrArray* arr = g_object_get_data(G_OBJECT(list_item), "handlers");
  GtkGesture* gesture = g_object_get_data(G_OBJECT(list_item), "gesture");
  g_signal_handler_disconnect(gesture,
                              GPOINTER_TO_INT(g_ptr_array_index(arr, 0)));
  gtk_widget_remove_controller(gtk_widget_get_parent(box), GTK_EVENT_CONTROLLER(gesture));

  Playlist* playlist = APP_PLAYLIST(gtk_list_item_get_item(list_item));
  g_signal_handler_disconnect(playlist,
                              GPOINTER_TO_INT(g_ptr_array_index(arr, 1)));

  g_ptr_array_free(arr, TRUE);

    gtk_widget_remove_css_class(gtk_widget_get_parent(box), "playlistPopover");

  g_object_set_data(G_OBJECT(list_item), "handlers", NULL);
  g_object_set_data(G_OBJECT(list_item), "gesture", NULL);
}

static void
list_factory_teardown(GtkSignalListItemFactory* factory,
                      GtkListItem* list_item,
                      gpointer user_data)
{
}

GtkListItemFactory*
music_app_setup_list_factory(MusicApp* app)
{
  GtkListItemFactory* list_factory = gtk_signal_list_item_factory_new();

  g_signal_connect(list_factory, "setup", G_CALLBACK(list_factory_setup), app);
  g_signal_connect(list_factory, "bind", G_CALLBACK(list_factory_bind), app);
  g_signal_connect(
    list_factory, "unbind", G_CALLBACK(list_factory_unbind), app);
  g_signal_connect(
    list_factory, "teardown", G_CALLBACK(list_factory_teardown), NULL);

  return list_factory;
}