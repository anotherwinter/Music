#include "contextmenu.h"
#include "dialog.h"
#include "gtk/gtk.h"
#include "playlist.h"
#include "timsort.h"
#include "trackwidget.h"

struct ContextMenu
{
  GtkPopoverMenu* trackWidgetMenu;
  GtkPopoverMenu* playlistMenu;
  MusicApp* app;
  gpointer data;
  ContextMenuCallback callback;
};

static ContextMenu* context_menu = NULL;

static void
on_remove_track_action(GSimpleAction* action,
                       GVariant* parameter,
                       gpointer user_data)
{
  music_app_remove_track_widget(context_menu->app,
                                APP_TRACK_WIDGET(context_menu->data));
}

static void
on_sort_tracks_action(GSimpleAction* action,
                      GVariant* parameter,
                      gpointer user_data)
{
  Playlist* playlist = music_app_get_selected_playlist(context_menu->app);
  playlist_reverse(playlist);
  timSort(playlist_get_tracks(playlist));
  playlist_save(playlist);
  playlist_update_indices(playlist);
  music_app_update_track_widgets_indices(context_menu->app);
}

static void
on_add_tracks_action(GSimpleAction* action,
                     GVariant* parameter,
                     gpointer user_data)
{
  DialogData* data = g_new(DialogData, 1);
  data->app = context_menu->app;
  data->user_data1 = context_menu->data;
  dialog_create_file_dialog(context_menu->app, data);
}

static void
on_rename_playlist_action(GSimpleAction* action,
                          GVariant* parameter,
                          gpointer user_data)
{
  GtkWidget* dialog = GTK_WIDGET(dialog_create_text_input_for_app(
    context_menu->app, context_menu->data, "Rename playlist..."));

  gtk_widget_set_visible(dialog, TRUE);
}

static void
on_duplicate_playlist_action(GSimpleAction* action,
                             GVariant* parameter,
                             gpointer user_data)
{
  music_app_duplicate_playlist(context_menu->app,
                               gtk_list_item_get_item(context_menu->data));
}

static void
on_remove_playlist_action(GSimpleAction* action,
                          GVariant* parameter,
                          gpointer user_data)
{
  // Remove the item from the store, remove and free playlist and trackwidgets
  // associated with that item
  MusicApp* app = MUSIC_APP(context_menu->app);
  int index = gtk_list_item_get_position(context_menu->data);
  Playlist* playlist = APP_PLAYLIST(gtk_list_item_get_item(context_menu->data));

  int offset = playlist_delete(playlist);
  music_app_remove_playlist(app, index);
  if (offset) {
    music_app_shift_playlists_lines(app, index, offset);
  }
}

static void
on_change_playlist_description_action(GSimpleAction* action,
                                      GVariant* parameter,
                                      gpointer user_data)
{
  g_object_set_data(context_menu->data, "flag", "1");
  GtkWidget* dialog = GTK_WIDGET(dialog_create_text_input_for_app(
    context_menu->app, context_menu->data, "Rename playlist..."));

  gtk_widget_set_visible(dialog, TRUE);
}

