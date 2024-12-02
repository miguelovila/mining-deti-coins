#include <stdint.h>
#include "../md5.h"

typedef unsigned char u08_t;
typedef unsigned int u32_t;

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

__device__ uint32_t count_trailing_zeros(u32_t hash[4]) {
    for(int i = 3; i >= 0; i--) {
        if(hash[i] != 0) {
            // Count trailing zeros in this word
            uint32_t val = hash[i];
            uint32_t count = 0;
            while((val & 1) == 0 && count < 32) {
                count++;
                val >>= 1;
            }
            return i * 32 + count;
        }
    }
    return 128;
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
    
    u32_t coin[13] = {0};  // Initialize all to 0
    u32_t hash[4], state[4], x[16];
    u08_t *bytes = (u08_t *)coin;
    
    // Initialize coin template
    init_coin_template(bytes);
    
    // Generate unique sequence for this thread
    u32_t seed = tid;
    for(u32_t i = 0; i < n_random_words * 4 && i + 10 < 51; i++) {
        seed = ((seed * 1664525u + 1013904223u) & 0xffffffffu);
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
    
    // Reverse bytes in each hash word
    for(int i = 0; i < 4; i++) {
        hash[i] = __byte_perm(hash[i], 0, 0x0123);
    }
    
    // Check for 32+ trailing zeros
    uint32_t zeros = count_trailing_zeros(hash);
    if(zeros >= 32) {
        save_valid_coin(storage, coin);
    }
}