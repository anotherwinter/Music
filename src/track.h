#pragma once
#include "glib.h"
#include <taglib/tag_c.h>

typedef struct Track
{
  gchar* path;
  TagLib_Tag* tag;
  gchar* title;
  gchar* artist;
  guint index;
  guint lineIndex;
  guint refCount;
} Track;

// Fetching track from path and setting index of line in playlists file if
// needed, can be set to -1 if fetched for new playlist or from folder
Track*
fetch_track(gchar* path);
void
track_free(Track* track);
void
track_ref(Track* track);