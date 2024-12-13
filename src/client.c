#include <gtk/gtk.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <fcntl.h>
#include <stdbool.h>
#include "database.h"
#include "./model/message.h"

#define PORT 8080
#define BUFFER_SIZE 1024
// Global variables for game state
#define WORD_LENGTH 5
#define MAX_ATTEMPTS 12

// Message queue structure
#define MAX_QUEUE_SIZE 100

typedef struct
{
    Message messages[MAX_QUEUE_SIZE];
    int front;
    int rear;
    pthread_mutex_t mutex;
    pthread_cond_t not_empty;
    pthread_cond_t not_full;
} MessageQueue;

// Global variables
char client_name[50];
char client_password[50];

static MessageQueue send_queue;
static MessageQueue receive_queue;
static pthread_t network_thread;
static volatile int network_running = 1;
// Add at top with other globals
static int sockfd; // Socket toàn cục

GtkBuilder *builder;
GtkWidget *window;
GtkStack *stack;
// Add to client.c globals
static int game_session_id = -1;
static int player_number = 0;
static int player_num = 0;
static int opponent_attempts = 0;
static char current_word[WORD_LENGTH + 1];
static char target_word[WORD_LENGTH + 1];
static int current_row = 0;
static int current_col = 0;
static GtkLabel *game_grid[MAX_ATTEMPTS][WORD_LENGTH];
static GtkWidget *game_status_label;
static GtkWidget *GameBoard;
static GtkEntry *word_entry;
static GtkWidget *submit_button;
// static GtkWidget *user_list_box;

void show_error_dialog(const char *message)
{
    GtkWidget *dialog = gtk_message_dialog_new(GTK_WINDOW(window),
                                               GTK_DIALOG_MODAL,
                                               GTK_MESSAGE_ERROR,
                                               GTK_BUTTONS_OK,
                                               "%s", message);

    gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
}

void show_dialog(const char *message)
{
    GtkWidget *dialog = gtk_message_dialog_new(GTK_WINDOW(window),
                                               GTK_DIALOG_MODAL,
                                               GTK_MESSAGE_INFO,
                                               GTK_BUTTONS_OK,
                                               "%s", message);

    gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
}

// Initialize message queue
void init_message_queue(MessageQueue *queue)
{
    queue->front = 0;
    queue->rear = 0;
    pthread_mutex_init(&queue->mutex, NULL);
    pthread_cond_init(&queue->not_empty, NULL);
    pthread_cond_init(&queue->not_full, NULL);
}

// Queue operations
void queue_push(MessageQueue *queue, Message *msg)
{
    printf("Pushing message of type %d, content: %s\n", msg->message_type, msg->payload);
    pthread_mutex_lock(&queue->mutex);
    while ((queue->rear + 1) % MAX_QUEUE_SIZE == queue->front)
    {
        pthread_cond_wait(&queue->not_full, &queue->mutex);
    }
    queue->messages[queue->rear] = *msg;
    queue->rear = (queue->rear + 1) % MAX_QUEUE_SIZE;
    pthread_cond_signal(&queue->not_empty);
    pthread_mutex_unlock(&queue->mutex);
}

int queue_pop(MessageQueue *queue, Message *msg)
{
    pthread_mutex_lock(&queue->mutex);
    if (queue->front == queue->rear)
    {
        pthread_mutex_unlock(&queue->mutex);
        return -1;
    }
    *msg = queue->messages[queue->front];
    queue->front = (queue->front + 1) % MAX_QUEUE_SIZE;
    pthread_cond_signal(&queue->not_full);
    pthread_mutex_unlock(&queue->mutex);
    return 0;
}

int send_message(int sockfd, const Message *msg)
{
    int bytes_sent = send(sockfd, msg, sizeof(Message), 0);
    if (bytes_sent < 0)
    {
        perror("Send failed");
        return -1;
    }
    printf("Sent message of type %d, content: %s\n", msg->message_type, msg->payload);
    return 0;
}
int receive_message(int sockfd, Message *msg)
{
    int bytes_received = recv(sockfd, msg, sizeof(Message), 0);
    if (bytes_received < 0)
    {
        perror("Receive failed");
        return -1;
    }
    printf("Received message of type %d, content: %s, status: %d\n", msg->message_type, msg->payload, msg->status);
    return 0;
}

// Kết nối TCP
int init_tcp_socket(const char *server_ip, int port)
{

    int sockfd;
    struct sockaddr_in server_addr;

    // Tạo socket
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
    {
        perror("Socket creation failed");
        return -1;
    }

    // Cấu hình địa chỉ máy chủ
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    if (inet_pton(AF_INET, server_ip, &server_addr.sin_addr) <= 0)
    {
        perror("Invalid address or Address not supported");
        return -1;
    }

    // Kết nối đến máy chủ
    if (connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        perror("Connection failed");
        return -1;
    }
    printf("Connecting to server %s:%d\n", server_ip, port);
    return sockfd;
}

//Hanlde for game score
void handle_score_update(Message *msg) {
    char player1_name[50], player2_name[50];
    int player1_score, player2_score;
    sscanf(msg->payload, "%49[^|]|%d|%49[^|]|%d", player1_name, &player1_score, player2_name, &player2_score);

    // Update player 1 score label
    GtkLabel *player1_score_label = GTK_LABEL(gtk_builder_get_object(builder, "player1_score_label"));
    if (player1_score_label) {
        char score_text[100];
        snprintf(score_text, sizeof(score_text), "%s : %d", player1_name, player1_score);
        gtk_label_set_text(player1_score_label, score_text);
    }

    // Update player 2 score label
    GtkLabel *player2_score_label = GTK_LABEL(gtk_builder_get_object(builder, "player2_score_label"));
    if (player2_score_label) {
        char score_text[100];
        snprintf(score_text, sizeof(score_text), "%s : %d", player2_name, player2_score);
        gtk_label_set_text(player2_score_label, score_text);
    }
}

