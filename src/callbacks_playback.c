#include "callbacks_playback.h"
#include "audiosystem.h"
#include "enum_types.h"
#include "musicapp.h"

static gboolean
update_ui_from_music_finished(gpointer user_data)
{
  if (audio_system_get_state() == AUDIO_STOPPED) {
    return G_SOURCE_REMOVE;
  }
  MusicApp* app = MUSIC_APP(user_data);

  gtk_range_set_value(GTK_RANGE(music_app_get_audio_position_scale(app)), 1.0f);

  PlaybackOptions options = music_app_get_options(app);
  if (options & PLAYBACK_ONETRACK) {
    audio_system_restart_media();
    audio_system_play_audio();
    return G_SOURCE_REMOVE;
  }

  if (options == PLAYBACK_NONE) {
    audio_system_stop_audio();
    music_app_update_current_track_widget(app, AUDIO_STOPPED);
    music_app_switch_playback_icon(app, BUTTON_PLAY);
    return G_SOURCE_REMOVE;
  }

  Playlist* playlist = music_app_get_active_playlist(app);
  Track* track = music_app_get_current_track(app);
  guint index = track->index;
  if (options & PLAYBACK_SHUFFLE) {
    while (track->index == index) {
      track =
        playlist_get_track(playlist, rand() % playlist_get_length(playlist));
    }
  } else {
    track = playlist_get_next_track(playlist, track->index);
  }
  music_app_set_current_track(app, track);
  music_app_play_track(app);

  return G_SOURCE_REMOVE;
}

void
music_finished(const libvlc_event_t* event, void* user_data)
{
  g_idle_add(update_ui_from_music_finished, user_data);
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