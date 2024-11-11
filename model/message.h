#ifndef __MESSAGE__
#define __MESSAGE__

#include <stdint.h>
#define BUFFER_SIZE 1024

enum MessageType { SIGNUP_REQUEST = 0, LOGIN_REQUEST = 1, LOGOUT_REQUEST = 2 };

typedef struct {
  uint8_t message_type;
  uint8_t status;
  char payload[BUFFER_SIZE];
} Message;

#endif
