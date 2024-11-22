#include <string.h>
#include "elements.h"
#include "utils.h"
#include "messages.h"
#include "defines.h"

LoginWindow createLoginWindow(int console_width, int console_height)
{
    LoginWindow window = {
        .username = "",
        .inputPos = 0,
        .input_num = 0};
    const int width = min(LOGIN_WIDTH, console_width);
    const int height = min(LOGIN_HEIGHT, console_height);
    const int starty = (console_height - height) / 2;
    const int startx = (console_width - width) / 2;
    window.win = newwin(height, width, starty, startx);
    return window;
}

void resizeLoginWindow(LoginWindow *win, int console_width, int console_height)
{
    const int width = min(LOGIN_WIDTH, console_width);
    const int height = min(LOGIN_HEIGHT, console_height);
    const int starty = (console_height - height) / 2;
    const int startx = (console_width - width) / 2;
    delwin(win->win);
    win->win = newwin(height, width, starty, startx);
}

void drawLoginWindow(LoginWindow *win)
{
    wclear(win->win);
    mvwprintw(win->win, 0, 0, "Username");
    mvwprintw(win->win, 1, 0, "Server IP");
    mvwprintw(win->win, 2, 0, "Server PORT");
    mvwprintw(win->win, 0, LOGIN_PADDING, win->username);
    mvwprintw(win->win, 1, LOGIN_PADDING, win->server_ip);
    mvwprintw(win->win, 2, LOGIN_PADDING, win->server_port);
    wmove(win->win, win->input_num, win->inputPos + LOGIN_PADDING);
    wrefresh(win->win);
}

void handleChLoginWindow(LoginWindow *win, int ch)
{
    if ((ch == '\n' || ch == KEY_DOWN) && win->input_num < 2)
    {
        win->input_num++;
        win->inputPos = min(win->inputPos, win->input_num == 1 ? strlen(win->server_ip) : strlen(win->server_port));
    }
    else if (ch == KEY_LEFT && win->inputPos > 0)
        win->inputPos--;
    else if (ch == KEY_RIGHT)
    {
        if (win->input_num == 0 && win->inputPos < strlen(win->username))
            win->inputPos++;
        else if (win->input_num == 1 && win->inputPos < strlen(win->server_ip))
            win->inputPos++;
        else if (win->input_num == 2 && win->inputPos < strlen(win->server_port))
            win->inputPos++;
    }
    else if (ch == KEY_UP && win->input_num > 0)
    {
        win->input_num--;
        win->inputPos = min(win->inputPos, win->input_num == 1 ? strlen(win->server_ip) : strlen(win->username));
    }
    else if (win->input_num == 0)
    {
        if ((ch == KEY_BACKSPACE || ch == 127) && win->inputPos > 0)
        {
            win->inputPos--;
            for (int i = win->inputPos + 1; i < USERNAME_BUFFER; i++)
                win->username[i - 1] = win->username[i];
        }
        else if (win->inputPos < USERNAME_BUFFER - 1 && ch >= 32 && ch <= 126 && strlen(win->username) < USERNAME_BUFFER - 1)
        {
            for (int i = USERNAME_BUFFER - 2; i > win->inputPos; i--)
                win->username[i] = win->username[i - 1];
            win->username[win->inputPos++] = ch;
        }
    }
    else if (win->input_num == 1)
    {
        if ((ch == KEY_BACKSPACE || ch == 127) && win->inputPos > 0)
        {
            win->inputPos--;
            for (int i = win->inputPos + 1; i < SERVER_IP_BUFFER; i++)
                win->server_ip[i - 1] = win->server_ip[i];
        }
        else if (win->inputPos < SERVER_IP_BUFFER - 1 && ('0' <= ch && ch <= '9' || ch == '.') && strlen(win->server_ip) < SERVER_IP_BUFFER - 1)
        {
            for (int i = SERVER_IP_BUFFER - 2; i > win->inputPos; i--)
                win->server_ip[i] = win->server_ip[i - 1];
            win->server_ip[win->inputPos++] = ch;
        }
    }
    else if (win->input_num == 2)
    {
        if ((ch == KEY_BACKSPACE || ch == 127) && win->inputPos > 0)
        {
            win->inputPos--;
            for (int i = win->inputPos + 1; i < SERVER_PORT_BUFFER; i++)
                win->server_port[i - 1] = win->server_port[i];
        }
        else if (win->inputPos < SERVER_PORT_BUFFER - 1 && '0' <= ch && ch <= '9' && strlen(win->server_port) < SERVER_PORT_BUFFER - 1)
        {
            for (int i = SERVER_PORT_BUFFER - 2; i > win->inputPos; i--)
                win->server_port[i] = win->server_port[i - 1];
            win->server_port[win->inputPos++] = ch;
        }
    }
}

ChatWindow createChatWindow(int console_width, int console_height)
{
    ChatWindow window = {
        .inputPos = 0,
        .messageCount = 0,
        .message = ""};
    const int width = min(CHAT_MESSAGE_WIDTH + 4, console_width);
    const int height = console_height;
    const int startx = (console_width - width) / 2;
    window.win = newwin(height, width, 0, startx);
    window.messagesWindow = derwin(window.win, height - CHAT_MESSAGE_HEIGHT, width, 0, 0);
    window.messageWindow = derwin(window.win, CHAT_MESSAGE_HEIGHT, width, height - CHAT_MESSAGE_HEIGHT, 0);
    return window;
}

