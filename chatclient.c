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
    // Serizalizing message
    uint32_t fromLength = strlen(message.from);
    uint32_t contentLength = strlen(message.content);
    if (fromLength >= USERNAME_BUFFER)
        fromLength = USERNAME_BUFFER - 1;
    if (contentLength >= MESSAGE_BUFFER)
        contentLength = MESSAGE_BUFFER - 1;
    size_t totalSize = sizeof(uint32_t) * 2 + fromLength + contentLength;
    char *buffer = malloc(totalSize);
    if (buffer == NULL)
    {
        debugLog(ERROR, "Failed to allocate memory for message buffer");
        exit(1);
    }
    uint32_t netFromLength = htonl(fromLength);
    uint32_t netContentLength = htonl(contentLength);
    size_t offset = 0;
    memcpy(buffer + offset, &netFromLength, sizeof(netFromLength));
    offset += sizeof(netFromLength);
    memcpy(buffer + offset, &netContentLength, sizeof(netContentLength));
    offset += sizeof(netContentLength);
    memcpy(buffer + offset, message.from, fromLength);
    offset += fromLength;
    memcpy(buffer + offset, message.content, contentLength);
    offset += contentLength;

    ssize_t totalSent = 0;
    ssize_t n;
    while (totalSent < totalSize)
    {
        n = send(client->server_socket, buffer + totalSent, totalSize - totalSent, 0);
        if (n < 0)
        {
            debugLog(ERROR, "Failed to send message (sent %zu of %zu bytes)", totalSent, totalSize);
            free(buffer);
            exit(1);
        }
        totalSent += n;
    }
    debugLog(INFO, "Sent message \"%s\" to server", message.content);
    free(buffer);
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
    while (1)
    {
        uint32_t netFromLength = 0;
        uint32_t netContentLength = 0;
        ssize_t n;
        n = recv(client.server_socket, &netFromLength, sizeof(netFromLength), MSG_WAITALL);
        if (n <= 0)
        {
            if (n == 0)
                debugLog(INFO, "Server closed the connection");
            else
                debugLog(ERROR, "Error receiving 'fromLength': %s (%d)", strerror(errno), errno);
            break;
        }
        n = recv(client.server_socket, &netContentLength, sizeof(netContentLength), MSG_WAITALL);
        if (n <= 0)
        {
            if (n == 0)
                debugLog(INFO, "Server closed the connection");
            else
                debugLog(ERROR, "Error receiving 'contentLength': %s (%d)", strerror(errno), errno);
            break;
        }
        uint32_t fromLength = ntohl(netFromLength);
        uint32_t contentLength = ntohl(netContentLength);
        if (fromLength >= USERNAME_BUFFER || contentLength >= MESSAGE_BUFFER)
        {
            debugLog(ERROR, "Received invalid lengths: from_length=%u, content_length=%u", fromLength, contentLength);
            break;
        }
        char from[USERNAME_BUFFER];
        char content[MESSAGE_BUFFER];
        n = recv(client.server_socket, from, fromLength, MSG_WAITALL);
        if (n <= 0)
        {
            if (n == 0)
                debugLog(INFO, "Server closed the connection");
            else
                debugLog(ERROR, "Error receiving 'from': %s (%d)", strerror(errno), errno);
            break;
        }
        from[fromLength] = '\0';
        n = recv(client.server_socket, content, contentLength, MSG_WAITALL);
        if (n <= 0)
        {
            if (n == 0)
                debugLog(INFO, "Server closed the connection");
            else
                debugLog(ERROR, "Error receiving 'content': %s (%d)", strerror(errno), errno);
            break;
        }
        content[contentLength] = '\0';
        
        // Deserializing message
        Message msg;
        strcpy(msg.from, from);
        strcpy(msg.content, content);
        debugLog(INFO, "Message from \"%s\": %s", msg.from, msg.content);
        if (receiveMessage != NULL)
            receiveMessage(msg);
        else
            debugLog(ERROR, "ReceiveMessage is NULL");
    }

    pthread_exit(NULL);
}

void connectToChat(ChatClient *client, char username[USERNAME_BUFFER], MessageReceivedCallback receiveMessage)
{
    if (connect(client->server_socket, (struct sockaddr *)&client->server_address, sizeof(client->server_address)) < 0)
    {
        debugLog(ERROR, "Falied to connect to server");
        exit(1);
    }
    debugLog(INFO, "Connected to server");
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
        debugLog(ERROR, "Falied to create client thread");
        free(data);
        exit(1);
    }
    debugLog(VERBOSE, "Created client thread");

    // Sending username
    Message msg = {.content = "[Joined the chat]"};
    strcpy(msg.from, username);
    sendMessage(client, msg);
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