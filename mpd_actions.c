#include <gtk/gtk.h>
#include <mpd/client.h>
#include <mpd/status.h>
#include <stdbool.h>

#include "mpd_actions.h"
#include "main.h"

void mpd_play(GSimpleAction* action, GVariant* parameter, gpointer mpd_r) {
	struct mpd_connection* mpd = (struct mpd_connection*)mpd_r;
	mpd_run_pause(mpd, false);
}

void mpd_pause(GSimpleAction* action, GVariant* parameter, gpointer mpd_r) {
	struct mpd_connection* mpd = (struct mpd_connection*)mpd_r;
	mpd_run_pause(mpd, true);
}

void mpd_stop(GSimpleAction* action, GVariant* parameter, gpointer mpd_r) {
	struct mpd_connection* mpd = (struct mpd_connection*)mpd_r;
	mpd_run_stop(mpd);
}

void mpd_toggle(struct mpd_connection* mpd) {
	mpd_run_toggle_pause(mpd);
	play_pause_button_click();
}

void mpd_toggle_button(GtkButton* button, gpointer mpd_r) {
	mpd_toggle((struct mpd_connection*)mpd_r);
}

void mpd_toggle_act(GSimpleAction* action, GVariant* parameter, gpointer mpd_r) {
	mpd_toggle((struct mpd_connection*)mpd_r);
}

void mpd_next(GSimpleAction* action, GVariant* parameter, gpointer mpd_r) {
	struct mpd_connection* mpd = (struct mpd_connection*)mpd_r;
	mpd_run_next(mpd);
	update_track_info();
}

void mpd_prev(GSimpleAction* action, GVariant* parameter, gpointer mpd_r) {
	struct mpd_connection* mpd = (struct mpd_connection*)mpd_r;
	mpd_run_previous(mpd);
	update_track_info();
}

void mpd_vol_up(GSimpleAction* action, GVariant* parameter, gpointer mpd_r) {
	struct mpd_connection* mpd = (struct mpd_connection*)mpd_r;
	mpd_run_change_volume(mpd, +5);
	set_volume_bar_level(mpd_get_vol(mpd));
}

void mpd_vol_down(GSimpleAction* action, GVariant* parameter, gpointer mpd_r) {
	struct mpd_connection* mpd = (struct mpd_connection*)mpd_r;
	mpd_run_change_volume(mpd, -5);
	set_volume_bar_level(mpd_get_vol(mpd));
}

void mpd_vol_mute(GSimpleAction* action, GVariant* parameter, gpointer mpd_r) {
	struct mpd_connection* mpd = (struct mpd_connection*)mpd_r;
	mpd_run_set_volume(mpd, 0);
	set_volume_bar_level(mpd_get_vol(mpd));
}

int mpd_get_vol(struct mpd_connection* mpd) {
	struct mpd_status* status;
	mpd_send_status(mpd);
	status = mpd_recv_status(mpd);
	int v = mpd_status_get_volume(status);
	free(status);
	return v;
}

int mpd_set_vol(struct mpd_connection* mpd, int vol) {
	mpd_run_set_volume(mpd, vol);
}

void mpd_play_song_id(struct mpd_pass* pass) {
	mpd_run_play_id(pass->mpd, pass->v);
	update_track_info();
}

void mpd_unqueue_song_id(struct mpd_pass* pass) {
	mpd_run_delete_id(pass->mpd, pass->v);
}

void mpd_play_song_id_button(GtkButton* button, gpointer mpd_r) {
	mpd_play_song_id((struct mpd_pass*)mpd_r);
}

void mpd_play_song_pos(struct mpd_pass* pass) {
	mpd_run_play_pos(pass->mpd, pass->v);
	update_track_info();
}

void mpd_play_song_pos_button(GtkButton* button, gpointer mpd_r) {
	mpd_play_song_pos((struct mpd_pass*)mpd_r);
}

void mpd_toggle_repeat_button(GtkButton* button, gpointer mpd_r) {
	struct mpd_connection* mpd = (struct mpd_connection*)mpd_r;
	mpd_toggle_repeat(mpd);
}

