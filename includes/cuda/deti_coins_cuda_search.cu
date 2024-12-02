#include <stdint.h>
#include "../md5.h"

#define MAX_COINS 50
#define COIN_SIZE 52

typedef uint8_t u08_t;
typedef uint32_t u32_t;

// Template string stored as bytes
__device__ __constant__ u08_t template_bytes[] = {
    'D', 'E', 'T', 'I', ' ', 'c', 'o', 'i', 'n', ' '
};

// Align coin structure to 4 bytes
struct __align__(4) CoinData {
    u08_t data[52];
};

__device__ void init_coin(CoinData* coin) {
    // Copy template
    for (int i = 0; i < 10; i++) {
        coin->data[i] = template_bytes[i];
    }
    
    // Fill with spaces
    for (int i = 10; i < 51; i++) {
        coin->data[i] = ' ';
    }
    
    // Add newline
    coin->data[51] = '\n';
}

__device__ uint32_t count_trailing_zeros(u32_t hash[4]) {
    for (int i = 3; i >= 0; i--) {
        if (hash[i] != 0) {
            return i * 32 + __clz(__brev(hash[i]));
        }
    }
    return 128;
}

__device__ void store_coin(u32_t* storage, const CoinData* coin) {
    u32_t idx = atomicAdd(storage, 1);
    if (idx < MAX_COINS) {
        u32_t base_idx = 1 + idx * 13;
        const u32_t* src = (const u32_t*)coin->data;
        
        #pragma unroll
        for (int i = 0; i < 13; i++) {
            storage[base_idx + i] = src[i];
        }
    }
}

extern "C" __global__ void deti_coins_cuda_kernel_search(
    u32_t* storage,
    u32_t n_random_words
) {
    const u32_t tid = blockIdx.x * blockDim.x + threadIdx.x;
    
    // Aligned coin data
    __shared__ CoinData coins[32];
    CoinData* coin = &coins[threadIdx.x % 32];
    
    // Initialize coin with template
    init_coin(coin);
    
    // Add some randomness based on thread ID
    u32_t rand = tid;
    for (int i = 10; i < 51; i++) {
        rand = ((rand << 13) ^ rand) * 0x2fd;
        coin->data[i] = ' ' + (rand % 95);
    }
    
    // MD5 variables
    u32_t hash[4], state[4], x[16];
    u32_t a, b, c, d;
    
    // Initialize MD5 state
    state[0] = 0x67452301u;
    state[1] = 0xEFCDAB89u;
    state[2] = 0x98BADCFEu;
    state[3] = 0x10325476u;
    
    a = state[0];
    b = state[1];
    c = state[2];
    d = state[3];
    
    // Calculate hash
    #define C(c) (c)
    #define ROTATE(x,n) (((x) << (n)) | ((x) >> (32 - (n))))
    #define DATA(idx) ((u32_t*)coin->data)[idx]
    #define HASH(idx) hash[idx]
    #define STATE(idx) state[idx]
    #define X(idx) x[idx]
    
    CUSTOM_MD5_CODE();
    
    #undef C
    #undef ROTATE
    #undef DATA
    #undef HASH
    #undef STATE
    #undef X
    
    // Check if valid and store
    if (count_trailing_zeros(hash) >= 32) {
        store_coin(storage, coin);
    }
}