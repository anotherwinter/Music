#include "track.h"
#include "glib.h"
#include <stdio.h>
#include <taglib/tag_c.h>

Track*
fetch_track(gchar* path)
{
  TagLib_File* file = taglib_file_new(path);
  if (file == NULL)
    return NULL;
  TagLib_Tag* tag = taglib_file_tag(file);

  if (tag == NULL)
    g_print("ERROR: fetch_track(): Tag is null\n");

  Track* track = malloc(sizeof(Track));
  track->path = g_strdup(path);
  track->title = taglib_tag_title(tag);
  track->artist = taglib_tag_artist(tag);
  track->refCount = 1;

  if (g_strcmp0(track->title, "") == 0) {
    track->title = g_path_get_basename(path);
  }
  if (g_strcmp0(track->artist, "") == 0) {
    track->artist = "Unknown artist";
  }

  taglib_file_free(file);
  return track;
}

void
track_unref(Track* track)
{
  track->refCount--;
  if (track->refCount == 0) {
    // Title or path are always not NULL
    g_free(track->title);
    g_free(track->path);

    // If other fields contain static literal placeholders, do not free them
    if (g_strcmp0(track->artist, "Unknown artist")) {
      g_free(track->artist);
    }

    // We should free all memory allocated to struct
    g_free(track);
  }
}

void track_ref(Track* track) {
  track->refCount++;
}