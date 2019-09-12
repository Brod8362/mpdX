#include <gtk/gtk.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <argp.h>

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

int main(int argc, char *argv[]) {
	int opt;
	while ((opt= getopt(argc, argv, ":if:lrx")) != -1) {
		switch (opt) {
			case 'd':
				debug=true;
				debug_log("running in debug mode");
		}	
	}
	GtkApplication* app;
	int status;
	app = gtk_application_new("pw.byakuren.mpdX", G_APPLICATION_FLAGS_NONE);
	g_signal_connect(app, "activate", G_CALLBACK(activate), NULL);
	status = g_application_run(G_APPLICATION(app), argc, argv);
	g_object_unref(app);

	return status;
}
