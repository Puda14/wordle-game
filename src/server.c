#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <signal.h>
#include <errno.h>
#include <sys/select.h>
#include <sqlite3.h>
#include <time.h>
#include "database.h"
#include "./model/message.h"

#define PORT 8080
#define MAX_CLIENTS 30
#define BUFFER_SIZE 1024
#define DB_FILE "database.db"
#define WORD_LENGTH 5
#define MAX_WORDS 15000
#define MAX_ATTEMPTS 12

volatile sig_atomic_t got_signal = 0;
sqlite3 *db;

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
  int game_active;
  int current_attempts;
  PlayTurn turns[MAX_ATTEMPTS];
} GameSession;

typedef struct{
  char game_id[20];
  char player1_name[50];
  char player2_name[50];
  char target_word[WORD_LENGTH + 1];
  int player1_attempts;
  int player2_attempts;
  int player1_score;
  int player2_score;
  int game_result;
  int current_attempts;
  PlayTurn turns[MAX_ATTEMPTS];
}GameHistory;



void generate_game_id(char *game_id, size_t size)
{
  time_t now = time(NULL);
  snprintf(game_id, size, "GAME-%ld", now);
}

#define MAX_SESSIONS 15
GameSession game_sessions[MAX_SESSIONS];

typedef struct
{
  char player_name[50]; // Tên người chơi
  int player_sock;      // Socket của người chơi
} PlayerInfo;

#define MAX_PLAYERS 100

PlayerInfo player_list[MAX_PLAYERS]; // Mảng chứa thông tin các player
int player_count = 0;                // Số lượng người chơi hiện tại

int add_player(const char *player_name, int player_sock)
{
  // Kiểm tra nếu mảng đã đầy
  if (player_count >= MAX_PLAYERS)
  {
    printf("Player list is full!\n");
    return -1; // Danh sách người chơi đầy
  }

  // Kiểm tra nếu player đã tồn tại
  for (int i = 0; i < player_count; i++)
  {
    if (strcmp(player_list[i].player_name, player_name) == 0)
    {
      printf("Player %s already exists!\n", player_name);
      return -1; // Player đã tồn tại
    }
  }

  // Thêm player mới vào mảng
  strcpy(player_list[player_count].player_name, player_name);
  player_list[player_count].player_sock = player_sock;
  player_count++;

  return 0; // Thêm thành công
}

int get_player_sock(const char *player_name)
{
  for (int i = 0; i < player_count; i++)
  {
    if (strcmp(player_list[i].player_name, player_name) == 0)
    {
      return player_list[i].player_sock; // Trả về socket của player
    }
  }

  printf("Player %s not found!\n", player_name);
  return -1; // Không tìm thấy player
}

char *get_random_word(void);
void check_guess(const char *guess, const char *target, char *result);
int load_words(const char *filename, char words[][WORD_LENGTH + 1]);
void init_wordle(void);
// Signal handler to handle interruptions (SIGINT)
void signal_handler(int sig)
{
  got_signal = 1;
  printf("Caught signal %d\n", sig);
}

// Setup the signal handler for SIGINT
void setup_signal_handler()
{
  struct sigaction sa;
  sa.sa_handler = signal_handler;
  sa.sa_flags = 0;
  sigemptyset(&sa.sa_mask);
  sigaction(SIGINT, &sa, NULL);
}

int open_database()
{
  int rc = sqlite3_open(DB_FILE, &db);
  if (rc)
  {
    fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
    return 1;
  }
  printf("Database opened successfully\n");
  return 0;
}

void close_database()
{
  sqlite3_close(db);
  printf("Database closed successfully\n");
}

int handle_sign_up(const char *username, const char *password)
{
  printf("Username: %s\n", username);
  printf("Password: %s\n", password);
  return 0;

  //   if (user_exists(db, username)) {
  //     return 1;
  //   } else {
  //     //...
  //   }
}

