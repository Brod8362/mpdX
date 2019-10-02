#include <gtk/gtk.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <argp.h>
#include <mpd/client.h>
#include <mpd/status.h>
#include <mpd/entity.h>
#include <mpd/socket.h>

#include "mpd_actions.h"
#include "actions.h"

char* mpd_conn_dest = "localhost";
int mpd_port = 6600;
int mpd_timeout = 10000;

struct mpd_connection *mpd;

GtkWidget* title_text; //text buffer for songname
GtkWidget* artist_text;

GtkButton* play_pause_button;
GtkAdjustment* volume_bar;
GtkAdjustment* time_bar;

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

void set_volume_bar_level(int val) {
	gtk_adjustment_set_value(volume_bar, val);
}

static void vol_change() {
	int val = (int)gtk_adjustment_get_value(volume_bar);
	mpd_set_vol(mpd, val);
	set_volume_bar_level(val);
}

void update_play_bar() {
	struct mpd_song* song = mpd_run_current_song(mpd);
	struct mpd_status* status = mpd_run_status(mpd);
	
	int length = mpd_song_get_duration(song);
	int elapsed = mpd_status_get_elapsed_time(status);

	gtk_adjustment_set_value(time_bar, elapsed);
	gtk_adjustment_set_upper(time_bar, length);

	mpd_song_free(song);
	mpd_status_free(status);
}

void update_track_info() {
	struct mpd_song* song = mpd_run_current_song(mpd);
	if (song == NULL) {
		puts("song is null in update_track_info()");
		return;
	}
	const char* title = mpd_song_get_tag(song, MPD_TAG_TITLE, 0);
	const char* artist = mpd_song_get_tag(song, MPD_TAG_ARTIST, 0);
	const char* album = mpd_song_get_tag(song, MPD_TAG_ALBUM, 0);
	if (title != NULL) {
		char* str[128];
		if (album == NULL) {
			album = "<none>";
		}
		malloc(sizeof(artist)+sizeof(album)+2);
		snprintf(str, "%s [%s]", artist, album);
		gtk_label_set_text(GTK_LABEL(title_text), title);
		gtk_label_set_text(GTK_LABEL(artist_text), (const char*)str);
	} else {
		gtk_label_set_text(GTK_LABEL(title_text), mpd_song_get_uri(song));
		gtk_label_set_text(GTK_LABEL(artist_text), "<Unknown Artist>");
	}
	mpd_song_free(song);
	update_play_bar();
}


void play_pause_button_click() {
	struct mpd_status* status = mpd_run_status(mpd);
	if (status == NULL) {
		handle_mpd_error();
	}
	enum mpd_state st = mpd_status_get_state((const struct mpd_status*)status);
	GtkImage* but_img = (GtkImage*)gtk_button_get_image(play_pause_button);
	mpd_status_free(status);
	if (st == MPD_STATE_PLAY) {
		gtk_image_set_from_icon_name(but_img, "media-playback-pause", GTK_ICON_SIZE_BUTTON);
	} else if (st == MPD_STATE_PAUSE) {
		gtk_image_set_from_icon_name(but_img, "media-playback-start", GTK_ICON_SIZE_BUTTON);
	}
	update_track_info();
}

static void fill_playlist(GtkWidget* list_box) {
	struct mpd_status* status = mpd_run_status(mpd);
	int len = mpd_status_get_queue_length(status);
	struct mpd_song* song;
	for (int i = len-1; i >= 0; i--) {
		song = mpd_run_get_queue_song_pos(mpd, i);
		g_assert(song != NULL);
		const char* title = mpd_song_get_tag(song, MPD_TAG_TITLE, 0);
		GtkWidget* song_button = gtk_button_new_with_label(title);
		gtk_button_set_image_position(GTK_BUTTON(song_button), i);
		struct mpd_pass* henlo_score = malloc(sizeof *henlo_score);
		henlo_score->mpd=mpd;
		henlo_score->v=i;
		g_signal_connect(song_button, "clicked", G_CALLBACK(mpd_play_song_pos), henlo_score);
		gtk_list_box_insert(GTK_LIST_BOX(list_box), song_button, 0);
		mpd_song_free(song);
	}
	mpd_status_free(status);
}