// Add message response handlers
void handle_game_start_response(Message *msg)
{
    if (msg->status == SUCCESS)
    {
        sscanf(msg->payload, "%d|%d", &game_session_id, &player_num);

        printf("Game session %d started and you are player %d\n", game_session_id, player_num);
        init_game_state(game_session_id);
        // Switch to game view
        gtk_stack_set_visible_child_name(GTK_STACK(stack), "game");
        GtkLabel *turn_notify_label = GTK_LABEL(gtk_builder_get_object(builder, "turn_notify"));
        if (turn_notify_label) {
            if(player_num == 1) {
                gtk_widget_set_sensitive(word_entry, TRUE);
                gtk_widget_set_sensitive(submit_button, TRUE);
                gtk_label_set_text(turn_notify_label, "Your turn");
            } else {
                gtk_widget_set_sensitive(word_entry, FALSE);
                gtk_widget_set_sensitive(submit_button, FALSE);
                gtk_label_set_text(turn_notify_label, "Opponent's turn");
            }
        }
        // Reset game state
        current_row = 0;
        opponent_attempts = 0;
        current_col = 0;
        memset(current_word, 0, sizeof(current_word));
    }
    else
    {
        show_error_dialog("Failed to start game");
    }
}

void handle_game_turn_response(Message *msg) {
    if (msg->message_type == GAME_TURN) {
        int turn;
        sscanf(msg->payload, "%d", &turn);

        // Hiển thị thông báo lượt chơi
        g_print("It's turn of player %d \n", turn);

        // Nếu trò chơi đã kết thúc
        if (turn == 0) {
            // show_dialog("Game over!");
            gtk_widget_set_sensitive(word_entry, FALSE);
            gtk_widget_set_sensitive(submit_button, FALSE);
            return;
        }
         // Update turn notification label
        GtkLabel *turn_notify_label = GTK_LABEL(gtk_builder_get_object(builder, "turn_notify"));

        // Cập nhật lượt chơi
        if (turn == 1 && player_num == 1) {
            gtk_widget_set_sensitive(word_entry, TRUE);
            gtk_widget_set_sensitive(submit_button, TRUE);
            if (turn_notify_label) {
            gtk_label_set_text(turn_notify_label, "Your turn");
            }
        } else if (turn == 2 && player_num == 2) {
            gtk_widget_set_sensitive(word_entry, TRUE);
            gtk_widget_set_sensitive(submit_button, TRUE);
            if (turn_notify_label) {
            gtk_label_set_text(turn_notify_label, "Your turn");
            }
        } else {
            gtk_widget_set_sensitive(word_entry, FALSE);
            gtk_widget_set_sensitive(submit_button, FALSE);
            if (turn_notify_label) {
            gtk_label_set_text(turn_notify_label, "Opponent's turn");
            }
        }
    }
}

void handle_game_guess_response(Message *msg)
{
    if (msg->status != SUCCESS)
    {
        show_error_dialog(msg->payload);
        g_print("Error in game guess response\n");
        gtk_widget_set_sensitive(word_entry, TRUE);
        gtk_widget_set_sensitive(submit_button, TRUE);
        return;
    }

    char result[WORD_LENGTH + 1] = {0};
    int current_player = 0, p1_attempts = 0, p2_attempts = 0;
    char guess[WORD_LENGTH + 1] = {0};

    if (strncmp(msg->payload, "WIN", 3) == 0) {
        // Nếu là WIN, có 4 phần: WIN|result|player_num|p1_attempts|p2_attempts
        if (sscanf(msg->payload, "WIN|%[^|]|%d|%d|%d|%s", result, &current_player, &p1_attempts, &p2_attempts, guess) != 5) {
            g_print("Failed to parse WIN game guess response payload\n");
            return;
        }
        // Hiển thị thông báo chiến thắng cho người chơi
        if (current_player == player_num) {
            show_dialog("You Win! Congratulations!");
        } else {
            show_dialog("You lose! Better luck next time.");
        }
    } else if (strncmp(msg->payload, "DRAW", 4) == 0) {
        // Nếu là DRAW, có 2 phần: DRAW|p1_attempts|p2_attempts
        if (sscanf(msg->payload, "DRAW|%d|%d", &p1_attempts, &p2_attempts) != 2) {
            g_print("Failed to parse DRAW game guess response payload\n");
            return;
        }
        show_dialog("It's a draw! No winner this time.");
    } else if (strncmp(msg->payload, "CONTINUE", 8) == 0) {
        // Nếu là CONTINUE, có 4 phần: CONTINUE|result|current_player|p1_attempts|p2_attempts
        if (sscanf(msg->payload, "CONTINUE|%[^|]|%d|%d|%d|%s", result, &current_player, &p1_attempts, &p2_attempts, guess) != 5) {
            g_print("Failed to parse CONTINUE game guess response payload\n");
            return;
        }
    } else {
        g_print("Unknown game response: %s\n", msg->payload);
        return;
    }

    // Show word in grid immediately
    for (int i = 0; i < WORD_LENGTH; i++)
    {
        char letter[2] = {guess[i], '\0'};
        gtk_label_set_text(game_grid[current_row][i], letter);
    }
    // Cập nhật màu sắc lưới (grid) cho kết quả đoán
    for (int i = 0; i < WORD_LENGTH; i++)
    {
        GtkStyleContext *context = gtk_widget_get_style_context(
            GTK_WIDGET(game_grid[current_row][i]));

        switch (result[i])
        {
        case 'G': // Correct letter and position
            gtk_style_context_add_class(context, "correct");
            break;
        case 'Y': // Correct letter, wrong position
            gtk_style_context_add_class(context, "wrong-position");
            break;
        default: // Incorrect letter
            gtk_style_context_add_class(context, "wrong");
            break;
        }
    }

    // Tăng hàng hiện tại để chuẩn bị cho lượt tiếp theo
    current_row++;

    // Xóa ô nhập liệu sau khi đoán
    gtk_entry_set_text(word_entry, "");

    // Cập nhật số lần thử của đối thủ dựa trên người chơi hiện tại
    opponent_attempts = (player_number == 1) ? p2_attempts : p1_attempts;

    // Cập nhật trạng thái lượt chơi
    gboolean is_our_turn = (current_player == player_number);
    gtk_widget_set_sensitive(word_entry, is_our_turn);
    gtk_widget_set_sensitive(submit_button, is_our_turn);

    // Hiển thị thông báo trạng thái lượt chơi
    g_print("Current Player: %d, Your Turn: %s\n",
            current_player, is_our_turn ? "Yes" : "No");
}

