#pragma once

#include <sys/stat.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>
#include <algorithm>
#include <filesystem>

#include "Environment.h"
#include "Compression/Compression.h"
#undef stat

#include "groups.h"

extern void InitCRC();
extern uint CalcCRC(void *Addr, uint Size);
extern uint UpdateCRC(void *Addr, uint Size, uint StartCRC);

namespace fs = std::filesystem;

#define ARC_SIG   0x01437241
#define ARC_VER   0x07060000
#define CRC_INIT  0xFFFFFFFF


enum BType { BT_DESCR = 0, BT_HEADER = 1, BT_DATA = 2, BT_DIR = 3, BT_FOOTER = 4 };


// varint encoding matching ArcStructure.h readInteger
struct WBuf : std::vector<uint8_t> {
    void vi(uint64_t x) {
        if      (x < (1ull <<  7)) { uint8_t  v =  x << 1;        ap(&v, 1);            }
        else if (x < (1ull << 14)) { uint16_t v = (x << 2) | 1;   ap((uint8_t *)&v, 2); }
        else if (x < (1ull << 21)) { uint32_t v = (x << 3) | 3;   ap((uint8_t *)&v, 3); }
        else if (x < (1ull << 28)) { uint32_t v = (x << 4) | 7;   ap((uint8_t *)&v, 4); }
        else if (x < (1ull << 35)) { uint64_t v = (x << 5) | 15;  ap((uint8_t *)&v, 5); }
        else if (x < (1ull << 42)) { uint64_t v = (x << 6) | 31;  ap((uint8_t *)&v, 6); }
        else if (x < (1ull << 49)) { uint64_t v = (x << 7) | 63;  ap((uint8_t *)&v, 7); }
        else if (x < (1ull << 56)) { uint64_t v = (x << 8) | 127; ap((uint8_t *)&v, 8); }
        else { push_back(255); ap((uint8_t *)&x, 8); }
    }

    void u32(uint32_t v) { ap((uint8_t *)&v, 4); }
    void u8(uint8_t v)   { push_back(v); }

    void str(const char *s) { while (*s) push_back(*s++); push_back(0); }
    void ap(const uint8_t *p, int n) { insert(end(), p, p + n); }
};


struct FE {
    std::string rel, dir, name;
    uint64_t    sz;
    uint32_t    mt, crc;
    bool        isdir;
    int         block;
    FileGroup   group;
};


static void collect(const std::string &root, std::vector<FE> &out) {
    for (auto &e : fs::recursive_directory_iterator(root)) {
        FE f;
        f.rel   = fs::relative(e.path(), root).string();
        f.isdir = e.is_directory();
        f.sz    = f.isdir ? 0 : e.file_size();

        auto s = f.rel.rfind('/');
        f.dir  = s != std::string::npos ? f.rel.substr(0, s) : "";
        f.name = s != std::string::npos ? f.rel.substr(s + 1) : f.rel;

        struct ::stat st;
        ::stat(e.path().c_str(), &st);
        f.mt    = (uint32_t)st.st_mtime;
        f.crc   = 0;
        f.block = -1;
        f.group = f.isdir ? GRP_BINARY : classifyExt(f.name);

        out.push_back(f);
    }

    std::sort(out.begin(), out.end(), [](const FE &a, const FE &b) {
        if (a.isdir != b.isdir) return a.isdir > b.isdir;
        if (a.group != b.group) return a.group < b.group;

        auto ea = a.name.rfind('.'), eb = b.name.rfind('.');
        std::string xa = ea != std::string::npos ? a.name.substr(ea) : "";
        std::string xb = eb != std::string::npos ? b.name.substr(eb) : "";
        if (xa != xb) return xa < xb;

        return a.rel < b.rel;
    });
}


struct BInfo {
    int         type;
    std::string comp;
    uint64_t    pos, orig, csz;
    uint32_t    crc;
};


static void writeDescr(FILE *f, const BInfo &b) {
    WBuf d;
    d.u32(ARC_SIG);
    d.vi(b.type);
    d.str(b.comp.c_str());
    d.vi(b.orig);
    d.vi(b.csz);
    d.u32(b.crc);
    d.u32(CalcCRC(d.data(), d.size()));

    fwrite(d.data(), 1, d.size(), f);
}


static int compressSmall(const char *m, const uint8_t *in, int sz, std::vector<uint8_t> &out) {
    out.resize(sz * 2 + 65536);
    int r = CompressMem((char *)m, (void *)in, sz, out.data(), out.size());
    if (r > 0) out.resize(r);

    return r;
}


struct CCtx {
    std::vector<FE> *files;
    int       startFile, endFile;
    std::string srcDir;
    int       curFile;
    FILE     *curFP;
    uint64_t  remain;
    uint32_t  crc;
    FILE     *outFP;
    uint64_t  written;
};


static int streamCb(const char *what, void *data, int size, void *aux) {
    auto *c = (CCtx *)aux;

    if (strcmp(what, "read") == 0) {
        int total = 0;
        auto *dst = (uint8_t *)data;

        while (total < size) {
            if (!c->curFP) {
                while (c->curFile < c->endFile) {
                    auto &f = (*c->files)[c->curFile];
                    if (f.isdir) { c->curFile++; continue; }

                    c->curFP  = fopen((c->srcDir + "/" + f.rel).c_str(), "rb");
                    c->remain = f.sz;
                    c->crc    = CRC_INIT;
                    break;
                }
                if (!c->curFP) break;
            }

            int want = std::min((uint64_t)(size - total), std::min(c->remain, (uint64_t)0x7FFFFFFF));
            int got  = fread(dst + total, 1, want, c->curFP);
            if (got <= 0) break;

            c->remain -= got;
            total     += got;
            c->crc     = UpdateCRC(dst + total - got, got, c->crc);

            if (c->remain == 0) {
                fclose(c->curFP);
                c->curFP = nullptr;
                (*c->files)[c->curFile].crc = c->crc ^ CRC_INIT;
                c->curFile++;
            }
        }

        return total;

    } else if (strcmp(what, "write") == 0) {
        int w = fwrite(data, 1, size, c->outFP);
        c->written += w;

        return w;
    }

    return FREEARC_ERRCODE_NOT_IMPLEMENTED;
}
