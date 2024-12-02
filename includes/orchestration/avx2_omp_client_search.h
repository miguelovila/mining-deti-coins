#ifndef DETI_COINS_CPU_AVX2_OMP_CLIENT_SEARCH
#define DETI_COINS_CPU_AVX2_OMP_CLIENT_SEARCH

#include "../common/init_coin_template_avx2.h"
#include <unistd.h>
#include <omp.h>

// Override coin saving to send to server instead
static int server_socket = -1;
static char client_hostname[9];

static void client_save_deti_coin(u32_t coin[13])
{
    if (server_socket != -1)
    {
        message_t msg;
        msg.type = MSG_TYPE_COIN_FOUND;
        strncpy(msg.hostname, client_hostname, sizeof(msg.hostname));
        memcpy(msg.coin, coin, 13 * sizeof(u32_t));

        if (send_message(server_socket, &msg) < 0)
        {
            perror("[ERR] Failed to send coin to server");
        }
        //else
        //{
        //    printf("[INF] Sent coin to server\n");
        //}
    }
}

static void deti_coins_cpu_avx2_omp_client_search(const char *server_ip, int port, u32_t seconds)
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
    msg.omp_threads = omp_get_max_threads();

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

    const int n_threads = omp_get_max_threads();
    u64_t global_attempts = 0ul, global_coins = 0ul;

    #pragma omp parallel reduction(+ : global_attempts, global_coins)
    {
        u32_t coin_data[13u * 8u] __attribute__((aligned(32)));
        u32_t hash_data[4u * 8u] __attribute__((aligned(32)));
        u08_t *bytes = (u08_t *)coin_data;
        const int thread_id = omp_get_thread_num();

        #pragma omp critical
        {
            for (u32_t lane = 0; lane < 8u; lane++)
            {
                init_coin_template_avx2(bytes, lane, n_random_words);
            }
        }

        u64_t thread_attempts = 0ul, thread_coins = 0ul;
        u32_t start_pos = 10u + n_random_words * 4u;

        #if DEBUG > 0
            if (thread_id == 0)
            {
                printf("Initializing Thread %02d:\n", thread_id);
                print_lanes(coin_data, 8, 8);
                printf("\n");
            }
        #endif

        while (!stop_request)
        {
            md5_cpu_avx2((v8si *)coin_data, (v8si *)hash_data);

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
                    #pragma omp critical
                    {
                        client_save_deti_coin(coin);
                    }
                    thread_coins++;
                }

                #if DEBUG > 0
                    if (thread_id == 0)
                    {
                        printf("Thread %02d:\n", thread_id);
                        print_coin_in_lane(coin_data, lane, 8);
                        printf("\n");
                    }
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

                if (carry)
                {
                    init_coin_template_avx2(bytes, lane, n_random_words);
                }
            }

            thread_attempts += 8ul;
        }

        global_attempts += thread_attempts;
        global_coins += thread_coins;

        printf("[INF] thread %02d: %03lu DETI coins found in %lu attempts\n", thread_id, thread_coins, thread_attempts);
    }

    printf("[INF] deti_coins_cpu_avx2_omp_search: %05lu DETI coins sent in %lu attempts using %02d threads (expected %.2f coins)\n",
           global_coins, global_attempts, n_threads, (double)global_attempts / (double)(1ul << 32));

    close(server_socket);
    server_socket = -1;

    printf("[INF] Disconnected from server\n");
}

#endif