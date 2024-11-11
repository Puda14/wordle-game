// client.c

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define PORT 8080
#define BUFFER_SIZE 1024

// typedef struct {
//     uint8_t message_type;
//     char payload[BUFFER_SIZE];
// } Message;
#include <../model/message.h>

enum MessageType { REGISTER_REQUEST = 0, LOGIN_REQUEST = 1, LOGOUT_REQUEST = 2 };

void send_register_request(int sock, const char *username, const char *password);
void send_login_request(int sock, const char *username, const char *password);
void send_logout_request(int sock, const char *username);

int main() {
    int sock;
    struct sockaddr_in server_addr;
    char username[50], password[50];
    int choice;

    sock = socket(AF_INET, SOCK_STREAM, 0);
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    server_addr.sin_port = htons(PORT);

    if (connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Connection failed");
        return 1;
    }

    printf("Select option:\n1. Register\n2. Login\nChoice: ");
    scanf("%d", &choice);
    printf("Enter username: ");
    scanf("%s", username);
    printf("Enter password: ");
    scanf("%s", password);

    if (choice == 1) {
        // Send register request
        send_register_request(sock, username, password);
    } else if (choice == 2) {
        // Send login request
        send_login_request(sock, username, password);
    } else {
        printf("Invalid choice\n");
        close(sock);
        return 1;
    }

    // Receive response from server
    Message response;
    if (recv(sock, &response, sizeof(Message), 0) > 0) {
        printf("Server response: %s\n", response.payload);
    }

    // Lựa chọn để đăng xuất khi cần
    while (1) {
        printf("Enter 'logout' to log out or 'exit' to quit the application: ");
        char command[BUFFER_SIZE];
        scanf("%s", command);

        if (strcmp(command, "logout") == 0) {
            // Gửi yêu cầu đăng xuất
            send_logout_request(sock, username);
            if (recv(sock, &response, sizeof(Message), 0) > 0) {
                printf("Server response: %s\n", response.payload);
            }
            break;
        } else if (strcmp(command, "exit") == 0) {
            printf("Exiting application.\n");
            break;
        } else {
            printf("Unknown command. Please enter 'logout' or 'exit'.\n");
        }
    }

    close(sock);
    return 0;
}

void send_register_request(int sock, const char *username, const char *password) {
    Message message;
    message.message_type = REGISTER_REQUEST;
    snprintf(message.payload, sizeof(message.payload), "%s|%s", username, password);
    send(sock, &message, sizeof(message), 0);
}

void send_login_request(int sock, const char *username, const char *password) {
    Message message;
    message.message_type = LOGIN_REQUEST;
    snprintf(message.payload, sizeof(message.payload), "%s|%s", username, password);
    send(sock, &message, sizeof(message), 0);
}

void send_logout_request(int sock, const char *username) {
    Message message;
    message.message_type = LOGOUT_REQUEST;
    snprintf(message.payload, sizeof(message.payload), "%s", username);
    send(sock, &message, sizeof(message), 0);
}
