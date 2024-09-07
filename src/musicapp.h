#pragma once
#include "enum_types.h"
#include "glib.h"
#include "playlist.h"
#include <gtk/gtk.h>

#define MUSIC_APP_TYPE (music_app_get_type())
G_DECLARE_FINAL_TYPE(MusicApp, music_app, MUSIC, APP, GtkApplication)

typedef struct _TrackWidget TrackWidget;
typedef void (*SelectionCallback)(GPtrArray* selected, int addedCount);

MusicApp*
music_app_new();
Playlist*
music_app_get_active_playlist(MusicApp* app);
void
music_app_set_active_playlist(MusicApp* app, Playlist* playlist);
Playlist*
music_app_get_selected_playlist(MusicApp* app);
GtkWindow*
music_app_get_main_window(MusicApp* app);
void
music_app_add_track_widget(MusicApp* app, TrackWidget* widget);

// Removes track widget. Pass with batch TRUE to avoid saving playlist after every deletion
void
music_app_remove_track_widget(MusicApp* app, TrackWidget* widget, bool batch);

// Removes track widgets specified in array and then saves playlist
void
music_app_remove_track_widgets_batch(MusicApp* app, GPtrArray* widgets);

void
music_app_switch_playback_icon(MusicApp* app, ButtonTypes type);
Track*
music_app_get_current_track(MusicApp* app);
void
music_app_set_current_track(MusicApp* app, Track* track);
TrackWidget*
music_app_get_track_widget(MusicApp* app, guint index);
PlaybackOptions
music_app_get_options(MusicApp* app);
void
music_app_set_options(MusicApp* app, PlaybackOptions options);

// Only to use with playlist_free(), otherwise expect memleaks
void
music_app_clear_track_widgets(MusicApp* app);

void
music_app_update_track_widgets_indices(MusicApp* app);
Playlist*
music_app_get_playlist(MusicApp* app, guint index);
void
music_app_add_playlist(MusicApp* app, Playlist* playlist);
void
music_app_remove_playlist(MusicApp* app, guint index);
void
music_app_dropdown_select(MusicApp* app, guint index);
void
music_app_play_track(MusicApp* app);
void
music_app_duplicate_playlist(MusicApp* app, Playlist* playlist);
void
music_app_shift_playlists_lines(MusicApp* app, guint index, int offset);
void
music_app_update_audio_position_label(MusicApp* app);

// Formats milliseconds into hh:mm:ss string
char*
music_app_format_into_time_string(long int milliseconds);

// Takes overall length in ms, then multiplies it by current progression (0.0
// to 1.0) and sets formatted string
void
music_app_update_position_label_by_length(MusicApp* app, long int milliseconds);
void
music_app_update_length_label(MusicApp* app);
guint
music_app_get_playlists_count(MusicApp* app);
GtkScale*
music_app_get_audio_position_scale(MusicApp* app);
void
music_app_update_current_track_widget(MusicApp* app, AudioState state);
GPtrArray*
music_app_get_selected_track_widgets(MusicApp* app);
void
music_app_set_selection_cb(MusicApp* app, SelectionCallback cb);
int
music_app_invoke_selection_cb(MusicApp* app, int count);

// Get all track widgets in [start;end) range. Function won't add any pointers
// if boundaries are invalid. Returns amount of widgets added
int
music_app_retrieve_track_widgets(MusicApp* app,
                                 int start,
                                 int end,
                                 GPtrArray* arr);

char
music_app_get_flags(MusicApp* app);
void
music_app_set_flag(MusicApp* app, AppFlags flag, bool value);