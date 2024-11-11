#ifndef __USER__
#define __USER__

#define BUFFER_SIZE 1024

typedef struct {
  int id;
  char username[50];
  char password[50];
  int score;
  int status;
} User;

/* Functions */

char* getUsername(User* u);

#endif
