#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>


#define PORT 8080
#define BUFFER_SIZE 1024

typedef struct {
    uint8_t message_type;
    char payload[BUFFER_SIZE];
} Message;

enum MessageType { REGISTER_REQUEST = 0, LOGIN_REQUEST = 1, LOGOUT_REQUEST = 2, LIST_USERS_REQUEST = 3,
                    CHALLENGE_REQUEST = 4, CHALLENGE_RESPONSE = 5 };

int is_logged_in = 0; // Biến kiểm tra trạng thái đăng nhập
int sock;

// Gửi yêu cầu đăng ký
void send_register_request(int sock, const char *username, const char *password) {
    Message message;
    message.message_type = REGISTER_REQUEST;
    snprintf(message.payload, sizeof(message.payload), "%s|%s", username, password);
    send(sock, &message, sizeof(message), 0);
}

// Gửi yêu cầu đăng nhập
void send_login_request(int sock, const char *username, const char *password) {
    Message message;
    message.message_type = LOGIN_REQUEST;
    snprintf(message.payload, sizeof(message.payload), "%s|%s", username, password);
    send(sock, &message, sizeof(message), 0);
}

// Gửi yêu cầu đăng xuất
void send_logout_request(int sock, const char *username) {
    Message message;
    message.message_type = LOGOUT_REQUEST;
    snprintf(message.payload, sizeof(message.payload), "%s", username);
    send(sock, &message, sizeof(message), 0);
}

// Gửi yêu cầu xem danh sách người dùng
void send_list_users_request(int sock) {
    Message message;
    message.message_type = LIST_USERS_REQUEST;
    snprintf(message.payload, sizeof(message.payload), "Requesting user list...");
    send(sock, &message, sizeof(message), 0);
}

// Gửi yêu cầu thách đấu
void send_challenge_request(int sock, const char *challenger, const char *opponent) {
    Message message;
    message.message_type = CHALLENGE_REQUEST;
    snprintf(message.payload, sizeof(message.payload), "%s %s", challenger, opponent);
    send(sock, &message, sizeof(message), 0);
}

// Gửi phản hồi thách đấu
void send_challenge_response(int sock, const char *challenger, const char *opponent, int response) {
    Message message;
    message.message_type = CHALLENGE_RESPONSE;
    snprintf(message.payload, sizeof(message.payload), "%s %s %d", challenger, opponent, response);
    send(sock, &message, sizeof(message), 0);
}

// Kiểm tra thông báo từ server
void check_for_notifications(int sock, const char *username) {
    Message message;
    while (recv(sock, &message, sizeof(Message), MSG_DONTWAIT) > 0) {
        if (message.message_type == CHALLENGE_REQUEST) {
            printf("\n%s\n", message.payload);  // Hiển thị thông báo thách đấu
            printf("Accept challenge (1) or Reject challenge (-1): ");
            int response;
            scanf("%d", &response);

            char challenger[50];
            sscanf(message.payload, "You have been challenged by %49s", challenger);
            send_challenge_response(sock, challenger, username, response);
        } else {
            printf("\nServer response: %s\n", message.payload);  // Hiển thị các thông báo khác
        }
    }
}
// Luồng phụ để lắng nghe thông báo từ server
void *listen_for_notifications(void *arg) {
    Message message;
    while (recv(sock, &message, sizeof(Message), 0) > 0) {
        if (message.message_type == CHALLENGE_REQUEST) {
            printf("\n%s\n", message.payload);  // In thông báo thách đấu
            printf("Accept challenge (1) or Reject challenge (-1): ");
            int response;
            scanf("%d", &response);

            char challenger[50];
            if (sscanf(message.payload, "You have been challenged by %49s", challenger) == 1) {
                send_challenge_response(sock, challenger, "your_username", response);  // Gửi phản hồi thách đấu
            } else {
                printf("Error parsing challenger name from message.\n");
            }
        } else {
            printf("\nServer response: %s\n", message.payload);  // In thông báo khác từ server
        }
    }
    return NULL;
}
int main() {
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
        // Tạo luồng lắng nghe thông báo
    pthread_t listener_thread;
    pthread_create(&listener_thread, NULL, listen_for_notifications, NULL);
    while (1) {
        // Kiểm tra và xử lý thông báo từ server trước khi hiển thị menu
        // if (is_logged_in) {
        //     check_for_notifications(sock, username);
        // }

        // Hiển thị menu lựa chọn
        printf("\nSelect option:\n1. Register\n2. Login\n3. View User List\n4. Send Challenge\n5. Respond to Challenge\n6. Logout\n7. Exit\nChoice: ");
        scanf("%d", &choice);

        if (choice == 1 || choice == 2) {
            printf("Enter username: ");
            scanf("%s", username);
            printf("Enter password: ");
            scanf("%s", password);
        }

        switch (choice) {
            case 1:
                send_register_request(sock, username, password);
                break;
            case 2:
                send_login_request(sock, username, password);

                // Nhận phản hồi đăng nhập từ server
                Message response;
                if (recv(sock, &response, sizeof(Message), 0) > 0) {
                    printf("Server response: %s\n", response.payload);

                    // Nếu đăng nhập thành công, cập nhật trạng thái
                    if (strstr(response.payload, "Login successful")) {
                        is_logged_in = 1;
                    }
                }
                break;
            case 3:
                if (is_logged_in) send_list_users_request(sock);
                else printf("You must login first.\n");
                break;
            case 4: {
                if (is_logged_in) {
                    char opponent[50];
                    printf("Enter opponent's username: ");
                    scanf("%s", opponent);
                    send_challenge_request(sock, username, opponent);
                } else {
                    printf("You must login first.\n");
                }
                break;
            }
            case 5: {
                printf("Respond to Challenge is handled automatically on receiving challenge notifications.\n");
                break;
            }
            case 6:
                if (is_logged_in) {
                    send_logout_request(sock, username);
                    is_logged_in = 0;
                } else {
                    printf("You are not logged in.\n");
                }
                break;
            case 7:
                printf("Exiting application.\n");
                close(sock);
                return 0;
            default:
                printf("Invalid choice\n");
                continue;
        }

        // Nhận phản hồi từ server cho các thao tác khác
        // if (is_logged_in) {
        //     check_for_notifications(sock, username);
        // }
    }
    pthread_join(listener_thread, NULL);

    close(sock);
    return 0;
}