void handle_game_detail_response(Message *msg) {
  if (msg->status != SUCCESS) {
    g_print("Error in fetching game details: %s\n", msg->payload);
    show_error_dialog("Failed to retrieve game details.");
    return;
  }

  // Parse the game details
  GameHistory game_details;
  memset(&game_details, 0, sizeof(GameHistory));

  int parsed_fields = sscanf(msg->payload,
       "%19[^|]|%49[^|]|%49[^|]|%d|%d|%49[^|]|%5[^|]|%19[^|]|%19[^|]",
       game_details.game_id, game_details.player1, game_details.player2,
       &game_details.player1_score, &game_details.player2_score,
       game_details.winner, game_details.word,
       game_details.start_time, game_details.end_time);

  // Parse moves (assuming they're appended in the payload)
  char *moves_start = strstr(msg->payload, "\nMoves:");
  if (moves_start) {
    moves_start += 7; // Skip the "\nMoves:" prefix
    char *line = strtok(moves_start, "\n");
    int move_index = 0;

    while (line && move_index < 12) {
      sscanf(line, "%49[^|]|%5[^|]|%5s",
             game_details.moves[move_index].player_name,
             game_details.moves[move_index].guess,
             game_details.moves[move_index].result);

      move_index++;
      line = strtok(NULL, "\n");
    }
  }

  // Create and populate the dialog with game details
  GtkWidget *dialog = gtk_message_dialog_new(
      GTK_WINDOW(window), GTK_DIALOG_MODAL, GTK_MESSAGE_INFO, GTK_BUTTONS_OK, "Game Details");

  // Format the game details into a readable string
  char details[2048] = {0};
  snprintf(details, sizeof(details),
           "Game ID: %s\n"
           "Players: %s vs %s\n"
           "Score: %d - %d\n"
           "Winner: %s\n"
           "Word: %s\n"
           "Start Time: %s\n"
           "End Time: %s\n\n"
           "Moves:\n",
           game_details.game_id, game_details.player1, game_details.player2,
           game_details.player1_score, game_details.player2_score, game_details.winner,
           game_details.word, game_details.start_time, game_details.end_time);

  for (int i = 0; i < 12; i++) {
    if (strlen(game_details.moves[i].guess) == 0) {
      break; // No more moves
    }
    char move[256];
    snprintf(move, sizeof(move), "Player: %s | Guess: %s | Result: %s\n",
             game_details.moves[i].player_name, game_details.moves[i].guess,
             game_details.moves[i].result);
    strncat(details, move, sizeof(details) - strlen(details) - 1);
  }

  gtk_message_dialog_format_secondary_text(GTK_MESSAGE_DIALOG(dialog), "%s", details);

  // Run and destroy the dialog
  gtk_dialog_run(GTK_DIALOG(dialog));
  gtk_widget_destroy(dialog);
}

// Add response handlers
void handle_game_get_target_response(Message *msg)
{
    if (msg->status == SUCCESS)
    {
        strcpy(target_word, msg->payload);
        g_print("Game session %d initialized with target word\n", game_session_id);
    }
    else
    {
        g_print("Failed to get target word\n");
    }
}

int parse_user_list(char *payload, User *user_list, int max_users) {
    char *line = strtok(payload, "\n");  // Tách theo từng dòng
    int user_count = 0;

    while (line != NULL && user_count < max_users) {
        int id, score, is_online;
        char username[50], password[50];

        if (sscanf(line, "ID: %d, Username: %[^,], Score: %d, Online: %d",
                   &id, username, &score, &is_online) == 4) {
            user_list[user_count].id = id;
            strcpy(user_list[user_count].username, username);
            user_list[user_count].score = score;
            user_list[user_count].is_online = is_online;
            user_count++;
        }

        line = strtok(NULL, "\n");
    }

    return user_count;
}

void update_user_list_box(User *user_list, int user_count) {
    GtkListBox *list_box = GTK_LIST_BOX(gtk_builder_get_object(builder, "user_list_box"));
    if (!list_box) {
        g_print("Cannot find user list box\n");
        return;
    }

    // Remove existing rows
    GtkListBoxRow *row;
    while ((row = gtk_list_box_get_row_at_index(list_box, 0)) != NULL) {
        gtk_container_remove(GTK_CONTAINER(list_box), GTK_WIDGET(row));
    }

    // Add new rows
    for (int i = 0; i < user_count; i++) {
        // Create row container
        GtkWidget *row_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
        gtk_widget_set_margin_start(row_box, 5);
        gtk_widget_set_margin_end(row_box, 5);
        gtk_widget_set_margin_top(row_box, 5);
        gtk_widget_set_margin_bottom(row_box, 5);

        // Create labels for user info
        char user_info[256];
        snprintf(user_info, sizeof(user_info), "%s (Score: %d) %s",
                user_list[i].username,
                user_list[i].score,
                user_list[i].is_online ? "Online" : "Offline");

        GtkWidget *label = gtk_label_new(user_info);
        gtk_widget_set_halign(label, GTK_ALIGN_START);
        gtk_box_pack_start(GTK_BOX(row_box), label, TRUE, TRUE, 0);

        // Create row and add container
        GtkWidget *row = gtk_list_box_row_new();
        gtk_container_add(GTK_CONTAINER(row), row_box);
        gtk_list_box_insert(list_box, row, -1);

        // Show all widgets
        gtk_widget_show_all(row);
    }

    // Show the list box
    gtk_widget_show_all(GTK_WIDGET(list_box));
}

void handle_list_user(Message *msg)
{   if(msg->status != SUCCESS)
    {
        g_print("Error in list user response\n");
        return;
    }
    User user_list[10];
    int user_count = parse_user_list(msg->payload, user_list, 10);

    // Hiển thị danh sách người dùng đã phân tích
    printf("Received %d users:\n", user_count);
    for (int i = 0; i < user_count; i++) {
        printf("ID: %d, Username: %s, Score: %d, Online: %d\n",
               user_list[i].id, user_list[i].username, user_list[i].score, user_list[i].is_online);
    }
    printf("Create user list box\n");
    GtkWidget *user_list_box = GTK_WIDGET(gtk_builder_get_object(builder, "user_list_box"));
    if (user_list_box == NULL)
    {
        g_print("Cannot find user list box\n");
        return;
    }
    printf("Build user list box\n");
    update_user_list_box(user_list, user_count);
}

