#ifndef DETI_COINS_CPU_AVX2_CLIENT_SEARCH
#define DETI_COINS_CPU_AVX2_CLIENT_SEARCH

#include "../common/init_coin_template_avx2.h"
#include <unistd.h>
static void deti_coins_cpu_avx2_client_search(const char *server_ip, int port, u32_t seconds)
{
    // Get hostname
    if (gethostname(client_hostname, sizeof(client_hostname)) < 0)
    {
        strncpy(client_hostname, "unknown", sizeof(client_hostname));
    }
    client_hostname[sizeof(client_hostname) - 1] = '\0';

    // Connect to server
    server_socket = connect_to_server(server_ip, port);

    // Send hello message
    message_t msg;
    msg.type = MSG_TYPE_HELLO;
    strncpy(msg.hostname, client_hostname, sizeof(msg.hostname));
    msg.tech_type = TECH_TYPE_AVX2;
    msg.omp_threads = 0;

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

    u32_t coin_data[13u * 8u] __attribute__((aligned(32)));
    u32_t hash_data[4u * 8u] __attribute__((aligned(32)));
    u08_t *bytes = (u08_t *)coin_data;

    // Initialize all lanes
    for (u32_t lane = 0; lane < 8u; lane++)
    {
        init_coin_template_avx2(bytes, lane, n_random_words);
    }

    #if DEBUG > 0
        print_lanes(coin_data, 8, 8);
    #endif

    u64_t n_attempts = 0ul, n_coins = 0ul;
    u32_t start_pos = 10u + n_random_words * 4u;

    while (!stop_request)
    {
        // Compute hashes for all lanes
        md5_cpu_avx2((v8si *)coin_data, (v8si *)hash_data);

        // Check each lane for valid coins
        for (u32_t lane = 0; lane < 8u; lane++)
        {
            u32_t hash[4];
            for (u32_t i = 0; i < 4u; i++)
            {
                hash[i] = hash_data[i * 8u + lane];
            }

            hash_byte_reverse(hash);
            if (deti_coin_power(hash) >= 32u)
            {
                u32_t coin[13];
                for (u32_t i = 0; i < 13u; i++)
                {
                    coin[i] = coin_data[i * 8u + lane];
                }
                save_deti_coin(coin);
                n_coins++;
            }

            #if DEBUG > 0
                print_coin_in_lane(coin_data, lane, 8);
            #endif

            // Increment search space
            u32_t carry = 1;
            for (u32_t i = start_pos; carry && i < 51u; i++)
            {
                u32_t byte_pos = (i / 4u) * 8u * 4u + (i % 4u) + lane * 4u;
                if (bytes[byte_pos] == '~')
                {
                    bytes[byte_pos] = ' ';
                }
                else
                {
                    bytes[byte_pos]++;
                    carry = 0;
                }
            }

            // If we've exhausted the search space, generate new random content
            if (carry)
            {
                init_coin_template_avx2(bytes, lane, n_random_words);
            }
        }

        n_attempts += 8ul;
    }

    printf("[INF] deti_coins_cpu_avx2_search: %06lu DETI coins sent in %lu attempts (expected %.2f coins)\n",
           n_coins, n_attempts, (double)n_attempts / (double)(1ul << 32));

    close(server_socket);
    server_socket = -1;

    printf("[INF] Disconnected from server\n");
}

#endif