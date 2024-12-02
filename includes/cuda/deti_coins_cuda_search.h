#if USE_CUDA > 0
#ifndef DETI_COINS_CUDA_SEARCH
#define DETI_COINS_CUDA_SEARCH

static void deti_coins_cuda_search(u32_t n_random_words, bool isClient)
{
    void *params[1];
    u64_t n_attempts = 0, n_coins = 0;

    // Initialize CUDA
    initialize_cuda(0, "deti_coins_cuda_kernel_search.cubin", "deti_coins_cuda_kernel_search", 1024, 0);

    while (!stop_request)
    {
        // Reset storage
        host_data[0] = 1;
        CU_CALL(cuMemcpyHtoD, (device_data, host_data, sizeof(u32_t)));

        // Launch kernel
        params[0] = &device_data;
        CU_CALL(cuLaunchKernel, (cu_kernel,
                                 256, 1, 1, // Grid
                                 256, 1, 1, // Block
                                 0,         // Shared memory
                                 (CUstream)0,
                                 params,
                                 NULL));

        // Get results
        CU_CALL(cuMemcpyDtoH, (host_data, device_data, 1024 * sizeof(u32_t)));

        // Process coins
        uint32_t offset = 1;
        while (offset < host_data[0] && offset < 1024)
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

        n_attempts += 256 * 256;

        // Progress report
        if (n_attempts % (1ULL << 24) == 0)
        {
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