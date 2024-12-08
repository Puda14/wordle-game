#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <time.h>

#define PORT 8080
#define BUFFER_SIZE 1024
#define WORD_LENGTH 5
#define MAX_ATTEMPTS 12
#define MAX_USERS 100
#define MAX_CLIENTS 30
#define MAX_WORDS 5000
#define USER_FILE "users.txt"

// Định nghĩa enum cho loại thông điệp
enum MessageType {
    SIGN_UP = 0,
    LOGIN = 1,
    LOGOUT = 2,
    LIST_USERS = 3,
    CHALLENGE_REQUEST = 4,
    CHALLENGE_RESPONSE = 5,
    START_GAME = 6,
    GUESS = 7,
    FEEDBACK = 8,
    WIN = 9,
    LOSE = 10,
    INVALID_GUESS = 11,
    END_GAME = 12
};

// Cấu trúc thông điệp
typedef struct {
    int message_type;
    char payload[BUFFER_SIZE];
} Message;

// Cấu trúc người dùng
typedef struct {
    char username[50];
    char password[50];
    int status;
} User;

User users[MAX_USERS];
int user_count = 0;

// Dữ liệu từ vựng Wordle
char valid_solutions[MAX_WORDS][WORD_LENGTH + 1];
char valid_guesses[MAX_WORDS][WORD_LENGTH + 1];
int solution_count = 0;
int guess_count = 0;

// Danh sách socket của người dùng
int client_sockets[MAX_USERS] = {0};

// Hàm tải danh sách từ
int load_words(const char *filename, char words[][WORD_LENGTH + 1]) {
    FILE *file = fopen(filename, "r");
    if (file == NULL) {
        perror("Error opening file");
        return -1;
    }
    int count = 0;
    while (fscanf(file, "%5s", words[count]) == 1) {
        count++;
        if (count >= MAX_WORDS) break;
    }
    fclose(file);
    return count;
}

// Hàm đọc danh sách người dùng từ file
void load_users_from_file() {
    FILE *file = fopen(USER_FILE, "r");
    if (file == NULL) {
        printf("No existing user file found. Starting fresh.\n");
        return;
    }
    while (fscanf(file, "%49s %49s %d", users[user_count].username, users[user_count].password, &users[user_count].status) != EOF) {
        user_count++;
    }
    fclose(file);
}

// Hàm lưu người dùng mới vào file
void save_user_to_file(const User *user) {
    FILE *file = fopen(USER_FILE, "a");
    if (file == NULL) {
        perror("Failed to open user file");
        return;
    }
    fprintf(file, "%s %s %d\n", user->username, user->password, user->status);
    fclose(file);
}

// Xử lý yêu cầu đăng ký
void handle_sign_up(int client_sock, Message *message) {
    char username[50], password[50];
    sscanf(message->payload, "%49[^|]|%49s", username, password);

    for (int i = 0; i < user_count; i++) {
        if (strcmp(users[i].username, username) == 0) {
            strcpy(message->payload, "Username already exists.\n");
            send(client_sock, message, sizeof(Message), 0);
            return;
        }
    }

    if (user_count < MAX_USERS) {
        strcpy(users[user_count].username, username);
        strcpy(users[user_count].password, password);
        users[user_count].status = 0;
        save_user_to_file(&users[user_count]);
        user_count++;
        strcpy(message->payload, "Sign up successful.\n");
    } else {
        strcpy(message->payload, "User limit reached.\n");
    }
    send(client_sock, message, sizeof(Message), 0);
}

// Xử lý yêu cầu đăng nhập
void handle_login(int client_sock, Message *message) {
    char username[50], password[50];
    sscanf(message->payload, "%49[^|]|%49s", username, password);

    for (int i = 0; i < user_count; i++) {
        if (strcmp(users[i].username, username) == 0 && strcmp(users[i].password, password) == 0) {
            if (users[i].status == 1) {
                strcpy(message->payload, "User already logged in.\n");
            } else {
                users[i].status = 1;
                client_sockets[i] = client_sock; // Lưu socket của người dùng
                strcpy(message->payload, "Login successful.\n");
            }
            send(client_sock, message, sizeof(Message), 0);
            return;
        }
    }
    strcpy(message->payload, "Invalid username or password.\n");
    send(client_sock, message, sizeof(Message), 0);
}

