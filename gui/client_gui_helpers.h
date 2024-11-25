#ifndef __CLIENT_GUI_HELPERS__
#define __CLIENT_GUI_HELPERS__

#include <gtk/gtk.h>
#include <glib.h>
#include <arpa/inet.h>

#include "../model/message.h"

extern char logged_in_username[50];

// Struct for Form Data Request
typedef struct {
  GtkWidget *username_entry;
  GtkWidget *password_entry;
  GtkWidget *confirm_password_entry;
  GtkWidget *infor_label;
  int sock;
  int request_type;
} FormDataRequest;

// Struct to pass data to the thread for Request Handling
typedef struct {
  int sock;
  char username[256];
  char password[256];
  GtkWidget *stack;
  GtkWidget *infor_label;
  enum MessageType request_type;
} ThreadDataRequest;

// Struct for Game Page Data
typedef struct {
  GtkWidget *stack;
  int sock;
  char username[256];
} GamePageData;

GtkWidget* create_main_window(GtkApplication *app);

// Sign up
GtkWidget* create_signup_form_page(FormDataRequest *form_data, GtkWidget *stack);

// Log in
GtkWidget* create_login_form_page(FormDataRequest *form_data, GtkWidget *stack);

GtkWidget* create_game_page(GamePageData *game_data, GtkWidget *stack);

void setStack(GtkWidget *stack);

void send_logout_request(int sock, const char *username);

#endif
