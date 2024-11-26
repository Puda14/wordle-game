#ifndef __MESSAGE__
#define __MESSAGE__

#include <stdint.h>
#define BUFFER_SIZE 1024

enum MessageType { SIGNUP_REQUEST = 0, LOGIN_REQUEST = 1, LOGOUT_REQUEST = 2 };

enum StatusCode {
  // 2xx: Successful responses
  SUCCESS = 200,                   // OK: The request was successful
  CREATED = 201,                   // Created: A new resource was successfully created
  ACCEPTED = 202,                  // Accepted: The request has been accepted for processing, but not completed yet

  // 4xx: Client-side errors
  BAD_REQUEST = 400,               // Bad Request: The server could not understand the request due to invalid syntax or data
  UNAUTHORIZED = 401,              // Unauthorized: The client needs to authenticate or the provided credentials are invalid
  FORBIDDEN = 403,                 // Forbidden: The client does not have permission to access the requested resource
  NOT_FOUND = 404,                 // Not Found: The requested resource could not be found (e.g., invalid username or endpoint)

  // 5xx: Server-side errors
  INTERNAL_SERVER_ERROR = 500,     // Internal Server Error: The server encountered an unexpected condition
  NOT_IMPLEMENTED = 501,           // Not Implemented: The server does not support the functionality required to fulfill the request
  SERVICE_UNAVAILABLE = 503       // Service Unavailable: The server is temporarily unable to handle the request (due to overload or maintenance)
};

typedef struct {
  enum MessageType message_type;
  enum StatusCode status;
  char payload[BUFFER_SIZE];
} Message;

#endif