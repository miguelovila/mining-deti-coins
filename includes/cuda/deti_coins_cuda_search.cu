#include <stdint.h>
#include "../md5.h"

#define MAX_COINS 50
#define COIN_SIZE 52

typedef uint8_t u08_t;
typedef uint32_t u32_t;

// Constant memory for the template
__constant__ char template_str[10] = {'D', 'E', 'T', 'I', ' ', 'c', 'o', 'i', 'n', ' '};

__device__ void generate_printable_string(char* str, uint32_t idx, uint32_t n_random_words) {
    // Copy template
    for (int i = 0; i < 10; i++) {
        str[i] = template_str[i];
    }
    
    // Fill middle section with printable characters
    uint32_t rand_val = idx;
    for (int i = 10; i < 51; i++) {
        // Use thread ID and position to generate pseudo-random printable characters
        rand_val = ((rand_val << 13) ^ rand_val) + i;
        str[i] = ' ' + (rand_val % 95); // ASCII 32-126 (printable range)
    }
    
    // Ensure newline at the end
    str[51] = '\n';
}

__device__ uint32_t count_trailing_zeros(uint32_t hash[4]) {
    for (int i = 3; i >= 0; i--) {
        if (hash[i] != 0) {
            return i * 32 + __clz(__brev(hash[i]));
        }
    }
    return 128;
}

__device__ void atomic_store_coin(uint32_t* storage, const char* coin_str, const uint32_t* hash) {
    uint32_t idx = atomicAdd(storage, 1);
    if (idx < MAX_COINS) {
        uint32_t base_offset = 1 + idx * 13; // Skip counter, each coin takes 13 words
        
        // Store coin data (52 bytes / 13 words)
        uint32_t* dst = storage + base_offset;
        const uint32_t* src = (const uint32_t*)coin_str;
        for (int i = 0; i < 13; i++) {
            dst[i] = src[i];
        }
    }
}

extern "C" __global__ void deti_coins_cuda_kernel_search(
    uint32_t* storage,
    uint32_t n_random_words
) {
    const uint32_t tid = blockIdx.x * blockDim.x + threadIdx.x;
    
    char coin[COIN_SIZE];
    uint32_t hash[4], state[4], x[16];
    
    // Generate unique coin attempt based on thread ID
    generate_printable_string(coin, tid, n_random_words);
    
    // MD5 state variables - these need to be declared before CUSTOM_MD5_CODE()
    uint32_t a, b, c, d;
    
    // Initialize state
    state[0] = 0x67452301u;
    state[1] = 0xEFCDAB89u;
    state[2] = 0x98BADCFEu;
    state[3] = 0x10325476u;
    
    // Initialize variables from state
    a = state[0];
    b = state[1];
    c = state[2];
    d = state[3];
    
    // Calculate MD5 hash
    #define C(c) (c)
    #define ROTATE(x,n) (((x) << (n)) | ((x) >> (32 - (n))))
    #define DATA(idx) ((uint32_t*)coin)[idx]
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
    
    // Check if it's a valid DETI coin (32+ trailing zeros)
    if (count_trailing_zeros(hash) >= 32) {
        atomic_store_coin(storage, coin, hash);
    }
}
