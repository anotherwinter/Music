#include "dialog.h"
#include "handlers.h"

GtkWindow*
dialog_create_text_input_for_app(MusicApp* app,
                                 gpointer user_data,
                                 const char* title)
{
  GtkBuilder* builder = gtk_builder_new_from_resource("/org/aw/Music/main.ui");
  GtkWindow* dialog =
    GTK_WINDOW(gtk_builder_get_object(builder, "textInputDialog"));
  GtkWidget* entry = GTK_WIDGET(gtk_builder_get_object(builder, "dialogEntry"));
  GtkWidget* ok_button =
    GTK_WIDGET(gtk_builder_get_object(builder, "okButton"));
  GtkWidget* cancel_button =
    GTK_WIDGET(gtk_builder_get_object(builder, "cancelButton"));

  gtk_window_set_modal(dialog, TRUE);
  gtk_window_set_transient_for(dialog, music_app_get_main_window(app));
  if (title != NULL) {
    gtk_window_set_title(dialog, title);
  }

  gtk_entry_buffer_set_max_length(gtk_entry_get_buffer(GTK_ENTRY(entry)), 64);

  DialogData* data = NULL;
  data = g_new(DialogData, 1);
  data->app = app;
  data->dialog = dialog;
  data->user_data1 = entry;
  if (user_data != NULL) {
    data->user_data2 = user_data;
  }

  g_signal_connect(
    ok_button, "clicked", G_CALLBACK(on_text_field_dialog_response), data);
  g_signal_connect_swapped(cancel_button,
                           "clicked",
                           G_CALLBACK(gtk_window_destroy),
                           GTK_WINDOW(dialog));

  return dialog;
}

void
dialog_create_file_dialog(MusicApp* app, gpointer user_data)
{
  GtkFileDialog* dialog = gtk_file_dialog_new();
  gtk_file_dialog_set_modal(dialog, TRUE);
  GtkFileFilter* filter = gtk_file_filter_new();
  gtk_file_filter_set_name(filter, "Audio files");
  gtk_file_filter_add_pattern(filter, "*.mp3");
  gtk_file_filter_add_pattern(filter, "*.ogg");
  gtk_file_filter_add_pattern(filter, "*.flac");
  gtk_file_filter_add_pattern(filter, "*.wav");
  gtk_file_filter_add_pattern(filter, "*.m4a");
  GListStore* liststore = g_list_store_new(GTK_TYPE_FILE_FILTER);
  g_list_store_append(liststore, filter);
  gtk_file_dialog_set_filters(dialog, G_LIST_MODEL(liststore));

  g_object_unref(liststore);
  g_object_unref(filter);

  gtk_file_dialog_open_multiple(dialog,
                                music_app_get_main_window(app),
                                NULL,
                                (GAsyncReadyCallback)openFiles_cb,
                                user_data);
}