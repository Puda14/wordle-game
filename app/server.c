  #include <stdio.h>
  #include <stdlib.h>
  #include <string.h>
  #include <arpa/inet.h>
  #include <sys/select.h>

  #define PORT 8080
  #define MAX_USERS 100
  #define BUFFER_SIZE 1024
  #define MAX_CLIENTS 30
  #define USER_FILE "users.txt"

  // typedef struct {
  //     uint8_t message_type;
  //     char payload[BUFFER_SIZE];
  // } Message;

  // typedef struct {
  //     char username[50];
  //     char password[50];
  //     int status;
  // } User;

  #include <../model/message.h>
  #include <../model/user.h>

  User users[MAX_USERS];
  int user_count = 0;

  enum MessageType { REGISTER_REQUEST = 0, LOGIN_REQUEST = 1, LOGOUT_REQUEST = 2 };

  int register_user(const char *username, const char *password);
  int login_user(const char *username, const char *password);
  void logout_user(const char *username);
  void handle_message(int client_sock, Message *message);
  void load_users_from_file();
  void save_user_to_file(const User *user);

  int main() {
      int server_sock, new_sock, client_socks[MAX_CLIENTS];
      struct sockaddr_in server_addr, client_addr;
      fd_set read_fds;
      socklen_t addr_len = sizeof(client_addr);

      // Load user data from file on server startup
      load_users_from_file();

      // Initialize client socket array
      for (int i = 0; i < MAX_CLIENTS; i++) client_socks[i] = 0;

      // Create socket
      server_sock = socket(AF_INET, SOCK_STREAM, 0);
      server_addr.sin_family = AF_INET;
      server_addr.sin_addr.s_addr = INADDR_ANY;
      server_addr.sin_port = htons(PORT);

      bind(server_sock, (struct sockaddr *)&server_addr, sizeof(server_addr));
      listen(server_sock, 3);
      printf("Server listening on port %d\n", PORT);

      while (1) {
          FD_ZERO(&read_fds);
          FD_SET(server_sock, &read_fds);
          int max_sd = server_sock;

          for (int i = 0; i < MAX_CLIENTS; i++) {
              int sock = client_socks[i];
              if (sock > 0) FD_SET(sock, &read_fds);
              if (sock > max_sd) max_sd = sock;
          }

          select(max_sd + 1, &read_fds, NULL, NULL, NULL);

          // New connection
          if (FD_ISSET(server_sock, &read_fds)) {
              new_sock = accept(server_sock, (struct sockaddr *)&client_addr, &addr_len);
              printf("New connection accepted\n");

              for (int i = 0; i < MAX_CLIENTS; i++) {
                  if (client_socks[i] == 0) {
                      client_socks[i] = new_sock;
                      printf("Added client socket to list: %d\n", new_sock);
                      break;
                  }
              }
          }

          // Handle I/O for each client
          for (int i = 0; i < MAX_CLIENTS; i++) {
              int sock = client_socks[i];
              if (FD_ISSET(sock, &read_fds)) {
                  Message message;
                  int read_size = recv(sock, &message, sizeof(Message), 0);
                  if (read_size == 0) {
                      close(sock);
                      client_socks[i] = 0;
                  } else {
                      handle_message(sock, &message);
                  }
              }
          }
      }

      return 0;
  }

  void handle_message(int client_sock, Message *message) {
      if (message->message_type == REGISTER_REQUEST) {
          char username[50], password[50];
          sscanf(message->payload, "%49[^|]|%49s", username, password);

          if (register_user(username, password)) {
              printf("User %s Pwd %s registrated\n",username,password);
              strcpy(message->payload, "Registration successful\n");
          } else {
              strcpy(message->payload, "Registration failed: User may already exist\n");
          }
          send(client_sock, message, sizeof(Message), 0);

      } else if (message->message_type == LOGIN_REQUEST) {
          char username[50], password[50];
          sscanf(message->payload, "%49[^|]|%49s", username, password);

          if (login_user(username, password)) {
              printf("User %s Pwd %s login\n",username,password);
              strcpy(message->payload, "Login successful\n");
          } else {
              strcpy(message->payload, "Login failed\n");
          }
          send(client_sock, message, sizeof(Message), 0);

      } else if (message->message_type == LOGOUT_REQUEST) {
          char username[50];
          sscanf(message->payload, "%49s", username);
          logout_user(username);
          printf("User %s logout\n",username);
          strcpy(message->payload, "Logout successful\n");
          send(client_sock, message, sizeof(Message), 0);
      }
  }

  int register_user(const char *username, const char *password) {
      for (int i = 0; i < user_count; i++) {
          if (strcmp(users[i].username, username) == 0) {
              return 0; // User already exists
          }
      }

      if (user_count < MAX_USERS) {
          strcpy(users[user_count].username, username);
          strcpy(users[user_count].password, password);
          users[user_count].status = 0;
          save_user_to_file(&users[user_count]); // Save new user to file
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

  void load_users_from_file() {
      FILE *file = fopen(USER_FILE, "r");
      if (file == NULL) {
          printf("No existing user file found, starting fresh.\n");
          return;
      }

      while (fscanf(file, "%49s %49s %d\n", users[user_count].username, users[user_count].password, &users[user_count].status) != EOF) {
          user_count++;
      }

      fclose(file);
  }

  void save_user_to_file(const User *user) {
      FILE *file = fopen(USER_FILE, "a");
      if (file == NULL) {
          perror("Failed to open user file");
          return;
      }

      fprintf(file, "%s %s %d\n", user->username, user->password, user->status);
      fclose(file);
  }
