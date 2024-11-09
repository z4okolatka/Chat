#include "messages.h"
#include <string.h>

Message createMessage(char content[MESSAGE_BUFFER], char username[USERNAME_BUFFER]) {
    Message message;
    strcpy(message.content, content);
    strcpy(message.username, username);
    time_t t = time(NULL);
    message.time = localtime(&t);
    return message;
}

void freeMessage(Message *msg)
{
}