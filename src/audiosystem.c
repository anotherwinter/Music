#include "audiosystem.h"
#include "enum_types.h"
#include "filelister.h"
#include <vlc/libvlc_events.h>

struct AudioSystem
{
  libvlc_instance_t* vlc_instance;
  libvlc_media_player_t* player;
  libvlc_media_t* media;
  AudioState state;
  bool manual;
};

static AudioSystem* instance;

int
audio_system_init()
{

  instance = malloc(sizeof(AudioSystem));
  if (instance == NULL) {
    printf("ERROR: audio_system_init(): Failed to allocate memory for "
           "audiosystem\n");
    return -1;
  }
  const char* vlc_args[] = { "--no-video" };
  instance->vlc_instance = libvlc_new(1, vlc_args);
  instance->player = libvlc_media_player_new(instance->vlc_instance);
  instance->state = AUDIO_STOPPED;
  instance->manual = false;

  return 0;
}

void
audio_system_free()
{
  audio_system_stop_audio();
  libvlc_media_player_release(instance->player);
  libvlc_release(instance->vlc_instance);
  free(instance);
}

AudioSystem*
audio_system_get_instance()
{
  return instance;
}

int
audio_system_open_audio(const char* path)
{
  if (!file_exists(path)) {
    printf("ERROR: audio_system_open_audio(): File '%s' doesn't exist\n", path);
    return -1;
  }
  instance->media = libvlc_media_new_path(instance->vlc_instance, path);
  if (!instance->media) {
    printf("ERROR: audio_system_open_audio(): Failed to open music for '%s'\n",
           path);
    return -1;
  }
  libvlc_media_player_set_media(instance->player, instance->media);

  return 0;
}

void
audio_system_play_audio()
{
  if (libvlc_media_player_play(instance->player) == -1) {
    printf("ERROR: audio_system_play_audio(): Failed to play music\n");
  } else {
    instance->state = AUDIO_PLAYING;
  }
}

void
audio_system_stop_audio()
{
  instance->state = AUDIO_STOPPED;
  libvlc_media_t* media = libvlc_media_player_get_media(instance->player);
  if (media) {
    if (libvlc_media_player_is_playing(instance->player))
      libvlc_media_player_stop(instance->player);
    libvlc_media_release(media);
  }
}

void
audio_system_pause_audio()
{
  if (instance->media) {
    libvlc_media_player_pause(instance->player);
    instance->state = AUDIO_PAUSED;
  }
}

void
audio_system_resume_audio()
{
  if (!libvlc_media_player_play(instance->player)) {
    instance->state = AUDIO_PLAYING;
  }
}

int
audio_system_get_state()
{
  return instance->state;
}

void
audio_system_set_volume(double value)
{
  libvlc_audio_set_volume(instance->player, 100 * value);
}

void
audio_system_set_finished_callback(void* music_finished, void* user_data)
{
  libvlc_event_attach(libvlc_media_player_event_manager(instance->player),
                      libvlc_MediaPlayerEndReached,
                      music_finished,
                      user_data);
}

void
audio_system_set_time_changed_callback(void* time_changed, void* user_data)
{
  libvlc_event_attach(libvlc_media_player_event_manager(instance->player),
                      libvlc_MediaPlayerTimeChanged,
                      time_changed,
                      user_data);
}

void
audio_system_set_length_changed_callback(void* length_changed, void* user_data)
{
  libvlc_event_attach(libvlc_media_player_event_manager(instance->player),
                      libvlc_MediaPlayerLengthChanged,
                      length_changed,
                      user_data);
}

float
audio_system_get_audio_position()
{
  return libvlc_media_player_get_position(instance->player);
}

void
audio_system_set_audio_position(float position)
{
  libvlc_media_player_set_position(instance->player, position);
}

void
audio_system_restart_media()
{
  if (instance->media) {
    const char* prefix = "file://";
    size_t prefix_len = strlen(prefix);
    char* str = strdup(libvlc_media_get_mrl(instance->media) + prefix_len);
    libvlc_media_t* media = libvlc_media_new_path(instance->vlc_instance, str);
    if (media == NULL) {
    }
    libvlc_media_player_set_media(instance->player, media);
    libvlc_media_release(instance->media);
    instance->media = media;
  }
}

void
audio_system_set_is_manual_position(bool manual)
{
  instance->manual = manual;
}

bool
audio_system_is_manual_position()
{
  return instance->manual;
}

long int
audio_system_get_time()
{
  return libvlc_media_player_get_time(instance->player);
}

long int
audio_system_get_length()
{
  return libvlc_media_player_get_length(instance->player);
}