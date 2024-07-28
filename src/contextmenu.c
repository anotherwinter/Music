#include "contextmenu.h"
#include "dialog.h"
#include "glib.h"
#include "gtk/gtk.h"
#include "musicapp.h"
#include "playlist.h"
#include "timsort.h"
#include "trackwidget.h"

struct ContextMenu
{
  GtkPopoverMenu* trackWidgetMenu;
  GtkPopoverMenu* playlistMenu;
  MusicApp* app;
  gpointer data;
};

static ContextMenu* context_menu = NULL;

static void
on_remove_track_action(GSimpleAction* action,
                       GVariant* parameter,
                       gpointer user_data)
{
  gtk_widget_unparent(GTK_WIDGET(context_menu->trackWidgetMenu));
  music_app_remove_track_widget(context_menu->app,
                                APP_TRACK_WIDGET(context_menu->data));
}

static void
on_sort_tracks_action(GSimpleAction* action,
                      GVariant* parameter,
                      gpointer user_data)
{
  Playlist* playlist = music_app_get_active_playlist(context_menu->app);
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
  guint* index = g_new(guint, 1);
  *index = gtk_list_item_get_position(
    g_object_get_data(G_OBJECT(context_menu->data), "item"));
  data->dialog = index;
  data->user_data1 = context_menu->app;
  data->user_data2 =
    g_object_get_data(G_OBJECT(context_menu->data), "playlist");
  dialog_create_file_dialog(context_menu->app, data);
}

static void
on_rename_playlist_action(GSimpleAction* action,
                          GVariant* parameter,
                          gpointer user_data)
{
  dialog_create_text_input_for_app(context_menu->app, context_menu->data);
}

static void
on_duplicate_playlist_action(GSimpleAction* action,
                             GVariant* parameter,
                             gpointer user_data)
{
  music_app_duplicate_playlist(
    context_menu->app,
    g_object_get_data(G_OBJECT(context_menu->data), "playlist"));
}

static void
on_remove_playlist_action(GSimpleAction* action,
                          GVariant* parameter,
                          gpointer user_data)
{
  GListStore* store =
    G_LIST_STORE(g_object_get_data(G_OBJECT(context_menu->data), "store"));
  GtkListItem* item =
    GTK_LIST_ITEM(g_object_get_data(G_OBJECT(context_menu->data), "item"));

  if (item != NULL) {
    // Remove the item from the store, remove and free playlist and trackwidgets
    // associated with that item
    MusicApp* app = MUSIC_APP(context_menu->app);
    int index = gtk_list_item_get_position(item);
    Playlist* playlist = music_app_get_playlist(app, index);

    if (playlist != NULL) {
      int offset = playlist_delete(playlist);
      music_app_remove_playlist(app, index);
      if (offset) {
        music_app_shift_playlists_lines(app, index, offset);
      }
    }

    // This is hack for fixing auto-select not emitting notify signal when
    // appended to/removed from liststore
    music_app_dropdown_select(app, GTK_INVALID_LIST_POSITION);

    g_list_store_remove(store, index);
  }
}

void
context_menu_init(GtkApplication* app)
{
  if (context_menu == NULL) {
    context_menu = g_malloc(sizeof(ContextMenu));
    context_menu->app = MUSIC_APP(app);
  }

  GtkBuilder* builder =
    gtk_builder_new_from_resource("/org/aw/Music/menus.xml");
  GMenuModel* menu_model =
    G_MENU_MODEL(gtk_builder_get_object(builder, "trackwidget-menu"));
  context_menu->trackWidgetMenu =
    GTK_POPOVER_MENU(gtk_popover_menu_new_from_model(menu_model));

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