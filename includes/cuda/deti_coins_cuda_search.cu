// includes/cuda/deti_coins_cuda_search.cu

#include <stdint.h>
#include <stdio.h>
#include <cuda_runtime.h>
#include "../../includes/md5.h"
#include "deti_coins_cuda_common.h"

__device__ uint32_t count_trailing_zeros_gpu(uint32_t* hash) {
    // Start counting from least significant bits (hash[3])
    if(hash[3] == 0) {
        if(hash[2] == 0) {
            if(hash[1] == 0) {
                if(hash[0] == 0) {
                    return 128;
                }
                return 96 + __clz(__brev(hash[0]));
            }
            return 64 + __clz(__brev(hash[1]));
        }
        return 32 + __clz(__brev(hash[2]));
    }
    return __clz(__brev(hash[3]));
}

__device__ void verify_hash(uint32_t* coin, uint32_t* hash, uint32_t power) {
    if(threadIdx.x == 0 && blockIdx.x == 0) {
        printf("\nPOSSIBLE COIN FOUND!\n");
        unsigned char* bytes = (unsigned char*)coin;
        printf("Coin data: ");
        for(int i = 0; i < 52; i++) {
            printf("%c", bytes[i]);
        }
        printf("\nHash: %08x %08x %08x %08x\n", hash[0], hash[1], hash[2], hash[3]);
        printf("Power calculated: %u\n\n", power);
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
    
    // Try multiple search space variations
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
        
        // Reverse byte order of each word
        for(int i = 0; i < 4; i++) {
            uint32_t v = hash[i];
            hash[i] = ((v & 0xff000000) >> 24) |
                     ((v & 0x00ff0000) >> 8)  |
                     ((v & 0x0000ff00) << 8)  |
                     ((v & 0x000000ff) << 24);
        }
        
        // Check if we found a coin
        uint32_t power = count_trailing_zeros_gpu(hash);
        
        // Debug output for first thread of first block
        if(tid == 0 && iter < 5) {
            verify_hash(coin, hash, power);
        }
        
        if(power >= 32) {
            int32_t idx = atomicAdd(&found_coins->count, 1);
            if(idx < COINS_BUFFER_SIZE) {
                // Save the coin
                for(int i = 0; i < 13; i++) {
                    found_coins->coins[idx][i] = coin[i];
                }
                verify_hash(coin, hash, power);
            }
        }
        
        // Move to next search space variation
        bool carry = true;
        for(int32_t i = start_pos; carry && i < 51; i++) {
            bytes[i]++;
            if(bytes[i] > '~') {
                bytes[i] = ' ';
            } else {
                carry = false;
            }
        }
        
        // If we've exhausted this search space, move to a new area
        if(carry) {
            thread_space = tid + (seed + iter + 1) * blockDim.x * gridDim.x;
            for(int32_t i = start_pos; i < 51; i++) {
                bytes[i] = ' ' + (thread_space % 95);
                thread_space /= 95;
            }
        }
    }
}