bool mpd_get_repeat(struct mpd_connection* mpd) {
	struct mpd_status* status = mpd_run_status(mpd);
	bool r = mpd_status_get_repeat(status);
	mpd_status_free(status);
	return r;
}

void mpd_set_repeat(struct mpd_connection* mpd, bool m) {
	mpd_run_repeat(mpd, m);
}

void mpd_toggle_repeat(struct mpd_connection* mpd) {
	mpd_set_repeat(mpd, !mpd_get_repeat(mpd));
}

void init_mpd_actions(GtkApplication* app, struct mpd_connection* mpd) {

	g_assert(mpd != NULL);
	g_assert(app != NULL);

	mpd_play_action = g_simple_action_new("play", NULL);
	g_signal_connect(mpd_play_action, "activate", G_CALLBACK(mpd_play), mpd);
	g_action_map_add_action(G_ACTION_MAP(app), G_ACTION(mpd_play_action));

	mpd_pause_action = g_simple_action_new("pause", NULL);
	g_signal_connect(mpd_pause_action, "activate", G_CALLBACK(mpd_pause), mpd);
	g_action_map_add_action(G_ACTION_MAP(app), G_ACTION(mpd_pause_action));

	mpd_stop_action = g_simple_action_new("stop", NULL);
	g_signal_connect(mpd_stop_action, "activate", G_CALLBACK(mpd_stop), mpd);
	g_action_map_add_action(G_ACTION_MAP(app), G_ACTION(mpd_stop_action));

	mpd_next_action = g_simple_action_new("next", NULL);
	g_signal_connect(mpd_next_action, "activate", G_CALLBACK(mpd_next), mpd);
	g_action_map_add_action(G_ACTION_MAP(app), G_ACTION(mpd_next_action));

	mpd_prev_action = g_simple_action_new("prev", NULL);
	g_signal_connect(mpd_prev_action, "activate", G_CALLBACK(mpd_prev), mpd);
	g_action_map_add_action(G_ACTION_MAP(app), G_ACTION(mpd_prev_action));

	mpd_playback_toggle = g_simple_action_new("toggle", NULL);
	g_signal_connect(mpd_playback_toggle, "activate", G_CALLBACK(mpd_toggle_act), mpd);
	g_action_map_add_action(G_ACTION_MAP(app), G_ACTION(mpd_playback_toggle));

	mpd_vol_up_action = g_simple_action_new("volup", NULL);
	g_signal_connect(mpd_vol_up_action, "activate", G_CALLBACK(mpd_vol_up), mpd);
	g_action_map_add_action(G_ACTION_MAP(app), G_ACTION(mpd_vol_up_action));

	mpd_vol_down_action = g_simple_action_new("voldown", NULL);
	g_signal_connect(mpd_vol_down_action, "activate", G_CALLBACK(mpd_vol_down), mpd);
	g_action_map_add_action(G_ACTION_MAP(app), G_ACTION(mpd_vol_down_action));

	mpd_mute_action = g_simple_action_new("mute", NULL);
	g_signal_connect(mpd_mute_action, "activate", G_CALLBACK(mpd_vol_mute), mpd);
	g_action_map_add_action(G_ACTION_MAP(app), G_ACTION(mpd_mute_action));

	GActionEntry app_entries[] = {
		{"app.play", mpd_play, NULL, NULL, NULL},
		{"app.pause", mpd_pause, NULL, NULL, NULL},
		{"app.stop", mpd_stop, NULL, NULL, NULL},
		{"app.toggle", mpd_toggle_act, NULL, NULL, NULL},
		{"app.next", mpd_next, NULL, NULL, NULL},
		{"app.prev", mpd_prev, NULL, NULL, NULL},
		{"app.mute", mpd_vol_mute, NULL, NULL, NULL}
	};	
	g_action_map_add_action_entries(G_ACTION_MAP(app), app_entries, G_N_ELEMENTS(app_entries), app);
}
