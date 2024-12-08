#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/select.h>
#include <time.h>

#define PORT 8080
#define MAX_USERS 100
#define BUFFER_SIZE 1024
#define MAX_CLIENTS 30
#define USER_FILE "users.txt"
#define MAX_WORDS 5000
#define WORD_LENGTH 5
#define MAX_ATTEMPTS 12
#define GAME_HISTORY_FILE "game_history.txt"

typedef struct {
    char game_id[20];      // Unique identifier for the game
    char player1[50];
    char player2[50];
    char word[WORD_LENGTH + 1];
    int result;            // 1: Player 1 wins, 2: Player 2 wins, 0: Draw
    char moves[MAX_ATTEMPTS][WORD_LENGTH + 1];
    int move_count;
} GameHistory;

GameHistory game_history[8000];
int history_count = 0;


typedef struct {
    uint8_t message_type;
    char payload[BUFFER_SIZE];
} Message;

typedef struct {
    char username[50];
    char password[50];
    int status;
    int elo;
    

} User;

User users[MAX_USERS];
int user_count = 0;

typedef struct {
    char challenger[50];  // Người thách đấu
    char opponent[50];    // Người bị thách đấu
    int status;           // 0: chưa phản hồi, 1: đã đồng ý, -1: đã từ chối
} Challenge;

Challenge challenges[MAX_USERS];  // Lưu trữ các lời thách đấu
int challenge_count = 0;  // Số lượng lời thách đấu
typedef struct {
    char player1[50];
    char player2[50];
    int player1_sock;
    int player2_sock;
    char target_word[WORD_LENGTH + 1];
    int current_turn;  // 0: player1's turn, 1: player2's turn
    int attempts;
    char moves[MAX_ATTEMPTS][WORD_LENGTH + 1];
    int move_count;
} GameSession;

GameSession active_games[MAX_USERS]; // Support up to MAX_USERS / 2 games
int active_game_count = 0;

typedef struct {
    char username[50];
    int client_sock;
} ReadyClient;

ReadyClient ready_clients[MAX_USERS];
int ready_count = 0;

enum MessageType { REGISTER_REQUEST = 0, LOGIN_REQUEST = 1, LOGOUT_REQUEST = 2, 
                   LIST_USERS_REQUEST = 3, CHALLENGE_REQUEST = 4, CHALLENGE_RESPONSE = 5,
                   START_GAME = 6, GUESS = 7, FEEDBACK = 8, WIN = 9, LOSE = 10, INVALID_GUESS = 11, END_GAME = 12 };

// Dữ liệu từ vựng Wordle
char valid_solutions[MAX_WORDS][WORD_LENGTH + 1];
char valid_guesses[MAX_WORDS][WORD_LENGTH + 1];
int solution_count = 0;
int guess_count = 0;

int register_user(const char *username, const char *password);
int login_user(const char *username, const char *password);
void logout_user(const char *username);
void handle_message(int client_sock, int client_socks[], Message *message);
void load_users_from_file();
void save_user_to_file(const User *user);
// Khai báo các hàm xử lý thách đấu
void handle_challenge_request(int client_sock, int client_socks[], Message *message);
void handle_challenge_response(int client_sock, int client_socks[], Message *message);

//Hàm xử lí trò chơi
int load_words(const char *filename, char words[][WORD_LENGTH + 1]);
void handle_start_game(int client_socks[]);
int is_valid_guess(const char *guess);
void get_feedback(const char *guess, const char *target, char *feedback);
void handle_start_game_request(int client_sock, const char *username) {
    // Check if the user is already in a game
    for (int i = 0; i < active_game_count; i++) {
        if (strcmp(active_games[i].player1, username) == 0 || strcmp(active_games[i].player2, username) == 0) {
            Message message;
            message.message_type = START_GAME;
            snprintf(message.payload, sizeof(message.payload), "You are already in a game.");
            send(client_sock, &message, sizeof(message), 0);
            return;
        }
    }

    // Add the user to the ready queue
    strcpy(ready_clients[ready_count].username, username);
    ready_clients[ready_count].client_sock = client_sock;
    ready_count++;

    // Match two players if available
    if (ready_count >= 2) {
        ReadyClient player1 = ready_clients[0];
        ReadyClient player2 = ready_clients[1];

        // Shift the queue
        for (int i = 0; i < ready_count - 2; i++) {
            ready_clients[i] = ready_clients[i + 2];
        }
        ready_count -= 2;

        // Create a new game session
        GameSession new_game = {0};
        strcpy(new_game.player1, player1.username);
        strcpy(new_game.player2, player2.username);
        new_game.player1_sock = player1.client_sock;
        new_game.player2_sock = player2.client_sock;
        new_game.current_turn = 0; // Player 1 starts
        new_game.attempts = 0;
        new_game.move_count = 0;

        // Select a random target word
        srand(time(NULL));
        strcpy(new_game.target_word, valid_solutions[rand() % solution_count]);

        active_games[active_game_count++] = new_game;

        printf("Game started between %s and %s\n", player1.username, player2.username);

        // Notify both players
        Message start_message;
        start_message.message_type = START_GAME;
        snprintf(start_message.payload, sizeof(start_message.payload), "Game started! Player 1's turn.");
        send(player1.client_sock, &start_message, sizeof(start_message), 0);
        snprintf(start_message.payload, sizeof(start_message.payload), "Game started! Waiting for Player 1.");
        send(player2.client_sock, &start_message, sizeof(start_message), 0);
    } else {
        // Notify the user they are waiting for an opponent
        Message message;
        message.message_type = START_GAME;
        snprintf(message.payload, sizeof(message.payload), "Waiting for an opponent...");
        send(client_sock, &message, sizeof(message), 0);
    }
}

