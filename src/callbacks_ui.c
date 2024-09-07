#include "callbacks_ui.h"
#include "audiosystem.h"
#include "contextmenu.h"
#include "dialog.h"
#include "enum_types.h"
#include "filelister.h"
#include "gdk/gdk.h"
#include "glib.h"
#include "gtk/gtk.h"
#include "musicapp.h"
#include "playlist.h"
#include "trackwidget.h"

void
volumeScale_value_changed(GtkRange* self, gpointer user_data)
{
  audio_system_set_volume(gtk_range_get_value(self));
}

void
prevButton_clicked(GtkButton* self, gpointer user_data)
{
  MusicApp* app = MUSIC_APP(user_data);
  Track* track =
    playlist_get_prev_track(music_app_get_active_playlist(app),
                            music_app_get_current_track(app)->index);
  music_app_set_current_track(app, track);
  music_app_play_track(app);
}

void
nextButton_clicked(GtkButton* self, gpointer user_data)
{
  MusicApp* app = MUSIC_APP(user_data);
  Track* track = music_app_get_current_track(app);
  if (music_app_get_options(app) & PLAYBACK_SHUFFLE) {
    Playlist* playlist = music_app_get_active_playlist(app);
    guint index = track->index;
    while (track->index == index) {
      track =
        playlist_get_track(playlist, rand() % playlist_get_length(playlist));
    }
  } else {
    track =
      playlist_get_next_track(music_app_get_active_playlist(app), track->index);
  }
  music_app_set_current_track(app, track);
  music_app_play_track(app);
}

void
playButton_clicked(GtkButton* self, gpointer user_data)
{
  MusicApp* app = MUSIC_APP(user_data);
  Track* current = music_app_get_current_track(app);
  if (current == NULL) {
    Playlist* playlist = music_app_get_selected_playlist(app);
    if (playlist != NULL) {
      current = playlist_get_track(playlist, 0);
      if (current == NULL)
        return;
      music_app_set_current_track(app, current);
      music_app_set_active_playlist(app, playlist);
      music_app_play_track(app);
    }
    return;
  }
  if (audio_system_get_state() == AUDIO_PLAYING) {
    audio_system_pause_audio();
  } else {
    if (audio_system_get_state() == AUDIO_STOPPED) {
      if (audio_system_open_audio(current->path) == -1)
        return;
    }
    audio_system_play_audio();
  }
  music_app_update_current_track_widget(app, audio_system_get_state());
  music_app_switch_playback_icon(app, BUTTON_PLAY);
}

void
trackWidget_clicked(GtkGestureClick* self,
                    gint n_press,
                    gdouble x,
                    gdouble y,
                    gpointer user_data)
{
  GtkWidget* widget =
    gtk_event_controller_get_widget(GTK_EVENT_CONTROLLER(self));

  switch (gtk_gesture_single_get_current_button(GTK_GESTURE_SINGLE(self))) {
    case GDK_BUTTON_PRIMARY: {
      MusicApp* app = MUSIC_APP(user_data);
      GPtrArray* selected = music_app_get_selected_track_widgets(app);
      GdkEvent* event = gtk_gesture_get_last_event(GTK_GESTURE(self), NULL);
      int count = 0;
      bool keyPress = false;
      if (event != NULL) {
        GdkModifierType state = gdk_event_get_modifier_state(event);
        if (state & GDK_SHIFT_MASK) {
          if (selected->len > 0 && g_ptr_array_index(selected, 0) != widget) {
            keyPress = true;
            int start = track_widget_get_index(
              APP_TRACK_WIDGET(g_ptr_array_index(selected, selected->len - 1)));
            int end = track_widget_get_index(APP_TRACK_WIDGET(widget));
            if (start < end) {
              start++;
            } else {
              start--;
            }
            count = music_app_retrieve_track_widgets(app, start, end, selected);
          }
        } else if (state & GDK_CONTROL_MASK) {
          keyPress = true;
        }
      }
      if (!keyPress) {
        for (int i = selected->len - 1; i >= 0; i--) {
          gtk_widget_remove_css_class(
            g_ptr_array_remove_index_fast(selected, i), "trackWidget-selected");
        }
      }
      music_app_set_flag(app, FLAG_MULTISELECT, keyPress);
      g_ptr_array_add(selected, (const gpointer)widget);
      count++;

      // default selection handler
      if (!music_app_invoke_selection_cb(app, count)) {
        for (int i = selected->len - count; i < selected->len; i++) {
          gtk_widget_add_css_class(g_ptr_array_index(selected, i),
                                   "trackWidget-selected");
        }
      }
      break;
    }

    case GDK_BUTTON_SECONDARY: {
      GtkWidget* menu = GTK_WIDGET(context_menu_get_menu(true));
      if (gtk_widget_get_parent(menu) != widget) {
        gtk_widget_unparent(menu);
        gtk_widget_set_parent(GTK_WIDGET(menu), widget);
        context_menu_set_data(widget);
      }
      gtk_popover_set_pointing_to(GTK_POPOVER(menu),
                                  &(GdkRectangle){ x, y, 1, 1 });
      gtk_popover_popup(GTK_POPOVER(menu));
      break;
    }
  }
}

