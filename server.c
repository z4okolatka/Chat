#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include "debug.h"
#include "defines.h"
#include "messages.h"
#include <errno.h>

#define MAX_CLIENTS 50
#define MAX_WAITING_CONNECTIONS 50
#define BUFFER_SIZE 1024

typedef struct
{
    char username[USERNAME_BUFFER];
    pthread_t thread;
    int socket;
    struct sockaddr_in address;
} Client;

Client clients[MAX_CLIENTS];
int clients_count = 0;
pthread_mutex_t clients_mutex;
void *handleClient(void *arg);
void broadcastMessage(Message message);
void removeClient(int clientSocket);
void initServer();
void destroyServer();
char *getLocalIPAddress();
void sendMessage(Client *client, Message msg);
int getPort();

int main()
{
    initServer();
    int server_socket;
    struct sockaddr_in server_address;
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(getPort());
    server_address.sin_addr.s_addr = INADDR_ANY;

    if (bind(server_socket, (struct sockaddr *)&server_address, sizeof(server_address)) < 0)
    {
        debug(ERROR, "Couldn't bind socket to address");
        close(server_socket);
        exit(1);
    }
    debug(VERBOSE, "Bind socket to address");

    if (listen(server_socket, MAX_WAITING_CONNECTIONS) < 0)
    {
        debug(ERROR, "Couldn't start listening\n");
        close(server_socket);
        exit(1);
    }
    debug(VERBOSE, "Started listening");

    debug(INFO, "Listening on %s:%d", getLocalIPAddress(), ntohs(server_address.sin_port));
    while (1)
    {
        int client_socket;
        struct sockaddr_in client_address;
        socklen_t client_len = sizeof(client_address);

        client_socket = accept(server_socket, (struct sockaddr *)&client_address, &client_len);
        debug(INFO, "Accepted new client");
        if (client_socket < 0)
        {
            debug(WARNING, "Coudln't receive client's socket, continuing");
            continue;
        }

        if (pthread_mutex_lock(&clients_mutex) != 0)
        {
            debug(ERROR, "Failed to lock mutex");
            exit(1);
        }
        debug(VERBOSE, "Locked mutex");
        if (clients_count < MAX_CLIENTS)
        {
            clients[clients_count].socket = client_socket;
            clients[clients_count].address = client_address;

            if (pthread_create(&clients[clients_count].thread, NULL, handleClient, (void *)&clients[clients_count]) != 0)
            {
                debug(ERROR, "Couldn't create thread for client");
                removeClient(clients[client_socket].socket);
                exit(1);
            }
            clients_count++;
            debug(VERBOSE, "Created new thread for client");
        }
        else
        {
            debug(WARNING, "Max clients reached. Rejecting new client");
            close(client_socket);
        }
        if (pthread_mutex_unlock(&clients_mutex) != 0)
        {
            debug(ERROR, "Failed to unlock mutex");
            exit(1);
        }
        debug(VERBOSE, "Unlocked mutex");
    }

    destroyServer();
    return 0;
}

void sendMessage(Client *client, Message message)
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
        debug(ERROR, "Failed to allocate memory for message buffer");
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
        n = send(client->socket, buffer + totalSent, totalSize - totalSent, 0);
        if (n <= 0)
        {
            if (n < 0)
                debug(ERROR, "Failed to broadcast (sent %zu of %zu bytes)", totalSent, totalSize);
            else
                debug(ERROR, "Client disconnected while sending message");
            free(buffer);
            close(client->socket);
            removeClient(client->socket);
            return;
        }
        totalSent += n;
        debug(VERBOSE, "Sent %d of %d", totalSent, totalSize);
    }
    debug(INFO, "Broadcasted to \"%s\"", client->username);
    free(buffer);
}

void broadcastMessage(Message message)
{
    if (pthread_mutex_lock(&clients_mutex) != 0)
    {
        debug(ERROR, "Failed to lock mutex");
        exit(1);
    }
    debug(VERBOSE, "Locked mutex");

    for (int i = 0; i < clients_count; i++)
        sendMessage(&clients[i], message);

    if (pthread_mutex_unlock(&clients_mutex) != 0)
    {
        debug(ERROR, "Failed to unlock mutex");
        exit(1);
    }
    debug(VERBOSE, "Unlocked mutex");
}

