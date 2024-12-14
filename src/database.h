#ifndef DATABASE_H
#define DATABASE_H

#include <sqlite3.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>

#define USER_TABLE "user"
#define MAX_USERNAME_LEN 50
#define MAX_PASSWORD_LEN 50
#define MAX_ATTEMPTS 12
#define WORD_LENGTH 5
#define MAX_WORDS 15000

typedef struct {
  int id;
  char username[50];
  char password[50];
  int score;
  int is_online;
} User;

typedef struct
{
  char player_name[50];
  char guess[WORD_LENGTH + 1];
  char result[WORD_LENGTH + 1];
} PlayTurn;

typedef struct
{
  char game_id[20];
  char player1_name[50];
  char player2_name[50];
  char target_word[WORD_LENGTH + 1];
  int current_player; // 1 or 2
  int player1_attempts;
  int player2_attempts;
  int player1_score;
  int player2_score;
  bool used[WORD_LENGTH];
  int game_active;
  int current_attempts;
  char start_time [20];
  char end_time [20];
  PlayTurn turns[MAX_ATTEMPTS];
} GameSession;

typedef struct {
  char game_id[20];
  char player1[50];
  char player2[50];
  int player1_score;
  int player2_score;
  char winner[51];
  char word[WORD_LENGTH + 1];
  PlayTurn moves[12];
  char start_time[20];
  char end_time[20];
} GameHistory;

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

int update_user_score(sqlite3 *db, const char *username, int score);

int list_users_online(sqlite3 *db, User *users, int *user_count);

int list_users_closest_score(sqlite3 *db, const char *target_username, User *users, int *user_count);

int save_game_history(sqlite3 *db, GameHistory *game);

int get_game_history_by_player(sqlite3 *db, const char *player_name, GameHistory *response);

int get_game_histories_by_player(sqlite3 *db, const char *player_name, GameHistory *history_list, int *history_count);

int get_game_history_by_id(sqlite3 *db, const char *game_id, GameHistory *game_details);

int get_score_by_username(sqlite3 *db, const char *username, int *score);

#endif
