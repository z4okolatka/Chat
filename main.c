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
#include "debug.h"

#define ctrl(x) ((x) & 0x1f)

typedef enum
{
    STATE_LOGIN,
    STATE_MAIN
} State;

LoginWindow loginWindow;
ChatWindow chatWindow;
ChatClient client;
State state = STATE_LOGIN;

void initWindows();
void exitProgram();
void draw();
void handleResize(int sig);
void rcvMsg(Message msg);

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
        if (ch == '\n' && loginWindow.input_num == 2)
        {
            state = STATE_MAIN;
        }
        else
        {
            handleChLoginWindow(&loginWindow, ch);
        }
        draw();
    }
    // ///////////////// TEMP /////////////////
    // state = STATE_MAIN;
    // strcpy(loginWindow.server_ip, "10.192.211.75");
    // strcpy(loginWindow.server_port, "5700");
    // strcpy(loginWindow.username, "z4okolatka");
    // \\\\\\\\\\\\\\\\\ TEMP \\\\\\\\\\\\\\\\\

    ChatClient client = initChatClient(loginWindow.server_ip, atoi(loginWindow.server_port), loginWindow.username);
    connectToChat(&client, loginWindow.username, rcvMsg);
    char msg[MESSAGE_BUFFER];
    draw();

    while (state == STATE_MAIN)
    {
        ch = getch();
        if (ch == ctrl('e'))
        {
            disconnect(&client);
            exitProgram();
        }
        else if (ch == '\n' || ch == KEY_ENTER)
        {
            sendMessage(&client, createMessage(chatWindow.message, chatWindow.username));
            clearMessageBox(&chatWindow);
        }
        else
        {
            handleChChatWindow(&chatWindow, ch);
        }
        draw();
    }

    exitProgram();
}

void initWindows()
{
    int console_height, console_width;
    getmaxyx(stdscr, console_height, console_width);
    loginWindow = createLoginWindow(console_width, console_height);
    chatWindow = createChatWindow(console_width, console_height);
}

void draw()
{
    clear();
    refresh();
    if (state == STATE_LOGIN)
        drawLoginWindow(&loginWindow);
    else if (state == STATE_MAIN)
        drawChatWindow(&chatWindow);
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
        resizeLoginWindow(&loginWindow, console_width, console_height);
    }
    else if (state == STATE_MAIN)
    {
        resizeChatWindow(&chatWindow, console_width, console_height);
    }

    draw();
}

void rcvMsg(Message msg)
{
    receiveMessage(&chatWindow, msg);
    draw();
}

void exitProgram()
{
    debugLog(VERBOSE, "Exiting program");
    if (loginWindow.win)
        delwin(loginWindow.win);
    endwin();
    // closeDebugLog();
    exit(0);
}