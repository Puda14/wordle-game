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
