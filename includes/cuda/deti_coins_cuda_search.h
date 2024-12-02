// includes/cuda/deti_coins_cuda_search.h

#ifndef DETI_COINS_CUDA_SEARCH
#define DETI_COINS_CUDA_SEARCH

#include <cuda.h>
#include "deti_coins_cuda_common.h"

static void deti_coins_cuda_search(u32_t n_random_words, bool is_client)
{
    CUdeviceptr d_found_coins;
    CUfunction kernel;
    u32_t template_data[13];
    u08_t *bytes = (u08_t *)template_data;
    u64_t total_attempts = 0, total_coins = 0;

    // Initialize template
    const u08_t prefix[] = {'D', 'E', 'T', 'I', ' ', 'c', 'o', 'i', 'n', ' '};
    memcpy(bytes, prefix, sizeof(prefix));

    // Fill template with spaces and newline
    for (u32_t i = sizeof(prefix); i < 51; i++)
    {
        bytes[i] = ' ';
    }
    bytes[51] = '\n';

    // Add random content if requested
    if (n_random_words > 0)
    {
        for (u32_t i = 0; i < n_random_words * 4 && i + 10 < 51; i++)
        {
            bytes[10 + i] = ' ' + (random() % 95);
        }
    }

    printf("Initial template: ");
    for (int i = 0; i < 52; i++)
    {
        printf("%c", bytes[i]);
    }
    printf("\n");

    // Initialize CUDA
    initialize_cuda(0, "deti_coins_cuda_kernel_search.cubin", "mine_deti_coins_kernel", 0, sizeof(struct FoundCoins));

    // Get kernel function and allocate memory
    CU_CALL(cuModuleGetFunction, (&kernel, cu_module, "mine_deti_coins_kernel"));
    CU_CALL(cuMemAlloc, (&d_found_coins, sizeof(struct FoundCoins)));

    // Create template structure
    struct CoinTemplate tmpl;
    memcpy(tmpl.data, template_data, sizeof(template_data));
    tmpl.n_random_words = n_random_words;

    printf("Starting CUDA search with:\n");
    printf("- %u random words\n", n_random_words);
    printf("- %d blocks x %d threads = %d total threads\n",
           BLOCKS_PER_GRID, THREADS_PER_BLOCK, BLOCKS_PER_GRID * THREADS_PER_BLOCK);
    printf("- %d iterations per thread\n", ITERATIONS_PER_THREAD);

    int32_t seed = 0;
    while (!stop_request)
    {
        struct FoundCoins found_coins = {0};
        CU_CALL(cuMemcpyHtoD, (d_found_coins, &found_coins, sizeof(struct FoundCoins)));

        void *kernel_args[] = {&tmpl, &d_found_coins, &seed};

        // Launch kernel
        CU_CALL(cuLaunchKernel, (kernel,
                                 BLOCKS_PER_GRID, 1, 1,
                                 THREADS_PER_BLOCK, 1, 1,
                                 0,
                                 0,
                                 kernel_args,
                                 0));

        CU_CALL(cuCtxSynchronize, ());

        // Get results
        CU_CALL(cuMemcpyDtoH, (&found_coins, d_found_coins, sizeof(struct FoundCoins)));

        // Process found coins
        for (int i = 0; i < found_coins.count && i < COINS_BUFFER_SIZE; i++)
        {
            is_client ? client_save_deti_coin(found_coins.coins[i]) : save_deti_coin(found_coins.coins[i]);
            total_coins++;
        }

        total_attempts += (u64_t)BLOCKS_PER_GRID * THREADS_PER_BLOCK * ITERATIONS_PER_THREAD;

        if (total_attempts % (10000000ul) == 0)
        {
            printf("Progress: %lu attempts, %lu coins (seed: %d)\n",
                   total_attempts, total_coins, seed);
        }

        seed++;
    }

    cuMemFree(d_found_coins);
    terminate_cuda();

    if (!is_client)
    {
        STORE_DETI_COINS();
    }

    printf("deti_coins_cuda_search: %lu DETI coins found in %lu attempts (expected %.2f coins)\n",
           total_coins, total_attempts, (double)total_attempts / (double)(1ul << 32));
}

#endif