// Xử lý yêu cầu đăng xuất
void handle_logout(int client_sock, Message *message) {
    char username[50];
    sscanf(message->payload, "%49s", username);

    for (int i = 0; i < user_count; i++) {
        if (strcmp(users[i].username, username) == 0) {
            users[i].status = 0;
            client_sockets[i] = 0; // Xóa socket
            strcpy(message->payload, "Logout successful.\n");
            send(client_sock, message, sizeof(Message), 0);
            return;
        }
    }
    strcpy(message->payload, "User not found.\n");
    send(client_sock, message, sizeof(Message), 0);
}

// Xử lý yêu cầu thách đấu
void handle_challenge_request(int client_sock, Message *message) {
    char challenger[50], opponent[50];
    sscanf(message->payload, "%49s %49s", challenger, opponent);

    int opponent_sock = -1;
    for (int i = 0; i < user_count; i++) {
        if (strcmp(users[i].username, opponent) == 0 && users[i].status == 1) {
            opponent_sock = client_sockets[i];
            break;
        }
    }

    if (opponent_sock == -1) {
        strcpy(message->payload, "Opponent not online.\n");
        send(client_sock, message, sizeof(Message), 0);
        return;
    }

    strcpy(message->payload, "You have been challenged.\n");
    send(opponent_sock, message, sizeof(Message), 0);
    strcpy(message->payload, "Challenge request sent.\n");
    send(client_sock, message, sizeof(Message), 0);
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

int main() {
    solution_count = load_words("valid_solutions.txt", valid_solutions);
        guess_count = load_words("valid_guesses.txt", valid_guesses);
    printf("Loaded %d solutions and %d guesses.\n", solution_count, guess_count);

    if (solution_count == -1 || guess_count == -1) {
        printf("Failed to load word lists.\n");
        return 1;
    }

    // Khởi tạo socket server
    int server_sock = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    bind(server_sock, (struct sockaddr *)&server_addr, sizeof(server_addr));
    listen(server_sock, MAX_CLIENTS);
    printf("Server listening on port %d\n", PORT);

    // Tải danh sách người dùng từ file
    load_users_from_file();

    // Mảng quản lý kết nối client
    int client_socks[MAX_CLIENTS] = {0};

    while (1) {
        struct sockaddr_in client_addr;
        socklen_t addr_len = sizeof(client_addr);
        int client_sock = accept(server_sock, (struct sockaddr *)&client_addr, &addr_len);
        printf("New client connected.\n");

        // Xử lý yêu cầu từ client
        Message message;
        recv(client_sock, &message, sizeof(message), 0);

        switch (message.message_type) {
            case SIGN_UP:
                handle_sign_up(client_sock, &message);
                break;
            case LOGIN:
                handle_login(client_sock, &message);
                break;
            case LOGOUT:
                handle_logout(client_sock, &message);
                break;
            case LIST_USERS:
                // Trả danh sách người chơi trực tuyến
                message.message_type = LIST_USERS;
                snprintf(message.payload, sizeof(message.payload), "Online users:\n");
                for (int i = 0; i < user_count; i++) {
                    if (users[i].status == 1) {
                        strncat(message.payload, users[i].username, sizeof(message.payload) - strlen(message.payload) - 1);
                        strncat(message.payload, "\n", sizeof(message.payload) - strlen(message.payload) - 1);
                    }
                }
                send(client_sock, &message, sizeof(message), 0);
                break;
            case CHALLENGE_REQUEST:
                handle_challenge_request(client_sock, &message);
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
                printf("Unknown message type: %d\n", message.message_type);
                break;
        }
    }

    close(server_sock);
    return 0;
}