void
createPlaylist_clicked(GtkButton* self, gpointer user_data)
{
  MusicApp* app = MUSIC_APP(user_data);

  Playlist* current = NULL;
  guint lastIndex = music_app_get_playlists_count(app) - 1;
  current = music_app_get_playlist(app, lastIndex);
  if (playlist_is_new(current)) {
    music_app_dropdown_select(app, lastIndex);
    return;
  }
  current =
    playlist_new(g_strdup("New Playlist"), g_strdup("playlists"), PLAYLIST_NEW);

  music_app_add_playlist(app, current);

  music_app_dropdown_select(app, ++lastIndex);
}

void
selection_changed(GtkDropDown* dropdown, GParamSpec* pspec, gpointer user_data)
{
  MusicApp* app = user_data;
  Track* track = NULL;
  music_app_clear_track_widgets(app);
  g_ptr_array_set_size(music_app_get_selected_track_widgets(app), 0);
  guint position = gtk_drop_down_get_selected(dropdown);
  if (position != GTK_INVALID_LIST_POSITION) {
    Playlist* playlist = music_app_get_playlist(app, position);
    GPtrArray* tracks = playlist_get_tracks(playlist);
    for (int i = 0; i < tracks->len; i++) {
      track = g_ptr_array_index(tracks, i);
      music_app_add_track_widget(app, track_widget_new(app, track));
    }
    track = music_app_get_current_track(app);
    if (track != NULL && track == g_ptr_array_index(tracks, track->index) &&
        music_app_get_active_playlist(app) == playlist) {
      music_app_update_current_track_widget(app, audio_system_get_state());
    }
  }
}

static void
change_playlist_info(gpointer user_data)
{
  DialogData* data = user_data;
  const char* text = data->user_data1;
  GtkListItem* item = data->user_data2;
  Playlist* playlist = gtk_list_item_get_item(item);

  if (g_object_get_data(G_OBJECT(item), "flag")) {
    playlist_set_description(playlist, text);
    g_object_set_data(G_OBJECT(item), "flag", NULL);
  } else {
    playlist_rename(playlist, text);
  }

  playlist_save(playlist);
}

void
playlist_clicked(GtkGestureClick* self,
                 gint n_press,
                 gdouble x,
                 gdouble y,
                 gpointer user_data)
{
  GtkWidget* menu = GTK_WIDGET(context_menu_get_menu(false));
  GtkWidget* box = gtk_list_item_get_child(user_data);
  if (context_menu_get_callback() != change_playlist_info) {
    context_menu_set_callback(change_playlist_info);
  }

  gtk_widget_unparent(menu);
  gtk_widget_set_parent(menu, box);
  context_menu_set_data(user_data);
  if (context_menu_get_data() != user_data) {
    context_menu_set_data(user_data);
  }
  gtk_popover_set_pointing_to(GTK_POPOVER(menu), &(GdkRectangle){ x, y, 1, 1 });
  gtk_popover_popup(GTK_POPOVER(menu));
}

void
on_text_field_dialog_response(GtkButton* self, gpointer user_data)
{
  DialogData* data = user_data;
  GtkWindow* dialog = data->dialog;
  GtkEntry* entry = data->user_data1;

  GtkEntryBuffer* buffer = gtk_entry_get_buffer(entry);

  if (gtk_entry_buffer_get_length(buffer) > 0) {
    if (context_menu_get_callback() != NULL) {
      data->user_data1 = (gpointer)gtk_entry_buffer_get_text(buffer);
      context_menu_trigger_callback(data);
    }
  }

  dialog_free(dialog);
  g_free(data);
}

void
openFiles_cb(GtkFileDialog* source, GAsyncResult* res, gpointer user_data)
{
  GListModel* list = gtk_file_dialog_open_multiple_finish(source, res, NULL);
  if (list) {
    DialogData* data = user_data;
    MusicApp* app = MUSIC_APP(data->app);
    Playlist* playlist = gtk_list_item_get_item(data->user_data1);
    guint appendCount = g_list_model_get_n_items(list);
    for (guint i = 0; i < appendCount; i++) {
      GFile* file = g_list_model_get_item(list, i);
      gchar* path = g_file_get_path(file);
      Track* track = fetch_track(path);
      playlist_add(playlist, track);
      g_object_unref(file);
      g_free(path);
    }
    if (playlist == music_app_get_selected_playlist(app)) {
      guint index = playlist_get_length(playlist) - appendCount;
      guint length = playlist_get_length(playlist) - 1;
      while (index < length) {
        Track* track = playlist_get_track(playlist, index);
        music_app_add_track_widget(app, track_widget_new(app, track));
        index++;
      }
    }
    music_app_shift_playlists_lines(app, appendCount, playlist_save(playlist));

    g_free(data);
    g_object_unref(list);
  }
}

void
audioPositionScale_value_changed(GtkRange* self, gpointer user_data)
{
  if (!audio_system_is_manual_position()) {
    music_app_update_audio_position_label(user_data);
  } else {
    long int length = audio_system_get_length();
    if (length != -1) {
      music_app_update_position_label_by_length(user_data, length);
    }
  }
}

