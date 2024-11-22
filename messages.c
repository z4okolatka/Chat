#include "messages.h"
#include <string.h>

Message createMessage(char content[MESSAGE_BUFFER], char from[USERNAME_BUFFER]) {
    Message message;
    strcpy(message.content, content);
    strcpy(message.from, from);
    return message;
}