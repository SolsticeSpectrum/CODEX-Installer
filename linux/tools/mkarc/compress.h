#pragma once

// 4x4 parallel compression via COMPRESSION_METHOD

#include <cstring>
#include <string>
#include <vector>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <queue>
#include <algorithm>

#include "Compression/LZMA2/C_LZMA.h"
#include "Compression/LZMA2/C/LzmaEnc.h"

#include "arcformat.h"

// matchfinder constants (from C_LZMA.cpp)
#define MF_HashChain  0
#define MF_BinaryTree 1
#define MF_HashTable  2
enum { kBT2=0, kBT3, kBT4, kHC4, kHT4 };

static void *SzAlloc4x4(void *, size_t size) { return size ? malloc(size) : nullptr; }
static void  SzFree4x4(void *, void *addr)   { free(addr); }
static ISzAlloc g_Alloc4x4 = { SzAlloc4x4, SzFree4x4 };


// byte frequency deviation test (fast-lzma2 algorithm)
// uniform distribution = incompressible
static bool isIncompressible(const uint8_t *data, int size) {
    if (size < 4096) return false;

    int sampleSz = std::min(size, 16384);
    uint32_t avg = sampleSz / 64; // expected freq*4 for uniform

    uint32_t freq[256] = {};
    for (int i = 0; i < sampleSz; i++)
        freq[data[i]] += 4;

    uint64_t charTotal = 0;
    for (int i = 0; i < 256; i++) {
        int64_t delta = (int64_t)freq[i] - avg;
        charTotal += (uint64_t)(delta * delta);
    }

    // charTotal <= 576 * sampleSz  (fast-lzma2 threshold)
    return charTotal <= (uint64_t)576 * sampleSz;
}


struct LzmaCtx {
    CLzmaEncHandle enc;
    CLzmaEncProps  props;
    bool           initialized;

    LzmaCtx() : enc(nullptr), initialized(false) {}

    bool init(COMPRESSION_METHOD *cm) {
        if (initialized) return true;
        enc = LzmaEnc_Create(&g_Alloc4x4);
        if (!enc) return false;

        auto *lm = (LZMA_METHOD*)cm;
        LzmaEncProps_Init(&props);
        props.dictSize     = lm->dictionarySize;
        props.hashSize     = lm->hashSize;
        props.algo         = lm->algorithm;
        props.fb           = lm->numFastBytes;
        props.mc           = lm->matchFinderCycles;
        props.lc           = lm->litContextBits;
        props.lp           = lm->litPosBits;
        props.pb           = lm->posStateBits;
        props.numThreads   = 1; // 4x4 parallelism at block level
        props.writeEndMark = 1;
        switch (lm->matchFinder) {
            case kHC4: props.btMode = MF_HashChain;  props.numHashBytes = 4; break;
            case kBT2: props.btMode = MF_BinaryTree; props.numHashBytes = 2; break;
            case kBT3: props.btMode = MF_BinaryTree; props.numHashBytes = 3; break;
            case kBT4: props.btMode = MF_BinaryTree; props.numHashBytes = 4; break;
            case kHT4: props.btMode = MF_HashTable;  props.numHashBytes = 4; break;
        }
        LzmaEncProps_Normalize(&props);

        SRes res = LzmaEnc_SetProps(enc, &props);
        if (res != SZ_OK) { destroy(); return false; }
        initialized = true;
        return true;
    }

    void destroy() {
        if (enc) { LzmaEnc_Destroy(enc, &g_Alloc4x4, &g_Alloc4x4); enc = nullptr; }
        initialized = false;
    }

    ~LzmaCtx() { destroy(); }
};


struct JobInStream {
    SRes (*Read)(void *p, void *buf, size_t *size);
    uint8_t *rp; int rl;
};

static SRes jobStreamRead(void *p, void *buf, size_t *size) {
    auto *s = (JobInStream*)p;
    int n = (int)*size < s->rl ? (int)*size : s->rl;
    if (n <= 0) { *size = 0; return SZ_OK; }
    memcpy(buf, s->rp, n);
    s->rp += n; s->rl -= n;
    *size = n;
    return SZ_OK;
}

struct JobOutStream {
    size_t (*Write)(void *p, const void *buf, size_t size);
    std::vector<uint8_t> *out;
    int wp;
};

static size_t jobStreamWrite(void *p, const void *buf, size_t size) {
    auto *s = (JobOutStream*)p;
    int need = s->wp + (int)size;
    if (need > (int)s->out->size()) s->out->resize(need * 2);
    memcpy(s->out->data() + s->wp, buf, size);
    s->wp += (int)size;
    return size;
}


struct Job4x4 {
    int                  seq;
    std::vector<uint8_t> in;
    std::vector<uint8_t> out;
    std::string          method;
    bool                 ready;
    uint8_t *rp; int rl;
    int      wp;
};

