/* Libs */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "auth.h"

/* Functions */

int register_user(const char *username, const char *password) {
    for (int i = 0; i < user_count; i++) {
        if (strcmp(users[i].username, username) == 0) {
            return 0; // Người dùng đã tồn tại
        }
    }

    if (user_count < MAX_USERS) {
        strcpy(users[user_count].username, username);
        strcpy(users[user_count].password, password);
        users[user_count].status = 0;
        save_user_to_file(&users[user_count]); // Lưu người dùng mới vào file
        user_count++;
        return 1;
    }
    return 0;
}

int login_user(const char *username, const char *password) {
    for (int i = 0; i < user_count; i++) {
        if (strcmp(users[i].username, username) == 0 && strcmp(users[i].password, password) == 0) {
            users[i].status = 1;
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
