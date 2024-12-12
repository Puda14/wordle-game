#include "database.h"

// Helper function to handle database errors
void handle_db_error(sqlite3 *db, const char *errMsg) {
  fprintf(stderr, "Database error: %s\n", errMsg);
}

// Initialize the SQLite database connection
int init_db(sqlite3 **db, const char *db_name) {
  int rc = sqlite3_open(db_name, db);
  if (rc) {
    fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(*db));
    return rc;
  }
  return SQLITE_OK;
}

// Create a new user in the database
int create_user(sqlite3 *db, const User *user) {
  const char *sql = "INSERT INTO user (username, password, score, isOnline) VALUES (?, ?, ?, ?)";
  sqlite3_stmt *stmt;

  int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, 0);
  if (rc != SQLITE_OK) {
    fprintf(stderr, "Failed to prepare statement: %s\n", sqlite3_errmsg(db));
    return rc;
  }

  sqlite3_bind_text(stmt, 1, user->username, -1, SQLITE_STATIC);
  sqlite3_bind_text(stmt, 2, user->password, -1, SQLITE_STATIC);

  int score = (user->score == -1) ? 0 : user->score;
  int is_online = (user->is_online == -1) ? 0 : user->is_online;

  sqlite3_bind_int(stmt, 3, score);
  sqlite3_bind_int(stmt, 4, is_online);

  rc = sqlite3_step(stmt);
  if (rc != SQLITE_DONE) {
    handle_db_error(db, sqlite3_errmsg(db));
    sqlite3_finalize(stmt);
    return rc;
  }

  sqlite3_finalize(stmt);
  return SQLITE_OK;
}

// Read a user from the database by username
int read_user(sqlite3 *db, const char *username, User *user) {
  const char *sql = "SELECT id, username, password, score, isOnline FROM user WHERE username = ?";
  sqlite3_stmt *stmt;
  int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, 0);
  if (rc != SQLITE_OK) {
    fprintf(stderr, "Failed to prepare statement: %s\n", sqlite3_errmsg(db));
    return rc;
  }

  sqlite3_bind_text(stmt, 1, username, -1, SQLITE_STATIC);

  rc = sqlite3_step(stmt);
  if (rc == SQLITE_ROW) {
    user->id = sqlite3_column_int(stmt, 0);
    strcpy(user->username, (const char *)sqlite3_column_text(stmt, 1));
    strcpy(user->password, (const char *)sqlite3_column_text(stmt, 2));
    user->score = sqlite3_column_int(stmt, 3);
    user->is_online = sqlite3_column_int(stmt, 4);
  } else if (rc == SQLITE_DONE) {
    fprintf(stderr, "User not found\n");
  } else {
    handle_db_error(db, sqlite3_errmsg(db));
  }

  sqlite3_finalize(stmt);
  return rc;
}

// Update an existing user's data in the database
int update_user(sqlite3 *db, const User *user) {
  const char *sql = "UPDATE user SET password = ?, score = ?, isOnline = ? WHERE id = ?";
  sqlite3_stmt *stmt;
  int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, 0);
  if (rc != SQLITE_OK) {
    fprintf(stderr, "Failed to prepare statement: %s\n", sqlite3_errmsg(db));
    return rc;
  }

  sqlite3_bind_text(stmt, 1, user->password, -1, SQLITE_STATIC);
  sqlite3_bind_int(stmt, 2, user->score);
  sqlite3_bind_int(stmt, 3, user->is_online);
  sqlite3_bind_int(stmt, 4, user->id);

  rc = sqlite3_step(stmt);
  if (rc != SQLITE_DONE) {
    handle_db_error(db, sqlite3_errmsg(db));
  }

  sqlite3_finalize(stmt);
  return rc;
}

