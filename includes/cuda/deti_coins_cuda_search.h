#if USE_CUDA > 0
#ifndef DETI_COINS_CUDA_SEARCH
#define DETI_COINS_CUDA_SEARCH

static void deti_coins_cuda_search(u32_t n_random_words, bool isClient)
{
    const uint32_t BLOCKS = 512;
    const uint32_t THREADS = 256;
    void *params[2];
    u64_t n_attempts = 0, n_coins = 0;

    // Initialize CUDA
    initialize_cuda(0, "deti_coins_cuda_kernel_search.cubin",
                    "deti_coins_cuda_kernel_search", 1024, 0);

    while (!stop_request)
    {
        // Reset storage
        host_data[0] = 1;
        CU_CALL(cuMemcpyHtoD, (device_data, (void *)host_data, sizeof(u32_t)));

        // Launch kernel
        params[0] = &device_data;
        params[1] = &n_random_words;

        CU_CALL(cuLaunchKernel, (cu_kernel,
                                 BLOCKS, 1, 1,  // Grid dimensions
                                 THREADS, 1, 1, // Block dimensions
                                 0,             // Shared memory size
                                 (CUstream)0,   // Stream
                                 params,        // Parameters
                                 NULL));        // Extra

        // Get results
        CU_CALL(cuMemcpyDtoH, ((void *)host_data, device_data,
                               1024 * sizeof(u32_t)));

        // Process found coins
        uint32_t count = (host_data[0] - 1) / 13;
        for (uint32_t i = 0; i < count; i++)
        {
            u32_t *coin = &host_data[1 + i * 13];
            if (isClient)
            {
                client_save_deti_coin(coin);
            }
            else
            {
                save_deti_coin(coin);
            }
            n_coins++;
        }

        n_attempts += BLOCKS * THREADS;

        // Progress report every ~16M attempts
        if ((n_attempts & ((1 << 24) - 1)) == 0)
        {
            printf("Progress: %lu attempts, %lu coins found\n",
                   n_attempts, n_coins);
        }
    }

    if (!isClient)
    {
        STORE_DETI_COINS();
    }

    printf("deti_coins_cuda_search: %lu DETI coins found in %lu attempts "
           "(expected %.2f coins)\n",
           n_coins, n_attempts, (double)n_attempts / (double)(1ul << 32));

    terminate_cuda();
}

#endif
#endif