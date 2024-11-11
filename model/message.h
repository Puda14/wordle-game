#ifndef __MESSAGE__
#define __MESSAGE__

#include <stdint.h>
#define BUFFER_SIZE 1024

  typedef struct {
    uint8_t message_type;
    char payload[BUFFER_SIZE];
  } Message;

#endif
