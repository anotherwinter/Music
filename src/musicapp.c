#include "musicapp.h"
#include "audiosystem.h"
#include "callbacks_playback.h"
#include "callbacks_ui.h"
#include "contextmenu.h"
#include "enum_types.h"
#include "factory.h"
#include "playlist.h"
#include "resources.h"
#include "track.h"
#include "trackwidget.h"

struct _MusicApp
{
  GtkApplication parent;
  GtkWindow* win;
  GtkScrolledWindow* tracksScrolledWindow;
  GtkBox* appBox;
  GtkBox* upperBox;
  GtkBox* lowerBox;
  GtkBox* playlistBox;
  GtkBox* trackButtonsBox;
  GtkBox* tracksBox;
  GtkDropDown* playlistDD;
  GListStore* playlistLS;
  GtkLabel* trackLabel;
  GtkButton* createPlaylistButton;
  GtkButton* openFolderButton;
  GtkButton* prevButton;
  GtkButton* playButton;
  GtkButton* nextButton;
  GtkButton* shuffleButton;
  GtkButton* loopButton;
  GtkScale* volumeScale;
  GtkScale* audioPositionScale;
  GtkLabel* audioPositionLabel;
  GtkLabel* audioLengthLabel;
  GtkBox* audioPositionBox;
  GtkWidget* playImage;
  GtkWidget* nextImage;
  GtkWidget* prevImage;
  GtkWidget* pauseImage;
  GtkWidget* shuffleImage;
  GtkWidget* shuffleActiveImage;
  GtkWidget* loopImage;
  GtkWidget* onetrackActiveImage;
  GtkWidget* loopActiveImage;
  GtkPopover* dropDownPopover;

  GtkListItemFactory* factory;
  GPtrArray* trackWidgets;
  Playlist* active;
  Track* current;
  PlaybackOptions options;
};

G_DEFINE_TYPE(MusicApp, music_app, GTK_TYPE_APPLICATION)

static void
init_systems()
{
  srand(time(NULL));
  if (audio_system_init() == -1) {
    g_printerr("ERROR: Failed to initialize audiosystem.");
    exit(1);
  }
}

static void
music_app_dispose(GApplication* object)
{
  MusicApp* app = MUSIC_APP(object);

  if (app->trackWidgets) {
    g_ptr_array_free(app->trackWidgets, TRUE);
    app->trackWidgets = NULL;
  }

  GListModel* list = G_LIST_MODEL(app->playlistLS);
  while (g_list_model_get_n_items(list) > 0) {
    guint index = g_list_model_get_n_items(list) - 1;
    Playlist* playlist = g_list_model_get_item(list, index);
    g_object_unref(playlist);
    g_list_store_remove(app->playlistLS, index);
  }

  g_object_unref(app->playlistLS);
  g_object_unref(app->factory);
  app->playlistLS = NULL;
  app->factory = NULL;
  app->active = NULL;
  app->current = NULL;

  context_menu_free();

  G_APPLICATION_CLASS(music_app_parent_class)->shutdown(object);
}

static void
music_app_init(MusicApp* app)
{
  init_systems();
}

static void
fetch_playlists(MusicApp* app, gchar* path)
{
  GPtrArray* playlists = g_ptr_array_new();
  parse_playlists(playlists, "playlists");
  if (playlists->len > 0) {
    for (guint i = 0; i < playlists->len; i++) {
      g_list_store_append(app->playlistLS, g_ptr_array_index(playlists, i));
      g_object_unref(g_ptr_array_index(playlists, i));
    }
  }
  g_ptr_array_free(playlists, TRUE);
}

static void
setup_factories(MusicApp* app)
{
  app->factory = music_app_setup_list_factory(app);
  gtk_drop_down_set_factory(app->playlistDD, app->factory);

  g_signal_connect(
    app->playlistDD, "notify::selected", G_CALLBACK(selection_changed), app);
}

