// includes/cuda/deti_coins_cuda_search.cu

#include <stdint.h>
#include <stdio.h>
#include <cuda_runtime.h>
#include "../../includes/md5.h"
#include "deti_coins_cuda_common.h"

__device__ void print_coin(uint32_t* coin) {
    printf("Coin content: ");
    unsigned char* bytes = (unsigned char*)coin;
    for(int i = 0; i < 52; i++) {
        printf("%c", bytes[i]);
    }
    printf("\n");
}

__device__ void print_hash(uint32_t* hash) {
    printf("Hash: %08x %08x %08x %08x\n", hash[0], hash[1], hash[2], hash[3]);
}

__device__ uint32_t deti_coin_power_gpu(uint32_t hash[4]) {
    uint32_t n;
    if(hash[3] != 0)
        n = __clz(__ffs(hash[3]));
    else if(hash[2] != 0)
        n = 32 + __clz(__ffs(hash[2]));
    else if(hash[1] != 0)
        n = 64 + __clz(__ffs(hash[1]));
    else if(hash[0] != 0)
        n = 96 + __clz(__ffs(hash[0]));
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

    if(thread_id == 0) {
        print_coin(coin);
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
    
    for(int32_t iter = 0; iter < iterations_per_thread && iter < 1000; iter++) {  // Limit iterations for debug
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
            uint32_t v = hash[i];
            hash[i] = ((v & 0xff) << 24) | ((v & 0xff00) << 8) |
                     ((v & 0xff0000) >> 8) | ((v & 0xff000000) >> 24);
        }
        
        // Debug output for first thread
        if(tid == 0 && iter < 5) {
            print_coin(coin);
            print_hash(hash);
            printf("Power: %u\n", deti_coin_power_gpu(hash));
        }
        
        uint32_t power = deti_coin_power_gpu(hash);
        if(power >= 32) {
            int32_t idx = atomicAdd(&found_coins->count, 1);
            printf("Thread %d found a coin with power %u!\n", tid, power);
            if(idx < COINS_BUFFER_SIZE) {
                for(int i = 0; i < 13; i++) {
                    found_coins->coins[idx][i] = coin[i];
                }
                print_coin(coin);
                print_hash(hash);
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