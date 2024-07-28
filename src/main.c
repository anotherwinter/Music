#include <gtk/gtk.h>
#include "musicapp.h"

int main(int argc, char *argv[]) {
    return g_application_run(G_APPLICATION(music_app_new()), argc, argv);
}