void handle_challange_request(Message *msg)
{
   // Extract challenger name
    char challenger[50], challenged[50];
    sscanf(msg->payload, "CHALLANGE_REQUEST|%[^|]|%s", challenger, challenged);

    // Show dialog only if we're the challenged player
    if (strcmp(challenged, client_name) == 0) {
        GtkWidget *dialog = gtk_message_dialog_new(GTK_WINDOW(window),
                                                 GTK_DIALOG_MODAL,
                                                 GTK_MESSAGE_QUESTION,
                                                 GTK_BUTTONS_YES_NO,
                                                 "Game challenge from %s. Accept?",
                                                 challenger);

        int response = gtk_dialog_run(GTK_DIALOG(dialog));
        gtk_widget_destroy(dialog);

        // Send response
        Message resp;
        resp.message_type = CHALLANGE_RESPONSE;
        if (response == GTK_RESPONSE_YES) {
            sprintf(resp.payload, "CHALLANGE_RESPONSE|%s|%s|ACCEPT", challenger, challenged);
        } else {
            sprintf(resp.payload, "CHALLANGE_RESPONSE|%s|%s|REJECT", challenger, challenged);
        }
        queue_push(&send_queue, &resp);
    }
}
void handle_challange_response(Message *msg)
{
    char challenger[50], challenged[50], response[10];
    sscanf(msg->payload, "CHALLANGE_RESPONSE|%[^|]|%[^|]|%s",
           challenger, challenged, response);

    if (strcmp(response, "ACCEPT") == 0) {
        // Start game if we're the challenger
        if (strcmp(challenger, client_name) == 0) {
            Message game_msg;
            game_msg.message_type = GAME_START;
            sprintf(game_msg.payload, "%s|%s", challenger, challenged);

            queue_push(&send_queue, &game_msg);
        }else{
            Message game_msg;
            game_msg.message_type = GAME_START;
            sprintf(game_msg.payload, "%s|%s", challenged, challenger);

            queue_push(&send_queue, &game_msg);
        }
    } else {
        if (strcmp(challenger, client_name) == 0) {
            show_error_dialog("Challenge rejected");
        }
    }
}

int parse_game_history_list(const char *payload, GameHistory *game_history_list, int max_games) {
  char *payload_copy = strdup(payload); // Duplicate payload to safely tokenize
  if (!payload_copy) {
    g_print("Memory allocation failed for payload copy.\n");
    return 0;
  }

  int game_count = 0;
  char *line = strtok(payload_copy, "\n"); // Each line represents a game history

  while (line != NULL && game_count < max_games) {
    GameHistory *current = &game_history_list[game_count];
    memset(current, 0, sizeof(GameHistory)); // Clear memory for safety

    // Parse the game history line
    sscanf(line, "Game ID: %19s | %49s vs %49s | Winner: %49s | Score: %d-%d",
           current->game_id, current->player1, current->player2,
           current->winner, &current->player1_score, &current->player2_score);

    game_count++;
    line = strtok(NULL, "\n"); // Get the next line
  }

  free(payload_copy); // Free the duplicated payload
  return game_count;
}

void update_history_list_box(GameHistory *game_history_list, int game_count) {
  GtkListBox *list_box = GTK_LIST_BOX(gtk_builder_get_object(builder, "history_list_box"));
  if (!list_box) {
    g_print("Cannot find history list box\n");
    return;
  }

  // Remove existing rows
  GtkListBoxRow *row;
  while ((row = gtk_list_box_get_row_at_index(list_box, 0)) != NULL) {
    gtk_container_remove(GTK_CONTAINER(list_box), GTK_WIDGET(row));
  }

  // Add new rows
  for (int i = 0; i < game_count; i++) {
    // Create row container
    GtkWidget *row_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    gtk_widget_set_margin_start(row_box, 5);
    gtk_widget_set_margin_end(row_box, 5);
    gtk_widget_set_margin_top(row_box, 5);
    gtk_widget_set_margin_bottom(row_box, 5);

    // Create labels for game history
    char game_info[256];
    snprintf(game_info, sizeof(game_info), "Game ID: %s | %s vs %s | Winner: %s | Score: %d-%d",
             game_history_list[i].game_id, game_history_list[i].player1, game_history_list[i].player2,
             game_history_list[i].winner, game_history_list[i].player1_score, game_history_list[i].player2_score);

    GtkWidget *label = gtk_label_new(game_info);
    gtk_widget_set_halign(label, GTK_ALIGN_START);
    gtk_box_pack_start(GTK_BOX(row_box), label, TRUE, TRUE, 0);

    // Create row and add container
    GtkWidget *row = gtk_list_box_row_new();
    gtk_container_add(GTK_CONTAINER(row), row_box);
    gtk_list_box_insert(list_box, row, -1);

    // Show all widgets
    gtk_widget_show_all(row);
  }

  // Show the list box
  gtk_widget_show_all(GTK_WIDGET(list_box));
}

void handle_history_list_response(Message *msg) {
    if (msg->status != SUCCESS) {
        g_print("Error in fetching history: %s\n", msg->payload);
        show_error_dialog("Failed to retrieve game history.");
        return;
    }

    // Parse the game history list
    GameHistory game_history_list[10];
    int game_count = parse_game_history_list(msg->payload, game_history_list, 10);

    // Debug: Print the retrieved history
    printf("Received %d game histories:\n", game_count);
    for (int i = 0; i < game_count; i++) {
        printf("Game ID: %s | Players: %s vs %s | Winner: %s\n",
                game_history_list[i].game_id, game_history_list[i].player1,
                game_history_list[i].player2, game_history_list[i].winner);
    }

    // Update the history list box
    update_history_list_box(game_history_list, game_count);
}