void
audioPositionScale_pressed(GtkGestureClick* self,
                           gint n_press,
                           gdouble x,
                           gdouble y,
                           gpointer user_data)
{
  audio_system_set_is_manual_position(true);
  long int length = audio_system_get_length();
  if (length != -1) {
    music_app_update_position_label_by_length(user_data, length);
  }
}

void
audioPositionScale_released(GtkGestureClick* self,
                            gint n_press,
                            gdouble x,
                            gdouble y,
                            gpointer user_data)
{
  audio_system_set_audio_position(gtk_range_get_value(GTK_RANGE(user_data)));
  audio_system_set_is_manual_position(false);
}

static void
openFolder_cb(GtkFileDialog* source, GAsyncResult* res, gpointer user_data)
{
  MusicApp* app = MUSIC_APP(user_data);
  GFile* folder = gtk_file_dialog_select_folder_finish(source, res, NULL);

  if (folder == NULL)
    return;

  gchar* path = g_file_get_path(folder);
  if (path != NULL) {
    GPtrArray* arr = list_audio_files(path);

    Playlist* playlist = playlist_new(path, path, PLAYLIST_FOLDER);

    for (int i = 0; i < arr->len; i++) {
      gpointer str = g_ptr_array_index(arr, i);

      // We're not freeing str pointer because it will be passed to
      // track and then stored in track struct
      Track* track = fetch_track(str);
      playlist_add(playlist, track);
    }
    music_app_add_playlist(app, playlist);
    music_app_dropdown_select(app, 0);

    g_free(path);
    g_object_unref(folder);
  }
}

void
openFolder_clicked(GtkButton* self, gpointer user_data)
{
  MusicApp* app = MUSIC_APP(user_data);
  GtkFileDialog* dialog = gtk_file_dialog_new();

  gtk_file_dialog_set_title(dialog, "Choose directory");
  gtk_file_dialog_set_modal(dialog, TRUE);
  gtk_file_dialog_select_folder(dialog,
                                music_app_get_main_window(app),
                                NULL,
                                (GAsyncReadyCallback)openFolder_cb,
                                app);
}

void
playtrackButton_clicked(GtkButton* self, gpointer user_data)
{
  MusicApp* app = MUSIC_APP(user_data);
  Track* new = track_widget_get_track(
    APP_TRACK_WIDGET(gtk_widget_get_parent(GTK_WIDGET(self))));

  if (audio_system_get_state() != AUDIO_STOPPED) {
    if (music_app_get_selected_playlist(app) ==
        music_app_get_active_playlist(app)) {
      if (audio_system_get_state() == AUDIO_PAUSED) {
        music_app_update_current_track_widget(app, AUDIO_PLAYING);
        audio_system_resume_audio();
      } else {
        music_app_update_current_track_widget(app, AUDIO_PAUSED);
        audio_system_pause_audio();
      }
      music_app_switch_playback_icon(app, BUTTON_PLAY);
      return;
    }
  }
  music_app_set_active_playlist(app, music_app_get_selected_playlist(app));
  music_app_set_current_track(app, new);
  music_app_play_track(app);
}

void
shuffleButton_clicked(GtkButton* self, gpointer user_data)
{
  MusicApp* app = MUSIC_APP(user_data);
  music_app_set_options(app, music_app_get_options(app) ^ PLAYBACK_SHUFFLE);
  music_app_switch_playback_icon(app, BUTTON_SHUFFLE);
}

void
loopButton_clicked(GtkButton* self, gpointer user_data)
{
  MusicApp* app = MUSIC_APP(user_data);
  PlaybackOptions options = music_app_get_options(app);
  if (options & PLAYBACK_ONETRACK) {
    options = (options ^ PLAYBACK_ONETRACK) ^ PLAYBACK_LOOP;
    music_app_set_options(app, options);
  } else {
    if (options & PLAYBACK_LOOP) {
      music_app_set_options(app, options ^ PLAYBACK_LOOP);
    } else {
      music_app_set_options(app, options ^ PLAYBACK_ONETRACK);
    }
  }
  music_app_switch_playback_icon(app, BUTTON_LOOP);
}

void
playlist_info_changed(Playlist* playlist, gpointer user_data)
{
  GtkLabel* nameLabel = NULL;
  GtkLabel* descriptionLabel = NULL;
  const char* name = playlist_get_name(playlist);
  const char* desc = playlist_get_description(playlist);

  GPtrArray* labels = g_object_get_data(G_OBJECT(user_data), "labels");
  if (!labels) {
    return;
  }
  nameLabel = GTK_LABEL(g_ptr_array_index(labels, 0));
  descriptionLabel = GTK_LABEL(g_ptr_array_index(labels, 1));

  if (g_strcmp0(gtk_label_get_text(nameLabel), name)) {
    gtk_label_set_text(nameLabel, playlist_get_name(playlist));
  }
  if (g_strcmp0(gtk_label_get_text(descriptionLabel), desc)) {
    gtk_label_set_text(descriptionLabel, playlist_get_description(playlist));
  }
}