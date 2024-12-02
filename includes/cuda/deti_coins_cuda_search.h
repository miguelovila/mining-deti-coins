#if USE_CUDA > 0
#ifndef DETI_COINS_CUDA_SEARCH
#define DETI_COINS_CUDA_SEARCH

static void deti_coins_cuda_search(u32_t n_random_words, bool isClient)
{
    const uint32_t BLOCKS = 1024;
    const uint32_t THREADS_PER_BLOCK = 256;
    void *params[2];
    u64_t n_attempts = 0, n_coins = 0;

    // Initialize CUDA
    initialize_cuda(0, "deti_coins_cuda_kernel_search.cubin", "deti_coins_cuda_kernel_search",
                    1024, 0); // Buffer for results

    while (!stop_request)
    {
        // Reset result counter
        host_data[0] = 1; // Start at 1 to skip counter
        CU_CALL(cuMemcpyHtoD, (device_data, (void *)host_data, sizeof(u32_t)));

        // Launch kernel
        params[0] = &device_data;
        params[1] = &n_random_words;

        CU_CALL(cuLaunchKernel, (cu_kernel,
                                 BLOCKS,            // Grid X
                                 1, 1,              // Grid Y,Z
                                 THREADS_PER_BLOCK, // Block X
                                 1, 1,              // Block Y,Z
                                 0,                 // Shared memory
                                 (CUstream)0,       // Stream
                                 params,
                                 NULL));

        // Retrieve results
        CU_CALL(cuMemcpyDtoH, ((void *)host_data, device_data, 1024 * sizeof(u32_t)));

        // Process found coins
        uint32_t offset = 1;
        while (offset < 1024 && offset < host_data[0])
        {
            if (isClient)
            {
                client_save_deti_coin(&host_data[offset]);
            }
            else
            {
                save_deti_coin(&host_data[offset]);
            }
            offset += 13;
            n_coins++;
        }

        n_attempts += BLOCKS * THREADS_PER_BLOCK;

        if (n_attempts % (1ULL << 24) == 0)
        { // Progress report every 16M attempts
            printf("Progress: %lu attempts, %lu coins found\n", n_attempts, n_coins);
        }
    }

    if (!isClient)
    {
        STORE_DETI_COINS();
    }

    printf("deti_coins_cuda_search: %lu DETI coins found in %lu attempts (expected %.2f coins)\n",
           n_coins, n_attempts, (double)n_attempts / (double)(1ul << 32));

    terminate_cuda();
}

#endif
#endif