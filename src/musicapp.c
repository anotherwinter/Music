#include "musicapp.h"
#include "audiosystem.h"
#include "contextmenu.h"
#include "enum_types.h"
#include "gio/gio.h"
#include "gtk/gtk.h"
#include "gtk/gtkcssprovider.h"
#include "handlers.h"
#include "playlist.h"
#include "resources.h"
#include "trackwidget.h"
#include <string.h>

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

  GPtrArray* playlists;
  GPtrArray* trackWidgets;
  Playlist* active;
  TrackWidget* current;
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
music_app_init(MusicApp* app)
{
  init_systems();
}

static void
fetch_playlists(MusicApp* app, GPtrArray* playlists, gchar* path)
{
  if (playlists == NULL) {
    playlists = g_ptr_array_new();
  }
  parse_playlists(playlists, "playlists");
  if (playlists->len > 0) {
    GtkStringObject* playlistNameSO = NULL;
    for (guint i = 0; i < playlists->len; i++) {
      playlistNameSO = gtk_string_object_new(
        playlist_get_name(g_ptr_array_index(playlists, i)));
      music_app_list_store_append(app, playlistNameSO, PLAYLIST_NONE);
      g_object_unref(playlistNameSO);
    }

    // Active playlist is being switched automatically in selection_changed()
    // callback, so no need to call music_app_switch_active_playlist()
    music_app_dropdown_select(app, 0);
  }
}

// Create "blank" widget for setting data on it then in bind_factory()
static void
setup_factory_signal(GtkSignalListItemFactory* factory,
                     GtkListItem* list_item,
                     gpointer user_data)
{
  MusicApp* app = MUSIC_APP(user_data);
  g_signal_connect(
    app->playlistDD, "notify::selected", G_CALLBACK(selection_changed), app);

  GtkWidget* box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
  GtkWidget* label = gtk_label_new(NULL);
  gtk_label_set_xalign(GTK_LABEL(label), 0.0f);
  gtk_widget_set_hexpand(label, TRUE);

  gtk_box_append(GTK_BOX(box), label);
  gtk_list_item_set_child(list_item, box);
}

// Bind data to widget after creating it
static void
bind_factory_signal(GtkSignalListItemFactory* factory,
                    GtkListItem* list_item,
                    gpointer user_data)
{
  GtkWidget* box = gtk_list_item_get_child(GTK_LIST_ITEM(list_item));
  GtkWidget* label = gtk_widget_get_first_child(box);
  GtkStringObject* item = GTK_STRING_OBJECT(gtk_list_item_get_item(list_item));
  const gchar* string = gtk_string_object_get_string(item);

  gtk_label_set_text(GTK_LABEL(label), string);

  g_object_set_data(G_OBJECT(box), "item", list_item);
  g_object_set_data(G_OBJECT(box), "store", user_data);
  g_object_set_data(
    G_OBJECT(box),
    "playlist",
    music_app_get_playlist(g_object_get_data(G_OBJECT(user_data), "app"),
                           gtk_list_item_get_position(list_item)));

  GtkGesture* gesture = gtk_gesture_click_new();
  gtk_gesture_single_set_button(GTK_GESTURE_SINGLE(gesture), 3);
  g_signal_connect(gesture, "pressed", G_CALLBACK(playlist_clicked), box);
  gtk_widget_add_controller(GTK_WIDGET(box), GTK_EVENT_CONTROLLER(gesture));

  // When binding item we have access to its GtkListItemWidget - container for
  // all other widgets that form our custom widget. We can easily access it by
  // getting parent of child (box here) for changing background color for whole
  // widget
  gtk_widget_add_css_class(gtk_widget_get_parent(box), "playlistPopover");
}

static void
unbind_factory_signal(GtkSignalListItemFactory* self,
                      GtkListItem* list_item,
                      gpointer user_data)
{
  GtkWidget* box = gtk_list_item_get_child(list_item);
  GtkWidget* label = gtk_widget_get_first_child(box);

  gtk_label_set_text(GTK_LABEL(label), NULL);
  g_object_set_data(G_OBJECT(box), "item", NULL);
  g_object_set_data(G_OBJECT(box), "store", NULL);
}

static void
teardown_factory_signal(GtkSignalListItemFactory* factory,
                        GtkListItem* list_item,
                        gpointer user_data)
{
  // GtkWidget* box = gtk_list_item_get_child(list_item); this is possibly
  // unwanted thing GtkWidget* button = gtk_widget_get_last_child(box);
  // GtkWidget* label = gtk_widget_get_first_child(box);
  // gtk_box_remove(GTK_BOX(box), label);
  // gtk_box_remove(GTK_BOX(box), button);
}

