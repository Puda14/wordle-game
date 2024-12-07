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

typedef struct {
    int player1_sock;
    int player2_sock;
    char target_word[WORD_LENGTH + 1];
    int current_player;  // 1 or 2
    int player1_attempts;
    int player2_attempts;
    int game_active;
} GameSession;

#define MAX_SESSIONS 15
GameSession game_sessions[MAX_SESSIONS];

char* get_random_word(void);
void check_guess(const char *guess, const char *target, char *result);
int load_words(const char *filename, char words[][WORD_LENGTH + 1]);
void init_wordle(void);
// Signal handler to handle interruptions (SIGINT)
void signal_handler(int sig) {
  got_signal = 1;
  printf("Caught signal %d\n", sig);
}

// Setup the signal handler for SIGINT
void setup_signal_handler() {
  struct sigaction sa;
  sa.sa_handler = signal_handler;
  sa.sa_flags = 0;
  sigemptyset(&sa.sa_mask);
  sigaction(SIGINT, &sa, NULL);
}

int open_database() {
  int rc = sqlite3_open(DB_FILE, &db);
  if (rc) {
    fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
    return 1;
  }
  printf("Database opened successfully\n");
  return 0;
}

void close_database() {
  sqlite3_close(db);
  printf("Database closed successfully\n");
}

int handle_sign_up(const char *username, const char *password) {
  printf("Username: %s\n", username);
  printf("Password: %s\n", password);
  return 0;

//   if (user_exists(db, username)) {
//     return 1;
//   } else {
//     //...
//   }
}

int create_game_session(int player1_sock) {
    for (int i = 0; i < MAX_SESSIONS; i++) {
        if (!game_sessions[i].game_active) {
            game_sessions[i].player1_sock = player1_sock;
            game_sessions[i].player2_sock = -1;
            game_sessions[i].player1_attempts = 0;
            game_sessions[i].player2_attempts = 0;
            game_sessions[i].current_player = 1;
            char *word = get_random_word();
            strcpy(game_sessions[i].target_word, word);
            game_sessions[i].game_active = 1;
            return i;
        }
    }
    return -1;
}
void handle_message(int client_sock, Message *message) {
  if (message->message_type == SIGNUP_REQUEST) {
    char username[50], password[50];

    sscanf(message->payload, "%49[^|]|%49s", username, password);

    int signup_status = handle_sign_up(username, password);

    if (signup_status == 0) {
      message->status = CREATED;
      strcpy(message->payload, "Sign up successful");
    } else if (signup_status == 1) {
      message->status = BAD_REQUEST;
      strcpy(message->payload, "Username already exists");
    } else {
      message->status = INTERNAL_SERVER_ERROR;
      strcpy(message->payload, "Sign up failed");
    }

    send(client_sock, message, sizeof(Message), 0);
  }
  if (message->message_type == GAME_START) {
    printf("Received game start request\n");
        int session_id = create_game_session(client_sock);
        sprintf(message->payload, "%d", session_id);
        message->status = SUCCESS;
        send(client_sock, message, sizeof(Message), 0);
    }
  else if (message->message_type == GAME_GET_TARGET) {
    printf("Received get target request\n");
        int session_id;
        sscanf(message->payload, "%d", &session_id);
        
        if (session_id >= 0 && session_id < MAX_SESSIONS && 
            game_sessions[session_id].game_active) {
            strcpy(message->payload, game_sessions[session_id].target_word);
            message->status = SUCCESS;
        } else {
            strcpy(message->payload, "Invalid session");
            message->status = INTERNAL_SERVER_ERROR;
        }
        send(client_sock, message, sizeof(Message), 0);
    }
    else if (message->message_type == GAME_GUESS) {
      printf("Received game guess\n");
        int session_id;
        char guess[WORD_LENGTH + 1];
        sscanf(message->payload, "%d|%s", &session_id, guess);
        
        GameSession *session = &game_sessions[session_id];
        int player_num = (client_sock == session->player1_sock) ? 1 : 2;
        
        if (player_num != session->current_player) {
            strcpy(message->payload, "Not your turn");
            message->status = BAD_REQUEST;
        }
        else {
            char result[WORD_LENGTH + 1];
            check_guess(guess, session->target_word, result);
            
            if (player_num == 1) {
                session->player1_attempts++;
                if (session->player1_attempts >= 6) {
                    session->current_player = 2;
                }
            } else {
                session->player2_attempts++;
                if (session->player2_attempts >= 6) {
                    session->current_player = 1;
                }
            }
            // Check win condition
          int game_over = 0;
          if (strcmp(guess, session->target_word) == 0) {
              sprintf(message->payload, "WIN|%s|%d|%d|%d", 
                      result, player_num, session->player1_attempts, session->player2_attempts);
              game_over = 1;
          }
          // Check lose condition
          else if (session->player1_attempts >= 6 && session->player2_attempts >= 6) {
              sprintf(message->payload, "LOSE|%s|%d|%d|%d", 
                      result, player_num, session->player1_attempts, session->player2_attempts);
              game_over = 1;
          }
          // Game continues
          else {
              sprintf(message->payload, "CONTINUE|%s|%d|%d|%d", 
                      result, session->current_player, session->player1_attempts, session->player2_attempts);
          }

          message->status = SUCCESS;

          // Send update to both players
          send(session->player1_sock, message, sizeof(Message), 0);
          send(session->player2_sock, message, sizeof(Message), 0);

          // Clean up finished game
          if (game_over) {
              session->game_active = 0;
          }
    }
}
}

void init_game_sessions() {
    for (int i = 0; i < MAX_SESSIONS; i++) {
        game_sessions[i].game_active = 0;
    }
}




