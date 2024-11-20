#include <ncurses.h>
#include <stdio.h>
#include "chatclient.h"
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <errno.h>
#include "defines.h"
#include "debug.h"

ChatClient initChatClient(char server_ip[SERVER_IP_BUFFER], int server_port, char username[USERNAME_BUFFER])
{
    ChatClient client;
    client.server_socket = socket(AF_INET, SOCK_STREAM, 0);
    client.server_address.sin_family = AF_INET;
    client.server_address.sin_port = htons(server_port);
    client.server_address.sin_addr.s_addr = inet_addr(server_ip);
    strcpy(client.username, username);

    return client;
}

void sendMessage(ChatClient *client, Message message)
{
    if (send(client->server_socket, message.content, strlen(message.content), 0) >= 0)
        debugLog(INFO, "Sent message \"%s\" to server", message.content);
    else
    {
        debugLog(ERROR, "Couldn't send message \"%s\" to server", message.content);
        exit(1);
    }
}

void *receiveMessages(void *arg)
{
    JoinChatData *data = (JoinChatData *)arg;
    ChatClient client = data->client;
    MessageReceivedCallback receiveMessage = data->receiveMessageCallback;
    free(data);
    char buffer[BUFFER_SIZE];
    ssize_t bytesReceived = 0;

    debugLog(VERBOSE, "Listening to messages");
    while ((bytesReceived = recv(client.server_socket, buffer, sizeof(buffer) - 1, 0)) > 0)
    {
        debugLog(VERBOSE, "Received \"%s\", size %d", buffer, bytesReceived);
        buffer[bytesReceived] = '\0';

        Message msg;
        strcpy(msg.from, client.username);
        strcpy(msg.content, buffer);
        msg.content[sizeof(msg.content) - 1] = '\0';
        if (receiveMessage != NULL)
            receiveMessage(msg);
        else
            debugLog(ERROR, "ReceiveMessage is NULL");
    }
    if (bytesReceived == 0)
    {
        debugLog(ERROR, "Server closed the connection\n");
        exit(1);
    }
    if (bytesReceived < 0)
    {
        debugLog(ERROR, "Error receiving message: %s (%d)\n", strerror(errno), errno);
        exit(1);
    }

    pthread_exit(NULL);
}

void connectToChat(ChatClient *client, char username[USERNAME_BUFFER], MessageReceivedCallback receiveMessage)
{
    if (connect(client->server_socket, (struct sockaddr *)&client->server_address, sizeof(client->server_address)) < 0)
    {
        debugLog(ERROR, "Couldn't connect to server");
        exit(1);
    }
    debugLog(INFO, "Connected to server");
    send(client->server_socket, username, USERNAME_BUFFER, 0);
    debugLog(VERBOSE, "Sent \"%s\" to server", username);

    pthread_t receiver_thread;
    JoinChatData *data = malloc(sizeof(JoinChatData));
    if (data == NULL)
    {
        debugLog(ERROR, "Failed to allocate memory for JoinChatData");
        exit(1);
    }
    data->client = *client;
    data->receiveMessageCallback = receiveMessage;
    if (pthread_create(&receiver_thread, NULL, receiveMessages, (void *)data) != 0)
    {
        debugLog(ERROR, "Couldn't create client thread");
        free(data);
        exit(1);
    }
    debugLog(VERBOSE, "Created client thread");
}

void disconnect(ChatClient *client)
{
    if (close(client->server_socket) < 0)
    {
        debugLog(ERROR, "Couldn't close connection with server");
        exit(1);
    }
    debugLog(VERBOSE, "Closed connection with server");
}