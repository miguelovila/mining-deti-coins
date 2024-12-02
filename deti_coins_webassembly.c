//
// Miguel Vila, December 2024
//
// Arquiteturas de Alto Desempenho 2024/2025
//

#ifndef DETI_COINS_CPU_SEARCH
#define DETI_COINS_CPU_SEARCH

#include <stdio.h>
#include <time.h>
typedef unsigned int u32_t;
typedef unsigned char u08_t;
#include "includes/md5.h"

#define next_value_to_try(v)                                \
        do {                                                \
            v++;                                            \
            if((v & 0xFF) == 0x7F) {                        \
                v += 0xA1;                                  \
                if(((v >> 8) & 0xFF) == 0x7F) {             \
                    v += 0xA1 << 8;                         \
                    if (((v >> 16) & 0xFF) == 0x7F) {       \
                        v += 0xA1 << 16;                    \
                        if (((v >> 24) & 0xFF) == 0x7F) {   \
                            v += 0xA1 << 24;                \
                        }                                   \
                    }                                       \
                }                                           \
            }                                               \
        }                                                   \
        while(0)                                            

#define ATTEMPTS 700000000u

int main(void)
{
    u32_t n_attempts, n_coins, v1, v2, coin[13u], hash[4u];
    clock_t t_start, t_end;
    double t_elapsed;

    coin[0] = 0x49544544u;
    coin[1] = 0x696f6320u;
    coin[2] = 0x6e20206eu;
    coin[3] = 0x65626d75u;
    coin[4] = 0x666f2072u;
    coin[5] = 0x65687420u;
    coin[6] = 0x74746120u;
    coin[7] = 0x74706d65u;
    coin[8] = 0x305b2020u;
    coin[9] = 0x30303030u;
    v1 = coin[10] = 0x33333530u;
    v2 = coin[11] = 0x35383238u;
    coin[12] = 0x0a5d3137u;

    // find deticoins

    printf("searching for DETI coins using deti_coins_cpu_webassembly_search()\n");

    t_start = clock();

    for (n_attempts = n_coins = 0ul; n_attempts < ATTEMPTS; n_attempts++)
    {
        // compute MD5 hash
        { // one message -> one MD5 hash
            u32_t a, b, c, d, state[4], x[16];
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
        }
        if (hash[3] == 0u)
        {
            printf("Found: ");
            for (int i = 0; i < 13; i++)
            {
                for (int j = 0; j < 4; j++)
                {
                    unsigned char c = (coin[i] >> (j * 8)) & 0xFF;
                    printf("%c", c);
                }
            }
            n_coins++;
        }
        // try next combination (byte range: 0x20..0x7E)
        next_value_to_try(v1);
        coin[10] = v1;
        if (v1 == 0x20202020)
        {
            next_value_to_try(v2);
            coin[11] = v2;
        }
    }
    
    t_elapsed = (double)(t_end = clock() - t_start) / (double)CLOCKS_PER_SEC;

    printf("deti_coins_cpu_webassembly_search: %u DETI coin%s found in %u attempt%s in %.3fs (expected %.2f coins)\n",
        n_coins, (n_coins == 1ul) ? "" : "s",
        n_attempts, (n_attempts == 1ul) ? "" : "s",
        t_elapsed, (double)n_attempts / (double)(1ul << 32));
}

#endif