static int job4x4Cb(const char *what, void *data, int size, void *aux) {
    auto *j = (Job4x4*)aux;
    if (strcmp(what, "read") == 0) {
        int n = std::min(size, j->rl);
        if (n <= 0) return 0;
        memcpy(data, j->rp, n);
        j->rp += n; j->rl -= n;
        return n;
    } else if (strcmp(what, "write") == 0) {
        int need = j->wp + size;
        if (need > (int)j->out.size()) j->out.resize(need * 2);
        memcpy(j->out.data() + j->wp, data, size);
        j->wp += size;
        return size;
    }
    return FREEARC_ERRCODE_NOT_IMPLEMENTED;
}

struct SubBlock {
    int         blkId;        // solid block index
    int         seq;          // sub-block sequence within solid block
    bool        lastInBlock;
    std::vector<uint8_t> in, out;
    std::string method;
    uint8_t    *rp; int rl; int wp; // for generic compress fallback
};


class Method4x4 : public COMPRESSION_METHOD {
public:
    std::string base;
    int         blockSize;
    int         nthreads;
    bool        useLzmaDirect;

    Method4x4(const char *b, int bs, int nt) : base(b), blockSize(bs), nthreads(nt) {
        addtime = -1;
        useLzmaDirect = (strncmp(b, "lzma", 4) == 0);
    }

    int decompress(CALLBACK_FUNC *cb, void *aux) override {
        int errcode = 0;
        for (;;) {
            uint32_t origSz, compSz;
            int r = cb("read", &origSz, 4, aux); if (r != 4) break;
            r = cb("read", &compSz, 4, aux);     if (r != 4) break;

            std::vector<uint8_t> comp(compSz);
            int pos = 0;
            while (pos < (int)compSz) {
                r = cb("read", comp.data()+pos, compSz-pos, aux);
                if (r <= 0) { errcode = FREEARC_ERRCODE_READ; goto done; }
                pos += r;
            }

            char *meth = (char*)comp.data();
            int mlen = strlen(meth) + 1;

            std::vector<uint8_t> orig(origSz);
            int dr = DecompressMem(meth, comp.data()+mlen, compSz-mlen, orig.data(), origSz);
            if (dr < 0) { errcode = dr; goto done; }

            r = cb("write", orig.data(), origSz, aux);
            if (r != (int)origSz) { errcode = FREEARC_ERRCODE_WRITE; goto done; }
        }
        done: return errcode;
    }

#ifndef FREEARC_DECOMPRESS_ONLY

    int compressBlockLzma(LzmaCtx *lctx, Job4x4 *j) {
        JobInStream  inS  = { jobStreamRead,  j->in.data(), (int)j->in.size() };
        JobOutStream outS = { jobStreamWrite, &j->out, 0 };
        j->out.resize(j->in.size() + j->in.size()/8 + 65536);

        SRes res = LzmaEnc_Encode(lctx->enc,
            (ISeqOutStream*)&outS, (ISeqInStream*)&inS,
            nullptr, &g_Alloc4x4, &g_Alloc4x4);

        if (res != SZ_OK) return -1;
        j->out.resize(outS.wp);
        return 0;
    }

    int compressBlockGeneric(Job4x4 *j) {
        j->rp = j->in.data(); j->rl = j->in.size();
        j->wp = 0;
        j->out.resize(j->in.size() + j->in.size()/8 + 65536);
        int r = Compress((char*)j->method.c_str(), job4x4Cb, j);
        if (r < 0) return r;
        j->out.resize(j->wp);
        return 0;
    }

