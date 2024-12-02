// includes/cuda/deti_coins_cuda_search.cu

#include <stdint.h>
#include <stdio.h>
#include <cuda_runtime.h>
#include "../../includes/md5.h"
#include "deti_coins_cuda_common.h"

__device__ void print_debug(const char* prefix, uint32_t* coin, uint32_t* hash, uint32_t power) {
    if(blockIdx.x == 0 && threadIdx.x == 0) {
        printf("%s: ", prefix);
        unsigned char* bytes = (unsigned char*)coin;
        for(int i = 0; i < 52; i++) {
            printf("%c", bytes[i]);
        }
        printf("\nHash: %08x %08x %08x %08x (power: %u)\n", 
               hash[0], hash[1], hash[2], hash[3], power);
    }
}

__device__ uint32_t deti_coin_power_gpu(uint32_t* hash) {
    if(hash[3] != 0u)
        return __clz(hash[3]);
    else if(hash[2] != 0u)
        return 32u + __clz(hash[2]);
    else if(hash[1] != 0u)
        return 64u + __clz(hash[1]);
    else if(hash[0] != 0u)
        return 96u + __clz(hash[0]);
    else
        return 128u;
}

__device__ void increment_search_space(uint32_t* coin, int32_t start_pos, int32_t thread_id) {
    unsigned char* bytes = (unsigned char*)coin;
    for(int32_t i = start_pos; i < 51; i++) {
        if(bytes[i] == '~') {
            bytes[i] = ' ';
        } else {
            bytes[i] = bytes[i] + 1;
            if(bytes[i] > '~') {
                bytes[i] = ' ';
                continue;
            }
            break;
        }
    }
}

extern "C" __global__ void mine_deti_coins_kernel(
    struct CoinTemplate tmpl,
    struct FoundCoins* found_coins,
    int32_t seed
) {
    const int32_t tid = blockIdx.x * blockDim.x + threadIdx.x;
    uint32_t coin[13];
    uint32_t hash[4];
    
    // Copy template
    for(int i = 0; i < 13; i++) {
        coin[i] = tmpl.data[i];
    }
    
    // Initialize search space based on thread ID
    unsigned char* bytes = (unsigned char*)coin;
    int32_t start_pos = 10 + tmpl.n_random_words * 4;
    
    // Set initial value based on thread ID and seed
    int32_t thread_space = tid + seed * blockDim.x * gridDim.x;
    for(int32_t i = start_pos; i < 51; i++) {
        bytes[i] = ' ' + (thread_space % 95);
        thread_space /= 95;
    }
    
    // Debug first thread's initial state
    if(tid == 0) {
        print_debug("Initial", coin, hash, 0);
    }
    
    // Try multiple variations per thread
    for(int32_t iter = 0; iter < ITERATIONS_PER_THREAD; iter++) {
        // Compute MD5 hash
        uint32_t state[4], x[16], a, b, c, d;
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
        
        // Reverse byte order
        for(int i = 0; i < 4; i++) {
            uint32_t v = hash[i];
            hash[i] = __byte_perm(v, 0, 0x0123);
        }
        
        // Debug first few hashes from first thread
        if(tid == 0 && iter < 3) {
            print_debug("Attempt", coin, hash, deti_coin_power_gpu(hash));
        }
        
        // Check if we found a coin
        uint32_t power = deti_coin_power_gpu(hash);
        if(power >= 32) {
            int32_t idx = atomicAdd(&found_coins->count, 1);
            if(idx < COINS_BUFFER_SIZE) {
                for(int i = 0; i < 13; i++) {
                    found_coins->coins[idx][i] = coin[i];
                }
                print_debug("Found coin", coin, hash, power);
            }
        }
        
        // Move to next combination
        increment_search_space(coin, start_pos, tid);
    }
}