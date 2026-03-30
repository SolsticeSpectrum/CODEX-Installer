// mkarc — FreeArc-compatible archive creator for Linux
// 4x4 parallel compression, method chaining, solid block splitting
//
// usage: mkarc [-m<method>] [-s<solidMB>] [-v<volMB>] [-t<threads>] <srcdir> <output.bin>

#include <sys/stat.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>
#include <algorithm>
#include <filesystem>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <queue>

#include "Environment.h"
#include "Compression/Compression.h"
#undef stat

extern void InitCRC();
extern uint CalcCRC(void *Addr, uint Size);
extern uint UpdateCRC(void *Addr, uint Size, uint StartCRC);

namespace fs = std::filesystem;

#define ARC_SIG         0x01437241
#define ARC_VER         0x07060000
#define CRC_INIT        0xFFFFFFFF

enum BType { BT_DESCR=0, BT_HEADER=1, BT_DATA=2, BT_DIR=3, BT_FOOTER=4 };


// ============================================================
// varint encoder (matches ArcStructure.h readInteger)
// ============================================================

struct WBuf : std::vector<uint8_t> {
    void vi(uint64_t x) {
        if      (x < (1ull<< 7)) { uint8_t  v=x<<1;      ap(&v,1); }
        else if (x < (1ull<<14)) { uint16_t v=(x<<2)|1;   ap((uint8_t*)&v,2); }
        else if (x < (1ull<<21)) { uint32_t v=(x<<3)|3;   ap((uint8_t*)&v,3); }
        else if (x < (1ull<<28)) { uint32_t v=(x<<4)|7;   ap((uint8_t*)&v,4); }
        else if (x < (1ull<<35)) { uint64_t v=(x<<5)|15;  ap((uint8_t*)&v,5); }
        else if (x < (1ull<<42)) { uint64_t v=(x<<6)|31;  ap((uint8_t*)&v,6); }
        else if (x < (1ull<<49)) { uint64_t v=(x<<7)|63;  ap((uint8_t*)&v,7); }
        else if (x < (1ull<<56)) { uint64_t v=(x<<8)|127; ap((uint8_t*)&v,8); }
        else { push_back(255); ap((uint8_t*)&x,8); }
    }
    void u32(uint32_t v) { ap((uint8_t*)&v,4); }
    void u8(uint8_t v)   { push_back(v); }
    void str(const char *s) { while(*s) push_back(*s++); push_back(0); }
    void ap(const uint8_t *p, int n) { insert(end(),p,p+n); }
};


// ============================================================
// file groups (FreeArc arc.groups classification)
// ============================================================

enum FileGroup {
    GRP_COMPRESSED = 0, // already compressed — store raw
    GRP_PRECOMP,        // zip/png/pdf — precomp can help but lzma alone won't
    GRP_JPG,            // jpeg — already compressed
    GRP_WAV,            // raw audio — TTA or delta+lzma
    GRP_BMP,            // raw bitmaps — delta+lzma
    GRP_TEXT,            // source code, markup, config
    GRP_EXE,            // executables — BCJ+lzma
    GRP_OBJ,            // object files
    GRP_BINARY,          // default catch-all
    GRP_COUNT
};

static const char *grpName[] = {
    "compressed","precomp","jpg","wav","bmp","text","exe","obj","binary"
};

// extension → group mapping (based on FreeArc arc.groups)
static bool extMatch(const char *ext, const char *list) {
    // list is space-separated extensions without dots
    char buf[4096]; strncpy(buf, list, sizeof(buf)-1); buf[sizeof(buf)-1]=0;
    for (char *t = strtok(buf," "); t; t = strtok(nullptr," "))
        if (strcasecmp(ext, t) == 0) return true;
    return false;
}

