#pragma once
#include <gtk/gtk.h>
#include <vlc/vlc.h>

void
openFolder_clicked(GtkButton* self, gpointer user_data);
void
playtrackButton_clicked(GtkButton* self, gpointer user_data);
void
music_finished(const libvlc_event_t* event, void* user_data);
void
shuffleButton_clicked(GtkButton* self, gpointer user_data);
void
loopButton_clicked(GtkButton* self, gpointer user_data);
void
volumeScale_value_changed(GtkRange* self, gpointer user_data);
void
prevButton_clicked(GtkButton* self, gpointer user_data);
void
nextButton_clicked(GtkButton* self, gpointer user_data);
void
playButton_clicked(GtkButton* self, gpointer user_data);
void
trackWidget_clicked(GtkGestureClick* self,
                    gint n_press,
                    gdouble x,
                    gdouble y,
                    gpointer user_data);
void
createPlaylist_clicked(GtkButton* self, gpointer user_data);
void
selection_changed(GtkDropDown* dropdown, GParamSpec* pspec, gpointer user_data);
void
playlist_clicked(GtkGestureClick* self,
                 gint n_press,
                 gdouble x,
                 gdouble y,
                 gpointer user_data);
void
on_text_field_dialog_response(GtkButton* self, gpointer user_data);
void
openFiles_cb(GtkFileDialog* source, GAsyncResult* res, gpointer user_data);
void
music_time_changed(const libvlc_event_t* event, void* user_data);
void
audioPositionScale_value_changed(GtkRange* self, gpointer user_data);
void
audioPositionScale_pressed(GtkGestureClick* self,
                           gint n_press,
                           gdouble x,
                           gdouble y,
                           gpointer user_data);
void
audioPositionScale_released(GtkGestureClick* self,
                            gint n_press,
                            gdouble x,
                            gdouble y,
                            gpointer user_data);
                            void
media_player_length_changed(const libvlc_event_t* event, void* user_data);
