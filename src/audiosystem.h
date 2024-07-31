#pragma once
#include <vlc/vlc.h>

typedef struct AudioSystem AudioSystem;

int
audio_system_init();
void
audio_system_free();
AudioSystem*
audio_system_get_instance();
int
audio_system_open_audio(const char* path);
void
audio_system_play_audio();
void
audio_system_stop_audio();
void
audio_system_pause_audio();
void
audio_system_resume_audio();
int
audio_system_get_state();
void
audio_system_set_volume(double value);
void
audio_system_set_finished_callback(void* music_finished, void* user_data);
void
audio_system_set_time_changed_callback(void* time_changed, void* user_data);
void
audio_system_set_length_changed_callback(void* length_changed, void* user_data);
float
audio_system_get_audio_position();
void
audio_system_set_audio_position(float position);
void
audio_system_restart_media();
void
audio_system_set_is_manual_position(bool manual);
bool
audio_system_is_manual_position();
long int
audio_system_get_time();
long int
audio_system_get_length();