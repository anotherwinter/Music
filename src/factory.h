#pragma once
#include "musicapp.h"

// Create and set factory for drawing selected widget in playlists dropdown
GtkListItemFactory*
music_app_setup_list_factory(MusicApp* app);