static FileGroup classifyExt(const std::string &name) {
    auto dot = name.rfind('.');
    if (dot == std::string::npos) return GRP_TEXT; // extensionless → text
    std::string ext = name.substr(dot+1);
    const char *e = ext.c_str();

    // already compressed — storing only
    if (extMatch(e, "mp3 mp4 mkv avi flac ogg opus aac wma wmv m4a m4v "
                     "webm webp avif heif heic "
                     "7z rar zip gz bz2 xz zst lz4 lzma lzo "
                     "arc arj lzh cab zoo pak hpk "
                     "pmd pmm pms ccm ccmx djvu chm "
                     "br snappy")) return GRP_COMPRESSED;

    // jpeg
    if (extMatch(e, "jpg jpeg jfif")) return GRP_JPG;

    // precomp candidates (containers with internal compression)
    if (extMatch(e, "pdf swf zip jar png gif gz tgz svgz "
                     "docx docm dotx dotm xlsx xlsm xltx xltm xlam "
                     "pptx pptm potx potm ppam ppsx ppsm "
                     "odt ott ods ots odg odp odf odb oxt "
                     "apk ipa xpi crx nupkg whl egg "
                     "fb2z sis gadget "
                     "pk3 pk4 pak")) return GRP_PRECOMP;

    // raw audio
    if (extMatch(e, "wav wave pcm aif aifc aiff au snd raw")) return GRP_WAV;

    // raw bitmaps
    if (extMatch(e, "bmp tif tiff tga wbm pgm pnm ppm dds")) return GRP_BMP;

    // executables
    if (extMatch(e, "exe dll so com scr sfx ocx bpl dpl "
                     "sys drv vxd ovr ovl")) return GRP_EXE;

    // object files
    if (extMatch(e, "obj o a lib dcu")) return GRP_OBJ;

    // text / source code
    if (extMatch(e, "txt asc lng css htm html xml xsl json yaml yml toml "
                     "md rst tex log ini cfg conf "
                     "c cpp cxx cc h hpp hxx ipp "
                     "cs java js ts jsx tsx "
                     "py rb pl pm lua tcl sh bash zsh fish "
                     "php asp asm s inc "
                     "pas dfm lfm lpi "
                     "hs lhs ml mli erl hrl scm "
                     "sql csv rtf "
                     "bat cmd vbs ps1 "
                     "makefile cmake "
                     "rc idl dsp dsw vcproj sln csproj")) return GRP_TEXT;

    return GRP_BINARY;
}


// ============================================================
// file entry + collection
// ============================================================

struct FE {
    std::string rel, dir, name;
    uint64_t  sz;
    uint32_t  mt, crc;
    bool      isdir;
    int       block;    // which solid block
    FileGroup group;
};

static void collect(const std::string &root, std::vector<FE> &out) {
    for (auto &e : fs::recursive_directory_iterator(root)) {
        FE f;
        f.rel   = fs::relative(e.path(), root).string();
        f.isdir = e.is_directory();
        f.sz    = f.isdir ? 0 : e.file_size();
        auto s  = f.rel.rfind('/');
        f.dir   = s != std::string::npos ? f.rel.substr(0,s) : "";
        f.name  = s != std::string::npos ? f.rel.substr(s+1) : f.rel;
        struct ::stat st; ::stat(e.path().c_str(), &st);
        f.mt    = (uint32_t)st.st_mtime;
        f.crc   = 0;
        f.block = -1;
        f.group = f.isdir ? GRP_BINARY : classifyExt(f.name);
        out.push_back(f);
    }
    // sort: dirs first, then by group, then by extension, then by path
    std::sort(out.begin(), out.end(), [](const FE &a, const FE &b) {
        if (a.isdir != b.isdir) return a.isdir > b.isdir;
        if (a.group != b.group) return a.group < b.group;
        auto ea = a.name.rfind('.'), eb = b.name.rfind('.');
        std::string xa = ea!=std::string::npos ? a.name.substr(ea) : "";
        std::string xb = eb!=std::string::npos ? b.name.substr(eb) : "";
        if (xa != xb) return xa < xb;
        return a.rel < b.rel;
    });
}


// ============================================================
// fast incompressibility detection (based on fast-lzma2)
// byte frequency deviation: uniform distribution = incompressible
// ============================================================

// byte frequency deviation test (fast-lzma2 algorithm)
// counts byte frequencies scaled by 4, compares deviation from uniform
// distribution against threshold. truly random/encrypted data has near-uniform
// distribution (low deviation), compressible data has skewed distribution (high deviation).
static bool isIncompressible(const uint8_t *data, int size) {
    if (size < 4096) return false;

    int sampleSz = std::min(size, 16384);
    uint32_t avg = sampleSz / 64; // expected freq*4 for uniform distribution

    uint32_t freq[256] = {};
    for (int i = 0; i < sampleSz; i++)
        freq[data[i]] += 4;

    uint64_t charTotal = 0;
    for (int i = 0; i < 256; i++) {
        int64_t delta = (int64_t)freq[i] - avg;
        charTotal += (uint64_t)(delta * delta);
    }

    // sqrt(charTotal) / sqrt(sampleSz) <= 24 (fast-lzma2 threshold)
    // equivalently: charTotal <= 576 * sampleSz
    return charTotal <= (uint64_t)576 * sampleSz;
}


// ============================================================
// per-group method selection
// ============================================================

// given the user's base method and a file group, return the actual method
// for that group's solid block. returns "storing" for already-compressed data.
static std::string methodForGroup(const char *userMethod, FileGroup grp) {
    switch (grp) {
        case GRP_COMPRESSED:
        case GRP_JPG:
            return "storing";

        case GRP_PRECOMP:
            // precomp containers — light compression only
            // if user specified 4x4:..., extract the base and use rep or light lzma
            return "storing";

        case GRP_WAV:
        case GRP_BMP:
            // raw multimedia — delta filter helps but we keep user method
            // TODO: delta+lzma chain
            return userMethod;

        default:
            return userMethod;
    }
}


