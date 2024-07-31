#pragma once
#include <vlc/vlc.h>

void
music_finished(const libvlc_event_t* event, void* user_data);
void
music_time_changed(const libvlc_event_t* event, void* user_data);
void
media_player_length_changed(const libvlc_event_t* event, void* user_data);