#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <signal.h>
#include <errno.h>
#include <sys/select.h>

#define PORT 8080
#define MAX_CLIENTS 30
#define BUFFER_SIZE 1024
#define USER_FILE "../app/users.txt"

#include <../model/user.h>
#include <../model/message.h>

volatile sig_atomic_t got_signal = 0;

void signal_handler(int sig) {
    got_signal = 1;
    printf("Caught signal %d\n", sig);
}

void setup_signal_handler() {
    struct sigaction sa;
    sa.sa_handler = signal_handler;
    sa.sa_flags = 0;
    sigemptyset(&sa.sa_mask);
    sigaction(SIGINT, &sa, NULL);
}

int handle_sign_up(const char *username, const char *password) {
    FILE *file = fopen(USER_FILE, "a+");
    if (!file) {
        perror("Error opening user file");
        return 0;
    }

    int id = 1, score = 0, status = 0;
    char line[BUFFER_SIZE];

    while (fgets(line, sizeof(line), file)) {
        int current_id;
        sscanf(line, "%d", &current_id);
        if (current_id >= id) {
            id = current_id + 1;
        }
    }

    printf("%d %s %s %d %d\n", id, username, password, score, status);

    fprintf(file, "%d %s %s %d %d\n", id, username, password, score, status);
    fclose(file);
    return 1;
}

// Hàm xử lý message từ client
void handle_message(int client_sock, Message *message) {
    if (message->message_type == SIGNUP_REQUEST) {
        char username[50], password[50];
        sscanf(message->payload, "%49[^|]|%49s", username, password);

        if (handle_sign_up(username, password)) {
            strcpy(message->payload, "Sign up successful\n");
        } else {
            strcpy(message->payload, "Sign up failed\n");
        }
        send(client_sock, message, sizeof(Message), 0);
    }
}

int main() {
    int server_sock, new_sock, client_socks[MAX_CLIENTS] = {0};
    struct sockaddr_in server_addr, client_addr;
    fd_set readfds;
    socklen_t addr_len = sizeof(client_addr);
    sigset_t block_mask, orig_mask;

    setup_signal_handler();

    // Block SIGINT
    sigemptyset(&block_mask);
    sigaddset(&block_mask, SIGINT);
    sigprocmask(SIG_BLOCK, &block_mask, &orig_mask);

    // Tạo socket
    if ((server_sock = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("Socket failed");
        exit(EXIT_FAILURE);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    if (bind(server_sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Bind failed");
        exit(EXIT_FAILURE);
    }
    if (listen(server_sock, 3) < 0) {
        perror("Listen failed");
        exit(EXIT_FAILURE);
    }

    printf("Server listening on port %d\n", PORT);

    while (1) {
        FD_ZERO(&readfds);
        FD_SET(server_sock, &readfds);
        int max_sd = server_sock;

        for (int i = 0; i < MAX_CLIENTS; i++) {
            int sock = client_socks[i];
            if (sock > 0) FD_SET(sock, &readfds);
            if (sock > max_sd) max_sd = sock;
        }

        int ready = pselect(max_sd + 1, &readfds, NULL, NULL, NULL, &orig_mask);
        if (ready == -1) {
            if (errno == EINTR) {
                printf("pselect() interrupted by signal.\n");
                if (got_signal) {
                    printf("Received SIGINT, shutting down server.\n");
                    break;  // Thoát khỏi vòng lặp khi nhận tín hiệu
                }
                continue;
            } else {
                perror("pselect");
                exit(EXIT_FAILURE);
            }
        }

        // Kết nối mới
        if (FD_ISSET(server_sock, &readfds)) {
            if ((new_sock = accept(server_sock, (struct sockaddr *)&client_addr, &addr_len)) < 0) {
                perror("Accept failed");
                continue;
            }
            printf("New connection, socket fd is %d\n", new_sock);

            for (int i = 0; i < MAX_CLIENTS; i++) {
                if (client_socks[i] == 0) {
                    client_socks[i] = new_sock;
                    break;
                }
            }
        }

        // Kiểm tra các kết nối client
        for (int i = 0; i < MAX_CLIENTS; i++) {
            int sock = client_socks[i];
            if (FD_ISSET(sock, &readfds)) {
                Message message;
                int read_size = recv(sock, &message, sizeof(Message), 0);
                if (read_size == 0) {
                    close(sock);
                    client_socks[i] = 0;
                } else {
                    handle_message(sock, &message);
                }
            }
        }

        // Kiểm tra cờ tín hiệu sau mỗi lần pselect
        if (got_signal) {
            printf("Received SIGINT, shutting down server.\n");
            break;
        }
    }

    // Đóng socket server trước khi thoát
    close(server_sock);
    printf("Server stopped.\n");
    return 0;
}
