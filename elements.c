#include <string.h>
#include "elements.h"
#include "utils.h"
#include "messages.h"

LoginWindow createLoginWindow(int console_width, int console_height)
{
    LoginWindow window = {
        .username = "",
        .input_pos = 0,
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
    wmove(win->win, win->input_num, win->input_pos + LOGIN_PADDING);
    wrefresh(win->win);
}

void handleCh(LoginWindow *win, int ch)
{
    if ((ch == '\n' || ch == KEY_DOWN) && win->input_num < 2)
    {
        win->input_num++;
        win->input_pos = min(win->input_pos, win->input_num == 1 ? strlen(win->server_ip) : strlen(win->server_port));
    }
    else if (ch == KEY_LEFT && win->input_pos > 0)
        win->input_pos--;
    else if (ch == KEY_RIGHT)
    {
        if (win->input_num == 0 && win->input_pos < strlen(win->username))
            win->input_pos++;
        else if (win->input_num == 1 && win->input_pos < strlen(win->server_ip))
            win->input_pos++;
        else if (win->input_num == 2 && win->input_pos < strlen(win->server_port))
            win->input_pos++;
    }
    else if (ch == KEY_UP && win->input_num > 0)
    {
        win->input_num--;
        win->input_pos = min(win->input_pos, win->input_num == 1 ? strlen(win->server_ip) : strlen(win->username));
    }
    else if (win->input_num == 0)
    {
        if ((ch == KEY_BACKSPACE || ch == 127) && win->input_pos > 0)
        {
            win->input_pos--;
            for (int i = win->input_pos + 1; i < USERNAME_BUFFER; i++)
                win->username[i - 1] = win->username[i];
        }
        else if (win->input_pos < USERNAME_BUFFER - 1 && ch >= 32 && ch <= 126 && strlen(win->username) < USERNAME_BUFFER - 1)
        {
            for (int i = USERNAME_BUFFER - 2; i > win->input_pos; i--)
                win->username[i] = win->username[i - 1];
            win->username[win->input_pos++] = ch;
        }
    }
    else if (win->input_num == 1)
    {
        if ((ch == KEY_BACKSPACE || ch == 127) && win->input_pos > 0)
        {
            win->input_pos--;
            for (int i = win->input_pos + 1; i < SERVER_IP_BUFFER; i++)
                win->server_ip[i - 1] = win->server_ip[i];
        }
        else if (win->input_pos < SERVER_IP_BUFFER - 1 && ('0' <= ch && ch <= '9' || ch == '.') && strlen(win->server_ip) < SERVER_IP_BUFFER - 1)
        {
            for (int i = SERVER_IP_BUFFER - 2; i > win->input_pos; i--)
                win->server_ip[i] = win->server_ip[i - 1];
            win->server_ip[win->input_pos++] = ch;
        }
    }
    else if (win->input_num == 2)
    {
        if ((ch == KEY_BACKSPACE || ch == 127) && win->input_pos > 0)
        {
            win->input_pos--;
            for (int i = win->input_pos + 1; i < SERVER_PORT_BUFFER; i++)
                win->server_port[i - 1] = win->server_port[i];
        }
        else if (win->input_pos < SERVER_PORT_BUFFER - 1 && '0' <= ch && ch <= '9' && strlen(win->server_port) < SERVER_PORT_BUFFER - 1)
        {
            for (int i = SERVER_PORT_BUFFER - 2; i > win->input_pos; i--)
                win->server_port[i] = win->server_port[i - 1];
            win->server_port[win->input_pos++] = ch;
        }
    }
}