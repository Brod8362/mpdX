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
#include <threads.h>
#include <mpd/playlist.h>

#include "mpd_actions.h"
#include "actions.h"
#include "song_search_dialog.h"

char* mpd_conn_dest = "localhost";
int mpd_port = 6600;
int mpd_timeout = 10000;

struct mpd_connection *mpd;
GtkApplication* app;
GtkWindow* main_window;

GtkWidget* title_text; //text buffer for songname
GtkWidget* artist_text;

GtkButton* play_pause_button;
GtkAdjustment* volume_bar;
GtkAdjustment* time_bar;

GtkToggleButton* repeat_but;
GtkToggleButton* single_but;
GtkToggleButton* shuffle_but;

GtkListBox* listbox;

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

void display_non_fatal_error(const char* message) {
	GtkWidget* dialog;
	GtkDialogFlags flags = GTK_DIALOG_DESTROY_WITH_PARENT;
	dialog = gtk_message_dialog_new(main_window, flags, GTK_MESSAGE_WARNING, GTK_BUTTONS_CLOSE, "A non-fatal error has occured. Reason:\n%s", message);
	int resp = gtk_dialog_run(GTK_DIALOG(dialog));
	
	if (resp == GTK_RESPONSE_CLOSE) {
		gtk_widget_destroy(dialog);
	}
}

void display_fatal_error(const char* message) {
	GtkWidget* dialog;
	GtkDialogFlags flags = GTK_DIALOG_DESTROY_WITH_PARENT;
	dialog = gtk_message_dialog_new(main_window, flags, GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE, "A fatal error has occured. The application will now close. Reason:\n%s", message);
	gtk_dialog_run(GTK_DIALOG(dialog));
	g_signal_connect_swapped(dialog, "response", G_CALLBACK(g_object_unref), app);
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
	if (song == NULL) return;
	struct mpd_status* status = mpd_run_status(mpd);
	if (status == NULL) return;
	
	int length = mpd_song_get_duration(song);
	int elapsed = mpd_status_get_elapsed_time(status);

	gtk_adjustment_set_value(time_bar, elapsed);
	gtk_adjustment_set_upper(time_bar, length);

	mpd_song_free(song);
	mpd_status_free(status);
}

int play_bar_thread(void* thr_data) {
	while (true) {
		sleep(1);
		update_play_bar();
		play_pause_button_click();
	}
}

