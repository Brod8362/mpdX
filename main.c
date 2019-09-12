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
	{ "port", 'p', "PORT", 0, "Set mpd port to conncet on"},
	{ "ip", 'l', "ADDRESS", 0, "Set IP address of mpd server"},
	{ 0 }
};

static bool debug = false;

static void debug_log(char* str) {
	if (debug) {
		printf("%s\n", str);
	}
}

static void initialize_menu_bar(GtkApplication* app) {
	GtkBuilder* builder;
	GMenuModel* app_menu;
	GResource* resources;

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
	g_signal_connect(app, "activate", G_CALLBACK(activate), NULL);
	status = g_application_run(G_APPLICATION(app), argc, argv);
	g_object_unref(app);

	/* init actions */
	init_mpd_actions(app, mpd);

	return status;
}