static void
set_icons(MusicApp* app)
{
  app->playImage =
    gtk_image_new_from_resource("/org/aw/Music/icons/media-playback-start.svg");
  gtk_image_set_pixel_size(GTK_IMAGE(app->playImage), 32);
  g_object_ref(app->playImage);

  app->nextImage =
    gtk_image_new_from_resource("/org/aw/Music/icons/media-skip-forward.svg");
  gtk_image_set_pixel_size(GTK_IMAGE(app->nextImage), 32);
  g_object_ref(app->nextImage);

  app->prevImage =
    gtk_image_new_from_resource("/org/aw/Music/icons/media-skip-backward.svg");
  gtk_image_set_pixel_size(GTK_IMAGE(app->prevImage), 32);
  g_object_ref(app->prevImage);

  app->pauseImage =
    gtk_image_new_from_resource("/org/aw/Music/icons/media-playback-pause.svg");
  gtk_image_set_pixel_size(GTK_IMAGE(app->pauseImage), 32);
  g_object_ref(app->pauseImage);

  app->shuffleImage = gtk_image_new_from_resource(
    "/org/aw/Music/icons/media-playlist-shuffle.svg");
  gtk_image_set_pixel_size(GTK_IMAGE(app->shuffleImage), 24);
  g_object_ref(app->shuffleImage);

  app->shuffleActiveImage = gtk_image_new_from_resource(
    "/org/aw/Music/icons/media-playlist-shuffle-active.svg");
  gtk_image_set_pixel_size(GTK_IMAGE(app->shuffleActiveImage), 24);
  g_object_ref(app->shuffleActiveImage);

  app->loopImage = gtk_image_new_from_resource(
    "/org/aw/Music/icons/media-playlist-repeat.svg");
  gtk_image_set_pixel_size(GTK_IMAGE(app->loopImage), 24);
  g_object_ref(app->loopImage);

  app->onetrackActiveImage =
    gtk_image_new_from_resource("/org/aw/Music/icons/media-repeat-single.svg");
  gtk_image_set_pixel_size(GTK_IMAGE(app->onetrackActiveImage), 24);
  g_object_ref(app->onetrackActiveImage);

  app->loopActiveImage = gtk_image_new_from_resource(
    "/org/aw/Music/icons/media-playlist-repeat-active.svg");
  gtk_image_set_pixel_size(GTK_IMAGE(app->loopActiveImage), 24);
  g_object_ref(app->loopActiveImage);

  gtk_button_set_child(app->playButton, app->playImage);
  gtk_button_set_child(app->nextButton, app->nextImage);
  gtk_button_set_child(app->prevButton, app->prevImage);
  gtk_button_set_child(app->loopButton, app->loopImage);
  gtk_button_set_child(app->shuffleButton, app->shuffleImage);
}