void handle_game_message(int client_sock, Message *message, const char *username) {
    for (int i = 0; i < active_game_count; i++) {
        GameSession *game = &active_games[i];

        // Find the game session for this username
        if (strcmp(game->player1, username) == 0 || strcmp(game->player2, username) == 0) {
            int is_player1 = (strcmp(game->player1, username) == 0);

            if (message->message_type == GUESS) {
                // Ensure it's the player's turn
                if ((is_player1 && game->current_turn != 0) || (!is_player1 && game->current_turn != 1)) {
                    Message turn_message;
                    turn_message.message_type = FEEDBACK;
                    snprintf(turn_message.payload, sizeof(turn_message.payload), "It's not your turn.");
                    send(client_sock, &turn_message, sizeof(turn_message), 0);
                    return;
                }

                // Validate the guess
                char *guess = message->payload;
                if (!is_valid_guess(guess)) {
                    Message invalid_message;
                    invalid_message.message_type = INVALID_GUESS;
                    snprintf(invalid_message.payload, sizeof(invalid_message.payload), "Invalid guess. Try again.");
                    send(client_sock, &invalid_message, sizeof(invalid_message), 0);
                    return;
                }

                game->attempts++;
                strcpy(game->moves[game->move_count++], guess);

                char feedback[WORD_LENGTH + 1];
                get_feedback(guess, game->target_word, feedback);

                if (strcmp(guess, game->target_word) == 0) {
                    // Notify players of win/lose
                    Message win_message, lose_message;
                    win_message.message_type = WIN;
                    lose_message.message_type = LOSE;

                    snprintf(win_message.payload, sizeof(win_message.payload), "You guessed the word: %s", game->target_word);
                    snprintf(lose_message.payload, sizeof(lose_message.payload), "Your opponent guessed the word: %s", game->target_word);

                    send(game->player1_sock, is_player1 ? &win_message : &lose_message, sizeof(win_message));
                    send(game->player2_sock, is_player1 ? &lose_message : &win_message, sizeof(lose_message));

                    // Remove game session
                    active_games[i] = active_games[--active_game_count];
                    return;
                } else {
                    // Provide feedback and switch turns
                    Message feedback_message;
                    feedback_message.message_type = FEEDBACK;
                    snprintf(feedback_message.payload, sizeof(feedback_message.payload), "Feedback: %s", feedback);

                    send(game->player1_sock, &feedback_message, sizeof(feedback_message));
                    send(game->player2_sock, &feedback_message, sizeof(feedback_message));

                    game->current_turn = 1 - game->current_turn; // Switch turn
                }
            }
            break;
        }
    }
}


//Hàm xử lí lưu kết quả
void save_game_history(const GameHistory *history);

void generate_game_id(char *game_id, size_t size) {
    time_t now = time(NULL);
    snprintf(game_id, size, "GAME-%ld", now);
}