// Tạo game mới với Player1 và Player2
int create_game_session(const char *player1_name, const char *player2_name)
{
  for (int i = 0; i < MAX_SESSIONS; i++)
  {
    if (!game_sessions[i].game_active)
    {
      generate_game_id(game_sessions[i].game_id, sizeof(game_sessions[i].game_id));
      // Tạo game mới với tên của cả hai người chơi
      strcpy(game_sessions[i].player1_name, player1_name);
      strcpy(game_sessions[i].player2_name, player2_name);
      game_sessions[i].player1_attempts = 0;
      game_sessions[i].player2_attempts = 0;
      game_sessions[i].current_player = 1; // Player 1 bắt đầu
      char *word = get_random_word();
      strcpy(game_sessions[i].target_word, word);
      game_sessions[i].player1_score = 0;
      game_sessions[i].player2_score = 0;
      game_sessions[i].game_active = 1;
      game_sessions[i].current_attempts = 0;
      return i; // Trả về ID game
    }
  }
  return -1; // Không có không gian cho game mới
}

void clear_game_session(int session_id) {
    GameSession *session = &game_sessions[session_id];
    
    // Reset player info
    memset(session->player1_name, 0, sizeof(session->player1_name));
    memset(session->player2_name, 0, sizeof(session->player2_name));
    memset(session->target_word, 0, sizeof(session->target_word));
    
    // Reset game state
    session->current_player = 0;
    session->player1_attempts = 0;
    session->player2_attempts = 0;
    session->game_active = 0;
    session->current_attempts = 0;
    session->player1_score = 0;
    session->player2_score = 0;
    // Clear turns history
    memset(session->turns, 0, sizeof(session->turns));
    
    printf("Cleared game session %d\n", session_id);
}

// Hàm tìm kiếm game đã có người chơi với tên Player1 và Player2
int find_existing_game(const char *player1_name, const char *player2_name)
{
  for (int i = 0; i < MAX_SESSIONS; i++)
  {
    if (game_sessions[i].game_active)
    {
      // Kiểm tra nếu Player1 và Player2 đã có trong game
      if ((strcmp(game_sessions[i].player1_name, player1_name) == 0 &&
           strcmp(game_sessions[i].player2_name, player2_name) == 0) ||
          (strcmp(game_sessions[i].player1_name, player2_name) == 0 &&
           strcmp(game_sessions[i].player2_name, player1_name) == 0))
      {
        return i; // Trả về ID game
      }
    }
  }
  return -1; // Không tìm thấy game
}


// Function to find a user by username
User* find_user_by_username(User users[], int size, const char *username) {
    for (int i = 0; i < size; i++) {
        if (strncmp(users[i].username, username, 50) == 0) {
            return &users[i];  // Return pointer to the found user
        }
    }
    return NULL;  // Return NULL if no user is found
}

void send_turn_update(GameSession *session)
{
  Message message;
  message.message_type = GAME_TURN;

  // Tạo thông báo về lượt chơi
  sprintf(message.payload, "%d", session->current_player);
  printf("Sending turn update to %s and %s : current turn %d\n", session->player1_name, session->player2_name, session->current_player);
  message.status = SUCCESS;

  // Gửi thông báo lượt chơi đến cả hai người chơi
  send(get_player_sock(session->player1_name), &message, sizeof(Message), 0);
  send(get_player_sock(session->player2_name), &message, sizeof(Message), 0);
}

void send_score_update(GameSession *session){
  Message message;
  message.message_type = GAME_SCORE;
  sprintf(message.payload, "%s|%d|%s|%d",session->player1_name, session->player1_score,session->player2_name, session->player2_score);
  message.status = SUCCESS;
  printf("Sending score update to %s and %s\n", session->player1_name, session->player2_name);
  send(get_player_sock(session->player1_name), &message, sizeof(Message), 0);
  send(get_player_sock(session->player2_name), &message, sizeof(Message), 0);
}

