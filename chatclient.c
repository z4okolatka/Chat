#include <ncurses.h>
#include <stdio.h>
#include "chatclient.h"
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <arpa/inet.h>
#include "defines.h"

ChatClient initChatClient(char server_ip[SERVER_IP_BUFFER], int server_port)
{
    ChatClient client;
    client.server_socket = socket(AF_INET, SOCK_STREAM, 0);
    client.server_address.sin_family = AF_INET;
    client.server_address.sin_port = htons(server_port);
    client.server_address.sin_addr.s_addr = inet_addr(server_ip);

    return client;
}

void sendMessage(ChatClient *client, Message message)
{
    send(client->server_socket, message.content, strlen(message.content), 0);
}

void *receiveMessages(void *arg)
{
    JoinChatData data = *(JoinChatData *)arg;
    ChatClient *client = data.client;
    MessageReceivedCallback receiveMessages = data.receiveMessageCallback;
    char buffer[BUFFER_SIZE];

    while (1)
    {
        int bytes_received = recv(client->server_socket, buffer, sizeof(buffer) - 1, 0);
        if (bytes_received > 0)
        {
            buffer[bytes_received] = '\0';
            // receiveMessage(buffer);
        }
        else if (bytes_received == 0)
        {
            printf("Server closed the connection\n");
            break;
        }
        else
        {
            printf("Error receiving message");
            break;
        }
    }

    return NULL;
}

void connectToChat(ChatClient *client, char username[USERNAME_BUFFER], MessageReceivedCallback receiveMessage)
{
    if (connect(client->server_socket, (struct sockaddr *)&client->server_address, sizeof(client->server_address)) < 0) {
        exit(1);
    }
    printw("sending username: %s\n", username);
    refresh();
    send(client->server_socket, username, USERNAME_BUFFER, 0);
    pthread_t receiver_thread;
    JoinChatData data = {
        .client = client,
        .receiveMessageCallback = receiveMessage
    };
    pthread_create(&receiver_thread, NULL, receiveMessages, (void *)&data);
}