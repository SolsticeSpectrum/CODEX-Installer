#include "ISExtractor.h"
#include <SDL.h>
#include <filesystem>
#include <cstring>
#include <sys/stat.h>

// unarc
#undef  ON_CHECK_FAIL
#define ON_CHECK_FAIL()   UnarcQuit()
void UnarcQuit();
#include "ArcStructure.h"
#include "ArcCommand.h"
#include "ArcProcess.h"
void UnarcQuit() { CurrentProcess->quit(FREEARC_ERRCODE_GENERAL); }

// 7zip C decoder
extern "C" {
#include "7zip/C/7z.h"
#include "7zip/C/7zAlloc.h"
#include "7zip/C/7zBuf.h"
#include "7zip/C/7zCrc.h"
#include "7zip/C/7zFile.h"
}

// unrar DLL API
#ifndef _UNIX
#define _UNIX
#endif
#include "unrar/dll.hpp"

namespace fs = std::filesystem;


// ---------- ISArcExtract (FreeArc .bin) ----------

class ExtractUI : public BASEUI {
public:
    ISDoneCallback callback;
    std::atomic<bool> *cancelled;
    uint64 totalBytes   = 0;
    uint64 writtenTotal = 0;
    double pctOfTotal   = 100.0;
    char   outdir[MY_FILENAME_MAX * 4] = {};

    bool AllowProcessing(char cmd, int silent, MYFILENAME arcname,
                         char *comment, int cmtsize, FILENAME _outdir) override {
        strncpy(outdir, _outdir, sizeof(outdir) - 1);
        return true;
    }

    FILENAME GetOutDir() override { return outdir; }
    void BeginProgress(uint64 total) override { totalBytes = total; writtenTotal = 0; }

    bool ProgressFile(bool isdir, const char *operation,
                      MYFILENAME filename, uint64 filesize) override {
        if (cancelled && *cancelled) return false;
        if (callback) {
            int pct = totalBytes > 0 ? (int)(writtenTotal * pctOfTotal / totalBytes) : 0;
            callback(pct * 10, 0, filename);
        }
        return true;
    }

    bool ProgressWrite(uint64 wb) override { writtenTotal = wb; return !(cancelled && *cancelled); }
    bool ProgressRead(uint64 rb) override  { return !(cancelled && *cancelled); }
    char AskOverwrite(MYFILENAME, uint64, time_t) override { return 'y'; }
    void Abort(COMMAND*, int) override {}
};


bool ISArcExtract::Extract(const std::string &inFile, const std::string &outPath,
                           double pctOfTotal, ISDoneCallback callback,
                           std::atomic<bool> *cancelled) {
    std::string dpPath = "-dp" + outPath;
    char *argv[] = {(char*)"unarc", (char*)"x",
                    (char*)inFile.c_str(),
                    (char*)dpPath.c_str(),
                    (char*)"-o+",
                    (char*)"--noarcext",
                    nullptr};

    ExtractUI ui;
    ui.callback   = callback;
    ui.cancelled  = cancelled;
    ui.pctOfTotal = pctOfTotal;

    COMMAND command(6, argv);
    if (!command.ok) return false;

    PROCESS process(&command, &ui);
    return true;
}


// ---------- IS7zipExtract (.7z) ----------

static void *ISAlloc(ISzAllocPtr, size_t size)  { return size ? malloc(size) : nullptr; }
static void  ISFree(ISzAllocPtr, void *addr)    { free(addr); }
static const ISzAlloc g_ISAlloc = { ISAlloc, ISFree };

