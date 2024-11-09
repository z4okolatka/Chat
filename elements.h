#ifndef ELEMENTS_H
#define ELEMENTS_H

#define LOGIN_WIDTH 30
#define LOGIN_HEIGHT 3
#define LOGIN_PADDING 13

#include <ncurses.h>
#include "defines.h"

typedef struct
{
    WINDOW *win;
    int input_pos;
    int input_num;
    char username[USERNAME_BUFFER];
    char server_ip[SERVER_IP_BUFFER];
    char server_port[SERVER_PORT_BUFFER];

} LoginWindow;

LoginWindow createLoginWindow(int console_width, int console_height);
void resizeLoginWindow(LoginWindow *win, int console_width, int console_height);
void drawLoginWindow(LoginWindow *win);
void handleCh(LoginWindow *win, int ch);

#endif