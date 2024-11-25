#include <gtk/gtk.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <glib.h>
#include <signal.h>

char logged_in_username[50] = "";
volatile sig_atomic_t quit_flag = 0;

void handle_sigint(int sig) {
    quit_flag = 1;
}

#define PORT 8080

#include "../model/user.h"
#include "../model/message.h"
#include "../gui/client_gui_helpers.h"

void activate(GtkApplication *app, gpointer user_data);

int main(int argc, char **argv) {
    struct sigaction sa;
    sa.sa_handler = handle_sigint;
    sa.sa_flags = 0;
    sigemptyset(&sa.sa_mask);
    sigaction(SIGINT, &sa, NULL);

    GtkApplication *app;
    int status;
    int sock;
    struct sockaddr_in server_addr;

    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("Socket creation error");
        return -1;
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    server_addr.sin_port = htons(PORT);

    if (connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Connection failed");
        close(sock);
        return -1;
    }

    // Initialize GTK application
    app = gtk_application_new("game.wordle.signup_login", G_APPLICATION_FLAGS_NONE);

    g_signal_connect(app, "activate", G_CALLBACK(activate), GINT_TO_POINTER(sock));

    status = g_application_run(G_APPLICATION(app), argc, argv);

    if (quit_flag && logged_in_username[0] != '\0') {
        send_logout_request(sock, logged_in_username);
    }

    status = g_application_run(G_APPLICATION(app), argc, argv);

    close(sock);
    g_object_unref(app);
    return status;
}

void activate(GtkApplication *app, gpointer user_data) {
    int sock = GPOINTER_TO_INT(user_data);

    // Create FormDataRequests for both signup and login
    FormDataRequest *signup_form_data = g_new0(FormDataRequest, 1);
    signup_form_data->sock = sock;
    signup_form_data->request_type = SIGNUP_REQUEST;

    FormDataRequest *login_form_data = g_new0(FormDataRequest, 1);
    login_form_data->sock = sock;
    login_form_data->request_type = LOGIN_REQUEST;

    // Create GamePageData
    GamePageData *game_data = g_new0(GamePageData, 1);
    game_data->sock = sock;
    // strncpy(game_data->username, logged_in_username, sizeof(game_data->username));

    // Create the main window
    GtkWidget *window = create_main_window(app);

    // Create the vertical box layout
    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_window_set_child(GTK_WINDOW(window), vbox);

    // Create the stack widget to hold pages
    GtkWidget *stack = gtk_stack_new();
    gtk_stack_set_transition_type(GTK_STACK(stack), GTK_STACK_TRANSITION_TYPE_SLIDE_LEFT_RIGHT);
    gtk_stack_set_transition_duration(GTK_STACK(stack), 300);

    setStack(stack);

    // Create Sign Up and Login pages and add to stack
    GtkWidget *signup_page = create_signup_form_page(signup_form_data, stack);
    GtkWidget *login_page = create_login_form_page(login_form_data, stack);
    GtkWidget *game_page = create_game_page(game_data, stack);

    gtk_stack_add_named(GTK_STACK(stack), signup_page, "signup_page");
    gtk_stack_add_named(GTK_STACK(stack), login_page, "login_page");
    gtk_stack_add_named(GTK_STACK(stack), game_page, "game_page");
    /* ... More pages ... */

    // Create the stack switcher to switch between pages
    GtkWidget *stack_switcher = gtk_stack_switcher_new();
    gtk_stack_switcher_set_stack(GTK_STACK_SWITCHER(stack_switcher), GTK_STACK(stack));
    gtk_box_append(GTK_BOX(vbox), stack_switcher);

    // Add the stack to the main vertical box
    gtk_box_append(GTK_BOX(vbox), stack);

    gtk_widget_show(window);
}
