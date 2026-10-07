# Running Mining DETI Coins

This is the command reference and implementation guide. The [README](README.md) tells the story behind the project; the [original report](report.pdf) contains the experiments from the 2024/2025 assignment.

## Build and run

The most straightforward setup is a Linux machine with an AVX2-capable Intel or AMD processor, GCC with OpenMP support, Make, and `md5sum`. The code assumes a little-endian platform with 32-bit `unsigned int` and 64-bit `unsigned long`.

From the repository root:

```bash
make deti_coins_intel
./deti_coins_intel -t
OMP_NUM_THREADS=4 ./deti_coins_intel -s5 2m
```

The last command searches with AVX2 and four OpenMP threads for two minutes. Found coins are written to `deti_coins_vault.txt` in the current directory when the search finishes. Finding no coins in a short run is possible: the expected yield is one coin per approximately 2³² attempts.

Specify the Make target explicitly. Running bare `make` selects `clean`, the first target in the current [makefile](makefile). Builds select instruction sets at compile time; there is no runtime CPU dispatch. Even scalar mode in the AVX2 executable requires a processor that can run that executable.

### Other build targets

| Target | Purpose |
| --- | --- |
| `deti_coins_intel` | AVX2 build with scalar, AVX, AVX2, OpenMP, and networking modes |
| `deti_coins_intel_avx512f` | Adds the AVX-512F search modes; requires compatible hardware |
| `deti_coins_intel_cuda` | Adds CUDA mining and CUDA hash tests |
| `deti_coins_intel_debug` | Builds `deti_coins_intel` with `-O0`, debug symbols, and search diagnostics |
| `deti_coins_webassembly` | Generates an Emscripten HTML page, JavaScript, and WebAssembly |
| `deti_coins_webassembly_test` | Builds the standalone browser miner source as a native executable |

Use `make -B deti_coins_intel` to force a release rebuild after using the debug target or editing headers. Debug and release builds share an output filename, and the makefile does not track every included header as a dependency.

For AVX-512F:

```bash
make deti_coins_intel_avx512f
./deti_coins_intel_avx512f -t
OMP_NUM_THREADS=4 ./deti_coins_intel_avx512f -s7 2m
```

For CUDA, install a compatible NVIDIA driver and CUDA toolkit, with `nvcc` on `PATH`. Override the toolkit directory and GPU architecture for your installation. These are the historical defaults, rather than automatic detection:

```bash
make deti_coins_intel_cuda CUDA_DIR=/usr/local/cuda-12.6 CUDA_ARCH=sm_61
./deti_coins_intel_cuda -t
./deti_coins_intel_cuda -s9 2m
```

Run the CUDA executable from the repository root: it loads the generated `.cubin` files using relative paths. It selects CUDA device 0.

There is also a `deti_coins_apple` target, but its recipe predates the unconditional OpenMP dependency and needs compiler/runtime adjustments. The repository includes a NEON MD5 primitive and its correctness test; a NEON coin-search implementation is not wired into the current program. Consequently, mode `8` is unavailable in this checkout. The report records NEON experiments, but those results do not establish a working ARM miner in the current tree.

## Command reference

Replace `MODE` with a suffix from the table below:

```text
./deti_coins_intel -t
./deti_coins_intel -sMODE [duration] [n_random_words]
./deti_coins_intel -o [port] [n_random_words]
./deti_coins_intel -cMODE IPv4 port [duration]
```

| Suffix | Search implementation | OpenMP |
| --- | --- | --- |
| `0` | Scalar CPU | No |
| `1` | Scalar CPU | Yes |
| `2` | AVX, four messages per hash call | No |
| `3` | AVX, four messages per thread per hash call | Yes |
| `4` | AVX2, eight messages per hash call | No |
| `5` | AVX2, eight messages per thread per hash call | Yes |
| `6` | AVX-512F, sixteen messages per hash call | No |
| `7` | AVX-512F, sixteen messages per thread per hash call | Yes |
| `9` | CUDA | GPU threads |
| `A` | AVX2 search with a fixed user phrase | Yes |