    int compress(CALLBACK_FUNC *cb, void *aux) override {
        std::vector<uint8_t> readBuf(blockSize);
        std::mutex mtx;
        std::condition_variable cvWork, cvWrite, cvFree;
        std::queue<Job4x4*> freeQ, workQ;
        std::vector<Job4x4*> pending;
        std::atomic<bool> allRead{false}, writerDone{false};
        std::atomic<int> errcode{0};
        int seqNum = 0;

        int poolSize = nthreads * 2;
        std::vector<Job4x4> jobs(poolSize);
        for (auto &j : jobs) freeQ.push(&j);

        COMPRESSION_METHOD *baseCm = nullptr;
        if (useLzmaDirect) {
            baseCm = ParseCompressionMethod((char*)base.c_str());
        }

        int nextWrite = 0;
        std::thread writerThread([&]() {
            while (!writerDone || !pending.empty()) {
                std::vector<Job4x4*> local;
                {
                    std::unique_lock<std::mutex> lk(mtx);
                    cvWrite.wait(lk, [&]{ return !pending.empty() || writerDone; });
                    local.swap(pending);
                }

                std::sort(local.begin(), local.end(), [](Job4x4 *a, Job4x4 *b){ return a->seq < b->seq; });
                while (!local.empty() && local.front()->seq == nextWrite) {
                    auto *j = local.front();
                    local.erase(local.begin());

                    uint32_t origSz = j->in.size();
                    uint32_t compSz = j->method.size() + 1 + j->out.size();
                    cb("write", &origSz, 4, aux);
                    cb("write", &compSz, 4, aux);
                    cb("write", (void*)j->method.c_str(), j->method.size()+1, aux);
                    cb("write", j->out.data(), j->out.size(), aux);

                    j->in.clear(); j->out.clear();
                    {
                        std::lock_guard<std::mutex> lk(mtx);
                        freeQ.push(j);
                    }
                    cvFree.notify_one();
                    nextWrite++;
                }

                if (!local.empty()) {
                    std::lock_guard<std::mutex> lk(mtx);
                    pending.insert(pending.end(), local.begin(), local.end());
                }
            }
        });

        auto worker = [&]() {
            LzmaCtx lctx;
            if (useLzmaDirect && baseCm) lctx.init(baseCm);

            for (;;) {
                Job4x4 *j = nullptr;
                {
                    std::unique_lock<std::mutex> lk(mtx);
                    cvWork.wait(lk, [&]{ return !workQ.empty() || allRead; });
                    if (workQ.empty()) return;
                    j = workQ.front(); workQ.pop();
                }

                int r;
                if (j->method != "storing" && isIncompressible(j->in.data(), j->in.size())) {
                    j->out.resize(j->in.size());
                    memcpy(j->out.data(), j->in.data(), j->in.size());
                    j->method = "storing";
                    r = 0;
                } else if (j->method == "storing") {
                    j->out.resize(j->in.size());
                    memcpy(j->out.data(), j->in.data(), j->in.size());
                    r = 0;
                } else if (useLzmaDirect && lctx.initialized) {
                    r = compressBlockLzma(&lctx, j);
                } else {
                    r = compressBlockGeneric(j);
                }

                // if compression expanded the data, store raw
                if (r == 0 && j->method != "storing" && (int)j->out.size() >= (int)j->in.size()) {
                    j->out.resize(j->in.size());
                    memcpy(j->out.data(), j->in.data(), j->in.size());
                    j->method = "storing";
                }

                if (r < 0) {
                    j->out.resize(j->in.size());
                    memcpy(j->out.data(), j->in.data(), j->in.size());
                    j->method = "storing";
                }
                j->ready = true;

                {
                    std::lock_guard<std::mutex> lk(mtx);
                    pending.push_back(j);
                }
                cvWrite.notify_one();
            }
        };

        std::vector<std::thread> workers;
        for (int t = 0; t < nthreads; t++)
            workers.emplace_back(worker);

        for (;;) {
            Job4x4 *j = nullptr;
            {
                std::unique_lock<std::mutex> lk(mtx);
                cvFree.wait(lk, [&]{ return !freeQ.empty(); });
                j = freeQ.front(); freeQ.pop();
            }

            int total = 0;
            while (total < blockSize) {
                int r = cb("read", readBuf.data()+total, blockSize-total, aux);
                if (r <= 0) break;
                total += r;
            }
            if (total == 0) {
                std::lock_guard<std::mutex> lk(mtx);
                freeQ.push(j);
                break;
            }

            j->seq    = seqNum++;
            j->in.assign(readBuf.data(), readBuf.data()+total);
            j->method = base;
            j->ready  = false;

            {
                std::lock_guard<std::mutex> lk(mtx);
                workQ.push(j);
            }
            cvWork.notify_all();
        }

        allRead = true;
        cvWork.notify_all();
        for (auto &w : workers) w.join();

        writerDone = true;
        cvWrite.notify_one();
        writerThread.join();

        if (baseCm) delete baseCm;

        return errcode;
    }

    void ShowCompressionMethod(char *buf) override {
        sprintf(buf, "4x4:%s", base.c_str());
    }
    MemSize GetCompressionMem()   override { return blockSize * nthreads * 3; }
    MemSize GetDecompressionMem() override { return blockSize * 2; }
    MemSize GetDictionary()       override { return blockSize; }
    MemSize GetBlockSize()        override { return blockSize; }
    void SetCompressionMem(MemSize m)   override { blockSize = m / (nthreads*3); }
    void SetDecompressionMem(MemSize m) override {}
    void SetDictionary(MemSize d)       override { blockSize = d; }
    void SetBlockSize(MemSize b)        override { blockSize = b; }
#endif
};

static COMPRESSION_METHOD *parse_4x4(char **params) {
    if (strcmp(params[0], "4x4") != 0) return nullptr;
    // 4x4[:blocksize][:threads]:basemethod[:baseparams]
    int bs = 8*1024*1024, nt = std::thread::hardware_concurrency();
    if (nt <= 0) nt = 4;

    std::string base;
    for (int i = 1; params[i]; i++) {
        if (params[i][0] == 'b' && isdigit(params[i][1])) {
            bs = atoi(params[i]+1) * 1024 * 1024;
        } else if (params[i][0] == 't' && isdigit(params[i][1])) {
            nt = atoi(params[i]+1);
        } else {
            if (!base.empty()) base += ':';
            base += params[i];
        }
    }
    if (base.empty()) base = "lzma:fast";
    return new Method4x4(base.c_str(), bs, nt);
}

static int _4x4_reg = AddCompressionMethod(parse_4x4);
