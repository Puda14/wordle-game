#include <gtk/gtk.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <arpa/inet.h>

#define PORT 8080
#define BUFFER_SIZE 1024
// Global variables for game state
#define WORD_LENGTH 5
#define MAX_ATTEMPTS 12

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

// void on_PlayGame_clicked(GtkButton *button, gpointer user_data) {
//   GtkStack *stack = GTK_STACK(user_data);
//   gtk_stack_set_visible_child_name(stack, "game");
// }


static char current_word[WORD_LENGTH + 1];
static char target_word[WORD_LENGTH + 1];
static int current_row = 0;
static int current_col = 0;
static GtkLabel *game_grid[MAX_ATTEMPTS][WORD_LENGTH];
static GtkWidget *GameBoard;
static GtkEntry *word_entry;
static GtkWidget *submit_button;

// Check the entered word
void check_word() {
    gboolean is_correct = TRUE;
    
    for (int i = 0; i < WORD_LENGTH; i++) {
        GtkStyleContext *context = gtk_widget_get_style_context(GTK_WIDGET(game_grid[current_row][i]));
        
        if (current_word[i] == target_word[i]) {
            // Correct letter, correct position - green
            gtk_style_context_add_class(context, "correct");
        } else {
            is_correct = FALSE;
            gboolean found = FALSE;
            
            // Check if letter exists in target word
            for (int j = 0; j < WORD_LENGTH; j++) {
                if (current_word[i] == target_word[j]) {
                    found = TRUE;
                    break;
                }
            }
            
            if (found) {
                // Correct letter, wrong position - yellow
                gtk_style_context_add_class(context, "wrong-position");
            } else {
                // Wrong letter - gray
                gtk_style_context_add_class(context, "wrong");
            }
        }
    }
    
    current_row++;
    current_col = 0;
    memset(current_word, 0, sizeof(current_word));
    
    if (is_correct) {
        // Show win dialog
        GtkWidget *dialog = gtk_message_dialog_new(GTK_WINDOW(window),
                                                 GTK_DIALOG_MODAL,
                                                 GTK_MESSAGE_INFO,
                                                 GTK_BUTTONS_OK,
                                                 "Congratulations! You won!");
        gtk_dialog_run(GTK_DIALOG(dialog));
        gtk_widget_destroy(dialog);
    } else if (current_row >= MAX_ATTEMPTS) {
        // Show game over dialog
        GtkWidget *dialog = gtk_message_dialog_new(GTK_WINDOW(window),
                                                 GTK_DIALOG_MODAL,
                                                 GTK_MESSAGE_INFO,
                                                 GTK_BUTTONS_OK,
                                                 "Game Over! The word was: %s", target_word);
        gtk_dialog_run(GTK_DIALOG(dialog));
        gtk_widget_destroy(dialog);
    }
}
void on_submit_word_clicked(GtkButton *button, gpointer user_data) {
    const gchar *word = gtk_entry_get_text(GTK_ENTRY(word_entry));
    printf("Submitted word: %s\n", word);
    g_print("Submitted word: %s\n", word); // Debug print

    if (strlen(word) != WORD_LENGTH) {
        g_print("Invalid word length\n");
        return;
    }

    // Convert to uppercase and display in grid
    for (int i = 0; i < WORD_LENGTH; i++) {
        current_word[i] = toupper(word[i]);
        char letter[2] = {current_word[i], '\0'};
        gtk_label_set_text(game_grid[current_row][i], letter);
        g_print("Setting letter %c at position %d\n", current_word[i], i); // Debug print
    }
    current_word[WORD_LENGTH] = '\0';

    check_word();
    gtk_entry_set_text(GTK_ENTRY(word_entry), "");
}
// Initialize the game board
void init_game_board(GtkBuilder *builder) {
  printf("init_game_board\n"); // Debug print
    // Get the grid widget from builder
    GtkWidget *grid = GTK_WIDGET(gtk_builder_get_object(builder, "wordle_grid"));
    if (!grid) {
        g_printerr("Failed to get wordle_grid from builder\n");
        return;
    }
    printf("grid: %p\n", grid); // Debug print

    // Verify it's a grid
    if (!GTK_IS_GRID(grid)) {
        g_printerr("wordle_grid is not a GtkGrid widget\n");
        return;
    }
         
    // Get entry widget reference
    word_entry = GTK_ENTRY(gtk_builder_get_object(builder, "word_entry")); 
    if (!word_entry) {
        g_printerr("Failed to get word_entry from builder\n");
        return;
    }
    printf("word_entry: %p\n", word_entry); // Debug print
    // Get submit button
    submit_button = GTK_WIDGET(gtk_builder_get_object(builder, "submit_word"));
    if (!submit_button) {
        g_printerr("Failed to get submit_button\n");
        return;
    }
    printf("submit_button: %p\n", submit_button); // Debug print
    // Connect submit button signal
    g_signal_connect(submit_button, "clicked", G_CALLBACK(on_submit_word_clicked), NULL);

    // Configure grid properties
    gtk_grid_set_row_spacing(GTK_GRID(grid), 5);
    gtk_grid_set_column_spacing(GTK_GRID(grid), 5);
    gtk_grid_set_row_homogeneous(GTK_GRID(grid), TRUE);
    gtk_grid_set_column_homogeneous(GTK_GRID(grid), TRUE);

    // Create and attach labels
    for (int i = 0; i < MAX_ATTEMPTS; i++) {
        for (int j = 0; j < WORD_LENGTH; j++) {
            GtkWidget *label = gtk_label_new("");
            gtk_widget_set_size_request(label, 50, 50);
            gtk_widget_set_name(label, "wordle-box");
            
            // Store reference to label
            game_grid[i][j] = GTK_LABEL(label);
            
            // Attach to grid
            gtk_grid_attach(GTK_GRID(grid), label, j, i, 1, 1);
            
            // Apply CSS styling
            GtkStyleContext *context = gtk_widget_get_style_context(label);
            gtk_style_context_add_class(context, "wordle-box");
        }
    }

    // Initialize game state
    strcpy(target_word, "HELLO");
    printf("Target word: %s\n", target_word); // Debug print
    memset(current_word, 0, sizeof(current_word));
 
}


