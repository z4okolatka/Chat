#ifndef ELEMENTS_H
#define ELEMENTS_H

#include <ncurses.h>
#include "defines.h"
#include "messages.h"

typedef struct
{
    WINDOW *win;
    int inputPos;
    int input_num;
    char username[USERNAME_BUFFER];
    char server_ip[SERVER_IP_BUFFER];
    char server_port[SERVER_PORT_BUFFER];

} LoginWindow;

typedef struct
{
    WINDOW *win;
    WINDOW *messagesWindow;
    WINDOW *messageWindow;
    int inputPos;
    char username[USERNAME_BUFFER];
    char message[MESSAGE_BUFFER];
    Message messages[MAX_MESSAGES_COUNT];
    size_t messageCount;
} ChatWindow;

LoginWindow createLoginWindow(int console_width, int console_height);
void resizeLoginWindow(LoginWindow *win, int console_width, int console_height);
void drawLoginWindow(LoginWindow *win);
void handleChLoginWindow(LoginWindow *win, int ch);

ChatWindow createChatWindow(int console_width, int console_height);
void resizeChatWindow(ChatWindow *win, int console_width, int console_height);
void drawChatWindow(ChatWindow *win);
void handleChChatWindow(ChatWindow *win, int ch);
void clearMessageBox(ChatWindow *win);
void receiveMessage(ChatWindow *win, Message msg);

#endif