#ifndef DETI_COINS_ORCHESTRATOR
#define DETI_COINS_ORCHESTRATOR

#include <pthread.h>

static volatile int orchestrator_running = 1;

// Handle client connection
static void handle_client(int client_socket, u32_t n_random_words)
{
    message_t msg;

    // Wait for hello
    if (receive_message(client_socket, &msg) <= 0 || msg.type != MSG_TYPE_HELLO)
    {
        perror("[ERR] Failed to receive client hello");
        close(client_socket);
        return;
    }

    // Log client connection and tore tech type and omp threads
    printf("[INF] Client %s just connected. Using %s%s%s%u%s\n",
        msg.hostname,
        msg.tech_type == TECH_TYPE_CPU ? "CPU" :
        msg.tech_type == TECH_TYPE_AVX ? "AVX" :
        msg.tech_type == TECH_TYPE_AVX2 ? "AVX2" :
        msg.tech_type == TECH_TYPE_AVX512 ? "AVX512" :
        msg.tech_type == TECH_TYPE_CUDA ? "CUDA" :
        msg.tech_type == TECH_TYPE_SPECIAL ? "AVX2 Special Search" : "Unknown",
        msg.omp_threads > 0 ? " with OpenMP (" : ". (",
        msg.omp_threads > 0 ? "" : "",
        msg.omp_threads > 0 ? msg.omp_threads : 1,
        msg.omp_threads > 0 ? " threads)" : " thread)");

    // Send configuration to client
    msg.type = MSG_TYPE_CONFIG;
    msg.n_random_words = n_random_words;

    if (send_message(client_socket, &msg) < 0)
    {
        perror("[ERR] Failed to send configuration");
        close(client_socket);
        return;
    }

    // Receive found coins from client
    while (orchestrator_running)
    {
        if (receive_message(client_socket, &msg) <= 0)
        {
            break; // Client disconnected or error
        }

        if (msg.type == MSG_TYPE_COIN_FOUND)
        {
            printf("[INF] Received coin from %s\n", msg.hostname);
            save_deti_coin(msg.coin);
            STORE_DETI_COINS();
        }
    }

    printf("[INF] Client %s disconnected\n", msg.hostname);
    close(client_socket);
}

// Client handler thread function
static void *client_thread(void *arg)
{
    struct
    {
        int client_socket;
        u32_t n_random_words;
    } *params = arg;

    handle_client(params->client_socket, params->n_random_words);
    free(params);
    return NULL;
}

// Main orchestrator function
static void run_orchestrator(int port, u32_t n_random_words)
{
    int server_fd, client_socket;
    struct sockaddr_in address;
    int addrlen = sizeof(address);

    server_fd = create_server_socket(port);
    printf("[INF] Server is listening on port %d\n", port);
    printf("[INF] Configuration: n_random_words = %u\n", n_random_words);

    while (orchestrator_running)
    {
        client_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen);
        if (client_socket < 0)
        {
            perror("[ERR] Accept failed");
            continue;
        }

        // Create thread parameters
        struct
        {
            int client_socket;
            u32_t n_random_words;
        } *params = malloc(sizeof(*params));
        params->client_socket = client_socket;
        params->n_random_words = n_random_words;

        // Create client handler thread
        pthread_t thread_id;
        if (pthread_create(&thread_id, NULL, client_thread, params) != 0)
        {
            perror("[ERR] Failed to create thread");
            free(params);
            close(client_socket);
            continue;
        }
        pthread_detach(thread_id);
    }

    close(server_fd);
}

#endif