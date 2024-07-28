#pragma once
#include "gtk/gtk.h"
#include "musicapp.h"

typedef void (*ContextMenuCallback)(gpointer);
typedef struct ContextMenu ContextMenu;

// alloc struct and build context menu from path
void
context_menu_init(MusicApp* app);

// If trackwidget is TRUE, returns context menu for trackwidget, otherwise
// returns menu for playlist
GtkPopoverMenu*
context_menu_get_menu(bool trackwidget);
void
context_menu_set_menu(const char* path);
void
context_menu_set_data(gpointer data);
gpointer
context_menu_get_data();
ContextMenuCallback
context_menu_get_callback();
void
context_menu_set_callback(ContextMenuCallback cb);
void
context_menu_trigger_callback(gpointer user_data);