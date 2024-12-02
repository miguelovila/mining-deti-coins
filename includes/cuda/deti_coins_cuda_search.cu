#include <stdint.h>
#include "../md5.h"

typedef unsigned char u08_t;
typedef unsigned int u32_t;

__device__ __constant__ u32_t initial_state[4] = {
    0x67452301u, 0xEFCDAB89u, 0x98BADCFEu, 0x10325476u
};

__device__ void hash_byte_reverse(u32_t hash[4]) {
    #pragma unroll
    for(int i = 0; i < 4; i++) {
        u32_t val = hash[i];
        hash[i] = ((val & 0xFF) << 24) | 
                  ((val & 0xFF00) << 8) |
                  ((val & 0xFF0000) >> 8) |
                  ((val & 0xFF000000) >> 24);
    }
}

__device__ uint32_t deti_coin_power(u32_t hash[4]) {
    if(hash[3] != 0) {
        return __clz(hash[3]);
    }
    if(hash[2] != 0) {
        return 32 + __clz(hash[2]);
    }
    if(hash[1] != 0) {
        return 64 + __clz(hash[1]);
    }
    if(hash[0] != 0) {
        return 96 + __clz(hash[0]);
    }
    return 128;
}

__device__ void initialize_coin(u32_t* coin, uint32_t tid) {
    u08_t* bytes = (u08_t*)coin;
    
    // Mandatory prefix
    bytes[0] = 'D';
    bytes[1] = 'E';
    bytes[2] = 'T';
    bytes[3] = 'I';
    bytes[4] = ' ';
    bytes[5] = 'c';
    bytes[6] = 'o';
    bytes[7] = 'i';
    bytes[8] = 'n';
    bytes[9] = ' ';
    
    // Fill middle with unique character sequence
    for(int i = 10; i < 51; i++) {
        uint32_t x = tid + i;
        x = ((x >> 16) ^ x) * 0x45d9f3b;
        x = ((x >> 16) ^ x) * 0x45d9f3b;
        x = (x >> 16) ^ x;
        bytes[i] = ' ' + (x % 95);  // ASCII 32-126
    }
    
    // Mandatory termination
    bytes[51] = '\n';
}

__device__ void save_valid_coin(u32_t* storage, u32_t* coin) {
    uint32_t idx = atomicAdd(&storage[0], 13);
    if(idx + 13 <= 1024) {
        #pragma unroll
        for(int i = 0; i < 13; i++) {
            storage[idx + i] = coin[i];
        }
    }
}

extern "C" __global__ void deti_coins_cuda_kernel_search(
    u32_t* storage,
    u32_t n_random_words
) {
    const uint32_t tid = blockIdx.x * blockDim.x + threadIdx.x;
    
    // Local storage
    u32_t coin[13];
    u32_t hash[4], state[4], x[16];
    
    // Initialize coin with template and unique sequence
    initialize_coin(coin, tid);
    
    // MD5 state variables
    u32_t a, b, c, d;
    
    // Initialize state
    #pragma unroll
    for(int i = 0; i < 4; i++) {
        state[i] = initial_state[i];
    }
    
    a = state[0];
    b = state[1];
    c = state[2];
    d = state[3];
    
    // Calculate hash
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
    
    // Byte reverse hash for correct zero counting
    hash_byte_reverse(hash);
    
    // Check number of trailing zeros and save if valid
    if(deti_coin_power(hash) >= 32) {
        save_valid_coin(storage, coin);
    }
}