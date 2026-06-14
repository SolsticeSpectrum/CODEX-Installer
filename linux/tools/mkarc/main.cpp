// mkarc - FreeArc-compatible archive creator
// usage: mkarc [-m<method>] [-s<solidMB>] [-v<volMB>] <srcdir> <output.bin>

#include "compress.h"
#include "multicompress.h"

extern "C" int AddExternalCompressor(char *params);


static std::string volumePath(const std::string &base, int vol) {
    auto dot = base.rfind('.');
    std::string stem = (dot != std::string::npos) ? base.substr(0, dot) : base;
    std::string ext  = (dot != std::string::npos) ? base.substr(dot) : ".bin";

    return stem + "-" + std::to_string(vol) + ext;
}


static void writeHeader(FILE *f, std::vector<BInfo> &ctrlBlocks) {
    WBuf h;
    h.u32(ARC_SIG);
    h.u32(ARC_VER);

    uint32_t c = CalcCRC(h.data(), h.size());
    uint64_t p = ftell(f);
    fwrite(h.data(), 1, h.size(), f);

    BInfo bi = {BT_HEADER, "storing", p, (uint64_t)h.size(), (uint64_t)h.size(), c};
    writeDescr(f, bi);
    ctrlBlocks.push_back(bi);
}


static void writeDirAndFooter(FILE *f,
                              const std::vector<BInfo> &ctrlBlocks,
                              const std::vector<BInfo> &dataBlocks,
                              const std::vector<FE> &files,
                              int blkStart, int blkEnd,
                              int fileStart, int fileEnd) {

    int nBlk = (int)dataBlocks.size();
    uint64_t dirPos = ftell(f);

    WBuf db;
    db.vi(nBlk);

    for (int b = blkStart; b < blkEnd; b++) {
        int cnt = 0;
        for (int i = fileStart; i < fileEnd; i++)
            if (files[i].block == b) cnt++;
        db.vi(cnt);
    }

    for (int d = 0; d < nBlk; d++) db.str(dataBlocks[d].comp.c_str());
    for (int d = 0; d < nBlk; d++) db.vi(dirPos - dataBlocks[d].pos);
    for (int d = 0; d < nBlk; d++) db.vi(dataBlocks[d].csz);

    // dirs used by files in this volume
    std::vector<std::string> volDirs;
    for (int i = fileStart; i < fileEnd; i++) {
        auto &d = files[i].dir;
        if (std::find(volDirs.begin(), volDirs.end(), d) == volDirs.end())
            volDirs.push_back(d);
    }

    db.vi((int)volDirs.size());
    for (auto &d : volDirs) db.str(d.c_str());

    for (int i = fileStart; i < fileEnd; i++) db.str(files[i].name.c_str());
    for (int i = fileStart; i < fileEnd; i++) {
        int dn = std::find(volDirs.begin(), volDirs.end(), files[i].dir) - volDirs.begin();
        db.vi(dn);
    }
    for (int i = fileStart; i < fileEnd; i++) db.vi(files[i].sz);
    for (int i = fileStart; i < fileEnd; i++) db.u32(files[i].mt);
    for (int i = fileStart; i < fileEnd; i++) db.u8(files[i].isdir ? 1 : 0);
    for (int i = fileStart; i < fileEnd; i++) db.u32(files[i].crc);

    uint32_t dirOrig = db.size();
    uint32_t dirCrc  = CalcCRC(db.data(), dirOrig);
    fwrite(db.data(), 1, dirOrig, f);

    std::vector<BInfo> allCtrl = ctrlBlocks;
    BInfo dirBi = {BT_DIR, "storing", dirPos, (uint64_t)dirOrig, (uint64_t)dirOrig, dirCrc};
    writeDescr(f, dirBi);
    allCtrl.push_back(dirBi);

    // footer
    uint64_t footerPos = ftell(f);

    WBuf fb;
    fb.vi((int)allCtrl.size());
    for (auto &cb : allCtrl) {
        fb.vi(cb.type);
        fb.str(cb.comp.c_str());
        fb.vi(footerPos - cb.pos);
        fb.vi(cb.orig);
        fb.vi(cb.csz);
        fb.u32(cb.crc);
    }
    fb.u8(0);
    fb.vi(0);

    uint32_t fOrig = fb.size();
    uint32_t fCrc  = CalcCRC(fb.data(), fOrig);
    fwrite(fb.data(), 1, fOrig, f);

    BInfo footBi = {BT_FOOTER, "storing", footerPos, (uint64_t)fOrig, (uint64_t)fOrig, fCrc};
    writeDescr(f, footBi);
}