void update_track_info() {
	struct mpd_song* song = mpd_run_current_song(mpd);
	if (song == NULL) {
		display_non_fatal_error("song is null in update_track_info()");
		return;
	}
	const char* title = mpd_song_get_tag(song, MPD_TAG_TITLE, 0);
	const char* artist = mpd_song_get_tag(song, MPD_TAG_ARTIST, 0);
	const char* album = mpd_song_get_tag(song, MPD_TAG_ALBUM, 0);
	if (title != NULL) {
		if (album == NULL) {
			album = "<none>";
		}
		size_t str_max = strlen(artist) + strlen(album) + 4;
		char* str = malloc(str_max);
		snprintf(str, str_max, "%s [%s]", artist, album);
		gtk_label_set_text(GTK_LABEL(title_text), title);
		gtk_label_set_text(GTK_LABEL(artist_text), (const char*)str);
		free(str);
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
		return;
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

static void destroy_button_group(GtkButton* button, gpointer ptr) {
	struct queue_button_group* group = (struct queue_button_group*)ptr;
	mpd_unqueue_song_id(group->pass);
	gtk_grid_remove_row(GTK_GRID(group->container), 0);
	gtk_widget_destroy(group->container);
	free(group);
}

void fill_playlist() {
	struct mpd_status* status = mpd_run_status(mpd);
	int len = mpd_status_get_queue_length(status);
	struct mpd_song* song;
	for (int i = len-1; i >= 0; i--) {
		song = mpd_run_get_queue_song_pos(mpd, i);
		g_assert(song != NULL);
		unsigned int id = mpd_song_get_id(song);
		const char* title = mpd_song_get_tag(song, MPD_TAG_TITLE, 0);
		if (title == NULL || strcmp(title, "") == 0) {
			title = mpd_song_get_uri(song);
		}
		GtkWidget* grid = gtk_grid_new();
		GtkWidget* song_button = gtk_button_new_with_label(title);
		GtkWidget* remove_button = gtk_button_new_from_icon_name("list-remove", GTK_ICON_SIZE_BUTTON);
		gtk_grid_attach(GTK_GRID(grid), song_button, 0, 1, 3, 1);
		gtk_grid_attach(GTK_GRID(grid), remove_button, 3, 1, 1, 1);
		gtk_widget_set_hexpand(song_button, true);
		struct mpd_pass* henlo_score = malloc(sizeof *henlo_score);
		henlo_score->mpd=mpd;
		henlo_score->v=id;

		struct queue_button_group* group = malloc(sizeof *group);
		group->container=grid;
		group->remove=remove_button;
		group->song=song_button;
		group->pass=henlo_score;
		g_signal_connect(song_button, "clicked", G_CALLBACK(mpd_play_song_id_button), henlo_score);
		g_signal_connect(remove_button, "clicked", G_CALLBACK(destroy_button_group), group);
		gtk_list_box_insert(GTK_LIST_BOX(listbox), grid, 0);
		mpd_song_free(song);
	}
	mpd_status_free(status);
}

void clear_playlist() {
	GList *children, *iter; 
	children = gtk_container_get_children(GTK_CONTAINER(listbox));
	for (iter=children; iter != NULL; iter = g_list_next(iter)) {
		gtk_widget_destroy(GTK_WIDGET(iter->data));
	}
	g_list_free(children);
}

static void save_playlist_dialog(GtkButton* button, GtkWindow* parent) {
	GtkDialog* dialog;
	GtkDialogFlags flags = GTK_DIALOG_DESTROY_WITH_PARENT;
	GtkTextBuffer* buf;
	GtkWidget* view;
	GtkWidget* label;
	GtkGrid* grid;

	GtkTextIter start, end;
	dialog = GTK_DIALOG(gtk_dialog_new_with_buttons("Playlist name", parent, flags, "_OK", GTK_RESPONSE_NONE, NULL));
	buf = gtk_text_buffer_new(NULL);
	view = gtk_text_view_new_with_buffer(buf);
	label = gtk_label_new("Playlist name:");
	grid = GTK_GRID(gtk_grid_new());

	gtk_grid_attach(grid, label, 0, 0, 1, 1);
	gtk_grid_attach(grid, view, 0, 1, 1, 1);
	gtk_widget_show_all(GTK_WIDGET(grid));
	gtk_container_add(GTK_CONTAINER(gtk_dialog_get_content_area(dialog)), GTK_WIDGET(grid));

	g_signal_connect_swapped(dialog, "response", G_CALLBACK(gtk_widget_destroy), dialog);
	gtk_dialog_run(dialog);

	gtk_text_buffer_get_bounds(buf, &start, &end);
	mpd_save_playlist(mpd, gtk_text_buffer_get_text(buf, &start, &end, false));
}

static void load_playlist_dialog(GtkButton* button, GtkWindow* parent) {
	GtkDialog* dialog;
	GtkDialogFlags flags = GTK_DIALOG_DESTROY_WITH_PARENT;
	GtkWidget* list_box;

	dialog = GTK_DIALOG(gtk_dialog_new_with_buttons("Load playlist", parent, flags, "_Cancel", GTK_RESPONSE_NONE, "_OK", GTK_RESPONSE_OK, NULL));
	list_box = gtk_list_box_new();

	mpd_command_list_begin(mpd, false);
	mpd_send_list_playlists(mpd);
	mpd_command_list_end(mpd);
	struct mpd_playlist* pl;
	int i = 0;
	while (true) {
		pl = mpd_recv_playlist(mpd);
		if (pl == NULL) break;
		gtk_list_box_insert(GTK_LIST_BOX(list_box), gtk_label_new(mpd_playlist_get_path(pl)), i++);
	}
	if (i == 0) {
		display_non_fatal_error("No playlists available.");
		return;
	}
	gtk_container_add(GTK_CONTAINER(gtk_dialog_get_content_area(dialog)), list_box);
	gtk_widget_show_all(list_box);
	g_signal_connect(dialog, "response", G_CALLBACK(NULL), NULL);
	int resp = gtk_dialog_run(dialog);

	if (resp != GTK_RESPONSE_OK) {
		gtk_widget_destroy(GTK_WIDGET(dialog));
		return;
	}

	GtkListBoxRow* selected = gtk_list_box_get_selected_row(GTK_LIST_BOX(list_box));
	GtkWidget* lbl = gtk_bin_get_child(GTK_BIN(selected));
	mpd_load_playlist(mpd, gtk_label_get_text(GTK_LABEL(lbl)));
	gtk_widget_destroy(GTK_WIDGET(dialog));
}

static void add_track_dialog(GtkButton* button, GtkWindow* parent) {
	queue_song_with_song_dialog(parent, mpd);
}

static void init_playlist_controls(GtkGrid* grid) {
	GtkWidget* add_track;
	GtkWidget* clear;
	GtkWidget* save_playlist;
	GtkWidget* load_playlist;

	add_track = gtk_button_new_from_icon_name("list-add", GTK_ICON_SIZE_BUTTON);
	clear = gtk_button_new_from_icon_name("document-revert", GTK_ICON_SIZE_BUTTON);
	save_playlist = gtk_button_new_from_icon_name("document-save", GTK_ICON_SIZE_BUTTON);
	load_playlist = gtk_button_new_from_icon_name("document-open", GTK_ICON_SIZE_BUTTON);

	gtk_widget_set_tooltip_text(add_track, "Add Track");
	gtk_widget_set_tooltip_text(clear, "Clear Queue");
	gtk_widget_set_tooltip_text(save_playlist, "Save as new Playlist");
	gtk_widget_set_tooltip_text(load_playlist, "Load existing playlist");

	g_signal_connect(clear, "clicked", G_CALLBACK(mpd_clear_queue_button), mpd);
	g_signal_connect(save_playlist, "clicked", G_CALLBACK(save_playlist_dialog), gtk_widget_get_parent_window(GTK_WIDGET(grid)));
	g_signal_connect(load_playlist, "clicked", G_CALLBACK(load_playlist_dialog), gtk_widget_get_parent_window(GTK_WIDGET(grid)));
	g_signal_connect(add_track, "clicked", G_CALLBACK(add_track_dialog), gtk_widget_get_parent_window(GTK_WIDGET(grid)));

	gtk_grid_attach(grid, add_track, 0, 1, 1, 1);
	gtk_grid_attach(grid, clear, 1, 1, 1, 1);
	gtk_grid_attach(grid, save_playlist, 2, 1, 1, 1);
	gtk_grid_attach(grid, load_playlist, 3, 1, 1, 1);

	gtk_grid_set_column_homogeneous(grid, true);
}

static void init_mini_grid(GtkGrid* grid) {
	GtkWidget* repeat_button;
	GtkWidget* shuffle_button;
	GtkWidget* single_button;
	repeat_button = gtk_toggle_button_new_with_label("REP");
	shuffle_button = gtk_toggle_button_new_with_label("RAND");
	single_button = gtk_toggle_button_new_with_label("1");

	repeat_but = GTK_TOGGLE_BUTTON(repeat_button);
	single_but = GTK_TOGGLE_BUTTON(single_button);
	shuffle_but = GTK_TOGGLE_BUTTON(shuffle_button);

	g_signal_connect(repeat_but, "clicked", G_CALLBACK(mpd_toggle_repeat_button), mpd);
	g_signal_connect(shuffle_but, "clicked", G_CALLBACK(mpd_toggle_random_button), mpd);
	g_signal_connect(single_but, "clicked", G_CALLBACK(mpd_toggle_single_button), mpd);

	gtk_toggle_button_set_mode(repeat_but, false);
	gtk_toggle_button_set_mode(single_but, false);
	gtk_toggle_button_set_mode(shuffle_but, false);

	mpd_set_repeat(mpd, false);
	mpd_set_single(mpd, false);
	mpd_set_random(mpd, false);

	gtk_grid_attach(grid, repeat_button, 0,1,1,1);
	gtk_grid_attach(grid, shuffle_button, 1,1,1,1);
	gtk_grid_attach(grid, single_button, 2,1,1,1);

	gtk_grid_set_column_homogeneous(grid, true);
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
	GtkGrid* mini_control_grid;
	GtkGrid* playlist_control_grid;
	GtkGrid* info_grid;
	GtkWidget* frame;
	GtkWidget* queueframe;
	

	textview = gtk_label_new("<DEFAULT>"); //load view w/ buffer
	artistview = gtk_label_new("<DEFAULT>");
	title_text = textview;
	artist_text = artistview;


	GtkAdjustment* adjust;

	GtkWidget* scrolled_list = gtk_scrolled_window_new(gtk_adjustment_new(0, 0, 100, 0, 0, 0), gtk_adjustment_new(0, 0, 100, 0, 0, 0));
	list_box = gtk_list_box_new();
	listbox = GTK_LIST_BOX(list_box);
	fill_playlist();
	gtk_container_add(GTK_CONTAINER(scrolled_list), list_box);

	grid = GTK_GRID(gtk_grid_new());

	mini_control_grid = GTK_GRID(gtk_grid_new());
	init_mini_grid(mini_control_grid);

	playlist_control_grid = GTK_GRID(gtk_grid_new());
	init_playlist_controls(playlist_control_grid);

	frame = gtk_frame_new("Song Info");
	info_grid = GTK_GRID(gtk_grid_new());
	gtk_widget_set_vexpand(GTK_WIDGET(info_grid), true);
	gtk_widget_set_hexpand(GTK_WIDGET(info_grid), true);
	gtk_grid_set_column_homogeneous(info_grid, true);
	gtk_grid_set_row_homogeneous(info_grid, true);
	gtk_grid_attach(info_grid, textview, 0, 0, 1, 1);
	gtk_grid_attach(info_grid, artistview, 0, 1, 1, 1);
	gtk_container_add(GTK_CONTAINER(frame), GTK_WIDGET(info_grid));

	queueframe = gtk_frame_new("Queue");
	gtk_container_add(GTK_CONTAINER(queueframe), GTK_WIDGET(scrolled_list));

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
	gtk_grid_set_column_homogeneous(grid, true);

	gtk_grid_attach(grid, frame, 0, 1, 3, 2);
	gtk_grid_attach(grid, queueframe, 3, 1, 2, 2);
	gtk_grid_attach(grid, vol_icon, 0, 3, 1, 1);
	gtk_grid_attach(grid, vol_bar, 1, 3, 2, 1);
	gtk_grid_attach(grid, prev_button, 0, 4, 1, 1);
	gtk_grid_attach(grid, toggle_button, 1, 4, 2, 1);
	gtk_grid_attach(grid, stop_button, 3, 4, 1, 1);
	gtk_grid_attach(grid, next_button, 4, 4, 1, 1);
	gtk_grid_attach(grid, GTK_WIDGET(playlist_control_grid), 3, 3, 1, 1);
	gtk_grid_attach(grid, GTK_WIDGET(mini_control_grid), 4, 3, 1, 1);
	gtk_grid_attach(grid, time_elapsed_bar, 0, 5, 5, 1);
	
	g_signal_connect(toggle_button, "clicked", G_CALLBACK(mpd_toggle_button), mpd);
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
	enum mpd_error err = mpd_connection_get_error(mpd);
	if (err == MPD_ERROR_SUCCESS) {
		return 0;
	}
	display_fatal_error(mpd_connection_get_error_message(mpd));
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
	main_window = GTK_WINDOW(window);
	init_grid(GTK_WINDOW(window));
	gtk_widget_show_all(window);

	thrd_t thr;
	thrd_create(&thr, play_bar_thread, NULL);
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
	int status;
	app = gtk_application_new("pw.byakuren.mpdX", G_APPLICATION_FLAGS_NONE);
	/* init actions */
	init_mpd_actions(app, mpd);
	g_signal_connect(app, "activate", G_CALLBACK(activate), NULL);
	status = g_application_run(G_APPLICATION(app), argc, argv);
	g_object_unref(app);

	return status;
}