// Handle keyboard input
gboolean on_key_press(GtkWidget *widget, GdkEventKey *event, gpointer data) {
    if (current_row >= MAX_ATTEMPTS) return TRUE;

    if (event->keyval >= GDK_KEY_A && event->keyval <= GDK_KEY_Z) {
        if (current_col < WORD_LENGTH) {
            char letter = event->keyval - GDK_KEY_A + 'A';
            current_word[current_col] = letter;
            char str[2] = {letter, '\0'};
            gtk_label_set_text(game_grid[current_row][current_col], str);
            current_col++;
        }
    }
    else if (event->keyval == GDK_KEY_BackSpace) {
        if (current_col > 0) {
            current_col--;
            current_word[current_col] = '\0';
            gtk_label_set_text(game_grid[current_row][current_col], "");
        }
    }
    else if (event->keyval == GDK_KEY_Return) {
        if (current_col == WORD_LENGTH) {
            check_word();
        }
    }
    
    return TRUE;
}


// Add CSS styles for the game
void add_css_styles() {
    GtkCssProvider *provider = gtk_css_provider_new();
    gtk_css_provider_load_from_data(provider,
        ".wordle-box { "
        "    background-color: white;"
        "    border: 2px solid #d3d6da;"
        "    font-size: 20px;"
        "    font-weight: bold;"
        "}"
        ".correct { background-color: #6aaa64; color: white; }"
        ".wrong-position { background-color: #c9b458; color: white; }"
        ".wrong { background-color: #787c7e; color: white; }",
        -1, NULL);
    
    gtk_style_context_add_provider_for_screen(gdk_screen_get_default(),
                                            GTK_STYLE_PROVIDER(provider),
                                            GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
    g_object_unref(provider);
}

// Modify your existing on_PlayGame_clicked function
void on_PlayGame_clicked(GtkButton *button, gpointer user_data) {
    GtkStack *stack = GTK_STACK(user_data);
    gtk_stack_set_visible_child_name(stack, "game");
    printf("Play game\n"); // Debug print
    // Reset game state
    current_row = 0;
    current_col = 0;
    memset(current_word, 0, sizeof(current_word));
    
    // Clear all labels and styles
    for (int i = 0; i < MAX_ATTEMPTS; i++) {
        for (int j = 0; j < WORD_LENGTH; j++) {
            gtk_label_set_text(game_grid[i][j], "");
            GtkStyleContext *context = gtk_widget_get_style_context(GTK_WIDGET(game_grid[i][j]));
            gtk_style_context_remove_class(context, "correct");
            gtk_style_context_remove_class(context, "wrong-position");
            gtk_style_context_remove_class(context, "wrong");
        }
    }
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

  // Insert the two functions here
  init_game_board(builder);  // Initialize game board first
  add_css_styles();         // Then add CSS styles
  set_signal_connect();

  gtk_widget_show_all(window);

  // int sock;
  // struct sockaddr_in server_addr;

  // sock = socket(AF_INET, SOCK_STREAM, 0);
  // if (sock < 0) {
  //   perror("Socket creation error");
  //   gtk_main_quit();
  //   return -1;
  // }

  // server_addr.sin_family = AF_INET;
  // server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
  // server_addr.sin_port = htons(PORT);

  // if (connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
  //   perror("Connection failed");
  //   close(sock);
  //   gtk_main_quit();
  //   return -1;
  // }

  // printf("Connected to server\n");

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

  // close(sock);
  gtk_main();

  return 0;
}