void on_view_history_clicked(GtkButton *button, gpointer user_data) {
  g_print("View history clicked\n");

  // Get the history_list_box widget
  GtkListBox *list_box = GTK_LIST_BOX(gtk_builder_get_object(builder, "history_list_box"));
  if (!list_box) {
    g_print("Cannot find history list box\n");
    return;
  }

  // Get the selected row
  GtkListBoxRow *selected_row = gtk_list_box_get_selected_row(list_box);
  if (!selected_row) {
    show_error_dialog("Please select a game first.");
    return;
  }

  // Get the box inside the row
  GtkWidget *box = gtk_bin_get_child(GTK_BIN(selected_row));
  if (!GTK_IS_BOX(box)) {
    g_print("Selected row does not contain a GtkBox\n");
    return;
  }

  // Get the label inside the box
  GList *children = gtk_container_get_children(GTK_CONTAINER(box));
  if (!children) {
    g_print("No children in the selected row's box\n");
    return;
  }

  GtkWidget *label = GTK_WIDGET(children->data); // First child is the label
  g_list_free(children);

  if (!GTK_IS_LABEL(label)) {
    g_print("First child of box is not a label\n");
    return;
  }

  const gchar *label_text = gtk_label_get_text(GTK_LABEL(label));
  if (!label_text) {
    g_print("Label text is null\n");
    return;
  }

  // Extract the game ID from the label text
  char game_id[20] = {0};
  sscanf(label_text, "Game ID: %19s", game_id);

  // Send GAME_DETAIL_REQUEST to the server
  Message message;
  message.message_type = GAME_DETAIL_REQUEST;
  snprintf(message.payload, sizeof(message.payload), "%s", game_id);

  queue_push(&send_queue, &message); // Push the request to the send queue
  g_print("Game detail request sent for Game ID: %s\n", game_id);
}

void on_send_challenge_clicked(GtkButton *button, gpointer user_data)
{
    printf("Send challenge clicked\n");
    GtkStack *stack = GTK_STACK(user_data);
   // Get list box widget
    GtkListBox *list_box = GTK_LIST_BOX(gtk_builder_get_object(builder, "user_list_box"));
    if (!list_box) {
        g_print("Cannot find user list box\n");
        return;
    }

    // Get selected row
    GtkListBoxRow *selected_row = gtk_list_box_get_selected_row(list_box);
    if (!selected_row) {
        show_error_dialog("Please select an opponent first");
        return;
    }

    // Get label from row box
    GtkWidget *box = gtk_bin_get_child(GTK_BIN(selected_row));
    GtkWidget *label = gtk_container_get_children(GTK_CONTAINER(box))->data;
    const gchar *label_text = gtk_label_get_text(GTK_LABEL(label));

    // Extract username from label text (format: "username (Score: X) status")
    char opponent_name[50] = {0};
    sscanf(label_text, "%[^ (]", opponent_name);

    if (strcmp(opponent_name, client_name) == 0) {
        show_error_dialog("Cannot challenge yourself!");
        return;
    }

    // Send challenge request
    Message msg;
    msg.message_type = CHALLANGE_REQUEST;
    sprintf(msg.payload, "CHALLANGE_REQUEST|%s|%s", client_name, opponent_name);

    queue_push(&send_queue, &msg);
    char *dialog_message = g_strdup_printf("Challenge sent to %s", opponent_name);
    show_dialog(dialog_message);
}

void on_send_rechallenge_clicked(GtkButton *button, gpointer user_data) {
  printf("Send rechallenge clicked\n");

  // Get the history_list_box widget
  GtkListBox *list_box = GTK_LIST_BOX(gtk_builder_get_object(builder, "history_list_box"));
  if (!list_box) {
    g_print("Cannot find history list box\n");
    return;
  }

  // Get the selected row
  GtkListBoxRow *selected_row = gtk_list_box_get_selected_row(list_box);
  if (!selected_row) {
    show_error_dialog("Please select a game first");
    return;
  }

  // Get the child box inside the selected row
  GtkWidget *box = gtk_bin_get_child(GTK_BIN(selected_row));
  if (!GTK_IS_BOX(box)) {
    g_print("Selected row does not contain a GtkBox\n");
    return;
  }

  // Get the first child (label) inside the box
  GList *children = gtk_container_get_children(GTK_CONTAINER(box));
  if (!children) {
    g_print("No children in the selected row's box\n");
    return;
  }

  GtkWidget *label = GTK_WIDGET(children->data);
  g_list_free(children);

  if (!GTK_IS_LABEL(label)) {
    g_print("First child of the box is not a GtkLabel\n");
    return;
  }

  const gchar *label_text = gtk_label_get_text(GTK_LABEL(label));
  if (!label_text || strlen(label_text) == 0) {
    show_error_dialog("Invalid selection. No game data found.");
    return;
  }

  // Debug: Print the label text
  g_print("Selected Label Text: %s\n", label_text);

  // Extract the opponent's username
  char player1[50] = {0};
  char player2[50] = {0};

  if (sscanf(label_text, "Game ID: %*[^|] | %49[^ ] vs %49[^ ] | %*s", player1, player2) != 2) {
    show_error_dialog("Failed to extract player names from selected game.");
    return;
  }

  // Determine opponent name based on current client_name
  char opponent_name[50] = {0};
  if (strcmp(client_name, player1) == 0) {
    strncpy(opponent_name, player2, sizeof(opponent_name) - 1);
  } else if (strcmp(client_name, player2) == 0) {
    strncpy(opponent_name, player1, sizeof(opponent_name) - 1);
  } else {
    show_error_dialog("You were not a participant in the selected game.");
    return;
  }

  // Validate that the opponent is not the client itself
  if (strcmp(opponent_name, client_name) == 0) {
    show_error_dialog("Cannot challenge yourself!");
    return;
  }

  // Send rechallenge request
  Message msg;
  msg.message_type = CHALLANGE_REQUEST;
  snprintf(msg.payload, sizeof(msg.payload), "CHALLANGE_REQUEST|%s|%s", client_name, opponent_name);

  queue_push(&send_queue, &msg);
  char *dialog_message = g_strdup_printf("Rechallenge sent to %s", opponent_name);
  show_dialog(dialog_message);
}

// UI thread network response handler
gboolean process_network_response(gpointer data)
{
    Message msg;
    while (queue_pop(&receive_queue, &msg) == 0)
    {
        switch (msg.message_type)
        {
        case GAME_START:
            handle_game_start_response(&msg);
            break;
        case GAME_GUESS:
            handle_game_guess_response(&msg);
            break;
        case GAME_TURN:
            handle_game_turn_response(&msg);
            break;
        case GAME_GET_TARGET:
            handle_game_get_target_response(&msg);
            break;
        case LIST_USER:
            // Handle user list response
            handle_list_user(&msg);
            break;
        case CHALLANGE_REQUEST:
            handle_challange_request(&msg);
            break;
        case CHALLANGE_RESPONSE:
            handle_challange_response(&msg);
            break;
        case GAME_SCORE:
            handle_score_update(&msg);
            break;
        case LIST_GAME_HISTORY:
            handle_history_list_response(&msg);
            break;
        case GAME_DETAIL_REQUEST:
            handle_game_detail_response(&msg);
            break;
        }
    }
    return G_SOURCE_REMOVE;
}


