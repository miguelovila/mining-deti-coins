#include <stdint.h>  
#include "../md5.h"

#define MAX_SIZE 1024
#define TEMPLATE_SIZE 52

typedef uint8_t u08_t;   // Alias for unsigned 8-bit integer
typedef uint32_t u32_t;  // Alias for unsigned 32-bit integer

__constant__ uint8_t template_str[TEMPLATE_SIZE] = {
    'D', 'E', 'T', 'I', ' ', 'c', 'o', 'i', 'n', ' ',
    ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ',
    ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ',
    ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ',
    ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ',
    ' ', '\n'
};

__device__ void build_coin(uint8_t *coin, uint32_t thread_num, uint32_t custom_word_1, uint32_t custom_word_2, uint32_t custom_word_3) {
    for (int i = 0; i < TEMPLATE_SIZE; i++) {
        coin[i] = template_str[i];
    }

    ((uint32_t *)coin)[4] = custom_word_1;
    ((uint32_t *)coin)[5] = custom_word_2;
    ((uint32_t *)coin)[6] = custom_word_3;

    uint32_t n = thread_num;
    ((uint32_t *)coin)[8] = (n % 64) | ((n / 64 % 64) << 8) | ((n / 4096 % 64) << 16) | ((n / 262144) << 24);
}

__device__ void compute_md5(uint32_t *coin, uint32_t *hash, uint32_t *state, uint32_t *x) {
    #define C(c) (c)
    #define ROTATE(x, n) (((x) << (n)) | ((x) >> (32 - (n))))
    #define DATA(idx) coin[idx]
    #define HASH(idx) hash[idx]
    #define STATE(idx) state[idx]
    #define X(idx) x[idx]

    // MD5 state variables
    uint32_t a, b, c, d;
    a = state[0];  
    b = state[1];
    c = state[2];
    d = state[3];

    CUSTOM_MD5_CODE();
}

__device__ void store_coin_if_valid(uint32_t *storage_area, uint32_t *coin, uint32_t *hash) {
        if (hash[3] == 0u) {
            int offset = atomicAdd(&storage_area[0], 13);  // Reserve space atomically
            if (offset + 13 < MAX_SIZE) {
                for (int i = 0; i < 13; i++) {
                    storage_area[offset + i] = coin[i];
                }
            }
        }
}

extern "C" __global__ void deti_coins_cuda_kernel_search(
    uint32_t *deti_coins_storage_area,
    uint32_t custom_word_1,
    uint32_t custom_word_2,
    uint32_t custom_word_3
) {
    __shared__ uint32_t shared_custom_words[2];
    
    const uint32_t thread_num = blockIdx.x * blockDim.x + threadIdx.x;
    
    // Coalesced memory access pattern
    __align__(16) uint8_t coin_buffer[TEMPLATE_SIZE];
    uint32_t hash[4], state[4], x[16];
    
    if (threadIdx.x == 0) {
        shared_custom_words[0] = custom_word_1;
        shared_custom_words[1] = custom_word_2;
    }
    __syncthreads();
    
    #pragma unroll
    for (int att = 0; att < 95; att++) {
        build_coin(coin_buffer, thread_num, 
                   shared_custom_words[0], 
                   shared_custom_words[1], 
                   custom_word_3);

        *((uint32_t*)coin_buffer + 8) = thread_num + att * gridDim.x * blockDim.x;
        
        compute_md5((uint32_t *)coin_buffer, hash, state, x);
        
        // hash validation
        store_coin_if_valid(deti_coins_storage_area, 
                            (uint32_t *)coin_buffer, 
                            hash);
    }
}