Use the corresponding executable for AVX-512F or CUDA. Bare `-s` and `-c` select scalar mode `0`. Set `OMP_NUM_THREADS` before launching to control the OpenMP worker count; otherwise the OpenMP runtime supplies its default.

Durations accept seconds or combinations such as `120`, `2m`, and `1h30m`. The default is 30 minutes. Every native search is clamped to a minimum of two minutes and a maximum of two hours. The alarm starts before the phrase prompt in mode `A`, so time spent entering the phrase counts toward the search duration.

`n_random_words` defaults to `1`. For the SIMD modes, it sets how many four-byte words of random printable ASCII follow the mandatory prefix in each lane. The remaining suffix is incremented through printable ASCII values, from space to `~`. A new random prefix is generated when that suffix is exhausted. Values above `9` are clamped to `9`; use `1`–`9`. Although `0` is accepted, it starts SIMD lanes with identical candidates and repeats work. Scalar, CUDA, and phrase modes ignore this option.

The orchestrator supplies `n_random_words` to clients. Do not add it to a `-c` command: some built-in usage lines list it, but the argument parser does not accept it there.

## Mining across machines

Start a collector on one machine:

```bash
./deti_coins_intel -o 12345 1
```

Start clients in other terminals or on other machines:

```bash
OMP_NUM_THREADS=4 ./deti_coins_intel -c5 127.0.0.1 12345 2m
./deti_coins_intel_cuda -c9 127.0.0.1 12345 2m
```

Replace `127.0.0.1` with the server's IPv4 address for remote clients. Ports must be between `1024` and `65535`; the default server port is `12345`. The server binds to all IPv4 interfaces, despite the startup message mentioning loopback. Remote connections require the chosen TCP port to be reachable.

The [protocol](includes/common/communication.h) has three messages:

1. `HELLO` identifies the client by hostname, backend, and OpenMP thread count.
2. `CONFIG` returns the server's `n_random_words` setting.
3. `COIN_FOUND` carries a discovered 52-byte coin back to the server.

The [server](includes/orchestration/server.h) creates a detached pthread for each connection. It checks each reported coin with the scalar MD5 implementation and immediately flushes accepted results to its vault. Clients do not maintain a local fallback vault.

This collector does not assign disjoint search ranges. Clients generate their own candidates, and the client path does not seed the CPU random-number generator. Identical clients can therefore search the same sequences. There is no deduplication or checkpoint/resume mechanism.

The networking code is an experiment with a small protocol: it sends native C structs and assumes each `send`/`recv` transfers a complete message. It does not handle partial transfers, portable serialization, authentication, encryption, or reconnection. The server's shared vault buffer also lacks a mutex between client-handler threads. These are the main areas to address before relying on a larger mining pool.

## Coins with a phrase

Mode `A` preserves a phrase after `DETI coin ` and searches the remaining bytes:

```bash
OMP_NUM_THREADS=4 ./deti_coins_intel -sA 2m
```

Enter up to **36 printable ASCII bytes** followed by Enter. For example, entering `AAD!` keeps those four characters in every candidate. The miner reserves another four bytes for a random value and increments the suffix after it. When that suffix is exhausted, it refreshes the random bytes while preserving the phrase.

Long phrases leave fewer bytes to enumerate and cause more frequent template resets. The report compares short and long phrases to show the cost. The prompt's 36-character limit is the intended bound; validation has an off-by-one mismatch, so stay within 36 bytes. Multibyte characters consume more than one byte each.

The network equivalent is:

```bash
OMP_NUM_THREADS=4 ./deti_coins_intel -cA 127.0.0.1 12345 2m
```

## Browser version

With the Emscripten environment active:

```bash
make deti_coins_webassembly
python3 -m http.server 8000
```

Open `http://localhost:8000/deti_coins_webassembly.html`. The generated files are ignored by Git.

