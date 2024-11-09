#ifndef MESSAGES_H
#define MESSAGES_H

#include "defines.h"
#include <time.h>

typedef struct
{
    char content[MESSAGE_BUFFER];
    struct tm *time;
    char username[USERNAME_BUFFER];
} Message;

Message createMessage(char content[MESSAGE_BUFFER], char username[USERNAME_BUFFER]);

#endif