#pragma once
#include "defines.h"

typedef struct
{
    char content[MESSAGE_BUFFER];
    char from[USERNAME_BUFFER];
} Message;

Message createMessage(char content[MESSAGE_BUFFER], char username[USERNAME_BUFFER]);