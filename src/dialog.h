#pragma once
#include "musicapp.h"

typedef struct
{
  gpointer app;
  gpointer dialog;
  gpointer user_data1;
  gpointer user_data2;
} DialogData;

GtkWindow*
dialog_create_text_input_for_app(MusicApp* app, gpointer user_data, const char* title);

void dialog_create_file_dialog(MusicApp* app, gpointer user_data);