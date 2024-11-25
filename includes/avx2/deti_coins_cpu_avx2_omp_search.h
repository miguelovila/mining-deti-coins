#ifndef DETI_COINS_CPU_AVX2_OMP_SEARCH
#define DETI_COINS_CPU_AVX2_OMP_SEARCH

#include <string.h>
#include <omp.h>

static void deti_coins_cpu_avx2_omp_search(u32_t n_random_words)
{
    u64_t total_attempts = 0ul, total_coins = 0ul;
    int num_threads;
    omp_lock_t vault_lock;

    // Initialize OpenMP lock for vault access
    omp_init_lock(&vault_lock);

#pragma omp parallel reduction(+ : total_attempts, total_coins)
    {
        // Thread-specific data structures - removed static keyword
        u32_t coin_data[13u * 8u] __attribute__((aligned(32)));
        u32_t hash_data[4u * 8u] __attribute__((aligned(32)));
        u64_t n_attempts = 0ul, n_coins = 0ul;
        u32_t lane, idx;
        unsigned int thread_seed = time(NULL) ^ omp_get_thread_num(); // Better seed initialization

// Record number of threads (only in first thread)
#pragma omp single
        {
            num_threads = omp_get_num_threads();
            printf("Running with %d threads\n", num_threads);
        }

        // Create base template for DETI coin
        u08_t template[52] = {
            'D', 'E', 'T', 'I', ' ', 'c', 'o', 'i', 'n', ' ',
            ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ',
            ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ',
            ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ',
            ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ',
            ' ', '\n'};

        // Initialize 8 parallel coin attempts with thread-specific starting points
        for (lane = 0u; lane < 8u; lane++)
        {
            // Copy template to this lane's data with proper interleaving
            for (idx = 0u; idx < 13u; idx++)
            {
                u32_t word = 0;
                memcpy(&word, template + idx * 4, 4);
                coin_data[idx * 8u + lane] = word;
            }

            // Add thread-specific randomization
            if (n_random_words > 0u)
            {
                for (idx = 0u; idx < n_random_words && idx < 10u; idx++)
                {
                    u32_t pos = 10u + idx;
                    // Use thread_seed for randomization
                    template[pos] = ' ' + ((rand_r(&thread_seed) % (126 - 32)));
                    u32_t word_idx = (pos / 4);
                    u32_t word = 0;
                    memcpy(&word, template + word_idx * 4, 4);
                    coin_data[word_idx * 8u + lane] = word;
                }
            }

            // Add thread-specific offset to ensure different search spaces
            u32_t thread_offset = omp_get_thread_num() * 8u + lane;
            u32_t start_pos = 10u + n_random_words;
            if (start_pos < 51u)
            { // Ensure we don't modify the newline
                u32_t word_idx = start_pos / 4;
                u32_t char_idx = start_pos % 4;
                u32_t word = coin_data[word_idx * 8u + lane];
                u08_t *chars = (u08_t *)&word;
                chars[char_idx] = ' ' + (thread_offset % (126 - 32));
                coin_data[word_idx * 8u + lane] = word;
            }
        }

        // Search loop
        while (!stop_request)
        {
            // Compute 8 hashes in parallel
            md5_cpu_avx2((v8si *)coin_data, (v8si *)hash_data);

            // Check all 8 results
            for (lane = 0u; lane < 8u; lane++)
            {
                u32_t hash[4];
                for (idx = 0u; idx < 4u; idx++)
                {
                    hash[idx] = hash_data[idx * 8u + lane];
                }

                hash_byte_reverse(hash);

                u32_t n = deti_coin_power(hash);
                if (n >= 32u)
                {
                    // Found a coin - deinterleave data
                    u32_t coin[13];
                    for (idx = 0u; idx < 13u; idx++)
                    {
                        coin[idx] = coin_data[idx * 8u + lane];
                    }

                    // Thread-safe vault access
                    omp_set_lock(&vault_lock);
                    save_deti_coin(coin);
                    omp_unset_lock(&vault_lock);

                    n_coins++;
                }
            }

            n_attempts += 8ul;

            // Increment search space
            for (lane = 0u; lane < 8u; lane++)
            {
                u32_t carry = 1;
                for (idx = (10u + n_random_words + 3u) / 4u; carry && idx < 13u; idx++)
                {
                    u32_t word = coin_data[idx * 8u + lane];
                    u08_t *chars = (u08_t *)&word;

                    for (u32_t j = 0; j < 4 && carry; j++)
                    {
                        if (chars[j] == 126)
                        {
                            chars[j] = ' ';
                        }
                        else
                        {
                            chars[j]++;
                            carry = 0;
                        }
                    }

                    coin_data[idx * 8u + lane] = word;
                }

                if (carry)
                {
                    // Reset this lane with new random values
                    for (idx = 0u; idx < n_random_words && idx < 10u; idx++)
                    {
                        u32_t pos = 10u + idx;
                        template[pos] = ' ' + ((rand_r(&thread_seed) % (126 - 32)));
                        u32_t word_idx = (pos / 4);
                        u32_t word = 0;
                        memcpy(&word, template + word_idx * 4, 4);
                        coin_data[word_idx * 8u + lane] = word;
                    }
                }
            }

            // Periodic vault store with thread safety
            if ((n_attempts & 0xFFFFFF) == 0)
            {
                omp_set_lock(&vault_lock);
                STORE_DETI_COINS();
                omp_unset_lock(&vault_lock);
            }
        }

        // Add thread totals to reduction variables
        total_attempts += n_attempts;
        total_coins += n_coins;
    }

    // Final store of coins with implicit thread synchronization
    omp_set_lock(&vault_lock);
    STORE_DETI_COINS();
    omp_unset_lock(&vault_lock);

    // Cleanup
    omp_destroy_lock(&vault_lock);

    printf("deti_coins_cpu_avx2_omp_search: %lu DETI coin%s found in %lu attempt%s (expected %.2f coins) using %d threads\n",
           total_coins, (total_coins == 1ul) ? "" : "s",
           total_attempts, (total_attempts == 1ul) ? "" : "s",
           (double)total_attempts / (double)(1ul << 32),
           num_threads);
}

#endif