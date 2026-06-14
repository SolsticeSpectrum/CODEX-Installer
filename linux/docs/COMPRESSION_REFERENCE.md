# FreeArc Compression Library -- Technical Reference

Source: `/linux/app/include/unarc/Compression/`

---

## 1. COMPRESSION_METHOD Base Class

Defined in `Compression.h` (line 275).

Abstract base class from which every compression algorithm inherits.

### Pure Virtual Methods

| Method | Scope | Purpose |
|--------|-------|---------|
| `decompress(CALLBACK_FUNC*, void*)` | Always | Run decompression via callback I/O |
| `compress(CALLBACK_FUNC*, void*)` | Compress-only | Run compression via callback I/O |
| `ShowCompressionMethod(char* buf)` | Compress-only | Serialize method+params to canonical string (inverse of parse) |
| `GetCompressionMem()` | Compress-only | Memory required to compress |
| `GetDecompressionMem()` | Always | Memory required to decompress |
| `GetDictionary()` | Compress-only | Dictionary size |
| `GetBlockSize()` | Compress-only | Block size |
| `SetCompressionMem(MemSize)` | Compress-only | Adjust parameters to fit within given compression memory budget |
| `SetDecompressionMem(MemSize)` | Compress-only | Adjust parameters to fit within given decompression memory budget |
| `SetDictionary(MemSize)` | Compress-only | Set dictionary size |
| `SetBlockSize(MemSize)` | Compress-only | Set block size |

### Non-Virtual / Default Methods

- **`LimitCompressionMem(mem)`**, **`LimitDecompressionMem(mem)`**, **`LimitDictionary(dict)`**, **`LimitBlockSize(bs)`** -- convenience wrappers that call the corresponding `Set*` only if the current value exceeds the limit.
- **`doit(char* what, int param, void* data, CALLBACK_FUNC*)`** -- generic service dispatch. Default implementation handles `"encryption?"` (returns 0) and `"GetCompressionMem"` / `"GetDecompressionMem"` (returns 0). Returns `FREEARC_ERRCODE_NOT_IMPLEMENTED` for anything else. Overridden by Tornado to handle `"VeryFast?"`.

### State

- **`double addtime`** -- additional CPU time spent outside the main thread (e.g. multithreaded matchfinder). Initialized to 0. Set to -1 when wall-clock timing should be used instead.

All methods guarded by `FREEARC_DECOMPRESS_ONLY` are excluded in decompress-only builds, leaving only `decompress()` and `GetDecompressionMem()`.

---

## 2. ParseCompressionMethod

Defined in `CompressionLibrary.cpp` (line 543).

```
COMPRESSION_METHOD* ParseCompressionMethod(char* method)
```

**Algorithm:**

1. Copy the method string into a local buffer (max `MAX_METHOD_STRLEN` = 2048 chars).
2. Split on `':'` (`COMPRESSION_METHOD_PARAMETERS_DELIMITER`) into an array of up to `MAX_PARAMETERS` (200) string pointers.
   - `parameters[0]` is the method name (e.g. `"lzma"`, `"rep"`, `"grzip"`).
   - `parameters[1..N]` are colon-separated parameter tokens.
   - `parameters[N+1]` is NULL.
3. Iterate the **external** compressor table (`cmExternalTable`, 0..cmExternalCount). For each entry, call `parser(parameters, data)`. Return the first non-NULL result.
4. Iterate the **built-in** compressor table (`cmTable`, 0..cmCount). For each entry, call `parser(parameters)`. Return the first non-NULL result.
5. Return NULL if no parser claimed the method.

External parsers are checked first so that user-defined `[External compressor]` sections can override built-in methods.

---

## 3. AddCompressionMethod and Static Registration

Defined in `CompressionLibrary.cpp` (lines 509-538).

### Built-in Methods

```c++
int cmCount = 0;
Parser<CM_PARSER> cmTable[MAX_COMPRESSION_METHODS];   // capacity 1000

int AddCompressionMethod(CM_PARSER parser) {
    CHECK(cmCount < elements(cmTable), ...);
    cmTable[cmCount++].parser = parser;
    return 0;
}
```

Every C_*.cpp file ends with a static initializer that calls `AddCompressionMethod`:

```c++
static int LZMA_x = AddCompressionMethod(parse_LZMA);
static int REP_x  = AddCompressionMethod(parse_REP);
// ... etc.
```

