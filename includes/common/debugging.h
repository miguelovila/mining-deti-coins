//
// Miguel Vila,  November 2024
//
// Arquiteturas de Alto Desempenho 2024/2025
//
// Useful functions for debugging and testing purposes
//

#if DEBUG > 0

/**
 * Print coin data lanes in a formatted table.
 * Supports AVX (4 lanes), AVX2 (8 lanes), and AVX512 (16 lanes).
 *
 * @param coin_data The interleaved coin data array
 * @param n_lanes Number of parallel lanes (4, 8 or 16)
 * @param stride The stride between elements (4 for AVX, 8 for AVX2, 16 for AVX512)
 */
static void print_lanes(u32_t *coin_data, int n_lanes, int stride)
{
    printf("\nLane Visualization (%d parallel attempts):\n", n_lanes);

    // Print header
    printf("    ");
    for (int lane = 0; lane < n_lanes; lane++)
    {
        printf("%02d   ", lane);
    }
    printf("\n    ");
    for (int lane = 0; lane < n_lanes; lane++)
    {
        printf("---- ");
    }
    printf("\n");

    // Print each word's data across all lanes
    for (int word = 0; word < 13; word++)
    {
        printf("%2d: ", word);
        for (int lane = 0; lane < n_lanes; lane++)
        {
            u32_t value = coin_data[word * stride + lane];
            for (int byte = 0; byte < 4; byte++)
            {
                char c = ((u08_t *)&value)[byte];
                if (c >= 32 && c <= 126)
                    printf("%c", c);
                else if (c == '\n')
                    printf("\\n");
                else
                    printf(".");
            }
            printf(" ");
        }
        printf("\n");
    }
}

/**
 * Print the contents of a specific lane
 *
 * @param coin_data The interleaved coin data array
 * @param lane The lane number to print
 * @param stride The stride between elements (4 for AVX, 8 for AVX2, 16 for AVX512)
 */
static void print_coin_in_lane(u32_t *coin_data, u32_t lane, u32_t stride)
{
    printf("\n────────────── Coin content in lane %02u ──────────────\n", lane);
    for (u32_t word = 0; word < 13; word++)
    {
        u32_t value = coin_data[word * stride + lane];
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
                printf(".");
            }
        }
    }
    printf("\n");
}

#endif