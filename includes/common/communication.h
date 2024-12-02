#ifndef DETI_COINS_ORCHESTRATION_COMMON
#define DETI_COINS_ORCHESTRATION_COMMON

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

// Message types
#define MSG_TYPE_CONFIG 1
#define MSG_TYPE_COIN_FOUND 2
#define MSG_TYPE_TELEMETRY 3

// Protocol message structure
typedef struct
{
    uint32_t type;           // Message type
    uint32_t n_random_words; // Configuration value
    uint32_t coin[13];       // Found coin data (when reporting found coins)
} message_t;

// Helper functions for socket communication
static int send_message(int socket_fd, message_t *msg)
{
    return send(socket_fd, msg, sizeof(message_t), 0);
}

static int receive_message(int socket_fd, message_t *msg)
{
    return recv(socket_fd, msg, sizeof(message_t), 0);
}

// Socket creation helper
static int create_server_socket(int port)
{
    int server_fd;
    struct sockaddr_in address;

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0)
    {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)))
    {
        perror("setsockopt failed");
        exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0)
    {
        perror("Bind failed");
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, 3) < 0)
    {
        perror("Listen failed");
        exit(EXIT_FAILURE);
    }

    return server_fd;
}

// Client connection helper
static int connect_to_server(const char *ip, int port)
{
    int sock = 0;
    struct sockaddr_in serv_addr;

    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0)
    {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);

    if (inet_pton(AF_INET, ip, &serv_addr.sin_addr) <= 0)
    {
        perror("Invalid address");
        exit(EXIT_FAILURE);
    }

    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
        perror("Connection failed");
        exit(EXIT_FAILURE);
    }

    return sock;
}

#endif