// includes/cuda/deti_coins_cuda_search.h

#ifndef DETI_COINS_CUDA_SEARCH
#define DETI_COINS_CUDA_SEARCH

#include <cuda.h>
#include "deti_coins_cuda_common.h"

#define CUDA_ITERATIONS_PER_BATCH 1000 // Reduced for debugging

static void deti_coins_cuda_search(u32_t n_random_words, bool is_client)
{
    CUdeviceptr d_found_coins;
    CUfunction kernel;
    u32_t template_data[13];
    u08_t *bytes = (u08_t *)template_data;
    u64_t total_attempts = 0, total_coins = 0;
    int32_t iterations_per_thread = CUDA_ITERATIONS_PER_BATCH / (THREADS_PER_BLOCK * MAX_BLOCKS);

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

    printf("Initial template:\n");
    for (int i = 0; i < 52; i++)
    {
        printf("%c", bytes[i]);
    }
    printf("\n");

    // Calculate MD5 of template on CPU for comparison
    u32_t cpu_hash[4];
    md5_cpu(template_data, cpu_hash);
    hash_byte_reverse(cpu_hash);
    printf("CPU hash of template: %08x %08x %08x %08x\n",
           cpu_hash[0], cpu_hash[1], cpu_hash[2], cpu_hash[3]);
    printf("CPU power: %u\n", deti_coin_power(cpu_hash));

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
    printf("Using %d blocks with %d threads per block\n", THREADS_PER_BLOCK, MAX_BLOCKS);
    printf("Iterations per thread: %d\n", iterations_per_thread);

    // Main search loop
    while (!stop_request)
    {
        struct FoundCoins found_coins = {0}; // Initialize with zero count

        // Reset found coins counter
        CU_CALL(cuMemcpyHtoD, (d_found_coins, &found_coins, sizeof(struct FoundCoins)));

        void *kernel_args[] = {&tmpl, &d_found_coins, &iterations_per_thread};

        // Launch kernel
        CU_CALL(cuLaunchKernel, (kernel,
                                 16, 1, 1, // Reduced grid dimensions for debug
                                 THREADS_PER_BLOCK, 1, 1,
                                 0,
                                 0,
                                 kernel_args,
                                 0));

        CU_CALL(cuCtxSynchronize, ());

        // Copy results back
        CU_CALL(cuMemcpyDtoH, (&found_coins, d_found_coins, sizeof(struct FoundCoins)));

        // Process found coins
        for (int i = 0; i < found_coins.count && i < COINS_BUFFER_SIZE; i++)
        {
            is_client ? client_save_deti_coin(found_coins.coins[i]) : save_deti_coin(found_coins.coins[i]);
            total_coins++;
        }

        total_attempts += CUDA_ITERATIONS_PER_BATCH;
        printf("Completed batch - attempts: %lu, coins: %lu\n", total_attempts, total_coins);

        if (total_attempts > 10000)
            break; // Exit after some attempts for debugging
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