int console_width1;
void resizeChatWindow(ChatWindow *win, int console_width, int console_height)
{
    console_width1 = console_width;
    const int width = min(CHAT_MESSAGE_WIDTH + 4, console_width);
    const int height = console_height;
    const int startx = (console_width - width) / 2;
    delwin(win->win);
    delwin(win->messagesWindow);
    delwin(win->messageWindow);
    win->win = newwin(height, width, 0, startx);
    win->messagesWindow = derwin(win->win, height - CHAT_MESSAGE_HEIGHT, width, 0, 0);
    win->messageWindow = derwin(win->win, CHAT_MESSAGE_HEIGHT, width, height - CHAT_MESSAGE_HEIGHT, 0);
}
int count = 0;
void drawChatWindow(ChatWindow *win)
{
    wclear(win->win);
    box(win->messageWindow, 0, 0);
    mvwprintw(win->messageWindow, 0, 2, " Message ");
    int rows = strlen(win->message) / CHAT_MESSAGE_WIDTH;
    int cursorRow = win->inputPos / CHAT_MESSAGE_WIDTH;
    int cursorCol = win->inputPos % CHAT_MESSAGE_WIDTH;
    for (int i = 0; i <= rows; i++)
    {
        for (int j = 0; j < CHAT_MESSAGE_WIDTH; j++)
            if (i * CHAT_MESSAGE_WIDTH + j >= strlen(win->message))
                break;
            else
                mvwaddch(win->messageWindow, 1 + i, 2 + j, win->message[i * CHAT_MESSAGE_WIDTH + j]);
    }
    int w, h;
    getmaxyx(win->messagesWindow, h, w);
    for (int bottom = h, i = win->messageCount; i > -1 && bottom > 0; i--)
    {
        Message msg = win->messages[i];
        int msgLength = strlen(msg.content);
        int msgWidth = w - CHAT_MESSAGES_PADDING;
        int msgHeight = msgLength / msgWidth + 1;
        if (strcmp(msg.from, win->username) == 0) // If it's my message
        {
            int usernameLen = strlen(msg.from);
            mvwprintw(win->messagesWindow, bottom - msgHeight - 2, w - usernameLen, msg.from);
            for (int i = 0; i < msgHeight; i++)
                for (int j = 0; j < msgWidth; j++)
                    if (i * msgWidth + j >= msgLength)
                        break;
                    else
                        mvwaddch(win->messagesWindow, bottom - msgHeight - 1 + i, w - min(msgLength, msgWidth) + j, msg.content[i * msgWidth + j]);
        }
        else
        {
            mvwprintw(win->messagesWindow, bottom - msgHeight - 2, 0, msg.from);
            for (int i = 0; i < msgHeight; i++)
                for (int j = 0; j < msgWidth; j++)
                    if (i * msgWidth + j >= msgLength)
                        break;
                    else
                        mvwaddch(win->messagesWindow, bottom - msgHeight - 1 + i, j, msg.content[i * msgWidth + j]);
        }
        bottom -= (msgHeight + 2);
    }
    wmove(win->messageWindow, 1 + cursorRow, 2 + cursorCol);
    wrefresh(win->messagesWindow);
    wrefresh(win->messageWindow);
}

void handleChChatWindow(ChatWindow *win, int ch)
{
    const int rows = strlen(win->message) / CHAT_MESSAGE_WIDTH;
    const int cursorRow = win->inputPos / CHAT_MESSAGE_WIDTH;
    const int cursorCol = win->inputPos % CHAT_MESSAGE_WIDTH;
    if (ch == KEY_DOWN)
    {
        if (cursorRow < rows)
        {
            win->inputPos += CHAT_MESSAGE_WIDTH;
            win->inputPos = min(win->inputPos, strlen(win->message));
        }
    }
    else if (ch == KEY_UP)
    {
        if (cursorRow > 0)
            win->inputPos -= CHAT_MESSAGE_WIDTH;
    }
    else if (ch == KEY_LEFT)
    {
        if (win->inputPos > 0)
            win->inputPos--;
    }
    else if (ch == KEY_RIGHT)
    {
        if (win->inputPos < strlen(win->message))
            win->inputPos++;
    }
    else if ((ch == KEY_BACKSPACE || ch == 127) && win->inputPos > 0)
    {
        win->inputPos--;
        for (int i = win->inputPos + 1; i < MESSAGE_BUFFER; i++)
            win->message[i - 1] = win->message[i];
    }
    else if (win->inputPos < MESSAGE_BUFFER - 1 && ch >= 32 && ch < 127 && strlen(win->message) < MESSAGE_BUFFER - 1)
    {
        for (int i = MESSAGE_BUFFER - 2; i > win->inputPos; i--)
            win->message[i] = win->message[i - 1];
        win->message[win->inputPos++] = ch;
    }
}

void clearMessageBox(ChatWindow *win)
{
    strcpy(win->message, (char[MESSAGE_BUFFER]){'\0'});
    win->inputPos = 0;
}

void receiveMessage(ChatWindow *win, Message msg)
{
    win->messages[win->messageCount++] = msg;
}