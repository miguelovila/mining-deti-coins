#include <stdint.h>
#include "../md5.h"

typedef unsigned char u08_t;
typedef unsigned int u32_t;

__device__ __constant__ u08_t coin_template[10] = {
    'D', 'E', 'T', 'I', ' ', 'c', 'o', 'i', 'n', ' '
};

__device__ void init_coin(u08_t *bytes) {
    // Initialize with template
    for(int i = 0; i < 10; i++) {
        bytes[i] = coin_template[i];
    }
    // Fill with spaces
    for(int i = 10; i < 51; i++) {
        bytes[i] = ' ';
    }
    // Add newline
    bytes[51] = '\n';
}

__device__ u32_t byte_swap(u32_t value) {
    value = ((value << 8) & 0xFF00FF00) | ((value >> 8) & 0xFF00FF);
    return (value << 16) | (value >> 16);
}

__device__ void save_valid_coin(u32_t *storage, u32_t *coin, u32_t *hash) {
    // Byte swap hash values for proper validation
    for(int i = 0; i < 4; i++) {
        hash[i] = byte_swap(hash[i]);
    }
    
    // Validate trailing zeros
    if(hash[3] == 0) {
        u32_t idx = atomicAdd(&storage[0], 13);
        if(idx + 13 <= 1024) {
            for(int i = 0; i < 13; i++) {
                storage[idx + i] = coin[i];
            }
        }
    }
}

extern "C" __global__ void deti_coins_cuda_kernel_search(
    u32_t *storage,
    u32_t n_random_words
) {
    const u32_t tid = blockDim.x * blockIdx.x + threadIdx.x;
    __shared__ u32_t shared_coin[13];
    u32_t hash[4], state[4], x[16];
    u08_t *bytes = (u08_t *)shared_coin;
    
    if (threadIdx.x == 0) {
        init_coin(bytes);
    }
    __syncthreads();
    
    // Copy to local array
    u32_t coin[13];
    for(int i = 0; i < 13; i++) {
        coin[i] = shared_coin[i];
    }
    bytes = (u08_t *)coin;
    
    // Add randomization based on thread ID
    u32_t seed = tid;
    for(u32_t i = 0; i < n_random_words * 4 && i + 10 < 51; i++) {
        seed = seed * 1664525u + 1013904223u;
        bytes[10 + i] = ' ' + (seed % 95);
    }
    
    // Initialize MD5 state
    state[0] = 0x67452301u;
    state[1] = 0xEFCDAB89u;
    state[2] = 0x98BADCFEu;
    state[3] = 0x10325476u;
    
    // MD5 variables
    u32_t a = state[0];
    u32_t b = state[1];
    u32_t c = state[2];
    u32_t d = state[3];
    
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
    
    save_valid_coin(storage, coin, hash);
}