Because these are file-scope static variables, `AddCompressionMethod` runs during C++ static initialization (before `main()`), populating `cmTable`.

### External Methods

```c++
int cmExternalCount = 0;
Parser<CM_PARSER2> cmExternalTable[MAX_COMPRESSION_METHODS];

int AddExternalCompressionMethod(CM_PARSER2 parser2, void* data);
```

External methods store both a parser function and a `void* data` pointer (typically an `EXTERNAL_METHOD` template struct). Registered via `AddExternalCompressor()` which parses INI-style `[External compressor:]` sections.

`ClearExternalCompressorsTable()` resets to only the built-in external methods (e.g. `tempfile`).

---

## 4. Callback API

### Signature

```c
typedef int CALLBACK_FUNC(const char* what, void* data, int size, void* auxdata);
```

### Protocol Commands

| `what` | Direction | `data` | `size` | Return value |
|--------|-----------|--------|--------|--------------|
| `"read"` | Caller provides buffer | Buffer to fill | Max bytes | Bytes actually read (0 = EOF, <0 = error) |
| `"write"` | Caller provides data | Data to write | Byte count | size on success, <0 on error |
| `"init"` | Signal | NULL | 0 | ignored |
| `"done"` | Signal | NULL | 0 | ignored |
| `"time"` | Report | `double*` pointing to CPU time | 0 | ignored |
| `"quasiwrite"` | Report | `int64*` of expected output size | size | ignored |

### Convenience Macros (Compression.h)

- **`checked_read(ptr,size)`** / **`checked_write(ptr,size)`** -- call callback, goto `finished` on error.
- **`READ(buf,size)`** / **`WRITE(buf,size)`** -- same, using `errcode` variable.
- **`READ4(var)`** / **`WRITE4(value)`** -- read/write a 4-byte little-endian header.
- **`READ_LEN(len,buf,size)`** / **`READ_LEN_OR_EOF(len,buf,size)`** -- read with length output.
- **`FOPEN()`** / **`FWRITE(buf,size)`** / **`FFLUSH()`** / **`FCLOSE()`** -- buffered output (64KB buffer).

All macros assume local variable `errcode` and label `finished:` exist in scope.

---

## 5. MultiDecompress Threading Model

Defined in `CompressionLibrary.cpp` (lines 240-402).

### Purpose

Decompresses a chained method string like `"delta+rep+lzma"` by running each stage in its own thread with pipeline parallelism.

### Data Structures

```c++
struct Params {
    CThread             thread;
    int                 thread_num;      // 0..N-1
    int                 threads_total;   // N
    CMETHOD             method;          // e.g. "lzma"
    CALLBACK_FUNC*      callback;        // original I/O callback
    void*               auxdata;
    BYTE*               buf;             // shared data pointer
    int                 size;            // data length (-1 = EOF)
    CManualResetEvent*  done;            // shared completion event
    int*                retcode;         // shared error code
    CCriticalSection*   retcode_cs;
    CSemaphore          read;            // this thread's read semaphore
    CSemaphore          write;           // this thread's write semaphore
};
```

### Flow

1. **`MultiDecompress`** splits the method string on `'+'`, reverses the order (methods are listed in compression order but execute in reverse for decompression), creates N `Params` structs with binary semaphores (capacity 1), and starts N threads.

2. Each thread calls `Decompress(method, multi_decompress_callback, &param[i])`.

3. **`multi_decompress_callback`** routes I/O:
   - **First thread** (thread_num=0): `"read"` goes to the original callback (reads from archive). `"write"` publishes data to the next thread.
   - **Last thread** (thread_num=N-1): `"read"` comes from the previous thread. `"write"` goes to the original callback (writes decompressed output).
   - **Middle threads**: both `"read"` and `"write"` go through the inter-thread pipe.

4. **Inter-thread communication** uses a pair of semaphores per thread boundary:
   - **Writer** (thread i) sets `param[i].buf` and `param[i].size`, then releases `param[i+1].read` semaphore, then blocks on `param[i].write` semaphore.
   - **Reader** (thread i) blocks on `param[i].read` semaphore, copies data from `param[i-1].buf`/`size`. When it needs more data than available, it releases `param[i-1].write` and loops.
   - EOF is signaled by setting `size = -1`.

5. **Termination**: the last thread signals the shared `done` ManualResetEvent. `MultiDecompress` then waits for all threads to join via `thread.Wait()`.