The [browser miner](deti_coins_webassembly.c) is a separate scalar program. It performs a fixed **700,000,000 attempts**, beginning with the known example coin, and prints discoveries and elapsed time. It does not accept the native CLI options, connect to an orchestrator, or write a vault. Its synchronous loop can leave the page unresponsive while it runs.

To compile the same source for a native comparison:

```bash
make deti_coins_webassembly_test
./deti_coins_webassembly
```

That executable performs the same full search. The report's browser measurements used a billion attempts; the committed source uses 700 million.

## Format, storage, and validation

A coin is exactly 52 bytes: the ten-byte prefix `DETI coin `, 41 variable bytes, and a final newline. Its printed MD5 digest must end in at least eight hexadecimal zeroes, equivalent to 32 trailing zero bits. The miner's MD5 code is specialized for this fixed message length.

Check the supplied example without mining:

```bash
wc -c deti_coin_example.txt
md5sum deti_coin_example.txt
```

The expected length is `52` and the digest is `c0b9d8c5d2c98be1296bc13500000000`.

The [vault](includes/deti_coins_vault.h) rechecks the prefix, newline, and hash before buffering a coin. Each stored record is 56 bytes: `Vnn:` followed by the original 52-byte coin. Here `nn` is the number of trailing zero bits beyond the required 32. For example, `V00:` denotes a power-32 coin and `V01:` denotes power 33. The assignment values a power-*p* coin at 2^(*p*−32) base coins.

Standalone searches flush their buffer when it fills or the timed search ends. Interrupting a run with Ctrl+C can lose buffered discoveries. The collector flushes after each received coin. Existing vault contents are appended to, with no duplicate filtering.

```bash
bash test_vault.bash
```

This script removes each record's four-byte header and prints its MD5 digest. Inspect the trailing zeroes; it is a hash display, not an automated pass/fail test or deduplication check.

## Interpreting the measurements

The `-t` command generates `1 << 20` random 52-byte messages. It checks the first 64 scalar results against `md5sum`, then compares compiled SIMD/CUDA implementations against the scalar results across the generated set. It also prints CPU and wall-clock nanoseconds per hash. The test uses the fixed temporary path `/tmp/hash.data`, so run one test process at a time.

CPU/SIMD timings repeatedly hash the same buffers and exclude candidate generation, coin checks, and persistence. The CUDA test measures the separate bulk-hashing implementation, including host/device transfers, rather than the mining kernel. These timings do not measure complete searches.

The README's throughput table divides the report's attempt counts by 120 seconds. Those counters measure hashing attempts, without checking candidate uniqueness. The report does not provide repeated-run statistics, exact compiler versions, or the OpenMP thread count for its debug comparison. For new comparisons, record the hardware, build flags, mode, thread count, duration, and random-prefix length alongside the results.

## Implementation notes

The scalar OpenMP miner shares its loop-index variable between threads, introducing a data race in candidate initialization and advancement. The `-t` hash checks do not exercise this search loop. The server's separate shared-vault issue is described under [mining across machines](#mining-across-machines).

The CUDA miner keeps candidates on the GPU and returns discoveries through a bounded wallet. Its current kernel overwrites the supplied `v2` word during template initialization, and wallet overflow drops additional results. The implementation should not be read as an exhaustive, lossless search of its nominal counter space.

## Original material and credits

This project was developed by **Miguel Vila and Matilde Teixeira** for *Arquiteturas de Alto Desempenho*, 2024/2025, at the University of Aveiro. It extends reference code supplied by **Tomás Oliveira e Silva**, including the fixed-length MD5 macro, scalar miner, AVX/NEON hash implementations, CUDA support, tests, and vault.

- [Assignment brief / proposal](proposal.pdf)
- [Project report (PDF, Portuguese)](report.pdf)
- [Editable report (ODT)](report.odt)
- [Example coin](deti_coin_example.txt)

The report preserves the original experiments. This guide follows the source currently in the repository where its build targets or available modes differ from that report.
