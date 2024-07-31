#pragma once

typedef enum AudioState
{
  AUDIO_STOPPED = 0,
  AUDIO_PLAYING = 1,
  AUDIO_PAUSED = 2
} AudioState;

typedef enum
{
  PLAYBACK_NONE = 0,
  PLAYBACK_LOOP = 1,
  PLAYBACK_ONETRACK = 2,
  PLAYBACK_SHUFFLE = 4
} PlaybackOptions;

typedef enum
{
  PLAYLIST_NONE = 0,
  PLAYLIST_NEW = 1,
  PLAYLIST_FOLDER = 2
} PlaylistTypes;

typedef enum
{
  BUTTON_PLAY = 0,
  BUTTON_SHUFFLE = 1,
  BUTTON_LOOP = 2
} ButtonTypes;