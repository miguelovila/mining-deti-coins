#ifndef DETI_COINS_CUDA_COMMON
#define DETI_COINS_CUDA_COMMON

#define THREADS_PER_BLOCK 256    // Good size for most GPUs
#define BLOCKS_PER_GRID 1024     // Increased for better parallelization
#define COINS_BUFFER_SIZE 1024   // Plenty of space for found coins
#define ITERATIONS_PER_THREAD 10000  // More iterations per kernel launch

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