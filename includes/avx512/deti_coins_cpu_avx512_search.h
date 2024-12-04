#ifndef DETI_COINS_CPU_AVX512_SEARCH
#define DETI_COINS_CPU_AVX512_SEARCH

#include "../common/init_coin_template_avx512.h"

/**
 * Search for DETI coins using AVX-512 instructions
 *
 * This function searches for DETI coins using AVX-512 instructions.
 * The search is performed in parallel for 16 lanes.
 *
 * @param n_random_words Number of random 4-byte words to fill. [1 - 9]
 */
static void deti_coins_cpu_avx512_search(u32_t n_random_words, bool is_client)
{
    u32_t coin_data[13u * 16u] __attribute__((aligned(64)));
    u32_t hash_data[4u * 16u] __attribute__((aligned(64)));
    u08_t *bytes = (u08_t *)coin_data;

    // Initialize all lanes
    for (u32_t lane = 0; lane < 16u; lane++)
    {
        init_coin_template_avx512(bytes, lane, n_random_words);
    }

    #if DEBUG > 0
        print_lanes(coin_data, 16, 16);
    #endif

    u64_t n_attempts = 0ul, n_coins = 0ul;
    u32_t start_pos = 10u + n_random_words * 4u;

    while (!stop_request)
    {
        // Compute hashes for all lanes
        md5_cpu_avx512((v16si *)coin_data, (v16si *)hash_data);

        // Check each lane for valid coins
        for (u32_t lane = 0; lane < 16u; lane++)
        {
            u32_t hash[4];
            for (u32_t i = 0; i < 4u; i++)
            {
                hash[i] = hash_data[i * 16u + lane];
            }

            hash_byte_reverse(hash);
            if (deti_coin_power(hash) >= 32u)
            {
                u32_t coin[13];
                for (u32_t i = 0; i < 13u; i++)
                {
                    coin[i] = coin_data[i * 16u + lane];
                }
                is_client ? client_save_deti_coin(coin) : save_deti_coin(coin);
                n_coins++;
            }

            #if DEBUG > 0
                print_coin_in_lane(coin_data, lane, 16);
            #endif

            // Increment search space
            u32_t carry = 1;
            for (u32_t i = start_pos; carry && i < 51u; i++)
            {
                u32_t byte_pos = (i / 4u) * 16u * 4u + (i % 4u) + lane * 4u;
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
                init_coin_special_template_avx2(bytes, lane, search_string);
            }
        }

        n_attempts += 16ul;
    }

    if (!is_client) { STORE_DETI_COINS(); }

    printf("deti_coins_cpu_avx512_search: %06lu DETI coins found in %lu attempts (expected %.2f coins)\n",
           n_coins, n_attempts, (double)n_attempts / (double)(1ul << 32));
}

#endif