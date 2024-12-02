#include <cuda_runtime.h>
#include "../md5.h"
#include <stdio.h>

// Constants for CUDA implementation
#define THREADS_PER_BLOCK 256
#define MAX_BLOCKS 65535
#define COINS_BUFFER_SIZE 1024

// Structure to store found coins
typedef struct {
    uint32_t count;
    uint32_t coins[COINS_BUFFER_SIZE][13];
} FoundCoins;

// Device-side structure for coin template
typedef struct {
    uint32_t data[13];
    uint32_t n_random_words;
} CoinTemplate;

// Initialize coin template with pattern
__device__ void init_coin_data(uint32_t* coin, const CoinTemplate* tmpl, uint32_t thread_id) {
    // Copy template
    for(int i = 0; i < 13; i++) {
        coin[i] = tmpl->data[i];
    }
    
    // Modify search space based on thread ID
    uint8_t* bytes = (uint8_t*)coin;
    uint32_t start_pos = 10 + tmpl->n_random_words * 4;
    
    for(uint32_t i = 0; i < 4; i++) {
        uint32_t pos = start_pos + i;
        if(pos < 51) {
            bytes[pos] = ' ' + ((thread_id + i) % 95);
        }
    }
}

// CUDA kernel for mining DETI coins
__global__ void mine_deti_coins_kernel(
    CoinTemplate tmpl,
    FoundCoins* found_coins,
    uint32_t iterations_per_thread
) {
    uint32_t tid = blockIdx.x * blockDim.x + threadIdx.x;
    uint32_t coin[13];
    uint32_t hash[4];
    
    // Initialize coin data for this thread
    init_coin_data(coin, &tmpl, tid);
    
    // Main mining loop
    for(uint32_t iter = 0; iter < iterations_per_thread; iter++) {
        // Compute MD5 hash
        #define C(c) (c)
        #define ROTATE(x,n) (((x) << (n)) | ((x) >> (32 - (n))))
        #define DATA(idx) coin[idx]
        #define HASH(idx) hash[idx]
        #define STATE(idx) state[idx]
        #define X(idx) x[idx]
        uint32_t state[4], x[16], a, b, c, d;
        CUSTOM_MD5_CODE();
        #undef C
        #undef ROTATE
        #undef DATA
        #undef HASH
        #undef STATE
        #undef X
        
        // Reverse byte order
        for(int i = 0; i < 4; i++) {
            hash[i] = __byte_perm(hash[i], 0, 0x0123);
        }
        
        // Check if we found a coin
        if(hash[3] == 0) {
            uint32_t idx = atomicAdd(&found_coins->count, 1);
            if(idx < COINS_BUFFER_SIZE) {
                // Save the coin
                for(int i = 0; i < 13; i++) {
                    found_coins->coins[idx][i] = coin[i];
                }
            }
        }
        
        // Update search space
        uint8_t* bytes = (uint8_t*)coin;
        bool carry = true;
        for(uint32_t i = 10 + tmpl.n_random_words * 4; carry && i < 51; i++) {
            if(bytes[i] == '~') {
                bytes[i] = ' ';
            } else {
                bytes[i]++;
                carry = false;
            }
        }
        
        // If we exhausted the search space, generate new content
        if(carry) {
            init_coin_data(coin, &tmpl, tid + iter * gridDim.x * blockDim.x);
        }
    }
}

// Host-side wrapper for CUDA mining
extern "C" void mine_deti_coins_cuda(
    uint32_t* template_data,
    uint32_t n_random_words,
    uint32_t* found_coins_data,
    uint32_t* found_coins_count,
    uint32_t iterations
) {
    // Allocate device memory
    CoinTemplate tmpl;
    memcpy(tmpl.data, template_data, sizeof(uint32_t) * 13);
    tmpl.n_random_words = n_random_words;
    
    FoundCoins* d_found_coins;
    cudaMalloc(&d_found_coins, sizeof(FoundCoins));
    cudaMemset(d_found_coins, 0, sizeof(FoundCoins));
    
    // Calculate grid dimensions
    dim3 block(THREADS_PER_BLOCK);
    dim3 grid(min(MAX_BLOCKS, (iterations + block.x - 1) / block.x));
    uint32_t iterations_per_thread = (iterations + grid.x * block.x - 1) / (grid.x * block.x);
    
    // Launch kernel
    mine_deti_coins_kernel<<<grid, block>>>(tmpl, d_found_coins, iterations_per_thread);
    
    // Copy results back
    FoundCoins h_found_coins;
    cudaMemcpy(&h_found_coins, d_found_coins, sizeof(FoundCoins), cudaMemcpyDeviceToHost);
    
    // Copy found coins to output buffer
    *found_coins_count = min(h_found_coins.count, (uint32_t)COINS_BUFFER_SIZE);
    memcpy(found_coins_data, h_found_coins.coins, sizeof(uint32_t) * 13 * *found_coins_count);
    
    // Cleanup
    cudaFree(d_found_coins);
}