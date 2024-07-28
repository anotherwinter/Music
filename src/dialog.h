#pragma once
#include "musicapp.h"

typedef struct
{
  gpointer dialog;
  gpointer user_data1;
  gpointer user_data2;
} DialogData;

void
dialog_create_text_input_for_app(MusicApp* app, gpointer user_data);

void dialog_create_file_dialog(MusicApp* app, gpointer user_data);