#pragma once

#include <string>
#include <functional>
#include <atomic>
#include <cstdint>
#include <vector>
#include <thread>


using ISDoneCallback = std::function<int(int overallPct, int currentPct,
                                         const char *currentFile)>;

class ISArcExtract {
public:
    static bool Extract(const std::string &inFile, const std::string &outPath,
                        double pctOfTotal, ISDoneCallback callback,
                        std::atomic<bool> *cancelled);
};


class IS7zipExtract {
public:
    static bool Extract(const std::string &inFile, const std::string &outPath,
                        ISDoneCallback callback, std::atomic<bool> *cancelled);
};


class ISRarExtract {
public:
    static bool Extract(const std::string &inFile, const std::string &outPath,
                        ISDoneCallback callback, std::atomic<bool> *cancelled);
};


class ISExtractor {
public:
    ~ISExtractor();

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

    // test mode
    int      FMockProgress = 0;
    int      FMockFile     = 0;
    uint32_t FLastTick     = 0;
};