static void
build_ui(MusicApp* app)
{
  GtkCssProvider* provider = gtk_css_provider_new();
  gtk_css_provider_load_from_resource(provider, "/org/aw/Music/styles.css");
  GdkDisplay* display = gdk_display_get_default();
  gtk_style_context_add_provider_for_display(
    display, GTK_STYLE_PROVIDER(provider), GTK_STYLE_PROVIDER_PRIORITY_USER);

  GtkBuilder* builder = gtk_builder_new_from_resource("/org/aw/Music/main.ui");

  app->win = GTK_WINDOW(gtk_builder_get_object(builder, "win"));
  app->tracksScrolledWindow = GTK_SCROLLED_WINDOW(
    gtk_builder_get_object(builder, "tracksScrolledWindow"));
  app->appBox = GTK_BOX(gtk_builder_get_object(builder, "appBox"));
  app->upperBox = GTK_BOX(gtk_builder_get_object(builder, "upperBox"));
  app->lowerBox = GTK_BOX(gtk_builder_get_object(builder, "lowerBox"));
  app->playlistBox = GTK_BOX(gtk_builder_get_object(builder, "playlistBox"));
  app->trackButtonsBox =
    GTK_BOX(gtk_builder_get_object(builder, "trackButtonsBox"));
  app->tracksBox = GTK_BOX(gtk_builder_get_object(builder, "tracksBox"));
  app->playlistDD =
    GTK_DROP_DOWN(gtk_builder_get_object(builder, "playlistDropDown"));
  app->trackLabel = GTK_LABEL(gtk_builder_get_object(builder, "trackLabel"));
  app->createPlaylistButton =
    GTK_BUTTON(gtk_builder_get_object(builder, "createPlaylistButton"));
  app->openFolderButton =
    GTK_BUTTON(gtk_builder_get_object(builder, "openFolderButton"));
  app->prevButton = GTK_BUTTON(gtk_builder_get_object(builder, "prevButton"));
  app->playButton = GTK_BUTTON(gtk_builder_get_object(builder, "playButton"));
  app->nextButton = GTK_BUTTON(gtk_builder_get_object(builder, "nextButton"));
  app->shuffleButton =
    GTK_BUTTON(gtk_builder_get_object(builder, "shuffleButton"));
  app->loopButton = GTK_BUTTON(gtk_builder_get_object(builder, "loopButton"));
  app->volumeScale = GTK_SCALE(gtk_builder_get_object(builder, "volumeScale"));
  app->audioPositionScale =
    GTK_SCALE(gtk_builder_get_object(builder, "audioPositionScale"));
  app->audioPositionLabel =
    GTK_LABEL(gtk_builder_get_object(builder, "audioPositionLabel"));
  app->audioLengthLabel =
    GTK_LABEL(gtk_builder_get_object(builder, "audioLengthLabel"));
  app->audioPositionBox =
    GTK_BOX(gtk_builder_get_object(builder, "audioPositionBox"));

  g_object_unref(builder);

  app->dropDownPopover =
    GTK_POPOVER(gtk_widget_get_last_child(GTK_WIDGET(app->playlistDD)));

  // Removing background class from dropdown's popover because of glitchy border
  gtk_widget_remove_css_class(GTK_WIDGET(app->dropDownPopover), "background");

  // Pop down dropdown's popover when child modal windows (dialogs) displayed
  gtk_popover_set_cascade_popdown(app->dropDownPopover, TRUE);

  set_icons(app);
}

static void
setup_signals(MusicApp* app)
{
  // Because GtkScale has its own GtkGestureClick controller, which has priority
  // over custom one, we could simply find that controller and bind our signals
  // to it
  GListModel* controllers_list =
    gtk_widget_observe_controllers(GTK_WIDGET(app->audioPositionScale));
  guint length = g_list_model_get_n_items(controllers_list);
  for (guint i = 0; i < length; i++) {
    GtkEventController* controller = g_list_model_get_item(controllers_list, i);
    if (GTK_IS_GESTURE_CLICK(controller)) {
      gtk_gesture_single_set_button(GTK_GESTURE_SINGLE(controller), 1);
      g_signal_connect(
        controller, "pressed", G_CALLBACK(audioPositionScale_pressed), app);
      g_signal_connect(controller,
                       "released",
                       G_CALLBACK(audioPositionScale_released),
                       app->audioPositionScale);
    }
  }

  audio_system_set_finished_callback(music_finished, app);
  audio_system_set_time_changed_callback(music_time_changed,
                                         app->audioPositionScale);
  audio_system_set_length_changed_callback(media_player_length_changed, app);

  g_signal_connect(app->createPlaylistButton,
                   "clicked",
                   G_CALLBACK(createPlaylist_clicked),
                   app);
  g_signal_connect(
    app->openFolderButton, "clicked", G_CALLBACK(openFolder_clicked), app);
  g_signal_connect(
    app->shuffleButton, "clicked", G_CALLBACK(shuffleButton_clicked), app);
  g_signal_connect(
    app->loopButton, "clicked", G_CALLBACK(loopButton_clicked), app);
  g_signal_connect(
    app->prevButton, "clicked", G_CALLBACK(prevButton_clicked), app);
  g_signal_connect(
    app->nextButton, "clicked", G_CALLBACK(nextButton_clicked), app);
  g_signal_connect(
    app->playButton, "clicked", G_CALLBACK(playButton_clicked), app);
  g_signal_connect(app->volumeScale,
                   "value-changed",
                   G_CALLBACK(volumeScale_value_changed),
                   NULL);
  g_signal_connect(app->audioPositionScale,
                   "value-changed",
                   G_CALLBACK(audioPositionScale_value_changed),
                   app);
}

