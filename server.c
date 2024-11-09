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
char *getIpAddress(struct sockaddr_in address);
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
        perror("Bind failed\n");
        close(server_socket);
        exit(1);
    }

    if (listen(server_socket, MAX_WAITING_CONNECTIONS) < 0)
    {
        perror("Listen failed\n");
        close(server_socket);
        exit(1);
    }

    printf("Listening on %s:%d\n", getLocalIPAddress(), ntohs(server_address.sin_port));

    while (1)
    {
        int client_socket;
        struct sockaddr_in client_address;
        socklen_t client_len = sizeof(client_address);

        client_socket = accept(server_socket, (struct sockaddr *)&client_address, &client_len);
        if (client_socket < 0)
        {
            perror("Accept failed");
            continue;
        }

        pthread_mutex_lock(&clients_mutex);
        if (clients_count < MAX_CLIENTS)
        {
            clients[clients_count].server_socket = client_socket;
            clients[clients_count].server_address = client_address;
            clients_count++;
            printf("Client connected\n", getIpAddress(client_address));

            pthread_t thread;
            pthread_create(&thread, NULL, handleClient, (void *)&clients[clients_count - 1]);
        }
        else
        {
            printf("Max clients reached. Rejecting new client.\n");
            close(client_socket);
        }
        pthread_mutex_unlock(&clients_mutex);
    }

    destroyServer();
    return 0;
}

void broadcastMessage(const char *message, int senderSocket)
{
    pthread_mutex_lock(&clients_mutex);

    for (int i = 0; i < clients_count; i++)
    {
        if (clients[i].server_socket != senderSocket)
        {
            send(clients[i].server_socket, message, strlen(message), 0);
        }
    }
}

void removeClient(int clientSocket)
{
    pthread_mutex_lock(&clients_mutex);
    for (int i = 0; i < clients_count; i++)
        if (clients[i].server_socket == clientSocket)
        {
            for (int j = i; j < clients_count; j++)
                clients[j] = clients[j + 1];
            clients_count--;
            break;
        }
    pthread_mutex_unlock(&clients_mutex);
}
void initServer() { pthread_mutex_init(&clients_mutex, NULL); }
void destroyServer() { pthread_mutex_destroy(&clients_mutex); }

void *handleClient(void *arg)
{
    Client *client = ((Client *)arg);
    char buffer[BUFFER_SIZE];
    int bytes_received = recv(client->server_socket, buffer, sizeof(buffer) - 1, 0);
    char username[USERNAME_BUFFER];
    strcpy(client->username, buffer);
    printf("Username: %s\n", buffer);

    while ((bytes_received = recv(client->server_socket, buffer, sizeof(buffer) - 1, 0)) > 0)
    {
        buffer[bytes_received] = '\0';
        printf("Received message: %s\n", buffer);

        broadcastMessage(buffer, client->server_socket);
    }

    if (bytes_received == 0)
    {
        printf("Client(%d) disconnected\n", client->server_socket);
    }
    else if (bytes_received == -1)
    {
        printf("Client(%d) recv failed\n", client->server_socket);
    }

    removeClient(client->server_socket);
    close(client->server_socket);

    return NULL;
}

char *getIpAddress(struct sockaddr_in address)
{
    return inet_ntoa(address.sin_addr);
}

char *getLocalIPAddress()
{
    char hostbuffer[256];
    int hostname = gethostname(hostbuffer, sizeof(hostbuffer));
    struct hostent *hostEntry = gethostbyname(hostbuffer);
    const int index = strcmp(hostEntry->h_addr_list[0], "127.0.0.1") ? 1 : 0;
    char *IPBuffer = inet_ntoa(*((struct in_addr*)hostEntry->h_addr_list[index]));
    return IPBuffer;
}

int getPort()
{
    srand(time(NULL));
    return rand() % 10000;
}