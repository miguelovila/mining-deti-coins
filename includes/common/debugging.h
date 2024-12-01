//
// Miguel Vila,  November 2024
//
// Arquiteturas de Alto Desempenho 2024/2025
//
// Useful functions for debugging and testing purposes
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
    printf("\nLane Visualization (%d parallel attempts):\n", n_lanes);
    if (n_lanes == 4)
    {
        printf("     00   01   02   03  \n");
        printf("    ---- ---- ---- ---- \n");
    }
    else if (n_lanes == 8)
    {
        printf("     00   01   02   03   04   05   06   07  \n");
        printf("    ---- ---- ---- ---- ---- ---- ---- ---- \n");
    }
    else if (n_lanes == 16)
    {
        printf("     00   01   02   03   04   05   06   07   08   09   10   11   12   13   14   15  \n");
        printf("    ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- \n");
    }
    else
    {
        return;
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