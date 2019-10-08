#ifndef INCLUDED_MPD_ACTIONS_H
#define INCLUDED_MPD_ACTIONS_H

#include <gtk/gtk.h>
#include <mpd/client.h>
#include "main.h"

GSimpleAction* mpd_play_action;
GSimpleAction* mpd_pause_action;
GSimpleAction* mpd_playback_toggle;
GSimpleAction* mpd_stop_action;
GSimpleAction* mpd_next_action;
GSimpleAction* mpd_prev_action;
GSimpleAction* mpd_vol_up_action;
GSimpleAction* mpd_vol_down_action;
GSimpleAction* mpd_mute_action;

void mpd_play(GSimpleAction* action, GVariant* parameter, gpointer mpd_r);

void mpd_pause(GSimpleAction* action, GVariant* parameter, gpointer mpd_r);

void mpd_stop(GSimpleAction* action, GVariant* parameter, gpointer mpd_r);

void mpd_toggle(struct mpd_connection* mpd);

void mpd_toggle_act(GSimpleAction* action, GVariant* parameter, gpointer mpd_r);

void mpd_toggle_button(GtkButton* button, gpointer mpd_r);

void mpd_vol_up(GSimpleAction* action, GVariant* parameter, gpointer mpd_r);

void mpd_vol_down(GSimpleAction* action, GVariant* parameter, gpointer mpd_r);

void mpd_vol_mute(GSimpleAction* action, GVariant* parameter, gpointer mpd_r);

void mpd_next(GSimpleAction* action, GVariant* parameter, gpointer mpd_r);

void mpd_prev(GSimpleAction* action, GVariant* parameter, gpointer mpd_r);

void mpd_unqueue_song_id(struct mpd_pass* pass);

void mpd_play_song_id(struct mpd_pass* pass);

void mpd_play_song_id_button(GtkButton* button, gpointer mpd_r);

void mpd_play_song_pos(struct mpd_pass* pass);

void mpd_play_song_pos_button(GtkButton *button, gpointer mpd_r);

int mpd_get_vol(struct mpd_connection* mpd);

int mpd_set_vol(struct mpd_connection* mpd, int vol);

bool mpd_get_repeat(struct mpd_connection* mpd);

void mpd_set_repeat(struct mpd_connection* mpd, bool m);

void mpd_toggle_repeat(struct mpd_connection* mpd);

void mpd_toggle_repeat_button(GtkButton* button, gpointer mpd_r);

void init_mpd_actions(GtkApplication* app, struct mpd_connection* mpd);



#endif