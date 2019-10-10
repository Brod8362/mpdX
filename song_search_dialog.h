#ifndef INCLUDED_SONG_SEARCH_H
#define INCLUDED_SONG_SEARCH_H

#include <mpd/client.h>
#include <gtk/gtk.h>

struct mpd_song* get_song_with_song_dialog(GtkWindow* parent, struct mpd_connection* mpd);

#endif