static void
music_app_activate(GApplication* app)
{
  MusicApp* this = MUSIC_APP(app);
  GResource* resource = resources_get_resource();
  g_resources_register(resource);

  build_ui(this);

  this->playlistLS = g_list_store_new(PLAYLIST_TYPE);
  this->current = NULL;
  this->active = NULL;
  this->options = PLAYBACK_NONE;
  this->trackWidgets = g_ptr_array_new();

  gtk_drop_down_set_model(this->playlistDD, G_LIST_MODEL(this->playlistLS));
  gtk_range_set_range(GTK_RANGE(this->volumeScale), 0.0f, 1.0f);
  gtk_range_set_value(GTK_RANGE(this->volumeScale), 0.5f);
  audio_system_set_volume(0.5f);
  gtk_range_set_range(GTK_RANGE(this->audioPositionScale), 0.0f, 1.0f);

  setup_factories(this);

  setup_signals(this);

  fetch_playlists(this, "playlists");
  context_menu_init(this);

  g_resources_unregister(resource);

  gtk_window_set_application(this->win, GTK_APPLICATION(app));
  gtk_window_present(this->win);
}

static void
music_app_class_init(MusicAppClass* klass)
{
  G_APPLICATION_CLASS(klass)->activate = music_app_activate;
  G_APPLICATION_CLASS(klass)->shutdown = music_app_dispose;
}

MusicApp*
music_app_new()
{
  return g_object_new(MUSIC_APP_TYPE,
                      "application-id",
                      "org.aw.Music",
                      "flags",
                      G_APPLICATION_DEFAULT_FLAGS,
                      NULL);
}

Playlist*
music_app_get_active_playlist(MusicApp* app)
{
  return app->active;
}

void
music_app_set_active_playlist(MusicApp* app, Playlist* playlist)
{
  app->active = playlist;
}

Playlist*
music_app_get_selected_playlist(MusicApp* app)
{
  return gtk_drop_down_get_selected_item(app->playlistDD);
}

GtkWindow*
music_app_get_main_window(MusicApp* app)
{
  return app->win;
}

void
music_app_add_track_widget(MusicApp* app, TrackWidget* widget)
{
  gtk_box_append(app->tracksBox, GTK_WIDGET(widget));
  g_ptr_array_add(app->trackWidgets, widget);
}

void
music_app_remove_track_widget(MusicApp* app, TrackWidget* widget)
{
  int index = track_widget_get_index(widget);

  g_ptr_array_remove_index(app->trackWidgets, index);

  Track* track =
    playlist_remove_track_by_index(music_app_get_selected_playlist(app), index);
  if (track != NULL) {
    if (app->current == track) {
      music_app_set_current_track(app, NULL);
    }
    track_unref(track);
  }

  int offset = playlist_save(music_app_get_selected_playlist(app));
  if (offset) {
    music_app_shift_playlists_lines(app, index, offset);
  }

  gtk_box_remove(app->tracksBox, GTK_WIDGET(widget));
}

void
music_app_reorder_track_widget(GtkBox* box, GtkWidget* widget, guint newPos)
{
  GPtrArray* children = g_ptr_array_new();
  GtkWidget* child = gtk_widget_get_first_child(widget);
  guint current = G_MAXUINT;
  gint index = 0;

  while (child != NULL) {
    g_ptr_array_add(children, child);
    gtk_box_remove(box, g_ptr_array_index(children, index));
    if (child == widget)
      current = index;
    child = gtk_widget_get_next_sibling(child);
    index++;
  }

  if (current == G_MAXUINT || newPos >= index) {
    return;
  }

  child = g_ptr_array_index(children, newPos);
  g_ptr_array_index(children, newPos) = widget;
  g_ptr_array_index(children, current) = child;

  for (index = 0; index < children->len; index++) {
    gtk_box_append(box, g_ptr_array_index(children, index));
  }

  g_ptr_array_free(children, false);
}

