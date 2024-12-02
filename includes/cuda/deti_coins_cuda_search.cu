// includes/cuda/deti_coins_cuda_search.cu

#include <stdint.h>
#include <cuda_runtime.h>
#include "../../includes/md5.h"
#include "deti_coins_cuda_common.h"

// Helper function to count trailing zeros
__device__ uint32_t count_trailing_zeros(uint32_t hash[4]) {
    uint32_t n;
    
    if(hash[3] != 0)
        n = __clz(__brev(hash[3]));  // Count leading zeros after bit reversal
    else if(hash[2] != 0)
        n = 32 + __clz(__brev(hash[2]));
    else if(hash[1] != 0)
        n = 64 + __clz(__brev(hash[1]));
    else if(hash[0] != 0)
        n = 96 + __clz(__brev(hash[0]));
    else
        n = 128;
    return n;
}

__device__ void init_coin_data(uint32_t* coin, const struct CoinTemplate* tmpl, int32_t thread_id) {
    // Copy template
    for(int i = 0; i < 13; i++) {
        coin[i] = tmpl->data[i];
    }
    
    // Modify search space based on thread ID
    unsigned char* bytes = (unsigned char*)coin;
    int32_t start_pos = 10 + tmpl->n_random_words * 4;
    
    for(int32_t i = 0; i < 4; i++) {
        int32_t pos = start_pos + i;
        if(pos < 51) {
            bytes[pos] = ' ' + ((thread_id + i) % 95);
        }
    }
}

extern "C" __global__ void mine_deti_coins_kernel(
    struct CoinTemplate tmpl,
    struct FoundCoins* found_coins,
    int32_t iterations_per_thread
) {
    const int32_t tid = blockIdx.x * blockDim.x + threadIdx.x;
    uint32_t coin[13];
    uint32_t hash[4];
    uint32_t state[4], x[16], a, b, c, d;
    
    init_coin_data(coin, &tmpl, tid);
    
    for(int32_t iter = 0; iter < iterations_per_thread; iter++) {
        // Compute MD5 hash
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
        
        // Byte-reverse each word to match CPU implementation
        for(int i = 0; i < 4; i++) {
            hash[i] = __byte_perm(hash[i], 0, 0x0123);
        }
        
        // Check if we found a coin (>= 32 trailing zeros)
        if(count_trailing_zeros(hash) >= 32) {
            int32_t idx = atomicAdd(&found_coins->count, 1);
            if(idx < COINS_BUFFER_SIZE) {
                for(int i = 0; i < 13; i++) {
                    found_coins->coins[idx][i] = coin[i];
                }
            }
        }
        
        // Update search space
        unsigned char* bytes = (unsigned char*)coin;
        bool carry = true;
        for(int32_t i = 10 + tmpl.n_random_words * 4; carry && i < 51; i++) {
            if(bytes[i] == '~') {
                bytes[i] = ' ';
            } else {
                bytes[i]++;
                carry = false;
            }
        }
        
        if(carry) {
            init_coin_data(coin, &tmpl, tid + iter * gridDim.x * blockDim.x);
        }
    }
}