bool IS7zipExtract::Extract(const std::string &inFile, const std::string &outPath,
                            ISDoneCallback callback, std::atomic<bool> *cancelled) {
    CrcGenerateTable();

    CFileInStream archiveStream;
    CLookToRead2 lookStream;
    CSzArEx db;
    SRes res;

    if (InFile_Open(&archiveStream.file, inFile.c_str()))
        return false;

    FileInStream_CreateVTable(&archiveStream);
    LookToRead2_CreateVTable(&lookStream, False);

    static Byte lookBuf[1 << 18];
    lookStream.buf      = lookBuf;
    lookStream.bufSize  = sizeof(lookBuf);
    lookStream.realStream = &archiveStream.vt;
    LookToRead2_INIT(&lookStream)

    SzArEx_Init(&db);
    res = SzArEx_Open(&db, &lookStream.vt, &g_ISAlloc, &g_ISAlloc);
    if (res != SZ_OK) {
        File_Close(&archiveStream.file);
        return false;
    }

    UInt32 blockIndex = 0xFFFFFFFF;
    Byte *outBuffer = nullptr;
    size_t outBufferSize = 0;

    fs::create_directories(outPath);

    for (UInt32 i = 0; i < db.NumFiles; i++) {
        if (cancelled && *cancelled) break;

        size_t nameLen = SzArEx_GetFileNameUtf16(&db, i, nullptr);
        std::vector<UInt16> nameBuf(nameLen);
        SzArEx_GetFileNameUtf16(&db, i, nameBuf.data());

        // UTF-16 to UTF-8
        std::string name;
        for (size_t j = 0; j < nameLen - 1; j++)
            name += (char)nameBuf[j];

        bool isDir = SzArEx_IsDir(&db, i);
        std::string fullPath = outPath + "/" + name;

        if (isDir) {
            fs::create_directories(fullPath);
            continue;
        }

        auto parentDir = fullPath.substr(0, fullPath.rfind('/'));
        fs::create_directories(parentDir);

        size_t offset = 0, outSizeProcessed = 0;
        res = SzArEx_Extract(&db, &lookStream.vt, i,
            &blockIndex, &outBuffer, &outBufferSize,
            &offset, &outSizeProcessed, &g_ISAlloc, &g_ISAlloc);

        if (res != SZ_OK) break;

        FILE *f = fopen(fullPath.c_str(), "wb");
        if (f) {
            fwrite(outBuffer + offset, 1, outSizeProcessed, f);
            fclose(f);
        }

        if (callback) {
            int pct = db.NumFiles > 0 ? (int)(i * 1000 / db.NumFiles) : 0;
            if (callback(pct, 0, name.c_str())) break;
        }
    }

    ISzAlloc_Free(&g_ISAlloc, outBuffer);
    SzArEx_Free(&db, &g_ISAlloc);
    File_Close(&archiveStream.file);
    return res == SZ_OK;
}


// ---------- ISRarExtract (.rar) ----------

struct RarCallbackData {
    ISDoneCallback callback;
    std::atomic<bool> *cancelled;
    int fileIndex;
    int totalFiles;
};

static int CALLBACK RarCallback(UINT msg, LPARAM userData, LPARAM p1, LPARAM p2) {
    auto *data = (RarCallbackData *)userData;
    if (msg == UCM_PROCESSDATA) {
        if (data->cancelled && *data->cancelled) return -1;
    }
    return 0;
}

bool ISRarExtract::Extract(const std::string &inFile, const std::string &outPath,
                           ISDoneCallback callback, std::atomic<bool> *cancelled) {
    // first pass: count files
    RAROpenArchiveData arcData = {};
    arcData.ArcName  = (char *)inFile.c_str();
    arcData.OpenMode = RAR_OM_LIST;
    HANDLE hArc = RAROpenArchive(&arcData);
    if (!hArc || arcData.OpenResult != ERAR_SUCCESS) return false;

    int totalFiles = 0;
    RARHeaderData header = {};
    while (RARReadHeader(hArc, &header) == ERAR_SUCCESS) {
        totalFiles++;
        RARProcessFile(hArc, RAR_SKIP, nullptr, nullptr);
    }
    RARCloseArchive(hArc);

    // second pass: extract
    arcData.OpenMode = RAR_OM_EXTRACT;
    hArc = RAROpenArchive(&arcData);
    if (!hArc || arcData.OpenResult != ERAR_SUCCESS) return false;

    RarCallbackData cbData = {callback, cancelled, 0, totalFiles};
    RARSetCallback(hArc, RarCallback, (LPARAM)&cbData);

    fs::create_directories(outPath);

    int fileIndex = 0;
    while (RARReadHeader(hArc, &header) == ERAR_SUCCESS) {
        if (cancelled && *cancelled) break;

        int ret = RARProcessFile(hArc, RAR_EXTRACT, (char *)outPath.c_str(), nullptr);
        if (ret != ERAR_SUCCESS) break;

        fileIndex++;
        cbData.fileIndex = fileIndex;

        if (callback) {
            int pct = totalFiles > 0 ? (int)(fileIndex * 1000 / totalFiles) : 0;
            if (callback(pct, 0, header.FileName)) break;
        }
    }

    RARCloseArchive(hArc);
    return true;
}


