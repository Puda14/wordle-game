#include <gtk/gtk.h>

// Function to handle "Sign Up" button click
void on_signup_button_clicked(GtkWidget *widget, gpointer stack) {
    gtk_stack_set_visible_child_name(GTK_STACK(stack), "signup_page");
}

// Function to handle "Login" button click
void on_login_button_clicked(GtkWidget *widget, gpointer stack) {
    gtk_stack_set_visible_child_name(GTK_STACK(stack), "login_page");
}

void activate(GtkApplication *app, gpointer user_data) {
    GtkWidget *window;
    GtkWidget *vbox;
    GtkWidget *stack;
    GtkWidget *stack_switcher;
    GtkWidget *signup_page, *login_page;

    // Create the main application window
    window = gtk_application_window_new(app);
    gtk_window_set_title(GTK_WINDOW(window), "Page Navigation Example");
    gtk_window_set_default_size(GTK_WINDOW(window), 600, 400);

    // Create a vertical box to hold the stack switcher and stack
    vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_window_set_child(GTK_WINDOW(window), vbox);

    // Create a GtkStack for switching pages
    stack = gtk_stack_new();
    gtk_stack_set_transition_type(GTK_STACK(stack), GTK_STACK_TRANSITION_TYPE_SLIDE_LEFT_RIGHT);
    gtk_stack_set_transition_duration(GTK_STACK(stack), 300);

    // Create a GtkStackSwitcher to navigate between pages
    stack_switcher = gtk_stack_switcher_new();
    gtk_stack_switcher_set_stack(GTK_STACK_SWITCHER(stack_switcher), GTK_STACK(stack));
    gtk_box_append(GTK_BOX(vbox), stack_switcher);

    // Create the "Sign Up" page
    signup_page = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_widget_set_margin_top(signup_page, 20);
    gtk_widget_set_margin_start(signup_page, 20);
    gtk_widget_set_margin_end(signup_page, 20);
    gtk_widget_set_margin_bottom(signup_page, 20);

    GtkWidget *signup_label = gtk_label_new("Welcome to the Sign Up Page");
    gtk_box_append(GTK_BOX(signup_page), signup_label);

    GtkWidget *goto_login_button = gtk_button_new_with_label("Go to Login");
    gtk_box_append(GTK_BOX(signup_page), goto_login_button);

    // Connect button to switch to login page
    g_signal_connect(goto_login_button, "clicked", G_CALLBACK(on_login_button_clicked), stack);

    // Add "Sign Up" page to stack
    gtk_stack_add_named(GTK_STACK(stack), signup_page, "signup_page");

    // Create the "Login" page
    login_page = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_widget_set_margin_top(login_page, 20);
    gtk_widget_set_margin_start(login_page, 20);
    gtk_widget_set_margin_end(login_page, 20);
    gtk_widget_set_margin_bottom(login_page, 20);

    GtkWidget *login_label = gtk_label_new("Welcome to the Login Page");
    gtk_box_append(GTK_BOX(login_page), login_label);

    GtkWidget *goto_signup_button = gtk_button_new_with_label("Go to Sign Up");
    gtk_box_append(GTK_BOX(login_page), goto_signup_button);

    // Connect button to switch to signup page
    g_signal_connect(goto_signup_button, "clicked", G_CALLBACK(on_signup_button_clicked), stack);

    // Add "Login" page to stack
    gtk_stack_add_named(GTK_STACK(stack), login_page, "login_page");

    // Add the stack to the main vertical box
    gtk_box_append(GTK_BOX(vbox), stack);

    // Show the window
    gtk_widget_show(window);
}

int main(int argc, char **argv) {
    GtkApplication *app;
    int status;

    app = gtk_application_new("com.example.page_navigation", G_APPLICATION_FLAGS_NONE);
    g_signal_connect(app, "activate", G_CALLBACK(activate), NULL);

    status = g_application_run(G_APPLICATION(app), argc, argv);
    g_object_unref(app);

    return status;
}
