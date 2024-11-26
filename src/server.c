#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <signal.h>
#include <errno.h>
#include <sys/select.h>
#include <sqlite3.h>
#include "database.h"
#include "./model/message.h"

#define PORT 8080
#define MAX_CLIENTS 30
#define BUFFER_SIZE 1024
#define DB_FILE "database.db"

volatile sig_atomic_t got_signal = 0;
sqlite3 *db;

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