// ---------- ISExtractor (orchestrator) ----------

void ISExtractor::SetSource(const std::string &sourceDir)  { FSourceDir = sourceDir; }
void ISExtractor::SetTarget(const std::string &installDir) { FTarget = installDir; }
void ISExtractor::SetToolsDir(const std::string &toolsDir) { FToolsDir = toolsDir; }
void ISExtractor::SetOnProgress(ProgressCallback cb) { FOnProgress = std::move(cb); }
void ISExtractor::SetOnFinish(FinishCallback cb)     { FOnFinish = std::move(cb); }


void ISExtractor::Start() {
    FRunning   = true;
    FPaused    = false;
    FCancelled = false;

    // count setup-N.bin (matches setup.iss ArcFileCount loop)
    FBinFiles.clear();
    FCurrentBin = 0;
    int idx = 1;
    while (true) {
        auto path = FSourceDir + "/setup-" + std::to_string(idx) + ".bin";
        if (!fs::exists(path)) break;
        FBinFiles.push_back(path);
        idx++;
    }

    if (FBinFiles.empty()) {
        FMockProgress = 0;
        FMockFile     = 0;
        FLastTick     = SDL_GetTicks();
        return;
    }

    FThread = std::thread([this]() {
        if (!FToolsDir.empty()) chdir(FToolsDir.c_str());

        int arcCount = (int)FBinFiles.size();
        bool error = false;

        for (int i = 0; i < arcCount && !FCancelled; i++) {
            FCurrentBin = i;
            double pctOfTotal = 100.0 / arcCount;

            bool ok = ISArcExtract::Extract(
                FBinFiles[i], FTarget, pctOfTotal,
                [this, i, arcCount](int overallPct, int currentPct, const char *file) -> int {
                    if (FCancelled) return 1;
                    while (FPaused && !FCancelled) SDL_Delay(50);
                    int totalPct = (i * 1000 / arcCount) + overallPct / arcCount;
                    if (FOnProgress) FOnProgress(totalPct, file);
                    return FCancelled ? 1 : 0;
                },
                &FCancelled
            );

            if (!ok) { error = true; break; }
        }

        FRunning = false;
        if (FOnFinish) FOnFinish(!error && !FCancelled);
    });
    FThread.detach();
}


void ISExtractor::Pause()  { FPaused = true; }
void ISExtractor::Resume() { FPaused = false; }

void ISExtractor::Cancel() {
    FCancelled = true;
    FRunning   = false;
    FPaused    = false;
}


void ISExtractor::Tick() {
    if (!FRunning || FPaused) return;

    // mockup when no .bin files
    if (FBinFiles.empty()) {
        uint32_t now = SDL_GetTicks();
        if (now - FLastTick < 50) return;
        FLastTick = now;

        FMockProgress += 5;
        if (FMockProgress % 10 == 0) {
            FMockFile++;
            if (FOnProgress)
                FOnProgress(FMockProgress, "Extracting file " + std::to_string(FMockFile) + " of 100...");
        }
        if (FMockProgress > 1000) {
            FRunning = false;
            if (FOnFinish) FOnFinish(true);
        }
    }
}
