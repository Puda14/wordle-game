#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define PORT 8080

#include <../model/user.h>
#include <../model/message.h>


void send_sign_up_request(int sock, const char *username, const char *password) {
    Message message;
    message.message_type = SIGNUP_REQUEST;
    snprintf(message.payload, sizeof(message.payload), "%s|%s", username, password);
    send(sock, &message, sizeof(message), 0);
}

int main() {
    int sock;
    struct sockaddr_in server_addr;
    char username[50], password[50];

    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("Socket creation error");
        return -1;
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    server_addr.sin_port = htons(PORT);

    if (connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Connection failed");
        return -1;
    }

    printf("Enter username: ");
    scanf("%s", username);
    printf("Enter password: ");
    scanf("%s", password);

    send_sign_up_request(sock, username, password);

    // Nhận phản hồi từ server
    Message response;
    read(sock, &response, sizeof(response));
    printf("Server response: %s\n", response.payload);

    close(sock);
    return 0;
}