int main(int argc, char *argv[]) {
    const char *method  = "4x4:lzma:fast";
    const char *srcDir  = nullptr;
    const char *outPath = nullptr;
    int solidMB  = 128;
    int volumeMB = 4700;

    for (int i = 1; i < argc; i++) {
        if (argv[i][0] != '-') {
            if (!srcDir) srcDir = argv[i]; else outPath = argv[i];
        } else switch (argv[i][1]) {
            case 'm': method   = argv[i] + 2; break;
            case 's': solidMB  = atoi(argv[i] + 2); break;
            case 'v': volumeMB = atoi(argv[i] + 2); break;
            default: fprintf(stderr, "unknown: %s\n", argv[i]); return 1;
        }
    }

    if (!srcDir || !outPath) {
        fprintf(stderr, "usage: mkarc [-m<method>] [-s<solidMB>] [-v<volMB>] <srcdir> <out.bin>\n");
        return 1;
    }

    InitCRC();
    SetCompressionThreads(1);

    // resolve tool paths relative to mkarc binary
    std::string binDir = fs::canonical(fs::path(argv[0])).parent_path().string();

    std::string precompDef =
        "[External compressor:precomp]\n"
        "datafile = arcdata.tmp\n"
        "packedfile = arcdata.pcf\n"
        "packcmd = " + binDir + "/precomp -cn arcdata.tmp\n"
        "unpackcmd = " + binDir + "/precomp -r arcdata.pcf\n";
    std::string srepDef =
        "[External compressor:srep]\n"
        "datafile = arcdata.tmp\n"
        "packedfile = arcpacked.tmp\n"
        "packcmd = " + binDir + "/srep arcdata.tmp arcpacked.tmp\n"
        "unpackcmd = " + binDir + "/srep -d arcpacked.tmp arcdata.tmp\n";
    AddExternalCompressor(strdup(precompDef.c_str()));
    AddExternalCompressor(strdup(srepDef.c_str()));

    std::vector<FE> files;
    collect(srcDir, files);

    uint64_t totalOrig = 0;
    int nFiles = 0;
    for (auto &f : files)
        if (!f.isdir) { totalOrig += f.sz; nFiles++; }

    // split into solid blocks on group boundary or size limit
    uint64_t solidLimit = (uint64_t)solidMB * 1024 * 1024;
    int nBlocks = 0;
    {
        uint64_t blockSz = 0;
        FileGroup curGrp = GRP_BINARY;
        bool first = true;

        for (auto &f : files) {
            if (f.isdir) { f.block = nBlocks; continue; }

            if (!first && (f.group != curGrp || (blockSz > 0 && blockSz + f.sz > solidLimit))) {
                nBlocks++;
                blockSz = 0;
            }

            f.block = nBlocks;
            curGrp  = f.group;
            blockSz += f.sz;
            first    = false;
        }
        nBlocks++;
    }

    // stats
    int grpCount[GRP_COUNT] = {};
    uint64_t grpSize[GRP_COUNT] = {};
    for (auto &f : files)
        if (!f.isdir) { grpCount[f.group]++; grpSize[f.group] += f.sz; }

    printf("%d files, %llu bytes, %d solid blocks, method: %s\n",
           nFiles, (unsigned long long)totalOrig, nBlocks, method);

    for (int g = 0; g < GRP_COUNT; g++)
        if (grpCount[g] > 0)
            printf("  %-12s: %3d files, %7.1f MB\n", grpName[g], grpCount[g], grpSize[g] / 1048576.0);

    // unique dirs
    std::vector<std::string> dirs;
    for (auto &f : files)
        if (std::find(dirs.begin(), dirs.end(), f.dir) == dirs.end())
            dirs.push_back(f.dir);

    // block metadata
    struct BlkInfo {
        int       startF, endF;
        uint64_t  origSz;
        FileGroup grp;
        std::string method;
    };

    std::vector<BlkInfo> blkInfos(nBlocks);
    for (int blk = 0; blk < nBlocks; blk++) {
        auto &bi = blkInfos[blk];
        bi.startF = -1;
        bi.endF   = -1;
        bi.origSz = 0;
        bi.grp    = GRP_BINARY;

        for (int i = 0; i < (int)files.size(); i++) {
            if (files[i].block == blk) {
                if (bi.startF < 0) bi.startF = i;
                bi.endF = i + 1;
            }
        }

        for (int i = bi.startF; i < bi.endF; i++) {
            if (!files[i].isdir) {
                bi.origSz += files[i].sz;
                if (bi.grp == GRP_BINARY) bi.grp = files[i].group;
            }
        }

        bi.method = methodForGroup(method, bi.grp);
    }

    // volume splitting
    uint64_t volumeLimit = volumeMB ? (uint64_t)volumeMB * 1024 * 1024 : 0;
    bool splitting = volumeLimit > 0;

    int volNum = 1;
    std::string curVolPath = splitting ? volumePath(outPath, volNum) : outPath;
    FILE *out = fopen(curVolPath.c_str(), "wb");
    if (!out) { fprintf(stderr, "can't create: %s\n", curVolPath.c_str()); return 1; }

    std::vector<BInfo> ctrlBlocks;
    std::vector<BInfo> dataBlocks;
    int volBlkStart  = 0;
    int volFileStart = 0;

    writeHeader(out, ctrlBlocks);

    // 4x4 or plain method
    bool use4x4 = (strncmp(method, "4x4", 3) == 0);
    std::string baseMethod;
    int subBlockSize = 8 * 1024 * 1024;
    int nWorkers = std::thread::hardware_concurrency();
    if (nWorkers <= 0) nWorkers = 4;

    if (use4x4) {
        COMPRESSION_METHOD *parsed = ParseCompressionMethod((char *)method);
        if (parsed) {
            char buf[256];
            parsed->ShowCompressionMethod(buf);
            char *colon = strchr(buf, ':');
            baseMethod = colon ? colon + 1 : "lzma:fast";
            subBlockSize = parsed->GetBlockSize();
            if (subBlockSize <= 0) subBlockSize = 8 * 1024 * 1024;
            delete parsed;
        } else {
            baseMethod = "lzma:fast";
        }
    }

    if (use4x4) {
        std::mutex mtx;
        std::condition_variable cvWork, cvWrite, cvFree;
        int poolSize = nWorkers * 2 + 2;
        std::vector<SubBlock> pool(poolSize);
        std::queue<SubBlock *> freeQ, workQ;
        std::vector<SubBlock *> pending;
        std::atomic<bool> allRead{false}, writerDone{false};
        for (auto &sb : pool) freeQ.push(&sb);

        bool useLzma = (strncmp(baseMethod.c_str(), "lzma", 4) == 0);
        COMPRESSION_METHOD *baseCm = nullptr;
        if (useLzma) baseCm = ParseCompressionMethod((char *)baseMethod.c_str());

        // writer
        int curWriteBlk = 0, nextSeqInBlk = 0;
        std::thread writerThread([&]() {

            uint64_t blockDataStart = ftell(out);

            while (true) {
                std::vector<SubBlock *> local;
                {
                    std::unique_lock<std::mutex> lk(mtx);
                    cvWrite.wait(lk, [&]{ return !pending.empty() || writerDone; });
                    if (pending.empty() && writerDone) break;
                    local.swap(pending);
                }

                std::sort(local.begin(), local.end(), [](SubBlock *a, SubBlock *b) {
                    return a->blkId != b->blkId ? a->blkId < b->blkId : a->seq < b->seq;
                });

                std::vector<SubBlock *> deferred;
                for (auto *sb : local) {
                    if (sb->blkId != curWriteBlk || sb->seq != nextSeqInBlk) {
                        deferred.push_back(sb);
                        continue;
                    }

                    uint32_t origSz = sb->in.size();
                    uint32_t compSz = sb->method.size() + 1 + sb->out.size();
                    fwrite(&origSz, 4, 1, out);
                    fwrite(&compSz, 4, 1, out);
                    fwrite(sb->method.c_str(), sb->method.size() + 1, 1, out);
                    fwrite(sb->out.data(), sb->out.size(), 1, out);
                    nextSeqInBlk++;

                    bool wasLast = sb->lastInBlock;
                    sb->in.clear();
                    sb->out.clear();
                    {
                        std::lock_guard<std::mutex> lk2(mtx);
                        freeQ.push(sb);
                    }
                    cvFree.notify_one();

                    if (wasLast) {
                        uint64_t dataEnd = ftell(out);
                        auto &bi = blkInfos[curWriteBlk];
                        BInfo binfo = {BT_DATA, std::string("4x4:") + baseMethod,
                                       blockDataStart, bi.origSz, dataEnd - blockDataStart, 0};
                        writeDescr(out, binfo);
                        dataBlocks.push_back(binfo);

                        curWriteBlk++;
                        nextSeqInBlk   = 0;
                        blockDataStart = ftell(out);

                        if (splitting && (uint64_t)ftell(out) >= volumeLimit && curWriteBlk < nBlocks) {
                            int volFileEnd = blkInfos[curWriteBlk].startF;
                            writeDirAndFooter(out, ctrlBlocks, dataBlocks, files,
                                              volBlkStart, curWriteBlk, volFileStart, volFileEnd);
                            fclose(out);
                            printf("  volume %d: %s\n", volNum, curVolPath.c_str());

                            volNum++;
                            curVolPath = volumePath(outPath, volNum);
                            out = fopen(curVolPath.c_str(), "wb");
                            ctrlBlocks.clear();
                            dataBlocks.clear();
                            volBlkStart  = curWriteBlk;
                            volFileStart = volFileEnd;
                            writeHeader(out, ctrlBlocks);
                            blockDataStart = ftell(out);
                        }
                    }
                }

                if (!deferred.empty()) {
                    std::lock_guard<std::mutex> lk(mtx);
                    pending.insert(pending.end(), deferred.begin(), deferred.end());
                }
            }
        });

        // workers
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

                if (sb->method == "storing" || isIncompressible(sb->in.data(), sb->in.size())) {
                    storeRaw(sb);

                } else if (useLzma && lctx.initialized) {
                    JobInStream inS   = { jobStreamRead, sb->in.data(), (int)sb->in.size() };
                    JobOutStream outS = { jobStreamWrite, &sb->out, 0 };
                    sb->out.resize(sb->in.size() + sb->in.size() / 8 + 65536);

                    SRes res = LzmaEnc_Encode(lctx.enc,
                        (ISeqOutStream *)&outS, (ISeqInStream *)&inS,
                        nullptr, &g_Alloc4x4, &g_Alloc4x4);

                    if (res != SZ_OK || outS.wp >= (int)sb->in.size())
                        storeRaw(sb);
                    else
                        sb->out.resize(outS.wp);

                } else {
                    Job4x4 j4;
                    j4.method = sb->method;
                    j4.rp     = sb->in.data();
                    j4.rl     = sb->in.size();
                    j4.wp     = 0;
                    j4.out.resize(sb->in.size() + sb->in.size() / 8 + 65536);
                    j4.in = sb->in;

                    int r = Compress((char *)j4.method.c_str(), job4x4Cb, &j4);

                    if (r < 0 || j4.wp >= (int)sb->in.size()) {
                        storeRaw(sb);
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

        // reader
        std::vector<uint8_t> readBuf(subBlockSize);
        for (int blk = 0; blk < nBlocks; blk++) {
            auto &bi = blkInfos[blk];
            if (bi.startF < 0 || bi.origSz == 0) continue;

            printf("block %d/%d [%s]: %llu bytes (%s), files %d-%d\n",
                   blk + 1, nBlocks, grpName[bi.grp], (unsigned long long)bi.origSz,
                   bi.method.c_str(), bi.startF, bi.endF - 1);

            CCtx ctx = {};
            ctx.files     = &files;
            ctx.startFile = bi.startF;
            ctx.endFile   = bi.endF;
            ctx.curFile   = bi.startF;
            ctx.srcDir    = srcDir;
            ctx.curFP     = nullptr;

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
                    int r = streamCb("read", readBuf.data() + total, wantBytes - total, &ctx);
                    if (r <= 0) break;
                    total += r;
                }
                remaining -= total;

                sb->blkId       = blk;
                sb->seq         = seq++;
                sb->in.assign(readBuf.data(), readBuf.data() + total);
                sb->method      = subMethod;
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
        for (int blk = 0; blk < nBlocks; blk++) {
            auto &bi = blkInfos[blk];
            if (bi.startF < 0) continue;

            printf("block %d/%d [%s]: %llu bytes (%s), files %d-%d\n",
                   blk + 1, nBlocks, grpName[bi.grp], (unsigned long long)bi.origSz,
                   bi.method.c_str(), bi.startF, bi.endF - 1);

            uint64_t dataPos = ftell(out);

            CCtx ctx      = {};
            ctx.files     = &files;
            ctx.startFile = bi.startF;
            ctx.endFile   = bi.endF;
            ctx.curFile   = bi.startF;
            ctx.srcDir    = srcDir;
            ctx.curFP     = nullptr;
            ctx.outFP     = out;

            const char *m = bi.method.c_str();
            int result;
            if (strchr(m, '+'))
                result = MultiCompress((char *)m, streamCb, &ctx);
            else
                result = Compress((char *)m, streamCb, &ctx);

            if (result < 0) {
                fprintf(stderr, "compression failed: %d\n", result);
                fclose(out);
                return 1;
            }

            uint64_t dataEnd = ftell(out);
            BInfo binfo = {BT_DATA, bi.method, dataPos, bi.origSz, dataEnd - dataPos, 0};
            writeDescr(out, binfo);
            dataBlocks.push_back(binfo);

            if (splitting && (uint64_t)ftell(out) >= volumeLimit && blk + 1 < nBlocks) {
                int nextBlk    = blk + 1;
                int volFileEnd = blkInfos[nextBlk].startF;

                writeDirAndFooter(out, ctrlBlocks, dataBlocks, files,
                                  volBlkStart, nextBlk, volFileStart, volFileEnd);
                fclose(out);
                printf("  volume %d: %s\n", volNum, curVolPath.c_str());

                volNum++;
                curVolPath = volumePath(outPath, volNum);
                out = fopen(curVolPath.c_str(), "wb");
                ctrlBlocks.clear();
                dataBlocks.clear();
                volBlkStart  = nextBlk;
                volFileStart = volFileEnd;
                writeHeader(out, ctrlBlocks);
            }
        }
    }

    // finalize last volume
    writeDirAndFooter(out, ctrlBlocks, dataBlocks, files,
                      volBlkStart, nBlocks, volFileStart, (int)files.size());

    fclose(out);
    if (splitting) printf("  volume %d: %s\n", volNum, curVolPath.c_str());

    uint64_t totalComp = 0;
    for (int v = 1; v <= volNum; v++) {
        std::string p = splitting ? volumePath(outPath, v) : outPath;
        totalComp += fs::file_size(p);
    }

    printf("done: %llu -> %llu bytes (%.1f%%)%s\n",
           (unsigned long long)totalOrig, (unsigned long long)totalComp,
           totalOrig > 0 ? 100.0 * totalComp / totalOrig : 0.0,
           splitting ? (std::string(", ") + std::to_string(volNum) + " volumes").c_str() : "");

    return 0;
}
