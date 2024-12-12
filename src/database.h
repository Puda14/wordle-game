#ifndef DATABASE_H
#define DATABASE_H

#include <sqlite3.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define USER_TABLE "user"
#define MAX_USERNAME_LEN 50
#define MAX_PASSWORD_LEN 50

typedef struct {
  int id;
  char username[50];
  char password[50];
  int score;
  int is_online;
} User;


int init_db(sqlite3 **db, const char *db_name);

int create_user(sqlite3 *db, const User *user);

int read_user(sqlite3 *db, const char *username, User *user);

int update_user(sqlite3 *db, const User *user);

int delete_user(sqlite3 *db, int user_id);

int user_exists(sqlite3 *db, const char *username);

int get_user_by_username(sqlite3 *db, const char *username, User *user);

void handle_db_error(sqlite3 *db, const char *errMsg);

int authenticate_user(sqlite3 *db, const char *username, const char *password);

int update_user_online(sqlite3 *db, const char *username);

int update_user_offline(sqlite3 *db, const char *username);

#endif
