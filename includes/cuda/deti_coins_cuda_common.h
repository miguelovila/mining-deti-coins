// includes/cuda/deti_coins_cuda_common.h

#ifndef DETI_COINS_CUDA_COMMON_H
#define DETI_COINS_CUDA_COMMON_H

#define THREADS_PER_BLOCK 512  // Increased from 256
#define MAX_BLOCKS 1024        // Adjusted for better occupancy
#define COINS_BUFFER_SIZE 1024
#define CUDA_ITERATIONS_PER_BATCH 10000000  // Increased batch size

// Structure to store found coins
struct FoundCoins {
    int32_t count;
    uint32_t coins[COINS_BUFFER_SIZE][13];
};

// Device-side structure for coin template
struct CoinTemplate {
    uint32_t data[13];
    int32_t n_random_words;
};

#endif