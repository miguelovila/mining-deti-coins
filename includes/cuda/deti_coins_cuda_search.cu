#include <stdint.h>
#include "../md5.h"

typedef unsigned char u08_t;
typedef unsigned int u32_t;

// The template string length is 10
__device__ __constant__ u08_t coin_template[10] = {
    'D', 'E', 'T', 'I', ' ', 'c', 'o', 'i', 'n', ' '
};

__device__ void init_coin_template(u08_t *bytes) {
    // Copy the template
    for(int i = 0; i < 10; i++) {
        bytes[i] = coin_template[i];
    }
    
    // Fill rest with spaces
    for(int i = 10; i < 51; i++) {
        bytes[i] = ' ';
    }
    
    // Add newline at end
    bytes[51] = '\n';
}

__device__ void save_valid_coin(u32_t *storage, u32_t *coin) {
    u32_t idx = atomicAdd(&storage[0], 13); // Reserve space for one coin
    if(idx + 13 <= 1024) { // Ensure we don't overflow
        // Copy the coin data
        for(int i = 0; i < 13; i++) {
            storage[idx + i] = coin[i];
        }
    }
}

extern "C" __global__ void deti_coins_cuda_kernel_search(
    u32_t *storage,
    u32_t n_random_words
) {
    const u32_t tid = blockDim.x * blockIdx.x + threadIdx.x;
    const u32_t start_pos = 10 + n_random_words * 4;
    
    u32_t coin[13];
    u32_t hash[4], state[4], x[16];
    u08_t *bytes = (u08_t *)coin;
    
    // Initialize coin template
    init_coin_template(bytes);
    
    // Add some randomness based on thread ID
    u32_t seed = tid;
    for(u32_t i = 0; i < n_random_words * 4 && i + 10 < 51; i++) {
        seed = (seed * 1664525u + 1013904223u);
        bytes[10 + i] = ' ' + (seed % 95); // ASCII 32-126
    }
    
    // MD5 state variables
    u32_t a, b, c, d;
    
    // Initialize state
    state[0] = 0x67452301u;
    state[1] = 0xEFCDAB89u;
    state[2] = 0x98BADCFEu;
    state[3] = 0x10325476u;
    
    a = state[0];
    b = state[1];
    c = state[2];
    d = state[3];
    
    // Calculate MD5 hash
    #define C(c) (c)
    #define ROTATE(x,n) (((x) << (n)) | ((x) >> (32 - (n))))
    #define DATA(idx) coin[idx]
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
    
    // Byte-reverse hash words
    hash[0] = __byte_perm(hash[0], 0, 0x0123);
    hash[1] = __byte_perm(hash[1], 0, 0x0123);
    hash[2] = __byte_perm(hash[2], 0, 0x0123);
    hash[3] = __byte_perm(hash[3], 0, 0x0123);
    
    // Check trailing zeros
    if(hash[3] == 0) {
        save_valid_coin(storage, coin);
    }
}