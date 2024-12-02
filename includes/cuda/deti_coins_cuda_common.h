// includes/cuda/deti_coins_cuda_common.h

#ifndef DETI_COINS_CUDA_COMMON_H
#define DETI_COINS_CUDA_COMMON_H

#define THREADS_PER_BLOCK 256
#define MAX_BLOCKS 65535
#define COINS_BUFFER_SIZE 1024

// Structure to store found coins
struct FoundCoins
{
    int32_t count;
    uint32_t coins[COINS_BUFFER_SIZE][13];
};

// Device-side structure for coin template
struct CoinTemplate
{
    uint32_t data[13];
    int32_t n_random_words;
};

#endif