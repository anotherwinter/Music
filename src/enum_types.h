#pragma once

typedef enum AudioState
{
  AUDIO_STOPPED = 0,
  AUDIO_PLAYING = 1,
  AUDIO_PAUSED = 2
} AudioState;

typedef enum
{
  TRACK_INACTIVE = 0,
  TRACK_ACTIVE = 1,
  TRACK_PLAYING = 2,
  TRACK_PAUSED = 3
} TrackWidgetState;

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