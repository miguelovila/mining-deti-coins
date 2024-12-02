#ifndef DETI_COINS_CUDA_SEARCH
#define DETI_COINS_CUDA_SEARCH

#define CUDA_ITERATIONS_PER_BATCH 1000000
#define MAX_FOUND_COINS_PER_BATCH 1024

// External CUDA function declarations
extern void mine_deti_coins_cuda(
    u32_t *template_data,
    u32_t n_random_words,
    u32_t *found_coins_data,
    u32_t *found_coins_count,
    u32_t iterations);

static void deti_coins_cuda_search(u32_t n_random_words, bool is_client)
{
    u32_t template_data[13];
    u08_t *bytes = (u08_t *)template_data;
    u64_t total_attempts = 0, total_coins = 0;

    // Initialize template with required prefix
    const u08_t prefix[] = {'D', 'E', 'T', 'I', ' ', 'c', 'o', 'i', 'n', ' '};
    memcpy(bytes, prefix, sizeof(prefix));

    // Fill remaining template with spaces and newline
    for (u32_t i = sizeof(prefix); i < 51; i++)
    {
        bytes[i] = ' ';
    }
    bytes[51] = '\n';

    // Generate initial random content if requested
    if (n_random_words > 0)
    {
        for (u32_t i = 0; i < n_random_words * 4 && i + 10 < 51; i++)
        {
            bytes[10 + i] = ' ' + (random() % 95);
        }
    }

    // Allocate buffers for found coins
    u32_t found_coins[MAX_FOUND_COINS_PER_BATCH][13];
    u32_t found_coins_count;

    printf("Starting CUDA search with %u random words...\n", n_random_words);

    // Main search loop
    while (!stop_request)
    {
        // Run a batch of iterations on the GPU
        mine_deti_coins_cuda(
            template_data,
            n_random_words,
            (u32_t *)found_coins,
            &found_coins_count,
            CUDA_ITERATIONS_PER_BATCH);

        // Process found coins
        for (u32_t i = 0; i < found_coins_count; i++)
        {
            is_client ? client_save_deti_coin(found_coins[i]) : save_deti_coin(found_coins[i]);
            total_coins++;
        }

        total_attempts += CUDA_ITERATIONS_PER_BATCH;

        // Optional: Print progress update
        if (total_attempts % (CUDA_ITERATIONS_PER_BATCH * 100) == 0)
        {
            printf("Progress: %lu attempts, %lu coins found\n", total_attempts, total_coins);
        }
    }

    if (!is_client)
    {
        STORE_DETI_COINS();
    }

    printf("deti_coins_cuda_search: %lu DETI coins found in %lu attempts (expected %.2f coins)\n",
           total_coins, total_attempts, (double)total_attempts / (double)(1ul << 32));
}

#endif