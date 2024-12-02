// includes/cuda/deti_coins_cuda_search.cu

#include <stdint.h>
#include <stdio.h>
#include <cuda_runtime.h>
#include "../../includes/md5.h"
#include "deti_coins_cuda_common.h"

__device__ uint32_t count_trailing_zeros_gpu(uint32_t* hash) {
    uint32_t zeros = 0;
    // Count from least significant bits to most
    for(int word = 3; word >= 0; word--) {
        if(hash[word] == 0) {
            zeros += 32;
            continue;
        }
        // Count trailing zeros in this word
        uint32_t v = hash[word];
        while((v & 1) == 0) {
            zeros++;
            v >>= 1;
        }
        break;
    }
    return zeros;
}

__device__ void verify_hash(uint32_t* coin, uint32_t* hash) {
    if(threadIdx.x == 0 && blockIdx.x == 0) {
        unsigned char* bytes = (unsigned char*)coin;
        printf("Verifying coin: ");
        for(int i = 0; i < 52; i++) {
            printf("%c", bytes[i]);
        }
        printf("\nHash before reverse: %08x %08x %08x %08x\n", hash[0], hash[1], hash[2], hash[3]);
        
        // Compute on-device power
        uint32_t power = count_trailing_zeros_gpu(hash);
        printf("Power: %u\n", power);
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
    
    // Main mining loop
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
        
        // Byte-reverse each word
        for(int i = 0; i < 4; i++) {
            uint32_t v = hash[i];
            hash[i] = ((v & 0xff) << 24) | ((v & 0xff00) << 8) |
                     ((v & 0xff0000) >> 8) | ((v & 0xff000000) >> 24);
        }
        
        // Debug first thread
        if(tid == 0 && iter == 0) {
            verify_hash(coin, hash);
        }
        
        // Check if we found a coin
        uint32_t zeros = count_trailing_zeros_gpu(hash);
        if(zeros >= 32) {
            int32_t idx = atomicAdd(&found_coins->count, 1);
            if(idx < COINS_BUFFER_SIZE) {
                // Save the coin
                for(int i = 0; i < 13; i++) {
                    found_coins->coins[idx][i] = coin[i];
                }
                // Debug output
                if(tid == 0 || blockIdx.x == 0) {
                    verify_hash(coin, hash);
                }
            }
        }
        
        // Update search space
        bool carry = true;
        for(int32_t i = start_pos; i < 51 && carry; i++) {
            bytes[i]++;
            if(bytes[i] > '~') {
                bytes[i] = ' ';
            } else {
                carry = false;
            }
        }
        
        if(carry) {
            // Generate new random content
            thread_space = tid + (seed + 1 + iter) * blockDim.x * gridDim.x;
            for(int32_t i = start_pos; i < 51; i++) {
                bytes[i] = ' ' + (thread_space % 95);
                thread_space /= 95;
            }
        }
    }
}