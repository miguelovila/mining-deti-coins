// includes/cuda/deti_coins_cuda_search.h

#ifndef DETI_COINS_CUDA_SEARCH_H
#define DETI_COINS_CUDA_SEARCH_H

#include <cuda.h>

#define CUDA_ITERATIONS_PER_BATCH 1000000
#define MAX_FOUND_COINS_PER_BATCH 1024

static void deti_coins_cuda_search(u32_t n_random_words, bool is_client)
{
    CUdeviceptr d_found_coins;
    CUfunction kernel;
    u32_t template_data[13];
    u08_t *bytes = (u08_t *)template_data;
    u64_t total_attempts = 0, total_coins = 0;

    // Initialize template with required prefix
    const u08_t prefix[] = {'D', 'E', 'T', 'I', ' ', 'c', 'o', 'i', 'n', ' '};
    memcpy(bytes, prefix, sizeof(prefix));

    // Fill remaining template with spaces and newline
    for (u32_t i = sizeof(prefix); i < 51; i++)
    {
        bytes[i] = ' ';
    }
    bytes[51] = '\n';

    // Generate initial random content if requested
    if (n_random_words > 0)
    {
        for (u32_t i = 0; i < n_random_words * 4 && i + 10 < 51; i++)
        {
            bytes[10 + i] = ' ' + (random() % 95);
        }
    }

    // Initialize CUDA
    initialize_cuda(0, "deti_coins_cuda_kernel_search.cubin", "mine_deti_coins_kernel", 0, sizeof(struct FoundCoins));

    // Get kernel function
    CU_CALL(cuModuleGetFunction, (&kernel, cu_module, "mine_deti_coins_kernel"));

    // Allocate device memory for results
    CU_CALL(cuMemAlloc, (&d_found_coins, sizeof(struct FoundCoins)));

    // Create coin template structure
    struct CoinTemplate tmpl;
    memcpy(tmpl.data, template_data, sizeof(template_data));
    tmpl.n_random_words = n_random_words;

    printf("Starting CUDA search with %u random words...\n", n_random_words);

    struct FoundCoins found_coins;
    void *args[] = {&tmpl, &d_found_coins, &CUDA_ITERATIONS_PER_BATCH};

    // Main search loop
    while (!stop_request)
    {
        // Reset found coins counter
        found_coins.count = 0;
        CU_CALL(cuMemcpyHtoD, (d_found_coins, &found_coins, sizeof(struct FoundCoins)));

        // Launch kernel
        CU_CALL(cuLaunchKernel, (kernel,
                                 256, 1, 1, // Grid dimensions
                                 256, 1, 1, // Block dimensions
                                 0,         // Shared memory bytes
                                 0,         // Stream
                                 args,      // Arguments
                                 0));       // Size of arguments

        // Copy results back
        CU_CALL(cuMemcpyDtoH, (&found_coins, d_found_coins, sizeof(struct FoundCoins)));

        // Process found coins
        for (int i = 0; i < found_coins.count && i < MAX_FOUND_COINS_PER_BATCH; i++)
        {
            is_client ? client_save_deti_coin(found_coins.coins[i]) : save_deti_coin(found_coins.coins[i]);
            total_coins++;
        }

        total_attempts += CUDA_ITERATIONS_PER_BATCH;

        if (total_attempts % (CUDA_ITERATIONS_PER_BATCH * 100) == 0)
        {
            printf("Progress: %lu attempts, %lu coins found\n", total_attempts, total_coins);
        }
    }

    // Cleanup
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