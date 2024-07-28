#pragma once
#include "enum_types.h"
#include "track.h"
#include <gtk/gtk.h>

#define PLAYLIST_TYPE (playlist_get_type())
G_DECLARE_FINAL_TYPE(Playlist, playlist, APP, PLAYLIST, GObject);
typedef struct _Playlist Playlist;

// Creates new playlist with optional path to playlists file. Type could be
// PLAYLIST_NONE for ordinary playlists or PLAYLIST_FOLDER, PLAYLIST_NEW for
// corresponding playlists
Playlist*
playlist_new(gchar* name, gchar* path, PlaylistTypes type);
void
playlist_free(Playlist* playlist);
void
playlist_populate(Playlist* playlist, GPtrArray* tracks);
guint
playlist_get_length(Playlist* playlist);
void
playlist_add(Playlist* playlist, Track* track);
Track*
playlist_get_track(Playlist* playlist, guint index);
void
playlist_rename(Playlist* playlist, const gchar* name);
Track*
playlist_remove_track_by_path(Playlist* playlist, gchar* trackPath);
Track*
playlist_remove_track_by_index(Playlist* playlist, guint index);
bool
playlist_is_folder(Playlist* playlist);
bool
playlist_is_new(Playlist* playlist);
Track*
playlist_get_next_track(Playlist* playlist, guint index);
Track*
playlist_get_prev_track(Playlist* playlist, guint index);
void
playlist_update_indices(Playlist* playlist);
void
playlist_reverse(Playlist* playlist);
GPtrArray*
playlist_get_tracks(Playlist* playlist);
bool
playlist_is_empty(Playlist* playlist);

// Parse playlists and their tracks from given file. file should be formatted in
// a valid manner, otherwise parsing would fail. You could safely parse invalid
// format, but there will be no change for playlists array though.
void
parse_playlists(GPtrArray* playlists, gchar* filename);

const gchar*
playlist_get_name(Playlist* playlist);

// Sets start and end lines for playlist info in configuration file. start/end
// can be G_MAXUINT for changing only one of them.
void
playlist_set_lines_in_conf(Playlist* playlist, guint start, guint end);

// Saves playlist file to a config file. Returns the offset value to shift other
// playlist configurations in file or 0 on error / no offset.
int
playlist_save(Playlist* playlist);

// Deletes playlist from playlists file. Returns the offset value to shift other
// playlist configurations in file.
int
playlist_delete(Playlist* playlist);

// Returns a copy of playlist with type changed to PLAYLIST_NONE.
Playlist*
playlist_duplicate(Playlist* playlist);

// This actually doesn't do anything with lines in file, just changes boundaries
// of a playlist (start line and end line).
void
playlist_offset_lines(Playlist* playlist, int offset);

const gchar*
playlist_get_description(Playlist* playlist);
void
playlist_set_description(Playlist* playlist, const gchar* str);