int main() {
    int server_sock, new_sock, client_socks[MAX_CLIENTS];
    struct sockaddr_in server_addr, client_addr;
    fd_set read_fds;
    socklen_t addr_len = sizeof(client_addr);

    // Load user data from file on server startup
    load_users_from_file();

    // Initialize client socket array
    for (int i = 0; i < MAX_CLIENTS; i++) client_socks[i] = 0;

    // Create socket
    server_sock = socket(AF_INET, SOCK_STREAM, 0);
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    bind(server_sock, (struct sockaddr *)&server_addr, sizeof(server_addr));
    listen(server_sock, 3);
    printf("Server listening on port %d\n", PORT);

    while (1) {
        FD_ZERO(&read_fds);
        FD_SET(server_sock, &read_fds);
        int max_sd = server_sock;

        for (int i = 0; i < MAX_CLIENTS; i++) {
            int sock = client_socks[i];
            if (sock > 0) FD_SET(sock, &read_fds);
            if (sock > max_sd) max_sd = sock;
        }

        select(max_sd + 1, &read_fds, NULL, NULL, NULL);

        // New connection
        if (FD_ISSET(server_sock, &read_fds)) {
            new_sock = accept(server_sock, (struct sockaddr *)&client_addr, &addr_len);
            printf("New connection accepted\n");

            for (int i = 0; i < MAX_CLIENTS; i++) {
                if (client_socks[i] == 0) {
                    client_socks[i] = new_sock;
                    printf("Added client socket to list: %d\n", new_sock);
                    break;
                }
            }
        }

        // Handle I/O for each client
        for (int i = 0; i < MAX_CLIENTS; i++) {
            int sock = client_socks[i];
            if (FD_ISSET(sock, &read_fds)) {
                Message message;
                int read_size = recv(sock, &message, sizeof(Message), 0);
                if (read_size == 0) {
                    close(sock);
                    client_socks[i] = 0;
                } else {
                    handle_message(sock, client_socks, &message);
                }
            }
        }
    }

    return 0;
}

// Function to handle registration request
void handle_register_request(int client_sock, Message *message) {
    char username[50], password[50];
    sscanf(message->payload, "%49[^|]|%49s", username, password);

    if (register_user(username, password)) {
        printf("User %s Pwd %s registered\n", username, password);
        strcpy(message->payload, "Registration successful\n");
    } else {
        strcpy(message->payload, "Registration failed: User may already exist\n");
    }
    send(client_sock, message, sizeof(Message), 0);
}

// Function to handle login request
void handle_login_request(int client_sock, Message *message) {
    char username[50], password[50];
    sscanf(message->payload, "%49[^|]|%49s", username, password);

    if (login_user(username, password)) {
        printf("User %s Pwd %s login\n", username, password);
        strcpy(message->payload, "Login successful\n");
    } else {
        strcpy(message->payload, "Login failed\n");
    }
    send(client_sock, message, sizeof(Message), 0);
}

// Function to handle logout request
void handle_logout_request(int client_sock, Message *message) {
    char username[50];
    sscanf(message->payload, "%49s", username);
    logout_user(username);
    printf("User %s logout\n", username);
    strcpy(message->payload, "Logout successful\n");
    send(client_sock, message, sizeof(Message), 0);
}

// Function to handle list users request
void handle_list_users_request(int client_sock, Message *message) {
    // Prepare the user list
    char user_list[BUFFER_SIZE] = "User List:\n";
    for (int i = 0; i < user_count; i++) {
        char user_info[100];
        snprintf(user_info, sizeof(user_info), "%s (Status: %d)\n", users[i].username, users[i].status);
        strncat(user_list, user_info, sizeof(user_list) - strlen(user_list) - 1);
    }
    strcpy(message->payload, user_list);
    send(client_sock, message, sizeof(Message), 0);
}

// Main function to handle all types of requests
void handle_message(int client_sock,int client_socks[], Message *message) {
    switch (message->message_type) {
        case REGISTER_REQUEST:
            handle_register_request(client_sock, message);
            break;
        case LOGIN_REQUEST:
            handle_login_request(client_sock, message);
            break;
        case LOGOUT_REQUEST:
            handle_logout_request(client_sock, message);
            break;
        case LIST_USERS_REQUEST:
            handle_list_users_request(client_sock, message);
            break;
        case CHALLENGE_REQUEST:
            handle_challenge_request(client_sock, client_socks, message);
            break;
        case CHALLENGE_RESPONSE:
            handle_challenge_response(client_sock, client_socks, message);
            break;
        case START_GAME:
            // Bắt đầu trò chơi khi có 2 người chơi sẵn sàng
            {
                static int ready_clients[2] = {0};
                static int ready_count = 0;

                ready_clients[ready_count++] = client_sock;

                if (ready_count == 2) {
                    handle_start_game(ready_clients);
                    ready_count = 0; // Reset số người chơi sẵn sàng
                }
            }
            break;
        default:
            printf("Unknown message type\n");
            break;
    }
}

// Hàm để tìm socket của người dùng theo tên
int find_user_socket(int client_socks[], const char *username) {
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (strcmp(users[i].username, username) == 0 && users[i].status == 1) {
            return client_socks[i];
        }
    }
    return -1; // Trả về -1 nếu không tìm thấy
}

