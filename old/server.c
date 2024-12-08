#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/select.h>

#define PORT 8080
#define MAX_USERS 100
#define BUFFER_SIZE 1024
#define MAX_CLIENTS 30
#define USER_FILE "users.txt"

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

enum MessageType { REGISTER_REQUEST = 0, LOGIN_REQUEST = 1, LOGOUT_REQUEST = 2, 
                   LIST_USERS_REQUEST = 3, CHALLENGE_REQUEST = 4, CHALLENGE_RESPONSE = 5 };

int register_user(const char *username, const char *password);
int login_user(const char *username, const char *password);
void logout_user(const char *username);
void handle_message(int client_sock, int client_socks[], Message *message);
void load_users_from_file();
void save_user_to_file(const User *user);
// Khai báo các hàm xử lý thách đấu
void handle_challenge_request(int client_sock, int client_socks[], Message *message);
void handle_challenge_response(int client_sock, int client_socks[], Message *message);

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

void save_user_to_file(const User *user) {
    FILE *file = fopen(USER_FILE, "a");
    if (file == NULL) {
        perror("Failed to open user file");
        return;
    }

    fprintf(file, "%s %s %d\n", user->username, user->password, user->status);
    fclose(file);
}
