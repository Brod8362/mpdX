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

static void add_song_to_queue(GtkButton* button, gpointer ptr) {
    struct play_container* v = (struct play_container*)ptr;
    mpd_run_add(v->mpd, v->uri);
}

static void populate_list_box(GtkListBox* listbox, struct mpd_connection* mpd, const char* search_term) {
    clear_list_box(listbox);
    handle_mpd_error();
    mpd_command_list_begin(mpd, false);
    mpd_search_db_songs(mpd, false);
    mpd_search_add_tag_constraint(mpd, MPD_OPERATOR_DEFAULT, MPD_TAG_TITLE, search_term);
    mpd_search_commit(mpd);
    mpd_command_list_end(mpd);

    struct mpd_song* s;
    while (true) {
        s = mpd_recv_song(mpd);
        if (s==NULL) break;
        GtkGrid* grid = GTK_GRID(gtk_grid_new());
        GtkButton* button = GTK_BUTTON(gtk_button_new_from_icon_name("list-add", GTK_ICON_SIZE_BUTTON));
        const char* title = mpd_song_get_tag(s, MPD_TAG_TITLE, 0);
        if (title == NULL || strcmp(title, "")==0) {
            title = mpd_song_get_uri(s);
        }
        GtkWidget* label = gtk_label_new(mpd_song_get_tag(s, MPD_TAG_TITLE, 0));
        //gtk_widget_set_hexpand(label, true);
        gtk_grid_attach(grid, label, 1, 0, 1, 1);
        gtk_grid_attach(grid, GTK_WIDGET(button), 0, 0, 1, 1);
        struct play_container* v = malloc(sizeof *v);
        v->mpd=mpd;
        v->uri=mpd_song_get_uri(s);
        g_signal_connect(button, "clicked", G_CALLBACK(add_song_to_queue), v);
        gtk_list_box_insert(listbox, GTK_WIDGET(grid), 0);
    }
    gtk_widget_show_all(GTK_WIDGET(listbox));
}

static void search_changed(GtkSearchEntry* entry, gpointer ptr) {
    struct search_container* cs = (struct search_container*)ptr;
    populate_list_box(cs->listbox, cs->mpd, gtk_entry_get_text(GTK_ENTRY(entry)));
}

void queue_song_with_song_dialog(GtkWindow* parent, struct mpd_connection* mpd) {
    GtkDialog* dialog;
    GtkDialogFlags flags = GTK_DIALOG_DESTROY_WITH_PARENT;
    GtkGrid* grid;
    GtkListBox* listbox;
    GtkWidget* search;
    GtkWidget* scrolled;

    dialog = GTK_DIALOG(gtk_dialog_new_with_buttons("Choose a song", parent, flags,"_Close", GTK_RESPONSE_NONE, NULL));
    gtk_window_set_default_size(GTK_WINDOW(dialog), 450, 200);
    grid = GTK_GRID(gtk_grid_new());
    listbox = GTK_LIST_BOX(gtk_list_box_new());
    scrolled = gtk_scrolled_window_new(gtk_adjustment_new(0, 0, 100, 0, 0, 0), gtk_adjustment_new(0, 0, 100, 0, 0, 0));
    search = gtk_search_entry_new();

    gtk_container_add(GTK_CONTAINER(scrolled), GTK_WIDGET(listbox));

    gtk_widget_set_vexpand(GTK_WIDGET(grid), true);
    gtk_widget_set_hexpand(GTK_WIDGET(grid), true);
    gtk_grid_set_column_homogeneous(grid, true);
    gtk_grid_set_row_homogeneous(grid, true);
    
    gtk_grid_attach(grid, GTK_WIDGET(search), 0, 0, 3, 1);
    gtk_grid_attach(grid, scrolled, 0, 1, 3, 6);

    struct search_container* cs = malloc(sizeof *cs);
    cs->listbox=listbox;
    cs->mpd=mpd;

    g_signal_connect(dialog, "response", G_CALLBACK(gtk_widget_destroy), NULL); // handle user response
    g_signal_connect(search, "search-changed", G_CALLBACK(search_changed), cs);

    gtk_container_add(GTK_CONTAINER(gtk_dialog_get_content_area(dialog)), GTK_WIDGET(grid));
    gtk_widget_show_all(GTK_WIDGET(grid));
    gtk_dialog_run(dialog);
}