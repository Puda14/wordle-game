#include "client_gui_helpers.h"

void apply_css(GtkWidget *widget, const char *css) {
  GtkCssProvider *provider = gtk_css_provider_new();
  gtk_css_provider_load_from_data(provider, css, -1);
  GtkStyleContext *context = gtk_widget_get_style_context(widget);
  gtk_style_context_add_provider(context, GTK_STYLE_PROVIDER(provider), GTK_STYLE_PROVIDER_PRIORITY_USER);
  g_object_unref(provider);
}

// Function to update the UI (runs in the main thread)
gboolean update_infor_label_sign_up_form(gpointer user_data) {
  char *input = (char *)user_data;

  char *status_str = strtok(input, "|");
  char *payload = strtok(NULL, "|");
  char *label_str = strtok(NULL, "|");

  if (!status_str || !payload || !label_str) {
    g_printerr("Invalid input format: %s\n", input);
    g_free(input);
    return G_SOURCE_REMOVE;
  }

  int response_status = atoi(status_str);
  GtkWidget *infor_label = (GtkWidget *)strtoul(label_str, NULL, 16);

  g_print("Response Status: %d\n", response_status);
  g_print("Response Payload: %s\n", payload);

  if (response_status == 0) {
    gtk_label_set_text(GTK_LABEL(infor_label), payload);
    apply_css(infor_label, "label { color: green; font-weight: bold; }");
  } else {
    gtk_label_set_text(GTK_LABEL(infor_label), payload);
    apply_css(infor_label, "label { color: red; font-weight: bold; }");
  }

  g_free(input);
  return G_SOURCE_REMOVE;
}

// Function to handle request in a separate thread
gpointer send_data_sign_up_thread(gpointer user_data) {
  ThreadDataRequest *data = (ThreadDataRequest *)user_data;

  char response_payload[256] = {0};
  int response_status = 0;

  // Create the message to send
  Message message;
  message.message_type = data->request_type;
  snprintf(message.payload, sizeof(message.payload), "%s|%s", data->username, data->password);

  // Send data
  if (send(data->sock, &message, sizeof(message), 0) < 0) {
    perror("Send failed");
    strncpy(response_payload, "Failed to send data!", sizeof(response_payload));
  } else {
    // Receive response from the server
    Message server_response;
    if (read(data->sock, &server_response, sizeof(server_response)) > 0) {
      strncpy(response_payload, server_response.payload, sizeof(response_payload));
      response_status = server_response.status;
    } else {
      strncpy(response_payload, "No response from server!", sizeof(response_payload));
    }
  }

  // Update the UI in the main thread
  g_idle_add((GSourceFunc)update_infor_label_sign_up_form, g_strdup_printf("%d|%s|%p", response_status, response_payload, data->infor_label));
  g_free(data);
  return NULL;
}

void on_submit_sign_up_click(GtkWidget *widget, gpointer data) {
  FormDataRequest *form_data = (FormDataRequest *)data;

  const gchar *username = gtk_editable_get_text(GTK_EDITABLE(form_data->username_entry));
  const gchar *password = gtk_editable_get_text(GTK_EDITABLE(form_data->password_entry));
  const gchar *confirm_password = form_data->confirm_password_entry ? gtk_editable_get_text(GTK_EDITABLE(form_data->confirm_password_entry)) : NULL;

  gtk_label_set_text(GTK_LABEL(form_data->infor_label), "");

  if (g_strcmp0(username, "") == 0 || g_strcmp0(password, "") == 0 || (confirm_password && g_strcmp0(confirm_password, "") == 0)) {
    gtk_label_set_text(GTK_LABEL(form_data->infor_label), "All fields are required!");
    apply_css(form_data->infor_label, "label { color: red; font-weight: bold; }");
    return;
  }

  if (confirm_password && g_strcmp0(password, confirm_password) != 0) {
    gtk_label_set_text(GTK_LABEL(form_data->infor_label), "Passwords do not match!");
    apply_css(form_data->infor_label, "label { color: red; font-weight: bold; }");
    return;
  }

  // Create data for the thread
  ThreadDataRequest *thread_data = g_new0(ThreadDataRequest, 1);
  thread_data->sock = form_data->sock;
  strncpy(thread_data->username, username, sizeof(thread_data->username));
  strncpy(thread_data->password, password, sizeof(thread_data->password));
  thread_data->infor_label = form_data->infor_label;
  thread_data->request_type = form_data->request_type;

  // Create a thread to handle the task
  g_thread_new("SendDataSignUpThread", send_data_sign_up_thread, thread_data);

  // Display the "Sending data..." status
  gtk_label_set_text(GTK_LABEL(form_data->infor_label), "Sending data...");
  apply_css(form_data->infor_label, "label { color: blue; font-weight: bold; }");
}

void on_submit_log_in_click(GtkWidget *widget, gpointer data) {
  //...
}

// Function to handle "Sign Up" button click
void on_signup_button_clicked(GtkWidget *widget, gpointer stack) {
  gtk_stack_set_visible_child_name(GTK_STACK(stack), "signup_page");
}

// Function to handle "Login" button click
void on_login_button_clicked(GtkWidget *widget, gpointer stack) {
  gtk_stack_set_visible_child_name(GTK_STACK(stack), "login_page");
}

