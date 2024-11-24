#include <gtk/gtk.h>

typedef struct {
    GtkWidget *username_entry;
    GtkWidget *password_entry;
    GtkWidget *confirm_password_entry;
    GtkWidget *error_label;
} FormData;

void apply_css(GtkWidget *widget, const char *css) {
    GtkCssProvider *provider = gtk_css_provider_new();
    gtk_css_provider_load_from_data(provider, css, -1);
    GtkStyleContext *context = gtk_widget_get_style_context(widget);
    gtk_style_context_add_provider(context, GTK_STYLE_PROVIDER(provider), GTK_STYLE_PROVIDER_PRIORITY_USER);
    g_object_unref(provider);
}

// Function to handle the submit button click
void on_submit_click(GtkWidget *widget, gpointer data) {
    FormData *form_data = (FormData *)data;

    const gchar *username = gtk_editable_get_text(GTK_EDITABLE(form_data->username_entry));
    const gchar *password = gtk_editable_get_text(GTK_EDITABLE(form_data->password_entry));
    const gchar *confirm_password = gtk_editable_get_text(GTK_EDITABLE(form_data->confirm_password_entry));

    gtk_label_set_text(GTK_LABEL(form_data->error_label), "");

    if (g_strcmp0(username, "") == 0 || g_strcmp0(password, "") == 0 || g_strcmp0(confirm_password, "") == 0) {
        gtk_label_set_text(GTK_LABEL(form_data->error_label), "All fields are required!");
        apply_css(form_data->error_label, "label { color: red; font-weight: bold; }");
        return;
    }

    if (g_strcmp0(password, confirm_password) != 0) {
        gtk_label_set_text(GTK_LABEL(form_data->error_label), "Passwords do not match!");
        apply_css(form_data->error_label, "label { color: red; font-weight: bold; }");
    } else {
        g_print("Username: %s\n", username);
        g_print("Password: %s\n", password);
    }
}


void activate(GtkApplication *app, gpointer user_data) {
    GtkWidget *window;
    GtkWidget *box; // Box to center the form
    GtkWidget *grid;
    GtkWidget *label;
    GtkWidget *username_label, *password_label, *confirm_password_label;
    GtkWidget *submit_button;

    // Create a new application window
    window = gtk_application_window_new(app);
    gtk_window_set_title(GTK_WINDOW(window), "Sign Up");
    gtk_window_set_default_size(GTK_WINDOW(window), 1400, 900);

    // Create a box to center the form
    box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_widget_set_halign(box, GTK_ALIGN_CENTER); // Center horizontally
    gtk_widget_set_valign(box, GTK_ALIGN_CENTER); // Center vertically
    gtk_window_set_child(GTK_WINDOW(window), box);

    // Create a grid for layout
    grid = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(grid), 10);
    gtk_grid_set_column_spacing(GTK_GRID(grid), 10);
    gtk_widget_set_margin_top(grid, 20);
    gtk_widget_set_margin_bottom(grid, 20);
    gtk_widget_set_margin_start(grid, 20);
    gtk_widget_set_margin_end(grid, 20);
    gtk_box_append(GTK_BOX(box), grid); // Add grid to the center box

    // Create the title label
    label = gtk_label_new("Sign Up");
    gtk_widget_set_halign(label, GTK_ALIGN_CENTER);
    gtk_widget_set_margin_bottom(label, 20);
    gtk_grid_attach(GTK_GRID(grid), label, 0, 0, 2, 1);

    // Create the error label
    GtkWidget *error_label = gtk_label_new("");
    gtk_widget_set_halign(error_label, GTK_ALIGN_START);
    gtk_widget_set_margin_bottom(error_label, 10);
    gtk_label_set_xalign(GTK_LABEL(error_label), 0);
    gtk_widget_set_name(error_label, "error-label");
    gtk_grid_attach(GTK_GRID(grid), error_label, 0, 1, 2, 1);

    // Create the username input
    username_label = gtk_label_new("User Name:");
    gtk_widget_set_halign(username_label, GTK_ALIGN_START);
    gtk_grid_attach(GTK_GRID(grid), username_label, 0, 2, 1, 1);

    GtkWidget *username_entry = gtk_entry_new();
    gtk_widget_set_hexpand(username_entry, TRUE);
    gtk_grid_attach(GTK_GRID(grid), username_entry, 1, 2, 1, 1);

    // Create the password input
    password_label = gtk_label_new("Password:");
    gtk_widget_set_halign(password_label, GTK_ALIGN_START);
    gtk_grid_attach(GTK_GRID(grid), password_label, 0, 3, 1, 1);

    GtkWidget *password_entry = gtk_entry_new();
    gtk_entry_set_visibility(GTK_ENTRY(password_entry), FALSE);
    gtk_widget_set_hexpand(password_entry, TRUE);
    gtk_grid_attach(GTK_GRID(grid), password_entry, 1, 3, 1, 1);

    // Create the confirm password input
    confirm_password_label = gtk_label_new("Confirm Password:");
    gtk_widget_set_halign(confirm_password_label, GTK_ALIGN_START);
    gtk_grid_attach(GTK_GRID(grid), confirm_password_label, 0, 4, 1, 1);

    GtkWidget *confirm_password_entry = gtk_entry_new();
    gtk_entry_set_visibility(GTK_ENTRY(confirm_password_entry), FALSE);
    gtk_widget_set_hexpand(confirm_password_entry, TRUE);
    gtk_grid_attach(GTK_GRID(grid), confirm_password_entry, 1, 4, 1, 1);

    // Create the submit button
    submit_button = gtk_button_new_with_label("Submit");
    gtk_widget_set_halign(submit_button, GTK_ALIGN_CENTER);
    gtk_widget_set_margin_top(submit_button, 20);
    gtk_grid_attach(GTK_GRID(grid), submit_button, 0, 5, 2, 1);

    // Form data to pass to the callback
    FormData *form_data = g_new0(FormData, 1);
    form_data->username_entry = username_entry;
    form_data->password_entry = password_entry;
    form_data->confirm_password_entry = confirm_password_entry;
    form_data->error_label = error_label;

    // Connect the submit button to the callback
    g_signal_connect(submit_button, "clicked", G_CALLBACK(on_submit_click), form_data);

    // Show the window
    gtk_widget_show(window);
}


int main(int argc, char **argv) {
    GtkApplication *app;
    int status;

    app = gtk_application_new("com.example.signup", G_APPLICATION_FLAGS_NONE);
    g_signal_connect(app, "activate", G_CALLBACK(activate), NULL);

    status = g_application_run(G_APPLICATION(app), argc, argv);
    g_object_unref(app);

    return status;
}