// ============================================================
// 4x4 parallel compression via COMPRESSION_METHOD
// ============================================================

// LZMA SDK direct access for persistent encoder reuse
#include "Compression/LZMA2/C_LZMA.h"
#include "Compression/LZMA2/C/LzmaEnc.h"

// matchfinder constants (from C_LZMA.cpp)
#define MF_HashChain  0
#define MF_BinaryTree 1
#define MF_HashTable  2
enum { kBT2=0, kBT3, kBT4, kHC4, kHT4 };

static void *SzAlloc4x4(void *, size_t size) { return size ? malloc(size) : nullptr; }
static void  SzFree4x4(void *, void *addr)   { free(addr); }
static ISzAlloc g_Alloc4x4 = { SzAlloc4x4, SzFree4x4 };

// persistent LZMA encoder — created once per worker, reused across blocks
struct LzmaCtx {
    CLzmaEncHandle    enc;
    CLzmaEncProps     props;
    bool              initialized;

    LzmaCtx() : enc(nullptr), initialized(false) {}

    bool init(COMPRESSION_METHOD *cm) {
        if (initialized) return true;
        enc = LzmaEnc_Create(&g_Alloc4x4);
        if (!enc) return false;

        auto *lm = (LZMA_METHOD*)cm;
        LzmaEncProps_Init(&props);
        props.dictSize   = lm->dictionarySize;
        props.hashSize   = lm->hashSize;
        props.algo       = lm->algorithm;
        props.fb         = lm->numFastBytes;
        props.mc         = lm->matchFinderCycles;
        props.lc         = lm->litContextBits;
        props.lp         = lm->litPosBits;
        props.pb         = lm->posStateBits;
        props.numThreads = 1;    // 4x4 provides parallelism at block level
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

// ISeqInStream/ISeqOutStream wrappers that read/write from job buffers
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
    // per-job read/write state for generic Compress() fallback
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

class Method4x4 : public COMPRESSION_METHOD {
public:
    std::string base;
    int         blockSize;
    int         nthreads;
    bool        useLzmaDirect; // true if base method is lzma (use persistent encoder)

    Method4x4(const char *b, int bs, int nt) : base(b), blockSize(bs), nthreads(nt) {
        addtime = -1;
        useLzmaDirect = (strncmp(b, "lzma", 4) == 0);
    }

    int decompress(CALLBACK_FUNC *cb, void *aux) override {
        // read framed blocks, decompress each
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

    // compress one block using persistent LZMA encoder (no create/destroy overhead)
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

    // compress one block using generic FreeArc Compress() (fallback for non-lzma methods)
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

        // parse base method for LZMA encoder init
        COMPRESSION_METHOD *baseCm = nullptr;
        if (useLzmaDirect) {
            baseCm = ParseCompressionMethod((char*)base.c_str());
        }

        // dedicated writer thread — outputs completed blocks in order
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

        // worker threads — each owns a persistent LZMA encoder
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
                // fast incompressibility check — skip LZMA for random/encrypted data
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

                // if compression expanded the data, store raw instead
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

        // reader thread (main) — reads blocks, dispatches to workers
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

    // rebuild base method from remaining params
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


// ============================================================
// MultiCompress: pipeline chaining for "rep+lzma" etc
// ============================================================

struct MCParam {
    std::thread thr;
    int         idx, total;
    char       *method;
    CALLBACK_FUNC *origCb;
    void          *origAux;

    // inter-thread pipe
    uint8_t *pipeBuf;
    int      pipeLen;   // -1 = EOF
    std::mutex mtx;
    std::condition_variable cvR, cvW;
    bool rReady, wReady;
};

static int mcCallback(const char *what, void *data, int size, void *aux) {
    auto *p = (MCParam*)aux;

    if (strcmp(what,"write")==0 && p->idx < p->total-1) {
        // pass to next thread
        p->pipeBuf = (uint8_t*)data;
        p->pipeLen = size;
        { std::lock_guard<std::mutex> lk(p->mtx); p->rReady = true; }
        p->cvR.notify_one();
        std::unique_lock<std::mutex> lk(p->mtx);
        p->cvW.wait(lk, [p]{ return p->wReady; });
        p->wReady = false;
        return p->pipeLen < 0 ? FREEARC_ERRCODE_WRITE : size;
    }
    else if (strcmp(what,"read")==0 && p->idx > 0) {
        auto *prev = p-1;
        int total = 0;
        auto *dst = (uint8_t*)data;
        while (total < size) {
            std::unique_lock<std::mutex> lk(prev->mtx);
            prev->cvR.wait(lk, [prev]{ return prev->rReady; });
            if (prev->pipeLen < 0) { prev->rReady = true; prev->cvR.notify_one(); return total; }
            int n = std::min(size-total, prev->pipeLen);
            memcpy(dst+total, prev->pipeBuf, n);
            prev->pipeBuf += n;
            prev->pipeLen -= n;
            total += n;
            if (prev->pipeLen == 0) {
                prev->rReady = false;
                prev->wReady = true;
                prev->cvW.notify_one();
            } else {
                prev->rReady = true; prev->cvR.notify_one();
            }
        }
        return total;
    }
    else {
        return p->origCb(what, data, size, p->origAux);
    }
}

static int MultiCompress(char *str, CALLBACK_FUNC *cb, void *aux) {
    char *m = strdup(str);
    char *methods[32]; int N = 0;
    for (char *t = strtok(m,"+"); t && N < 32; t = strtok(nullptr,"+"))
        methods[N++] = t;
    if (N <= 1) { int r = Compress(m, cb, aux); free(m); return r; }

    MCParam params[32];
    std::atomic<int> ret{0};
    for (int i = 0; i < N; i++) {
        params[i].idx = i; params[i].total = N;
        params[i].method = methods[i];
        params[i].origCb = cb; params[i].origAux = aux;
        params[i].rReady = false; params[i].wReady = false;
    }
    for (int i = 0; i < N; i++) {
        params[i].thr = std::thread([&params,i,&ret](){
            int r = Compress(params[i].method, mcCallback, &params[i]);
            if (r < 0) ret = r;
            if (i < params[i].total-1) {
                std::lock_guard<std::mutex> lk(params[i].mtx);
                params[i].pipeLen = -1;
                params[i].rReady = true;
                params[i].cvR.notify_one();
            }
        });
    }
    for (int i = 0; i < N; i++) params[i].thr.join();
    free(m);
    return ret;
}


// ============================================================
// streaming compress context (reads files sequentially)
// ============================================================

struct CCtx {
    std::vector<FE> *files;
    int startFile, endFile; // range within files[]
    std::string srcDir;
    int curFile;
    FILE *curFP;
    uint64_t remain;
    uint32_t crc;
    FILE *outFP;
    uint64_t written;
};

static int streamCb(const char *what, void *data, int size, void *aux) {
    auto *c = (CCtx*)aux;
    if (strcmp(what,"read")==0) {
        int total = 0;
        auto *dst = (uint8_t*)data;
        while (total < size) {
            if (!c->curFP) {
                while (c->curFile < c->endFile) {
                    auto &f = (*c->files)[c->curFile];
                    if (f.isdir) { c->curFile++; continue; }
                    c->curFP = fopen((c->srcDir+"/"+f.rel).c_str(), "rb");
                    c->remain = f.sz;
                    c->crc = CRC_INIT;
                    break;
                }
                if (!c->curFP) break;
            }
            int want = std::min((uint64_t)(size-total), std::min(c->remain,(uint64_t)0x7FFFFFFF));
            int got = fread(dst+total, 1, want, c->curFP);
            if (got <= 0) break;
            c->remain -= got;
            total += got;
            c->crc = UpdateCRC(dst+total-got, got, c->crc);
            if (c->remain == 0) {
                fclose(c->curFP); c->curFP = nullptr;
                (*c->files)[c->curFile].crc = c->crc ^ CRC_INIT;
                c->curFile++;
            }
        }
        return total;
    } else if (strcmp(what,"write")==0) {
        int w = fwrite(data, 1, size, c->outFP);
        c->written += w;
        return w;
    }
    return FREEARC_ERRCODE_NOT_IMPLEMENTED;
}


// ============================================================
// block descriptor
// ============================================================

struct BInfo { int type; std::string comp; uint64_t pos, orig, csz; uint32_t crc; };

static void writeDescr(FILE *f, const BInfo &b) {
    WBuf d;
    d.u32(ARC_SIG); d.vi(b.type); d.str(b.comp.c_str());
    d.vi(b.orig); d.vi(b.csz); d.u32(b.crc);
    d.u32(CalcCRC(d.data(), d.size()));
    fwrite(d.data(), 1, d.size(), f);
}

static int compressSmall(const char *m, const uint8_t *in, int sz, std::vector<uint8_t> &out) {
    out.resize(sz*2+65536);
    int r = CompressMem((char*)m, (void*)in, sz, out.data(), out.size());
    if (r > 0) out.resize(r); return r;
}


// ============================================================
// main
// ============================================================

int main(int argc, char *argv[]) {
    const char *method = "4x4:lzma:fast", *srcDir = nullptr, *outPath = nullptr;
    int solidMB = 128, volumeMB = 0;

    for (int i = 1; i < argc; i++) {
        if (argv[i][0] != '-') { if (!srcDir) srcDir = argv[i]; else outPath = argv[i]; }
        else switch(argv[i][1]) {
            case 'm': method = argv[i]+2; break;
            case 's': solidMB = atoi(argv[i]+2); break;
            case 'v': volumeMB = atoi(argv[i]+2); break;
            default: fprintf(stderr,"unknown: %s\n",argv[i]); return 1;
        }
    }
    if (!srcDir||!outPath) {
        fprintf(stderr,"usage: mkarc [-m<method>] [-s<solidMB>] [-v<volMB>] <srcdir> <out.bin>\n");
        fprintf(stderr,"  default method: 4x4:lzma:fast (parallel LZMA)\n");
        fprintf(stderr,"  examples: -mlzma  -mrep+lzma  -m4x4:lzma:max  -mstoring\n");
        return 1;
    }

    InitCRC();
    // 4x4 handles parallelism at block level, so LZMA uses single thread per instance
    SetCompressionThreads(1);

    std::vector<FE> files;
    collect(srcDir, files);

    uint64_t totalOrig = 0;
    int nFiles = 0;
    for (auto &f : files) if (!f.isdir) { totalOrig += f.sz; nFiles++; }

    // split into solid blocks (break on group boundary or size limit)
    uint64_t solidLimit = (uint64_t)solidMB * 1024 * 1024;
    int nBlocks = 0;
    {
        uint64_t blockSz = 0;
        FileGroup curGrp = GRP_BINARY;
        bool first = true;
        for (auto &f : files) {
            if (f.isdir) { f.block = nBlocks; continue; }
            // start new block on group change or size overflow
            if (!first && (f.group != curGrp || (blockSz > 0 && blockSz + f.sz > solidLimit))) {
                nBlocks++; blockSz = 0;
            }
            f.block = nBlocks;
            curGrp = f.group;
            blockSz += f.sz;
            first = false;
        }
        nBlocks++;
    }

    // count files per group for stats
    int grpCount[GRP_COUNT] = {};
    uint64_t grpSize[GRP_COUNT] = {};
    for (auto &f : files) if (!f.isdir) { grpCount[f.group]++; grpSize[f.group] += f.sz; }

    printf("%d files, %llu bytes, %d solid blocks, method: %s\n",
           nFiles, (unsigned long long)totalOrig, nBlocks, method);
    for (int g = 0; g < GRP_COUNT; g++) {
        if (grpCount[g] > 0)
            printf("  %-12s: %3d files, %7.1f MB\n", grpName[g], grpCount[g], grpSize[g]/1048576.0);
    }

    // unique dirs
    std::vector<std::string> dirs;
    for (auto &f : files)
        if (std::find(dirs.begin(),dirs.end(),f.dir)==dirs.end())
            dirs.push_back(f.dir);

    FILE *out = fopen(outPath, "wb");
    if (!out) { fprintf(stderr,"can't create: %s\n",outPath); return 1; }

    std::vector<BInfo> ctrlBlocks;
    std::vector<BInfo> dataBlocks;

    // HEADER_BLOCK
    {
        WBuf h; h.u32(ARC_SIG); h.u32(ARC_VER);
        uint32_t c = CalcCRC(h.data(), h.size());
        uint64_t p = ftell(out);
        fwrite(h.data(),1,h.size(),out);
        BInfo bi = {BT_HEADER,"storing",p,(uint64_t)h.size(),(uint64_t)h.size(),c};
        writeDescr(out,bi);
        ctrlBlocks.push_back(bi);
    }

    // pre-compute block metadata
    struct BlkInfo { int startF, endF; uint64_t origSz; FileGroup grp; std::string method; };
    std::vector<BlkInfo> blkInfos(nBlocks);
    for (int blk = 0; blk < nBlocks; blk++) {
        auto &bi = blkInfos[blk];
        bi.startF = -1; bi.endF = -1; bi.origSz = 0; bi.grp = GRP_BINARY;
        for (int i = 0; i < (int)files.size(); i++) {
            if (files[i].block == blk) {
                if (bi.startF < 0) bi.startF = i;
                bi.endF = i + 1;
            }
        }
        for (int i = bi.startF; i < bi.endF; i++) {
            if (!files[i].isdir) { bi.origSz += files[i].sz; if (bi.grp == GRP_BINARY) bi.grp = files[i].group; }
        }
        bi.method = methodForGroup(method, bi.grp);
    }

    // check if we're using a 4x4 method — if so, use global pipeline
    bool use4x4 = (strncmp(method, "4x4", 3) == 0);
    std::string baseMethod;
    int subBlockSize = 8*1024*1024;
    int nWorkers = std::thread::hardware_concurrency();
    if (nWorkers <= 0) nWorkers = 4;

    if (use4x4) {
        // parse 4x4 params from method string
        COMPRESSION_METHOD *parsed = ParseCompressionMethod((char*)method);
        if (parsed) {
            char buf[256]; parsed->ShowCompressionMethod(buf);
            // extract base from "4x4:base"
            char *colon = strchr(buf, ':');
            baseMethod = colon ? colon+1 : "lzma:fast";
            subBlockSize = parsed->GetBlockSize();
            if (subBlockSize <= 0) subBlockSize = 8*1024*1024;
            delete parsed;
        } else {
            baseMethod = "lzma:fast";
        }
    }

    if (use4x4) {
        // ============================================================
        // GLOBAL PIPELINE: reader → workers → writer across ALL blocks
        // no thread teardown between blocks
        // ============================================================

        // sub-block job: tagged with solid block ID for ordered output
        struct SubBlock {
            int blkId;              // solid block index
            int seq;                // sub-block sequence within solid block
            bool lastInBlock;       // last sub-block of this solid block
            std::vector<uint8_t> in, out;
            std::string method;
            uint8_t *rp; int rl; int wp; // for generic compress fallback
        };

        std::mutex mtx;
        std::condition_variable cvWork, cvWrite, cvFree;
        int poolSize = nWorkers * 2 + 2;
        std::vector<SubBlock> pool(poolSize);
        std::queue<SubBlock*> freeQ, workQ;
        std::vector<SubBlock*> pending;
        std::atomic<bool> allRead{false}, writerDone{false};
        for (auto &sb : pool) freeQ.push(&sb);

        // parse base LZMA method for persistent encoders
        bool useLzma = (strncmp(baseMethod.c_str(), "lzma", 4) == 0);
        COMPRESSION_METHOD *baseCm = nullptr;
        if (useLzma) baseCm = ParseCompressionMethod((char*)baseMethod.c_str());

        // writer thread: outputs sub-blocks in order per solid block
        // tracks block boundaries, writes DESCR at block transitions
        int curWriteBlk = 0, nextSeqInBlk = 0;
        std::thread writerThread([&]() {
            uint64_t blockDataStart = ftell(out);

            while (true) {
                std::vector<SubBlock*> local;
                {
                    std::unique_lock<std::mutex> lk(mtx);
                    cvWrite.wait(lk, [&]{ return !pending.empty() || writerDone; });
                    if (pending.empty() && writerDone) break;
                    local.swap(pending);
                }

                // sort by (blkId, seq)
                std::sort(local.begin(), local.end(), [](SubBlock *a, SubBlock *b){
                    return a->blkId != b->blkId ? a->blkId < b->blkId : a->seq < b->seq;
                });

                std::vector<SubBlock*> deferred;
                for (auto *sb : local) {
                    if (sb->blkId != curWriteBlk || sb->seq != nextSeqInBlk) {
                        deferred.push_back(sb);
                        continue;
                    }

                    // write 4x4 frame: [origSz][compSz][method\0][data]
                    uint32_t origSz = sb->in.size();
                    uint32_t compSz = sb->method.size() + 1 + sb->out.size();
                    fwrite(&origSz, 4, 1, out);
                    fwrite(&compSz, 4, 1, out);
                    fwrite(sb->method.c_str(), sb->method.size()+1, 1, out);
                    fwrite(sb->out.data(), sb->out.size(), 1, out);

                    nextSeqInBlk++;

                    bool wasLast = sb->lastInBlock;
                    sb->in.clear(); sb->out.clear();
                    {
                        std::lock_guard<std::mutex> lk2(mtx);
                        freeQ.push(sb);
                    }
                    cvFree.notify_one();

                    if (wasLast) {
                        // block boundary: write DESCR and advance
                        uint64_t dataEnd = ftell(out);
                        auto &bi = blkInfos[curWriteBlk];
                        BInfo binfo = {BT_DATA, std::string("4x4:") + baseMethod,
                                       blockDataStart, bi.origSz, dataEnd - blockDataStart, 0};
                        writeDescr(out, binfo);
                        dataBlocks.push_back(binfo);

                        curWriteBlk++;
                        nextSeqInBlk = 0;
                        blockDataStart = ftell(out);
                    }
                }

                if (!deferred.empty()) {
                    std::lock_guard<std::mutex> lk(mtx);
                    pending.insert(pending.end(), deferred.begin(), deferred.end());
                }
            }
        });

        // worker threads: persistent LZMA encoders, never torn down between blocks
        auto workerFn = [&]() {
            LzmaCtx lctx;
            if (useLzma && baseCm) lctx.init(baseCm);

            for (;;) {
                SubBlock *sb = nullptr;
                {
                    std::unique_lock<std::mutex> lk(mtx);
                    cvWork.wait(lk, [&]{ return !workQ.empty() || allRead; });
                    if (workQ.empty()) return;
                    sb = workQ.front(); workQ.pop();
                }

                if (sb->method == "storing") {
                    sb->out.resize(sb->in.size());
                    memcpy(sb->out.data(), sb->in.data(), sb->in.size());
                } else if (isIncompressible(sb->in.data(), sb->in.size())) {
                    sb->out.resize(sb->in.size());
                    memcpy(sb->out.data(), sb->in.data(), sb->in.size());
                    sb->method = "storing";
                } else if (useLzma && lctx.initialized) {
                    JobInStream inS = { jobStreamRead, sb->in.data(), (int)sb->in.size() };
                    JobOutStream outS = { jobStreamWrite, &sb->out, 0 };
                    sb->out.resize(sb->in.size() + sb->in.size()/8 + 65536);
                    SRes res = LzmaEnc_Encode(lctx.enc,
                        (ISeqOutStream*)&outS, (ISeqInStream*)&inS,
                        nullptr, &g_Alloc4x4, &g_Alloc4x4);
                    if (res != SZ_OK || outS.wp >= (int)sb->in.size()) {
                        sb->out.resize(sb->in.size());
                        memcpy(sb->out.data(), sb->in.data(), sb->in.size());
                        sb->method = "storing";
                    } else {
                        sb->out.resize(outS.wp);
                    }
                } else {
                    // generic fallback using job4x4Cb
                    Job4x4 j4;
                    j4.method = sb->method;
                    j4.rp = sb->in.data(); j4.rl = sb->in.size(); j4.wp = 0;
                    j4.out.resize(sb->in.size() + sb->in.size()/8 + 65536);
                    j4.in = sb->in; // keep sb->in intact
                    int r = Compress((char*)j4.method.c_str(), job4x4Cb, &j4);
                    if (r < 0 || j4.wp >= (int)sb->in.size()) {
                        sb->out.resize(sb->in.size());
                        memcpy(sb->out.data(), sb->in.data(), sb->in.size());
                        sb->method = "storing";
                    } else {
                        j4.out.resize(j4.wp);
                        sb->out = std::move(j4.out);
                    }
                }

                {
                    std::lock_guard<std::mutex> lk(mtx);
                    pending.push_back(sb);
                }
                cvWrite.notify_one();
            }
        };

        std::vector<std::thread> workers;
        for (int t = 0; t < nWorkers; t++)
            workers.emplace_back(workerFn);

        // reader: iterate ALL solid blocks, read sub-blocks, dispatch to workers
        std::vector<uint8_t> readBuf(subBlockSize);
        for (int blk = 0; blk < nBlocks; blk++) {
            auto &bi = blkInfos[blk];
            if (bi.startF < 0 || bi.origSz == 0) continue; // skip empty blocks

            printf("block %d/%d [%s]: %llu bytes (%s), files %d-%d\n",
                   blk+1, nBlocks, grpName[bi.grp], (unsigned long long)bi.origSz,
                   bi.method.c_str(), bi.startF, bi.endF-1);

            // set up file reader for this block
            CCtx ctx = {};
            ctx.files = &files; ctx.startFile = bi.startF; ctx.endFile = bi.endF;
            ctx.curFile = bi.startF; ctx.srcDir = srcDir; ctx.curFP = nullptr;

            // determine sub-block method
            std::string subMethod = (bi.method == "storing") ? "storing" : baseMethod;

            int seq = 0;
            uint64_t remaining = bi.origSz;
            while (remaining > 0) {
                SubBlock *sb = nullptr;
                {
                    std::unique_lock<std::mutex> lk(mtx);
                    cvFree.wait(lk, [&]{ return !freeQ.empty(); });
                    sb = freeQ.front(); freeQ.pop();
                }

                int wantBytes = (int)std::min((uint64_t)subBlockSize, remaining);
                int total = 0;
                while (total < wantBytes) {
                    int r = streamCb("read", readBuf.data()+total, wantBytes-total, &ctx);
                    if (r <= 0) break;
                    total += r;
                }
                remaining -= total;

                sb->blkId = blk;
                sb->seq = seq++;
                sb->in.assign(readBuf.data(), readBuf.data()+total);
                sb->method = subMethod;
                sb->lastInBlock = (remaining == 0);

                {
                    std::lock_guard<std::mutex> lk(mtx);
                    workQ.push(sb);
                }
                cvWork.notify_all();
            }
        }

        allRead = true;
        cvWork.notify_all();
        for (auto &w : workers) w.join();

        writerDone = true;
        cvWrite.notify_one();
        writerThread.join();

        if (baseCm) delete baseCm;

    } else {
        // non-4x4 method: sequential per-block Compress()
        for (int blk = 0; blk < nBlocks; blk++) {
            auto &bi = blkInfos[blk];
            if (bi.startF < 0) continue;

            printf("block %d/%d [%s]: %llu bytes (%s), files %d-%d\n",
                   blk+1, nBlocks, grpName[bi.grp], (unsigned long long)bi.origSz,
                   bi.method.c_str(), bi.startF, bi.endF-1);

            uint64_t dataPos = ftell(out);
            CCtx ctx = {};
            ctx.files = &files; ctx.startFile = bi.startF; ctx.endFile = bi.endF;
            ctx.curFile = bi.startF; ctx.srcDir = srcDir; ctx.curFP = nullptr; ctx.outFP = out;

            const char *m = bi.method.c_str();
            int result;
            if (strchr(m, '+')) result = MultiCompress((char*)m, streamCb, &ctx);
            else result = Compress((char*)m, streamCb, &ctx);

            if (result < 0) { fprintf(stderr,"compression failed: %d\n", result); fclose(out); return 1; }

            uint64_t dataEnd = ftell(out);
            BInfo binfo = {BT_DATA, bi.method, dataPos, bi.origSz, dataEnd-dataPos, 0};
            writeDescr(out, binfo);
            dataBlocks.push_back(binfo);
        }
    }

    // DIR_BLOCK
    uint64_t dirPos = ftell(out);
    {
        WBuf db;
        db.vi(nBlocks);

        // num_of_files per block (cumulative in FreeArc format)
        for (int blk = 0; blk < nBlocks; blk++) {
            int cnt = 0;
            for (auto &f : files) if (f.block == blk) cnt++;
            db.vi(cnt);
        }
        // compressor per block (uses actual method from dataBlocks)
        for (int blk = 0; blk < nBlocks; blk++)
            db.str(dataBlocks[blk].comp.c_str());
        // offset per block (relative backward from dirPos)
        for (int blk = 0; blk < nBlocks; blk++)
            db.vi(dirPos - dataBlocks[blk].pos);
        // compsize per block
        for (int blk = 0; blk < nBlocks; blk++)
            db.vi(dataBlocks[blk].csz);

        // directories
        db.vi((int)dirs.size());
        for (auto &d : dirs) db.str(d.c_str());

        // per-file data (ALL files, across ALL blocks, in order)
        for (auto &f : files) db.str(f.name.c_str());
        for (auto &f : files) {
            int dn = std::find(dirs.begin(),dirs.end(),f.dir) - dirs.begin();
            db.vi(dn);
        }
        for (auto &f : files) db.vi(f.sz);
        for (auto &f : files) db.u32(f.mt);
        for (auto &f : files) db.u8(f.isdir ? 1 : 0);
        for (auto &f : files) db.u32(f.crc);

        uint32_t dirOrig = db.size();
        uint32_t dirCrc  = CalcCRC(db.data(), dirOrig);
        std::vector<uint8_t> dirC;
        int dirCSz = compressSmall("storing", db.data(), dirOrig, dirC);
        if (dirCSz < 0) { fprintf(stderr,"dir failed\n"); return 1; }
        fwrite(dirC.data(), 1, dirCSz, out);

        BInfo bi = {BT_DIR,"storing",dirPos,(uint64_t)dirOrig,(uint64_t)dirCSz,dirCrc};
        writeDescr(out,bi);
        ctrlBlocks.push_back(bi);
    }

    // FOOTER_BLOCK
    uint64_t footerPos = ftell(out);
    {
        WBuf fb;
        fb.vi((int)ctrlBlocks.size());
        for (auto &cb : ctrlBlocks) {
            fb.vi(cb.type);
            fb.str(cb.comp.c_str());
            fb.vi(footerPos - cb.pos);
            fb.vi(cb.orig);
            fb.vi(cb.csz);
            fb.u32(cb.crc);
        }
        fb.u8(0); // not locked
        fb.vi(0); // no comment

        uint32_t fOrig = fb.size();
        uint32_t fCrc  = CalcCRC(fb.data(), fOrig);
        std::vector<uint8_t> fC;
        int fCSz = compressSmall("storing", fb.data(), fOrig, fC);
        if (fCSz < 0) { fprintf(stderr,"footer failed\n"); return 1; }
        fwrite(fC.data(), 1, fCSz, out);

        BInfo bi = {BT_FOOTER,"storing",footerPos,(uint64_t)fOrig,(uint64_t)fCSz,fCrc};
        writeDescr(out,bi);
    }

    fclose(out);
    uint64_t total = fs::file_size(outPath);
    printf("done: %llu -> %llu bytes (%.1f%%)\n",
           (unsigned long long)totalOrig, (unsigned long long)total,
           totalOrig>0 ? 100.0*total/totalOrig : 0.0);

    return 0;
}
