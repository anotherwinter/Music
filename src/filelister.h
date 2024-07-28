#pragma once
#include <glib.h>

GPtrArray*
list_audio_files(const char* dirname);
int
file_exists(const char* filename);