int update_user_score(sqlite3 *db, const User *user) {
  const char *sql = "UPDATE user SET score = ? WHERE id = ?";
  sqlite3_stmt *stmt;
  int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, 0);
  if (rc != SQLITE_OK) {
    fprintf(stderr, "Failed to prepare statement: %s\n", sqlite3_errmsg(db));
    return rc;
  }

  sqlite3_bind_int(stmt, 2, user->score);
  sqlite3_bind_int(stmt, 4, user->id);

  rc = sqlite3_step(stmt);
  if (rc != SQLITE_DONE) {
    handle_db_error(db, sqlite3_errmsg(db));
  }

  sqlite3_finalize(stmt);
  return rc;
}

// Delete a user from the database
int delete_user(sqlite3 *db, int user_id) {
  const char *sql = "DELETE FROM user WHERE id = ?";
  sqlite3_stmt *stmt;
  int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, 0);
  if (rc != SQLITE_OK) {
    fprintf(stderr, "Failed to prepare statement: %s\n", sqlite3_errmsg(db));
    return rc;
  }

  sqlite3_bind_int(stmt, 1, user_id);

  rc = sqlite3_step(stmt);
  if (rc != SQLITE_DONE) {
    handle_db_error(db, sqlite3_errmsg(db));
  }

  sqlite3_finalize(stmt);
  return rc;
}

// Check if a username exists in the database
int user_exists(sqlite3 *db, const char *username) {
  const char *sql = "SELECT COUNT(*) FROM user WHERE username = ?";
  sqlite3_stmt *stmt;
  int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, 0);
  if (rc != SQLITE_OK) {
    fprintf(stderr, "Failed to prepare statement: %s\n", sqlite3_errmsg(db));
    return 0;
  }

  sqlite3_bind_text(stmt, 1, username, -1, SQLITE_STATIC);
  rc = sqlite3_step(stmt);

  if (rc == SQLITE_ROW) {
    int count = sqlite3_column_int(stmt, 0);
    sqlite3_finalize(stmt);
    return count > 0;
  } else {
    handle_db_error(db, sqlite3_errmsg(db));
    sqlite3_finalize(stmt);
    return 0;
  }
}

// Get user details by username
int get_user_by_username(sqlite3 *db, const char *username, User *user) {
  const char *sql = "SELECT id, username, password, score, isOnline FROM user WHERE username = ?";
  sqlite3_stmt *stmt;
  int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, 0);
  if (rc != SQLITE_OK) {
    fprintf(stderr, "Failed to prepare statement: %s\n", sqlite3_errmsg(db));
    return rc;
  }

  sqlite3_bind_text(stmt, 1, username, -1, SQLITE_STATIC);
  rc = sqlite3_step(stmt);

  if (rc == SQLITE_ROW) {
    user->id = sqlite3_column_int(stmt, 0);
    strcpy(user->username, (const char *)sqlite3_column_text(stmt, 1));
    strcpy(user->password, (const char *)sqlite3_column_text(stmt, 2));
    user->score = sqlite3_column_int(stmt, 3);
    user->is_online = sqlite3_column_int(stmt, 4);
    sqlite3_finalize(stmt);
    return SQLITE_OK;
  } else if (rc == SQLITE_DONE) {
    fprintf(stderr, "User not found\n");
    sqlite3_finalize(stmt);
    return SQLITE_ERROR;
  } else {
    handle_db_error(db, sqlite3_errmsg(db));
    sqlite3_finalize(stmt);
    return rc;
  }

}

int authenticate_user(sqlite3 *db, const char *username, const char *password) {
  sqlite3_stmt *stmt;
  const char *sql = "SELECT password FROM user WHERE username = ?";

  if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) {
    fprintf(stderr, "SQL error: %s\n", sqlite3_errmsg(db));
    return -1;
  }

  sqlite3_bind_text(stmt, 1, username, -1, SQLITE_STATIC);

  int rc = sqlite3_step(stmt);
  if (rc == SQLITE_ROW) {
    const char *stored_password = (const char *)sqlite3_column_text(stmt, 0);
    if (strcmp(stored_password, password) == 0) {
      sqlite3_finalize(stmt);
      return 1;
    }
  }

  sqlite3_finalize(stmt);
  return 0;
}

