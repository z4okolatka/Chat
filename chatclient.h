#ifndef CHAT_H
#define CHAT_H

#define MAX_WAITING_CONNECTIONS 20
#define BUFFER_SIZE 1024

#include "messages.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

typedef void (*MessageReceivedCallback)(Message msg);

typedef struct
{
    int server_socket;
    struct sockaddr_in server_address;
    Message messages[MAX_MESSAGES_COUNT];
} ChatClient;

typedef struct {
    ChatClient *client;
    MessageReceivedCallback receiveMessageCallback;
} JoinChatData;

ChatClient initChatClient(char server_ip[SERVER_IP_BUFFER], int server_port);
void sendMessage(ChatClient *client, Message message);
void connectToChat(ChatClient *client, char username[USERNAME_BUFFER], MessageReceivedCallback receiveMessage);

#endif