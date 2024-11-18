#ifndef DETI_COINS_CPU_AVX_SEARCH
#define DETI_COINS_CPU_AVX_SEARCH

#include <immintrin.h>

// Ensure 32-byte alignment for AVX
#define ALIGN32 __attribute__((aligned(32)))

static void deti_coins_cpu_avx_search(int n_random_words)
{
  u32_t n,idx;
  u64_t n_attempts,n_coins;
  
  // Aligned arrays for AVX operations
  ALIGN32 u32_t coin[13u];
  ALIGN32 u32_t hash[4u]; 
  u08_t *bytes;

  bytes = (u08_t *)&coin[0];
  //
  // mandatory for a DETI coin
  //
  bytes[0u] = 'D';
  bytes[1u] = 'E';
  bytes[2u] = 'T';
  bytes[3u] = 'I';
  bytes[4u] = ' ';
  bytes[5u] = 'c';
  bytes[6u] = 'o';
  bytes[7u] = 'i';
  bytes[8u] = 'n';
  bytes[9u] = ' ';
  //
  // arbitrary, but printable utf-8 data terminated with a '\n' is highly desirable
  //
  for(idx = 10u;idx < 13u * 4u - 1u;idx++)
    bytes[idx] = ' ';
  //
  // mandatory termination
  //
  bytes[13u * 4u -  1u] = '\n';
  //
  // find DETI coins
  //
  for(n_attempts = n_coins = 0ul;stop_request == 0;n_attempts++)
  {
    //
    // compute MD5 hash using AVX instructions
    //
    md5_cpu(coin,hash);
    //
    // byte-reverse each word (that's how the MD5 message digest is printed...)
    //
    hash_byte_reverse(hash);
    //
    // count the number of trailing zeros of the MD5 hash
    //
    n = deti_coin_power(hash);
    //
    // if the number of trailing zeros is >= 32 we have a DETI coin
    //
    if(n >= 32u)
    {
      save_deti_coin(coin);
      n_coins++;
    }
    //
    // try next combination (byte range: 0x20..0x7E)
    //
    for(idx = 10u;idx < 13u * 4u - 1u && bytes[idx] == (u08_t)126;idx++)
      bytes[idx] = ' ';
    if(idx < 13u * 4u - 1u)
      bytes[idx]++;
  }
  STORE_DETI_COINS();
  printf("deti_coins_cpu_avx_search: %lu DETI coin%s found in %lu attempt%s (expected %.2f coins)\n",n_coins,(n_coins == 1ul) ? "" : "s",n_attempts,(n_attempts == 1ul) ? "" : "s",(double)n_attempts / (double)(1ul << 32));
}

#endif