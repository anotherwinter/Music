#pragma once
#include "gtk/gtk.h"

typedef struct ContextMenu ContextMenu;

// alloc struct and build context menu from path
void
context_menu_init(GtkApplication* app);

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