void handle_message(int client_sock, Message *message)
{
  if(message->message_type == SIGNUP_REQUEST){
    char username[50], password[50];
    sscanf(message->payload, "%49[^|]|%49s", username, password);

    if (username == NULL || strlen(username) == 0 || password == NULL || strlen(password) == 0) {
      message->status = BAD_REQUEST;
      strcpy(message->payload, "Username or password is missing.");
    } else if (user_exists(db, username)) {
      message->status = BAD_REQUEST;
      strcpy(message->payload, "Username already exists.");
    } else {
      User new_user;
      strncpy(new_user.username, username, sizeof(new_user.username) - 1);
      new_user.username[sizeof(new_user.username) - 1] = '\0';
      strncpy(new_user.password, password, sizeof(new_user.password) - 1);
      new_user.password[sizeof(new_user.password) - 1] = '\0';
      new_user.score = 0;
      new_user.is_online = 0;

      int rc = create_user(db, &new_user);
      if (rc == SQLITE_OK) {
        message->status = SUCCESS;
        strcpy(message->payload, "User registered successfully.");
      } else {
        message->status = INTERNAL_SERVER_ERROR;
        strcpy(message->payload, "Error occurred while registering user.");
      }
    }
    send(client_sock, message, sizeof(Message), 0);
  }
  else if (message->message_type == LOGIN_REQUEST)
  {
    char username[50], password[50];
    sscanf(message->payload, "%49[^|]|%49s", username, password);

    int login_status = authenticate_user(db, username, password);

    if (login_status == 1) {
      int update_status = update_user_online(db, username);
      if (update_status != SQLITE_DONE) {
        message->status = INTERNAL_SERVER_ERROR;
        strcpy(message->payload, "Failed to update user status");
      } else {
        message->status = SUCCESS;
        strcpy(message->payload, "Login successful");
        if (add_player(username, client_sock) == 0)
        {
          printf("Player %s connected with socket %d\n", username, client_sock);
        }
        else
        {
          printf("Failed to add player %s\n", username);
          close(client_sock);
        }
      }
    } else if (login_status == 0) {
      message->status = UNAUTHORIZED;
      strcpy(message->payload, "Invalid username or password");
    } else {
      message->status = INTERNAL_SERVER_ERROR;
      strcpy(message->payload, "Login failed");
    }
    send(client_sock, message, sizeof(Message), 0);
  } else if (message->message_type == LOGOUT_REQUEST) {
    char username[50], password[50];
    sscanf(message->payload, "%49[^|]|%49s", username, password);
    printf("Logout request from user: %s\n", username);
    int auth_status = authenticate_user(db, username, password);
    if (auth_status == 1) {
        int update_status = update_user_offline(db, username);
        if (update_status == SQLITE_DONE || update_status == SQLITE_OK) {
            message->status = SUCCESS;
            strcpy(message->payload, "Logout successful");
        } else {
            message->status = INTERNAL_SERVER_ERROR;
            strcpy(message->payload, "Failed to update user status");
        }
    } else if (auth_status == 0) {
        message->status = UNAUTHORIZED;
        strcpy(message->payload, "Invalid username or password");
    } else {
        message->status = INTERNAL_SERVER_ERROR;
        strcpy(message->payload, "Logout failed");
    }
    send(client_sock, message, sizeof(Message), 0);
  } else if (message->message_type == LIST_USER) {
    User users[10];
    int user_count = 0;

    int rc = list_users_online(db, users, &user_count);

    if (rc == SQLITE_OK) {
      char response[2048] = {0};
      char buffer[128];

      for (int i = 0; i < user_count; i++) {
        snprintf(buffer, sizeof(buffer), "ID: %d, Username: %s, Score: %d, Online: %d\n",
                  users[i].id, users[i].username, users[i].score, users[i].is_online);
        strncat(response, buffer, 2048 - strlen(response) - 1);
      }

      if (strlen(response) > 1) {
        response[strlen(response) - 1] = '\0';
      }
      strcpy(message->payload, response);
      message->status = SUCCESS;
    } else if (rc == NOT_FOUND) {
        message->status = NOT_FOUND;
        strcpy(message->payload, "No online users found.");
    } else {
        message->status = INTERNAL_SERVER_ERROR;
        strcpy(message->payload, "Internal server error occurred.");
    }

    send(client_sock, message, sizeof(Message), 0);
  } else if(message->message_type == CHALLANGE_REQUEST){
    char player1[50], player2[50];
    sscanf(message->payload, "CHALLANGE_REQUEST|%49[^|]|%49s", player1, player2);
    int player1_sock = get_player_sock(player1);
    int player2_sock = get_player_sock(player2);
    if(player1_sock == -1 || player2_sock == -1){
      message->status = BAD_REQUEST;
      strcpy(message->payload, "Player not found");
      send(client_sock, message, sizeof(Message), 0);
      return;
    }
    message->status = SUCCESS;
    send(player1_sock, message, sizeof(Message), 0);
    send(player2_sock, message, sizeof(Message), 0);
  } else if(message->message_type == CHALLANGE_RESPONSE){
    char player1[50], player2[50], response[50];
    sscanf(message->payload, "CHALLANGE_RESPONSE|%49[^|]|%49[^|]|%49s", player1, player2, response);
    int player1_sock = get_player_sock(player1);
    int player2_sock = get_player_sock(player2);
    if(player1_sock == -1 || player2_sock == -1){
      message->status = BAD_REQUEST;
      strcpy(message->payload, "Player not found");
      send(client_sock, message, sizeof(Message), 0);
      return;
    }
    if(strcmp(response, "ACCEPT") == 0){
      message->status = SUCCESS;
      send(player1_sock, message, sizeof(Message), 0);

      send(player2_sock, message, sizeof(Message), 0);
    }else{
      message->status = BAD_REQUEST;
      strcpy(message->payload, "Challange rejected");
      send(player1_sock, message, sizeof(Message), 0);
    }
  } else if (message->message_type == GAME_START){
    printf("Received game request\n");

    char player1_name[50], player2_name[50];
    sscanf(message->payload, "%49[^|]|%49s", player1_name, player2_name); // Lấy tên của cả 2 người chơi

    // Kiểm tra nếu người chơi là Player 1
    if (client_sock == get_player_sock(player1_name))
    {
      int session_id = find_existing_game(player1_name, player2_name);
      if (session_id != -1)
      {
        printf("Game session found with ID %d between %s and %s\n", session_id, player1_name, player2_name);
        message->status = SUCCESS;
        GameSession *session = &game_sessions[session_id];
        int player_num = (strcmp(player1_name, session->player1_name) == 0) ? 1 : 2;
        sprintf(message->payload, "%d|%d", session_id, player_num);
      }
      else
      {
        printf("Creating game session between %s and %s\n", player1_name, player2_name);
        session_id = create_game_session(player1_name, player2_name);
        if (session_id != -1)
        {
          message->status = SUCCESS;
          GameSession *session = &game_sessions[session_id];
          int player_num = (strcmp(player1_name, session->player1_name) == 0) ? 1 : 2;
          sprintf(message->payload, "%d|%d", session_id, player_num);
        }
        else
        {
          message->status = INTERNAL_SERVER_ERROR;
          strcpy(message->payload, "Failed to create game session");
        }
      }
    }
    else if (client_sock == get_player_sock(player2_name))
    {
      int session_id = find_existing_game(player1_name, player2_name);
      printf("Game session found with ID %d between %s and %s\n", session_id, player1_name, player2_name);
      if (session_id != -1)
      {
        message->status = SUCCESS;
        sprintf(message->payload, "%d", session_id);
      }
      else
      {
        message->status = BAD_REQUEST;
        strcpy(message->payload, "Game not found");
      }
    }
    // Gửi phản hồi cho cả hai người chơi
    send(client_sock, message, sizeof(Message), 0);
  } else if (message->message_type == GAME_GET_TARGET){
    printf("Received get target request\n");
    printf("Payload: %s\n", message->payload);
    int session_id;
    sscanf(message->payload, "%d", &session_id);

    if (session_id >= 0 && session_id < MAX_SESSIONS && game_sessions[session_id].game_active)
    {
      strcpy(message->payload, game_sessions[session_id].target_word);
      message->status = SUCCESS;
    }
    else
    {
      strcpy(message->payload, "Invalid session");
      message->status = INTERNAL_SERVER_ERROR;
    }
    send(client_sock, message, sizeof(Message), 0);
  } else if (message->message_type == GAME_GUESS){
    printf("Received game guess\n");
    printf("Payload: %s\n", message->payload);
    int session_id;
    char guess[WORD_LENGTH + 1];
    char player_name[50];
    sscanf(message->payload, "%d|%49[^|]|%49s", &session_id, player_name, guess);

    GameSession *session = &game_sessions[session_id];
    User user;
    int get_user = get_user_by_username(db, player_name, &user);
    if (get_user == SQLITE_OK) {
      printf("User found: %s\n", user.username);
    } else {
      printf("User not found\n");
      // message->status = NOT_FOUND;
      // strcpy(message->payload, "User not found");
      // send(client_sock, message, sizeof(Message), 0);
      // return;
    }
    int player_num = (strcmp(player_name, session->player1_name) == 0) ? 1 : 2;

    if (player_num != session->current_player)
    {
      strcpy(message->payload, "Not your turn");
      message->status = BAD_REQUEST;
      send(client_sock, message, sizeof(Message), 0);
      return;
    }

    // Check the player's guess
    char result[WORD_LENGTH + 1];
    check_guess(guess, session->target_word, result);
    // Calculate points
    int points = 0;
    for (int i = 0; i < WORD_LENGTH; i++) {
        if (result[i] == 'G') {
            points += 1;
        }
    }
    // Update attempts and turn
    if (player_num == 1)
    {
      session->player1_attempts++;
      session->player1_score += points;
      if (session->player1_attempts < 6)
      {
        session->current_player = 2; // Pass turn to player 2
      }
    }
    else
    {
      session->player2_attempts++;
      session->player2_score += points;
      if (session->player2_attempts < 6)
      {
        session->current_player = 1; // Pass turn to player 1
      }
    }

    // Save the guess
    strcpy(session->turns[session->current_attempts].player_name, player_name);
    strcpy(session->turns[session->current_attempts].guess, guess);
    strcpy(session->turns[session->current_attempts].result, result);
    
    // Update current attempts
    session->current_attempts++;
    // Check game-over conditions
    int game_over = 0;
    if (strcmp(guess, session->target_word) == 0)
    {
      points += 20 - session->current_attempts; // Bonus points for correct guess
      if (player_num == 1){
        session->player1_score += points;
      } else {
        session->player2_score += points;
      }
      sprintf(message->payload, "WIN|%s|%d|%d|%d|%s",
              result, player_num, session->player1_attempts, session->player2_attempts, guess);
      game_over = 1;
    }
    else if (session->player1_attempts >= 6 && session->player2_attempts >= 6)
    {
      sprintf(message->payload, "DRAW|%d|%d", session->player1_attempts, session->player2_attempts);
      game_over = 1;
    }
    else
    {
      sprintf(message->payload, "CONTINUE|%s|%d|%d|%d|%s",
              result, session->current_player, session->player1_attempts, session->player2_attempts, guess);
    }

    // Update user score
    user.score += points;
    update_user_score(db, user.username, user.score);

    message->status = SUCCESS;

    // Send the result of the guess to both players
    send(get_player_sock(session->player1_name), message, sizeof(Message), 0);
    printf("Sent guess result to %s\n", session->player1_name);
    send(get_player_sock(session->player2_name), message, sizeof(Message), 0);
    printf("Sent guess result to %s\n", session->player2_name);
    send_score_update(session);
    // If game is over, end the session
    if (game_over)
    {
      session->game_active = 0;

      // Send a final turn update to both players
      Message turn_message;
      turn_message.message_type = GAME_TURN;
      sprintf(turn_message.payload, "%d", 0);
      turn_message.status = SUCCESS;
      send(get_player_sock(session->player1_name), &turn_message, sizeof(Message), 0);
      send(get_player_sock(session->player2_name), &turn_message, sizeof(Message), 0);
      clear_game_session(session_id);
    }
    else
    {
      // Send turn update to both players
      Message turn_message;
      turn_message.message_type = GAME_TURN;
      sprintf(turn_message.payload, "%d", session->current_player);
      turn_message.status = SUCCESS;
      send(get_player_sock(session->player1_name), &turn_message, sizeof(Message), 0);
      send(get_player_sock(session->player2_name), &turn_message, sizeof(Message), 0);
    }
  } else if (message->message_type == GAME_UPDATE)
  {
    printf("Received game update\n");
    printf("Payload: %s\n", message->payload);
    int session_id;
    char guess[WORD_LENGTH + 1];
    char player_name[50];
    sscanf(message->payload, "%d|%49", &session_id, player_name);

    GameSession *session = &game_sessions[session_id];
  }
}

