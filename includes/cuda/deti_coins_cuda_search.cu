#include <stdint.h>
#include "../md5.h"

__device__ void generate_coin(u08_t *bytes, u32_t thread_id) {
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

    // Fill remaining bytes with spaces
    for(int i = 10; i < 51; i++) {
        bytes[i] = ' ';
    }

    // Try different characters based on thread_id
    bytes[10] = ' ' + (thread_id % 95);
    bytes[11] = ' ' + ((thread_id / 95) % 95);
    bytes[12] = ' ' + ((thread_id / (95*95)) % 95);

    // Add newline
    bytes[51] = '\n';
}

__device__ uint32_t swap_bytes(uint32_t value) {
    return ((value & 0xFF) << 24) |
           ((value & 0xFF00) << 8) |
           ((value & 0xFF0000) >> 8) |
           ((value >> 24) & 0xFF);
}

extern "C" __global__ void deti_coins_cuda_kernel_search(u32_t *storage) {
    const u32_t thread_id = blockIdx.x * blockDim.x + threadIdx.x;
    
    u32_t coin[13] = {0};
    u32_t hash[4], state[4], x[16];
    u32_t a, b, c, d;

    // Generate coin
    generate_coin((u08_t*)coin, thread_id);

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

    // Byte-reverse hash
    for(int i = 0; i < 4; i++) {
        hash[i] = swap_bytes(hash[i]);
    }

    // Check trailing zeros in byte-reversed hash
    if(hash[3] == 0) {
        // Save coin if valid
        uint32_t idx = atomicAdd(storage, 13);
        if(idx + 13 < 1024) {
            for(int i = 0; i < 13; i++) {
                storage[idx + i] = coin[i];
            }
        }
    }
}