GtkWidget* create_main_window(GtkApplication *app) {
  GtkWidget *window = gtk_application_window_new(app);
  gtk_window_set_title(GTK_WINDOW(window), "Sign Up/Login");
  gtk_window_set_default_size(GTK_WINDOW(window), 1400, 900);
  return window;
}

GtkWidget* create_signup_form_page(FormDataRequest *form_data, GtkWidget *stack) {
  GtkWidget *page = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
  gtk_widget_set_halign(page, GTK_ALIGN_CENTER);
  gtk_widget_set_valign(page, GTK_ALIGN_CENTER);

  GtkWidget *grid = gtk_grid_new();
  gtk_grid_set_row_spacing(GTK_GRID(grid), 10);
  gtk_grid_set_column_spacing(GTK_GRID(grid), 10);
  gtk_box_append(GTK_BOX(page), grid);

  // UI Components for Sign Up
  GtkWidget *label = gtk_label_new("Sign Up");
  gtk_grid_attach(GTK_GRID(grid), label, 0, 0, 2, 1);

  GtkWidget *infor_label = gtk_label_new("");
  gtk_grid_attach(GTK_GRID(grid), infor_label, 0, 1, 2, 1);

  GtkWidget *username_entry = gtk_entry_new();
  GtkWidget *password_entry = gtk_entry_new();
  gtk_entry_set_visibility(GTK_ENTRY(password_entry), FALSE);

  GtkWidget *confirm_password_entry = gtk_entry_new();
  gtk_entry_set_visibility(GTK_ENTRY(confirm_password_entry), FALSE);

  GtkWidget *submit_button = gtk_button_new_with_label("Sign Up");

  gtk_grid_attach(GTK_GRID(grid), gtk_label_new("Username:"), 0, 2, 1, 1);
  gtk_grid_attach(GTK_GRID(grid), username_entry, 1, 2, 1, 1);

  gtk_grid_attach(GTK_GRID(grid), gtk_label_new("Password:"), 0, 3, 1, 1);
  gtk_grid_attach(GTK_GRID(grid), password_entry, 1, 3, 1, 1);

  gtk_grid_attach(GTK_GRID(grid), gtk_label_new("Confirm Password:"), 0, 4, 1, 1);
  gtk_grid_attach(GTK_GRID(grid), confirm_password_entry, 1, 4, 1, 1);

  gtk_grid_attach(GTK_GRID(grid), submit_button, 0, 5, 2, 1);

  // Link form_data
  form_data->username_entry = username_entry;
  form_data->password_entry = password_entry;
  form_data->confirm_password_entry = confirm_password_entry;
  form_data->infor_label = infor_label;

  GtkWidget *goto_login_button = gtk_button_new_with_label("Go to Login");
  gtk_box_append(GTK_BOX(page), goto_login_button);

  // Connect signals
  g_signal_connect(submit_button, "clicked", G_CALLBACK(on_submit_sign_up_click), form_data);
  g_signal_connect(goto_login_button, "clicked", G_CALLBACK(on_login_button_clicked), stack);

  return page;
}

GtkWidget* create_login_form_page(FormDataRequest *form_data, GtkWidget *stack) {
  GtkWidget *page = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
  gtk_widget_set_halign(page, GTK_ALIGN_CENTER);
  gtk_widget_set_valign(page, GTK_ALIGN_CENTER);

  GtkWidget *grid = gtk_grid_new();
  gtk_grid_set_row_spacing(GTK_GRID(grid), 10);
  gtk_grid_set_column_spacing(GTK_GRID(grid), 10);
  gtk_box_append(GTK_BOX(page), grid);

  // UI Components for Login
  GtkWidget *label = gtk_label_new("Login");
  gtk_grid_attach(GTK_GRID(grid), label, 0, 0, 2, 1);

  GtkWidget *infor_label = gtk_label_new("");
  gtk_grid_attach(GTK_GRID(grid), infor_label, 0, 1, 2, 1);

  GtkWidget *username_entry = gtk_entry_new();
  GtkWidget *password_entry = gtk_entry_new();
  gtk_entry_set_visibility(GTK_ENTRY(password_entry), FALSE);

  GtkWidget *submit_button = gtk_button_new_with_label("Login");

  gtk_grid_attach(GTK_GRID(grid), gtk_label_new("Username:"), 0, 2, 1, 1);
  gtk_grid_attach(GTK_GRID(grid), username_entry, 1, 2, 1, 1);

  gtk_grid_attach(GTK_GRID(grid), gtk_label_new("Password:"), 0, 3, 1, 1);
  gtk_grid_attach(GTK_GRID(grid), password_entry, 1, 3, 1, 1);

  gtk_grid_attach(GTK_GRID(grid), submit_button, 0, 4, 2, 1);

  // Link form_data
  form_data->username_entry = username_entry;
  form_data->password_entry = password_entry;
  form_data->confirm_password_entry = NULL;
  form_data->infor_label = infor_label;

  GtkWidget *goto_signup_button = gtk_button_new_with_label("Go to Sign Up");
  gtk_box_append(GTK_BOX(page), goto_signup_button);

  // Connect signals
  g_signal_connect(submit_button, "clicked", G_CALLBACK(on_submit_log_in_click), form_data);
  g_signal_connect(goto_signup_button, "clicked", G_CALLBACK(on_signup_button_clicked), stack);

  return page;
}
