#pragma once
#include "enum_types.h"
#include "playlist.h"
#include <gtk/gtk.h>

#define MUSIC_APP_TYPE (music_app_get_type())
G_DECLARE_FINAL_TYPE(MusicApp, music_app, MUSIC, APP, GtkApplication)

typedef struct _TrackWidget TrackWidget;

MusicApp*
music_app_new();
Playlist*
music_app_get_active_playlist(MusicApp* app);
GtkWindow*
music_app_get_main_window(MusicApp* app);
void
music_app_add_track_widget(MusicApp* app, TrackWidget* widget);
void
music_app_remove_track_widget(MusicApp* app, TrackWidget* widget);
void
music_app_switch_playback_icon(MusicApp* app, ButtonTypes type);
TrackWidget*
music_app_get_current_track_widget(MusicApp* app);

// Sets new current track widget, index is optional and can be G_MAXUINT to set
// by widget, firstly it tries to set by index, then by widget pointer. Current
// track widget can be "cleared" by setting widget to NULL and index to
// G_MAXUINT: there is music_app_reset_current_track_widget() for that task
void
music_app_set_current_track_widget(MusicApp* app,
                                   TrackWidget* widget,
                                   guint index,
                                   TrackWidgetState state);
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
music_app_reset_current_track_widget(MusicApp* app);
void
music_app_dropdown_select(MusicApp* app, guint index);
void
music_app_play_widget(MusicApp* app, TrackWidget* widget);
void
music_app_duplicate_playlist(MusicApp* app, Playlist* playlist);
void
music_app_shift_playlists_lines(MusicApp* app, guint index, int offset);
void
music_app_update_audio_position_label(MusicApp* app);

// Formats milliseconds into hh:mm:ss string
char*
music_app_format_into_time_string(long int milliseconds);

// Takes overall length in ms, then multiplies it by current progression (0.0 to 1.0)
// and sets formatted string
void
music_app_update_position_label_by_length(MusicApp* app, long int milliseconds);
void
music_app_update_length_label(MusicApp* app);
guint
music_app_get_playlists_count(MusicApp* app);
GtkScale*
music_app_get_audio_position_scale(MusicApp* app);