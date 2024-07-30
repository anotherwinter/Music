#include "handlers.h"
#include "audiosystem.h"
#include "contextmenu.h"
#include "dialog.h"
#include "filelister.h"
#include "musicapp.h"
#include "trackwidget.h"

static void
openFolder_cb(GtkFileDialog* source, GAsyncResult* res, gpointer user_data)
{
  MusicApp* app = MUSIC_APP(user_data);
  GFile* folder = gtk_file_dialog_select_folder_finish(source, res, NULL);

  if (folder == NULL)
    return;

  gchar* path = g_file_get_path(folder);
  if (path != NULL) {
    g_print("Selected folder: %s\n", path);
    GPtrArray* arr = list_audio_files(path);

    Playlist* playlist = playlist_new(path, path, PLAYLIST_FOLDER);

    for (int i = 0; i < arr->len; i++) {
      gpointer str = g_ptr_array_index(arr, i);

      // We're not freeing str pointer because it will be passed to
      // track and then stored in track struct
      Track* track = fetch_track(str);
      playlist_add(playlist, track);
      g_print("%s\n", (gchar*)str);
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
  TrackWidget* current = music_app_get_current_track_widget(app);
  TrackWidget* new = APP_TRACK_WIDGET(gtk_widget_get_parent(GTK_WIDGET(self)));
  if (audio_system_is_playing()) {
    int active = track_widget_get_state(new);

    // Assuming that if new widget is active, then it's the same widget as
    // previous therefore it can't be NULL
    if (active) {
      if (audio_system_is_paused()) {
        track_widget_set_state(current, TRACK_PLAYING);
        audio_system_resume_audio();
      } else {
        track_widget_set_state(current, TRACK_PAUSED);
        audio_system_pause_audio();
      }
      music_app_switch_playback_icon(app, BUTTON_PLAY);
      return;
    }
  }
  music_app_play_widget(app, new);
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

static gboolean
update_ui_from_music_finished(gpointer user_data)
{
  if (audio_system_is_stopped()) {
    return G_SOURCE_REMOVE;
  }

  MusicApp* app = MUSIC_APP(user_data);

  gtk_range_set_value(GTK_RANGE(music_app_get_audio_position_scale(app)), 1.0f);

  PlaybackOptions options = music_app_get_options(app);
  if (options & PLAYBACK_ONETRACK) {
    audio_system_play_audio();
    return G_SOURCE_REMOVE;
  }
  TrackWidget* widget = music_app_get_current_track_widget(app);

  if (options == PLAYBACK_NONE) {
    track_widget_set_state(widget, TRACK_INACTIVE);
    audio_system_stop_audio();
    music_app_switch_playback_icon(app, BUTTON_PLAY);
    return G_SOURCE_REMOVE;
  }

  Playlist* playlist = music_app_get_active_playlist(app);
  Track* track = track_widget_get_track(widget);
  if (options & PLAYBACK_SHUFFLE) {
    while (track->index == track_widget_get_index(widget)) {
      track =
        playlist_get_track(playlist, rand() % playlist_get_length(playlist));
    }
  } else {
    track = playlist_get_next_track(playlist, track->index);
  }
  music_app_play_widget(app, music_app_get_track_widget(app, track->index));

  return G_SOURCE_REMOVE;
}

void
music_finished(const libvlc_event_t* event, void* user_data)
{
  g_idle_add(update_ui_from_music_finished, user_data);
}

void
volumeScale_value_changed(GtkRange* self, gpointer user_data)
{
  audio_system_set_volume(gtk_range_get_value(self));
}

void
prevButton_clicked(GtkButton* self, gpointer user_data)
{
  MusicApp* app = MUSIC_APP(user_data);
  TrackWidget* widget = music_app_get_current_track_widget(app);
  Track* track = playlist_get_prev_track(music_app_get_active_playlist(app),
                                         track_widget_get_index(widget));
  music_app_play_widget(app, music_app_get_track_widget(app, track->index));
}

void
nextButton_clicked(GtkButton* self, gpointer user_data)
{
  MusicApp* app = MUSIC_APP(user_data);
  TrackWidget* currentWidget = music_app_get_current_track_widget(app);
  Track* track = NULL;
  if (music_app_get_options(app) & PLAYBACK_SHUFFLE) {
    Playlist* playlist = music_app_get_active_playlist(app);
    track = track_widget_get_track(currentWidget);
    while (track->index == track_widget_get_index(currentWidget)) {
      track =
        playlist_get_track(playlist, rand() % playlist_get_length(playlist));
    }
  } else {
    track = playlist_get_next_track(music_app_get_active_playlist(app),
                                    track_widget_get_index(currentWidget));
  }
  music_app_play_widget(app, music_app_get_track_widget(app, track->index));
}

void
playButton_clicked(GtkButton* self, gpointer user_data)
{
  MusicApp* app = MUSIC_APP(user_data);
  TrackWidget* widget = music_app_get_current_track_widget(app);
  if (widget == NULL) {
    widget = music_app_get_track_widget(app, 0);
    if (widget == NULL)
      return;
    music_app_play_widget(app, widget);
    return;
  }
  if (audio_system_is_playing()) {
    audio_system_pause_audio();
    track_widget_set_state(widget, TRACK_PAUSED);
  } else {
    if (audio_system_is_stopped()) {
      if (audio_system_open_audio(track_widget_get_track(widget)->path) == -1)
        return;
    }
    audio_system_play_audio();
    track_widget_set_state(widget, TRACK_PLAYING);
  }
  music_app_switch_playback_icon(app, BUTTON_PLAY);
}

void
trackWidget_clicked(GtkGestureClick* self,
                    gint n_press,
                    gdouble x,
                    gdouble y,
                    gpointer user_data)
{
  GtkWidget* menu = GTK_WIDGET(context_menu_get_menu(true));
  if (gtk_widget_get_parent(menu) != GTK_WIDGET(user_data)) {
    gtk_widget_unparent(menu);
    gtk_widget_set_parent(GTK_WIDGET(menu), GTK_WIDGET(user_data));
    context_menu_set_data(user_data);
  }
  gtk_popover_set_pointing_to(GTK_POPOVER(menu), &(GdkRectangle){ x, y, 1, 1 });
  gtk_popover_popup(GTK_POPOVER(menu));
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
  music_app_reset_current_track_widget(app);
  music_app_clear_track_widgets(app);

  guint position = gtk_drop_down_get_selected(dropdown);
  if (position != GTK_INVALID_LIST_POSITION) {
    GPtrArray* tracks =
      playlist_get_tracks(music_app_get_playlist(app, position));
    for (int i = 0; i < tracks->len; i++) {
      Track* track = g_ptr_array_index(tracks, i);
      music_app_add_track_widget(app, track_widget_new(app, track));
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

  gtk_window_destroy(dialog);
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
    if (playlist == music_app_get_active_playlist(app)) {
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

static gboolean
update_ui_from_time_changed(gpointer user_data)
{
  gtk_range_set_value(GTK_RANGE(user_data), audio_system_get_audio_position());
  return G_SOURCE_REMOVE;
}

void
music_time_changed(const libvlc_event_t* event, void* user_data)
{
  if (!audio_system_is_manual_position()) {
    // Because it's not thread-safe to access/change UI elements from all but
    // main thread (vlc runs in some other thread) we should queue changes in
    // main thread using g_idle_add
    g_idle_add(update_ui_from_time_changed, user_data);
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

static gboolean
update_ui_from_length_changed(gpointer user_data)
{
  music_app_update_length_label(user_data);
  return G_SOURCE_REMOVE;
}

void
media_player_length_changed(const libvlc_event_t* event, void* user_data)
{
  g_idle_add(update_ui_from_length_changed, user_data);
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