#ifndef INCLUDED_MPD_ACTIONS_H
#define INCLUDED_MPD_ACTIONS_H

#include <gtk/gtk.h>
#include <mpd/client.h>

GSimpleAction* mpd_play_action;
GSimpleAction* mpd_pause_action;
GSimpleAction* mpd_playback_toggle;
GSimpleAction* mpd_stop_action;
GSimpleAction* mpd_next_action;
GSimpleAction* mpd_prev_action;

static void mpd_play(GSimpleAction* action, GVariant* parameter, gpointer mpd_r);

static void mpd_pause(GSimpleAction* action, GVariant* parameter, gpointer mpd_r);

static void mpd_stop(GSimpleAction* action, GVariant* parameter, gpointer mpd_r);

static void mpd_toggle(GSimpleAction* action, GVariant* parameter, gpointer mpd_r);

static void init_mpd_actions(GtkApplication* app, struct mpd_connection* mpd);

#endif