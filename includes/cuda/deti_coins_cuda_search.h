#ifndef DETI_COINS_CUDA_SEARCH
#define DETI_COINS_CUDA_SEARCH

#include <time.h>
#include <stdint.h>
#include <stdio.h>

typedef unsigned int u32_t;
typedef unsigned long u64_t;

typedef struct
{
    uint32_t hash[4];     // MD5 hash
    char coin_string[52]; // 52-character coin string
} DetiCoin;

u32_t next_value_to_try(v)
{
    do
    {
        v++;
        if ((v & 0xFF) == 0x7F)
        {
            v += 0xA1;
            if (((v >> 8) & 0xFF) == 0x7F)
            {
                v += 0xA1 << 8;
                if (((v >> 16) & 0xFF) == 0x7F)
                {
                    v += 0xA1 << 16;
                    if (((v >> 24) & 0xFF) == 0x7F)
                    {
                        v += 0xA1 << 24;
                    }
                }
            }
        }
    } while (0);
}

static void deti_coins_cuda_search(u32_t n_random_words, bool is_client)
{
    u32_t idx, max_idx, random_word, custom_word_1, custom_word_2;
    u64_t n_attempts, n_coins;
    void *params[4]; // Kernel parameters

    random_word = 0x20202020u + time(NULL) % 95; // Randomized base word
    custom_word_1 = 0x20202020u;
    custom_word_2 = 0x20202020u;

    initialize_cuda(0, "deti_coins_cuda_kernel_search.cubin", "deti_coins_cuda_kernel_search", 1024u, 0u);
    max_idx = 1u;

    for (n_attempts = n_coins = 0ul; stop_request == 0; n_attempts += (64ul << 20))
    {
        host_data[0] = 1u;
        CU_CALL(cuMemcpyHtoD, (device_data, (void *)host_data, (size_t)1024 * sizeof(u32_t)));

        params[0] = &device_data;
        params[1] = &random_word;
        params[2] = &custom_word_1;
        params[3] = &custom_word_2;

        CU_CALL(cuLaunchKernel, (cu_kernel,
                                 (1u << 20) / 128u, // Block Number
                                 1u,                // Y-dimension
                                 1u,                // Z-dimension
                                 128u,              // Threads per block
                                 1u,                // block Y-dimension
                                 1u,                // block Z-dimension
                                 0u,                // Size Shared Memory
                                 (CUstream)0,       // Stream
                                 &params[0],
                                 NULL));

        CU_CALL(cuMemcpyDtoH, ((void *)host_data, device_data, (size_t)1024 * sizeof(u32_t)));

        if (host_data[0] > max_idx)
        {
            max_idx = host_data[0];
        }

        for (idx = 1u; idx < host_data[0] && idx <= 1024u - 13u; idx += 13u)
        {
            DetiCoin coin;
            memcpy(coin.coin_string, &host_data[idx], 52); // Coin string
            memcpy(coin.hash, &host_data[idx + 13], 16);   // MD5 hash

            save_deti_coin((u32_t *)coin.coin_string);
            printf("Found a DETI Coin! %.*s\n", 52, coin.coin_string);
            n_coins++;
        }

        u32_t custom_word = next_value_to_try(custom_word_1);
        if (custom_word == 0x20202020)
        {
            next_value_to_try(custom_word_2);
        }
    }

    STORE_DETI_COINS();
    printf("deti_coins_cuda_search: %lu DETI coins found in %lu attempts (expected %.2f coins)\n",
           n_coins, n_attempts, (double)n_attempts / (double)(1ul << 32));
    terminate_cuda();
}

#endif