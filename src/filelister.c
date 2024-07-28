#include "filelister.h"
#include <dirent.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#define PATH_SIZE 1024

static int
is_audio_file(const char* filename)
{
  const char* ext = strrchr(filename, '.');
  if (!ext) {
    return 0;
  }
  return !strcasecmp(ext, ".mp3") || !strcasecmp(ext, ".ogg") ||
         !strcasecmp(ext, ".flac") || !strcasecmp(ext, ".wav") ||
         !strcasecmp(ext, ".m4a");
}

GPtrArray*
list_audio_files(const char* dirname)
{
  GPtrArray* audiofiles;
  DIR* dir;
  struct dirent* entry;
  char path[PATH_SIZE];

  audiofiles = g_ptr_array_new();
  dir = opendir(dirname);
  while ((entry = readdir(dir)) != NULL) {
    if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
      continue;
    }

    if (is_audio_file(entry->d_name)) {
      snprintf(path, PATH_SIZE, "%s/%s", dirname, entry->d_name);
      g_ptr_array_add(audiofiles, g_strdup(path));
    }
  }

  closedir(dir);
  return audiofiles;
}

int
file_exists(const char* filename)
{
  return access(filename, F_OK) != -1;
}