void *network_thread_func(void *arg) {
    int sockfd = init_tcp_socket("127.0.0.1", 8080);
    if (sockfd < 0) {
        return NULL;
    }

    // Set socket to non-blocking mode
    int flags = fcntl(sockfd, F_GETFL, 0);
    fcntl(sockfd, F_SETFL, flags | O_NONBLOCK);

    fd_set read_fds, write_fds;
    struct timeval tv;

    while (network_running) {
        FD_ZERO(&read_fds);
        FD_ZERO(&write_fds);
        FD_SET(sockfd, &read_fds);

        // Only add to write set if we have messages to send
        Message send_msg;
        bool has_message = (queue_pop(&send_queue, &send_msg) == 0);
        if (has_message) {
            FD_SET(sockfd, &write_fds);
        }

        tv.tv_sec = 0;
        tv.tv_usec = 100000; // 100ms timeout

        int activity = select(sockfd + 1, &read_fds, &write_fds, NULL, &tv);

        if (activity < 0) {
            if (errno != EINTR) {
                perror("select error");
                break;
            }
            continue;
        }

        // Handle incoming messages
        if (FD_ISSET(sockfd, &read_fds)) {
            Message recv_msg;
            if (receive_message(sockfd, &recv_msg) == 0) {
                queue_push(&receive_queue, &recv_msg);
                gdk_threads_add_idle(process_network_response, NULL);
            }
        }

        // Handle outgoing messages
        if (has_message && FD_ISSET(sockfd, &write_fds)) {
            if (send_message(sockfd, &send_msg) < 0) {
                g_print("Failed to send message\n");
            }
        }

        usleep(10000); // Small sleep to prevent CPU hogging
    }

    close(sockfd);
    return NULL;
}

// Modified send_request function for async operation
void async_send_request(enum MessageType type, const char *payload)
{
    Message msg;
    msg.message_type = type;
    strncpy(msg.payload, payload, BUFFER_SIZE);
    printf("Sending message of type %d, content: %s\n", msg.message_type, msg.payload);
    queue_push(&send_queue, &msg);
}

// Initialize networking
void init_networking()
{
    init_message_queue(&send_queue);
    init_message_queue(&receive_queue);
    // Start network thread
    if (pthread_create(&network_thread, NULL, network_thread_func, NULL) != 0)
    {
        g_printerr("Failed to create network thread\n");
        return;
    }
    printf("Network thread created\n");
}
void disconnect_from_server(int sockfd)
{
    // Đóng kết nối socket
    if (sockfd >= 0)
    {
        close(sockfd);
        printf("Disconnected from server.\n");
    }
}
// Cleanup networking
void cleanup_networking(int sockfd)
{
    network_running = 0;
    pthread_join(network_thread, NULL);
    disconnect_from_server(sockfd);
}

void on_GoToSignup_clicked(GtkButton *button, gpointer user_data)
{
    GtkStack *stack = GTK_STACK(user_data);
    gtk_stack_set_visible_child_name(stack, "signup");
}

void on_GoToLogin_clicked(GtkButton *button, gpointer user_data)
{
    GtkStack *stack = GTK_STACK(user_data);
    gtk_stack_set_visible_child_name(stack, "login");
}

void on_LoginSubmit_clicked(GtkButton *button, gpointer user_data)
{
    GtkStack *stack = GTK_STACK(user_data);
    GtkEntry *input_field;

  // Null check for builder
    if (!builder) {
        g_print("Builder is null\n");
        return;
    }

    // Get username field
    input_field = GTK_ENTRY(gtk_builder_get_object(builder, "LoginUsernameEntry"));
    if (!input_field) {
        g_print("Cannot find username entry\n");
        return;
    }
    const gchar *username = gtk_entry_get_text(input_field);
    printf("Username: %s\n", username);
    // Get password field
    input_field = GTK_ENTRY(gtk_builder_get_object(builder, "LoginPasswordEntry"));
    if (!input_field) {
        g_print("Cannot find password entry\n");
        return;
    }
    const gchar *password = gtk_entry_get_text(input_field);
    printf("Password: %s\n", password);
    char username_buf[50];
    char password_buf[50];
    memset(username_buf, 0, sizeof(username_buf));  // Clear buffer first
    strncpy(username_buf, username, 50 - 1);  // Copy safely with size limit
    username_buf[50 - 1] = '\0';  // Ensure null termination
    // Clear buffer first
    memset(password_buf, 0, sizeof(password_buf));

    // Copy safely with size limit
    strncpy(password_buf, password, 50 - 1);

    // Ensure null termination
    password_buf[50 - 1] = '\0';
    // Validate input
    if (strlen(username_buf) == 0 || strlen(password_buf) == 0) {
        printf("Username or password cannot be empty\n");
        GtkWidget *dialog = gtk_message_dialog_new(GTK_WINDOW(window),
                                                 GTK_DIALOG_MODAL,
                                                 GTK_MESSAGE_ERROR,
                                                 GTK_BUTTONS_OK,
                                                 "Username or password cannot be empty!");
        gtk_dialog_run(GTK_DIALOG(dialog));
        gtk_widget_destroy(dialog);
        return;  // Return after showing error
    }
    printf("Sending login request\n");
    Message message;
    message.message_type = LOGIN_REQUEST;
    sprintf(message.payload, "%s|%s", username_buf, password_buf);
    queue_push(&send_queue, &message);
    int timeout = 0;
    Message response;
    while (timeout < 10)
    { // Try for 1 second
        if (queue_pop(&receive_queue, &response) == 0)
        {
            if (response.status == SUCCESS)
            {
                // Safe string copy
                memset(client_name, 0, sizeof(client_name));  // Clear buffer first
                strncpy(client_name, username_buf, sizeof(client_name) - 1);  // Leave room for null terminator
                memset(client_password, 0, sizeof(client_password));  // Clear buffer first
                strncpy(client_password, password_buf, sizeof(client_password) - 1);
                printf("Client name: %s\n", client_name);

                // Switch to homepage
                if (stack) {
                    Message message;
                    message.message_type = LIST_USER;
                    sprintf(message.payload, "%s", "");
                    queue_push(&send_queue, &message);
                    gtk_stack_set_visible_child_name(stack, "homepage");
                } else {
                    g_print("Stack widget is null\n");
                }
            }
            else{
                show_error_dialog(response.payload);
            }
            break;
        }
        usleep(100000); // Wait 100ms
        timeout++;
    }
    if (timeout >= 10)
    {
        show_error_dialog("Login failed: Timeout");
    }
}

