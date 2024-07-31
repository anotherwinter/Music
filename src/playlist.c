#include "playlist.h"
#include "filelister.h"

enum
{
  INFO_CHANGED,
  LAST_SIGNAL
};

static guint playlist_signals[LAST_SIGNAL] = { 0 };

struct _Playlist
{
  GObject parent_instance;
  gchar* name;
  gchar* path;
  gchar* description;
  GPtrArray* tracks;
  PlaylistTypes type;
  guint startLine;
  guint endLine;
  bool wasEverSelected;
};

G_DEFINE_TYPE(Playlist, playlist, G_TYPE_OBJECT)

static void
playlist_dispose(GObject* object)
{
  Playlist* playlist = APP_PLAYLIST(object);
  while (playlist->tracks->len > 0) {
    Track* track = g_ptr_array_remove_index_fast(playlist->tracks, 0);
    track_unref(track);
  }
  g_free(playlist->name);
  g_free(playlist->path);
  g_free(playlist->description);

  G_OBJECT_CLASS(playlist_parent_class)->dispose(object);
}

static void
playlist_finalize(GObject* object)
{
  G_OBJECT_CLASS(playlist_parent_class)->finalize(object);
}

static void
playlist_init(Playlist* self)
{
}

static void
playlist_class_init(PlaylistClass* klass)
{
  playlist_signals[INFO_CHANGED] = g_signal_new("info-changed",
                                                G_TYPE_FROM_CLASS(klass),
                                                G_SIGNAL_RUN_LAST,
                                                0,
                                                NULL,
                                                NULL,
                                                g_cclosure_marshal_VOID__VOID,
                                                G_TYPE_NONE,
                                                0);

  G_OBJECT_CLASS(klass)->dispose = playlist_dispose;
  G_OBJECT_CLASS(klass)->finalize = playlist_finalize;
}

Playlist*
playlist_new(gchar* name, gchar* path, PlaylistTypes type)
{
  Playlist* playlist = g_object_new(PLAYLIST_TYPE, NULL);
  playlist->name = g_strdup(name);
  playlist->path = g_strdup(path);
  playlist->description = g_strdup("No description");
  playlist->tracks = g_ptr_array_new();
  playlist->type = type;
  playlist->startLine = G_MAXUINT;
  playlist->endLine = G_MAXUINT;
  playlist->wasEverSelected = false;

  return playlist;
}

static void
playlist_notify_change(Playlist* playlist)
{
  g_signal_emit(playlist, playlist_signals[INFO_CHANGED], 0);
}

void
playlist_populate(Playlist* playlist, GPtrArray* tracks)
{
  g_ptr_array_extend(playlist->tracks, tracks, NULL, NULL);
}

guint
playlist_get_length(Playlist* playlist)
{
  return playlist->tracks->len;
}

void
playlist_add(Playlist* playlist, Track* track)
{
  if (playlist->tracks->len == 0 && playlist->type == PLAYLIST_NEW) {
    // The playlist is no longer considered new after adding
    // first track, and another new playlist can be created
    playlist->type = PLAYLIST_NONE;
  }
  g_ptr_array_add(playlist->tracks, track);
  track->index = playlist->tracks->len - 1;
}

Track*
playlist_get_track(Playlist* playlist, guint index)
{
  if (index >= playlist->tracks->len)
    return NULL;
  return g_ptr_array_index(playlist->tracks, index);
}

void
playlist_rename(Playlist* playlist, const gchar* name)
{
  if (playlist->name != NULL) {
    g_free(playlist->name);
  }
  playlist->name = g_strdup(name);
  playlist_notify_change(playlist);
}

Track*
playlist_remove_track_by_index(Playlist* playlist, guint index)
{
  if (index >= playlist->tracks->len)
    return NULL;
  Track* track = g_ptr_array_remove_index(playlist->tracks, index);
  while (index < playlist->tracks->len) {
    Track* next = g_ptr_array_index(playlist->tracks, index);
    next->index = ++index - 1; // Shift indices of tracks after removing one
  }
  return track;
}

bool
playlist_is_folder(Playlist* playlist)
{
  return playlist->type == PLAYLIST_FOLDER;
}

bool
playlist_is_new(Playlist* playlist)
{
  return playlist->type == PLAYLIST_NEW;
}

Track*
playlist_get_next_track(Playlist* playlist, guint index)
{
  index = (index == playlist_get_length(playlist) - 1) ? 0 : index + 1;

  return g_ptr_array_index(playlist->tracks, index);
}

Track*
playlist_get_prev_track(Playlist* playlist, guint index)
{
  index = (index == 0) ? 0 : index - 1;

  return g_ptr_array_index(playlist->tracks, index);
}

void
playlist_reverse(Playlist* playlist)
{
  if (playlist->tracks->len == 0)
    return;
  guint start = 0;
  guint end = playlist->tracks->len - 1;

  while (start < end) {
    gpointer temp = g_ptr_array_index(playlist->tracks, start);
    g_ptr_array_index(playlist->tracks, start) =
      g_ptr_array_index(playlist->tracks, end);
    g_ptr_array_index(playlist->tracks, end) = temp;

    start++;
    end--;
  }
}