6. **Error handling**: `SetExitCode(code)` atomically sets the first error code under a critical section and signals `done`.

---

## 6. CompressMem vs Compress -- Memory vs Streaming APIs

### Streaming API

```c
int Compress(char* method, CALLBACK_FUNC* callback, void* auxdata);
int Decompress(char* method, CALLBACK_FUNC* callback, void* auxdata);
```

Parses method string, creates a `COMPRESSION_METHOD` object, calls `timed_compress`/`timed_decompress`, deletes the object, returns result. Data flows through callback.

### Memory API

```c
int CompressMem(char* method, void* input, int inputSize, void* output, int outputSize);
int DecompressMem(char* method, void* input, int inputSize, void* output, int outputSize);
```

Sets global state (`readPtr`/`readLeft`/`writePtr`/`writeLeft`), then calls `Compress`/`Decompress` with `ReadWriteMem` as the callback. `ReadWriteMem` implements `"read"` as memcpy from input buffer and `"write"` as memcpy to output buffer (returns `FREEARC_ERRCODE_OUTBLOCK_TOO_SMALL` if output overflows). Returns bytes written on success, error code on failure.

### WithHeader Variants

- **`CompressWithHeader`**: writes the canonical method string (null-terminated) to the output stream before compressed data.
- **`DecompressWithHeader`**: reads the method string byte-by-byte until `'\0'`, then calls `Decompress`.
- Memory variants (`CompressMemWithHeader`, `DecompressMemWithHeader`) combine the header approach with the mem callback.

---

## 7. Compression Method Parse Functions and Parameters

### 7.1 LZMA (`LZMA/C_LZMA.cpp`)

**Method name**: `"lzma"`

**Parameters** (defaults in parentheses):

| Field | Default | Parse prefix | Description |
|-------|---------|-------------|-------------|
| `dictionarySize` | 64 MB | `d` or bare memory value | LZ dictionary size |
| `hashSize` | 0 (auto) | `h` | Hash table size override |
| `algorithm` | 1 | `a` | 0=fast, 1=normal, 2=max |
| `numFastBytes` | 32 | `fb` or bare integer | Fast bytes for match length |
| `matchFinder` | kHT4 (4) | `mf` or bare name | BT2/BT3/BT4/HC4/HT4 |
| `matchFinderCycles` | 0 (auto) | `mc` | Match finder depth |
| `posStateBits` | 2 | `pb` | Position state bits |
| `litContextBits` | 3 | `lc` | Literal context bits |
| `litPosBits` | 0 | `lp` | Literal position bits |

**Presets**: `fastest` (a0/HT4/fb32/mc1), `fast` (a0/HT4/fb32), `normal` (a1/HT4/fb32), `max` (a2/BT4/fb128), `ultra` (a2/BT4/fb128/mc128).

**Unnamed params**: tried as matchfinder name, then integer (numFastBytes), then memory value (dictionarySize).

**Memory**: Compression memory depends on matchfinder type (BT variants use ~9-11.5x dict, HC4 ~7.5x, HT4 ~1.75x, plus hash tables). Decompression = `dictionarySize + RangeDecoderBufferSize`.

**Threading**: When algorithm > 0 or matchFinder != HC4 and threads > 1, uses multithreaded matchfinder. Sets `addtime = -1` to force wall-clock timing.

### 7.2 REP (`REP/C_REP.cpp`)

**Method name**: `"rep"`

**Parameters**:

| Field | Default | Parse prefix | Description |
|-------|---------|-------------|-------------|
| `BlockSize` | 64 MB | `b` or bare memory | Processing block size |
| `MinCompression` | 100 | `N%` suffix | Min compression ratio (percent); if worse, store raw |
| `MinMatchLen` | 512 | `l` or bare integer | Minimum match length for dedup reference |
| `HashSizeLog` | 0 (auto) | `h` | Log2 of hash table size |
| `Barrier` | INT_MAX | `d` | Distance barrier -- beyond this, use SmallestLen |
| `SmallestLen` | 512 | `s` | Minimum match length beyond barrier |
| `Amplifier` | 1 | `a` | Match amplification factor |

**Decompression memory** = `BlockSize`.

### 7.3 Tornado (`Tornado/C_Tornado.cpp`)

**Method name**: `"tor"`

**Parameters** (stored in embedded `PackMethod m` struct):

