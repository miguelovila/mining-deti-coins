//
// Miguel Vila,  November 2024
//
// Arquiteturas de Alto Desempenho 2024/2025
//
// Useful functions for implementing normal and parallel versions of the coin search
//

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
static void init_coin_template_avx2(u08_t *bytes, u32_t lane, u32_t n_random_words)
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

/**
 * Initialize the coin with a special tamplate :)
 *
 * This function initializes the coin with the template plus a user string and one
 * random word.
 *
 * @param bytes The interleaved coin data array
 * @param lane The lane number to write the coin data to [0 - 7]
 * @param search_string The user string to be added to the coin (max 36 characters)
 */
static void init_coin_special_template_avx2(u08_t *bytes, u32_t lane, char *search_string)
{
    static const u08_t template[10] = {'D', 'E', 'T', 'I', ' ', 'c', 'o', 'i', 'n', ' '};

    // Apply template bytes
    for (u32_t i = 0; i < 10; i++)
    {
        u32_t byte_pos = (i / 4u) * 8u * 4u + (i % 4u) + lane * 4u;
        bytes[byte_pos] = template[i];
    }

    // Apply search string
    int search_string_len = strlen(search_string);
    for (u32_t i = 0; i < search_string_len && i < 36; i++)
    {
        u32_t byte_pos = ((10u + i) / 4u) * 8u * 4u + ((10u + i) % 4u) + lane * 4u;
        bytes[byte_pos] = search_string[i];
    }

    // Initialize remaining bytes
    for (u32_t i = 10 + search_string_len; i < 51; i++)
    {
        u32_t byte_pos = (i / 4u) * 8u * 4u + (i % 4u) + lane * 4u;
        bytes[byte_pos] = ' ';
    }

    for (u32_t i = 10 + search_string_len; i < 10 + search_string_len + 4; i++)
    {
        u32_t byte_pos = (i / 4u) * 8u * 4u + (i % 4u) + lane * 4u;
        bytes[byte_pos] += random() % 95; // ASCII 32-126
    }

    // Set newline
    bytes[(12u * 8u * 4u) + (3u) + lane * 4u] = '\n';
}

#endif