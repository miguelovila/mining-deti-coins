
#include "includes/md5.h"
#include <stdio.h>
#include <curand_kernel.h>

typedef unsigned char u08_t;
typedef unsigned int u32_t;


// the nvcc compiler stores x[] and state[] in registers (constant indices!)
//
// global thread number: n = threadIdx.x + blockDim.x * blockIdx.x
// global warp number: n >> 5
// warp thread number: n & 31

extern "C" __global__ __launch_bounds__(128, 1) void deti_coins_cuda(u32_t *wallet, u32_t *v1, u32_t *v2) {
    u32_t n, a, b, c, d, coin[13], hash[4], state[4], x[16];
    n = (u32_t)threadIdx.x + (u32_t)blockDim.x * (u32_t)blockIdx.x;

    curandState curand_state;
    curand_init((unsigned long long)clock() + n, 0, 0, &curand_state);

    u08_t *bytes = (u08_t *)&coin[0];
    bytes[0]  = 'D';
    bytes[1]  = 'E';
    bytes[2]  = 'T';
    bytes[3]  = 'I';
    bytes[4]  = ' ';
    bytes[5]  = 'c';
    bytes[6]  = 'o';
    bytes[7]  = 'i';
    bytes[8]  = 'n';
    bytes[9]  = ' ';
    bytes[10] = curand(&curand_state) % (0x7E - 0x20 + 1) + 0x20;
    bytes[11] = curand(&curand_state) % (0x7E - 0x20 + 1) + 0x20;
    // thread number
    coin[3]  = 0x20202020;
    coin[3] |= (n & 0x0000003f) << 24;
    coin[3] |= ((n & 0x00000fc0) >> 6) << 16;
    coin[3] |= ((n & 0x0003f000) >> 12) << 8;
    coin[3] |= ((n & 0x00fc0000) >> 18);
    coin[4] = 0x20202020; 
    coin[4] = ((curand(&curand_state) % (0x7E - 0x20 + 1)) + 0x20) |
             (((curand(&curand_state) % (0x7E - 0x20 + 1)) + 0x20) << 8) |
             (((curand(&curand_state) % (0x7E - 0x20 + 1)) + 0x20) << 16) |
             (((curand(&curand_state) % (0x7E - 0x20 + 1)) + 0x20) << 24);
    coin[5] = *v1;
    coin[6] = *v2;
    memset(&bytes[6 * 4], ' ', (13 * 4 - 6 * 4));
    bytes[51] = '\n';

    for (int i = 0; i < 95; i++) {
        #define C(c) (c)
        #define ROTATE(x, n) (((x) << (n)) | ((x) >> (32 - (n))))
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

        if (hash[3] == 0x00000000)
        {
            int idx = atomicAdd(&wallet[0], 13);
            if (idx <= 1011)
            {
                u32_t *p = &wallet[idx];
                for(int i = 0; i < 13; i++) p[i] = coin[i];
            }
        }
        coin[10]++;
    }

}