| Field | Parse prefix | Description |
|-------|-------------|-------------|
| `m.number` | bare integer | Preset level (selects from `std_Tornado_method[]`) |
| `m.buffer` | `b` or bare memory | Dictionary / buffer size |
| `m.hashsize` | `h` | Hash table size |
| `m.hash_row_width` | `l` | Hash row width |
| `m.encoding_method` | `c` | Encoding method selector |
| `m.match_parser` | `p` | Match parser strategy |
| `m.update_step` | `u` | Hash update step |
| `m.find_tables` | `t` | Table detection |
| `m.auxhash_size` | `ah` | Auxiliary hash size |
| `m.auxhash_row_width` | `al` | Auxiliary hash row width |

**Compression memory** = `hashsize + buffer + outbuf_size(buffer)`. **Decompression memory** = `buffer`.

Overrides `doit("VeryFast?")` -- returns true if `hash_row_width <= 2`.

### 7.4 PPMD (`PPMD/C_PPMD.cpp`)

**Method name**: `"ppmd"`

**Parameters**:

| Field | Default | Parse prefix | Description |
|-------|---------|-------------|-------------|
| `order` | 10 | `o` or bare integer | PPM model order |
| `mem` | 48 MB | `m` or `mem` or bare memory | Suballocator memory |
| `MRMethod` | 0 | `r` (single char = 1) | Model restoration method (0/1/2) |

**Compression memory** = `mem`. **Decompression memory** = `mem`.

`SetCompressionMem` adjusts both `mem` and `order` together: `order += log2(newmem/oldmem) * 4`.

### 7.5 GRZip (`GRZip/C_GRZip.cpp`)

**Method name**: `"grzip"`

BWT-based block-sorting compressor with optional LZP preprocessing, delta filtering, and adaptive block sizing. Supports multithreaded compression and decompression via the `MTCompressor` framework.

**Parameters**:

| Field | Default | Parse prefix | Description |
|-------|---------|-------------|-------------|
| `Method` | 1 | `m` | 1=BWT+WFC, 2=BWT+MTF, 3=ST4+WFC, 4=ST4+MTF |
| `BlockSize` | 8 MB | `b` or bare memory | Block size (capped at `GRZ_MaxBlockSize`) |
| `EnableLZP` | 1 | `l` (single char disables) | Enable LZP preprocessing |
| `MinMatchLen` | 32 | `l` + int, or bare int | LZP minimum match length |
| `HashSizeLog` | 15 | `h` | LZP hash table log2 size |
| `AlternativeBWTSort` | 0 | `s` | Use stronger (slower) BWT sort |
| `AdaptiveBlockSize` | 0 | `a` | Enable adaptive block sizing |
| `DeltaFilter` | 0 | `d` | Enable delta filter |

Preset `p` = AdaptiveBlockSize off, LZP off, DeltaFilter on.

**Compression memory**: `[7-9] * BlockSize + 1MB` (normal mode) or `5 * BlockSize + 1MB` (fast/ST4 mode).
**Decompression memory**: `5 * BlockSize + 1MB`.

### 7.6 Delta (`Delta/C_Delta.cpp`)

**Method name**: `"delta"`

**Parameters**:

| Field | Default | Parse prefix | Description |
|-------|---------|-------------|-------------|
| `BlockSize` | 8 MB | `b` or bare memory | Processing block size |
| `ExtendedTables` | 0 | `x` | Enable extended detection tables |

### 7.7 Dict (`Dict/C_Dict.cpp`)

**Method name**: `"dict"`

Dictionary-based preprocessor that detects and replaces repeated byte sequences.

**Parameters**:

| Field | Default | Parse prefix | Description |
|-------|---------|-------------|-------------|
| `BlockSize` | 64 MB | `b` or bare memory | Processing block size |
| `MinCompression` | 100 | `N%` | Minimum compression ratio |
| `MinWeakChars` | 20 | `c` or bare integer | Min weak characters |
| `MinLargeCnt` | 2048 | `l` | Min count for large patterns |
| `MinMediumCnt` | 100 | `m` | Min count for medium patterns |
| `MinSmallCnt` | 50 | `s` | Min count for small patterns |
| `MinRatio` | 4 | `r` | Min compression ratio for patterns |

Presets: `p` = precise (8192/400/100/4), `f` = fast (2048/100/50/0).

### 7.8 LZP (`LZP/C_LZP.cpp`)

