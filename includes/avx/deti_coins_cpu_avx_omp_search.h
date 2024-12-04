#ifndef DETI_COINS_CPU_AVX_OMP_SEARCH
#define DETI_COINS_CPU_AVX_OMP_SEARCH

#include "../common/init_coin_template_avx.h"
#include <omp.h>

static void deti_coins_cpu_avx_omp_search(u32_t n_random_words, bool is_client)
{
    const int n_threads = omp_get_max_threads();
    u64_t global_attempts = 0ul, global_coins = 0ul;

    #pragma omp parallel reduction(+ : global_attempts, global_coins)
    {
        u32_t coin_data[13u * 4u] __attribute__((aligned(16)));
        u32_t hash_data[4u * 4u] __attribute__((aligned(16)));
        u08_t *bytes = (u08_t *)coin_data;
        const int thread_id = omp_get_thread_num();

        // Initialize thread's lanes with different random seeds
        #pragma omp critical
        {
            for (u32_t lane = 0; lane < 4u; lane++)
            {
                init_coin_template_avx(bytes, lane, n_random_words);
            }
        }

        u64_t thread_attempts = 0ul, thread_coins = 0ul;
        u32_t start_pos = 10u + n_random_words * 4u;

        #if DEBUG > 0
            if (thread_id == 0)
            {
                printf("Initializing Thread %02d:\n", thread_id);
                print_lanes(coin_data, 4, 4);
                printf("\n");
            }
        #endif

        while (!stop_request)
        {
            md5_cpu_avx((v4si *)coin_data, (v4si *)hash_data);

            for (u32_t lane = 0; lane < 4u; lane++)
            {
                u32_t hash[4];
                for (u32_t i = 0; i < 4u; i++)
                {
                    hash[i] = hash_data[i * 4u + lane];
                }

                hash_byte_reverse(hash);
                if (deti_coin_power(hash) >= 32u)
                {
                    u32_t coin[13];
                    for (u32_t i = 0; i < 13u; i++)
                    {
                        coin[i] = coin_data[i * 4u + lane];
                    }
                    #pragma omp critical
                    {
                        is_client ? client_save_deti_coin(coin) : save_deti_coin(coin);
                    }
                    thread_coins++;
                }

                #if DEBUG > 0
                    if (thread_id == 0)
                    {
                        printf("Thread %02d:\n", thread_id);
                        print_coin_in_lane(coin_data, lane, 4);
                        printf("\n");
                    }
                #endif

                // Increment search space
                u32_t carry = 1;
                for (u32_t i = start_pos; carry && i < 51u; i++)
                {
                    u32_t byte_pos = (i / 4u) * 4u * 4u + (i % 4u) + lane * 4u;
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
                    init_coin_template_avx(bytes, lane, n_random_words);
                }
            }

            thread_attempts += 4ul;
        }

        global_attempts += thread_attempts;
        global_coins += thread_coins;

        printf("Thread %02d: %03lu DETI coins found in %lu attempts\n", thread_id, thread_coins, thread_attempts);
    }

    if (!is_client) { STORE_DETI_COINS(); }
    
    printf("deti_coins_cpu_avx_omp_search: %05lu DETI coins found in %lu attempts using %02d threads (expected %.2f coins)\n",
           global_coins, global_attempts, n_threads, (double)global_attempts / (double)(1ul << 32));
}

#endif