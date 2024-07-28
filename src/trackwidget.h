#pragma once
#include "musicapp.h"

#define TRACK_WIDGET_TYPE (track_widget_get_type())
G_DECLARE_FINAL_TYPE(TrackWidget, track_widget, APP, TRACK_WIDGET, GtkBox);

typedef struct _TrackWidget TrackWidget;

TrackWidget*
track_widget_new(MusicApp* app, Track* track);
TrackWidget*
track_widget_configure(TrackWidget* widget, MusicApp* app, Track* track);
int
track_widget_get_state(TrackWidget* widget);
void
track_widget_set_state(TrackWidget* widget, TrackWidgetState state);
Track*
track_widget_get_track(TrackWidget* widget);
void
track_widget_set_track(TrackWidget* widget, Track* track);
int
track_widget_get_index(TrackWidget* widget);
void
track_widget_update_index(TrackWidget* widget);