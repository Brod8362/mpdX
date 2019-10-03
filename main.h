#ifndef INCLUDED_MAIN_H
#define INCLUDED_MAIN_H


#include <gtk/gtk.h>

struct mpd_pass {
    unsigned int v;
    struct mpd_connection* mpd;
};

struct queue_button_group {
    GtkWidget* container;
    GtkWidget* song;
    GtkWidget* remove;
    struct mpd_pass* pass;
};

GtkButton* play_pause_button;

void display_non_fatal_error(const char* message);

void display_fatal_error(const char* message);

void update_track_info();

void play_pause_button_click();

void set_volume_bar_level();

int handle_mpd_error();

#endif