void
playlist_update_indices(Playlist* playlist)
{
  for (int i = 0; i < playlist->tracks->len; i++) {
    Track* track = g_ptr_array_index(playlist->tracks, i);
    track->index = i;
  }
}

GPtrArray*
playlist_get_tracks(Playlist* playlist)
{
  return playlist->tracks;
}

bool
playlist_is_empty(Playlist* playlist)
{
  return playlist->tracks->len == 0;
}

void
parse_playlists(GPtrArray* playlists, gchar* filename)
{
  GError* error = NULL;
  GFile* file = g_file_new_for_path(filename);
  GFileIOStream* stream = g_file_open_readwrite(file, NULL, &error);

  if (error != NULL) {
    g_printerr("ERROR: parse_playlists(): %s\n", error->message);
    g_clear_error(&error);
    return;
  }

  GDataInputStream* instream = g_data_input_stream_new(
    g_io_stream_get_input_stream(&stream->parent_instance));
  gchar* line = NULL;
  gsize length;
  guint startIndex, endIndex, lineIndex, startLine;
  lineIndex = 0;
  bool name = false;
  GPtrArray* tracks = NULL;
  Playlist* playlist = NULL;

  while ((line = g_data_input_stream_read_line(
            instream, &length, NULL, &error)) != NULL) {
    endIndex = 0;
    while (endIndex < length) {
      switch (line[endIndex]) {
        case ':': {
          if (name) {
            // When reading first line of playlist (playlist name), like
            // ":MyPlaylist:", playlist instance is created
            if (playlist == NULL) {
              // Substring is being extracted at [start;end), so we need to
              // increment startIndex to get the name without colon symbol
              gchar* playlistName =
                g_utf8_substring(line, startIndex + 1, endIndex);
              playlist = playlist_new(playlistName, filename, false);
              g_free(playlistName);
              name = false;
              startLine = lineIndex;
              break;
            }

            // When reading last line of playlist (note/info about playlist),
            // like
            // ":This is MyPlaylist:" playlist instance exists and being
            // populated with fetched tracks
            else {
              playlist_populate(playlist, tracks);
              playlist_set_lines_in_conf(playlist, startLine, lineIndex);
              g_ptr_array_add(playlists, playlist);
              g_ptr_array_free(tracks, TRUE);

              gchar* playlistDescription =
                g_utf8_substring(line, startIndex + 1, endIndex);
              playlist_set_description(playlist, playlistDescription);
              g_free(playlistDescription);

              playlist = NULL;
              tracks = NULL;
              name = false;
              endIndex = length;
              break;
            }
          } else {

            // Parsing playlist name/note start here
            startIndex = endIndex;
            name = true;

            // Tracks could be NULL only at first line of the playlist, but this
            // block could be executed also at the last line of playlist, so we
            // need NULL check to avoid potential memleak
            if (tracks == NULL)
              tracks = g_ptr_array_new();
            break;
          }
          break;
        }
        default: {

          // Name could be FALSE only when reading start of playlist name/note
          // AND when reading ordinary lines (playlist null check was added for
          // avoiding parsing comment lines)
          if (!name && playlist != NULL) {
            if (file_exists(line)) {
              Track* track = fetch_track(line);
              if (tracks != NULL) {
                g_ptr_array_add(tracks, track);
                track->index = tracks->len - 1;
              }
            } else {
              g_printerr(
                "ERROR: parse_playlists(): No such file at line %d: %s\n",
                lineIndex + 1,
                line);
            }

            // Skip cycle if not reading playlist name or info
            endIndex = length;
          }
          break;
        }
      }

      endIndex++;
    }
    g_free(line);
    lineIndex++;
  }

  // Assuming that EOF reached and there was no end of playlist indicator
  if (playlist != NULL) {
    g_printerr("ERROR: parse_playlists(): EOF reached without playlist end\n");
    g_object_unref(playlist);
    if (tracks != NULL) {
      g_ptr_array_free(tracks, TRUE);
    }
  }
}

const gchar*
playlist_get_name(Playlist* playlist)
{
  return playlist->name;
}

void
playlist_set_lines_in_conf(Playlist* playlist, guint start, guint end)
{
  if (start != G_MAXUINT) {
    playlist->startLine = start;
  }
  if (end != G_MAXUINT) {
    playlist->endLine = end;
  }
}

static GPtrArray*
playlist_make_lines(Playlist* playlist)
{
  GPtrArray* lines = g_ptr_array_new();
  char* line = g_new(char, strlen(playlist->name) + 4);
  g_snprintf(line, strlen(playlist->name) + 4, ":%s:\n", playlist->name);

  g_ptr_array_add(lines, line);

  for (int i = 0; i < playlist->tracks->len; i++) {
    g_ptr_array_add(
      lines,
      g_strconcat(
        ((Track*)g_ptr_array_index(playlist->tracks, i))->path, "\n", NULL));
  }
  line = g_new(char, strlen(playlist->description) + 4);
  g_snprintf(
    line, strlen(playlist->description) + 4, ":%s:\n", playlist->description);
  g_ptr_array_add(lines, line);

  return lines;
}

