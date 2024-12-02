#ifndef DETI_COINS_CLIENT
#define DETI_COINS_CLIENT

#include <unistd.h>
#include <omp.h>

static void client_search_wrapper(const char *server_ip, int port, u32_t seconds, uint32_t tech_type, uint32_t omp)
{
    // Get hostname
    if (gethostname(client_hostname, sizeof(client_hostname)) < 0)
    {
        strncpy(client_hostname, "unknown", sizeof(client_hostname));
    }
    client_hostname[sizeof(client_hostname) - 1] = '\0';

    omp = omp > 0 ? tech_type : 0;

    // Connect to server
    server_socket = connect_to_server(server_ip, port);

    // Send hello message
    message_t msg;
    msg.type = MSG_TYPE_HELLO;
    strncpy(msg.hostname, client_hostname, sizeof(msg.hostname));
    msg.tech_type = tech_type;
    msg.omp_threads = omp;

    if (send_message(server_socket, &msg) < 0)
    {
        perror("Failed to send hello message");
        close(server_socket);
        return;
    }

    printf("[INF] Connected to server %s:%d\n", server_ip, port);
    printf("[INF] Client identifier: %s\n", client_hostname);

    // Receive configuration
    if (receive_message(server_socket, &msg) <= 0 || msg.type != MSG_TYPE_CONFIG)
    {
        perror("[ERR] Failed to receive configuration");
        close(server_socket);
        return;
    }

    u32_t n_random_words = msg.n_random_words;
    printf("[INF] Received configuration: n_random_words = %u\n", n_random_words);

    // Search for coins
    switch (tech_type)
    {
    case TECH_TYPE_CPU:
        if (omp > 0)
        {
            deti_coins_cpu_omp_search(true);
        }
        else
        {
            deti_coins_cpu_search(true);
        }
        break;
    case TECH_TYPE_AVX:
        if (omp > 0)
        {
            #ifdef DETI_COINS_CPU_AVX_OMP_SEARCH
                deti_coins_cpu_avx_omp_search(n_random_words, true);
            #endif
        }
        else
        {
            #ifdef DETI_COINS_CPU_AVX_SEARCH
                deti_coins_cpu_avx_search(n_random_words, true);
            #endif
        }
        break;
    case TECH_TYPE_AVX2:
        if (omp > 0)
        {
            #ifdef DETI_COINS_CPU_AVX2_OMP_SEARCH
                deti_coins_cpu_avx2_omp_search(n_random_words, true);
            #endif
        }
        else
        {
            #ifdef DETI_COINS_CPU_AVX2_SEARCH
                deti_coins_cpu_avx2_search(n_random_words, true);
            #endif
        }
        break;
    case TECH_TYPE_AVX512:
        if (omp > 0)
        {
            #ifdef DETI_COINS_CPU_AVX512_OMP_SEARCH
                deti_coins_cpu_avx512_omp_search(n_random_words, true);
            #endif
        }
        else
        {
            #ifdef DETI_COINS_CPU_AVX512_SEARCH
                deti_coins_cpu_avx512_search(n_random_words, true);
            #endif
        }
        break;
    #ifdef DETI_COINS_CPU_NEON_SEARCH
    case TECH_TYPE_NEON:
        deti_coins_cpu_neon_search(n_random_words);
        break;
    #endif
    #ifdef DETI_COINS_CUDA_SEARCH
    case TECH_TYPE_CUDA:
        deti_coins_cuda_search(n_random_words);
        break;
    #endif
    #ifdef DETI_COINS_CPU_SPECIAL_SEARCH
    case TECH_TYPE_SPECIAL:
        deti_coins_cpu_special_search();
        break;
    #endif
    default:
        printf("[ERR] Invalid technology type\n");
        break;
    }

    close(server_socket);
    server_socket = -1;

    printf("[INF] Disconnected from server\n");
}

#endif