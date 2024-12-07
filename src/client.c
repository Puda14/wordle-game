#include <gtk/gtk.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include "./model/message.h"
#include "database.h"

#define PORT 8080
#define BUFFER_SIZE 1024

char current_username[51] = "";
char current_password[51] = "";

GtkBuilder *builder;
GtkWidget *window;
GtkStack *stack;
int sock;

/* Thread Data */
typedef struct {
  char username[51];
  char password[51];
  int request_type;
} ThreadDataRequest;


/* Logic No Thread */
int send_message(const char *message) {
  if (sock < 0) {
    perror("Socket not connected");
    return -1;
  }

  ssize_t bytes_sent = send(sock, message, strlen(message), 0);
  if (bytes_sent < 0) {
    perror("Failed to send message");
    return -1;
  }
  return 0;
}

int receive_message(char *buffer, size_t buffer_size) {
  if (sock < 0) {
    perror("Socket not connected");
    return -1;
  }

  ssize_t bytes_received = recv(sock, buffer, buffer_size, 0);
  if (bytes_received < 0) {
    perror("Failed to receive message");
    return -1;
  }
  return 0;
}

/* Update GUI */

typedef struct {
  GtkStack *stack;
  const gchar *child_name;
} StackTransitionData;

gboolean transition_to_stack(gpointer user_data) {
  StackTransitionData *data = (StackTransitionData*)user_data;

  if (data && data->stack && data->child_name) {
    gtk_stack_set_visible_child_name(data->stack, data->child_name);
    g_free(data);
  }

  return G_SOURCE_REMOVE;
}

void show_error_dialog(const gchar *message) {
  GtkWidget *dialog = gtk_message_dialog_new(GTK_WINDOW(window),
                                             GTK_DIALOG_MODAL,
                                             GTK_MESSAGE_ERROR,
                                             GTK_BUTTONS_OK,
                                             "%s", message);
  gtk_dialog_run(GTK_DIALOG(dialog));
  gtk_widget_destroy(dialog);
}

/* Thread data */

void* send_login_thread(void* arg) {
  ThreadDataRequest* login_data = (ThreadDataRequest*)arg;

  Message login_message;
  login_message.message_type = LOGIN_REQUEST;
  login_message.status = SUCCESS;
  snprintf(login_message.payload, BUFFER_SIZE, "%s|%s", login_data->username, login_data->password);

  printf("Sending login message: %s\n", login_message.payload);

  ssize_t bytes_sent = send(sock, &login_message, sizeof(login_message), 0);
  if (bytes_sent < 0) {
    perror("Failed to send message");
    free(login_data);
    return NULL;
  }

  Message response_message;
  ssize_t bytes_received = recv(sock, &response_message, sizeof(response_message), 0);
  if (bytes_received < 0) {
    perror("Failed to receive response");
    free(login_data);
    return NULL;
  }

  if (response_message.status == SUCCESS) {
    StackTransitionData *data = g_malloc(sizeof(StackTransitionData));
    data->stack = stack;
    data->child_name = "homepage";

    strncpy(current_username, login_data->username, sizeof(current_username) - 1);
    current_username[sizeof(current_username) - 1] = '\0';

    strncpy(current_password, login_data->password, sizeof(current_password) - 1);
    current_password[sizeof(current_password) - 1] = '\0';

    g_idle_add(transition_to_stack, data);

    printf("Login successful, proceeding to homepage...\n");
  } else {
    g_idle_add((GSourceFunc)show_error_dialog, response_message.payload);
  }

  printf("Response message: %s\n", response_message.payload);

  free(login_data);
  return NULL;
}

void* send_logout_thread(void* arg) {
  ThreadDataRequest* logout_data = (ThreadDataRequest*)arg;

  Message logout_message;
  logout_message.message_type = LOGOUT_REQUEST;
  logout_message.status = SUCCESS;
  snprintf(logout_message.payload, BUFFER_SIZE, "%s|%s", logout_data->username, logout_data->password);

  printf("Sending logout message: %s\n", logout_message.payload);

  ssize_t bytes_sent = send(sock, &logout_message, sizeof(logout_message), 0);
  if (bytes_sent < 0) {
    perror("Failed to send message");
    free(logout_data);
    return NULL;
  }

  Message response_message;
  ssize_t bytes_received = recv(sock, &response_message, sizeof(response_message), 0);
  if (bytes_received < 0) {
    perror("Failed to receive response");
    free(logout_data);
    return NULL;
  }

  if (response_message.status == SUCCESS) {
    StackTransitionData *data = g_malloc(sizeof(StackTransitionData));
    data->stack = stack;
    data->child_name = "login";

    g_idle_add(transition_to_stack, data);

    printf("Logout successful, returning to login page...\n");
  } else {
    g_idle_add((GSourceFunc)show_error_dialog, response_message.payload);
  }

  printf("Response message: %s\n", response_message.payload);

  free(logout_data);
  return NULL;
}

/* Logic button */

void on_GoToSignup_clicked(GtkButton *button, gpointer user_data) {
  GtkStack *stack = GTK_STACK(user_data);
  gtk_stack_set_visible_child_name(stack, "signup");
}

void on_GoToLogin_clicked(GtkButton *button, gpointer user_data) {
  GtkStack *stack = GTK_STACK(user_data);
  gtk_stack_set_visible_child_name(stack, "login");
}