void handle_game_message(int client_sock, Message *message) {
    
}

int initialize_server(int *server_sock, struct sockaddr_in *server_addr) {

  // Create the server socket
  if ((*server_sock = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
    perror("Socket failed");
    exit(EXIT_FAILURE);
  }

  server_addr->sin_family = AF_INET;
  server_addr->sin_addr.s_addr = INADDR_ANY;
  server_addr->sin_port = htons(PORT);

  // Bind the socket to the specified IP address and port
  if (bind(*server_sock, (struct sockaddr *)server_addr, sizeof(*server_addr)) < 0) {
    perror("Bind failed");
    exit(EXIT_FAILURE);
  }

  // Start listening for incoming client connections
  if (listen(*server_sock, 3) < 0) {
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
typedef struct {
    char target_word[WORD_LENGTH + 1];
    int attempts_left;
    int game_won;
} GameState;

// Add functions
int is_valid_guess(const char *guess) {
    for (int i = 0; i < guess_count; i++) {
        if (strcmp(guess, valid_guesses[i]) == 0) {
            return 1;
        }
    }
    for (int i = 0; i < solution_count; i++) {
        if (strcmp(guess, valid_solutions[i]) == 0) {
            return 1;
        }
    }
    return 0;
}

int load_words(const char *filename, char words[][WORD_LENGTH + 1]) {
    FILE *file = fopen(filename, "r");
    if (file == NULL) {
        perror("Error opening file");
        return -1;
    }
    int count = 0;
    while (fscanf(file, "%5s", words[count]) == 1) {
        count++;
        if (count >= MAX_WORDS) break;
    }
    fclose(file);
    return count;
}

char* get_random_word() {
    int index = rand() % solution_count;
    return valid_solutions[index];
}

void check_guess(const char *guess, const char *target, char *result) {
    // Initialize all positions as wrong
    memset(result, 'X', WORD_LENGTH);
    result[WORD_LENGTH] = '\0';
    
    // Track used letters in target
    int used[WORD_LENGTH] = {0};
    
    // First pass - mark correct positions
    for (int i = 0; i < WORD_LENGTH; i++) {
        if (guess[i] == target[i]) {
            result[i] = 'G';  // Green
            used[i] = 1;
        }
    }
    
    // Second pass - mark wrong positions
    for (int i = 0; i < WORD_LENGTH; i++) {
        if (result[i] == 'X') {
            for (int j = 0; j < WORD_LENGTH; j++) {
                if (!used[j] && guess[i] == target[j]) {
                    result[i] = 'Y';  // Yellow
                    used[j] = 1;
                    break;
                }
            }
        }
    }
}

// Add to main() after database initialization
void init_wordle() {
    srand(time(NULL));
    solution_count = load_words("valid_solutions.txt", valid_solutions);
    guess_count = load_words("valid_guesses.txt", valid_guesses);
    printf("Loaded %d solutions and %d guesses.\n", solution_count, guess_count);

    if (solution_count == -1 || guess_count == -1) {
        printf("Failed to load word lists.\n");
        exit(1);
    }
}

int main() {
  int server_sock, new_sock, client_socks[MAX_CLIENTS] = {0};
  struct sockaddr_in server_addr, client_addr;
  fd_set readfds;
  socklen_t addr_len = sizeof(client_addr);
  sigset_t block_mask, orig_mask;

  int rc = open_database();
  if (rc) {
    return 1;
  }

  init_wordle();

  // Set up the signal handler
  setup_signal_handler();

  // Block SIGINT signal to handle it later
  sigemptyset(&block_mask);
  sigaddset(&block_mask, SIGINT);
  sigprocmask(SIG_BLOCK, &block_mask, &orig_mask);

  // Initialize server socket and bind to address
  initialize_server(&server_sock, &server_addr);

  while (1) {
    FD_ZERO(&readfds);
    FD_SET(server_sock, &readfds);
    int max_sd = server_sock;

    // Add client sockets to the read set
    for (int i = 0; i < MAX_CLIENTS; i++) {
      int sock = client_socks[i];
      if (sock > 0) FD_SET(sock, &readfds);
      if (sock > max_sd) max_sd = sock;
    }

    // Wait for activity on any of the sockets
    int ready = pselect(max_sd + 1, &readfds, NULL, NULL, NULL, &orig_mask);
    if (ready == -1) {
      if (errno == EINTR) {
        printf("pselect() interrupted by signal.\n");
        if (got_signal) {
          printf("Received SIGINT, shutting down server.\n");
          break;  // Exit the loop when SIGINT is received
        }
        continue;
      } else {
        perror("pselect");
        exit(EXIT_FAILURE);
      }
    }

    // Check if there's a new incoming connection
    if (FD_ISSET(server_sock, &readfds)) {
      if ((new_sock = accept(server_sock, (struct sockaddr *)&client_addr, &addr_len)) < 0) {
        perror("Accept failed");
        continue;
      }
      printf("New connection, socket fd is %d\n", new_sock);

      // Add the new socket to the client sockets array
      for (int i = 0; i < MAX_CLIENTS; i++) {
        if (client_socks[i] == 0) {
          client_socks[i] = new_sock;
          break;
        }
      }
    }

    // Handle messages from connected clients
    for (int i = 0; i < MAX_CLIENTS; i++) {
      int sock = client_socks[i];
      if (FD_ISSET(sock, &readfds)) {
        Message message;
        int read_size = recv(sock, &message, sizeof(Message), 0);
        if (read_size == 0) {
          // Client disconnected
          close(sock);
          client_socks[i] = 0;
        } else {
          // Process the received message
          handle_message(sock, &message);
        }
      }
    }

    // Check if a signal was received after each pselect call
    if (got_signal) {
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