static void
playlist_append_to_conf(Playlist* playlist)
{
  FILE* file = fopen(playlist->path, "a+");
  GPtrArray* lines = playlist_make_lines(playlist);
  char buffer[1024];
  int lineIndex = -1;
  while (fgets(buffer, sizeof(buffer), file) != NULL) {
    lineIndex++;
  }
  playlist->startLine = ++lineIndex;
  playlist->endLine = lineIndex + lines->len;
  for (guint i = 0; i < lines->len; i++) {
    fputs(g_ptr_array_index(lines, i), file);
  }

  fclose(file);
  g_ptr_array_free(lines, TRUE);
}

int
playlist_save(Playlist* playlist)
{
  if (playlist->type == PLAYLIST_NONE) {
    if (playlist->startLine == G_MAXUINT) {
      playlist_append_to_conf(playlist);
      return 0;
    }

    FILE* tmp = tmpfile();
    if (!tmp) {
      g_printerr("ERROR: parse_playlists(): Failed to open tmpfile\n");
      return 0;
    }
    FILE* file = fopen(playlist->path, "r");
    GPtrArray* lines = playlist_make_lines(playlist);

    char buffer[1024];
    int lineIndex = -1;
    if (file) {
      while (fgets(buffer, sizeof(buffer), file) != NULL) {
        lineIndex++;
        if (lineIndex == playlist->startLine) {
          for (guint i = 0; i < lines->len; i++) {
            fputs(g_ptr_array_index(lines, i), tmp);
          }
        }
        if (lineIndex >= playlist->startLine &&
            lineIndex <= playlist->endLine) {
          continue;
        }
        fputs(buffer, tmp);
      }
    } else {
      for (guint i = 0; i < lines->len; i++) {
        fputs(g_ptr_array_index(lines, i), tmp);
      }
    }

    rewind(tmp);
    file =
      file ? freopen(playlist->path, "w", file) : fopen(playlist->path, "w");

    while (fgets(buffer, sizeof(buffer), tmp) != NULL) {
      fputs(buffer, file);
    }

    int offset = lines->len - (playlist->endLine - playlist->startLine) - 1;
    playlist->endLine += offset;

    fclose(tmp);
    fclose(file);
    g_ptr_array_free(lines, TRUE);
    return offset;
  }
  return 0;
}

int
playlist_delete(Playlist* playlist)
{
  if (playlist->type == PLAYLIST_NONE) {
    FILE* tmp = tmpfile();
    if (!tmp) {
      g_printerr("ERROR: parse_playlists(): Failed to open tmpfile\n");
      return 0;
    }

    FILE* file = fopen(playlist->path, "r");
    if (!file)
      return 0;

    char buffer[1024];
    int lineIndex = -1;

    while (fgets(buffer, sizeof(buffer), file) != NULL) {
      lineIndex++;
      if (lineIndex >= playlist->startLine && lineIndex <= playlist->endLine) {
        continue;
      }
      fputs(buffer, tmp);
    }

    rewind(tmp);
    file = freopen(playlist->path, "w", file);
    while (fgets(buffer, sizeof(buffer), tmp) != NULL) {
      fputs(buffer, file);
    }

    fclose(tmp);
    fclose(file);
    return -(playlist->tracks->len + 2);
  }
  return 0;
}

Playlist*
playlist_duplicate(Playlist* playlist)
{
  Playlist* copy = g_object_new(PLAYLIST_TYPE, NULL);
  copy->path = g_strdup(playlist->path);
  copy->startLine = G_MAXUINT;
  copy->endLine = G_MAXUINT;
  copy->name = g_strconcat(g_strdup(playlist->name), " (copy)", NULL);
  copy->description = g_strdup(playlist->description);
  copy->type = PLAYLIST_NONE;
  copy->tracks = g_ptr_array_new();
  g_ptr_array_set_size(copy->tracks, playlist->tracks->len);
  for (int i = 0; i < copy->tracks->len; i++) {
    Track* track = g_ptr_array_index(playlist->tracks, i);
    track_ref(track);
    g_ptr_array_index(copy->tracks, i) = track;
  }

  return copy;
}

void
playlist_offset_lines(Playlist* playlist, int offset)
{
  playlist->startLine += offset;
  playlist->endLine += offset;
}

const gchar*
playlist_get_description(Playlist* playlist)
{
  return playlist->description;
}

void
playlist_set_description(Playlist* playlist, const gchar* str)
{
  if (playlist->description != NULL) {
    g_free(playlist->description);
  }
  playlist->description = g_strdup(str);
  playlist_notify_change(playlist);
}