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
  guint refCount;
} Track;

// Fetch track from path
Track*
fetch_track(gchar* path);
void
track_unref(Track* track);
void
track_ref(Track* track);