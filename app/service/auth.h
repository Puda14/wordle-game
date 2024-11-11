#ifndef __AUTH__
#define __AUTH__

#include <stdint.h>
#include "user.h"  // Đảm bảo rằng cấu trúc User đã được định nghĩa
#define MAX_USERS 100

int register_user(const char *username, const char *password);
int login_user(const char *username, const char *password);
void logout_user(const char *username);

#endif