void on_LoginSubmit_clicked(GtkButton *button, gpointer user_data) {
  GtkEntry *input_field;

  input_field = GTK_ENTRY(gtk_builder_get_object(builder, "LoginUsernameEntry"));
  const gchar *username = gtk_entry_get_text(input_field);

  input_field = GTK_ENTRY(gtk_builder_get_object(builder, "LoginPasswordEntry"));
  const gchar *password = gtk_entry_get_text(input_field);

  if (strlen(username) == 0 || strlen(password) == 0) {
    GtkWidget *dialog = gtk_message_dialog_new(GTK_WINDOW(window),
                                                GTK_DIALOG_MODAL,
                                                GTK_MESSAGE_ERROR,
                                                GTK_BUTTONS_OK,
                                                "Username or password cannot be empty!");
    gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
    return;
  }

  ThreadDataRequest* login_data = malloc(sizeof(ThreadDataRequest));

  strncpy(login_data->username, username, strlen(username) + 1);
  strncpy(login_data->password, password, strlen(password) + 1);
  login_data->username[sizeof(login_data->username) - 1] = '\0';
  login_data->password[sizeof(login_data->password) - 1] = '\0';

  pthread_t thread_id;
  printf("Before Thread\n");
  pthread_create(&thread_id, NULL, send_login_thread, login_data);
  pthread_detach(thread_id);
  printf("After Thread\n");
}

void on_PlayGame_clicked(GtkButton *button, gpointer user_data) {
  GtkStack *stack = GTK_STACK(user_data);
  gtk_stack_set_visible_child_name(stack, "game");
}

void on_BackToHome_clicked(GtkButton *button, gpointer user_data) {
  GtkStack *stack = GTK_STACK(user_data);
  gtk_stack_set_visible_child_name(stack, "homepage");
}

void on_GoToHistory_clicked(GtkButton *button, gpointer user_data) {
  GtkStack *stack = GTK_STACK(user_data);
  gtk_stack_set_visible_child_name(stack, "history");
}

void on_Logout_clicked(GtkButton *button, gpointer user_data) {
  ThreadDataRequest* logout_data = malloc(sizeof(ThreadDataRequest));

  strncpy(logout_data->username, current_username, strlen(current_username) + 1);
  strncpy(logout_data->password, current_password, strlen(current_password) + 1);
  logout_data->username[sizeof(logout_data->username) - 1] = '\0';
  logout_data->password[sizeof(logout_data->password) - 1] = '\0';

  pthread_t thread_id;
  printf("Before Logout Thread\n");
  pthread_create(&thread_id, NULL, send_logout_thread, logout_data);
  pthread_detach(thread_id);
  printf("After Logout Thread\n");
}

void set_signal_connect(){
  GtkWidget *button;

  button = GTK_WIDGET(gtk_builder_get_object(builder, "GoToSignup"));
  if (button) {
    g_signal_connect(button, "clicked", G_CALLBACK(on_GoToSignup_clicked), stack);
  }

  button = GTK_WIDGET(gtk_builder_get_object(builder, "GoToLogin"));
  if (button) {
    g_signal_connect(button, "clicked", G_CALLBACK(on_GoToLogin_clicked), stack);
  }

  button = GTK_WIDGET(gtk_builder_get_object(builder, "LoginSubmit"));
  if (button) {
    g_signal_connect(button, "clicked", G_CALLBACK(on_LoginSubmit_clicked), stack);
  }

  button = GTK_WIDGET(gtk_builder_get_object(builder, "PlayGame"));
  if (button) {
    g_signal_connect(button, "clicked", G_CALLBACK(on_PlayGame_clicked), stack);
  }

  button = GTK_WIDGET(gtk_builder_get_object(builder, "BackToHome1"));
  if (button) {
    g_signal_connect(button, "clicked", G_CALLBACK(on_BackToHome_clicked), stack);
  }

  button = GTK_WIDGET(gtk_builder_get_object(builder, "BackToHome2"));
  if (button) {
    g_signal_connect(button, "clicked", G_CALLBACK(on_BackToHome_clicked), stack);
  }

  button = GTK_WIDGET(gtk_builder_get_object(builder, "GoToHistory"));
  if (button) {
    g_signal_connect(button, "clicked", G_CALLBACK(on_GoToHistory_clicked), stack);
  }

  button = GTK_WIDGET(gtk_builder_get_object(builder, "Logout"));
  if (button) {
    g_signal_connect(button, "clicked", G_CALLBACK(on_Logout_clicked), stack);
  }
}

int main(int argc, char *argv[]) {

  struct sockaddr_in server_addr;

  sock = socket(AF_INET, SOCK_STREAM, 0);
  if (sock < 0) {
    perror("Socket creation error");
    gtk_main_quit();
    return -1;
  }

  server_addr.sin_family = AF_INET;
  server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
  server_addr.sin_port = htons(PORT);

  if (connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
    perror("Connection failed");
    close(sock);
    gtk_main_quit();
    return -1;
  }

  printf("Connected to server\n");

  gtk_init(&argc, &argv);

  builder = gtk_builder_new_from_file("wordle.glade");
  if (!builder) {
    g_printerr("Failed to load Glade file\n");
    return 1;
  }

  window = GTK_WIDGET(gtk_builder_get_object(builder, "main_window"));
  if (!window) {
    g_printerr("Failed to get main_window from Glade file\n");
    return 1;
  }

  stack = GTK_STACK(gtk_builder_get_object(builder, "stack"));
  if (!stack) {
    g_printerr("Failed to get stack from Glade file\n");
    return 1;
  }

  set_signal_connect();

  gtk_widget_show_all(window);

  gtk_main();
  close(sock);

  return 0;
}