void init_game_sessions()
{
  for (int i = 0; i < MAX_SESSIONS; i++)
  {
    game_sessions[i].game_active = 0;
  }
}

void handle_game_message(int client_sock, Message *message)
{
}

int initialize_server(int *server_sock, struct sockaddr_in *server_addr)
{

  // Create the server socket
  if ((*server_sock = socket(AF_INET, SOCK_STREAM, 0)) == 0)
  {
    perror("Socket failed");
    exit(EXIT_FAILURE);
  }

  server_addr->sin_family = AF_INET;
  server_addr->sin_addr.s_addr = INADDR_ANY;
  server_addr->sin_port = htons(PORT);

  // Bind the socket to the specified IP address and port
  if (bind(*server_sock, (struct sockaddr *)server_addr, sizeof(*server_addr)) < 0)
  {
    perror("Bind failed");
    exit(EXIT_FAILURE);
  }

  // Start listening for incoming client connections
  if (listen(*server_sock, 3) < 0)
  {
    perror("Listen failed");
    exit(EXIT_FAILURE);
  }

  printf("Server listening on port %d\n", PORT);
  return 0;
}

// Add word lists and counts
char valid_solutions[MAX_WORDS][WORD_LENGTH + 1];
char valid_guesses[MAX_WORDS][WORD_LENGTH + 1];
int solution_count = 0;
int guess_count = 0;

