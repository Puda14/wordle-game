#include <gtk/gtk.h>

// Forward declaration of on_activate function
void on_activate(GtkApplication *app);

int main(int argc, char** argv) {
    // Create a new GtkApplication
    GtkApplication *app = gtk_application_new("com.example.myapp", G_APPLICATION_FLAGS_NONE);

    // Connect the "activate" signal to the on_activate callback function
    g_signal_connect(app, "activate", G_CALLBACK(on_activate), NULL);

    // Run the application
    int status = g_application_run(G_APPLICATION(app), argc, argv);

    // Free application memory
    g_object_unref(app);

    return status;
}

// Definition of the on_activate function
void on_activate(GtkApplication *app) {
    GtkWidget *window = gtk_application_window_new(app);
    gtk_window_set_title(GTK_WINDOW(window), "My GTK App");
    gtk_window_set_default_size(GTK_WINDOW(window), 800, 600);

    // Show the window
    gtk_widget_show(window);
}
