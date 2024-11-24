#include <gtk/gtk.h>

// Forward declarations for future data handling functions
void on_letter_input(GtkWidget *widget, gpointer data);
void on_submit_click(GtkWidget *widget, gpointer data);

void activate(GtkApplication *app, gpointer user_data) {
    GtkWidget *window;
    GtkWidget *grid;
    GtkWidget *frame;
    GtkWidget *label;
    GtkWidget *vbox;
    GtkWidget *hbox_main;
    GtkWidget *submit_box;

    // Create a new application window
    window = gtk_application_window_new(app);
    gtk_window_set_title(GTK_WINDOW(window), "Wordle Game GUI");
    gtk_window_set_default_size(GTK_WINDOW(window), 1400, 900);
    gtk_window_set_resizable(GTK_WINDOW(window), FALSE);

    // Create a vertical box to hold everything
    vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 20);
    gtk_widget_set_margin_top(vbox, 20);
    gtk_widget_set_margin_bottom(vbox, 20);
    gtk_widget_set_margin_start(vbox, 20);
    gtk_widget_set_margin_end(vbox, 20);
    gtk_window_set_child(GTK_WINDOW(window), vbox);

    // Create a horizontal box to hold the two grids side by side
    hbox_main = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 50);
    gtk_box_append(GTK_BOX(vbox), hbox_main);

    // Create vertical box for "You"
    GtkWidget *vbox_you = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_box_append(GTK_BOX(hbox_main), vbox_you);

    // Create grid for "You"
    label = gtk_label_new("YOU");
    gtk_box_append(GTK_BOX(vbox_you), label);

    grid = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(grid), 5);
    gtk_grid_set_column_spacing(GTK_GRID(grid), 5);
    gtk_box_append(GTK_BOX(vbox_you), grid);

    // Create the 6x5 grid for "YOU"
    for (int i = 0; i < 6; i++) {
        for (int j = 0; j < 5; j++) {
            frame = gtk_frame_new(NULL);
            gtk_widget_set_size_request(frame, 80, 80);
            gtk_grid_attach(GTK_GRID(grid), frame, j, i, 1, 1);
        }
    }

    // Create vertical box for "Opponent"
    GtkWidget *vbox_opponent = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_widget_set_halign(vbox_opponent, GTK_ALIGN_CENTER);
    gtk_widget_set_margin_start(vbox_opponent, 150);

    gtk_box_append(GTK_BOX(hbox_main), vbox_opponent);

    // Create grid for "Opponent"
    label = gtk_label_new("OPPONENT");
    gtk_box_append(GTK_BOX(vbox_opponent), label);

    grid = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(grid), 5);
    gtk_grid_set_column_spacing(GTK_GRID(grid), 5);
    gtk_box_append(GTK_BOX(vbox_opponent), grid);

    // Create the 6x5 grid for "OPPONENT"
    for (int i = 0; i < 6; i++) {
        for (int j = 0; j < 5; j++) {
            frame = gtk_frame_new(NULL);
            gtk_widget_set_size_request(frame, 80, 80);
            gtk_grid_attach(GTK_GRID(grid), frame, j, i, 1, 1);
        }
    }

    // Create an area for user input below the two grids
    submit_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 20);
    gtk_widget_set_halign(submit_box, GTK_ALIGN_START);
    gtk_widget_set_margin_start(submit_box, 20);
    gtk_widget_set_margin_top(submit_box, 20);

    gtk_widget_set_margin_top(submit_box, 50);
gtk_box_append(GTK_BOX(vbox), submit_box);

    // Placeholder for input field (future implementation)
    GtkWidget *entry = gtk_entry_new();
gtk_widget_set_size_request(entry, 200, 40);
    gtk_widget_set_margin_start(entry, 20);
    gtk_widget_set_margin_end(entry, 20);
    gtk_box_append(GTK_BOX(submit_box), entry);

    // Placeholder for submit button (future implementation)
    GtkWidget *submit_button = gtk_button_new_with_label("Submit");
    g_signal_connect(submit_button, "clicked", G_CALLBACK(on_submit_click), entry);
    gtk_box_append(GTK_BOX(submit_box), submit_button);

    // Show the window
    gtk_widget_show(window);
}

int main(int argc, char **argv) {
    GtkApplication *app;
    int status;

    // Create a new GtkApplication
    app = gtk_application_new("com.example.wordlegame", G_APPLICATION_FLAGS_NONE);
    g_signal_connect(app, "activate", G_CALLBACK(activate), NULL);

    // Run the application
    status = g_application_run(G_APPLICATION(app), argc, argv);

    // Clean up
    g_object_unref(app);

    return status;
}

// Placeholder for letter input handler (to be implemented)
void on_letter_input(GtkWidget *widget, gpointer data) {
    // TODO: Handle user letter input
}

// Placeholder for submit button click handler (to be implemented)
void on_submit_click(GtkWidget *widget, gpointer data) {
    // TODO: Handle submit button click
    // Entry data can be accessed using gtk_entry_get_text(GTK_ENTRY(data));
}