// Game state structure
typedef struct
{
  char target_word[WORD_LENGTH + 1];
  int attempts_left;
  int game_won;
} GameState;

// Add functions
int is_valid_guess(const char *guess)
{
  for (int i = 0; i < guess_count; i++)
  {
    if (strcmp(guess, valid_guesses[i]) == 0)
    {
      return 1;
    }
  }
  for (int i = 0; i < solution_count; i++)
  {
    if (strcmp(guess, valid_solutions[i]) == 0)
    {
      return 1;
    }
  }
  return 0;
}

int load_words(const char *filename, char words[][WORD_LENGTH + 1])
{
  FILE *file = fopen(filename, "r");
  if (file == NULL)
  {
    perror("Error opening file");
    return -1;
  }
  int count = 0;
  while (fscanf(file, "%5s", words[count]) == 1)
  {
    count++;
    if (count >= MAX_WORDS)
      break;
  }
  fclose(file);
  return count;
}

char *get_random_word()
{
  int index = rand() % solution_count;
  return valid_solutions[index];
}

void check_guess(const char *guess, const char *target, char *result)
{
  // Initialize all positions as wrong
  memset(result, 'X', WORD_LENGTH);
  result[WORD_LENGTH] = '\0';

  // Track used letters in target
  int used[WORD_LENGTH] = {0};

  // First pass - mark correct positions
  for (int i = 0; i < WORD_LENGTH; i++)
  {
    if (guess[i] == target[i])
    {
      result[i] = 'G'; // Green
      used[i] = 1;
    }
  }

  // Second pass - mark wrong positions
  for (int i = 0; i < WORD_LENGTH; i++)
  {
    if (result[i] == 'X')
    {
      for (int j = 0; j < WORD_LENGTH; j++)
      {
        if (!used[j] && guess[i] == target[j])
        {
          result[i] = 'Y'; // Yellow
          used[j] = 1;
          break;
        }
      }
    }
  }
}