int update_user_online(sqlite3 *db, const char *username) {
  const char *sql = "UPDATE user SET isOnline = 1 WHERE username = ?";
  sqlite3_stmt *stmt;

  int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, 0);
  if (rc != SQLITE_OK) {
    fprintf(stderr, "Failed to prepare statement: %s\n", sqlite3_errmsg(db));
    return rc;
  }

  if (username == NULL || strlen(username) == 0) {
    fprintf(stderr, "Username is NULL or empty.\n");
    sqlite3_finalize(stmt);
    return SQLITE_MISUSE;
  }

  rc = sqlite3_bind_text(stmt, 1, username, -1, SQLITE_STATIC);
  if (rc != SQLITE_OK) {
    fprintf(stderr, "Failed to bind parameter: %s\n", sqlite3_errmsg(db));
    sqlite3_finalize(stmt);
    return rc;
  }

  rc = sqlite3_step(stmt);
  if (rc != SQLITE_DONE) {
    fprintf(stderr, "Failed to update user status: %s\n", sqlite3_errmsg(db));
  }

  sqlite3_finalize(stmt);
  return rc;
}

int update_user_offline(sqlite3 *db, const char *username) {
  const char *sql = "UPDATE user SET isOnline = 0 WHERE username = ?";
  sqlite3_stmt *stmt;

  int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, 0);
  if (rc != SQLITE_OK) {
    fprintf(stderr, "Failed to prepare statement: %s\n", sqlite3_errmsg(db));
    return rc;
  }

  if (username == NULL || strlen(username) == 0) {
    fprintf(stderr, "Username is NULL or empty.\n");
    sqlite3_finalize(stmt);
    return SQLITE_MISUSE;
  }

  rc = sqlite3_bind_text(stmt, 1, username, -1, SQLITE_STATIC);
  if (rc != SQLITE_OK) {
    fprintf(stderr, "Failed to bind parameter: %s\n", sqlite3_errmsg(db));
    sqlite3_finalize(stmt);
    return rc;
  }

  rc = sqlite3_step(stmt);
  if (rc != SQLITE_DONE) {
    fprintf(stderr, "Failed to update user status: %s\n", sqlite3_errmsg(db));
  }

  sqlite3_finalize(stmt);
  return rc;
}

int list_users_online(sqlite3 *db, User *users, int *user_count) {
  // Câu lệnh SQL lấy danh sách người dùng online
  const char *sql = "SELECT id, username, score, isOnline FROM user WHERE isOnline = 1";
  sqlite3_stmt *stmt;
  int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, 0);

  if (rc != SQLITE_OK) {
    fprintf(stderr, "Failed to prepare statement: %s\n", sqlite3_errmsg(db));
    return rc;
  }

  int count = 0;

  // Duyệt qua kết quả truy vấn
  while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
    if (count >= 10) {
      break;  // Giới hạn số lượng người dùng trả về là 10
    }

    // Lấy dữ liệu từ các cột của dòng
    users[count].id = sqlite3_column_int(stmt, 0);
    strncpy(users[count].username, (const char *)sqlite3_column_text(stmt, 1), sizeof(users[count].username) - 1);
    users[count].score = sqlite3_column_int(stmt, 2);
    users[count].is_online = sqlite3_column_int(stmt, 3);

    count++;
  }

  *user_count = count;  // Trả về số lượng người dùng online

  sqlite3_finalize(stmt);
  return SQLITE_OK;
}



// typedef struct {
//     char game_id[20];
//     char player1[50];
//     char player2[50];
//     char word[51]; // Word length + 1 for null-termination
//     int result; // 1: Player1 wins, 2: Player2 wins, 0: Draw
//     char moves[100][51]; // Array of moves
//     int move_count;
// } GameHistory;

// int save_game_history_to_db(sqlite3 *db, GameHistory *history) {
//     char *sql = "INSERT INTO game_history (game_id, player1, player2, word, result, moves, move_count) VALUES (?, ?, ?, ?, ?, ?, ?);";
//     sqlite3_stmt *stmt;

