#pragma once

#include <string>
#include <functional>
#include <atomic>
#include <cstdint>
#include <vector>
#include <thread>


// returns 0 to continue, nonzero to cancel
using ISDoneCallback = std::function<int(int overalPct, int currentPct,
                                         const char *currentFile)>;


// ISArcExtract — FreeArc .bin via unarc COMMAND/PROCESS
class ISArcExtract {
public:
    static bool Extract(const std::string &inFile, const std::string &outPath,
                        double pctOfTotal, ISDoneCallback callback,
                        std::atomic<bool> *cancelled);
};

// IS7zipExtract — .7z via official 7zip C decoder
class IS7zipExtract {
public:
    static bool Extract(const std::string &inFile, const std::string &outPath,
                        ISDoneCallback callback, std::atomic<bool> *cancelled);
};

// ISRarExtract — .rar via unrar DLL API
class ISRarExtract {
public:
    static bool Extract(const std::string &inFile, const std::string &outPath,
                        ISDoneCallback callback, std::atomic<bool> *cancelled);
};

// ISPrecompExtract, ISSrepExtract, ISxDeltaExtract are handled
// internally by unarc's EXTERNAL_METHOD via shell tools (srep, precomp, xdelta3).
// They don't need direct implementation — unarc calls them automatically
// when the .bin archive header specifies those compression methods.


// orchestrates extraction of setup-N.bin archives
class ISExtractor {
public:
    using ProgressCallback = std::function<void(int percent, const std::string &file)>;
    using FinishCallback   = std::function<void(bool success)>;

    void SetSource(const std::string &sourceDir);
    void SetTarget(const std::string &installDir);
    void SetToolsDir(const std::string &toolsDir);
    void SetOnProgress(ProgressCallback cb);
    void SetOnFinish(FinishCallback cb);

    void Start();
    void Pause();
    void Resume();
    void Cancel();

    void Tick();

    bool IsRunning() const { return FRunning; }
    bool IsPaused() const  { return FPaused; }

private:
    std::string FSourceDir;
    std::string FTarget;
    std::string FToolsDir;
    ProgressCallback FOnProgress;
    FinishCallback   FOnFinish;

    std::atomic<bool> FRunning   = false;
    std::atomic<bool> FPaused    = false;
    std::atomic<bool> FCancelled = false;

    std::vector<std::string> FBinFiles;
    int FCurrentBin = 0;
    std::thread FThread;

    // mockup fallback
    int      FMockProgress = 0;
    int      FMockFile     = 0;
    uint32_t FLastTick     = 0;
};
