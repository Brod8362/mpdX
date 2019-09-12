#include <gtk/gtk.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <argp.h>
#include <mpd/client.h>
#include <mpd/status.h>
#include <mpd/entity.h>

#include "mpd_actions.h"
#include "actions.h"

char* mpd_conn_dest = "localhost";
int mpd_port = 6600;
int mpd_timeout = 10000;

struct mpd_connection *mpd;

const char* argp_program_version = "mpdX v0.0";
const char* argp_program_bug_address = "brod8362@gmail.com";
static char doc[] = "mpd controller written in C for the X window system";
static char args_doc[] = "";
static struct argp_option options[] = { 
	{ "debug", 'd', 0, 0, "Enable debug mode"},
	{ "port", 'p', "PORT", 0, "Set mpd port to connect on"},
	{ "ip", 'l', "ADDRESS", 0, "Set IP address of mpd server"},
	{ 0 }
};

static bool debug = false;

static void debug_log(char* str) {
	if (debug) {
		printf("%s\n", str);
	}
}

static void vol_change(GtkAdjustment* adj, gpointer v) {
	int val = (int)gtk_adjustment_get_value(adj);
	mpd_set_vol(mpd, val);
	gtk_adjustment_set_value(adj, val);
}

static void play_pause_button_click(GtkButton* button, gpointer a) {
	mpd_run_toggle_pause(mpd);
	const struct mpd_status* status;
	mpd_send_status(mpd);
	status = mpd_recv_status(mpd);
	enum mpd_state st = mpd_status_get_state(status);
	GtkImage* but_img = (GtkImage*)gtk_button_get_image(button);
	if (st == MPD_STATE_PLAY) {
		gtk_image_set_from_icon_name(but_img, "media-playback-pause", GTK_ICON_SIZE_BUTTON);
	} else if (st == MPD_STATE_PAUSE) {
		gtk_image_set_from_icon_name(but_img, "media-playback-start", GTK_ICON_SIZE_BUTTON);
	}
}

static void init_grid(GtkWindow* window) {
	GtkGrid* grid;
	GtkWidget* toggle_button;
	GtkWidget* prev_button;
	GtkWidget* next_button;
	GtkWidget* stop_button;
	GtkWidget* vol_bar;
	GtkWidget* vol_icon;

	GtkAdjustment* adjust;

	grid = GTK_GRID(gtk_grid_new());
	
	toggle_button = gtk_button_new_from_icon_name("media-playback-start", GTK_ICON_SIZE_BUTTON);
	prev_button = gtk_button_new_from_icon_name("media-skip-backward", GTK_ICON_SIZE_BUTTON);
	next_button = gtk_button_new_from_icon_name("media-skip-forward", GTK_ICON_SIZE_BUTTON);
	stop_button = gtk_button_new_from_icon_name("media-playback-stop", GTK_ICON_SIZE_BUTTON);

	
	vol_icon = gtk_image_new_from_icon_name("audio-volume-medium", GTK_ICON_SIZE_BUTTON);

	adjust = gtk_adjustment_new(0, 0, 100, 1, 0, 0);
	vol_bar = gtk_scale_new(GTK_ORIENTATION_HORIZONTAL,adjust);
	gtk_adjustment_set_value(adjust, mpd_get_vol(mpd));
	
	for (int i = 0; i <= 100; i+=25) {
		gtk_scale_add_mark(GTK_SCALE(vol_bar), i, GTK_BASELINE_POSITION_CENTER, NULL);
	}

	gtk_grid_attach(grid, vol_icon, 0, 1, 1, 1);
	gtk_grid_attach(grid, vol_bar, 1, 1, 4, 1);
	gtk_grid_attach(grid, prev_button, 0, 5, 1, 1);
	gtk_grid_attach(grid, toggle_button, 1, 5, 2, 1);
	gtk_grid_attach(grid, stop_button, 3, 5, 1, 1);
	gtk_grid_attach(grid, next_button, 4, 5, 1, 1);
	
	g_signal_connect(toggle_button, "clicked", G_CALLBACK(play_pause_button_click), NULL);
	g_signal_connect(stop_button, "clicked", G_CALLBACK(mpd_stop), mpd);
	g_signal_connect(next_button, "clicked", G_CALLBACK(mpd_next), mpd);
	g_signal_connect(prev_button, "clicked", G_CALLBACK(mpd_prev), mpd);
	g_signal_connect(adjust, "value-changed", G_CALLBACK(vol_change), NULL);

	gtk_container_add(GTK_CONTAINER(window), GTK_WIDGET(grid));
}

static void initialize_menu_bar(GtkApplication* app) {
	GtkBuilder* builder;
	GMenuModel* app_menu;
	GResource* resources;

	init_actions(app);
	
	builder = gtk_builder_new_from_resource("/byakuren/resources/menubar.xml");
	app_menu = G_MENU_MODEL(gtk_builder_get_object(builder, "menubar"));
	gtk_application_set_app_menu(app, app_menu);
	g_object_unref(builder); //free memory used by builder
}

static int handle_mpd_error() {
	g_assert(mpd_connection_get_error(mpd) != MPD_ERROR_SUCCESS);

	debug_log((char*)mpd_connection_get_error_message(mpd));
	mpd_connection_free(mpd);
	return 1;
}

static int init_mpd() {
	mpd = mpd_connection_new(mpd_conn_dest, mpd_port, mpd_timeout);
	
	if (mpd_connection_get_error(mpd) != MPD_ERROR_SUCCESS) {
		debug_log("failed to connect to mpd");
		return handle_mpd_error(mpd);
	}
	debug_log("connected to mpd");

	g_assert(mpd != NULL);
	return 0;
}

static void activate(GtkApplication* app, gpointer data) {
	GtkWidget* window;
	debug_log("Initializing mpdX");

	/* init base window */	
	window = gtk_application_window_new(app);
	gtk_window_set_title(GTK_WINDOW(window), "mpdX");
	gtk_window_set_default_size(GTK_WINDOW(window), 825, 400);

	/* init menubar */
	initialize_menu_bar(app);
	init_grid(GTK_WINDOW(window));
	gtk_widget_show_all(window);
}

static error_t parse_opt(int key, char* arg, struct argp_state *state) {
	 struct arguments *arguments = state->input;
    switch (key) {
		case 'd': 
			debug=true;
			debug_log("running in debug mode");
			break;
		case 'p':
			mpd_port=atoi(arg);
			break;
		case 'l':
			mpd_conn_dest=arg;
			break;
		default: return ARGP_ERR_UNKNOWN;
    }   
    return 0;
}

/* main argp struct */
static struct argp argp = { options, parse_opt, args_doc, 0, 0, 0};
// known issue - arguments say "unknown option"

int main(int argc, char *argv[]) {
	/* cli argument init */
	argp_parse(&argp, argc, argv, 0, 0, 0);

	/* init mpd conn */
	int r = init_mpd();
	if (r != 0) {
		return r;
	}

	/* gtk init */
	GtkApplication* app;
	int status;
	app = gtk_application_new("pw.byakuren.mpdX", G_APPLICATION_FLAGS_NONE);
	/* init actions */
	init_mpd_actions(app, mpd);
	g_signal_connect(app, "activate", G_CALLBACK(activate), NULL);
	status = g_application_run(G_APPLICATION(app), argc, argv);
	g_object_unref(app);

	

	return status;
}