//     // Prepare the SQL statement
//     int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, 0);
//     if (rc != SQLITE_OK) {
//         fprintf(stderr, "Failed to prepare statement: %s\n", sqlite3_errmsg(db));
//         return rc;
//     }

//     // Bind values
//     sqlite3_bind_text(stmt, 1, history->game_id, -1, SQLITE_STATIC);
//     sqlite3_bind_text(stmt, 2, history->player1, -1, SQLITE_STATIC);
//     sqlite3_bind_text(stmt, 3, history->player2, -1, SQLITE_STATIC);
//     sqlite3_bind_text(stmt, 4, history->word, -1, SQLITE_STATIC);
//     sqlite3_bind_int(stmt, 5, history->result);

//     // Convert moves array to string (could use JSON or CSV format)
//     char moves_str[1000] = "";
//     for (int i = 0; i < history->move_count; i++) {
//         strcat(moves_str, history->moves[i]);
//         if (i < history->move_count - 1) {
//             strcat(moves_str, "|"); // Separator between moves
//         }
//     }
//     sqlite3_bind_text(stmt, 6, moves_str, -1, SQLITE_STATIC);
//     sqlite3_bind_int(stmt, 7, history->move_count);

//     // Execute the SQL statement
//     rc = sqlite3_step(stmt);
//     if (rc != SQLITE_DONE) {
//         fprintf(stderr, "Execution failed: %s\n", sqlite3_errmsg(db));
//         sqlite3_finalize(stmt);
//         return rc;
//     }

//     sqlite3_finalize(stmt);
//     return SQLITE_OK;
// }

// // Hàm lấy dữ liệu GameHistory theo username
// int get_game_histories_by_username(sqlite3 *db, const char *username, char *response) {
//     const char *sql = "SELECT game_id, player1, player2, word, result, moves, move_count FROM game_history WHERE player1 = ? OR player2 = ? ORDER BY timestamp DESC";
//     sqlite3_stmt *stmt;
//     int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, 0);
//     if (rc != SQLITE_OK) {
//         fprintf(stderr, "Failed to prepare statement: %s\n", sqlite3_errmsg(db));
//         return rc;
//     }

//     // Bind username vào câu truy vấn
//     sqlite3_bind_text(stmt, 1, username, -1, SQLITE_STATIC);
//     sqlite3_bind_text(stmt, 2, username, -1, SQLITE_STATIC);

//     // Duyệt qua kết quả truy vấn và lưu kết quả vào response (JSON format)
//     char buffer[1024];
//     snprintf(response, 2048, "[");

//     while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
//         char game_id[20], player1[50], player2[50], word[51], moves[1024];
//         strncpy(game_id, (const char*)sqlite3_column_text(stmt, 0), sizeof(game_id) - 1);
//         strncpy(player1, (const char*)sqlite3_column_text(stmt, 1), sizeof(player1) - 1);
//         strncpy(player2, (const char*)sqlite3_column_text(stmt, 2), sizeof(player2) - 1);
//         strncpy(word, (const char*)sqlite3_column_text(stmt, 3), sizeof(word) - 1);
//         strncpy(moves, (const char*)sqlite3_column_text(stmt, 5), sizeof(moves) - 1);

//         // Tạo JSON response cho mỗi game history
//         snprintf(buffer, sizeof(buffer), "{\"game_id\": \"%s\", \"player1\": \"%s\", \"player2\": \"%s\", \"word\": \"%s\", \"moves\": \"%s\"},", game_id, player1, player2, word, moves);
//         strcat(response, buffer);
//     }

//     // Xóa dấu phẩy thừa ở cuối
//     if (strlen(response) > 1) {
//         response[strlen(response) - 1] = '\0';  // Loại bỏ dấu phẩy cuối cùng
//     }

//     strcat(response, "]");
//     sqlite3_finalize(stmt);
//     return SQLITE_OK;
// }
