#include <stdio.h>
#include <sqlite3.h>
#include <stdlib.h>

int create_table(sqlite3 *db) {
  const char *sql =
    "CREATE TABLE IF NOT EXISTS user ("
    "id INTEGER PRIMARY KEY AUTOINCREMENT, "
    "username TEXT NOT NULL, "
    "password TEXT NOT NULL, "
    "score INTEGER NOT NULL, "
    "isOnline INTEGER NOT NULL);";

  char *errMsg = 0;
  int rc = sqlite3_exec(db, sql, 0, 0, &errMsg);

  if (rc != SQLITE_OK) {
    printf("SQL error: %s\n", errMsg);
    sqlite3_free(errMsg);
    return rc;
  }

  printf("Table 'user' created successfully or already exists.\n");
  return SQLITE_OK;
}

int insert_sample_data(sqlite3 *db) {
  const char *sql_insert =
    "INSERT INTO user (username, password, score, isOnline) VALUES "
    "('ShadowHunter', '123', 100, 1), "
    "('BlazeFury', '123', 150, 0), "
    "('MysticKnight', '123', 200, 1), "
    "('LunarBlade', '123', 250, 1), "
    "('ArcaneMage', '123', 300, 0), "
    "('SteelTitan', '123', 350, 1), "
    "('StormRider', '123', 400, 0), "
    "('ViperVenom', '123', 450, 1), "
    "('DragonSoul', '123', 500, 0), "
    "('ThunderStrike', '123', 550, 1);";

  char *errMsg = 0;
  int rc = sqlite3_exec(db, sql_insert, 0, 0, &errMsg);

  if (rc != SQLITE_OK) {
    printf("SQL error: %s\n", errMsg);
    sqlite3_free(errMsg);
    return rc;
  }

  printf("Sample data inserted successfully.\n");
  return SQLITE_OK;
}

int main() {
  sqlite3 *db;
  int rc = sqlite3_open("../database.db", &db);

  if (rc) {
    printf("Can't open database: %s\n", sqlite3_errmsg(db));
    return 0;
  } else {
    printf("Opened database successfully.\n");
  }

  // Create table if it does not exist
  if (create_table(db) != SQLITE_OK) {
    sqlite3_close(db);
    return 1;
  }

  // Insert sample data
  if (insert_sample_data(db) != SQLITE_OK) {
    sqlite3_close(db);
    return 1;
  }

  // Close database
  sqlite3_close(db);
  printf("Database connection closed.\n");

  return 0;
}