// Add to main() after database initialization
void init_wordle()
{
  srand(time(NULL));
  solution_count = load_words("valid_solutions.txt", valid_solutions);
  guess_count = load_words("valid_guesses.txt", valid_guesses);
  printf("Loaded %d solutions and %d guesses.\n", solution_count, guess_count);

  if (solution_count == -1 || guess_count == -1)
  {
    printf("Failed to load word lists.\n");
    exit(1);
  }
}

int main()
{
  int server_sock, new_sock, client_socks[MAX_CLIENTS] = {0};
  struct sockaddr_in server_addr, client_addr;
  fd_set readfds;
  socklen_t addr_len = sizeof(client_addr);
  sigset_t block_mask, orig_mask;

  int rc = open_database();
  if (rc)
  {
    return 1;
  }


  // Seed the users
  // seed_users(users, &size);
  init_wordle();

  // Set up the signal handler
  setup_signal_handler();

  // Block SIGINT signal to handle it later
  sigemptyset(&block_mask);
  sigaddset(&block_mask, SIGINT);
  sigprocmask(SIG_BLOCK, &block_mask, &orig_mask);

  // Initialize server socket and bind to address
  initialize_server(&server_sock, &server_addr);

  while (1)
  {
    FD_ZERO(&readfds);
    FD_SET(server_sock, &readfds);
    int max_sd = server_sock;

    // Add client sockets to the read set
    for (int i = 0; i < MAX_CLIENTS; i++)
    {
      int sock = client_socks[i];
      if (sock > 0)
        FD_SET(sock, &readfds);
      if (sock > max_sd)
        max_sd = sock;
    }

    // Wait for activity on any of the sockets
    int ready = pselect(max_sd + 1, &readfds, NULL, NULL, NULL, &orig_mask);
    if (ready == -1)
    {
      if (errno == EINTR)
      {
        printf("pselect() interrupted by signal.\n");
        if (got_signal)
        {
          printf("Received SIGINT, shutting down server.\n");
          break; // Exit the loop when SIGINT is received
        }
        continue;
      }
      else
      {
        perror("pselect");
        exit(EXIT_FAILURE);
      }
    }

    // Check if there's a new incoming connection
    if (FD_ISSET(server_sock, &readfds))
    {
      if ((new_sock = accept(server_sock, (struct sockaddr *)&client_addr, &addr_len)) < 0)
      {
        perror("Accept failed");
        continue;
      }
      printf("New connection, socket fd is %d\n", new_sock);

      // Add the new socket to the client sockets array
      for (int i = 0; i < MAX_CLIENTS; i++)
      {
        if (client_socks[i] == 0)
        {
          client_socks[i] = new_sock;
          break;
        }
      }
    }

    // Handle messages from connected clients
    for (int i = 0; i < MAX_CLIENTS; i++)
    {
      int sock = client_socks[i];
      if (FD_ISSET(sock, &readfds))
      {
        Message message;
        int read_size = recv(sock, &message, sizeof(Message), 0);
        if (read_size == 0)
        {
          // Client disconnected
          close(sock);
          client_socks[i] = 0;
        }
        else
        {
          // Process the received message
          handle_message(sock, &message);
        }
      }
    }

    // Check if a signal was received after each pselect call
    if (got_signal)
    {
      printf("Received SIGINT, shutting down server.\n");
      break;
    }
  }

  // Close the database connection and the server socket before exiting
  close_database();
  close(server_sock);
  printf("Server stopped.\n");
  return 0;
}