// Hàm xử lý yêu cầu thách đấu
void handle_challenge_request(int client_sock, int client_socks[], Message *message) {
    char challenger[50], opponent[50];
    sscanf(message->payload, "%49s %49s", challenger, opponent);
    printf("Received challenge request from %s to %s.\n", challenger, opponent);

    int opponent_sock = find_user_socket(client_socks, opponent);

    if (opponent_sock != -1) {
        printf("Opponent %s is online, preparing to send challenge.\n", opponent);

        // Thêm lời thách đấu vào danh sách
        if (challenge_count < MAX_USERS) {
            strcpy(challenges[challenge_count].challenger, challenger);
            strcpy(challenges[challenge_count].opponent, opponent);
            challenges[challenge_count].status = 0;
            challenge_count++;
            printf("Challenge added to the list. Total challenges: %d\n", challenge_count);

            // Gửi thông báo thách đấu đến đối thủ
            Message challenge_message;
            challenge_message.message_type = CHALLENGE_REQUEST;
            snprintf(challenge_message.payload, sizeof(challenge_message.payload), "You have been challenged by %s", challenger);
            send(opponent_sock, &challenge_message, sizeof(Message), 0);
            printf("Challenge notification sent to opponent %s.\n", opponent);

            // Xác nhận gửi lời thách đấu thành công cho người thách đấu
            strcpy(message->payload, "Challenge sent successfully.");
        } else {
            printf("Challenge queue is full. Unable to add new challenge.\n");
            strcpy(message->payload, "Challenge queue full.");
        }
    } else {
        printf("Opponent %s is not online.\n", opponent);
        strcpy(message->payload, "Opponent is not online.");
    }
    send(client_sock, message, sizeof(Message), 0);
    printf("Response sent to challenger %s: %s\n", challenger, message->payload);
}

// Hàm xử lý phản hồi thách đấu
void handle_challenge_response(int client_sock, int client_socks[], Message *message) {
    char challenger[50], opponent[50];
    int response;
    sscanf(message->payload, "%49s %49s %d", challenger, opponent, &response);
    printf("Received challenge response from %s to %s. Response: %d\n", opponent, challenger, response);

    int challenger_sock = find_user_socket(client_socks, challenger);

    if (challenger_sock != -1) {
        printf("Challenger %s is online, sending response.\n", challenger);

        // Cập nhật trạng thái của lời thách đấu
        for (int i = 0; i < challenge_count; i++) {
            if (strcmp(challenges[i].challenger, challenger) == 0 &&
                strcmp(challenges[i].opponent, opponent) == 0) {
                challenges[i].status = response;
                printf("Updated challenge status for %s vs %s to %d.\n", challenger, opponent, response);
                break;
            }
        }

        Message response_message;
        response_message.message_type = CHALLENGE_RESPONSE;
        if (response == 1) {
            snprintf(response_message.payload, sizeof(response_message.payload), "%s accepted your challenge.", opponent);
            printf("Challenge accepted by %s. Notifying %s.\n", opponent, challenger);
        } else {
            snprintf(response_message.payload, sizeof(response_message.payload), "%s rejected your challenge.", opponent);
            printf("Challenge rejected by %s. Notifying %s.\n", opponent, challenger);
        }
        send(challenger_sock, &response_message, sizeof(Message), 0);
    } else {
        printf("Challenger %s is not online. Unable to send response.\n", challenger);
        strcpy(message->payload, "Challenger is not online.");
        send(client_sock, message, sizeof(Message), 0);
    }
}

// Hàm xử lý yêu cầu bắt đầu trò chơi
void handle_start_game(int client_socks[]) {
    printf("Both players connected. Starting Wordle game.\n");

    srand(time(NULL));
    const char *target_word = valid_solutions[rand() % solution_count];
    int attempts = 0;
    int current_player = 0;

    Message message;

    while (attempts < MAX_ATTEMPTS) {
        // Gửi thông báo lượt chơi đến client hiện tại
        message.message_type = START_GAME;
        snprintf(message.payload, sizeof(message.payload), "It's your turn.");
        send(client_socks[current_player], &message, sizeof(message), 0);

        // Nhận từ đoán từ client
        memset(&message, 0, sizeof(message));
        recv(client_socks[current_player], &message, sizeof(message), 0);

        if (message.message_type == GUESS) {
            if (!is_valid_guess(message.payload)) {
                message.message_type = INVALID_GUESS;
                snprintf(message.payload, sizeof(message.payload), "Invalid guess. Try again.");
                send(client_socks[current_player], &message, sizeof(message), 0);
                continue;
            }

            attempts++;
            char feedback[WORD_LENGTH + 1];
            get_feedback(message.payload, target_word, feedback);

            if (strcmp(message.payload, target_word) == 0) {
                message.message_type = WIN;
                snprintf(message.payload, sizeof(message.payload), "You guessed the word: %s", target_word);
                for (int i = 0; i < 2; i++) {
                    send(client_socks[i], &message, sizeof(message), 0);
                }
                break;
            } else {
                message.message_type = FEEDBACK;
                snprintf(message.payload, sizeof(message.payload), "Feedback: %s", feedback);
                for (int i = 0; i < 2; i++) {
                    send(client_socks[i], &message, sizeof(message), 0);
                }
                current_player = 1 - current_player; // Chuyển lượt
            }
        }
    }

    if (attempts >= MAX_ATTEMPTS) {
        message.message_type = LOSE;
        snprintf(message.payload, sizeof(message.payload), "The word was: %s", target_word);
        for (int i = 0; i < 2; i++) {
            send(client_socks[i], &message, sizeof(message), 0);
        }
    }
}

