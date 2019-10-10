#include <mpd/client.h>
#include <gtk/gtk.h>

#include "song_search_dialog.h"
#include "main.h"
#include "mpd_actions.h"

void clear_list_box(GtkListBox* listbox) {
	GList *children, *iter; 
	children = gtk_container_get_children(GTK_CONTAINER(listbox));
	for (iter=children; iter != NULL; iter = g_list_next(iter)) {
		gtk_widget_destroy(GTK_WIDGET(iter->data));
	}
	g_list_free(children);
}

static void populate_list_box(GtkListBox* listbox, struct mpd_connection* mpd, const char* search_term) {
    mpd_command_list_begin(mpd, false);
    mpd_search_add_tag_constraint(mpd, MPD_OPERATOR_DEFAULT, MPD_TAG_TITLE, search_term);
    mpd_search_commit(mpd);
    mpd_command_list_end(mpd);
    clear_list_box(listbox);

    struct mpd_song* s;
    while (true) {
        s = mpd_recv_song(mpd);
        if (s==NULL) break;
        GtkGrid* grid = GTK_GRID(gtk_grid_new());
        char str[8];
        snprintf(str, 8, "%d", mpd_song_get_id(s));
        gtk_grid_attach(grid, gtk_label_new(str), 0, 0, 1, 1);
        gtk_grid_attach(grid, gtk_label_new(mpd_song_get_tag(s, MPD_TAG_TITLE, 0)), 0, 1, 3, 1);
        gtk_list_box_insert(listbox, GTK_WIDGET(grid), 0);
    }
}

struct mpd_song* get_song_with_song_dialog(GtkWindow* parent, struct mpd_connection* mpd) {
    GtkDialog* dialog;
    GtkDialogFlags flags = GTK_DIALOG_DESTROY_WITH_PARENT;
    GtkGrid* grid;
    GtkListBox* listbox;
    GtkWidget* search;

    dialog = GTK_DIALOG(gtk_dialog_new_with_buttons("Choose a song", parent, flags, "_OK", GTK_RESPONSE_NONE, NULL));
    grid = GTK_GRID(gtk_grid_new());
    listbox = GTK_LIST_BOX(gtk_list_box_new());
    search = gtk_search_entry_new();
    gtk_grid_attach(grid, GTK_WIDGET(search), 0, 0, 1, 1);
    gtk_grid_attach(grid, GTK_WIDGET(listbox), 0, 1, 1, 1);
    populate_list_box(listbox, mpd, "");

    g_signal_connect(dialog, "response", G_CALLBACK(NULL), NULL); // handle user response

    gtk_container_add(GTK_CONTAINER(gtk_dialog_get_content_area(dialog)), GTK_WIDGET(grid));
    gtk_widget_show_all(GTK_WIDGET(listbox));
    gtk_dialog_run(dialog);

    GtkListBoxRow* selected = gtk_list_box_get_selected_row(listbox);
	GtkWidget* child = gtk_bin_get_child(GTK_BIN(selected));
	gtk_widget_destroy(GTK_WIDGET(dialog));
}