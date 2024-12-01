//
// OpenMP implementation for DETI coin search
//

#ifndef DETI_COINS_CPU_OMP_SEARCH
#define DETI_COINS_CPU_OMP_SEARCH

#include <omp.h>

static void deti_coins_cpu_omp_search(void)
{
    u32_t idx;
    u64_t total_attempts = 0ul, total_coins = 0ul;

    #pragma omp parallel reduction(+ : total_attempts, total_coins)
    {
        u32_t n, thread_id, coin[13u], hash[4u];
        u64_t thread_attempts = 0ul, thread_coins = 0ul;
        u08_t *bytes;

        thread_id = omp_get_thread_num();
        bytes = (u08_t *)&coin[0];

        // Mandatory prefix for a DETI coin
        bytes[0u] = 'D';
        bytes[1u] = 'E';
        bytes[2u] = 'T';
        bytes[3u] = 'I';
        bytes[4u] = ' ';
        bytes[5u] = 'c';
        bytes[6u] = 'o';
        bytes[7u] = 'i';
        bytes[8u] = 'n';
        bytes[9u] = ' ';

        // Initialize the search space
        for (idx = 10u; idx < 13u * 4u - 1u; idx++)
        {
            bytes[idx] = ' ' + (thread_id % (126 - 32));
        }

        bytes[13u * 4u - 1u] = '\n';

        // Search for DETI coins
        while (stop_request == 0)
        {
            thread_attempts++;
            md5_cpu(coin, hash);
            hash_byte_reverse(hash);
            n = deti_coin_power(hash);
            if (n >= 32u)
            {
                #pragma omp critical
                {
                    save_deti_coin(coin);
                }
                thread_coins++;
            }

            // Generate next combination (byte range: 0x20..0x7E)
            for (idx = 10u; idx < 13u * 4u - 1u && bytes[idx] == (u08_t)126; idx++)
                bytes[idx] = ' ';
            if (idx < 13u * 4u - 1u)
                bytes[idx]++;
            else
                // If the search space is exhausted, start over with a new random combination
                for (idx = 10u; idx < 13u * 4u - 1u; idx++)
                    bytes[idx] = ' ' + (rand() % 95);
        }

        total_attempts += thread_attempts;
        total_coins += thread_coins;

        printf("Thread %d: %lu DETI coin%s found in %lu attempt%s\n",
               thread_id, thread_coins, (thread_coins == 1ul) ? "" : "s",
               thread_attempts, (thread_attempts == 1ul) ? "" : "s");
    }

    STORE_DETI_COINS();
    printf("deti_coins_cpu_openmp_search: %lu DETI coin%s found in %lu attempt%s (expected %.2f coins)\n",
           total_coins, (total_coins == 1ul) ? "" : "s",
           total_attempts, (total_attempts == 1ul) ? "" : "s",
           (double)total_attempts / (double)(1ul << 32));
    printf("Using %d threads\n", omp_get_max_threads());
}

#endif