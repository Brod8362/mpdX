#ifndef INCLUDED_SONG_SEARCH_H
#define INCLUDED_SONG_SEARCH_H

#include <mpd/client.h>
#include <gtk/gtk.h>

struct search_container {
    GtkListBox* listbox;
    struct mpd_connection* mpd;
};

struct play_container {
    struct mpd_connection* mpd;
    const char* uri;
};

void queue_song_with_song_dialog(GtkWindow* parent, struct mpd_connection* mpd);

#endif