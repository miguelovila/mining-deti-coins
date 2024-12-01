//
// Miguel Vila,  November 2024
//
// Arquiteturas de Alto Desempenho 2024/2025
//
// Useful functions for implementing normal and parallel versions of the coin search
//

#ifndef CPU_AVX_UTILITIES
#define CPU_AVX_UTILITIES

/**
 * Initialize the coin
 *
 * This function initializes the coin with the template and random words in the beginning and
 * when the search space is exhausted thereby generating new random content.
 *
 * @param bytes The interleaved coin data array
 * @param lane The lane number to write the coin data to [0 - 3]
 * @param n_random_words The number of random 4-byte words to fill. [1 - 9]
 */
static void init_coin_template_avx(u08_t *bytes, u32_t lane, u32_t n_random_words)
{
    static const u08_t template[10] = {'D', 'E', 'T', 'I', ' ', 'c', 'o', 'i', 'n', ' '};

    // Apply template bytes
    for (u32_t i = 0; i < 10; i++)
    {
        u32_t byte_pos = (i / 4u) * 4u * 4u + (i % 4u) + lane * 4u;
        bytes[byte_pos] = template[i];
    }

    // Initialize remaining bytes with spaces
    for (u32_t i = 10; i < 51; i++)
    {
        u32_t byte_pos = (i / 4u) * 4u * 4u + (i % 4u) + lane * 4u;
        bytes[byte_pos] = ' ';
    }

    // Set newline
    bytes[(12u * 4u * 4u) + (3u) + lane * 4u] = '\n';

    // Generate initial random words
    if (n_random_words > 0)
    {
        for (u32_t i = 0; i < n_random_words * 4 && i + 10 < 51; i++)
        {
            u32_t byte_pos = ((10u + i) / 4u) * 4u * 4u + ((10u + i) % 4u) + lane * 4u;
            bytes[byte_pos] = ' ' + (random() % 95); // ASCII 32-126
        }
    }
}

#endif