void
context_menu_init(MusicApp* app)
{
  if (context_menu == NULL) {
    context_menu = g_malloc(sizeof(ContextMenu));
    context_menu->app = app;
    context_menu->data = NULL;
    context_menu->callback = NULL;
  }

  GtkBuilder* builder =
    gtk_builder_new_from_resource("/org/aw/Music/menus.xml");
  GMenuModel* menu_model =
    G_MENU_MODEL(gtk_builder_get_object(builder, "trackwidget-menu"));
  context_menu->trackWidgetMenu =
    GTK_POPOVER_MENU(gtk_popover_menu_new_from_model(menu_model));
  g_object_unref(menu_model);

  GSimpleAction* remove_track_action =
    g_simple_action_new("remove_track", NULL);
  g_signal_connect(
    remove_track_action, "activate", G_CALLBACK(on_remove_track_action), NULL);
  g_action_map_add_action(G_ACTION_MAP(context_menu->app),
                          G_ACTION(remove_track_action));

  GSimpleAction* sort_tracks_action = g_simple_action_new("sort_tracks", NULL);
  g_signal_connect(
    sort_tracks_action, "activate", G_CALLBACK(on_sort_tracks_action), NULL);
  g_action_map_add_action(G_ACTION_MAP(context_menu->app),
                          G_ACTION(sort_tracks_action));

  menu_model = G_MENU_MODEL(gtk_builder_get_object(builder, "playlist-menu"));
  context_menu->playlistMenu =
    GTK_POPOVER_MENU(gtk_popover_menu_new_from_model(menu_model));
  g_object_unref(menu_model);

  g_object_unref(builder);

  GSimpleAction* rename_playlist_action =
    g_simple_action_new("rename_playlist", NULL);
  g_signal_connect(rename_playlist_action,
                   "activate",
                   G_CALLBACK(on_rename_playlist_action),
                   NULL);
  g_action_map_add_action(G_ACTION_MAP(context_menu->app),
                          G_ACTION(rename_playlist_action));

  GSimpleAction* duplicate_playlist_action =
    g_simple_action_new("duplicate_playlist", NULL);
  g_signal_connect(duplicate_playlist_action,
                   "activate",
                   G_CALLBACK(on_duplicate_playlist_action),
                   NULL);
  g_action_map_add_action(G_ACTION_MAP(context_menu->app),
                          G_ACTION(duplicate_playlist_action));

  GSimpleAction* add_tracks_action = g_simple_action_new("add_tracks", NULL);
  g_signal_connect(
    add_tracks_action, "activate", G_CALLBACK(on_add_tracks_action), NULL);
  g_action_map_add_action(G_ACTION_MAP(context_menu->app),
                          G_ACTION(add_tracks_action));

  GSimpleAction* remove_playlist_action =
    g_simple_action_new("remove_playlist", NULL);
  g_signal_connect(remove_playlist_action,
                   "activate",
                   G_CALLBACK(on_remove_playlist_action),
                   NULL);
  g_action_map_add_action(G_ACTION_MAP(context_menu->app),
                          G_ACTION(remove_playlist_action));

  GSimpleAction* change_playlist_description_action =
    g_simple_action_new("change_playlist_description", NULL);
  g_signal_connect(change_playlist_description_action,
                   "activate",
                   G_CALLBACK(on_change_playlist_description_action),
                   NULL);
  g_action_map_add_action(G_ACTION_MAP(context_menu->app),
                          G_ACTION(change_playlist_description_action));

  // Adding +1 to reference counter so that when parent
  // for menu is changed by unparenting the menu itself
  // doesn't get destroyed or freed
  g_object_ref(context_menu->trackWidgetMenu);
  g_object_ref(context_menu->playlistMenu);
}

GtkPopoverMenu*
context_menu_get_menu(bool trackwidget)
{
  return trackwidget ? context_menu->trackWidgetMenu
                     : context_menu->playlistMenu;
}

void
context_menu_set_data(gpointer data)
{
  context_menu->data = data;
}

gpointer
context_menu_get_data()
{
  return context_menu->data;
}

ContextMenuCallback
context_menu_get_callback()
{
  return context_menu->callback;
}

void
context_menu_set_callback(ContextMenuCallback cb)
{
  context_menu->callback = cb;
}

void
context_menu_trigger_callback(gpointer user_data)
{
  if (context_menu->callback != NULL) {
    context_menu->callback(user_data);
  }
}

void
context_menu_free()
{
  g_action_map_remove_action(G_ACTION_MAP(context_menu->app), "remove_track");
  g_action_map_remove_action(G_ACTION_MAP(context_menu->app), "sort_tracks");
  g_action_map_remove_action(G_ACTION_MAP(context_menu->app), "add_tracks");
  g_action_map_remove_action(G_ACTION_MAP(context_menu->app),
                             "rename_playlist");
  g_action_map_remove_action(G_ACTION_MAP(context_menu->app),
                             "change_playlist_description");
  g_action_map_remove_action(G_ACTION_MAP(context_menu->app),
                             "duplicate_playlist");
  g_action_map_remove_action(G_ACTION_MAP(context_menu->app),
                             "remove_playlist");
  context_menu->data = NULL;
  context_menu->app = NULL;
  context_menu->callback = NULL;
  g_object_unref(
    gtk_popover_menu_get_menu_model(context_menu->trackWidgetMenu));
  g_object_unref(gtk_popover_menu_get_menu_model(context_menu->playlistMenu));
  gtk_widget_unparent(GTK_WIDGET(context_menu->playlistMenu));
  gtk_widget_unparent(GTK_WIDGET(context_menu->trackWidgetMenu));
  context_menu->playlistMenu = NULL;
  context_menu->trackWidgetMenu = NULL;
  g_free(context_menu);
}