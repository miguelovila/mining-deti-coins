#ifndef DETI_COINS_CPU_AVX2_SEARCH
#define DETI_COINS_CPU_AVX2_SEARCH

#include "../common/init_coin_template_avx2.h"

/**
 * Search for DETI coins using AVX2 instructions
 *
 * This function searches for DETI coins using AVX2 instructions.
 * The search is performed in parallel for 8 lanes.
 *
 * @param n_random_words nNmber of random 4-byte words to fill. [1 - 9]
 */
static void deti_coins_cpu_avx2_search(u32_t n_random_words, bool is_client)
{
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
                is_client ? client_save_deti_coin(coin) : save_deti_coin(coin);
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

            if (carry)
            {
                init_coin_template_avx2(bytes, lane, n_random_words);
            }
        }

        n_attempts += 8ul;
    }

    if (!is_client) { STORE_DETI_COINS(); }

    printf("deti_coins_cpu_avx2_search: %06lu DETI coins found in %lu attempts (expected %.2f coins)\n",
           n_coins, n_attempts, (double)n_attempts / (double)(1ul << 32));
}

#endif