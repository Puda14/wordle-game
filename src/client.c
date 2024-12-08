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
            gtk_label_set_text(GTK_LABEL(game_status_label), "Game over!");
            gtk_widget_set_sensitive(word_entry, FALSE);
            gtk_widget_set_sensitive(submit_button, FALSE);
            return;
        }

        // Cập nhật lượt chơi
        if (turn == 1 && player_num == 1) {
            gtk_widget_set_sensitive(word_entry, TRUE);
            gtk_widget_set_sensitive(submit_button, TRUE);
        } else if (turn == 2 && player_num == 2) {
            gtk_widget_set_sensitive(word_entry, TRUE);
            gtk_widget_set_sensitive(submit_button, TRUE);
        } else {
            gtk_widget_set_sensitive(word_entry, FALSE);
            gtk_widget_set_sensitive(submit_button, FALSE);
        }
    }
}

void handle_game_guess_response(Message *msg)
{
    // Kiểm tra trạng thái thành công của thông điệp
    if (msg->status != SUCCESS)
    {
        g_print("Error in game guess response\n");
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
        if (current_player == player_number) {
            show_dialog("You Win! Congratulations!");
        } else {
            show_dialog("Player X Wins! Better luck next time.");
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
        
        }
    }
    return G_SOURCE_REMOVE;
}

// Network thread function
// void *network_thread_func(void *arg)
// {
//     int sockfd = init_tcp_socket("127.0.0.1", 8080); // Kết nối tới máy chủ với IP và cổng tương ứng
//     if (sockfd < 0)
//     {
//         return NULL; // Nếu kết nối thất bại, thoát khỏi luồng
//     }
//     printf("Network thread with %d\n", sockfd);
//     while (network_running)
//     {
//         // Check for messages to send
//         Message send_msg;
//         if (queue_pop(&send_queue, &send_msg) == 0)
//         {
//             if (send_message(sockfd, &send_msg) < 0)
//             {
//                 // Handle send error
//                 continue;
//             }

//             // Wait for response
//             Message recv_msg;
//             if (receive_message(sockfd, &recv_msg) == 0)
//             {
//                 queue_push(&receive_queue, &recv_msg);

//                 // Signal UI update needed
//                 gdk_threads_add_idle(process_network_response, NULL);
//             }
//         }


//         usleep(10000); // Sleep to prevent busy waiting
//     }
//     close(sockfd); // Đóng kết nối khi luồng kết thúc

//     return NULL;
// }

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
    // Safe string copy
    memset(client_name, 0, sizeof(client_name));  // Clear buffer first
    strncpy(client_name, username_buf, sizeof(client_name) - 1);  // Leave room for null terminator
    printf("Client name: %s\n", client_name);
    Message message;
    message.message_type = LOGIN_REQUEST;
    sprintf(message.payload, "%s", client_name);
    queue_push(&send_queue, &message);
    int timeout = 0;
    Message response;
    while (timeout < 10)
    { // Try for 1 second
        if (queue_pop(&receive_queue, &response) == 0)
        {
            if (response.status == SUCCESS)
            {
                // Switch to homepage
                if (stack) {
                    gtk_stack_set_visible_child_name(stack, "homepage");
                } else {
                    g_print("Stack widget is null\n");
                }
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

    // // Show waiting dialog
    // GtkWidget *dialog = gtk_message_dialog_new(GTK_WINDOW(window),
    //                                            GTK_DIALOG_MODAL,
    //                                            GTK_MESSAGE_INFO,
    //                                            GTK_BUTTONS_NONE,
    //                                            "Connecting to game...");
    // gtk_widget_show_all(dialog);

    // // Wait for response with timeout
    // int timeout = 0;
    // Message response;
    // while (timeout < 10)
    // { // Try for 1 second
    //     if (queue_pop(&receive_queue, &response) == 0)
    //     {
    //         if (response.status == SUCCESS)
    //         {
    //             // Parse game session info
    //             if (sscanf(response.payload, "%d", &game_session_id) == 1)
    //             {
    //                 printf("Game session started: id=%d\n",
    //                        game_session_id);
    //                 init_game_state(game_session_id);
    //                 gtk_stack_set_visible_child_name(stack, "game");
    //                 gtk_widget_destroy(dialog);
    //                 return;
    //             }
    //         }
    //         break;
    //     }
    //     usleep(100000); // Wait 100ms
    //     timeout++;
    // }

    // If we get here, connection failed
    // gtk_widget_destroy(dialog);
    // show_error_dialog("Failed to connect to game server");
}

void on_BackToHome_clicked(GtkButton *button, gpointer user_data)
{
    GtkStack *stack = GTK_STACK(user_data);
    gtk_stack_set_visible_child_name(stack, "homepage");
}

void on_GoToHistory_clicked(GtkButton *button, gpointer user_data)
{
    GtkStack *stack = GTK_STACK(user_data);
    gtk_stack_set_visible_child_name(stack, "history");
}

void on_Logout_clicked(GtkButton *button, gpointer user_data)
{
    GtkStack *stack = GTK_STACK(user_data);
    gtk_stack_set_visible_child_name(stack, "login");
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

    button = GTK_WIDGET(gtk_builder_get_object(builder, "PlayGame"));
    if (button)
    {
        g_signal_connect(button, "clicked", G_CALLBACK(on_PlayGame_clicked), stack);
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
