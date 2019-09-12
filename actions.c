#include "actions.h"

static int quit_action(GSimpleAction* action, GVariant* parameter, gpointer ap) {
    printf("quit\n");
	g_object_unref((GtkApplication*)ap);
	return 0;
}


void init_actions(GtkApplication* app) {

    g_assert(app != NULL);

    GSimpleAction* quit_action = g_simple_action_new("quit", NULL);
    g_signal_connect(quit_action, "activate", G_CALLBACK(quit_action), app);
    g_action_map_add_action(G_ACTION_MAP(app), G_ACTION(quit_action));
    
}