static void init_grid(GtkWindow* window) {
	GtkGrid* grid;
	GtkWidget* toggle_button;
	GtkWidget* prev_button;
	GtkWidget* next_button;
	GtkWidget* stop_button;
	GtkWidget* vol_bar;
	GtkWidget* vol_icon;
	GtkWidget* textview;
	GtkWidget* artistview;
	GtkWidget* list_box;
	GtkWidget* time_elapsed_bar;

	textview = gtk_label_new("<DEFAULT>"); //load view w/ buffer
	artistview = gtk_label_new("<DEFAULT>");
	title_text = textview;
	artist_text = artistview;


	GtkAdjustment* adjust;

	GtkWidget* scrolled_list = gtk_scrolled_window_new(gtk_adjustment_new(0, 0, 100, 0, 0, 0), gtk_adjustment_new(0, 0, 100, 0, 0, 0));
	list_box = gtk_list_box_new();
	fill_playlist(list_box);
	gtk_container_add(GTK_CONTAINER(scrolled_list), list_box);

	grid = GTK_GRID(gtk_grid_new());
	
	toggle_button = gtk_button_new_from_icon_name("media-playback-start", GTK_ICON_SIZE_BUTTON);
	prev_button = gtk_button_new_from_icon_name("media-skip-backward", GTK_ICON_SIZE_BUTTON);
	next_button = gtk_button_new_from_icon_name("media-skip-forward", GTK_ICON_SIZE_BUTTON);
	stop_button = gtk_button_new_from_icon_name("media-playback-stop", GTK_ICON_SIZE_BUTTON);

	GtkWidget* buttons[] = {toggle_button, prev_button, next_button, stop_button};
	for (int i = 0; i < G_N_ELEMENTS(buttons); i++) {
		gtk_widget_set_hexpand(buttons[i], true);
	}
	gtk_widget_set_hexpand(list_box, true);
	
	play_pause_button = GTK_BUTTON(toggle_button);
	
	vol_icon = gtk_image_new_from_icon_name("audio-volume-medium", GTK_ICON_SIZE_BUTTON);

	adjust = gtk_adjustment_new(0, 0, 100, 1, 0, 0);
	volume_bar=adjust;
	vol_bar = gtk_scale_new(GTK_ORIENTATION_HORIZONTAL,adjust);
	gtk_adjustment_set_value(adjust, mpd_get_vol(mpd));
	
	time_bar = gtk_adjustment_new(0, 0, 1, 1 ,0, 0);
	time_elapsed_bar = gtk_scale_new(GTK_ORIENTATION_HORIZONTAL, time_bar);

	for (int i = 0; i <= 100; i+=50) {
		gtk_scale_add_mark(GTK_SCALE(vol_bar), i, GTK_BASELINE_POSITION_CENTER, NULL);
	}

	//gtk_grid_set_row_spacing(grid, 3);
	gtk_grid_set_row_homogeneous(grid, true);
	gtk_grid_set_column_homogeneous(grid, true);

	gtk_grid_attach(grid, textview, 0, 1, 2, 1);
	gtk_grid_attach(grid, artistview, 0, 2, 2, 1);
	gtk_grid_attach(grid, gtk_label_new("Queue"), 3, 0, 2, 1);
	gtk_grid_attach(grid, scrolled_list, 3, 1, 2, 2);
	gtk_grid_attach(grid, vol_icon, 0, 3, 1, 1);
	gtk_grid_attach(grid, vol_bar, 1, 3, 2, 1);
	gtk_grid_attach(grid, prev_button, 0, 4, 1, 1);
	gtk_grid_attach(grid, toggle_button, 1, 4, 2, 1);
	gtk_grid_attach(grid, stop_button, 3, 4, 1, 1);
	gtk_grid_attach(grid, next_button, 4, 4, 1, 1);
	gtk_grid_attach(grid, time_elapsed_bar, 0, 5, 5, 1);
	
	g_signal_connect(toggle_button, "clicked", G_CALLBACK(mpd_toggle), mpd);
	g_signal_connect(stop_button, "clicked", G_CALLBACK(mpd_stop), mpd);
	g_signal_connect(next_button, "clicked", G_CALLBACK(mpd_next), mpd);
	g_signal_connect(prev_button, "clicked", G_CALLBACK(mpd_prev), mpd);
	g_signal_connect(adjust, "value-changed", G_CALLBACK(vol_change), NULL);

	gtk_widget_set_hexpand(GTK_WIDGET(grid), true);
	gtk_widget_set_vexpand(GTK_WIDGET(grid), true);

	gtk_container_add(GTK_CONTAINER(window), GTK_WIDGET(grid));

	update_track_info();
}

static void initialize_menu_bar(GtkApplication* app) {
	GtkBuilder* builder;
	GMenuModel* app_menu;
	GResource* resources;

	init_actions(app);
	
	builder = gtk_builder_new_from_resource("/byakuren/resources/menubar.xml");
	app_menu = G_MENU_MODEL(gtk_builder_get_object(builder, "menubar"));
	gtk_application_set_menubar(app, app_menu);
	g_object_unref(builder); //free memory used by builder
}

int handle_mpd_error() {
	if (mpd_connection_get_error(mpd) == MPD_ERROR_SUCCESS) {
		return 0;
	}

	printf((char*)mpd_connection_get_error_message(mpd));
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