static void
create_factory(MusicApp* app)
{
  GtkSignalListItemFactory* factory =
    GTK_SIGNAL_LIST_ITEM_FACTORY(gtk_signal_list_item_factory_new());

  g_object_set_data(G_OBJECT(app->playlistLS), "app", app);

  g_signal_connect(factory, "setup", G_CALLBACK(setup_factory_signal), app);
  g_signal_connect(
    factory, "bind", G_CALLBACK(bind_factory_signal), app->playlistLS);
  g_signal_connect(factory, "unbind", G_CALLBACK(unbind_factory_signal), NULL);
  g_signal_connect(
    factory, "teardown", G_CALLBACK(teardown_factory_signal), NULL);

  gtk_drop_down_set_factory(app->playlistDD, GTK_LIST_ITEM_FACTORY(factory));

  // Removing background class from dropdown's popover because of glitchy border
  gtk_widget_remove_css_class(
    gtk_widget_get_last_child(GTK_WIDGET(app->playlistDD)), "background");
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
  GResource* resource = resources_get_resource();
  g_resources_register(resource);

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

  set_icons(app);

  g_resources_unregister(resource);
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

  build_ui(this);

  this->playlistLS = g_list_store_new(GTK_TYPE_STRING_OBJECT);
  this->current = NULL;
  this->active = NULL;
  this->options = PLAYBACK_NONE;
  this->playlists = g_ptr_array_new();
  this->trackWidgets = g_ptr_array_new();

  gtk_drop_down_set_model(this->playlistDD, G_LIST_MODEL(this->playlistLS));
  gtk_range_set_range(GTK_RANGE(this->volumeScale), 0.0f, 1.0f);
  gtk_range_set_value(GTK_RANGE(this->volumeScale), 0.5f);
  audio_system_set_volume(0.5f);
  gtk_range_set_range(GTK_RANGE(this->audioPositionScale), 0.0f, 1.0f);

  create_factory(this);

  setup_signals(this);

  fetch_playlists(this, this->playlists, "playlists");
  context_menu_init(GTK_APPLICATION(this));

  gtk_window_set_application(this->win, GTK_APPLICATION(app));
  gtk_window_present(this->win);
}