// Function to validate if the guessed word exists in the valid guesses list
int is_valid_guess(const char *guess) {
    for (int i = 0; i < guess_count; i++) {
        if (strcmp(guess, valid_guesses[i]) == 0) {
            return 1;
        }
    }
    for (int i = 0; i < solution_count; i++){
        if (strcmp(guess, valid_solutions[i]) == 0) return 1;
    }
    return 0;
}

// Function to provide feedback for the guess
void get_feedback(const char *guess, const char *target, char *feedback) {
    for (int i = 0; i < WORD_LENGTH; i++) {
        if (guess[i] == target[i]) {
            feedback[i] = 'G';  // Green for correct position
        } else if (strchr(target, guess[i]) != NULL) {
            feedback[i] = 'Y';  // Yellow for correct letter in wrong position
        } else {
            feedback[i] = 'B';  // Black for incorrect letter
        }
    }
    feedback[WORD_LENGTH] = '\0'; // Null-terminate feedback string
}

int register_user(const char *username, const char *password) {
    for (int i = 0; i < user_count; i++) {
        if (strcmp(users[i].username, username) == 0) {
            return 0; // User already exists
        }
    }

    if (user_count < MAX_USERS) {
        strcpy(users[user_count].username, username);
        strcpy(users[user_count].password, password);
        users[user_count].status = 0;
        save_user_to_file(&users[user_count]); // Save new user to file
        user_count++;
        return 1;
    }
    return 0;
}

int login_user(const char *username, const char *password) {
    for (int i = 0; i < user_count; i++) {
        if (strcmp(users[i].username, username) == 0 && strcmp(users[i].password, password) == 0) {
            users[i].status = 1;
            printf("user : %s online\n",username);
            return 1;
        }
    }
    return 0;
}

void logout_user(const char *username) {
    for (int i = 0; i < user_count; i++) {
        if (strcmp(users[i].username, username) == 0) {
            users[i].status = 0;
            break;
        }
    }
}

// Hàm yêu cầu danh sách người dùng
void list_users() {
    for (int i = 0; i < user_count; i++) {
        printf("%s (Status: %d)\n", users[i].username, users[i].status);
    }
}


void load_users_from_file() {
    FILE *file = fopen(USER_FILE, "r");
    if (file == NULL) {
        printf("No existing user file found, starting fresh.\n");
        return;
    }

    while (fscanf(file, "%49s %49s %d\n", users[user_count].username, users[user_count].password, &users[user_count].status) != EOF) {
        user_count++;
    }

    fclose(file);
}

void save_game_history(const GameHistory *history) {
    FILE *file = fopen(GAME_HISTORY_FILE, "a");
    if (file == NULL) {
        perror("Failed to open game history file");
        return;
    }

    fprintf(file, "Game ID: %s\n", history->game_id);
    fprintf(file, "Players: %s vs %s\n", history->player1, history->player2);
    fprintf(file, "Word: %s\n", history->word);
    fprintf(file, "Result: %s\n", history->result == 1 ? "Player 1 Wins" :
                                  history->result == 2 ? "Player 2 Wins" : "Draw");
    fprintf(file, "Moves:\n");
    for (int i = 0; i < history->move_count; i++) {
        fprintf(file, "%d. %s\n", i + 1, history->moves[i]);
    }
    fprintf(file, "=========================\n");

    fclose(file);
}


void save_user_to_file(const User *user) {
    FILE *file = fopen(USER_FILE, "a");
    if (file == NULL) {
        perror("Failed to open user file");
        return;
    }

    fprintf(file, "%s %s %d\n", user->username, user->password, user->status);
    fclose(file);
}
