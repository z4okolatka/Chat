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

#define MAX_CLIENTS 50
#define MAX_WAITING_CONNECTIONS 50
#define BUFFER_SIZE 1024

typedef struct
{
    int server_socket;
    struct sockaddr_in server_address;
    char username[USERNAME_BUFFER];
} Client;

Client clients[MAX_CLIENTS];
int clients_count = 0;
pthread_mutex_t clients_mutex;
void *handleClient(void *arg);
void broadcastMessage(const char *message, int senderSocket);
void removeClient(int clientSocket);
void initServer();
void destroyServer();
char *getLocalIPAddress();
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
            debug(ERROR, "Couldn't lock mutex");
            exit(1);
        }
        debug(VERBOSE, "Locked mutex");
        if (clients_count < MAX_CLIENTS)
        {
            clients[clients_count].server_socket = client_socket;
            clients[clients_count].server_address = client_address;
            clients_count++;

            pthread_t thread;
            if (pthread_create(&thread, NULL, handleClient, (void *)&clients[clients_count - 1]) != 0)
            {
                debug(ERROR, "Couldn't create thread for client");
                close(client_socket);
                exit(1);
            }
            debug(VERBOSE, "Created new thread for client");
        }
        else
        {
            debug(WARNING, "Max clients reached. Rejecting new client");
            close(client_socket);
        }
        if (pthread_mutex_unlock(&clients_mutex) != 0)
        {
            debug(ERROR, "Couldn't unlock mutex");
            exit(1);
        }
        debug(VERBOSE, "Unlocked mutex");
    }

    destroyServer();
    return 0;
}

void broadcastMessage(const char *message, int senderSocket)
{
    if (pthread_mutex_lock(&clients_mutex) != 0)
    {
        debug(ERROR, "Couldn't lock mutex");
        exit(1);
    }
    debug(VERBOSE, "Locked mutex");

    for (int i = 0; i < clients_count; i++)
        // if (clients[i].server_socket != senderSocket)
        send(clients[i].server_socket, message, strlen(message), 0);

    if (pthread_mutex_unlock(&clients_mutex) != 0)
    {
        debug(ERROR, "Couldn't unlock mutex");
        exit(1);
    }
    debug(VERBOSE, "Unlocked mutex");
}

void removeClient(int clientSocket)
{
    if (pthread_mutex_lock(&clients_mutex) != 0)
    {
        debug(ERROR, "Couldn't lock mutex");
        exit(1);
    }
    debug(VERBOSE, "Locked mutex");
    for (int i = 0; i < clients_count; i++)
        if (clients[i].server_socket == clientSocket)
        {
            for (int j = i; j < clients_count; j++)
                clients[j] = clients[j + 1];
            clients_count--;
            break;
        }
    if (pthread_mutex_unlock(&clients_mutex) != 0)
    {
        debug(ERROR, "Couldn't unlock mutex\n");
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
    char buffer[BUFFER_SIZE];
    int bytesReceived = recv(client->server_socket, buffer, sizeof(buffer) - 1, 0);
    strcpy(client->username, buffer);
    debug(INFO, "Client's username: \"%s\"", client->username, bytesReceived);

    debug(VERBOSE, "Listening to messages from \"%s\"", client->username);
    while ((bytesReceived = recv(client->server_socket, buffer, sizeof(buffer) - 1, 0)) > 0)
    {
        buffer[bytesReceived] = '\0';
        debug(INFO, "Client's (\"%s\") message: \"%s\" of size %d", client->username, buffer, bytesReceived);

        debug(VERBOSE, "Broadcasting message to everyone");
        broadcastMessage(buffer, client->server_socket);
        memset(buffer, 0, BUFFER_SIZE);
    }

    if (bytesReceived == 0)
        debug(WARNING, "Client disconnected");
    else if (bytesReceived == -1)
        debug(WARNING, "Client recv failed");

    removeClient(client->server_socket);
    close(client->server_socket);

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
    return rand() % 10000;
}