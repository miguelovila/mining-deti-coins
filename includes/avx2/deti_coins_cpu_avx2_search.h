#ifndef DETI_COINS_CPU_AVX2_SEARCH
#define DETI_COINS_CPU_AVX2_SEARCH

#include <string.h>

static void deti_coins_cpu_avx2_search(u32_t n_random_words)
{
    static u32_t coin_data[13u * 8u] __attribute__((aligned(32)));
    static u32_t hash_data[4u * 8u] __attribute__((aligned(32)));
    u64_t n_attempts = 0ul, n_coins = 0ul;
    u32_t lane, idx;

    // Create base template for DETI coin
    u08_t template[52] = {
        'D', 'E', 'T', 'I', ' ', 'c', 'o', 'i', 'n', ' ',
        ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ',
        ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ',
        ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ',
        ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ',
        ' ', '\n'};

    // Initialize 8 parallel coin attempts
    for (lane = 0u; lane < 8u; lane++)
    {
        // Copy template to this lane's data with proper interleaving
        for (idx = 0u; idx < 13u; idx++)
        {
            u32_t word = 0;
            memcpy(&word, template + idx * 4, 4);
            coin_data[idx * 8u + lane] = word;
        }

        // Add randomization if requested
        if (n_random_words > 0u)
        {
            for (idx = 0u; idx < n_random_words && idx < 10u; idx++)
            {
                u32_t pos = 10u + idx;
                template[pos] = ' ' + (random() % (126 - 32));
                u32_t word_idx = (pos / 4);
                u32_t word = 0;
                memcpy(&word, template + word_idx * 4, 4);
                coin_data[word_idx * 8u + lane] = word;
            }
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
                save_deti_coin(coin);
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
                    template[pos] = ' ' + (random() % (126 - 32));
                    u32_t word_idx = (pos / 4);
                    u32_t word = 0;
                    memcpy(&word, template + word_idx * 4, 4);
                    coin_data[word_idx * 8u + lane] = word;
                }
            }
        }
    }

    STORE_DETI_COINS();
    printf("deti_coins_cpu_avx2_search: %lu DETI coin%s found in %lu attempt%s (expected %.2f coins)\n",
           n_coins, (n_coins == 1ul) ? "" : "s",
           n_attempts, (n_attempts == 1ul) ? "" : "s",
           (double)n_attempts / (double)(1ul << 32));
}

#endif