void on_SignupSubmit_clicked(GtkButton *button, gpointer user_data){
    GtkStack *stack = GTK_STACK(user_data);
    GtkEntry *input_field;

    if (!builder) {
        g_print("Builder is null\n");
        return;
    }

    input_field = GTK_ENTRY(gtk_builder_get_object(builder, "SignupUsernameEntry"));
    if (!input_field) {
        g_print("Cannot find username entry\n");
        return;
    }
    const gchar *username = gtk_entry_get_text(input_field);

    input_field = GTK_ENTRY(gtk_builder_get_object(builder, "SignupPasswordEntry"));
    if (!input_field) {
        g_print("Cannot find password entry\n");
        return;
    }
    const gchar *password = gtk_entry_get_text(input_field);

    input_field = GTK_ENTRY(gtk_builder_get_object(builder, "SignupConfirmPassword"));
    if (!input_field) {
        g_print("Cannot find confirm password entry\n");
        return;
    }
    const gchar *confirm_password = gtk_entry_get_text(input_field);

    if (strlen(username) == 0) {
        show_error_dialog("Username cannot be empty!");
        return;
    }
    if (strlen(password) == 0) {
        show_error_dialog("Password cannot be empty!");
        return;
    }
    if (strlen(confirm_password) == 0) {
        show_error_dialog("Confirm password cannot be empty!");
        return;
    }
    if (strcmp(password, confirm_password) != 0) {
        show_error_dialog("Passwords do not match!");
        return;
    }

    printf("Sending signup request\n");
    Message message;
    message.message_type = SIGNUP_REQUEST;
    sprintf(message.payload, "%s|%s", username, password);
    queue_push(&send_queue, &message);

    int timeout = 0;
    Message response;
    while (timeout < 10)
    {
        if (queue_pop(&receive_queue, &response) == 0)
        {
            if (response.status == SUCCESS)
            {
                printf("Signup successful\n");

                show_dialog("Signup successful! Please log in.");

                if (stack) {
                    gtk_stack_set_visible_child_name(stack, "loginpage");
                } else {
                    g_print("Stack widget is null\n");
                }
            }
            else {
                show_error_dialog(response.payload);
            }
            return;
        }
        usleep(100000);
        timeout++;
    }

    if (timeout >= 10) {
        show_error_dialog("Signup failed: Timeout");
    }
}

// Add new function for resetting game state
void reset_game_state()
{
    current_row = 0;
    current_col = 0;
    memset(current_word, 0, sizeof(current_word));

    // Reset grid
    for (int i = 0; i < MAX_ATTEMPTS; i++)
    {
        for (int j = 0; j < WORD_LENGTH; j++)
        {
            gtk_label_set_text(game_grid[i][j], "");
            GtkStyleContext *context = gtk_widget_get_style_context(GTK_WIDGET(game_grid[i][j]));
            gtk_style_context_remove_class(context, "correct");
            gtk_style_context_remove_class(context, "wrong-position");
            gtk_style_context_remove_class(context, "wrong");
        }
    }
}

// Modify on_submit_word_clicked to use message queue
void on_submit_word_clicked(GtkButton *button, gpointer user_data)
{
    const gchar *word = gtk_entry_get_text(word_entry);

    if (strlen(word) != WORD_LENGTH)
    {
        g_print("Invalid word length\n");
        show_error_dialog("Word length is invalid! Please enter a word of length 5.");
        return;
    }

    Message message;
    message.message_type = GAME_GUESS;
    sprintf(message.payload, "%d|%s|%s", game_session_id, client_name ,word);

    // Push to send queue instead of direct socket send
    queue_push(&send_queue, &message);

    // Disable input until response
    gtk_widget_set_sensitive(word_entry, FALSE);
    gtk_widget_set_sensitive(submit_button, FALSE);
}

// Modify init_game_state to only request target after game start
void init_game_state(int session_id)
{
    if (session_id != -1)
    {
        Message message;
        message.message_type = GAME_GET_TARGET;
        sprintf(message.payload, "%d", session_id);
        queue_push(&send_queue, &message);
    }
    reset_game_state();
}

