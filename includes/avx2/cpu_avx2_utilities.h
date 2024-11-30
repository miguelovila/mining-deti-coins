//
// Miguel Vila,  November 2024
//
// Arquiteturas de Alto Desempenho 2024/2025
//
// Useful functions for the AVX2 implementation.
//

#if DEBUG > 0

/**
 *  Print the 8 lanes of the coin data (Debugging purposes 😅)
 *
 *  @param coin_data The interleaved coin data array
 *  @param n_lanes The number of lanes to print (4 or 8)
 */
static void print_lanes(u32_t *coin_data, int n_lanes)
{
    if (n_lanes != 4 && n_lanes != 8)
    {
        return;
    }

    printf("\nLane Visualization (%d parallel attempts):\n", n_lanes);   
    if (n_lanes == 4) {
        printf("     00   01   02   03  \n");
        printf("    ---- ---- ---- ---- \n");
    } else {
        printf("     00   01   02   03   04   05   06   07  \n");
        printf("    ---- ---- ---- ---- ---- ---- ---- ---- \n");
    }

    for (int word = 0; word < 13; word++)
    {
        printf("%2d: ", word);
        for (int lane = 0; lane < n_lanes; lane++)
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

/**
 * Print the contents of a specific lane (Debugging purposes 😅)
 *
 * @param coin_data The interleaved coin data array
 * @param lane The lane number to print (0-7)
 * @param show_all If true, shows all bytes including non-printable ones
 */
static void print_coin_in_lane(u32_t *coin_data, u32_t lane)
{
    printf("\n────────────── Coin content in lane %02u ──────────────\n", lane);
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


#endif

#ifndef CPU_AVX2_UTILITIES
#define CPU_AVX2_UTILITIES

/**
 * Initialize the coin
 *
 * This function initializes the coin with the template and random words in the begining and
 * when the search space is exhausted therby generating new random content.
 *
 * @param bytes The interleaved coin data array
 * @param lane The lane number to write the coin data to [0 - 7]
 * @param n_random_words The number of random 4-byte words to fill. [1 - 9]
 */
static void init_coin_template(u08_t *bytes, u32_t lane, u32_t n_random_words)
{
    static const u08_t template[10] = {'D', 'E', 'T', 'I', ' ', 'c', 'o', 'i', 'n', ' '};

    // Apply template bytes
    for (u32_t i = 0; i < 10; i++)
    {
        u32_t byte_pos = (i / 4u) * 8u * 4u + (i % 4u) + lane * 4u;
        bytes[byte_pos] = template[i];
    }

    // Initialize remaining bytes with spaces
    for (u32_t i = 10; i < 51; i++)
    {
        u32_t byte_pos = (i / 4u) * 8u * 4u + (i % 4u) + lane * 4u;
        bytes[byte_pos] = ' ';
    }

    // Set newline
    bytes[(12u * 8u * 4u) + (3u) + lane * 4u] = '\n';

    // Generate initial random words
    if (n_random_words > 0)
    {
        for (u32_t i = 0; i < n_random_words * 4 && i + 10 < 51; i++)
        {
            u32_t byte_pos = ((10u + i) / 4u) * 8u * 4u + ((10u + i) % 4u) + lane * 4u;
            bytes[byte_pos] = ' ' + (random() % 95); // ASCII 32-126
        }
    }
}

#endif
