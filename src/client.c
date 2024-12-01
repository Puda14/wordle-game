#include <gtk/gtk.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define PORT 8080
#define BUFFER_SIZE 1024

GtkBuilder *builder;
GtkWidget *window;
GtkStack *stack;

void on_GoToSignup_clicked(GtkButton *button, gpointer user_data) {
  GtkStack *stack = GTK_STACK(user_data);
  gtk_stack_set_visible_child_name(stack, "signup");
}

void on_GoToLogin_clicked(GtkButton *button, gpointer user_data) {
  GtkStack *stack = GTK_STACK(user_data);
  gtk_stack_set_visible_child_name(stack, "login");
}

void on_LoginSubmit_clicked(GtkButton *button, gpointer user_data) {
  GtkStack *stack = GTK_STACK(user_data);
  gtk_stack_set_visible_child_name(stack, "homepage");
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
  GtkStack *stack = GTK_STACK(user_data);
  gtk_stack_set_visible_child_name(stack, "login");
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

// Function to send a message to the server
int send_message(int sock, const char *message) {
  ssize_t bytes_sent = send(sock, message, strlen(message), 0);
  if (bytes_sent < 0) {
    perror("Failed to send message");
    return -1;
  }
  return 0;
}

// Function to receive a message from the server
int receive_message(int sock, char *buffer, size_t buffer_size) {
  ssize_t bytes_received = recv(sock, buffer, buffer_size, 0);
  if (bytes_received < 0) {
    perror("Failed to receive message");
    return -1;
  }
  return 0;
}

int main(int argc, char *argv[]) {

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

  int sock;
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

  // const char *message = "Hello, server!";
  // if (send_message(sock, message) == -1) {
  //   close(sock);
  //   gtk_main_quit();
  //   return -1;
  // }

  // // Example receiving a response from the server
  // char response[BUFFER_SIZE];
  // if (receive_message(sock, response, sizeof(response)) == -1) {
  //   close(sock);
  //   gtk_main_quit();
  //   return -1;
  // }

  // printf("Server response: %s\n", response);

  close(sock);
  gtk_main();

  return 0;
}