void
music_app_switch_playback_icon(MusicApp* app, ButtonTypes type)
{
  switch (type) {
    case BUTTON_PLAY: {
      if (audio_system_get_state() == AUDIO_PLAYING) {
        gtk_button_set_child(app->playButton, app->pauseImage);
      } else {
        gtk_button_set_child(app->playButton, app->playImage);
      }
      break;
    }
    case BUTTON_SHUFFLE: {
      if (app->options & PLAYBACK_SHUFFLE) {
        gtk_button_set_child(app->shuffleButton, app->shuffleActiveImage);
      } else {
        gtk_button_set_child(app->shuffleButton, app->shuffleImage);
      }
      break;
    }
    case BUTTON_LOOP: {
      if (app->options & PLAYBACK_ONETRACK) {
        gtk_button_set_child(app->loopButton, app->onetrackActiveImage);
        break;
      }
      if (app->options & PLAYBACK_LOOP) {
        gtk_button_set_child(app->loopButton, app->loopActiveImage);
      } else {
        gtk_button_set_child(app->loopButton, app->loopImage);
      }
      break;
    }
  }
}

Track*
music_app_get_current_track(MusicApp* app)
{
  return app->current;
}

void
music_app_set_current_track(MusicApp* app, Track* track)
{
  if (app->current != NULL) {
    audio_system_stop_audio();
    music_app_update_current_track_widget(app, AUDIO_STOPPED);
    track_unref(app->current);
  }
  if (track != NULL) {
    app->current = track;
    gtk_label_set_text(app->trackLabel, track->title);
    track_ref(track);
  } else {
    app->current = NULL;
    gtk_label_set_text(app->trackLabel, "No track...");
  }
}

TrackWidget*
music_app_get_track_widget(MusicApp* app, guint index)
{
  if (index >= app->trackWidgets->len)
    return NULL;
  return g_ptr_array_index(app->trackWidgets, index);
}

PlaybackOptions
music_app_get_options(MusicApp* app)
{
  return app->options;
}

void
music_app_set_options(MusicApp* app, PlaybackOptions options)
{
  app->options = options;
}

void
music_app_clear_track_widgets(MusicApp* app)
{
  TrackWidget* widget = NULL;
  while (app->trackWidgets->len > 0) {
    widget =
      APP_TRACK_WIDGET(g_ptr_array_remove_index_fast(app->trackWidgets, 0));
    gtk_box_remove(app->tracksBox, GTK_WIDGET(widget));
  }
}

void
music_app_update_track_widgets_indices(MusicApp* app)
{
  GPtrArray* arr = g_ptr_array_new();
  TrackWidget* temp;
  g_ptr_array_set_size(arr, app->trackWidgets->len);
  for (int i = 0; i < app->trackWidgets->len; i++) {
    temp = g_ptr_array_index(app->trackWidgets, i);
    g_ptr_array_index(arr, track_widget_get_index(temp)) = temp;
  }
  g_ptr_array_free(app->trackWidgets, FALSE);
  app->trackWidgets = arr;
  for (int i = 1; i < arr->len; i++) {
    gtk_box_reorder_child_after(
      app->tracksBox, g_ptr_array_index(arr, i), g_ptr_array_index(arr, i - 1));
  }
}

Playlist*
music_app_get_playlist(MusicApp* app, guint index)
{
  if (index >= g_list_model_get_n_items(G_LIST_MODEL(app->playlistLS)))
    return NULL;
  Playlist* playlist =
    g_list_model_get_item(G_LIST_MODEL(app->playlistLS), index);
  g_object_unref(playlist);
  return playlist;
}

void
music_app_add_playlist(MusicApp* app, Playlist* playlist)
{
  if (playlist_is_folder(playlist)) {
    Playlist* first = music_app_get_playlist(app, 0);
    if (first != NULL && playlist_is_folder(first)) {
      if (app->active == first) {
        music_app_clear_track_widgets(app);
      }
      GObject* new = G_OBJECT(playlist);
      g_list_store_splice(app->playlistLS, 0, 1, (gpointer) & new, 1);
    } else {
      g_list_store_insert(app->playlistLS, 0, playlist);
    }
  } else {
    g_list_store_append(app->playlistLS, playlist);
  }
  // Decrement refcount that was incremented upon allocation because appended
  // playlist will be managed right from listmodel
  g_object_unref(playlist);
}

