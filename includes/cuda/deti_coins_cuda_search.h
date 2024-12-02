
#ifndef DETI_COINS_CUDA_SEARCH
#define DETI_COINS_CUDA_SEARCH

#include <cuda_runtime.h>

static void check_cuda_error(cudaError_t err, const char *msg)
{
    if (err != cudaSuccess)
    {
        fprintf(stderr, "CUDA Error: %s: %s\n", msg, cudaGetErrorString(err));
        exit(1);
    }
}

static void deti_coins_cuda_search(u32_t n_random_words, bool is_client)
{
    CoinBuffer *d_coin_buffer;
    CoinBuffer *h_coin_buffer;
    cudaError_t err;

    // Allocate host memory
    h_coin_buffer = (CoinBuffer *)malloc(sizeof(CoinBuffer) + MAX_COINS_BUFFER * 13 * sizeof(uint32_t));
    if (h_coin_buffer == NULL)
    {
        fprintf(stderr, "Failed to allocate host memory\n");
        exit(1);
    }

    // Allocate device memory
    err = cudaMalloc(&d_coin_buffer, sizeof(CoinBuffer) + MAX_COINS_BUFFER * 13 * sizeof(uint32_t));
    check_cuda_error(err, "Failed to allocate device memory");

    // Calculate grid dimensions
    int device;
    cudaDeviceProp prop;
    err = cudaGetDevice(&device);
    check_cuda_error(err, "Failed to get device");
    err = cudaGetDeviceProperties(&prop, device);
    check_cuda_error(err, "Failed to get device properties");

    const int blocks = min(MAX_BLOCKS, (prop.maxThreadsPerMultiProcessor * prop.multiProcessorCount) / THREADS_PER_BLOCK);

    printf("CUDA search starting with %d blocks, %d threads per block\n", blocks, THREADS_PER_BLOCK);

    u64_t total_attempts = 0ul;
    u64_t total_coins = 0ul;

    while (!stop_request)
    {
        // Reset coin buffer
        h_coin_buffer->count = 0;
        err = cudaMemcpy(d_coin_buffer, h_coin_buffer, sizeof(CoinBuffer), cudaMemcpyHostToDevice);
        check_cuda_error(err, "Failed to copy coin buffer to device");

        // Launch kernel
        deti_coins_cuda_kernel<<<blocks, THREADS_PER_BLOCK>>>(d_coin_buffer, n_random_words);
        err = cudaGetLastError();
        check_cuda_error(err, "Kernel launch failed");

        // Wait for kernel to finish
        err = cudaDeviceSynchronize();
        check_cuda_error(err, "Kernel synchronization failed");

        // Copy results back
        err = cudaMemcpy(h_coin_buffer, d_coin_buffer, sizeof(CoinBuffer) + h_coin_buffer->count * 13 * sizeof(uint32_t), cudaMemcpyDeviceToHost);
        check_cuda_error(err, "Failed to copy results from device");

        // Process found coins
        for (uint32_t i = 0; i < h_coin_buffer->count && i < MAX_COINS_BUFFER; i++)
        {
            uint32_t coin[13];
            memcpy(coin, &h_coin_buffer->coin_data[i * 13], 13 * sizeof(uint32_t));
            is_client ? client_save_deti_coin(coin) : save_deti_coin(coin);
            total_coins++;
        }

        total_attempts += (u64_t)blocks * THREADS_PER_BLOCK * 1000; // 1000 is max_attempts per thread
    }

    // Clean up
    cudaFree(d_coin_buffer);
    free(h_coin_buffer);

    if (!is_client)
    {
        STORE_DETI_COINS();
    }

    printf("deti_coins_cuda_search: %lu DETI coin%s found in %lu attempt%s (expected %.2f coins)\n",
           total_coins, (total_coins == 1ul) ? "" : "s",
           total_attempts, (total_attempts == 1ul) ? "" : "s",
           (double)total_attempts / (double)(1ul << 32));
}

#endif