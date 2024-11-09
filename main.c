#include <ncurses.h>
#include <signal.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include "utils.h"
#include "defines.h"
#include "elements.h"
#include "chatclient.h"
#include "messages.h"

typedef enum
{
    STATE_LOGIN,
    STATE_MAIN
} State;

char username[USERNAME_BUFFER];
LoginWindow login_win;
ChatClient client;
State state = STATE_LOGIN;

void initWindows();
void exitProgram();
void draw();
void handleResize(int sig);
void receiveMessage(Message msg);

int main()
{
    signal(SIGWINCH, handleResize);
    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    initWindows();
    draw();

    int ch;
    while (state == STATE_LOGIN)
    {
        ch = getch();
        if (ch == '\n' && login_win.input_num == 2)
        {
            strcpy(username, login_win.username);
            state = STATE_MAIN;
        }
        else
        {
            handleCh(&login_win, ch);
        }
        draw();
    }
    ChatClient client = initChatClient(login_win.server_ip, atoi(login_win.server_port));
    connectToChat(&client, username, receiveMessage);
    echo();
    char msg[MESSAGE_BUFFER] = "\0";
    while (state == STATE_MAIN)
    {
        ch = getch();
        if (ch == '\n')
            sendMessage(&client, createMessage(msg, username));
        else
        {
            msg[strlen(msg) + 1] = '\0';
            msg[strlen(msg)] = ch;
        }
    }

    exitProgram();
    return 0;
}

void receiveMessage(Message msg)
{
}

void initWindows()
{
    int console_height, console_width;
    getmaxyx(stdscr, console_height, console_width);
    login_win = createLoginWindow(console_width, console_height);
}

void draw()
{
    if (state == STATE_LOGIN)
    {
        drawLoginWindow(&login_win);
    }
}

void handleResize(int sig)
{
    endwin();
    refresh();
    clear();

    int console_height, console_width;
    getmaxyx(stdscr, console_height, console_width);

    if (state == STATE_LOGIN)
    {
        resizeLoginWindow(&login_win, console_width, console_height);
    }
    else if (state == STATE_MAIN)
    {
    }

    draw();
}

void exitProgram()
{
    if (login_win.win)
        delwin(login_win.win);
    endwin();
}