**Method name**: `"lzp"`

LZP (Lempel-Ziv Prediction) preprocessor based on Shkarin's algorithm.

**Parameters**:

| Field | Default | Parse prefix | Description |
|-------|---------|-------------|-------------|
| `BlockSize` | 8 MB | `b` or bare memory | Processing block size |
| `MinCompression` | 100 | `N%` | Min compression ratio |
| `MinMatchLen` | 64 | `l` or bare integer | Minimum match length |
| `HashSizeLog` | 18 | `h` | Log2 of hash table size |
| `Barrier` | INT_MAX | `d` | Distance barrier |
| `SmallestLen` | 32 | `s` | Min match len beyond barrier |

### 7.9 MM (`MM/C_MM.cpp`)

**Method name**: `"mm"`

Multimedia (audio) preprocessor.

**Parameters**:

| Field | Default | Parse prefix | Description |
|-------|---------|-------------|-------------|
| `mode` | 9 | `d` | Compression mode/level |
| `skip_header` | 0 | `s` | Skip WAV header |
| `is_float` | 0 | `f` | Floating-point samples |
| `num_chan` | 0 (auto) | `c` | Number of channels |
| `word_size` | 0 (auto) | `w` | Sample word size in bytes |
| `offset` | 0 | `o` | Data offset |
| `reorder` | 0 | `r` | Channel reorder mode |

**Special syntax**: `N*M` sets `num_chan=N, word_size=M`. Appending `f` (e.g. `2*4f`) sets `is_float=1`.

### 7.10 TTA (`MM/C_TTA.cpp`)

**Method name**: `"tta"`

True Audio lossless compressor.

**Parameters**:

| Field | Default | Parse prefix | Description |
|-------|---------|-------------|-------------|
| `level` | 3 | `m` | Compression level |
| `skip_header` | 0 | `s` | Skip WAV header |
| `is_float` | 0 | `f` | Floating-point samples |
| `num_chan` | 0 (auto) | `c` | Number of channels |
| `word_size` | 0 (auto) | `w` | Sample word size |
| `offset` | 0 | `o` | Data offset |
| `raw_data` | 0 | `r` | Raw data mode |

Same `N*M[f]` shorthand as MM.

### 7.11 External (`External/C_External.cpp`)

**Method name**: configurable via `[External compressor:]` INI sections.

**Built-in external**: `"pmm"` (PPMonstr) -- hardcoded parser `parse_PPMONSTR` with fields: `order` (default 16), `cmem`/`dmem` (192 MB), `MRMethod` (1). Constructs shell commands to invoke `ppmonstr e` / `ppmonstr d`.

**How external tools are called** (`external_program()`):

1. Creates a temporary directory.
2. Reads all input data from the callback into a temp file (`datafile`).
3. If compressing and `useHeader`, writes a 1-byte flag (1=compressed, 0=stored).
4. Runs the external command via `RunCommand()` in the temp directory, waiting for exit.
5. Opens the output temp file and writes its contents back through the callback.
6. If the external tool fails or produces no output, falls back to storing uncompressed data (for compression) or returns an error (for decompression).

**`prepare_cmd()`** replaces `{options}` or `{...option...}` templates in command strings with actual method options.

**`AddExternalCompressor()`** parses INI-format sections like:

```ini
[External compressor: ccm123, ccmx123]
mem = 276
packcmd = {compressor} c $$arcdatafile$$.tmp $$arcpackedfile$$.tmp
unpackcmd = {compressor} d $$arcpackedfile$$.tmp $$arcdatafile$$.tmp
```

Creates one `EXTERNAL_METHOD` template per version name and registers each with `AddExternalCompressionMethod(parse_EXTERNAL, &version[i])`.

The `"tempfile"` method is registered as a built-in external compressor with an empty command -- it stores data through a temp file, used to materialize piped data between methods like REP and LZMA.

---

## 8. GetCompressionThreads / SetCompressionThreads

Defined in `CompressionLibrary.cpp` (lines 448-458).

```c
static int CompressionThreads = 1;

int  GetCompressionThreads(void)  { return CompressionThreads; }
void SetCompressionThreads(int threads) {
    CompressionThreads = threads == 0 ? 1 : threads;
    // On Windows, also propagate to facompress.dll if loaded
}
```

