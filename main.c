#include <gtk/gtk.h>
#include <stdio.h>
static void activate(GtkApplication* app, gpointer data) {
	GtkWidget* window;
	printf("Initializing mpdX\n");
	
	window = gtk_application_window_new(app);
	gtk_window_set_title(GTK_WINDOW(window), "mpdX");
	gtk_window_set_default_size(GTK_WINDOW(window), 825, 400);

	gtk_widget_show_all(window);
}

int main(int argc, char *argv[]) {

	GtkApplication* app;
	int status;
	app = gtk_application_new("pw.byakuren.mpdX", G_APPLICATION_FLAGS_NONE);
	g_signal_connect(app, "activate", G_CALLBACK(activate), NULL);
	status = g_application_run(G_APPLICATION(app), argc, argv);
	g_object_unref(app);

	return status;
}