static void
music_app_class_init(MusicAppClass* klass)
{
  G_APPLICATION_CLASS(klass)->activate = music_app_activate;
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

GtkWindow*
music_app_get_main_window(MusicApp* app)
{
  return app->win;
}

GtkBox*
music_app_get_tracks_box(MusicApp* app)
{
  return app->tracksBox;
}

const GPtrArray*
music_app_get_playlists(MusicApp* app)
{
  return app->playlists;
}

GtkDropDown*
music_app_get_dropdown(MusicApp* app)
{
  return app->playlistDD;
}

GListStore*
music_app_get_liststore(MusicApp* app)
{
  return app->playlistLS;
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

  if (app->current == widget) {
    music_app_reset_current_track_widget(app);
  }

  g_ptr_array_remove_index(app->trackWidgets, index);

  Track* track = playlist_remove_track_by_index(
    music_app_get_active_playlist(app), track_widget_get_track(widget)->index);
  if (track != NULL) { // Free track pointer if trackwidget had it
    track_free(track);
  }
  int offset = playlist_save(music_app_get_active_playlist(app));
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
      if (audio_system_is_playing()) {
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

TrackWidget*
music_app_get_current_track_widget(MusicApp* app)
{
  return app->current;
}

void
music_app_set_current_track_widget(MusicApp* app,
                                   TrackWidget* widget,
                                   guint index,
                                   TrackWidgetState state)
{
  if (app->current != NULL) {
    track_widget_set_state(app->current, TRACK_INACTIVE);
  }
  if (index != G_MAXUINT) {
    widget = music_app_get_track_widget(app, index);
  }
  if (widget != NULL) {
    app->current = widget;
    track_widget_set_state(app->current, state);
    gtk_label_set_text(app->trackLabel, track_widget_get_track(widget)->title);
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
  music_app_reset_current_track_widget(app);
  TrackWidget* widget = NULL;
  while (app->trackWidgets->len > 0) {
    widget =
      APP_TRACK_WIDGET(g_ptr_array_remove_index_fast(app->trackWidgets, 0));
    gtk_box_remove(app->tracksBox, GTK_WIDGET(widget));
  }
}

GPtrArray*
music_app_get_track_widgets(MusicApp* app)
{
  return app->trackWidgets;
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
  if (index >= app->playlists->len)
    return NULL;
  return g_ptr_array_index(app->playlists, index);
}

void
music_app_add_playlist(MusicApp* app, Playlist* playlist)
{
  if (playlist_is_folder(playlist)) {
    Playlist* first = music_app_get_playlist(app, 0);
    if (first != NULL && playlist_is_folder(first)) {
      if (app->active == first) {
        music_app_clear_track_widgets(app);
        music_app_set_active_playlist(app, NULL);
      }
      playlist_free(g_ptr_array_remove_index_fast(app->playlists, 0));
      g_list_store_remove(app->playlistLS, 0);
    }
    g_ptr_array_insert(app->playlists, 0, playlist);
  } else {
    g_ptr_array_add(app->playlists, playlist);
  }
}

void
music_app_switch_playlist(MusicApp* app, Playlist* new)
{
  if (app->active != NULL) {
    if (!playlist_is_empty(app->active)) {
      music_app_reset_current_track_widget(app);
      music_app_clear_track_widgets(app);
    }
  }
  if (new != NULL) {
    GPtrArray* tracks = playlist_get_tracks(new);
    for (int i = 0; i < tracks->len; i++) {
      Track* track = g_ptr_array_index(tracks, i);
      music_app_add_track_widget(
        app, track_widget_configure(track_widget_new(app, track), app, track));
    }
  }
  music_app_set_active_playlist(app, new);
}

void
music_app_remove_playlist(MusicApp* app, guint index)
{
  if (index >= app->trackWidgets->len)
    return;

  Playlist* playlist = g_ptr_array_remove_index(app->playlists, index);

  if (playlist == music_app_get_active_playlist(app)) {
    music_app_switch_playlist(app, NULL);
  }
  playlist_free(playlist);
}

void
music_app_reset_current_track_widget(MusicApp* app)
{
  if (audio_system_is_playing()) {
    audio_system_stop_audio();
    music_app_switch_playback_icon(app, BUTTON_PLAY);
  }
  music_app_set_current_track_widget(app, NULL, G_MAXUINT, TRACK_INACTIVE);
  gtk_label_set_text(app->audioPositionLabel, "00:00:00");
  gtk_label_set_text(app->audioLengthLabel, "00:00:00");
}

void
music_app_list_store_append(MusicApp* app,
                            GtkStringObject* str,
                            PlaylistTypes type)
{
  switch (type) {
    case PLAYLIST_NEW: {
      g_list_store_append(app->playlistLS, str);
      gtk_drop_down_set_selected(app->playlistDD, app->playlists->len - 1);
      break;
    }
    case PLAYLIST_FOLDER: {
      g_list_store_insert(app->playlistLS, 0, str);
      gtk_drop_down_set_selected(app->playlistDD, 0);
      break;
    }
    default: {
      g_list_store_append(app->playlistLS, str);
      break;
    }
  }
}

void
music_app_dropdown_select(MusicApp* app, guint index)
{
  gtk_drop_down_set_selected(app->playlistDD, index);
}

void
music_app_play_widget(MusicApp* app, TrackWidget* widget)
{
  Track* track = track_widget_get_track(widget);
  gtk_range_set_value(GTK_RANGE(app->audioPositionScale), 0.0f);
  gtk_label_set_text(app->audioPositionLabel, "00:00:00");

  audio_system_stop_audio();
  if (audio_system_open_audio(track->path) == -1) {
    return;
  }

  music_app_set_current_track_widget(app, NULL, track->index, TRACK_PLAYING);

  audio_system_play_audio();
  music_app_switch_playback_icon(app, BUTTON_PLAY);
}

void
music_app_duplicate_playlist(MusicApp* app, Playlist* playlist)
{
  Playlist* copy = playlist_duplicate(playlist);
  music_app_add_playlist(app, copy);
  GtkStringObject* so = gtk_string_object_new(playlist_get_name(copy));
  music_app_list_store_append(app, so, PLAYLIST_NONE);
  g_object_unref(so);
  playlist_save(copy);
}

void
music_app_shift_playlists_lines(MusicApp* app, guint index, int offset)
{
  while (index < app->playlists->len) {
    playlist_offset_lines(g_ptr_array_index(app->playlists, index), offset);
    index++;
  }
}

GtkScale*
music_app_get_audio_position_scale(MusicApp* app)
{
  return app->audioPositionScale;
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
  long int seconds = milliseconds / 1000;
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
music_app_set_position_label_text(MusicApp* app, long int milliseconds)
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