// Initialize the game board
void init_game_board(GtkBuilder *builder)
{
    printf("init_game_board\n"); // Debug print
    // Get the grid widget from builder
    GtkWidget *grid = GTK_WIDGET(gtk_builder_get_object(builder, "wordle_grid"));
    if (!grid)
    {
        g_printerr("Failed to get wordle_grid from builder\n");
        return;
    }
    printf("grid: %p\n", grid); // Debug print

    // Verify it's a grid
    if (!GTK_IS_GRID(grid))
    {
        g_printerr("wordle_grid is not a GtkGrid widget\n");
        return;
    }

    // Get entry widget reference
    word_entry = GTK_ENTRY(gtk_builder_get_object(builder, "word_entry"));
    if (!word_entry)
    {
        g_printerr("Failed to get word_entry from builder\n");
        return;
    }
    printf("word_entry: %p\n", word_entry); // Debug print
    // Get submit button
    submit_button = GTK_WIDGET(gtk_builder_get_object(builder, "submit_word"));
    if (!submit_button)
    {
        g_printerr("Failed to get submit_button from builder\n");
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
    for (int i = 0; i < MAX_ATTEMPTS; i++)
    {
        for (int j = 0; j < WORD_LENGTH; j++)
        {
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
    reset_game_state();
}

// Add CSS styles for the game
void add_css_styles()
{
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

void on_PlayGame_clicked(GtkButton *button, gpointer user_data)
{
    printf("PlayGame clicked\n");
    GtkStack *stack = GTK_STACK(user_data);

     // Get opponent entry widget
    GtkEntry *opponent_entry = GTK_ENTRY(gtk_builder_get_object(builder, "opponent_name_entry"));
    if (!opponent_entry) {
        g_print("Cannot find opponent entry\n");
        return;
    }

    const gchar *opponent_name = gtk_entry_get_text(opponent_entry);

    // Validate opponent name
    if (!opponent_name || strlen(opponent_name) == 0) {
        show_error_dialog("Please enter the opponent's name.");
        return;
    }
    // Initialize message properly
    Message message;
    memset(&message, 0, sizeof(Message)); // Clear message structure
    message.message_type = GAME_START;
    message.status = 0;          // No status for initial request
     // Payload chứa tên player và tên đối thủ
    snprintf(message.payload, sizeof(message.payload), "%s|%s", client_name, opponent_name);
    // Push game start request to send queue
    queue_push(&send_queue, &message);
}

void on_BackToHome_clicked(GtkButton *button, gpointer user_data)
{
    GtkStack *stack = GTK_STACK(user_data);
    gtk_stack_set_visible_child_name(stack, "homepage");
}

void on_GoToHistory_clicked(GtkButton *button, gpointer user_data)
{
    GtkStack *stack = GTK_STACK(user_data);

    Message message;
    message.message_type = LIST_GAME_HISTORY;
    snprintf(message.payload, sizeof(message.payload), "%s", client_name);

    queue_push(&send_queue, &message);

    gtk_stack_set_visible_child_name(stack, "history");

    g_print("LIST_HISTORY request sent for user: %s\n", client_name);
}

void on_Logout_clicked(GtkButton *button, gpointer user_data)
{
    Message message;
    memset(&message, 0, sizeof(Message));
    message.message_type = LOGOUT_REQUEST;
    message.status = 0;
    snprintf(message.payload, sizeof(message.payload), "%s|%s", client_name, client_password);
    queue_push(&send_queue, &message);

    int timeout = 0;
    Message response;
    while (timeout < 10)
    {
        if (queue_pop(&receive_queue, &response) == 0)
        {
            if (response.status == SUCCESS)
            {
                if (stack) {
                    GtkStack *stack = GTK_STACK(user_data);
                    gtk_stack_set_visible_child_name(stack, "login");
                } else {
                    g_print("Stack widget is null\n");
                }
            }
            else{
                show_error_dialog(response.payload);
            }
            break;
        }
        usleep(100000);
        timeout++;
    }
    if (timeout >= 10)
    {
        show_error_dialog("Logout failed: Timeout");
    }
}

void set_signal_connect()
{
    GtkWidget *button;

    button = GTK_WIDGET(gtk_builder_get_object(builder, "GoToSignup"));
    if (button)
    {
        g_signal_connect(button, "clicked", G_CALLBACK(on_GoToSignup_clicked), stack);
    }

    button = GTK_WIDGET(gtk_builder_get_object(builder, "GoToLogin"));
    if (button)
    {
        g_signal_connect(button, "clicked", G_CALLBACK(on_GoToLogin_clicked), stack);
    }

    button = GTK_WIDGET(gtk_builder_get_object(builder, "LoginSubmit"));
    if (button)
    {
        g_signal_connect(button, "clicked", G_CALLBACK(on_LoginSubmit_clicked), stack);
    }

    button = GTK_WIDGET(gtk_builder_get_object(builder, "SignupSubmit"));
    if (button)
    {
        g_signal_connect(button, "clicked", G_CALLBACK(on_SignupSubmit_clicked), stack);
    }

    button = GTK_WIDGET(gtk_builder_get_object(builder, "PlayGame"));
    if (button)
    {
        g_signal_connect(button, "clicked", G_CALLBACK(on_PlayGame_clicked), stack);
    }

    button = GTK_WIDGET(gtk_builder_get_object(builder, "send_challenge_button"));
    if (button)
    {
        g_signal_connect(button, "clicked", G_CALLBACK(on_send_challenge_clicked), stack);
    }

    button = GTK_WIDGET(gtk_builder_get_object(builder, "BackToHome1"));
    if (button)
    {
        g_signal_connect(button, "clicked", G_CALLBACK(on_BackToHome_clicked), stack);
    }

    button = GTK_WIDGET(gtk_builder_get_object(builder, "BackToHome2"));
    if (button)
    {
        g_signal_connect(button, "clicked", G_CALLBACK(on_BackToHome_clicked), stack);
    }

    button = GTK_WIDGET(gtk_builder_get_object(builder, "GoToHistory"));
    if (button)
    {
        g_signal_connect(button, "clicked", G_CALLBACK(on_GoToHistory_clicked), stack);
    }

    button = GTK_WIDGET(gtk_builder_get_object(builder, "Logout"));
    if (button)
    {
        g_signal_connect(button, "clicked", G_CALLBACK(on_Logout_clicked), stack);
    }

    button = GTK_WIDGET(gtk_builder_get_object(builder, "view_history_button"));
    if (button)
    {
        g_signal_connect(button, "clicked", G_CALLBACK(on_view_history_clicked), stack);
    }

    button = GTK_WIDGET(gtk_builder_get_object(builder, "send_rechallenge_button"));
    if (button)
    {
        g_signal_connect(button, "clicked", G_CALLBACK(on_send_rechallenge_clicked), stack);
    }
}

int main(int argc, char *argv[])
{

    gtk_init(&argc, &argv);

    builder = gtk_builder_new_from_file("wordle.glade");
    if (!builder)
    {
        g_printerr("Failed to load Glade file\n");
        return 1;
    }

    window = GTK_WIDGET(gtk_builder_get_object(builder, "main_window"));
    if (!window)
    {
        g_printerr("Failed to get main_window from Glade file\n");
        return 1;
    }

    stack = GTK_STACK(gtk_builder_get_object(builder, "stack"));
    if (!stack)
    {
        g_printerr("Failed to get stack from Glade file\n");
        return 1;
    }
    printf("init networking\n");
    init_networking(); // Initialize networking
    printf("init game board\n");
    // Insert the two functions here
    init_game_board(builder); // Initialize game board first
    add_css_styles();         // Then add CSS styles
    set_signal_connect();

    gtk_widget_show_all(window);
    gtk_main();
    cleanup_networking(sockfd); // Cleanup networking
    return 0;
}
