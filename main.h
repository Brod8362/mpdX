#ifndef INCLUDED_MAIN_H
#define INCLUDED_MAIN_H


#include <gtk/gtk.h>

struct mpd_pass {
    int v;
    struct mpd_connection* mpd;
};

GtkButton* play_pause_button;

void update_track_info();

void play_pause_button_click();

void set_volume_bar_level();

int handle_mpd_error();

#endif