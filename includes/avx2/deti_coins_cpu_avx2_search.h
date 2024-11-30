#ifndef DETI_COINS_CPU_AVX2_SEARCH
#define DETI_COINS_CPU_AVX2_SEARCH

#include <string.h>

/**
 *  Print lanes
 *
 *  Print the 8 lanes of the coin data in a human-readable format
 *  for debugging purposes. 😅
 */
/*
static void print_lanes(u32_t *coin_data)
{
    printf("\nLane Visualization (8 parallel attempts):\n");
    printf("     00   01   02   03   04   05   06   07  \n");
    printf("    ---- ---- ---- ---- ---- ---- ---- ---- \n");

    for (int word = 0; word < 13; word++)
    {
        printf("%2d: ", word);
        for (int lane = 0; lane < 8; lane++)
        {
            u32_t value = coin_data[word * 8 + lane];
            for (int byte = 0; byte < 4; byte++)
            {
                char c = ((u08_t *)&value)[byte];
                if (c >= 32 && c <= 126)
                    printf("%c", c);
                else if (c == '\n')
                    printf("\\n");
            }
            printf(" ");
        }
        printf("\n");
    }
}

static void print_lane(u32_t *coin_data, u32_t lane)
{
    printf("\nLane Visualization:\n");
    printf("  %2d\n", lane);
    printf(" ---- \n");

    for (int word = 0; word < 13; word++)
    {
        printf("%2d: ", word);
        u32_t value = coin_data[word * 8 + lane];
        for (int byte = 0; byte < 4; byte++)
        {
            char c = ((u08_t *)&value)[byte];
            if (c >= 32 && c <= 126)
                printf("%c", c);
            else if (c == '\n')
                printf("\\n");
        }
        printf("\n");
    }
}
*/

/**
 * Print the contents of a specific lane in a human-readable format
 *
 * @param coin_data The interleaved coin data array
 * @param lane The lane number to print (0-7)
 * @param show_all If true, shows all bytes including non-printable ones
 */
static void print_coin_in_lane(u32_t *coin_data, u32_t lane)
{
    printf("\n──── Coin content in lane %02u ────\n", lane);
    printf("Chr:");
    for (u32_t word = 0; word < 13; word++)
    {
        u32_t value = coin_data[word * 8 + lane];
        for (int byte = 0; byte < 4; byte++)
        {
            u08_t c = ((u08_t *)&value)[byte];
            if (c >= 32 && c <= 126)
            {
                printf("%c", c);
            }
            else if (c == '\n')
            {
                printf("\\n");
            }
            else
            {
                printf(" .");
            }
        }
    }
}

/**
 * Search for DETI coins using AVX2 instructions
 *
 *  This function searches for DETI coins using AVX2 instructions.
 *  The search is performed in parallel for 8 lanes.
 *
 *  n_random_words: number of random 4-byte words to fill. [1 - 9]
 */
static void deti_coins_cpu_avx2_search(u32_t n_random_words)
{
    u64_t n_attempts = 0ul, n_coins = 0ul;

    /**
     *  Coin initialization
     *
     *  Each lane is composed of a deti coin that respects the
     *  template. After the base coin is created, the remaining
     *  bytes are filled with random words (1 word = 4 bytes).
     *
     *  n_random_words: number of random 4-byte words to fill. [1 - 9]
     */

    u32_t coin_data[13u * 8u] __attribute__((aligned(32)));
    u32_t hash_data[4u * 8u] __attribute__((aligned(32)));
    u32_t start_pos = 10u + n_random_words * 4u;

    u32_t lane, idx;
    u08_t template[52] = {
        'D', 'E', 'T', 'I', ' ', 'c', 'o', 'i', 'n', ' ',
        ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ',
        ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ',
        ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ',
        ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ',
        ' ', '\n'};

    for (lane = 0u; lane < 8u; lane++)
    {
        // Add randomization
        for (idx = 0u; idx < n_random_words * 4; idx++)
            template[10u + idx] = ' ' + (random() % 95);

        // Interleaves the template
        for (idx = 0u; idx < 13u; idx++)
        {
            u32_t word = 0;
            memcpy(&word, template + idx * 4, 4);
            coin_data[idx * 8u + lane] = word;
        }
    }

    // print_lanes(coin_data);

    /**
     *  Search loop
     *
     *  The search loop computes 8 hashes in parallel and checks
     *  all 8 results for DETI coins (power >= 32). Then it increments
     *  the coin by 1 and repeats the process.
     */
    while (!stop_request)
    {
        // print_lanes(coin_data);

        // Compute and verify hashes
        //md5_cpu_avx2((v8si *)coin_data, (v8si *)hash_data);
        for (lane = 0u; lane < 8u; lane++)
        {
            // u32_t hash[4];
            // for (idx = 0u; idx < 4u; idx++)
            //     hash[idx] = hash_data[idx * 8u + lane];

            // // printf("hash: %08x %08x %08x %08x\n", hash[0], hash[1], hash[2], hash[3]);

            // hash_byte_reverse(hash);
            // if (deti_coin_power(hash) >= 32u)
            // {
            //     u32_t coin[13];
            //     for (idx = 0u; idx < 13u; idx++)
            //         coin[idx] = coin_data[idx * 8u + lane];
            //     save_deti_coin(coin);
            //     n_coins++;
            // }

            // print the coin in the lane
            if (lane == 0 )
                print_coin_in_lane(coin_data, lane);

            // Start incrementing after random words
            u08_t *bytes = (u08_t *)coin_data;
            for (u32_t i = start_pos; i < 51u; i++)
            {
                u32_t byte_pos = (i / 4u) * 8u * 4u + (i % 4u) + lane * 4u;
                if (bytes[byte_pos] == '~')
                    bytes[byte_pos] = ' '; // Reset to space
                else
                {
                    bytes[byte_pos]++;
                    break;
                }
            }
        }
        n_attempts += 8ul;
    }

    STORE_DETI_COINS();
    printf("deti_coins_cpu_avx2_search: %lu DETI coin%s found in %lu attempt%s (expected %.2f coins)\n",
           n_coins, (n_coins == 1ul) ? "" : "s",
           n_attempts, (n_attempts == 1ul) ? "" : "s",
           (double)n_attempts / (double)(1ul << 32));
}

#endif