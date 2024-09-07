#include "trackwidget.h"
#include "callbacks_ui.h"
#include "enum_types.h"
#include "track.h"
#include <gtk/gtk.h>

struct _TrackWidget
{
  GtkBox parent_instance;
  GtkButton* playButton;

  int index;
  Track* track;
};

G_DEFINE_TYPE(TrackWidget, track_widget, GTK_TYPE_BOX);

void
track_widget_init(TrackWidget* widget)
{
  widget->index = 0;
  gtk_orientable_set_orientation(GTK_ORIENTABLE(widget),
                                 GTK_ORIENTATION_HORIZONTAL);
  gtk_box_set_spacing(GTK_BOX(widget), 5);
  widget->playButton =
    GTK_BUTTON(gtk_button_new_from_icon_name("media-playback-start"));
  gtk_box_append(GTK_BOX(widget), GTK_WIDGET(widget->playButton));
  gtk_widget_set_margin_start(GTK_WIDGET(widget->playButton), 5);
  gtk_widget_set_margin_end(GTK_WIDGET(widget->playButton), 5);
  gtk_widget_add_css_class(GTK_WIDGET(widget), "trackWidget");
}

static void
track_widget_dispose(GObject* object)
{
  TrackWidget* widget = APP_TRACK_WIDGET(object);

  if (GTK_IS_BUTTON(widget->playButton)) {
    // parents hold a reference for child widgets, so
    // to unref gtk_widget_unparent is called
    // (referencecount for that one gets to 0 then and
    // memory then released automatically), NEVER CALL
    // g_object_unref() for unreffing childs
    gtk_widget_unparent(GTK_WIDGET(widget->playButton));
    widget->playButton = NULL;
  }
  widget->track = NULL;
  gtk_widget_unparent(GTK_WIDGET(object));
  G_OBJECT_CLASS(track_widget_parent_class)->dispose(object);
}

static void
track_widget_finalize(GObject* object)
{
  G_OBJECT_CLASS(track_widget_parent_class)->finalize(object);
}

void
track_widget_class_init(TrackWidgetClass* klass)
{
  G_OBJECT_CLASS(klass)->dispose = track_widget_dispose;
  G_OBJECT_CLASS(klass)->finalize = track_widget_finalize;
}

void
track_widget_set_icon(TrackWidget* widget, AudioState state)
{
  switch (state) {
    case AUDIO_STOPPED: {
      gtk_button_set_icon_name(widget->playButton, "media-playback-start");
      gtk_widget_remove_css_class(GTK_WIDGET(widget), "trackWidget-selected");
      break;
    }
    case AUDIO_PLAYING: {
      gtk_button_set_icon_name(widget->playButton, "media-playback-pause");
      gtk_widget_add_css_class(GTK_WIDGET(widget), "trackWidget-selected");
      break;
    }
    case AUDIO_PAUSED: {
      gtk_button_set_icon_name(widget->playButton, "media-playback-start");
      break;
    }
  }
}

Track*
track_widget_get_track(TrackWidget* widget)
{
  return widget->track;
}

void
track_widget_set_track(TrackWidget* widget, Track* track)
{
  widget->track = track;
}

TrackWidget*
track_widget_new(MusicApp* app, Track* track)
{
  TrackWidget* widget = g_object_new(TRACK_WIDGET_TYPE, NULL);

  GtkLabel* trackLabel = GTK_LABEL(gtk_label_new(""));
  GString* str = g_string_new("");
  g_string_append(str, track->title);
  if (track->artist != NULL) {
    g_string_append(str, "\n");
    g_string_append(str, track->artist);
  }
  gtk_label_set_text(trackLabel, str->str);
  gtk_widget_set_margin_top(GTK_WIDGET(trackLabel), 3);
  gtk_widget_set_margin_bottom(GTK_WIDGET(trackLabel), 3);

  // margin is playButton width + its start and end margins, to keep label in
  // middle
  gtk_widget_set_margin_end(GTK_WIDGET(trackLabel), 34);
  gtk_label_set_justify(trackLabel, GTK_JUSTIFY_CENTER);
  gtk_widget_set_hexpand(GTK_WIDGET(trackLabel), TRUE);
  gtk_box_insert_child_after(
    GTK_BOX(widget), GTK_WIDGET(trackLabel), GTK_WIDGET(widget->playButton));
  g_string_free(str, true);

  widget->track = track;

  g_signal_connect(widget->playButton,
                   "clicked",
                   G_CALLBACK(playtrackButton_clicked),
                   (gpointer)app);

  GtkGesture* gesture = gtk_gesture_click_new();

  gtk_gesture_single_set_button(GTK_GESTURE_SINGLE(gesture), 0);
  g_signal_connect(gesture, "pressed", G_CALLBACK(trackWidget_clicked), app);

  // no need to store gesture pointer to free it manually,
  // gesture gets destroyed automatically after its widget got
  // destroyed
  gtk_widget_add_controller(GTK_WIDGET(widget), GTK_EVENT_CONTROLLER(gesture));

  return widget;
}

int
track_widget_get_index(TrackWidget* widget)
{
  return widget->track->index;
}

void
track_widget_update_index(TrackWidget* widget)
{
  widget->index = widget->track->index;
}