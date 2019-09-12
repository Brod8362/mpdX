#include <gtk/gtk.h>
#include <mpd/client.h>
#include <stdbool.h>

#include "mpd_actions.h"

void mpd_play(GSimpleAction* action, GVariant* parameter, gpointer mpd_r) {
	struct mpd_connection* mpd = (struct mpd_connection*)mpd_r;
	mpd_send_pause(mpd, false);
}

void mpd_pause(GSimpleAction* action, GVariant* parameter, gpointer mpd_r) {
	struct mpd_connection* mpd = (struct mpd_connection*)mpd_r;
	mpd_send_pause(mpd, true);
}

void mpd_stop(GSimpleAction* action, GVariant* parameter, gpointer mpd_r) {
	struct mpd_connection* mpd = (struct mpd_connection*)mpd_r;
	mpd_send_stop(mpd);
}

void mpd_toggle(GSimpleAction* action, GVariant* parameter, gpointer mpd_r) {
	struct mpd_connection* mpd = (struct mpd_connection*)mpd_r;
	mpd_send_toggle_pause(mpd);
}

void init_mpd_actions(GtkApplication* app, struct mpd_connection* mpd) {
	mpd_play_action = g_simple_action_new("play", NULL);
	g_signal_connect(mpd_play_action, "activate", G_CALLBACK(mpd_play), mpd);

	mpd_pause_action = g_simple_action_new("pause", NULL);
	g_signal_connect(mpd_pause_action, "activate", G_CALLBACK(mpd_pause), mpd);

	mpd_stop_action = g_simple_action_new("stop", NULL);
	g_signal_connect(mpd_stop_action, "activate", G_CALLBACK(mpd_stop), mpd);

	mpd_next_action = g_simple_action_new("next", NULL);
	//g_signal_connect(mpd_next_action, "activate", G_CALLBACK(), mpd);

	mpd_prev_action = g_simple_action_new("prev", NULL);
	//g_signal_connect(mpd_prev_action, "activate", G_CALLBACK(mpd_play), mpd);

	mpd_playback_toggle = g_simple_action_new("toggle", NULL);
	g_signal_connect(mpd_playback_toggle, "activate", G_CALLBACK(mpd_toggle), mpd);

	GActionEntry app_entries[] = {
		{"app.play", mpd_play, NULL, NULL, NULL},
		{"app.pause", mpd_pause, NULL, NULL, NULL},
		{"app.stop", mpd_stop, NULL, NULL, NULL},
		{"app.toggle", mpd_toggle, NULL, NULL, NULL}
	};

	g_action_map_add_action_entries(G_ACTION_MAP(app), app_entries, G_N_ELEMENTS(app_entries), app);
	g_action_map_add_action(G_ACTION_MAP(app), G_ACTION(mpd_play_action));
	g_action_map_add_action(G_ACTION_MAP(app), G_ACTION(mpd_pause_action));
	g_action_map_add_action(G_ACTION_MAP(app), G_ACTION(mpd_stop_action));
	g_action_map_add_action(G_ACTION_MAP(app), G_ACTION(mpd_playback_toggle));
}