void removeClient(int clientSocket)
{
    if (pthread_mutex_lock(&clients_mutex) != 0)
    {
        debug(ERROR, "Failed to lock mutex");
        exit(1);
    }
    debug(VERBOSE, "Locked mutex");
    Client *client = NULL;
    size_t index = -1;
    for (int i = 0; i < clients_count; i++)
        if (clients[i].socket == clientSocket)
        {
            index = i;
            break;
        }
    if (index == -1)
        return;
    client = &clients[index];
    close(client->socket);
    if (pthread_cancel(client->thread) != 0)
    {
        debug(ERROR, "Failed to cancel client's thread");
        exit(1);
    }
    debug(VERBOSE, "Canceled client's thread");

    for (int j = index; j < clients_count; j++)
        clients[j] = clients[j + 1];
    clients_count--;
    if (pthread_mutex_unlock(&clients_mutex) != 0)
    {
        debug(ERROR, "Failed to unlock mutex");
        exit(1);
    }
    debug(VERBOSE, "Unlocked mutex");
}

void initServer()
{
    if (pthread_mutex_init(&clients_mutex, NULL) != 0)
    {
        debug(ERROR, "Couldn't start thread mutex\n");
        exit(1);
    }
    debug(VERBOSE, "Initialised mutex");
}

void destroyServer()
{
    for (int i = 0; i < clients_count; i++)
    {
        removeClient(clients[i].socket);
    }
    if (pthread_mutex_destroy(&clients_mutex) != 0)
    {
        debug(ERROR, "Coludn't destroy mutex");
        exit(1);
    }
    debug(VERBOSE, "Destroyed mutex");
}

void *handleClient(void *arg)
{
    Client *client = ((Client *)arg);

    while (1)
    {
        uint32_t netFromLength = 0;
        uint32_t netContentLength = 0;
        ssize_t n;
        n = recv(client->socket, &netFromLength, sizeof(netFromLength), MSG_WAITALL);
        if (n <= 0)
        {
            if (n == 0)
                debug(ERROR, "Client disconnected while receiving 'fromLength'");
            else
                debug(ERROR, "Error receiving 'fromLength': %s (%d)", strerror(errno), errno);
            break;
        }
        debug(VERBOSE, "Received 'netFromLength'");
        n = recv(client->socket, &netContentLength, sizeof(netContentLength), MSG_WAITALL);
        if (n <= 0)
        {
            if (n == 0)
                debug(ERROR, "Client disconnected while receiving 'contentLength'");
            else
                debug(ERROR, "Error receiving 'contentLength': %s (%d)", strerror(errno), errno);
            removeClient(client->socket);
            break;
        }
        debug(VERBOSE, "Received 'netContentLength'");
        uint32_t fromLength = ntohl(netFromLength);
        uint32_t contentLength = ntohl(netContentLength);
        if (fromLength >= USERNAME_BUFFER || contentLength >= MESSAGE_BUFFER)
        {
            debug(ERROR, "Received invalid lengths: from_length=%u, content_length=%u", fromLength, contentLength);
            break;
        }
        char from[USERNAME_BUFFER];
        char content[MESSAGE_BUFFER];
        n = recv(client->socket, from, fromLength, MSG_WAITALL);
        if (n <= 0)
        {
            if (n == 0)
                debug(ERROR, "Client disconnected while receiving 'from'");
            else
                debug(ERROR, "Error receiving 'from': %s (%d)", strerror(errno), errno);
            break;
        }
        debug(VERBOSE, "Received 'from'");
        from[fromLength] = '\0';
        n = recv(client->socket, content, contentLength, MSG_WAITALL);
        if (n <= 0)
        {
            if (n == 0)
                debug(ERROR, "Client disconnected while receiving 'content'");
            else
                debug(ERROR, "Error receiving 'content': %s (%d)", strerror(errno), errno);
            break;
        }
        debug(VERBOSE, "Received 'content'");
        content[contentLength] = '\0';

        // Deserializing message
        Message msg;
        strcpy(msg.from, from);
        strcpy(msg.content, content);
        if (strcmp(msg.content, "[Joined the chat]") == 0)
            strcpy(client->username, msg.from);
        debug(INFO, "\"%s\": %s", from, content);
        broadcastMessage(msg);
    }

    removeClient(client->socket);

    return NULL;
}

char *getLocalIPAddress()
{
    char hostbuffer[256];
    int hostname = gethostname(hostbuffer, sizeof(hostbuffer));
    if (hostname == -1)
    {
        debug(ERROR, "Couldn't retrieve hostname");
        exit(1);
    }
    debug(VERBOSE, "Retrieved hostname");
    struct hostent *hostEntry = gethostbyname(hostbuffer);
    if (hostEntry == NULL)
    {
        debug(ERROR, "Couldn't retrieve hostEntry");
        exit(1);
    }
    debug(VERBOSE, "Retrieved hostEntry");
    for (int i = 0; i < hostEntry->h_length; i++)
    {
        char *ip = inet_ntoa(*((struct in_addr *)(hostEntry->h_addr_list[i])));
        if (strcmp(ip, "127.0.0.1") != 0)
            return ip;
    }
    return NULL;
}

int getPort()
{
    srand(time(NULL));
    return (rand() % 9000) + 1000;
}