void
music_app_remove_playlist(MusicApp* app, guint index)
{
  if (index >= g_list_model_get_n_items(G_LIST_MODEL(app->playlistLS)))
    return;

  Playlist* playlist =
    g_list_model_get_item(G_LIST_MODEL(app->playlistLS), index);

  if (playlist == music_app_get_active_playlist(app)) {
    music_app_set_active_playlist(app, NULL);
    music_app_set_current_track(app, NULL);
  }
  g_object_unref(playlist);
  g_list_store_remove(app->playlistLS, index);
}

void
music_app_dropdown_select(MusicApp* app, guint index)
{
  gtk_drop_down_set_selected(app->playlistDD, index);
}

void
music_app_play_track(MusicApp* app)
{
  gtk_range_set_value(GTK_RANGE(app->audioPositionScale), 0.0f);
  Track* track = app->current;

  if (audio_system_open_audio(track->path) == -1) {
    return;
  }

  audio_system_play_audio();
  music_app_update_current_track_widget(app, AUDIO_PLAYING);
  music_app_switch_playback_icon(app, BUTTON_PLAY);
}

void
music_app_duplicate_playlist(MusicApp* app, Playlist* playlist)
{
  Playlist* copy = playlist_duplicate(playlist);
  music_app_add_playlist(app, copy);
  playlist_save(copy);
}

void
music_app_shift_playlists_lines(MusicApp* app, guint index, int offset)
{
  while (index < g_list_model_get_n_items(G_LIST_MODEL(app->playlistLS))) {
    playlist_offset_lines(
      g_list_model_get_item(G_LIST_MODEL(app->playlistLS), index), offset);
    index++;
  }
}

void
music_app_update_audio_position_label(MusicApp* app)
{
  long int time = audio_system_get_time();
  if (time != -1) {
    char* str = music_app_format_into_time_string(time);
    gtk_label_set_text(app->audioPositionLabel, str);
    g_free(str);
  } else {
    gtk_label_set_text(app->audioPositionLabel, "00:00:00");
  }
}

char*
music_app_format_into_time_string(long int milliseconds)
{
  long int temp = milliseconds / 1000;
  long int seconds =
    (milliseconds / 1000.0f) - (float)temp >= 0.5f ? temp + 1 : temp;
  int minutes = 0;
  int hours = 0;
  if (seconds >= 60) {
    minutes = seconds / 60;
    seconds = seconds % 60;
  }
  if (minutes >= 60) {
    hours = minutes / 60;
    minutes = minutes % 60;
  }
  char* str = g_new(char, 128);
  g_snprintf(str, 128, "%02d:%02d:%02ld", hours, minutes, seconds);
  return str;
}

void
music_app_update_position_label_by_length(MusicApp* app, long int milliseconds)
{
  milliseconds =
    milliseconds * gtk_range_get_value(GTK_RANGE(app->audioPositionScale));
  char* str = music_app_format_into_time_string(milliseconds);
  gtk_label_set_text(app->audioPositionLabel, str);
  g_free(str);
}

void
music_app_update_length_label(MusicApp* app)
{
  long int length = audio_system_get_length();
  if (length != -1) {
    char* str = music_app_format_into_time_string(length);
    gtk_label_set_text(app->audioLengthLabel, str);
    g_free(str);
  } else {
    gtk_label_set_text(app->audioLengthLabel, "00:00:00");
  }
}

guint
music_app_get_playlists_count(MusicApp* app)
{
  return g_list_model_get_n_items(G_LIST_MODEL(app->playlistLS));
}

GtkScale*
music_app_get_audio_position_scale(MusicApp* app)
{
  return app->audioPositionScale;
}

void
music_app_update_current_track_widget(MusicApp* app, AudioState state)
{
  if (app->current != NULL) {
    TrackWidget* widget = music_app_get_track_widget(app, app->current->index);
    if (app->current == track_widget_get_track(widget)) {
      track_widget_set_icon(
        music_app_get_track_widget(app, app->current->index), state);
    }
  }
}