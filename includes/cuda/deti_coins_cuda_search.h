#if USE_CUDA > 0

#ifndef DETI_COINS_CUDA_SEARCH
#define DETI_COINS_CUDA_SEARCH

#ifndef N_MESSAGES
#define N_MESSAGES 128 * 1024
#endif
#ifndef ARRAY_SIZE
    #define ARRAY_SIZE 1024
#endif

static void init_array(u32_t *a)
{
    a[0] = 1;
    for (int i = 1; i < ARRAY_SIZE; i++)
        a[i] = 0;
}

static void increment(u32_t *v)
{
    *v += 1;
    if ((*v & 0xFF) == 0x7F)
    {
        *v += 0xA1;
        if (((*v >> 8) & 0xFF) == 0x7F)
        {
            *v += 0xA1 << 8;
            if (((*v >> 16) & 0xFF) == 0x7F)
            {
                *v += 0xA1 << 16;
                if (((*v >> 24) & 0xFF) == 0x7F)
                {
                    *v += 0xA1 << 24;
                }
            }
        }
    }
}

static void deti_coins_cuda_search(int n_random_words, bool is_client)
{
    initialize_cuda(0, "deti_coins_cuda_kernel_search.cubin", "deti_coins_cuda", 0, N_MESSAGES * 4u);

    void *params[3];
    u64_t n_attempts, n_coins;
    u32_t a[ARRAY_SIZE];
    u32_t v1 = 0x20202020;
    u32_t v2 = 0x20202020;

    init_array(a);

    CU_CALL(cuMemAlloc, (&device_v1, sizeof(u32_t)));
    CU_CALL(cuMemAlloc, (&device_v2, sizeof(u32_t)));
    CU_CALL(cuMemAlloc, (&device_array, ARRAY_SIZE * sizeof(u32_t)));

    params[0] = &device_array;
    params[1] = &device_v1;
    params[2] = &device_v2;

    for (n_attempts = n_coins = 0ul; stop_request == 0; n_attempts += N_MESSAGES * 95)
    {
        CU_CALL(cuMemcpyHtoD, (device_array, a, sizeof(u32_t) * ARRAY_SIZE));
        CU_CALL(cuMemcpyHtoD, (device_v1, &v1, sizeof(u32_t)));
        CU_CALL(cuMemcpyHtoD, (device_v2, &v2, sizeof(u32_t)));
        CU_CALL(cuLaunchKernel, (cu_kernel, N_MESSAGES / 128u, 1u, 1u, 128u, 1u, 1u, 0u, (CUstream)0, &params[0], NULL));
        CU_CALL(cuStreamSynchronize, (0));
        CU_CALL(cuMemcpyDtoH, (a, device_array, sizeof(u32_t) * ARRAY_SIZE));

        for (int i = 1; i < ARRAY_SIZE - 13; i += 13)
        {
            if (a[i] == 0)
                break;
            u32_t coin[13];
            for (int j = 0; j < 13; j++)
                coin[j] = a[i + j];
            is_client ? client_save_deti_coin(coin) : save_deti_coin(coin);
            n_coins++;
        }

        increment(&v1);
        if (v1 == 0x20202020) increment(&v2);
        init_array(a);
    }

    CU_CALL(cuMemFree, (device_v1));
    CU_CALL(cuMemFree, (device_v2));
    CU_CALL(cuMemFree, (device_array));
    terminate_cuda();

    STORE_DETI_COINS();
    printf("deti_coins_cuda_search: %lu DETI coin%s found in %lu attempt%s (expected %.2f coins)\n", n_coins, (n_coins == 1ul) ? "" : "s", n_attempts, (n_attempts == 1ul) ? "" : "s", (double)n_attempts / (double)(1ul << 32));
}

#endif
#endif