Default is 1 thread. Setting 0 is treated as 1 (autodetect disabled in this implementation). The value is read by:
- LZMA's `compress()` to decide whether to enable multithreaded matchfinder.
- `MTCompressor::CreateJobs()` (GRZip's multithreading framework) to determine thread pool size: `NumThreads = CompressionThreads + CompressionThreads/2 + 1`.

---

## 9. COMPRESSION_METHODS_DELIMITER '+' Chaining

Defined in `Compression.h` (line 50):

```c
#define COMPRESSION_METHODS_DELIMITER '+'
```

A compressor string like `"delta+rep+lzma"` represents a pipeline: data flows through delta, then rep, then lzma during compression. For decompression, the order is reversed.

**`MultiDecompress`** splits on `'+'` using `split()`, then processes methods in reverse order (line 294: `param[i].method = cm[N-1-i]`).

**`compressorGetDecompressionMem`** sums decompression memory across all methods in a chain.

**`compressorIsEncrypted`** checks whether any method in the chain reports itself as encryption via `CompressionService(method, "encryption?")`.

---

## 10. timed_compress / timed_decompress

Defined in `CompressionLibrary.cpp` (lines 6-18, 91-103).

```c++
int timed_decompress(COMPRESSION_METHOD* compressor, CALLBACK_FUNC* callback, void* auxdata) {
    double time0 = GetThreadCPUTime();
    int result = compressor->decompress(callback, auxdata);
    double time1 = GetThreadCPUTime();
    double t;
    if (time0 >= 0 && time1 >= 0 && compressor->addtime >= 0)
        t = compressor->addtime + time1 - time0;
    else
        t = -1;   // timing unavailable
    callback("time", &t, 0, auxdata);
    return result;
}
```

**Behavior**:

1. Records thread CPU time before and after the compress/decompress call.
2. Adds `compressor->addtime` (extra time from child threads, e.g. multithreaded matchfinder overhead).
3. If any timing value is negative (unavailable or explicitly disabled via `addtime = -1`), reports `t = -1`.
4. Reports the elapsed time via callback with `what = "time"`.

Both `Compress()` and `Decompress()` wrap the actual method call through `timed_compress` / `timed_decompress` respectively. This is what provides per-method timing statistics to the caller.

---

## Appendix: Common.h Key Types

| Type | Definition |
|------|-----------|
| `MemSize` | `unsigned` (32-bit memory size) |
| `FILENAME` | `char*` |
| `FILESIZE` | `int64` (Win) or `off_t` (Unix) |
| `MYFILE` | File wrapper with handle-based I/O, temp file tracking, cross-platform path handling |
| `MYDIR` | Directory wrapper extending MYFILE with `create_tempdir()` |

**Memory allocation**: `BigAlloc`/`BigFree` use `MyAlloc`/`MyFree` on Linux (which call `malloc`). On Windows, they attempt large pages via `VirtualAlloc`. `MidAlloc` = `BigAlloc` on Linux.

**Utility functions**: `parseMem(str, &error)` parses memory strings with b/k/m/g/^ suffixes. `parseInt(str, &error)` parses plain integers. `showMem(mem, buf)` formats memory values. `split(str, delimiter, result, max)` tokenizes strings.

---

## Appendix: Error Codes

| Code | Value | Meaning |
|------|-------|---------|
| `FREEARC_OK` | 0 | Success |
| `FREEARC_ERRCODE_GENERAL` | -1 | Generic error |
| `FREEARC_ERRCODE_INVALID_COMPRESSOR` | -2 | Bad method name or parameters |
| `FREEARC_ERRCODE_ONLY_DECOMPRESS` | -3 | Decompress-only build |
| `FREEARC_ERRCODE_OUTBLOCK_TOO_SMALL` | -4 | Output buffer too small (CompressMem) |
| `FREEARC_ERRCODE_NOT_ENOUGH_MEMORY` | -5 | Allocation failure |
| `FREEARC_ERRCODE_READ` | -6 | Read error |
| `FREEARC_ERRCODE_BAD_COMPRESSED_DATA` | -7 | Corrupt data |
| `FREEARC_ERRCODE_NOT_IMPLEMENTED` | -8 | Feature not supported |
| `FREEARC_ERRCODE_NO_MORE_DATA_REQUIRED` | -9 | Early termination (enough data decompressed) |
| `FREEARC_ERRCODE_OPERATION_TERMINATED` | -10 | User cancellation |
| `FREEARC_ERRCODE_WRITE` | -11 | Write error |
